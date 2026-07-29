#include <3ds.h> 
#include "ui.h" 
#include "player.h" 
int main(void){ 
if(ui_init()!=0)return 1; 
player_init(); 
while(aptMainLoop()){ 
hidScanInput();if(hidKeysDown()&KEY_START)break; 
ui_render(NULL);} 
ui_exit();return 0;} 
