#include <3ds.h>
#include "ui.h"

int main(void) {
    if (ui_init() != 0) return 1;
    
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;
        
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(ui_get_top(), C2D_Color32(0x00,0x96,0xED,0xFF));
        C2D_SceneBegin(ui_get_top());
        C3D_FrameEnd(0);
    }
    
    ui_exit();
    return 0;
}