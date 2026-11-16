/* T273 - gt2k_menu::RawLabel.  0x001D3E40..0x001D3F70.
 * A list item that only sets its focus picture (its slot 8 / factory are in
 * T272). */
#include "game/menu/framework/menu_draw_state_internal.h"

namespace gt2k_menu {

/* p0: the layout node whose image shows with the focus. */
// FUNC 0x001D3E40 gt2k_menu::RawLabel::RawLabel
RawLabel::RawLabel(const RawWindowArgs &args)
    : RawListItem(args)
{
    const char *focusImage = (const char *)args.p0;
    if (focusImage == NULL)
        focusImage = "label_focus";
    setFocusPropertyIf(focusImage);
    /* EH: ~RawListItem() if it throws */
}

// FUNC 0x001D3EC8 gt2k_menu::RawLabel::~RawLabel
RawLabel::~RawLabel()
{
    /* implicit: only ~RawListItem() (flags 0), then operator delete if flags & 1 */
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x001D3F20 __tfQ29gt2k_menu8RawLabel
extern "C" void *__tfQ29gt2k_menu8RawLabel(void)
{
    if (__tiQ29gt2k_menu8RawLabel[0] == 0) {
        __tfQ29gt2k_menu11RawListItem();
        __rtti_si(__tiQ29gt2k_menu8RawLabel, "Q29gt2k_menu8RawLabel",
                  (const type_info *)__tiQ29gt2k_menu11RawListItem);
    }
    return __tiQ29gt2k_menu8RawLabel;
}
#endif
