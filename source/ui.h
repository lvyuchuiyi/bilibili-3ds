#ifndef _UI_H_
#define _UI_H_
#include <3ds.h>
#include "bilibili.h"
#define MAX_VISIBLE_ITEMS 8
typedef enum {
    SCREEN_SPLASH,SCREEN_MAIN_MENU,SCREEN_POPULAR,SCREEN_SEARCH_INPUT,
    SCREEN_SEARCH_RESULTS,SCREEN_VIDEO_DETAIL,SCREEN_PLAYING,SCREEN_EXIT
} app_screen_t;
typedef struct {
    app_screen_t current_screen,prev_screen;
    bili_video_list_t popular_list,search_list;
    bili_video_t current_video;
    char search_text[128]; int search_text_pos;
    int selected_index,scroll_offset;
    int kb_shift,kb_page;
} app_state_t;
void draw_char(int x,int y,char ch,u32 col);
void draw_str(int x,int y,u32 col,const char*s);
void draw_rect(int x,int y,int w,int h,u32 col);
int ui_init(void);
void ui_exit(void);
void ui_render(app_state_t *state);
int ui_handle_touch(app_state_t *state,touchPosition *touch);
int ui_handle_keys(app_state_t *state,u32 keys_down);
#endif