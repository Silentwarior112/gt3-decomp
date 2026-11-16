#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/game/menu/framework/T270_raw_animation.cpp"
#pragma implementation "src/game/menu/framework/T270_raw_animation.cpp"
#pragma implementation "T270_raw_animation.h"
#endif
/* T270 - gt2k_menu::RawAnimation.  0x001D28C0..0x001D29F0.
 * The base of the window animations: it only remembers its window. */
#include "game/menu/framework/menu_draw_state_internal.h"

namespace gt2k_menu {

// FUNC 0x001D28C0 gt2k_menu::RawAnimation::RawAnimation
RawAnimation::RawAnimation()
    : Object()
{
    window = NULL;
}

// FUNC 0x001D28F8 gt2k_menu::RawAnimation::~RawAnimation
RawAnimation::~RawAnimation()
{
    /* implicit: only ~Object() (flags 0), then operator delete if flags & 1 */
}

/* Called by RawWindow::virtual_1 and RawList::virtual_9 once a frame. */
// FUNC 0x001D2950 gt2k_menu::RawAnimation::update
void RawAnimation::update()
{
    if (window != NULL)
        virtual_1();
}

// FUNC 0x001D2988 gt2k_menu::RawAnimation::virtual_5
void RawAnimation::virtual_5(RawWindow *w)
{
    window = w;
}

// FUNC 0x001D2990 gt2k_menu::RawAnimation::virtual_1
void RawAnimation::virtual_1()
{
}

/* No callers: an inline member emitted out of line. */
// FUNC 0x001D2998 gt2k_menu::RawAnimation::getWindow
/* inline in game/menu/framework/T270_raw_animation.h; egcs emits this copy (#pragma implementation) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x001D29A0 __tfQ29gt2k_menu12RawAnimation
extern "C" void *__tfQ29gt2k_menu12RawAnimation(void)
{
    if (__tiQ29gt2k_menu12RawAnimation[0] == 0) {
        __tfQ29gt2k_menu6Object();
        __rtti_si(__tiQ29gt2k_menu12RawAnimation, "Q29gt2k_menu12RawAnimation",
                  (const type_info *)__tiQ29gt2k_menu6Object);
    }
    return __tiQ29gt2k_menu12RawAnimation;
}
#endif
