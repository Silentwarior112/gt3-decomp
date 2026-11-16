/* T036_item_text.h - gt2k_menu::ItemText, a class only retail T036 saw
 * (translation unit T036, src/game/menu/gtmode/T036_garage.cpp).
 *
 * Only its name is known: "Q29gt2k_menu8ItemText" (retail 0x00311CD8) is in
 * T036's run of type names, between SmartIndexT<int>'s and Cell<Button>'s,
 * and nowhere else in the binary.  egcs writes a type name (and a __ti
 * common) for every polymorphic class without a key function that a unit
 * sees, and nothing else when the unit does not construct it: T036 has no
 * ItemText vtable, typeinfo function or member, so the class had only inline
 * (or pure) virtual functions and T036 did not use it.  Its __ti common is
 * probably the unreferenced 16-byte slot 0x004B15C0, right before DecayItem's
 * (0x004B15D0, the other class only T036 saw; the commons around them are 16
 * bytes apart).
 *
 * The members below are a stand-in (an exact form, not the original): one
 * inline virtual destructor, which makes the class polymorphic without a key
 * function.  Nothing uses the class; it is here for T036's data layout only,
 * and T036_garage.cpp includes it where retail T036 defined the class (after
 * SmartIndexT<int>, before Button). */
#ifndef GAME_T036_ITEM_TEXT_H
#define GAME_T036_ITEM_TEXT_H

#include "game/game.h"

namespace gt2k_menu {

class ItemText {
public:
    virtual ~ItemText() {}
};

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself (a common) */
extern "C" {
/* type_info storage of ItemText (? the address: see above). */
extern u32 __tiQ29gt2k_menu8ItemText[2];            /* 0x004B15C0 */
}
#endif

#endif /* GAME_T036_ITEM_TEXT_H */
