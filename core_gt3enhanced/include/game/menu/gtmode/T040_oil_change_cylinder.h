/* T040_oil_change_cylinder.h - gt2k_menu::OilChangeCylinder, the oil change
 * animation of the GT-mode shop (translation unit T040, unit u008),
 * 0x00116E00..0x00117320.
 *
 * After start(): wait 60 frames, sound 6, drain (level 1 -> 0 by 0.01 a
 * frame), wait 60 frames, clean (dirt = 0), sound 6, refill (0 -> 1), then
 * PageManager::setFreeze(0) and idle again.  The oil is the picture `oil'
 * ("dirty"), drawn with its bottom edge fixed and level times the full
 * height, its vertex colours mixed between `clean' and `dirty' by dirt.
 *
 * gt2k_menu::Color4f (u008, u013, u027, u070) and gt2k_menu::TValue (u008,
 * u013, u040, u063, u067, u068, u070) are first-pass names of the global
 * Color4f (T218) and MotionValue (T260): the same layouts, the vertex
 * colours and coordinates of Drawable (T259); they are typedefs here.  (The
 * draw below default-constructs a Color4f temporary: its four dead stores
 * of 0.502 (0x3F008312) show that Color4f() sets 0.502 in the original.) */
#ifndef GAME_T040_OIL_CHANGE_CYLINDER_H
#define GAME_T040_OIL_CHANGE_CYLINDER_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */
#include "game/menu/framework/T218_color.h"                /* Color4f (gt2k_menu::Color4f) */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue (gt2k_menu::TValue) */

namespace gt2k_menu {
class PageManager;                          /* T211 (pointers only) */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */

/* gt2k_menu::Color4f is ::Color4f (T218), found from inside the namespace; a
 * typedef here would be ambiguous under `using namespace gt2k_menu` (egcs). */
typedef ::MotionValue TValue;               /* 0xC bytes: force, velocity, value (the first pass's
                                               unk0, unk4, value) */

/* vtable 0x0030E508 (8 slots: [3] Object's 0x001BFF00, [4] Item's
 * 0x001BA898, [5] Image's 0x001CF358, [6] / [7] Transitive's).  typeinfo
 * __tfQ29gt2k_menu17OilChangeCylinder 0x001172D0 (si, base Item; storage
 * 0x004B1550).  Size 0x124 (operator new in T036 F_00123048). */
class OilChangeCylinder : public Item {
public:
    /* Item(res, layout); level 1; the pictures "dirty", "clean", "dirty".
     * dirt is the second parameter (it still travels in $f12, res and
     * layout in $a2 / $a3): the one caller, T036's make_oil_change_cylinder
     * 0x00123048, sets $f12 before $a2 / $a3, which egcs does only in this
     * order (exact; with dirt last it sets it after them). */
    OilChangeCylinder(PageManager *pages, f32 dirt, MenuLayoutItem *res,
                      PageLayout *layout);  /* 0x00116E00 */
    virtual ~OilChangeCylinder();           /* [0] +0x0C 0x00116F70 deletes dirty and clean (oil is
                                               never deleted) */
    virtual void virtual_1();               /* [1] +0x14 0x00117018 Item's, oil's, then the drain /
                                               refill steps (state 0 .. 5) */
    virtual void virtual_2();               /* [2] +0x1C 0x00117170 draw: oil, then this item */
    void start();                           /* 0x00117008 (T036 F_00126B98) */

    PageManager *pages;                     /* 0x100 told when the refill is done */
    Item *dirty;                            /* 0x104 "dirty" (colour and size source) */
    Item *clean;                            /* 0x108 "clean" (colour source) */
    Item *oil;                              /* 0x10C "dirty", the picture drawn */
    f32   dirt;                             /* 0x110 the colour mix: 1 dirty, 0 clean */
    f32   level;                            /* 0x114 1: full */
    int   started;                          /* 0x118 start() */
    int   state;                            /* 0x11C 0 .. 5 */
    int   timer;                            /* 0x120 frames */
};
EGCS_SIZE(gt2k_menu::OilChangeCylinder, 0x124);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu17OilChangeCylinder(void);    /* 0x001172D0 */
#endif

#endif /* GAME_T040_OIL_CHANGE_CYLINDER_H */
