#include <stdio.h>
#include <uv.h>

int64_t counter = 0;

void wait_for_a_while(uv_idle_t* handle,int status){
  counter++;
  if(counter >= 10e6)
    uv_idle_stop(handle);
}

int main(){
  uv_idle_t idler;

  uv_idle_init(uv_default_loop(),&idler);
  uv_idle_start(&idler,wait_for_a_while);

  printf("%s\n","Idling...." );

  uv_run(uv_default_loop(),UV_RUN_DEFAULT);
}