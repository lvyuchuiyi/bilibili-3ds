#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include "ui.h"
#include "network.h"
#include "bilibili.h"

static app_state_t state;

static void load_popular(void){
    bili_popular(&state.popular_list);
}

static void load_search(void){
    if(strlen(state.search_text)>0)
        bili_search(state.search_text,&state.search_list);
}

int main(void){
    if(ui_init()!=0)return 1;
    int net_ok=(net_init()==0);
    (void)net_ok;
    /* player_init skipped - MVD not available on Azahar */

    memset(&state,0,sizeof(state));
    state.current_screen=SCREEN_MAIN_MENU;
    state.prev_screen=SCREEN_MAIN_MENU;

    int load_requested=0;
    int prev_screen=SCREEN_SPLASH;

    while(aptMainLoop()){
        if(load_requested==1){load_requested=0;load_popular();}
        else if(load_requested==2){load_requested=0;load_search();}

        hidScanInput();
        u32 keys_down=hidKeysDown();
        int handled=ui_handle_keys(&state,keys_down);
        if(handled==2){} // would start playback

        if(state.current_screen==SCREEN_POPULAR&&prev_screen!=SCREEN_POPULAR)
            load_requested=1;
        if(state.current_screen==SCREEN_SEARCH_RESULTS&&prev_screen==SCREEN_SEARCH_INPUT)
            load_requested=2;
        prev_screen=state.current_screen;

        ui_render(&state);
    }
    ui_exit();return 0;
}