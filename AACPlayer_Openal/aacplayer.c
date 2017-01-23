#include <memory.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stddef.h>

#include <neaacdec.h>

#include <AL/al.h>
#include <AL/alc.h>

#define MAX_CHANNEL 6

typedef struct {
	FILE* fptr;
	uint32_t fileSize;
	unsigned char* buffer;
	uint32_t bufferSize;
	uint16_t tagSize;
	uint8_t type;
} AAC_File_struct;

typedef struct{
	unsigned char* framePtr;
	int32_t frameLength;
	uint32_t offset;
} AAC_Frame_Buffer_struct;

static int adts_sample_rates[] = {96000,88200,64000,48000,44100,32000,24000,22050,16000,12000,11025,8000,7350,0,0,0};
float getADTSFramePerSec(unsigned char* buffer){
	float samplerate = adts_sample_rates[(buffer[2]&0x3c)>>2];
	return (float)samplerate/1024.0f;
}

int updateBufferByOffset(AAC_File_struct* aacfile,int32_t offset,uint8_t where){
	fseek(aacfile->fptr,offset,where);
	memset(aacfile->buffer,1,aacfile->bufferSize);
	return fread(aacfile->buffer,1,aacfile->bufferSize,aacfile->fptr);
}

int openAACFile(char* filepath,AAC_File_struct* aacfile){
	FILE* file = fopen(filepath,"rb");
	if(!file)
		return -1;
	
	fseek(file,0,SEEK_END);
	aacfile->fileSize = ftell(file);
	fseek(file,0,SEEK_SET);
	
	aacfile->fptr = file;
	if(!(aacfile->buffer = (unsigned char*)malloc(FAAD_MIN_STREAMSIZE*MAX_CHANNEL)))
		return -2;//no enough memory
	
	aacfile->bufferSize = FAAD_MIN_STREAMSIZE*MAX_CHANNEL;
	memset(aacfile->buffer,0,aacfile->bufferSize);
	int readsize = fread(aacfile->buffer,1,aacfile->bufferSize,file);
	if(readsize <=0 )
		return -1;
	
	if(!memcmp(aacfile->buffer,"ID3",3)){
		aacfile->tagSize = (aacfile->buffer[6] << 21 | (aacfile->buffer[7] << 14) | (aacfile->buffer[8] << 7) | (aacfile->buffer[9] << 0));
		aacfile->tagSize += 10;
	}
	
	updateBufferByOffset(aacfile,aacfile->tagSize,SEEK_SET);
	
	if((aacfile->buffer[0] == 0xFF) && ((aacfile->buffer[1] & 0xF6) == 0xF0)){
		aacfile->type = 1;
	}else if(memcmp(aacfile->buffer,"ADIF",4) == 0){
		aacfile->type = 2;
	}
	
	return 1;
}

int32_t getADTSFrameLength(unsigned char* framebuffer){
	if (!((framebuffer[0] == 0xFF)&&((framebuffer[1] & 0xF6) == 0xF0)))
		return -1;
	return ((((uint16_t)framebuffer[3] & 0x3)) << 11 ) | ((uint16_t)framebuffer[4] << 3) | (framebuffer[5] >> 5);
}

int32_t getNextADTSFrame(AAC_File_struct* aacfile,AAC_Frame_Buffer_struct* aacframebuffer){
	if((aacframebuffer->frameLength == 0) && (aacframebuffer->framePtr == NULL)){
		aacframebuffer->framePtr = aacfile->buffer;
		aacframebuffer->frameLength = getADTSFrameLength(aacfile->buffer);
		aacframebuffer->offset = 0;
	}else{
		uint32_t spaceleft = aacfile->bufferSize - aacframebuffer->frameLength - aacframebuffer->offset;
		int32_t framelength = -1;
		
		if(spaceleft > 7){
			framelength = getADTSFrameLength(aacframebuffer->framePtr + aacframebuffer->frameLength);
		}else{
			if(updateBufferByOffset(aacfile,-spaceleft,SEEK_CUR) <= 0)
				return 0;//eof
			aacframebuffer->offset = 0;
			aacframebuffer->framePtr = aacfile->buffer;
			aacframebuffer->frameLength = 0;
			spaceleft = aacfile->bufferSize;
			framelength = getADTSFrameLength(aacfile->buffer);
		}
		
		if(framelength < 0)
			return -1;
		
		if(spaceleft < framelength){
			if(updateBufferByOffset(aacfile,-spaceleft,SEEK_CUR) <= 0)
				return 0;
			aacframebuffer->offset = 0;
			aacframebuffer->framePtr = aacfile->buffer;
		}else{
			aacframebuffer->framePtr += aacframebuffer->frameLength;
			aacframebuffer->offset += aacframebuffer->frameLength;
		}
		aacframebuffer->frameLength = framelength;
	}
	return 1;
}

int main(int argc,char* argv[]){
	AAC_File_struct aacfile;
	AAC_Frame_Buffer_struct aacframebuffer = {.framePtr = NULL,.frameLength = 0,.offset = 0};
	memset(&aacfile,0,sizeof(aacfile));
	openAACFile("sample.aac",&aacfile);
	
	NeAACDecHandle hDecoder = NeAACDecOpen();
	NeAACDecConfigurationPtr config;
	NeAACDecFrameInfo frameInfo;
	unsigned long samplerate;
	uint8_t channels;
	
	NeAACDecInit(hDecoder,aacfile.buffer,aacfile.bufferSize,&samplerate,&channels);
	config = NeAACDecGetCurrentConfiguration(hDecoder);
	NeAACDecSetConfiguration(hDecoder,config);
	
	printf("There %.3f frame/sec \n",getADTSFramePerSec(aacfile.buffer));
	
	ALCdevice *device;
	ALCcontext *context = NULL;
	ALuint source,buffer;
	ALint state,num;
	device = alcOpenDevice(NULL);
	if (device) {
		context = alcCreateContext(device, NULL);
		alcMakeContextCurrent(context);
	}
	alGenSources(1, &source);
	ALenum format = AL_FORMAT_STEREO16;
	while(getNextADTSFrame(&aacfile,&aacframebuffer) > 0){
		unsigned char* sample_buffer = NeAACDecDecode(hDecoder,&frameInfo,aacframebuffer.framePtr,aacframebuffer.frameLength);
		alGetSourcei(source, AL_BUFFERS_QUEUED, &num);
		if(num < 32){
			alGenBuffers(1, &buffer);
		}else{
			alGetSourcei(source,AL_SOURCE_STATE,&state);
			if (state != AL_PLAYING){
				alSourcePlay(source);
			}
			while (1) {
				alGetSourcei(source, AL_BUFFERS_PROCESSED, &num);
				if(num != 0)
					break;
			}
			alSourceUnqueueBuffers(source, 1, &buffer);
		}

		alBufferData(buffer,format,sample_buffer,frameInfo.samples * frameInfo.channels,frameInfo.samplerate);
		alSourceQueueBuffers(source,1,&buffer);
		if(frameInfo.error > 0){
			printf("ERROR: %s \n",NeAACDecGetErrorMessage(frameInfo.error));
		}
	}
	
	while(alGetSourcei(source,AL_SOURCE_STATE,&state),state == AL_PLAYING){}

	alDeleteSources(1,&source);
	alcMakeContextCurrent(NULL);
	alcDestroyContext(context);
	alcCloseDevice(device);
	
	return 1;
}
