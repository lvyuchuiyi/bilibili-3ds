#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>

int main(void) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    consoleInit(GFX_BOTTOM, NULL);  // 文字放下面
    printf("BiliBili 3DS\n");
    
    C2D_Font fnt = C2D_FontLoadSystem(1);
    printf("Font: %s\n", fnt ? "ok" : "null");
    
    C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    
    C2D_TextBuf buf = C2D_TextBufNew(4096);
    C2D_Text txt;
    bool text_ok = false;
    
    if (fnt) {
        C2D_TextFontParse(&txt, fnt, buf, "BILIBILI 3DS");
        C2D_TextOptimize(&txt);
        text_ok = true;
    }
    
    while (aptMainLoop()) {
        hidScanInput();
        if (hidKeysDown() & KEY_START) break;
        
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(top, C2D_Color32(0x00,0x00,0x00,0xFF));
        C2D_SceneBegin(top);
        
        if (text_ok) {
            // 大号文字，黄色
            C2D_DrawText(&txt, C2D_WithColor, 20, 60, 1.0f, 1.0f, 0.5f, C2D_Color32(0xFF,0xFF,0x00,0xFF));
        }
        
        C3D_FrameEnd(0);
    }
    
    C2D_TextBufDelete(buf);
    if (fnt) C2D_FontFree(fnt);
    C2D_Fini(); C3D_Fini(); gfxExit();
    return 0;
}