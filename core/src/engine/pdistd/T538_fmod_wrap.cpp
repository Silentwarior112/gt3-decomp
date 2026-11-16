/* T538 - a float modulo helper (.text 0x0026E2C8..0x0026E310), a C++
 * translation unit (it has an FDE) with this one function. */
#include "engine/pdistd/T538_fmod_wrap.h"   /* (only its own header: the original saw no class) */

// FUNC 0x0026E2C8 fmod_wrap
/* x mod y with truncation: rem = x - (f32)(s32)(x / y) * y (div.s, cvt.w.s
 * truncates toward zero, cvt.s.w, mul.s, sub.s).  If `wrap' is non-zero and
 * x (not rem) is negative, y is added (bc1tl: the add.s runs only when
 * x < 0.0f), bringing a negative remainder into [0, y) - so for a negative
 * exact multiple of y the result is y, not 0.
 * Callers (12 call sites, every one with wrap = 1): gt2k_menu::Cross (T255)
 * drawCross, drawCorners, wrapTargetX, wrapTargetY and virtual_1 - the
 * screen-space wrap of the cross-hair (y = 680.0f / 488.0f; virtual_1 wraps a
 * difference into [-w/2, w/2) as fmod_wrap(1, d + w/2, w) - w/2, with w read
 * from the object) - and MenuCarCamera::virtual_8 (T411, y = 360.0f: an angle
 * in degrees).
 * The function starts with two nops: they are inside its FDE (pc 0x0026E2C8..
 * 0x0026E30C) and are the R5900 FPU hazard padding the retail compiler put
 * in front of every div.s (tools/mcc.py's div_nops pass reproduces it). */
f32 fmod_wrap(int wrap, f32 x, f32 y)
{
    f32 rem;

    rem = x - (f32)(s32)(x / y) * y;
    if (wrap != 0 && x < 0.0f)
        rem += y;
    return rem;
}
