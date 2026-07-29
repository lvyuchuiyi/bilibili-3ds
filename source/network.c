#include <3ds.h> 
#include "network.h" 
static u32*m=NULL;static bool s=0; 
int net_init(void){if(s)return 0;m=(u32*)linearAlloc(0x100000);if(!m)return -1; 
if(R_FAILED(socInit(m,0x100000))){linearFree(m);m=NULL;return -2;}s=1;return 0;} 
void net_exit(void){if(s){socExit();s=0;}if(m){linearFree(m);m=NULL;}} 
int http_get(const char*u,http_response_t*res){(void)u;(void)res;return -99;} 
void http_response_free(http_response_t*res){(void)res;} 
