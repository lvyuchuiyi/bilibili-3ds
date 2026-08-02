#include <3ds.h>
#include <citro2d.h>
#include <string.h>
#include <stdio.h>
#include "ui.h"
#include "player.h"
static C2D_Font sys_font = NULL;  /* CHN font when available */
int ui_debug_font = -1;                /* 0=default 1=CHN loaded */
int ui_debug_romfs = -1;
int ui_debug_file = -1;
int ui_debug_file_size = -1;
static C2D_TextBuf sys_buf = NULL;

/* ===== 8x8 bitmap font: 0-9=0..9, A-Z=10..35, a-z=36..61, sp=62, :=63, -=64, /=65 ===== */
static const unsigned char font[66][8]={
{0x3C,0x66,0x6E,0x7E,0x76,0x66,0x3C,0x00},{0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00},
{0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00},{0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00},
{0x0C,0x1C,0x3C,0x6C,0xFE,0x0C,0x0C,0x00},{0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00},
{0x3C,0x66,0x60,0x7C,0x66,0x66,0x3C,0x00},{0x7E,0x06,0x0C,0x18,0x30,0x30,0x30,0x00},
{0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00},{0x3C,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00},
{0x7C,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},{0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00},
{0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00},{0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00},
{0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00},{0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00},
{0x3C,0x66,0xC0,0xCE,0xC6,0x66,0x3E,0x00},{0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00},
{0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00},{0x06,0x06,0x06,0x06,0x06,0x66,0x3C,0x00},
{0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00},{0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00},
{0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00},{0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00},
{0x3C,0x66,0xC6,0xC6,0xC6,0x66,0x3C,0x00},{0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00},
{0x3C,0x66,0xC6,0xC6,0xCE,0x66,0x3E,0x00},{0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00},
{0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00},{0x7E,0x5A,0x18,0x18,0x18,0x18,0x3C,0x00},
{0xC6,0xC6,0xC6,0xC6,0xC6,0x66,0x3C,0x00},{0xC6,0xC6,0xC6,0x6C,0x6C,0x38,0x10,0x00},
{0xC6,0xD6,0xD6,0xD6,0xFE,0x6C,0x44,0x00},{0xC6,0x6C,0x38,0x38,0x6C,0xC6,0xC6,0x00},
{0xC6,0xC6,0x6C,0x38,0x38,0x38,0x7C,0x00},{0xFE,0xCC,0x18,0x30,0x60,0xC6,0xFE,0x00},
{0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00},{0xE0,0x60,0x7C,0x66,0x66,0x66,0xDC,0x00},
{0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00},{0x1C,0x0C,0x3C,0x6C,0xCC,0xCC,0x76,0x00},
{0x00,0x00,0x3C,0x66,0xFC,0x60,0x3C,0x00},{0x38,0x6C,0x60,0xF0,0x60,0x60,0xF0,0x00},
{0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0xF8},{0xE0,0x60,0x6C,0x76,0x66,0x66,0xE6,0x00},
{0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00},{0x06,0x00,0x06,0x06,0x06,0x66,0x3C,0x00},
{0xE0,0x60,0x66,0x6C,0x78,0x6C,0xE6,0x00},{0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00},
{0x00,0x00,0xEC,0xFE,0xD6,0xC6,0xC6,0x00},{0x00,0x00,0xDC,0x66,0x66,0x66,0x66,0x00},
{0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00},{0x00,0x00,0xDC,0x66,0x66,0x7C,0x60,0xF0},
{0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0x1E},{0x00,0x00,0xDC,0x76,0x66,0x60,0xF0,0x00},
{0x00,0x00,0x7C,0x60,0x3C,0x06,0x7C,0x00},{0x10,0x30,0xFC,0x30,0x30,0x34,0x18,0x00},
{0x00,0x00,0xCC,0xCC,0xCC,0xCC,0x76,0x00},{0x00,0x00,0xC6,0xC6,0x6C,0x6C,0x38,0x00},
{0x00,0x00,0xC6,0xD6,0xD6,0xFE,0x6C,0x00},{0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00},
{0x00,0x00,0xC6,0xC6,0xCE,0x76,0x06,0x7C},{0x00,0x00,0xFC,0x98,0x30,0x64,0xFC,0x00},
{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0x00,0x00,0x18,0x18,0x18,0x00,0x18,0x00},
{0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00},{0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18},
};

