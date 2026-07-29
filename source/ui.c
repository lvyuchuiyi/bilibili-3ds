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

#define CLR_BG     C2D_Color32(0xF5,0xF5,0xF5,0xFF)
#define CLR_PRI    C2D_Color32(0x00,0x96,0xED,0xFF)
#define CLR_WHITE  C2D_Color32(0xFF,0xFF,0xFF,0xFF)
#define CLR_CARD   C2D_Color32(0xFF,0xFF,0xFF,0xFF)
#define CLR_TLT    C2D_Color32(0x88,0x88,0x88,0xFF)
#define TOP_W 400
#define TOP_H 240
#define BOT_W 320
#define BOT_H 240

static void draw_rect(int x, int y, int w, int h, u32 c) {
    C2D_DrawRectSolid(x, y, 0.5f, w, h, c);
}

/* Each call creates its own local C2D_Text - like ClouDS Music */
static void draw_text(int x, int y, float sc, u32 col, const char *f, ...) {
    char buf[256]; va_list a; va_start(a, f); vsnprintf(buf,256,f,a); va_end(a);
    C2D_Text t;
    C2D_TextFontParse(&t, font, textbuf, buf);
    C2D_TextOptimize(&t);
    C2D_DrawText(&t, C2D_WithColor, x, y, 0.5f, sc, sc, col);
}

int ui_init(void) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    font = C2D_FontLoadSystem(1);
    if (!font) return -1;
    textbuf = C2D_TextBufNew(65536);
    if (!textbuf) { C2D_FontFree(font); return -1; }
    top_target = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bot_target = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!top_target || !bot_target) return -1;
    return 0;
}

void ui_exit(void) {
    if (textbuf) C2D_TextBufDelete(textbuf);
    if (font) C2D_FontFree(font);
    C2D_Fini(); C3D_Fini(); gfxExit();
}

void ui_render(app_state_t *state) {
    if (!state) return;
    C2D_TextBufClear(textbuf);
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(top_target, CLR_BG);
    C2D_SceneBegin(top_target);
    draw_rect(0, 0, TOP_W, 40, CLR_PRI);
    draw_text(16, 10, 0.65f, CLR_WHITE, "BiliBili for 3DS");
    draw_rect(100, 70, 200, 48, CLR_PRI);
    draw_text(116, 84, 0.55f, CLR_WHITE, "Popular");
    draw_rect(100, 140, 200, 48, CLR_PRI);
    draw_text(170, 154, 0.55f, CLR_WHITE, "Search");
    C2D_TargetClear(bot_target, C2D_Color32(0xE8,0xE8,0xE8,0xFF));
    C2D_SceneBegin(bot_target);
    draw_rect(0, 0, BOT_W, 24, CLR_CARD);
    draw_text(16, 50, 0.45f, CLR_TLT, "A-Select");
    draw_text(16, 80, 0.40f, CLR_TLT, "START-Exit");
    C3D_FrameEnd(0);
}

int ui_handle_touch(app_state_t *s, touchPosition *t) { (void)s;(void)t;return 0; }
int ui_handle_keys(app_state_t *s, u32 k) { (void)s;(void)k;return 0; }
