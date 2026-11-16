/* T036_decay_item.h - gt2k_menu::DecayItem, the item of the "slow_decay"
 * hook (translation unit T036, game/menu/gtmode/T036_garage.h; src/game/menu/gtmode/T036_garage.cpp).
 *
 * Its own header because of the data layout: T036 is its only user, and
 * retail T036 defined it after every other polymorphic class it saw.  egcs
 * writes a unit's type names in the reverse order of the class definitions
 * (the __tf functions it synthesizes at the end of the unit), and retail
 * T036's run of names (0x00311C70..0x00311EE8) starts with
 * "Q29gt2k_menu9DecayItem" (__tfQ29gt2k_menu9DecayItem 0x00125908 refers to
 * it).  Defined in T036_garage.h (the TU's first include) it came last.  So
 * T036_garage.cpp includes this header after all the others. */
#ifndef GAME_T036_DECAY_ITEM_H
#define GAME_T036_DECAY_ITEM_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */
#include "game/menu/framework/T217_operator.h"             /* gt2k_menu::OperatorCode */

/* The operator script of DecayItem::virtual_4: alpha out over 1.5 s, then
 * the item is killed (T036 .rodata, defined in front of make_slow_decay,
 * where retail has it: after text_spec_max_torque's "%.2t%s/%s", before the
 * hook table's names).  This declaration also gives the const definition
 * external linkage. */
extern const gt2k_menu::OperatorCode g_00310A00[2];    /* 0x00310A00 */

namespace gt2k_menu {

class PageLayout;                           /* T229 (pointers only) */
struct MenuLayoutItem;                      /* T229 */

/* An Item that leaves by a fade (the operator script g_00310A00, T036
 * .rodata) instead of the script it is given.  vtable 0x00311BC8 (8 slots:
 * [1] / [2] Item's 0x001B9F98 / 0x001BA780, [3] Object's 0x001BFF00, [5]
 * Image's 0x001CF358, [6] / [7] Transitive's 0x001BB9F8 / 0x001BBA00).
 * typeinfo __tfQ29gt2k_menu9DecayItem 0x00125908 (si, base Item; storage
 * 0x004B15D0).  No data of its own: size 0x100 (operator new in
 * make_slow_decay).
 *
 * No key function: every member is inline (T036's frame info, ref/data/u010/
 * notes.md).  egcs writes the vtable where the inline constructor needs it
 * (make_slow_decay) and, at the end of the unit, virtual_4 (referenced by
 * the vtable), the typeinfo function and the implicit destructor, in that
 * order (0x001258E8, 0x00125908, 0x00125958); the constructor has no copy
 * (with virtual_4 out of line, the key function, egcs wrote one).
 *
 * The destructor is implicit in the original: 0x00125958 does not store the
 * vptr (~Item with flags 0, then Object's operator delete). */
class DecayItem : public Item {
public:
    /* Inline (expanded in make_slow_decay): Item(res, layout), the vptr. */
    DecayItem(MenuLayoutItem *res, PageLayout *layout) : Item(res, layout) {}
    /*  [0] +0x0C 0x00125958 ~DecayItem: implicit (see above) */
    /* [4] +0x2C 0x001258E8 disappear: starts the fade-out script g_00310A00
     * instead of the caller's script (Transitive's virtual_4 runs the script
     * given, or its default one); script is not used.  Its copy is T036's
     * (marker in src/game/menu/gtmode/T036_garage.cpp). */
    virtual void virtual_4(const OperatorCode *script) {
        start(g_00310A00);                  /* Transitive::start */
    }
};
EGCS_SIZE(gt2k_menu::DecayItem, 0x100);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates and places it itself */
extern "C" {
void *__tfQ29gt2k_menu9DecayItem(void);             /* 0x00125908 */
/* type_info storage of DecayItem, filled by its __tf function. */
extern u32 __tiQ29gt2k_menu9DecayItem[3];           /* 0x004B15D0 __si_type_info */
}
#endif

#endif /* GAME_T036_DECAY_ITEM_H */
