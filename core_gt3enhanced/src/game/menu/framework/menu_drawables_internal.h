/* menu_drawables_internal.h - unit u072 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001CE5D8..0x001D1AE0, second pass.
 *
 * Translation units (link order):
 *   T256  gt2k_menu::Rectangle: a Transitive drawn as a flat quad (or the
 *         16-tap blur), with an optional bevelled / shadowed frame
 *   T257  gt2k_menu::Image (a Rectangle textured from a Tex1 image) and
 *         gt2k_menu::ImageGZ (an Image that inflates its own gzip'd image)
 *   T258  gt2k_menu::String: a Transitive that prints a text with the
 *         system font
 *   T259  gt2k_menu::Drawable: position, size, vertex colours, blur,
 *         anchor and draw settings of every menu object; vertex helpers
 *   T260  MotionValue: a spring-driven coordinate (force, velocity, value)
 *
 * Every class is defined in its canonical header (include/game/T256..T260);
 * this header keeps what only this unit uses: named constants of the render
 * state, the frame and anchor codes, and the typeinfo storage of ImageGZ and
 * String.  It includes no class header (u072's data pass): each source
 * includes the canonical headers it needs itself, in the order that gives
 * the original's type names (a header that defines a class with virtual
 * functions puts its type name into every object that reads it; T259's
 * original saw the GameStatus but no menu operator class, T260's no class).
 *
 * egcs ABI reminder (ref/CONTEXT.md): the vptr is a data member after the
 * data of the first polymorphic class of the hierarchy (Object, vptr +0x1C).
 * Offsets in the comments are the original (egcs) ones; GCC 14 puts the vptr
 * at +0x00 and moves every member of these classes by 4.
 */
#ifndef U072_H
#define U072_H

#include "types.h"
#include <stddef.h>

#include "engine/pgl/G001_flare.h"                /* PGL_BLEND_SEL / PGL_SEL_* (the blends below); no type names */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* type_info, __rtti_si (GCC 14 only) */
#endif

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* The menus' virtual screen: MENU_SCREEN_* (T261_menu_draw_state.h, scaled
 * to the buffer drawn, g_00352758). */

/* gfx_blend_func selectors (G001 PGL_BLEND_SEL: colour = (A - B) * C + D). */
enum {
    MENU_BLEND_ALPHA   = PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD),   /* 0x44 (Cs - Cd) * As + Cd */
    MENU_BLEND_ADD     = PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_AS, PGL_SEL_CD), /* 0x48 Cs * As + Cd */
    MENU_BLEND_SUB     = PGL_BLEND_SEL(PGL_SEL_ZERO, PGL_SEL_CS, PGL_SEL_AS, PGL_SEL_CD), /* 0x42 Cd - Cs * As */
    MENU_BLEND_NO_DEST = PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_ZERO), /* 0x84 (Cs - Cd) * As */
    MENU_BLEND_FIX     = PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_FIX, PGL_SEL_CD)   /* 0x64 (Cs - Cd) * FIX + Cd */
};

/* Drawable::drawFlags: T259's DRAW_* (DRAW_BLEND_* select MENU_BLEND_ALPHA /
 * _ADD / _SUB / _NO_DEST with the alpha test); Drawable::anchorType: T259's
 * ANCHOR_*. */

/* gt2k_menu::Object::condition: Object::CONDITION_* (T220). */

/* Rectangle::frameType: T256's FRAME_* (see drawRaised for what the first two
 * look like). */

/* The frame colour: grey 0x80, alpha 0x80 (1.0 for the GS). */
#define FRAME_GREY 0x80808080u

/* Rectangle::shiftByAlign: G001 font_set_align's bits (bits 0-1 horizontal,
 * 2-3 vertical; 3 / 0xC centre). */
enum {
    ALIGN_H_MASK = 0x3, ALIGN_LEFT = 0x1, ALIGN_RIGHT = 0x2,
    ALIGN_V_MASK = 0xC, ALIGN_TOP  = 0x4, ALIGN_BOTTOM = 0x8
};

/* Drawable::drawBlur: the taps of g_00337830. */
#define BLUR_TAPS 16

/* String's font style without one of its own (T258 .sdata 0x00352708). */
#define STRING_DEFAULT_FONT "sw_16"

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself; these are for GCC 14 only.
 * The __si_type_info storage (.bss, 3 words) of the classes only their own
 * typeinfo function reads (Rectangle's, Image's and Drawable's are declared
 * by T256 / T257 / T259). */
extern "C" {
extern u32 __tiQ29gt2k_menu7ImageGZ[3];     /* 0x004B26D0 */
extern u32 __tiQ29gt2k_menu6String[3];      /* 0x004B26E0 */
}
#endif /* !BOOT_OLD_GCC */

#endif /* U072_H */
