#ifndef _UI_H_
#define _UI_H_
#include <3ds.h>
void draw_char(int x,int y,char ch,u32 col);
void draw_str(int x,int y,u32 col,const char*s);
void draw_rect(int x,int y,int w,int h,u32 col);
int ui_init(void);
void ui_exit(void);
void ui_render(void);
#endif