/* T005_transition.h - TransitionObject and the four screen transitions
 * (translation unit T005, unit u001), 0x00102088..0x00102960.
 *
 * start_transition(type, flag) sets up one of four file-local objects and
 * makes it the render thread's object (T002): type 0 FadeoutTransitionObject
 * g_0049BB40, 1 DelayFadeoutTransitionObject g_0049BB28, 2
 * LineoutTransitionObject g_0049BB10, 3 ComboutTransitionObject g_0049B700
 * (.bss; T246's enum TransitionType names the values); other types do
 * nothing.  Each draws over the last frame after TransitionObject::render's
 * 640 x 448 set-up; the darkening ones subtract their colour (blend
 * PGL_BLEND_SEL(ZERO, CS, AS, CD), 0x42).  After `delay' frames the fade and
 * comb transitions also blink a small square at (20, 20)-(36, 36)
 * (drawBusyMark).  The objects
 * have the implicit constructors (egcs's static initialiser
 * _GLOBAL_$I$init__16TransitionObject 0x00102328: GameObject() and the vptr;
 * the members stay as .bss left them) and destructors (_GLOBAL_$D$...
 * 0x001022D0).
 *
 * The derived classes' render / init and TransitionObject::setFlag are
 * inline in the class bodies, as in the original: egcs emits them with each
 * class's vtable at the end of T005, after the static initialisers and
 * grouped with the class's typeinfo function and implicit destructor; the
 * classes are defined in the order that gives retail's sequence (Combout,
 * Lineout, DelayFadeout, Fadeout, TransitionObject: the reverse of the
 * definition order, as their type names in .rodata). */
#ifndef GAME_T005_TRANSITION_H
#define GAME_T005_TRANSITION_H

#include "game/game.h"
#include "game/app/T002_render_thread.h"        /* base GameObject */
#include "engine/pgl/G001_flare.h"                /* gfx_* / pgl_* / gs_draw_flat_sprite (the inline renders) */
#include "engine/pdistd/T531_random.h"               /* Random (ComboutTransitionObject::init) */

class GSBuffer;                             /* T514 (references only) */

/* The screen the transitions cover, and the blend of the darkening ones:
 * Cd - Cs * As (fix 1.0). */
enum { TRANSITION_SCREEN_W = 640, TRANSITION_SCREEN_H = 448 };
#define TRANSITION_BLEND PGL_BLEND_SEL(PGL_SEL_ZERO, PGL_SEL_CS, PGL_SEL_AS, PGL_SEL_CD)  /* 0x42 */

/* vtable 0x00308A20 (6 slots: [1] / [3] GameObject's).  typeinfo
 * __tf16TransitionObject 0x001028F0 (si, base GameObject; __si_type_info
 * 0x004B1410).  vptr +0x00 (GameObject's).  Size 0x10 (Fadeout adds
 * nothing; the others' data starts at +0x10). */
class TransitionObject : public GameObject {
public:
    /* [0] +0x0C 0x00102940 ~TransitionObject: implicit (no vptr store) */
    /* The 640 x 448 orthographic set-up (pgl_ortho, G001) on buf, the
     * draw state the derived renders expect. */
    virtual void render(GSBuffer &buf);     /* [2] +0x1C 0x00102098 */
    virtual void init();                    /* [4] +0x2C 0x00102088 counter 0, delay 180 */
    virtual int  virtual_5();               /* [5] +0x34 0x00102220 1 */
    /* After `delay' frames: a 16 x 16 square at (20, 20), light grey or
     * black every 32 frames. */
    void drawBusyMark(GSBuffer &buf);       /* 0x00102170 */
    void setFlag(int flag) { this->flag = flag; }   /* 0x001028E8 (the out-of-line copy) */

    int flag;                               /* 0x04 start_transition's flag (not read in T005) */
    int delay;                              /* 0x08 frames before the busy mark shows */
    int counter;                            /* 0x0C the busy mark's blink counter (bit 5) */
};
EGCS_SIZE(TransitionObject, 0x10);

/* vtable 0x003089E8 ([4] TransitionObject::init).  typeinfo
 * __tf23FadeoutTransitionObject 0x00102878 (__si_type_info 0x004B1400).  A
 * dark full-screen rectangle and the busy mark.  Size 0x10. */
class FadeoutTransitionObject : public TransitionObject {
public:
    /* [0] +0x0C 0x001028C8 ~FadeoutTransitionObject: implicit (no vptr store) */
    /* A dark full-screen rectangle (0x0C0C0C0C subtracted) and the busy
     * mark. */
    virtual void render(GSBuffer &buf)      /* [2] +0x1C 0x00102800 */
    {
        TransitionObject::render(buf);
        gfx_enable(PGL_CAP_BLEND);
        gfx_blend_func(TRANSITION_BLEND, 1.0f);
        pgl_color(0x0C0C0C0C);
        gs_draw_flat_sprite(0, 0, TRANSITION_SCREEN_W, TRANSITION_SCREEN_H);
        drawBusyMark(buf);
    }
};
EGCS_SIZE(FadeoutTransitionObject, 0x10);

/* vtable 0x003089B0.  typeinfo __tf28DelayFadeoutTransitionObject
 * 0x00102790 (__si_type_info 0x004B1420).  Nothing for `wait' frames, then
 * a translucent dark full-screen rectangle and the busy mark.  Size 0x14.
 * Here and in the next two classes init is defined before render: egcs
 * emits a class's inline functions in the reverse order (retail: render,
 * init). */
