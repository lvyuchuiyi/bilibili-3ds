#include <3ds.h> 
#include <citro2d.h> 
int main(void) { 
    gfxInitDefault(); 
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE); 
    C2D_Init(4096); 
    C2D_Prepare(); 
    gfxSet3D(false); 
    C2D_Font font = C2D_FontLoadSystem(1); 
    if (!font) return 1; 
    C2D_TextBuf buf = C2D_TextBufNew(65536); 
    if (!buf) { C2D_FontFree(font); return 1; } 
    C2D_Text txt; 
    C2D_TextFontParse(&txt, font, buf, "BiliBili 3DS"); 
    C2D_TextOptimize(&txt); 
    C3D_RenderTarget *top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT); 
    C3D_RenderTarget *bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT); 
    if (!top || !bot) return 1; 
    while (aptMainLoop()) { 
        hidScanInput(); 
        if (hidKeysDown() & KEY_START) break; 
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW); 
        C2D_TextBufClear(buf); 
        C2D_TargetClear(top, C2D_Color32(0x00,0x96,0xED,0xFF)); 
        C2D_SceneBegin(top); 
        C2D_DrawText(&txt, C2D_WithColor, 20, 60, 0.5f, 1.0f, 1.0f, C2D_Color32(0xFF,0xFF,0xFF,0xFF)); 
        C2D_TargetClear(bot, C2D_Color32(0xE8,0xE8,0xE8,0xFF)); 
        C2D_SceneBegin(bot); 
        C3D_FrameEnd(0); 
    } 
    C2D_TextBufDelete(buf); C2D_FontFree(font); 
    C2D_Fini(); C3D_Fini(); gfxExit(); 
    return 0; 
} 