static int font_idx(char ch){
if(ch>='0'&&ch<='9')return ch-'0';
if(ch>='A'&&ch<='Z')return ch-'A'+10;
if(ch>='a'&&ch<='z')return ch-'a'+36;
if(ch==' ')return 62;if(ch==':')return 63;if(ch=='-')return 64;if(ch=='/')return 65;
return 62;
}

void draw_char(int x,int y,char ch,u32 col){
int idx=font_idx(ch);
for(int r=0;r<8;r++){unsigned char b=font[idx][r];
for(int c=0;c<8;c++){if(b&(0x80>>c))C2D_DrawRectSolid(x+c,y+r,0.5f,1,1,col);}}
}

void draw_str(int x,int y,u32 col,const char*s){
while(*s){draw_char(x,y,*s,col);x+=9;s++;}
}

void draw_rect(int x,int y,int w,int h,u32 col){
C2D_DrawRectSolid(x,y,0.5f,w,h,col);
}
/* System font supports Chinese; use for video titles */
void draw_text_cjk(int x,int y,float size,u32 col,const char*s){
if(!sys_buf||!s||!*s)return;
C2D_Text text;
C2D_TextBufClear(sys_buf);
if(sys_font)C2D_TextFontParse(&text,sys_font,sys_buf,s);
else C2D_TextParse(&text,sys_buf,s);
float sc=size/30.0f;
C2D_DrawText(&text,C2D_WithColor,x,y,0.5f,sc,sc,col);
}
void draw_text_hybrid(int x,int y,float size,u32 col,const char*s){
if(!s)return;
int cx=x;
const unsigned char *p=(const unsigned char*)s;
while(*p){
if(*p<0x80){
char ch=(char)*p;
draw_char(cx,y,ch,col);
cx+=9;
p++;
}else{
int len=1;
if((*p&0xE0)==0xC0)len=2;
else if((*p&0xF0)==0xE0)len=3;
else if((*p&0xF8)==0xF0)len=4;
char buf[5]={0};
for(int k=0;k<len&&p[k];k++)buf[k]=(char)p[k];
buf[len]=0;
if(sys_buf){
C2D_Text text;
C2D_TextBufClear(sys_buf);
if(sys_font)C2D_TextFontParse(&text,sys_font,sys_buf,buf);
else C2D_TextParse(&text,sys_buf,buf);
float sc=size/30.0f;
C2D_DrawText(&text,C2D_WithColor,cx,y,0.5f,sc,sc,col);
cx+=(int)(text.width*sc)+1;
}
p+=len;
}
}
}

#define CLR_BG C2D_Color32(0xF5,0xF5,0xF5,0xFF)
#define CLR_PRI C2D_Color32(0x00,0x96,0xED,0xFF)
#define CLR_W C2D_Color32(0xFF,0xFF,0xFF,0xFF)
#define CLR_T C2D_Color32(0x22,0x22,0x22,0xFF)
#define CLR_TL C2D_Color32(0x88,0x88,0x88,0xFF)
#define CLR_RED C2D_Color32(0xFB,0x72,0x99,0xFF)
#define CLR_SEL C2D_Color32(0xE3,0xF2,0xFD,0xFF)
#define CLR_CARD C2D_Color32(0xFF,0xFF,0xFF,0xFF)
#define TOP_W 400
#define TOP_H 240
#define BOT_W 320
#define BOT_H 240
#define MARGIN 8
#define LIST_ITEM_H 26
#define KB_KEY_W 24
#define KB_KEY_H 20
#define KB_GAP 2

static C3D_RenderTarget *t=NULL,*b=NULL;

int ui_init(void){
gfxInitDefault();C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
C2D_Init(4096);C2D_Prepare();gfxSet3D(false);
t=C2D_CreateScreenTarget(GFX_TOP,GFX_LEFT);
b=C2D_CreateScreenTarget(GFX_BOTTOM,GFX_LEFT);
  sys_buf=C2D_TextBufNew(512);
  ui_debug_romfs=R_SUCCEEDED(romfsInit())?1:0;
  {
    FILE *f=fopen("romfs:/switch_font.bcfnt","rb");
    ui_debug_file=f?1:0;
    if(f){fseek(f,0,SEEK_END);ui_debug_file_size=ftell(f);fclose(f);}
  }
  sys_font=C2D_FontLoad("romfs:/switch_font.bcfnt");
  ui_debug_font=sys_font?1:0;
if(!t||!b)return 1;
return 0;
}

