#include <3ds.h>
#include <string.h>
#include "network.h"

int main(void) {
    gfxInitDefault();

    /* net_init() handles SOC + libcurl init via curl_global_init */
    int ret = net_init();

    /* Display result - use raw framebuffer without citro2d */
    u8 *fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    u32 color = (ret == 0) ? 0x0000FF00 : 0x000000FF;
    for (int i = 0; i < 400 * 240; i++) ((u32*)fb)[i] = color;
    gfxSwapBuffers();

    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;
    }
    net_exit();
    gfxExit();
    return 0;
}
