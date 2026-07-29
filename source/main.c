#include <3ds.h> 
#include "ui.h" 
#include "network.h" 
int main(void){ 
if(ui_init()!=0)return 1; 
net_init(); 
while(aptMainLoop()){ 
hidScanInput();if(hidKeysDown()&KEY_START)break; 
ui_render(NULL);} 
ui_exit();return 0;} 
