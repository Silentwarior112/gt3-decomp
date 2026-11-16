/* T429_garage_car_list.h - GarageCarList, the garage's car list (translation
 * unit T429, unit u098; GameStatus holds it, game/save/T429_game_status.h).
 *
 * In its own header since the T074 merge (2026-10-16): T074's .rodata has
 * the type names of GarageCarList, List<GarageCar>, GarageCar and
 * Cell<GarageCar> (0x0031EEB0..0x0031EF20, between MemorycardControl's and
 * MemorycardFileManagerBase's) but none of GarageWheelList's, so the
 * original's T074 saw the car list without the wheel list.
 * T429_game_status.h includes it right after GarageCar's header (u040's data
 * pass, 2026-10-17: the original defined the car and its list, then the
 * wheel and its list; T429_garage_wheel_list.h has the evidence).  It needs
 * GarageCar only (T036_garage_car.h, not T036_garage.h). */
#ifndef GAME_T429_GARAGE_CAR_LIST_H
#define GAME_T429_GARAGE_CAR_LIST_H

#include "game/game.h"
#include "game/menu/framework/menu_list.h"                  /* template gt2k_menu::List (the base) */
#include "game/menu/gtmode/T036_garage_car.h"           /* GarageCar, the element */

/* The garage's car list.  vtable _vt$13GarageCarList 0x00340668 (2 slots:
 * [1] pList's 0x0023C580), typeinfo __tf13GarageCarList 0x0021A900 (si,
 * base List<GarageCar>: vtable 0x00340680, typeinfo 0x0021A978, both in
 * T429).  vptr +0x08 (pList's).  Size 0xC (GameStatus +0x1B6B8).
 *
 * No members of its own: the constructor and the destructor are implicit
 * (GameStatus() / ~GameStatus expand them: List<GarageCar>'s vptr, then
 * ~pList; [0] +0x0C 0x0021A950 is the destructor copy egcs emits for the
 * vtable).  Evidence, retail T429's end-of-unit order (its frame info):
 * __tf13GarageCarList then the destructor, as GarageCar's implicit one
 * (an inline destructor comes before the __tf), and List<GarageCar>::~List
 * 0x0021A508 instantiated by GameStatus() (first among the unit's inline
 * copies), not by an inline constructor or destructor here (u098's data
 * pass, ref/data/u098/notes.md). */
class GarageCarList : public gt2k_menu::List<GarageCar> {
};
EGCS_SIZE(GarageCarList, 0xC);

#endif /* GAME_T429_GARAGE_CAR_LIST_H */
