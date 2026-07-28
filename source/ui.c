#include <3ds.h>
#include <citro2d.h>

static C3D_RenderTarget *top = NULL;
static C3D_RenderTarget *bot = NULL;
static C2D_Font font = NULL;
static C2D_TextBuf textbuf = NULL;
static C2D_Text txt_hello;

int ui_init(void) {
    gfxInitDefault();
    C3D_Init(0x100000);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    font = C2D_FontLoadSystem(1);
    if (!font) return -1;

    textbuf = C2D_TextBufNew(4096);
    if (!textbuf) return -1;

    C2D_TextFontParse(&txt_hello, font, textbuf, "BILIBILI 3DS");
    C2D_TextOptimize(&txt_hello);

    top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    if (!top) return -1;

    bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!bot) return -1;

    return 0;
}

void ui_render(void) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    C2D_TargetClear(top, C2D_Color32(0x00, 0x96, 0xED, 0xFF));
    C2D_SceneBegin(top);
    C2D_DrawText(&txt_hello, C2D_WithColor, 20, 60, 0.5f, 1.0f, 1.0f,
                 C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF));

    C2D_TargetClear(bot, C2D_Color32(0xE8, 0xE8, 0xE8, 0xFF));
    C2D_SceneBegin(bot);

    C3D_FrameEnd(0);
}

void ui_exit(void) {
    C2D_TextBufDelete(textbuf);
    C2D_FontFree(font);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}