void ui_exit(void){
if(sys_buf){C2D_TextBufDelete(sys_buf);sys_buf=NULL;}
if(sys_font){C2D_FontFree(sys_font);sys_font=NULL;}
  romfsExit();
C2D_Fini();C3D_Fini();gfxExit();
}

/* ===== Top screen renderers ===== */
static void render_main_menu(void){
draw_rect(0,0,TOP_W,40,CLR_PRI);
draw_str(16,10,CLR_W,"BiliBili for 3DS");
draw_rect(100,70,200,48,CLR_PRI);
draw_str(130,84,CLR_W,"Popular Videos");
draw_rect(100,140,200,48,CLR_PRI);
draw_str(155,154,CLR_W,"Search");
}

static void render_video_list(const char *title, bili_video_list_t *list, int sel, int scroll){
draw_rect(0,0,TOP_W,32,CLR_PRI);
draw_str(12,8,CLR_W,title);
draw_str(TOP_W-80,8,CLR_W,"B-Back");
if(!list||list->count==0){
char dbgc[32];if(list)snprintf(dbgc,sizeof(dbgc),"count=%d",list->count);else strcpy(dbgc,"NULL");draw_str(60,84,CLR_TL,dbgc);extern int app_loading;extern int app_load_timed_out;if(app_loading)draw_str(60,100,CLR_TL,"Loading...");else if(app_load_timed_out)draw_str(60,100,CLR_RED,"Timeout");else draw_str(60,100,CLR_TL,"No videos found");return;}
int y=36,vis=(TOP_H-36-MARGIN)/LIST_ITEM_H;
if(vis>MAX_VISIBLE_ITEMS)vis=MAX_VISIBLE_ITEMS;
for(int i=scroll;i<list->count&&i<scroll+vis;i++){
bili_video_t *v=&list->videos[i];
int iy=y+(i-scroll)*LIST_ITEM_H;
if(i==sel)draw_rect(MARGIN,iy,TOP_W-MARGIN*2,LIST_ITEM_H,CLR_SEL);
draw_rect(MARGIN,iy,22,LIST_ITEM_H,CLR_RED);
char idx[4];snprintf(idx,4,"%d",i+1);draw_str(MARGIN+4,iy+9,CLR_W,idx);
char dt[60];int tl=strlen(v->title);
if(tl>52){strncpy(dt,v->title,52);dt[52]=0;}else strcpy(dt,v->title);
draw_text_hybrid(MARGIN+28,iy+2,12.0f,CLR_T,dt);
draw_text_hybrid(MARGIN+28,iy+14,10.0f,CLR_TL,v->author);
if(i<list->count-1)draw_rect(MARGIN,iy+LIST_ITEM_H-1,TOP_W-MARGIN*2,1,C2D_Color32(0xE0,0xE0,0xE0,0xFF));
}
if(scroll>0)draw_str(TOP_W/2-10,TOP_H-14,CLR_TL,"^");
if(scroll+vis<list->count)draw_str(TOP_W/2-10,TOP_H-14,CLR_TL,"v");
}

static void render_search_input(const char *text){
draw_rect(0,0,TOP_W,40,CLR_PRI);
draw_str(16,12,CLR_W,"Search");
draw_str(MARGIN,60,CLR_T,"Type on bottom screen");
draw_str(MARGIN,90,CLR_TL,"Type on bottom screen");
draw_str(MARGIN,110,CLR_TL,"B to go back");
}

