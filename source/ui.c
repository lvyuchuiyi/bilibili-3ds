#include <3ds.h>
#include <citro2d.h>

static C3D_RenderTarget *top = NULL;
static C2D_Font font = NULL;

int ui_init(void) {
    gfxInitDefault();
    C3D_Init(0x100000);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    font = C2D_FontLoadSystem(1);
    if (!font) return -1;

    top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    if (!top) return -1;
    return 0;
}

void ui_exit(void) {
    if (font) C2D_FontFree(font);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

C3D_RenderTarget* ui_get_top(void) { return top; }