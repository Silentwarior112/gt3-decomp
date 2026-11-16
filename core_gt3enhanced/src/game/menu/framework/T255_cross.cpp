#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/game/menu/framework/T255_cross.cpp"
#pragma implementation "src/game/menu/framework/T255_cross.cpp"
#pragma implementation "T255_cross.h"
#endif
/* T255 - gt2k_menu::Cross.  0x001CDA48..0x001CE5D8.
 *
 * An Item drawn in its vertex colours around a w x h window centred on its
 * (spring-driven) position: mode 0 draws a cross (a horizontal band through
 * the window and the vertical band above / below it), mode 1 the four corner
 * rectangles of the screen outside the window's row and column.  The
 * position follows targetX / targetY on springs (force 600 per unit, damping
 * 30); with a range set, the coordinate (and the way to the target) wraps
 * modulo the range.  The screen position wraps modulo 680 x 488 (640 x 448
 * plus a 20-pixel margin).
 *
 * Data (retail .rodata 0x003372F0..0x00337428): Cross's vtable, then the type
 * names Cross, OperatorList, List<OperatorHolder>, OperatorHolder,
 * Cell<OperatorHolder>, Operator: Item's header, then Cross's. */
#include "game/menu/framework/T255_cross.h"                /* (Item's operator classes first) */
#include "game/menu/framework/T259_menu_drawable.h"        /* gt2k_menu::Drawable, MENU_SCREEN_W / MENU_SCREEN_H */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue */
#include "engine/pdistd/T538_fmod_wrap.h"            /* fmod_wrap */
#include "engine/pgl/G001_flare.h"                /* gfx_alloc_tail, pgl_draw_strip, PrimVertex */
#include "game/menu/framework/render_base_internal.h"

