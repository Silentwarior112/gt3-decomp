/* T429_garage_wheel_list.h - GarageWheelList, the garage's wheel list
 * (translation unit T429, unit u098; GameStatus holds it,
 * game/save/T429_game_status.h).
 *
 * In its own header since u040's data pass (2026-10-17), as
 * T429_garage_car_list.h: every retail unit that sees the GameStatus has
 * the garage's type names in one order, GarageWheelList, List<GarageWheel>,
 * GarageWheel, Cell<GarageWheel>, GarageCarList, List<GarageCar>,
 * GarageCar, Cell<GarageCar> (T089 0x003250E0..0x003251C8, T090, T091,
 * T092, T115, ...): egcs writes type names in the reverse order of the
 * classes' definitions, so the original defined the car, its list, the
 * wheel, then its list.  T429_game_status.h includes the four headers in
 * that order. */
#ifndef GAME_T429_GARAGE_WHEEL_LIST_H
#define GAME_T429_GARAGE_WHEEL_LIST_H

#include "game/game.h"
#include "game/menu/framework/menu_list.h"                  /* template gt2k_menu::List (the base) */
#include "game/save/T491_garage_wheel.h"         /* GarageWheel, the element */

/* The garage's wheel list.  vtable _vt$15GarageWheelList 0x00340638 (2
 * slots: [1] pList's 0x0023C580), typeinfo __tf15GarageWheelList 0x0021A838
 * (si, base List<GarageWheel>: vtable 0x00340650, typeinfo 0x0021A8B0, both
 * in T429).  vptr +0x08.  Size 0xC (GameStatus +0x1F544).
 *
 * No members of its own, as GarageCarList (T429_garage_car_list.h has the
 * evidence): the constructor and the destructor are implicit (egcs's
 * destructor copy for the vtable: [0] +0x0C 0x0021A888). */
class GarageWheelList : public gt2k_menu::List<GarageWheel> {
};
EGCS_SIZE(GarageWheelList, 0xC);

#endif /* GAME_T429_GARAGE_WHEEL_LIST_H */
