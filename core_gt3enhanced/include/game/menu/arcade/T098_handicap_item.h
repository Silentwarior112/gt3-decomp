/* T098_handicap_item.h - gt2k_menu::HandicapItem, the handicap bar of the
 * 2-player options (translation unit T098, unit u014),
 * 0x00128D98..0x00129140.
 *
 * Built by the option page hook option_create_handicap (T064 0x00140AC0;
 * operator new 0x228) as HandicapItem(&owner->options, arg, item, layout).
 * The item is the bar: it grows from its left edge by
 * |options->splitBattle.handicap| * width / 150 on
 * player 1's item for a negative handicap, on player 2's for a positive one
 * (else length 0), easing there (rate 0.15); the "handi_1p" / "handi_2p"
 * picture is drawn at its far end. */
#ifndef GAME_T098_HANDICAP_ITEM_H
#define GAME_T098_HANDICAP_ITEM_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item, held by value (bar) */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue (barX, barW) */

class GameOptions;                          /* T428 (pointers only) */

namespace gt2k_menu {
class PageLayout;                           /* T229 (pointers only) */
struct MenuLayoutItem;                      /* T229 */

/* vtable 0x003137F8 (8 slots: [3] Object's 0x001BFF00, [4] Item's
 * 0x001BA898, [5] Image's 0x001CF358, [6] / [7] Transitive's).  typeinfo
 * __tfQ29gt2k_menu12HandicapItem 0x00129098 (si, base Item; storage
 * 0x004B1650).  Size 0x228 (operator new in option_create_handicap).  The destructor is
 * implicit in the original: 0x001290E8 does not store the vptr (bar's
 * ~Item with flags 2, ~Item with 0, then Object's operator delete when bit
 * 0 of the flags is set). */
class HandicapItem : public Item {
public:
    /* Item(res, layout), the picture "handi_1p" (player 1) or "handi_2p";
     * left / scale from the item's rectangle; barX / barW start at the
     * current length. */
    HandicapItem(::GameOptions *options, int player, MenuLayoutItem *res,
                 PageLayout *layout);       /* 0x00128D98 */
    /*  [0] +0x0C 0x001290E8 ~HandicapItem: implicit (see above) */
    virtual void virtual_1();               /* [1] +0x14 0x00128F70 update: Item's and bar's */
    /* draw: the item becomes the bar's rectangle (x, w from barX / barW,
     * easing to calcBar's), Item's draw, then bar at its right end. */
    virtual void virtual_2();               /* [2] +0x1C 0x00128FA0 */

    /* *bw the bar's length for the handicap (0 unless it favours this
     * item's player), *bx its centre. */
    void calcBar(f32 *bx, f32 *bw);         /* 0x00128EF0 */

    ::GameOptions *options;                 /* 0x100 the options being edited (OptionControl +0xE58) */
    int          player;                    /* 0x104 1 or 2 (the hook's p8) */
    Item         bar;                       /* 0x108 "handi_1p" / "handi_2p" */
    f32          left;                      /* 0x208 x - w / 2 */
    f32          scale;                     /* 0x20C w / 150: length of one handicap step */
    MotionValue  barX;                      /* 0x210 */
    MotionValue  barW;                      /* 0x21C */
};
EGCS_SIZE(gt2k_menu::HandicapItem, 0x228);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu12HandicapItem(void); /* 0x00129098 */
}
#endif

#endif /* GAME_T098_HANDICAP_ITEM_H */
