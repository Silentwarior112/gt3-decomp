/* T098 - gt2k_menu::HandicapItem.  0x00128D98..0x00129140.
 *
 * The handicap balance bar of the 2-player options: built by the T064 hook
 * option_create_handicap (Object::operator new 0x228) as
 * HandicapItem(&owner->options, p8, item, layout).  The bar picture moves
 * and stretches smoothly (rate 0.15) towards the length the handicap gives. */
/* Retail T098's .rodata (0x003137D8..0x00313A1E): "handi_1p", "handi_2p",
 * the vtable, then the type names HandicapItem, OperatorList,
 * List / OperatorHolder / Cell<OperatorHolder>, the garage's
 * (GarageWheelList .. Cell<GarageCar>), Operator: egcs writes them in the
 * reverse order of the class definitions, so the GameStatus (the garage)
 * comes first, the menu classes (T213, through T098's header) after it. */
#include "game/save/T429_game_status.h"          /* (the garage's type names) */
#include "game/save/T428_game_options.h"         /* GameOptions (splitBattle.handicap) */
#include "game/menu/arcade/T098_handicap_item.h"
#include "game/menu/options/option_items_internal.h"

namespace gt2k_menu {

/* barX / barW: MotionValue() clears all three words of each; the landing
 * pads run bar's and the base's ~Item (flags 0). */
// FUNC 0x00128D98 gt2k_menu::HandicapItem::HandicapItem
HandicapItem::HandicapItem(::GameOptions *options, int player, MenuLayoutItem *res,
                           PageLayout *layout)
    : Item(res, layout), options(options), player(player),
      bar(player == 1 ? "handi_1p" : "handi_2p", layout)
{
    left = x.value - w * 0.5f;
    scale = w / 150.0f;
    f32 barCentre, barLength;
    calcBar(&barCentre, &barLength);
    barX.setValue(barCentre);
    barW.setValue(barLength);
}

/* The bar for the current handicap: *bw its length (0 unless the handicap
 * favours this item's player: player 1 shows -h, player 2 shows h), *bx
 * its centre. */
// FUNC 0x00128EF0 gt2k_menu::HandicapItem::calcBar
void HandicapItem::calcBar(f32 *bx, f32 *bw)
{
    f32 length = (f32)options->splitBattle.handicap * scale;
    *bw = length;
    if (player == 1) {
        if (0.0f < length)
            *bw = 0.0f;
        else
            *bw = -length;
    } else if (length < 0.0f) {
        *bw = 0.0f;
    }
    *bx = left + *bw * 0.5f;
}

/* update: Item's, then the bar picture's (both Item::virtual_1, direct). */
// FUNC 0x00128F70 gt2k_menu::HandicapItem::virtual_1
void HandicapItem::virtual_1()
{
    Item::virtual_1();
    bar.virtual_1();
}

/* draw: the item itself becomes the bar's rectangle; the "handi_?p"
 * picture is drawn at its far end (both Item::virtual_2, direct). */
// FUNC 0x00128FA0 gt2k_menu::HandicapItem::virtual_2
void HandicapItem::virtual_2()
{
    f32 barCentre, barLength;
    calcBar(&barCentre, &barLength);
    barX.approach(barCentre, 0.15f);
    barW.approach(barLength, 0.15f);
    setX(barX.get());
    setW(barW.get());
    Item::virtual_2();
    bar.setX(getScreenX() + barW.get() * 0.5f + bar.getW() * 0.5f);
    bar.setY(getScreenY());
    bar.virtual_2();
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x00129098 __tfQ29gt2k_menu12HandicapItem
extern "C" void *__tfQ29gt2k_menu12HandicapItem(void)
{
    if (__tiQ29gt2k_menu12HandicapItem[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu12HandicapItem, "Q29gt2k_menu12HandicapItem",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu12HandicapItem;
}
#endif

// FUNC 0x001290E8 gt2k_menu::HandicapItem::~HandicapItem
/* the implicit destructor (game/menu/arcade/T098_handicap_item.h), no vptr store:
 * bar.~Item() (flags 2), Item::~Item(this, 0), then Object::operator delete
 * if flags & 1 */
/* implicit: ~Item, operator delete (base / member destructor, deleting destructor) */
