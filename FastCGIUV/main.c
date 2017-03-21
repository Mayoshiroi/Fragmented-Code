#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>
#include <gc.h>
#include "fcgi.h"

#define DEFAULT_BACKLOG 128
#define FCGI_DEBUG 1

#if FCGI_DEBUG
	#define D_PRINT(FMT, ...) printf(FMT, __VA_ARGS__)
#else
	#define D_PRINT(FMT, ...)
#endif // FCGI_DEBUG

uv_loop_t* loop;
struct sockaddr_in addr;

typedef struct {
	uv_write_t req;
	uv_stream_t* client;
} uv_write_ex;

void end_request(uv_write_t *req,int status){
	if(!uv_is_closing((uv_handle_t*) req->handle)){
		printf("Call CLOSE! \n");
		uv_close((uv_handle_t*) req->handle,NULL);
	}
}

void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf){
	buf->base = (char*) GC_MALLOC(suggested_size);
	buf->len = suggested_size;
}

void fcgi_beginRequest(void* buffer,uv_stream_t* client){
	char* next_header = buffer;
	FCGI_BeginRequestRecord beginRecord;
	if(ReadFCGIBeginRecord(&beginRecord,buffer) < 0){}
		//return;
	
	FCGI_Header fcgiHeader;
	if(ReadFCGIHeader(&fcgiHeader,(void*)((char*)(buffer) + 16)) < 0){}
		//return;
	
	if(fcgiHeader.type == FCGI_PARAMS){
		uint16_t ParamLength = (fcgiHeader.contentLengthB1 << 8) + fcgiHeader.contentLengthB0;
		next_header = (char*)buffer + 16 + ParamLength + fcgiHeader.paddingLength + 8;
		ReadFCGIHeader(&fcgiHeader,(void*)next_header);
		next_header +=8;
	}
	
	ReadFCGIHeader(&fcgiHeader,next_header);
	
	FCGI_Header requestHeader;
	memset(&requestHeader,0,sizeof(FCGI_Header));
	
	requestHeader.version = FCGI_VERSION_1;
	requestHeader.type = FCGI_STDOUT;
	requestHeader.requestIdB0 = fcgiHeader.requestIdB0;
	requestHeader.requestIdB1 = fcgiHeader.requestIdB1;
	
	char* request = "Content-type: text/html\r\n\r\n<html>Hello World From FCGISERVER</html>     ";
	uint16_t pLength = strlen(request) % 8;
	
	requestHeader.contentLengthB1 = 0;
	requestHeader.contentLengthB0 = strlen(request);
	
	requestHeader.paddingLength = 0;
	
	uv_write_t* req = (uv_write_t*)GC_MALLOC(sizeof(uv_write_t));
	memset(req,0,sizeof(uv_write_t));
	uv_buf_t writebuf = uv_buf_init((char*)&requestHeader,sizeof(FCGI_Header));
	uv_write(req,client,&writebuf,1,NULL);
	
	req = (uv_write_t*)GC_MALLOC(sizeof(uv_write_t));
	memset(req,0,sizeof(uv_write_t));
	writebuf = uv_buf_init(request,strlen(request));
	uv_write(req,client,&writebuf,1,NULL);
	
	req = (uv_write_t*)GC_MALLOC(sizeof(uv_write_t));
	memset(&requestHeader,0,sizeof(FCGI_Header));
	requestHeader.version = FCGI_VERSION_1;
	requestHeader.type = FCGI_STDOUT;
	requestHeader.requestIdB0 = fcgiHeader.requestIdB0;
	requestHeader.requestIdB1 = fcgiHeader.requestIdB1;
	writebuf = uv_buf_init((char*)&requestHeader,sizeof(FCGI_Header));
	memset(req,0,sizeof(uv_write_t));
	uv_write(req,client,&writebuf,1,NULL);
	
	FCGI_EndRequestRecord endRecord;
	memset(&endRecord,0,sizeof(FCGI_EndRequestRecord));
	endRecord.header.version = FCGI_VERSION_1;
	endRecord.header.type = FCGI_END_REQUEST;
	endRecord.header.contentLengthB0 = 8;
	endRecord.header.requestIdB0 = fcgiHeader.requestIdB0;
	endRecord.header.requestIdB1 = fcgiHeader.requestIdB1;
	endRecord.body.protocolStatus = FCGI_REQUEST_COMPLETE;
	writebuf = uv_buf_init((char*)(&endRecord),sizeof(FCGI_EndRequestRecord));
	req = (uv_write_t*)GC_MALLOC(sizeof(uv_write_t));
	memset(req,0,sizeof(uv_write_t));
	uv_write(req,client,&writebuf,1,end_request);
	
}

void fcgi_connection(uv_stream_t* client, ssize_t nread,const uv_buf_t* buf){
	D_PRINT("NREAD:-> %ld \n",nread);
	if(nread > 0){
		fcgi_beginRequest((void*) (buf->base),client);
		return;
	}else if (nread < 0){
		if (nread != UV_EOF)
			fprintf(stderr, "Read error %s\n", uv_err_name(nread));
		uv_close((uv_handle_t*) client, NULL);
	}
	
	GC_FREE(buf->base);
}

void new_connection(uv_stream_t* server,int status){
	printf("NEW CONNTCT! \n");
	if(status < 0){
		fprintf(stderr, "New connection error %s\n", uv_strerror(status));
	}
	
	
	uv_tcp_t* client = (uv_tcp_t*) GC_MALLOC(sizeof(uv_tcp_t));
	uv_tcp_init(loop,client);
	
	if (uv_accept(server,(uv_stream_t*)client) == 0){
		uv_read_start((uv_stream_t*) client,alloc_buffer,fcgi_connection);
	}else{
		uv_close((uv_handle_t*)client,NULL);
	}
	
}

int main(int argc,char** argv){
	GC_init();
	
	loop = uv_default_loop();
	
	uv_tcp_t server;
	uv_tcp_init(loop,&server);
	
	uv_ip4_addr("0.0.0.0",7000,&addr);
	
	uv_tcp_bind(&server,(const struct sockaddr*)&addr,0);
	
	int result = uv_listen((uv_stream_t*)&server,DEFAULT_BACKLOG,new_connection);
	
	if(result){
		fprintf(stderr, "Listen error %s\n", uv_strerror(result));
		return 1;
    }
    
    return uv_run(loop, UV_RUN_DEFAULT);
}
