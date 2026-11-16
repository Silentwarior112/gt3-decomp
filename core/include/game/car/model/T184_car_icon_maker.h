/* T184_car_icon_maker.h - CarIcon and CarIconMaker, the 64 x 32 icons of the
 * cars of a race (translation unit T184, unit u061), 0x001ADA68..0x001AE4E0.
 *
 * car_icons_make puts a CarIconMaker on its stack, hands it to the render
 * thread (T002) and sleeps.  In the render thread, render() draws every car
 * model (RaceCarModel, T180, posed by its MenuCarSpec, T278) into a 128x64
 * buffer, makes an alpha outline, halves both into a 64x32 buffer and
 * downloads it into the icon; update() counts the frames and from the third
 * one hands the render thread back and wakes the caller.  The race modes
 * keep 7 icons (RaceArcade +0x16EC0 ...); ResultBoard (T200) shows them.
 *
 * GsTexPacket (the plan put it here) is the texture register block the car
 * models reflect: u045 / u061's stack objects. */
#ifndef GAME_T184_CAR_ICON_MAKER_H
#define GAME_T184_CAR_ICON_MAKER_H

#include "game/game.h"
#include "game/app/T002_render_thread.h"        /* base GameObject */
#include "engine/pgl/G001_flare.h"                /* GsTexRegs (held by value) */

class GSBuffer;                             /* T514_gs_buffer.h (pointers only) */
class RaceOrganization;                     /* T125_race_organization.h */
class type_info;                            /* T548 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
/* The __si_type_info at 0x004B23A0, base __ti10GameObject (0x00353318). */
extern "C" const type_info &__tf12CarIconMaker(void);  /* 0x001AE468 */
#endif

/* ? name (u061).  The texture registers of a GSBuffer the car models
 * reflect (environment map): GSBuffer::makeTexRegs fills `regs',
 * modelset_set_env_tex takes &regs (T451).  0x50 bytes: the three frames
 * that hold one keep 0x50 for it up to the next local (RacePS2Base::
 * drawCars sp+0x00, drawCarReflections sp+0x40, CarIconMaker::drawIcon
 * sp+0x13B0); makeTexRegs writes only the first 0x28 (u045 / u061: u64
 * q[10]). */
struct GsTexPacket {
    GsTexRegs regs;                         /* 0x00 */
    u8        unk_28[0x28];                 /* 0x28 (? a second GsTexRegs) */
};
EGCS_SIZE(GsTexPacket, 0x50);

/* One car icon: a 64 x 32 RGBA image, 64-byte aligned inside the object.
 * Not polymorphic.  Size 0x2044 (the race modes' arrays of 7). */
struct CarIcon {
    CarIcon();                              /* 0x001ADA68 pixels = (this + 0x3F) & ~0x3F */

    u8  image[0x2040];                      /* 0x0000 (u046 / u050 / u052 data) */
    u8 *pixels;                             /* 0x2040 the aligned 64 x 32 x 4 bytes (u046 .. buffer) */
};
EGCS_SIZE(CarIcon, 0x2044);

/* vtable 0x00330C30 (4 slots: [3] GameObject's), typeinfo
 * __tf12CarIconMaker 0x001AE468 (si, base GameObject).  vptr +0 (GameObject
 * has no data).  Size 0x18, a stack object of car_icons_make. */
class CarIconMaker : public GameObject {
public:
    CarIconMaker(int count, CarIcon *icons, RaceOrganization *org);    /* 0x001ADA88 frame = 0 */
    /*  [0] +0x0C 0x001AE4B8 the implicit destructor (~GameObject; it stores
     *  no vptr of its own) */
    /* Once the icons are drawn: counts the frames, finish() from the
     * third. */
    virtual void update();                  /* [1] +0x14 0x001ADB70 */
    /* In the first frame every icon (buf is not used). */
    virtual void render(GSBuffer &buf);     /* [2] +0x1C 0x001ADBA8 */
    /* Hands itself to the render thread and sleeps until finish(). */
    void run();                             /* 0x001ADAF0 */
    /* The render thread back to nothing; wakes threadId. */
    void finish();                          /* 0x001ADB40 */
    /* Car `car' of the race into out (64 x 32). */
    void drawIcon(int car, GSBuffer *out);  /* 0x001ADCB8 */
    /* Light 1 (PGL_CAP_LIGHT0 + 1 = 0x66: ambient, diffuse, specular,
     * position from g_002C3EF8 .. g_002C3F28) on, lights 0, 2, 3 off. */
    void setLights();                       /* 0x001AE370 */

    int               count;                /* 0x04 the number of icons */
    CarIcon          *icons;                /* 0x08 */
    RaceOrganization *org;                  /* 0x0C */
    int               threadId;             /* 0x10 the thread waiting for the icons */
    int               frame;                /* 0x14 frames since the icons were drawn */
};
EGCS_SIZE(CarIconMaker, 0x18);

/* The icons of the race's cars (as many as there are, at most 7).
 * Callers: the race modes' virtual_43. */
void car_icons_make(CarIcon *icons, RaceOrganization *org);    /* 0x001AE3F8 */

#endif /* GAME_T184_CAR_ICON_MAKER_H */
