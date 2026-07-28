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

#define CLR_BG   C2D_Color32(0xF5, 0xF5, 0xF5, 0xFF)
#define CLR_PRI  C2D_Color32(0x00, 0x96, 0xED, 0xFF)
#define TOP_W 400
#define TOP_H 240
#define BOT_W 320
#define BOT_H 240

static void draw_rect(int x, int y, int w, int h, u32 c) {
    C2D_DrawRectSolid(x, y, 0.5f, w, h, c);
}

int ui_init(void) {
    gfxInitDefault();
    C3D_Init(0x100000);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    top_target = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    bot_target = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!top_target || !bot_target) return -1;
    return 0;
}

void ui_exit(void) {
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

void ui_render(app_state_t *state) {
    if (!state) return;
    player_info_t pinfo;
    player_get_info(&pinfo);
    (void)pinfo;

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(top_target, CLR_BG);
    C2D_SceneBegin(top_target);
    draw_rect(0, 0, TOP_W, 40, CLR_PRI);
    draw_rect(100, 60, 200, 48, CLR_PRI);
    draw_rect(100, 130, 200, 48, CLR_PRI);

    C2D_TargetClear(bot_target, C2D_Color32(0xE8, 0xE8, 0xE8, 0xFF));
    C2D_SceneBegin(bot_target);
    draw_rect(0, 0, BOT_W, 24, C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));
    C3D_FrameEnd(0);
}

int ui_handle_touch(app_state_t *state, touchPosition *touch) { (void)state; (void)touch; return 0; }
int ui_handle_keys(app_state_t *state, u32 keys_down) { (void)state; (void)keys_down; return 0; }