#include <3ds.h>
#include <stdio.h>
#include "network.h"

int main(void) {
    gfxInitDefault(); C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(4096); C2D_Prepare(); gfxSet3D(false);
    C3D_RenderTarget *t = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    if (!t) return 1;

    int ret = net_init();
    /* 0=green(GREEN) -1=red(RED) -2=yellow(YELLOW) -3=blue(BLUE) */
    u32 colors[] = {0xFF00FF00, 0xFFFF0000, 0xFFFFFF00, 0xFF0000FF};
    u32 col = (ret == 0) ? colors[0] : (ret <= -3) ? colors[3] : colors[-ret];

    char text[64]; snprintf(text, 64, "NET_INIT=%d", ret);

    while (aptMainLoop()) {
        hidScanInput(); if (hidKeysDown() & KEY_START) break;
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(t, C2D_Color32(0,0,0,0xFF));
        C2D_SceneBegin(t);
        C2D_DrawRectSolid(0, 0, 0.5f, 400, 240, col);
        C3D_FrameEnd(0);
    }
    C2D_Fini(); C3D_Fini(); gfxExit(); return 0;
}