static void render_video_detail(bili_video_t *v){
draw_rect(0,0,TOP_W,40,CLR_PRI);
draw_str(12,12,CLR_W,"Detail");
draw_str(TOP_W-80,12,CLR_W,"B-Back");
int y=50;
draw_rect(MARGIN,y,TOP_W-MARGIN*2,90,CLR_CARD);
draw_text_hybrid(MARGIN+8,y+6,14.0f,CLR_T,v->title);
char auth[64];snprintf(auth,64,"By: ");strncat(auth,v->author,60);
draw_text_hybrid(MARGIN+8,y+32,12.0f,CLR_TL,auth);
int m=v->duration/60,s=v->duration%60;
char stats[64];snprintf(stats,64,"%d:%02d - %d plays",m,s,v->play_count);
draw_str(MARGIN+8,y+54,CLR_TL,stats);
draw_rect(MARGIN,155,TOP_W-MARGIN*2,40,CLR_PRI);
draw_str(TOP_W/2-20,167,CLR_W,"Play");
}

static void render_playing(player_info_t *p){
(void)p;
extern void player_render(void);
extern char app_debug_playurl[128];
if(strncmp(app_debug_playurl,"MVD:HANG",8)==0){
draw_rect(0,0,TOP_W,TOP_H,C2D_Color32(0x00,0x00,0x00,0xFF));
draw_str(TOP_W/2-130,TOP_H/2-10,CLR_RED,"MVD not supported in emulator");
draw_str(TOP_W/2-80,TOP_H/2+16,CLR_TL,"B - Back");
return;
}
player_render();
draw_str(TOP_W/2-80,TOP_H-20,CLR_TL,"B-Stop X-Pause");
}
(void)p;
extern void player_render(void);
player_render();
draw_str(TOP_W/2-80,TOP_H-20,CLR_TL,"B-Stop X-Pause");
}

/* ===== Bottom screen renderers ===== */
static void render_keyboard(const char *text,int shift){
draw_rect(0,0,BOT_W,BOT_H,C2D_Color32(0xCC,0xCC,0xCC,0xFF));
draw_rect(0,0,BOT_W,24,CLR_W);
draw_str(4,4,CLR_T,text);
int kx=4,ky=28,kw=KB_KEY_W,kh=KB_KEY_H,gap=KB_GAP;
const char *rows[4];rows[0]=shift?"QWERTYUIOP":"qwertyuiop";
rows[1]=shift?"ASDFGHJKL":"asdfghjkl";
rows[2]=shift?"ZXCVBNM":"zxcvbnm";rows[3]=".,?!@-_:;()";
for(int r=0;r<4;r++){
int sx=kx+(BOT_W-(int)strlen(rows[r])*(kw+gap))/2;
for(int c=0;rows[r][c];c++){
int bx=sx+c*(kw+gap),by=ky+r*(kh+gap);
draw_rect(bx,by,kw,kh,CLR_W);
char ch[2]={rows[r][c],0};draw_str(bx+8,by+4,CLR_T,ch);
}}
int cy=ky+4*(kh+gap)+4,g2=4,key_w=(BOT_W-5*g2)/6;
draw_rect(g2,cy,key_w,24,CLR_PRI);draw_str(g2+4,cy+4,CLR_W,shift?"ABC":"abc");
draw_rect(g2*2+key_w,cy,key_w*2+2,24,CLR_CARD);draw_str(g2*2+key_w+10,cy+4,CLR_T,"Space");
draw_rect(g2*3+key_w*3,cy,key_w,24,C2D_Color32(0xFF,0x99,0x66,0xFF));
draw_str(g2*3+key_w*3+2,cy+4,CLR_W,"DEL");
draw_rect(g2*4+key_w*4,cy,key_w,24,C2D_Color32(0xFF,0x66,0x66,0xFF));
draw_str(g2*4+key_w*4+6,cy+4,CLR_W,"Clr");
draw_rect(g2*5+key_w*5,cy,key_w,24,C2D_Color32(0x00,0xCC,0x66,0xFF));
draw_str(g2*5+key_w*5+8,cy+4,CLR_W,"Go");
}

