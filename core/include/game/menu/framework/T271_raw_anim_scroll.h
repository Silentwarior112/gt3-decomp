/* T271_raw_anim_scroll.h - gt2k_menu::RawAnimScroll (translation unit T271,
 * unit u073), 0x001D29F0..0x001D2C88.
 *
 * Moves its window (its left / top edge) to a target: each frame by a
 * quarter of the distance measured when the scroll started, snapping to the
 * target on (or past) it; an axis whose step is 0 counts as arrived.
 * RawWindow (T264) creates one for every window; the lists
 * (RawList::scrollToFocus 0x001D4848) scroll with it. */
#ifndef GAME_T271_RAW_ANIM_SCROLL_H
#define GAME_T271_RAW_ANIM_SCROLL_H

#include "game/game.h"
#include "game/menu/framework/T270_raw_animation.h"        /* base gt2k_menu::RawAnimation */

namespace gt2k_menu {

/* vtable 0x00337B10 (6 slots: [5] RawAnimation's, [2]..[4] Object's).
 * typeinfo __tfQ29gt2k_menu13RawAnimScroll 0x001D2C38 (__si_type_info
 * 0x004B2710, base RawAnimation).  Size 0x38 (operator new(0x38) in
 * RawWindow's constructor). */
class RawAnimScroll : public RawAnimation {
public:
    RawAnimScroll();                        /* 0x001D29F0 done = 1, the rest 0 */
    virtual ~RawAnimScroll();               /* [0] +0x0C 0x001D2A40 only ~RawAnimation */
    virtual void virtual_1();               /* [1] +0x14 0x001D2A98 one step towards the target */

    void scrollTo(f32 x, f32 y);            /* 0x001D2BB8 starts a scroll of the window's left / top edge
                                               to (x, y); done at once without a window */

    int done;                               /* 0x24 the target is reached (1 initially) */
    f32 targetX;                            /* 0x28 */
    f32 targetY;                            /* 0x2C */
    f32 stepX;                              /* 0x30 a quarter of the distance */
    f32 stepY;                              /* 0x34 */
};
EGCS_SIZE(gt2k_menu::RawAnimScroll, 0x38);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu13RawAnimScroll(void); /* 0x001D2C38 */
}
#endif

#endif /* GAME_T271_RAW_ANIM_SCROLL_H */
