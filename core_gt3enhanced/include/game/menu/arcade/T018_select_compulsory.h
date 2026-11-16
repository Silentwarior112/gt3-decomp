/* T018_select_compulsory.h - translation unit T018 (unit u002),
 * 0x00103B70..0x00104EE0: the arcade "ar/select_compulsory" page.
 *
 * Its functions are members of gt2k_menu::ArcadeControl, declared with the
 * class in T010_arcade_control.h (compulsoryMiniName .. selectCompulsory).
 * File-local data: the member pointer constant 0x00350BF0
 * ({0, -1, compulsoryMiniName}, for createMiniCursor) and the strings.
 *
 * The plan put three classes here that are other units' classes under the
 * first pass's names; they are the same objects (offsets, methods and
 * sizes agree), so this header only maps the names (phase 2: use the
 * canonical names):
 *   MenuCar                 = MenuCarSpec (T232; u002 reads its weight
 *                             +0x2A, u007 its equip +0x1348 and current
 *                             +0x1530; constructor 0x001C4210, setCar(name)
 *                             0x001C4248)
 *   gt2k_menu::MenuArchive  = gt2k_menu::PageLayout (T229; u002 / u065 /
 *                             u069 call its findHiddenItem 0x001C2768 and
 *                             getTexture 0x001C27A8)
 *   gt2k_menu::MenuCoord    = MotionValue (T260; {unk0, unk4, value} =
 *                             {force, velocity, value}, u069's set(v)) */
#ifndef GAME_T018_SELECT_COMPULSORY_H
#define GAME_T018_SELECT_COMPULSORY_H

#include "game/game.h"
#include "game/menu/gtmode/T232_menu_car_spec.h"        /* MenuCarSpec */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue */

typedef MenuCarSpec MenuCar;                /* 0x1538 bytes */

namespace gt2k_menu {
typedef PageLayout    MenuArchive;
typedef ::MotionValue MenuCoord;            /* 0xC bytes */
}

#endif /* GAME_T018_SELECT_COMPULSORY_H */
