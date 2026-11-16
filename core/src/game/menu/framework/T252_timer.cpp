/* T252 - gt2k_menu::Timer.  0x001CCA88..0x001CCBD0.
 *
 * An Object of the menu tree that counts frames down and then calls
 * func(this, arg) and kills itself (RenderBase::sleep, T202's delayed
 * sounds, PageManager::startPage).
 *
 * Data (retail .rodata 0x00337130..0x00337178): Timer's vtable and type name
 * only. */
#include "game/menu/framework/T252_timer.h"
#include "game/menu/framework/render_base_internal.h"

namespace gt2k_menu {

/* Slot 1, the update (index.txt: gt2k_menu::Timer::virtual_1). */
// FUNC 0x001CCA88 gt2k_menu::Timer::virtual_1
void Timer::virtual_1()
{
    frames = frames - 1;
    if (frames < 0) {
        func(this, arg);
        kill();
    }
}

// FUNC 0x001CCAD0 gt2k_menu::Timer::Timer
/* inline in game/menu/framework/T252_timer.h; egcs emits this copy (no callers): Object(),
 * func / arg / frames, then the vptr. */
/* implicit: Object (base constructor) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Object). */
// FUNC 0x001CCB38 __tfQ29gt2k_menu5Timer
extern "C" void *__tfQ29gt2k_menu5Timer(void)
{
    if (__tiQ29gt2k_menu5Timer[0] == 0) {
        __tfQ29gt2k_menu6Object();
        __rtti_si(__tiQ29gt2k_menu5Timer, "Q29gt2k_menu5Timer",
                  (const type_info *)__tiQ29gt2k_menu6Object);
    }
    return __tiQ29gt2k_menu5Timer;
}
#endif

// FUNC 0x001CCB88 gt2k_menu::Timer::~Timer
/* implicit in game/menu/framework/T252_timer.h; egcs emits this copy (slot [0]):
 * Object::~Object (flags 0), then Object::operator delete with flags & 1;
 * no vptr store. */
/* implicit: ~Object (base destructor), operator delete (deleting destructor) */
