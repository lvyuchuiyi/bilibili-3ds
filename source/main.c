#include <3ds.h>
#include <string.h>

static u32 *m = NULL;

int main(void) {
    gfxInitDefault();
    m = (u32*)linearAlloc(0x100000);
    int ret = -99;
    if (m) {
        ret = R_SUCCEEDED(socInit(m, 0x100000)) ? 0 : -2;
    }

    /* Display result - use raw framebuffer without citro2d */
    u8 *fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    u32 color = (ret == 0) ? 0x0000FF00 : 0x000000FF;
    for (int i = 0; i < 400 * 240; i++) ((u32*)fb)[i] = color;
    gfxSwapBuffers();

    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;
    }
    gfxExit();
    return 0;
}