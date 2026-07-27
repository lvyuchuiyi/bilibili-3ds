#include <3ds.h>
#include <citro2d.h>

int ui_init(void) {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    return 0;
}

void ui_exit(void) {
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}