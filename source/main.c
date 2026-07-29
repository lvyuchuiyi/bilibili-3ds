#include <3ds.h> 
#include "ui.h" 
#include "network.h" 
int main(void){ 
if(ui_init()!=0)return 1; 
int net_ok=(net_init()==0); 
(void)net_ok; 
/* player_init skipped - MVD not available on emulator */ 
while(aptMainLoop()){ 
hidScanInput();if(hidKeysDown()&KEY_START)break; 
ui_render(NULL);} 
ui_exit();return 0;} 
