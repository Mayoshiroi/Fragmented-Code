#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#define DEFAULT_PORT 7000

uv_loop_t *loop;

void on_connect (uv_connect_t* req, int status){
  if (status < 0) {
      fprintf(stderr, "connection error %s\n", uv_strerror(status));
      // error!
      return;
  }

  char* sendData = "bitches";
  uv_write_t _req;

  char buffer[100];
  uv_buf_t buf = uv_buf_init(buffer, sizeof(buffer));
  buf.len = strlen(sendData);
  buf.base = sendData;

  uv_write(&_req,req->handle,&buf,1,NULL);
}

int main(){
  loop = uv_default_loop();

  uv_tcp_t client;
  uv_connect_t connect;

  uv_tcp_init(loop,&client);
  struct sockaddr_in server;

  uv_ip4_addr("127.0.0.1",DEFAULT_PORT,&server);
  uv_tcp_connect(&connect,&client,(const struct sockaddr*)&server, on_connect);

  return uv_run(loop, UV_RUN_DEFAULT);
}
