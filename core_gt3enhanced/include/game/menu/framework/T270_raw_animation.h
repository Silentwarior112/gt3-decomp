/* T270_raw_animation.h - gt2k_menu::RawAnimation (translation unit T270, unit
 * u073), 0x001D28C0..0x001D29F0.
 *
 * The base of the window animations: it only remembers its window.  Every
 * RawWindow (T264) keeps one (a RawAnimScroll, T271) and runs it once a
 * frame through update(). */
#ifndef GAME_T270_RAW_ANIMATION_H
#define GAME_T270_RAW_ANIMATION_H

#include "game/game.h"
#include "game/menu/framework/T220_object.h"               /* base gt2k_menu::Object */

namespace gt2k_menu {
class RawWindow;                            /* T264 (pointers only) */

#ifdef BOOT_OLD_GCC
/* From here on (egcs applies it to the classes defined after it), with
 * #pragma implementation in T270_raw_animation.cpp: egcs emits the copies of
 * the inline accessors below and the vtable only there, before __tf. */
#pragma interface
#endif

/* vtable 0x003379E0 (6 slots: [2]..[4] Object's).  typeinfo
 * __tfQ29gt2k_menu12RawAnimation 0x001D29A0 (__si_type_info 0x004B2700, base
 * Object).  Size 0x24 (RawAnimScroll's members start at +0x24). */
class RawAnimation : public Object {
public:
    RawAnimation();                         /* 0x001D28C0 window = NULL */
    virtual ~RawAnimation();                /* [0] +0x0C 0x001D28F8 only ~Object */
    virtual void virtual_1();               /* [1] +0x14 0x001D2990 one step (empty here) */
    virtual void virtual_5(RawWindow *w);   /* [5] +0x34 0x001D2988 window = w */

    void       update();                    /* 0x001D2950 virtual_1() if there is a window (RawWindow's
                                               virtual_1, RawList::virtual_9) */
    RawWindow *getWindow();                 /* 0x001D2998 (inline in the original; unreferenced copy) */

    RawWindow *window;                      /* 0x20 the window it moves */
};

/* The inline accessors 0x001D2998..0x001D2998: no callers, the users expand them; defined
 * in reverse address order (egcs emits the #pragma implementation copies
 * in reverse definition order). */
inline RawWindow *RawAnimation::getWindow() { return window; }  /* 0x001D2998 */
EGCS_SIZE(gt2k_menu::RawAnimation, 0x24);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu12RawAnimation(void); /* 0x001D29A0 */
}
#endif

#endif /* GAME_T270_RAW_ANIMATION_H */
