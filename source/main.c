#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include "network.h"

int main(void) {
    gfxInitDefault(); C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(4096); C2D_Prepare(); gfxSet3D(false);
    C3D_RenderTarget *t = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    if (!t) return 1;

    int ret = net_init();
    u32 col;
    if (ret == 0) col = C2D_Color32(0,255,0,255);       /* green */
    else if (ret == -1) col = C2D_Color32(255,0,0,255);   /* red */
    else if (ret == -2) col = C2D_Color32(255,255,0,255); /* yellow */
    else col = C2D_Color32(0,0,255,255);                   /* blue */

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