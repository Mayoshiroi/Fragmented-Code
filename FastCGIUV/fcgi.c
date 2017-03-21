#include "fcgi.h"

int8_t ReadFCGIHeader(FCGI_Header* header,char* buffer){
	memcpy(header,buffer,sizeof(FCGI_Header));
	if(header->version != FCGI_VERSION_1)
		return -1;
	if((header->type < FCGI_BEGIN_REQUEST) | (header->type > FCGI_MAXTYPE))
		return -1;
	
#if FCGI_DEBUG
	printf("CGI VERSION:-> %d TYPE:-> %d ID:->%d Size:->%d Panding:->%d \n",
			header->version,header->type,(header->requestIdB1 << 8) + header->requestIdB0,(header->contentLengthB1 << 8) + header->contentLengthB0,header->paddingLength);
#endif
	
	return 1;
}

int8_t ReadFCGIBeginRecord(FCGI_BeginRequestRecord* beginRecord,void* buffer){
	FCGI_Header fcgiheader;
	if(ReadFCGIHeader(&fcgiheader,buffer) < 0)
		return -1;
		
	if(fcgiheader.type != FCGI_BEGIN_REQUEST)
		return -1;
		
	memcpy(beginRecord,buffer,sizeof(FCGI_BeginRequestRecord));
	
	return 1;
}
