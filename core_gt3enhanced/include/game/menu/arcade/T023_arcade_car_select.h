/* T023_arcade_car_select.h - gt2k_menu::TransitiveEX and the spec panel of
 * the arcade car select (translation unit T023, unit u004),
 * 0x00109F20..0x0010C9E0.
 *
 * Most of T023 is the "ar/select_car" page: member functions of
 * gt2k_menu::ArcadeControl, declared with the class in
 * T010_arcade_control.h (runCarSelect, buildCarSelect, the colour chips,
 * the spec texts and gauges, the transmission / racing rows).  The
 * dimension panel of player 1's car (ArcadeControl::specPanel) is a
 * TransitiveEX holding the car's spec; its script g_0030AA58 calls the
 * specPanelShow* functions below, which add the texts as String children
 * (one a second: length, height, width, wheel base, then again). */
#ifndef GAME_T023_ARCADE_CAR_SELECT_H
#define GAME_T023_ARCADE_CAR_SELECT_H

#include "game/game.h"
#include "game/menu/framework/T216_menu_transitive.h"      /* base gt2k_menu::Transitive */

struct MenuCarSpec;                         /* T232 (pointers only) */

namespace gt2k_menu {

/* vtable 0x0030AF98 (8 slots: [1]..[7] Transitive's / Drawable's /
 * Object's).  typeinfo __tfQ29gt2k_menu12TransitiveEX 0x0010C948 (si, base
 * Transitive; storage 0x004B1490).  Size 0xC8 (operator new in
 * ArcadeControl::createSpecPanel).  The destructor is implicit in the
 * original: 0x0010C998 does not store the vptr (egcs stores it in every
 * user-declared destructor, not in the one it generates for a class with
 * one base): ~Transitive, then the deleting part. */
class TransitiveEX : public Transitive {
public:
    TransitiveEX(MenuCarSpec *i) : info(i) {}   /* inline (expanded in createSpecPanel) */
    /*  [0] +0x0C 0x0010C998 ~TransitiveEX: implicit (see above) */

    MenuCarSpec *info;                      /* 0xC4 the car the texts describe */
};
EGCS_SIZE(gt2k_menu::TransitiveEX, 0xC8);

/* The texts of the spec panel (font "sw_20#1", depth 4, the scripts
 * g_0030A898 / g_0030A988), called from the panel's script. */
void specPanelAddText(TransitiveEX *panel, const char *text);  /* 0x0010AC90 a String child */
void specPanelShowLength(TransitiveEX *panel);                 /* 0x0010AD48 "Length\n%dmm" */
void specPanelShowWidth(TransitiveEX *panel);                  /* 0x0010AD90 */
void specPanelShowHeight(TransitiveEX *panel);                 /* 0x0010ADD8 */
void specPanelShowWheelBase(TransitiveEX *panel);              /* 0x0010AE20 "WheelBase\n%dmm" */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu12TransitiveEX(void); /* 0x0010C948 */
}
#endif

#endif /* GAME_T023_ARCADE_CAR_SELECT_H */
