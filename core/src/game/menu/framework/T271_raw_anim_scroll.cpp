/* T271 - gt2k_menu::RawAnimScroll.  0x001D29F0..0x001D2C88.
 * Moves its window (left / top edge) to a target: each frame by a quarter
 * of the distance measured when the scroll started, snapping to the target
 * on (or past) it.  RawWindow creates one for every window (anim, +0x180);
 * RawList::scrollToFocus scrolls the lists with it. */
#include "game/menu/framework/menu_draw_state_internal.h"

namespace gt2k_menu {

// FUNC 0x001D29F0 gt2k_menu::RawAnimScroll::RawAnimScroll
RawAnimScroll::RawAnimScroll()
    : RawAnimation()
{
    done    = 1;
    targetX = 0.0f;                         /* sw $zero */
    targetY = 0.0f;
    stepX   = 0.0f;
    stepY   = 0.0f;
}

// FUNC 0x001D2A40 gt2k_menu::RawAnimScroll::~RawAnimScroll
RawAnimScroll::~RawAnimScroll()
{
    /* implicit: only ~RawAnimation() (flags 0), then operator delete if flags & 1 */
}

/* One step.  An axis whose step is 0 counts as arrived; done only when
 * both are.  The form retail shows: the new position in its own variables
 * (left / top, copied from the window's x / y: left is then in $f12, not in
 * a saved register), the fields read into locals in their order after the
 * two calls (all four loaded before the first test), done = arrivedX &&
 * arrivedY (a branch: `beqz; v0 = arrivedY'). */
// FUNC 0x001D2A98 gt2k_menu::RawAnimScroll::virtual_1
void RawAnimScroll::virtual_1()
{
    if (done != 0)
        return;

    int arrivedX = 1;
    int arrivedY = 1;
    f32 x = window->getLeft();
    f32 y = window->getTop();
    f32 left = x;
    f32 top  = y;
    f32 tx = targetX;
    f32 ty = targetY;
    f32 sx = stepX;
    f32 sy = stepY;

    if (sx != 0.0f) {
        left += sx;
        if (sx < 0.0f)
            arrivedX = (left <= tx);
        else
            arrivedX = (tx <= left);
        if (arrivedX)
            left = tx;
    }
    if (sy != 0.0f) {
        top += sy;
        if (sy < 0.0f)
            arrivedY = (top <= ty);
        else
            arrivedY = (ty <= top);
        if (arrivedY)
            top = ty;
    }
    window->setLeft(left);
    window->setTop(top);
    done = arrivedX && arrivedY;
}

/* Starts a scroll of the window's left / top edge to (x, y); without a
 * window it is done at once (1 caller: RawList::scrollToFocus). */
// FUNC 0x001D2BB8 gt2k_menu::RawAnimScroll::scrollTo
void RawAnimScroll::scrollTo(f32 x, f32 y)
{
    done = 1;
    if (window != NULL) {
        targetX = x;
        targetY = y;
        f32 dx = x - window->getLeft();
        f32 dy = y - window->getTop();
        done  = 0;
        stepX = dx * 0.25f;
        stepY = dy * 0.25f;
    }
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x001D2C38 __tfQ29gt2k_menu13RawAnimScroll
extern "C" void *__tfQ29gt2k_menu13RawAnimScroll(void)
{
    if (__tiQ29gt2k_menu13RawAnimScroll[0] == 0) {
        __tfQ29gt2k_menu12RawAnimation();
        __rtti_si(__tiQ29gt2k_menu13RawAnimScroll, "Q29gt2k_menu13RawAnimScroll",
                  (const type_info *)__tiQ29gt2k_menu12RawAnimation);
    }
    return __tiQ29gt2k_menu13RawAnimScroll;
}
#endif