static void render_bottom_default(app_screen_t screen){
draw_rect(0,0,BOT_W,BOT_H,C2D_Color32(0xE8,0xE8,0xE8,0xFF));
draw_rect(0,0,BOT_W,24,CLR_CARD);
/* debug lines */
{
char dbg[64];
extern int app_load_stage;
extern int ui_debug_font;
extern int ui_debug_romfs;
extern int net_debug_http_status;
extern int net_debug_stage;
extern int net_debug_data_len;
extern int player_debug_state;
extern int player_debug_init;
extern int player_debug_load;
extern int player_debug_h264;
extern int main_debug_load_count;
snprintf(dbg,sizeof(dbg),"al:%d f:%d r:%d load:%d",app_load_stage,ui_debug_font,ui_debug_romfs,main_debug_load_count);
draw_str(4,210,CLR_TL,dbg);
snprintf(dbg,sizeof(dbg),"st:%d sg:%d dl:%d",net_debug_http_status,net_debug_stage,net_debug_data_len);
draw_str(4,220,CLR_TL,dbg);
snprintf(dbg,sizeof(dbg),"pl:%d i:%d l:%d h264:%d",player_debug_state,player_debug_init,player_debug_load,player_debug_h264);
draw_str(4,230,CLR_TL,dbg);
extern char app_debug_playurl[128];
char dbgp[32];strncpy(dbgp,app_debug_playurl,30);dbgp[30]=0;draw_str(4,200,CLR_RED,dbgp);
}
switch(screen){
case SCREEN_MAIN_MENU:
draw_str(16,50,CLR_TL,"A - Select   X - Search");
draw_str(16,80,CLR_TL,"START - Exit");
break;
case SCREEN_POPULAR:
case SCREEN_SEARCH_RESULTS:
draw_str(16,50,CLR_TL,"A - Open video");
draw_str(16,80,CLR_TL,"B - Go back");
draw_str(16,110,CLR_TL,"UP/DOWN - Navigate");
break;
case SCREEN_VIDEO_DETAIL:
draw_str(16,50,CLR_TL,"A - Play video");
draw_str(16,80,CLR_TL,"B - Go back");
break;
case SCREEN_PLAYING:
draw_str(16,50,CLR_TL,"X - Pause");
draw_str(16,80,CLR_TL,"B - Stop");
break;
default:break;
}
}

void ui_render(app_state_t *state){
if(!state)return;
player_info_t pinfo;player_get_info(&pinfo);
C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
C2D_TargetClear(t,CLR_BG);C2D_SceneBegin(t);
switch(state->current_screen){
case SCREEN_MAIN_MENU:render_main_menu();break;
case SCREEN_POPULAR:render_video_list("Popular",&state->popular_list,state->selected_index,state->scroll_offset);break;
case SCREEN_SEARCH_RESULTS:render_video_list("Results",&state->search_list,state->selected_index,state->scroll_offset);break;
case SCREEN_SEARCH_INPUT:render_search_input(state->search_text);break;
case SCREEN_VIDEO_DETAIL:render_video_detail(&state->current_video);break;
case SCREEN_PLAYING:render_playing(&pinfo);break;
default:render_main_menu();break;
}
C2D_TargetClear(b,C2D_Color32(0xE8,0xE8,0xE8,0xFF));C2D_SceneBegin(b);
if(state->current_screen==SCREEN_SEARCH_INPUT)render_keyboard(state->search_text,state->kb_shift);
else render_bottom_default(state->current_screen);
C3D_FrameEnd(0);
}

/* ===== Input handling ===== */
static int handle_kb_touch(app_state_t *s,int px,int py){
int kx=4,ky=28,kw=KB_KEY_W,kh=KB_KEY_H,gap=KB_GAP;
const char *rows[4];rows[0]=s->kb_shift?"QWERTYUIOP":"qwertyuiop";
rows[1]=s->kb_shift?"ASDFGHJKL":"asdfghjkl";
rows[2]=s->kb_shift?"ZXCVBNM":"zxcvbnm";rows[3]=".,?!@-_:;()";
for(int r=0;r<4;r++){
int sx=kx+(BOT_W-(int)strlen(rows[r])*(kw+gap))/2;
for(int c=0;rows[r][c];c++){
int bx=sx+c*(kw+gap),by=ky+r*(kh+gap);
if(px>=bx&&px<bx+kw&&py>=by&&py<by+kh){
int len=strlen(s->search_text);
if(len<(int)sizeof(s->search_text)-2){s->search_text[len]=rows[r][c];s->search_text[len+1]=0;s->search_text_pos=len+1;}
return 1;
}}}
int cy=ky+4*(kh+gap)+4,g2=4,key_w=(BOT_W-5*g2)/6;
if(px>=g2&&px<g2+key_w&&py>=cy&&py<cy+24){s->kb_shift=!s->kb_shift;return 1;}
if(px>=g2*2+key_w&&px<g2*2+key_w+key_w*2&&py>=cy&&py<cy+24){
int len=strlen(s->search_text);if(len<(int)sizeof(s->search_text)-2){s->search_text[len]=' ';s->search_text[len+1]=0;}return 1;}
if(px>=g2*3+key_w*3&&px<g2*3+key_w*3+key_w&&py>=cy&&py<cy+24){
int len=strlen(s->search_text);if(len>0)s->search_text[len-1]=0;return 1;}
if(px>=g2*4+key_w*4&&px<g2*4+key_w*4+key_w&&py>=cy&&py<cy+24){s->search_text[0]=0;s->search_text_pos=0;return 1;}
if(px>=g2*5+key_w*5&&px<g2*5+key_w*5+key_w&&py>=cy&&py<cy+24){
if(strlen(s->search_text)>0){s->current_screen=SCREEN_SEARCH_RESULTS;s->selected_index=0;s->scroll_offset=0;return 2;}
return 1;}
return 0;
}

