/* T255_cross.h - gt2k_menu::Cross (translation unit T255, unit u071),
 * 0x001CDA48..0x001CE5D8.
 *
 * An Item drawn in its vertex colours around a w x h window centred on its
 * (spring-driven) position: mode 0 draws a cross (a horizontal band through
 * the window and the vertical band above / below it), mode 1 the four corner
 * rectangles of the screen outside the window's row and column.  The
 * position follows targetX / targetY on springs (force 600 per unit, damping
 * 30); with a range set, the coordinate (and the way to the target) wraps
 * modulo the range.  The screen position wraps modulo 680 x 488 (640 x 448
 * plus a 20-pixel margin); the pages scroll it by 680 per page. */
#ifndef GAME_T255_CROSS_H
#define GAME_T255_CROSS_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */

namespace gt2k_menu {

#ifdef BOOT_OLD_GCC
/* From here on (egcs applies it to the classes defined after it), with
 * #pragma implementation in T255_cross.cpp: egcs emits the copies of
 * the inline accessors below and the vtable only there, before __tf. */
#pragma interface
#endif

/* vtable 0x003372F0 (8 slots: [4] Item's, [5] Image's, [6] / [7]
 * Transitive's, [3] Object's).  typeinfo __tfQ29gt2k_menu5Cross 0x001CE540
 * (__si_type_info 0x004B26A0, base Item).  Size 0x114 (Object::operator new
 * in its four callers). */
class Cross : public Item {
public:
    Cross(int mode);                        /* 0x001CDA48 Item(NULL, NULL); targets and ranges 0 */
    /*  [0] +0x0C 0x001CE590 the implicit destructor: only ~Item, no vptr store */
    virtual void virtual_1();               /* [1] +0x14 0x001CE298 update: Item's, then the springs
                                               towards the targets (u071: update) */
    virtual void virtual_2();               /* [2] +0x1C 0x001CE4A8 draw: if visible, Drawable's,
                                               then drawCross / drawCorners by mode (u071: draw) */

    void drawCross();                       /* 0x001CDAA8 mode 0 */
    void drawCorners();                     /* 0x001CDDD0 mode 1 */
    void wrapTargetX(int page);             /* 0x001CE158 targetX mod 680, minus page screens */
    void wrapTargetY(int page);             /* 0x001CE1C0 targetY = targetX mod 488 - page screens (sic) */
    void jumpX(int page);                   /* 0x001CE228 wrapTargetX, then x at rest on targetX */
    void jumpY(int page);                   /* 0x001CE260 wrapTargetY, then y at rest on targetY */
    void setRangeY(f32 r);                  /* 0x001CE510 */
    void setRangeX(f32 r);                  /* 0x001CE518 */
    f32  getTargetY() const;                /* 0x001CE520 */
    f32  getTargetX() const;                /* 0x001CE528 */
    void setTargetY(f32 t);                 /* 0x001CE530 */
    void setTargetX(f32 t);                 /* 0x001CE538 */

    int mode;                               /* 0x100 0: a cross of two bands, 1: the four corners */
    f32 targetX;                            /* 0x104 the pages copy x.value into it (u002 / u003 / u005) */
    f32 targetY;                            /* 0x108 */
    f32 rangeX;                             /* 0x10C non-zero: x wraps modulo rangeX */
    f32 rangeY;                             /* 0x110 */
};

/* The inline accessors 0x001CE510..0x001CE538: no callers, the users expand them; defined
 * in reverse address order (egcs emits the #pragma implementation copies
 * in reverse definition order). */
inline void Cross::setTargetX(f32 t) { targetX = t; }  /* 0x001CE538 */
inline void Cross::setTargetY(f32 t) { targetY = t; }  /* 0x001CE530 */
inline f32 Cross::getTargetX() const { return targetX; }  /* 0x001CE528 */
inline f32 Cross::getTargetY() const { return targetY; }  /* 0x001CE520 */
inline void Cross::setRangeX(f32 r) { rangeX = r; }  /* 0x001CE518 */
inline void Cross::setRangeY(f32 r) { rangeY = r; }  /* 0x001CE510 */
EGCS_SIZE(gt2k_menu::Cross, 0x114);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu5Cross(void);         /* 0x001CE540 */
}
#endif

#endif /* GAME_T255_CROSS_H */
