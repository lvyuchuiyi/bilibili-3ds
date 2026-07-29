#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <citro2d.h>
#include "ui.h"
#include "player.h"
#include <stdarg.h>

static C3D_RenderTarget *top_target = NULL;
static C3D_RenderTarget *bot_target = NULL;
static C2D_Font font = NULL;
static C2D_TextBuf textbuf = NULL;

/* Pre-parsed text objects */
#define MAX_TEXTS 32
static C2D_Text text_objs[MAX_TEXTS];
static int text_count = 0;

/* Colors */
#define CLR_BG        C2D_Color32(0xF5, 0xF5, 0xF5, 0xFF)
#define CLR_PRIMARY   C2D_Color32(0x00, 0x96, 0xED, 0xFF)
#define CLR_TEXT      C2D_Color32(0x22, 0x22, 0x22, 0xFF)
#define CLR_TEXT_LT   C2D_Color32(0x88, 0x88, 0x88, 0xFF)
#define CLR_WHITE     C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
#define CLR_CARD      C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF)
#define CLR_SEL       C2D_Color32(0xE3, 0xF2, 0xFD, 0xFF)
#define CLR_RED       C2D_Color32(0xFB, 0x72, 0x99, 0xFF)
#define TOP_W  400
#define TOP_H  240
#define BOT_W  320
#define BOT_H  240
#define MARGIN  8

static void draw_rect(int x, int y, int w, int h, u32 color) {
    C2D_DrawRectSolid(x, y, 0.5f, w, h, color);
}

#define T_HEADER 0
#define T_BTN1   1
#define T_BTN2   2
#define T_BACK   3
#define T_NONE   4
#define T_ABACK  5
#define T_SELECT 6
#define T_EXIT   7
#define T_AOPEN  8
#define T_BBACK  9
#define T_UPNAV  10
#define T_APLAY  11
#define T_XPAUSE 12
#define T_BSTOP  13
#define T_PLAY   14
#define T_VIDEOS 15

static void init_texts(void) {
    C2D_TextBufClear(textbuf);
    #define PT(idx, str) C2D_TextFontParse(&text_objs[idx], font, textbuf, str); C2D_TextOptimize(&text_objs[idx])
    PT(T_HEADER, "BiliBili for 3DS");
    PT(T_BTN1,   "Popular Videos");
    PT(T_BTN2,   "Search");
    PT(T_BACK,   "B-Back");
    PT(T_NONE,   "No videos");
    PT(T_ABACK,  "A-Select");
    PT(T_SELECT, "A-Select");
    PT(T_EXIT,   "START-Exit");
    PT(T_AOPEN,  "A-Open video");
    PT(T_BBACK,  "B-Go back");
    PT(T_UPNAV,  "UP/DOWN-Navigate");
    PT(T_APLAY,  "A-Play");
    PT(T_XPAUSE, "X-Pause");
    PT(T_BSTOP,  "B-Stop");
    PT(T_PLAY,   "Play");
    PT(T_VIDEOS, "Videos");
    #undef PT
    text_count = 16;
}

static void draw_text_idx(int idx, int x, int y, float scale, u32 color) {
    if (idx >= 0 && idx < text_count)
        C2D_DrawText(&text_objs[idx], C2D_WithColor, x, y, 0.5f, scale, scale, color);
}

int ui_init(void) {
    gfxInitDefault();
    C3D_Init(0x100000);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    font = C2D_FontLoadSystem(1);
    if (!font) return -1;

    textbuf = C2D_TextBufNew(65536);
    if (!textbuf) { C2D_FontFree(font); return -1; }

    top_target = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bot_target = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!top_target || !bot_target) return -1;

    init_texts();
    return 0;
}

void ui_exit(void) {
    if (textbuf) C2D_TextBufDelete(textbuf);
    if (font) C2D_FontFree(font);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

void ui_render(app_state_t *state) {
    if (!state) return;
    player_info_t pinfo;
    player_get_info(&pinfo);

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    /* Top screen */
    C2D_TargetClear(top_target, CLR_BG);
    C2D_SceneBegin(top_target);
    draw_rect(0, 0, TOP_W, 40, CLR_PRIMARY);
    draw_text_idx(T_HEADER, 16, 10, 0.65f, CLR_WHITE);
    draw_rect(100, 70, 200, 48, CLR_PRIMARY);
    draw_text_idx(T_BTN1, 116, 84, 0.55f, CLR_WHITE);
    draw_rect(100, 140, 200, 48, CLR_PRIMARY);
    draw_text_idx(T_BTN2, 170, 154, 0.55f, CLR_WHITE);

    /* Bottom screen */
    C2D_TargetClear(bot_target, C2D_Color32(0xE8, 0xE8, 0xE8, 0xFF));
    C2D_SceneBegin(bot_target);
    draw_rect(0, 0, BOT_W, 24, CLR_CARD);
    draw_text_idx(T_SELECT, 16, 50, 0.45f, CLR_TEXT_LT);
    draw_text_idx(T_EXIT, 16, 80, 0.40f, CLR_TEXT_LT);

    C3D_FrameEnd(0);
}

int ui_handle_touch(app_state_t *state, touchPosition *touch) { (void)state; (void)touch; return 0; }
int ui_handle_keys(app_state_t *state, u32 keys_down) { (void)state; (void)keys_down; return 0; }