int ui_handle_touch(app_state_t *s,touchPosition *p){
if(!s||!p)return 0;
int px=p->px,py=p->py;
if(py>=240){if(s->current_screen==SCREEN_SEARCH_INPUT)return handle_kb_touch(s,px,py-240);return 0;}
switch(s->current_screen){
case SCREEN_MAIN_MENU:{
int bx=(TOP_W-200)/2;
if(px>=bx&&px<bx+200&&py>=70&&py<70+48){s->current_screen=SCREEN_POPULAR;s->selected_index=0;s->scroll_offset=0;return 1;}
if(px>=bx&&px<bx+200&&py>=140&&py<140+48){s->current_screen=SCREEN_SEARCH_INPUT;s->search_text[0]=0;s->kb_shift=0;return 1;}
}break;
default:break;
}
return 0;
}
int ui_handle_keys(app_state_t *s,u32 k){
if(!s)return 0;
switch(s->current_screen){
case SCREEN_MAIN_MENU:
if(k&KEY_A){s->current_screen=SCREEN_POPULAR;s->selected_index=0;s->scroll_offset=0;return 1;}
if(k&KEY_X){s->current_screen=SCREEN_SEARCH_INPUT;memset(s->search_text,0,128);s->kb_shift=0;return 1;}
break;
case SCREEN_POPULAR:
case SCREEN_SEARCH_RESULTS:{
int cnt=(s->current_screen==SCREEN_POPULAR)?s->popular_list.count:s->search_list.count;
int vis=(TOP_H-36-MARGIN)/LIST_ITEM_H;if(vis>MAX_VISIBLE_ITEMS)vis=MAX_VISIBLE_ITEMS;
if(k&KEY_DOWN&&s->selected_index<cnt-1){s->selected_index++;if(s->selected_index>=s->scroll_offset+vis)s->scroll_offset++;return 1;}
if(k&KEY_UP&&s->selected_index>0){s->selected_index--;if(s->selected_index<s->scroll_offset)s->scroll_offset--;return 1;}
if(k&KEY_B){s->current_screen=SCREEN_MAIN_MENU;return 1;}
if(k&KEY_A&&cnt>0&&s->selected_index>=0&&s->selected_index<cnt){
bili_video_t *v=(s->current_screen==SCREEN_POPULAR)?&s->popular_list.videos[s->selected_index]:&s->search_list.videos[s->selected_index];
s->current_video=*v;s->current_screen=SCREEN_VIDEO_DETAIL;return 1;}
}break;
case SCREEN_SEARCH_INPUT:
if(k&KEY_B){s->current_screen=SCREEN_MAIN_MENU;return 1;}
break;
case SCREEN_VIDEO_DETAIL:
if(k&KEY_A){return 2;}
if(k&KEY_B){s->current_screen=SCREEN_POPULAR;return 1;}
break;
case SCREEN_PLAYING:
if(k&KEY_B){s->current_screen=SCREEN_VIDEO_DETAIL;extern void player_stop(void);player_stop();return 1;}
if(k&KEY_X){extern void player_pause(void);player_pause();return 1;}
break;
default:break;
}
return 0;
}
