class DelayFadeoutTransitionObject : public TransitionObject {
public:
    /* [0] +0x0C 0x001027E0 ~DelayFadeoutTransitionObject: implicit (no vptr store) */
    virtual void init()                     /* [4] +0x2C 0x00102760 wait 10 */
    {
        TransitionObject::init();
        wait = 10;
    }
    /* Nothing for `wait' frames, then a translucent dark full-screen
     * rectangle (0.1, 0.1, 0.1, alpha 0.5; 0.1 = gp-0x7FE8 0x0034FA08,
     * subtracted) and the busy mark. */
    virtual void render(GSBuffer &buf)      /* [2] +0x1C 0x001026C0 */
    {
        if (wait > 0) {
            wait--;
            return;
        }
        TransitionObject::render(buf);
        gfx_enable(PGL_CAP_BLEND);
        gfx_blend_func(TRANSITION_BLEND, 1.0f);
        pgl_color4f(0.1f, 0.1f, 0.1f, 0.5f);
        gs_draw_flat_sprite(0, 0, TRANSITION_SCREEN_W, TRANSITION_SCREEN_H);
        drawBusyMark(buf);
    }

    int wait;                               /* 0x10 */
};
EGCS_SIZE(DelayFadeoutTransitionObject, 0x14);

/* vtable 0x00308978.  typeinfo __tf23LineoutTransitionObject 0x00102650
 * (__si_type_info 0x004B13F0).  Vertical 1-pixel lines every 2 pixels from
 * both edges, `width' wide, growing by 16 a frame.  Size 0x14. */
class LineoutTransitionObject : public TransitionObject {
public:
    /* [0] +0x0C 0x001026A0 ~LineoutTransitionObject: implicit (no vptr store) */
    virtual void init()                     /* [4] +0x2C 0x00102628 width 0 */
    {
        TransitionObject::init();
        width = 0;
    }
    /* Vertical 1-pixel lines every 2 pixels, black, from both edges,
     * `width' wide, which grows by 16 a frame. */
    virtual void render(GSBuffer &buf)      /* [2] +0x1C 0x00102580 */
    {
        TransitionObject::render(buf);
        gfx_disable(PGL_CAP_BLEND);
        pgl_color(0);
        for (int i = 0; i < width; i += 2) {
            gs_draw_flat_sprite(i, 0, i + 1, TRANSITION_SCREEN_H);
            gs_draw_flat_sprite(TRANSITION_SCREEN_W - 1 - i, 0, TRANSITION_SCREEN_W - i, TRANSITION_SCREEN_H);
        }
        width += 16;
    }

    int width;                              /* 0x10 */
};
EGCS_SIZE(LineoutTransitionObject, 0x14);

/* vtable 0x00308940.  typeinfo __tf23ComboutTransitionObject 0x00102510
 * (__si_type_info 0x004B13E0).  256 two-line bars from the left edge to
 * x[i], each growing by 80 / 3 pixels a frame.  Size 0x410. */
class ComboutTransitionObject : public TransitionObject {
public:
    /* [0] +0x0C 0x00102560 ~ComboutTransitionObject: implicit (no vptr store) */
    /* The bars' ends start at random positions in [-320, 0): a Random with
     * the fixed seed 100 (the inline constructor's 0x87654321 store is
     * dead, T531), stored once before the loop. */
    virtual void init()                     /* [4] +0x2C 0x001024B0 */
    {
        TransitionObject::init();
        Random rng;                         /* sp+0x00 */
        rng.seed = 100;
        for (int i = 0; i < 256; i++)
            x[i] = rng.range(-320.0f, 0.0f);
    }
    /* Bars of 2 lines from x = 0 to x[i], 256 of them, each growing by
     * 80 / 3 pixels a frame (gp-0x7FEC 0x0034FA04 = 0x41D55555), drawn
     * once x[i] >= 1; the colour 0x60202020 is subtracted. */
    virtual void render(GSBuffer &buf)      /* [2] +0x1C 0x001023C0 */
    {
        TransitionObject::render(buf);
        float speed = 26.666666f;
        gfx_enable(PGL_CAP_BLEND);
        gfx_blend_func(TRANSITION_BLEND, 1.0f);
        pgl_color(0x60202020);
        for (int i = 0; i < 256; i++) {
            if (1.0f <= x[i])
                gs_draw_flat_sprite(0, i * 2, (int)x[i], i * 2 + 2);
            x[i] += speed;
        }
        drawBusyMark(buf);
    }

    f32 x[256];                             /* 0x10 the bars' right ends */
};
EGCS_SIZE(ComboutTransitionObject, 0x410);

#include "game/app/T005_transition_entry.h"     /* start_transition */
/* The four objects' static initialiser 0x00102328 (.ctors 0x0028F02C) and
 * destructor 0x001022D0 (.dtors 0x0028F1E4) are egcs's, from their
 * definitions in T005. */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tf23ComboutTransitionObject(void);      /* 0x00102510 */
void *__tf23LineoutTransitionObject(void);      /* 0x00102650 */
void *__tf28DelayFadeoutTransitionObject(void); /* 0x00102790 */
void *__tf23FadeoutTransitionObject(void);      /* 0x00102878 */
void *__tf16TransitionObject(void);             /* 0x001028F0 */
extern u32 __ti23ComboutTransitionObject[3];    /* 0x004B13E0 (__si_type_info) */
extern u32 __ti23LineoutTransitionObject[3];    /* 0x004B13F0 */
extern u32 __ti23FadeoutTransitionObject[3];    /* 0x004B1400 */
extern u32 __ti16TransitionObject[3];           /* 0x004B1410 */
extern u32 __ti28DelayFadeoutTransitionObject[3];   /* 0x004B1420 */
}
#endif

#endif /* GAME_T005_TRANSITION_H */
