/* T274 - gt2k_menu::RawScrollBar.  0x001D5C10..0x001D5D60.
 *
 * Only the constructor (with the constructor of its 0x14-byte member at
 * +0x1E4), the destructor and the typeinfo function are here; className
 * and the factory are in T272.  Nothing in the image reads the members
 * (? an unfinished class: no slot of its own besides the class name).
 */
#include "game/menu/framework/T274_raw_scroll_bar.h"
#include "game/menu/framework/raw_list_internal.h"

namespace gt2k_menu {

/* (? a scroll range) 0, 0, 100.0f, 10.0f, 0; the floats are stored first. */
// FUNC 0x001D5C10 gt2k_menu::RawScrollBar::Range::Range
RawScrollBar::Range::Range()
{
    max = 100.0f;
    page = 10.0f;
    unk_00 = 0;
    unk_04 = 0;
    unk_10 = 0;
}

/* RawWindow(args), the vptr, Range::Range() on range, then the three
 * words.  The landing pad (0x001D5C88) destroys the RawWindow base and
 * rethrows when the member's constructor throws: the implicit C++
 * behaviour. */
// FUNC 0x001D5C40 gt2k_menu::RawScrollBar::RawScrollBar
RawScrollBar::RawScrollBar(const RawWindowArgs &args)
    : RawWindow(args), range() /* implicit: Range::Range */
{
    unk_1F8 = 0;
    unk_1FC = 0;
    unk_200 = 0;
}

/* index.txt: RawScrollBar::virtual_0.  Range has no destructor. */
// FUNC 0x001D5CB8 gt2k_menu::RawScrollBar::~RawScrollBar
RawScrollBar::~RawScrollBar()
{
    /* implicit: ~RawWindow() and, if (flags & 1), operator delete (added by egcs) */
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base RawWindow). */
// FUNC 0x001D5D10 __tfQ29gt2k_menu12RawScrollBar
extern "C" void *__tfQ29gt2k_menu12RawScrollBar(void)
{
    if (__tiQ29gt2k_menu12RawScrollBar[0] == 0) {
        __tfQ29gt2k_menu9RawWindow();
        __rtti_si(__tiQ29gt2k_menu12RawScrollBar, "Q29gt2k_menu12RawScrollBar",
                  (const type_info *)__tiQ29gt2k_menu9RawWindow);
    }
    return __tiQ29gt2k_menu12RawScrollBar;
}
#endif /* !BOOT_OLD_GCC */