namespace gt2k_menu {

/* Item(NULL, NULL), then the members. */
// FUNC 0x001CDA48 gt2k_menu::Cross::Cross
Cross::Cross(int m)
    : Item()
{
    mode = m;
    /* stored as integer zeros */
    targetX = 0;
    targetY = 0;
    rangeX = 0;
    rangeY = 0;
}

/* Mode 0: the horizontal band through the window, the whole width of the
 * screen, and the vertical band above and below it. */
// FUNC 0x001CDAA8 gt2k_menu::Cross::drawCross
void Cross::drawCross()
{
    u32 *colors = (u32 *)gfx_alloc_tail(1);
    getColors(colors);
    f32 cx = fmod_wrap(1, x.value + CROSS_MARGIN, CROSS_WRAP_W) - CROSS_MARGIN;
    f32 cy = fmod_wrap(1, y.value + CROSS_MARGIN, CROSS_WRAP_H) - CROSS_MARGIN;
    PrimVertex *v;

    /* the horizontal band */
    if (0.0f < cy + h * 0.5f && cy - h * 0.5f < MENU_SCREEN_H) {
        v = (PrimVertex *)gfx_alloc_tail(3);
        v[0].x = 0.0f;
        v[0].y = cy - h * 0.5f;
        v[0].z = 0.0f;
        v[1].x = MENU_SCREEN_W;
        v[1].y = cy - h * 0.5f;
        v[1].z = 0.0f;
        v[2].x = 0.0f;
        v[2].y = cy + h * 0.5f;
        v[2].z = 0.0f;
        v[3].x = MENU_SCREEN_W;
        v[3].y = cy + h * 0.5f;
        v[3].z = 0.0f;
        pgl_draw_strip(v, NULL, colors, NULL, 4);
    }

    /* the vertical band, above and below the horizontal one */
    if (0.0f < cx + w * 0.5f && cx - w * 0.5f < MENU_SCREEN_W) {
        if (0.0f < cy - h * 0.5f) {
            v = (PrimVertex *)gfx_alloc_tail(3);
            v[0].x = cx - w * 0.5f;
            v[0].y = 0.0f;
            v[0].z = 0.0f;
            v[1].x = cx + w * 0.5f;
            v[1].y = 0.0f;
            v[1].z = 0.0f;
            v[2].x = cx - w * 0.5f;
            v[2].y = cy - h * 0.5f;
            v[2].z = 0.0f;
            v[3].x = cx + w * 0.5f;
            v[3].y = cy - h * 0.5f;
            v[3].z = 0.0f;
            pgl_draw_strip(v, NULL, colors, NULL, 4);
        }
        if (cy + h * 0.5f < MENU_SCREEN_H) {
            v = (PrimVertex *)gfx_alloc_tail(3);
            v[0].x = cx - w * 0.5f;
            v[0].y = cy + h * 0.5f;
            v[0].z = 0.0f;
            v[1].x = cx + w * 0.5f;
            v[1].y = cy + h * 0.5f;
            v[1].z = 0.0f;
            v[2].x = cx - w * 0.5f;
            v[2].y = MENU_SCREEN_H;
            v[2].z = 0.0f;
            v[3].x = cx + w * 0.5f;
            v[3].y = MENU_SCREEN_H;
            v[3].z = 0.0f;
            pgl_draw_strip(v, NULL, colors, NULL, 4);
        }
    }
}

/* Mode 1: the four corners of the screen outside the window's row and
 * column. */
// FUNC 0x001CDDD0 gt2k_menu::Cross::drawCorners
void Cross::drawCorners()
{
    u32 *colors = (u32 *)gfx_alloc_tail(1);
    getColors(colors);
    f32 cx = fmod_wrap(1, x.value + CROSS_MARGIN, CROSS_WRAP_W) - CROSS_MARGIN;
    f32 cy = fmod_wrap(1, y.value + CROSS_MARGIN, CROSS_WRAP_H) - CROSS_MARGIN;
    PrimVertex *v;

    if (0.0f < cy - h * 0.5f) {
        /* top left */
        if (0.0f < cx - w * 0.5f) {
            v = (PrimVertex *)gfx_alloc_tail(3);
            v[0].x = 0.0f;
            v[0].y = 0.0f;
            v[0].z = 0.0f;
            v[1].x = cx - w * 0.5f;
            v[1].y = 0.0f;
            v[1].z = 0.0f;
            v[2].x = 0.0f;
            v[2].y = cy - h * 0.5f;
            v[2].z = 0.0f;
            v[3].x = cx - w * 0.5f;
            v[3].y = cy - h * 0.5f;
            v[3].z = 0.0f;
            pgl_draw_strip(v, NULL, colors, NULL, 4);
        }
        /* top right */
        if (cx + w * 0.5f < MENU_SCREEN_W) {
            v = (PrimVertex *)gfx_alloc_tail(3);
            v[0].x = cx + w * 0.5f;
            v[0].y = 0.0f;
            v[0].z = 0.0f;
            v[1].x = MENU_SCREEN_W;
            v[1].y = 0.0f;
            v[1].z = 0.0f;
            v[2].x = cx + w * 0.5f;
            v[2].y = cy - h * 0.5f;
            v[2].z = 0.0f;
            v[3].x = MENU_SCREEN_W;
            v[3].y = cy - h * 0.5f;
            v[3].z = 0.0f;
            pgl_draw_strip(v, NULL, colors, NULL, 4);
        }
    }
    if (cy + h * 0.5f < MENU_SCREEN_H) {
        /* bottom left */
        if (0.0f < cx - w * 0.5f) {
            v = (PrimVertex *)gfx_alloc_tail(3);
            v[0].x = 0.0f;
            v[0].y = cy + h * 0.5f;
            v[0].z = 0.0f;
            v[1].x = cx - w * 0.5f;
            v[1].y = cy + h * 0.5f;
            v[1].z = 0.0f;
            v[2].x = 0.0f;
            v[2].y = MENU_SCREEN_H;
            v[2].z = 0.0f;
            v[3].x = cx - w * 0.5f;
            v[3].y = MENU_SCREEN_H;
            v[3].z = 0.0f;
            pgl_draw_strip(v, NULL, colors, NULL, 4);
        }
        /* bottom right */
        if (cx + w * 0.5f < MENU_SCREEN_W) {
            v = (PrimVertex *)gfx_alloc_tail(3);
            v[0].x = cx + w * 0.5f;
            v[0].y = cy + h * 0.5f;
            v[0].z = 0.0f;
            v[1].x = MENU_SCREEN_W;
            v[1].y = cy + h * 0.5f;
            v[1].z = 0.0f;
            v[2].x = cx + w * 0.5f;
            v[2].y = MENU_SCREEN_H;
            v[2].z = 0.0f;
            v[3].x = MENU_SCREEN_W;
            v[3].y = MENU_SCREEN_H;
            v[3].z = 0.0f;
            pgl_draw_strip(v, NULL, colors, NULL, 4);
        }
    }
}

/* targetX modulo 680 (positive), minus `page' screens of 680. */
// FUNC 0x001CE158 gt2k_menu::Cross::wrapTargetX
void Cross::wrapTargetX(int page)
{
    f32 inScreen = fmod_wrap(1, targetX, CROSS_WRAP_W);
    targetX = (f32)(-page) * CROSS_WRAP_W + inScreen;
}

/* The same for targetY - but it wraps targetX (a bug of the original: the
 * load is +0x104, the store +0x108). */
// FUNC 0x001CE1C0 gt2k_menu::Cross::wrapTargetY
void Cross::wrapTargetY(int page)
{
    f32 inScreen = fmod_wrap(1, targetX, CROSS_WRAP_H);    /* sic: targetX */
    targetY = (f32)(-page) * CROSS_WRAP_H + inScreen;
}

/* wrapTargetX, then x at rest on the target. */
// FUNC 0x001CE228 gt2k_menu::Cross::jumpX
void Cross::jumpX(int page)
{
    wrapTargetX(page);
    x.set(targetX);                         /* MotionValue::set, inline */
}

// FUNC 0x001CE260 gt2k_menu::Cross::jumpY
void Cross::jumpY(int page)
{
    wrapTargetY(page);
    y.set(targetY);
}

/* (? names) The wrap of virtual_1, on a MotionValue (retail keeps &x / &y
 * in $s2 / $s1 for them): the spring force towards goal the short way round
 * [lo, hi), and the value wrapped into [lo, hi) (value - lo stored first). */
static inline void wrap_value(MotionValue *m, f32 lo, f32 hi)
{
    m->value = m->value - lo;
    m->value = fmod_wrap(1, m->value, hi - lo) + lo;
}

static inline void pull_wrapped(MotionValue *m, f32 goal, f32 k, f32 lo, f32 hi)
{
    /* one variable for the distance through the steps (written as one
     * expression, lo and k swap $f20 / $f21: 88.8%) */
    f32 d = goal - m->value;
    d = fmod_wrap(1, d - lo, hi - lo);
    d = d + lo;
    m->force += d * k;
}

/* Slot 1, the update (index.txt: gt2k_menu::Cross::virtual_1): Item's,
 * then x / y pulled towards the targets (the short way round when
 * wrapping), both springs stepped, then the values wrapped into
 * [-range / 2, range / 2). */
// FUNC 0x001CE298 gt2k_menu::Cross::virtual_1
void Cross::virtual_1()
{
    /* implicit: fmod_wrap (in the inlined helpers) */
    Item::virtual_1();

    if (rangeX != 0.0f)
        pull_wrapped(&x, targetX, CROSS_SPRING, -rangeX * 0.5f, rangeX * 0.5f);
    else
        x.pull(targetX, CROSS_SPRING);
    if (rangeY != 0.0f)
        pull_wrapped(&y, targetY, CROSS_SPRING, -rangeY * 0.5f, rangeY * 0.5f);
    else
        y.pull(targetY, CROSS_SPRING);

    x.update(CROSS_DAMPING);
    y.update(CROSS_DAMPING);

    if (rangeX != 0.0f)
        wrap_value(&x, -rangeX * 0.5f, rangeX * 0.5f);
    if (rangeY != 0.0f)
        wrap_value(&y, -rangeY * 0.5f, rangeY * 0.5f);
}

/* Slot 2, the draw (index.txt: gt2k_menu::Cross::virtual_2). */
// FUNC 0x001CE4A8 gt2k_menu::Cross::virtual_2
void Cross::virtual_2()
{
    if (visible != 0) {
        Drawable::virtual_2();
        switch (mode) {
        case 0:
            drawCross();
            break;
        case 1:
            drawCorners();
            break;
        }
    }
}

// FUNC 0x001CE510 gt2k_menu::Cross::setRangeY
/* inline in game/menu/framework/T255_cross.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001CE518 gt2k_menu::Cross::setRangeX
/* inline in game/menu/framework/T255_cross.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001CE520 gt2k_menu::Cross::getTargetY
/* inline in game/menu/framework/T255_cross.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001CE528 gt2k_menu::Cross::getTargetX
/* inline in game/menu/framework/T255_cross.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001CE530 gt2k_menu::Cross::setTargetY
/* inline in game/menu/framework/T255_cross.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001CE538 gt2k_menu::Cross::setTargetX
/* inline in game/menu/framework/T255_cross.h; egcs emits this copy (#pragma implementation) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x001CE540 __tfQ29gt2k_menu5Cross
extern "C" void *__tfQ29gt2k_menu5Cross(void)
{
    if (__tiQ29gt2k_menu5Cross[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu5Cross, "Q29gt2k_menu5Cross",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu5Cross;
}
#endif

// FUNC 0x001CE590 gt2k_menu::Cross::~Cross
/* implicit in game/menu/framework/T255_cross.h; egcs emits this copy (slot [0]):
 * Item::~Item (flags 0), then Object::operator delete with flags & 1; no
 * vptr store. */
/* implicit: ~Item (base destructor), operator delete (deleting destructor) */
