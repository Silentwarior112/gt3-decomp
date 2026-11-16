/* T061_car_get_entry.h - F_0013F508, the entry point of the prize car
 * screens and the ending movies after a race (translation unit T061, unit
 * u021; the controller class is in T061_car_get_control.h, which includes
 * this header where it declared the function).
 *
 * In its own header since u016's data pass (2026-10-17): retail T052's
 * .rodata has neither CarGetControl's type name nor those of the trophy
 * screens whose base it shares (T056): its caller saw the function without
 * the classes (egcs writes the type name of every class it sees whose
 * virtual functions are all inline). */
#ifndef GAME_T061_CAR_GET_ENTRY_H
#define GAME_T061_CAR_GET_ENTRY_H

#include "game/game.h"

/* car_get_prizes (the prize car screens of race `race'; arg: passed on to
 * T419's PrizeCarList), then the ending movies the GameStatus flags
 * (+0x23510) ask for, each once ("stream/ending_short_full.pss",
 * "stream/ending_long_full.pss").  Returns car_get_prizes'.  Callers: the
 * race modes of T052 / T053. */
int  F_0013F508(const char *race, int arg);                 /* 0x0013F508 */

#endif /* GAME_T061_CAR_GET_ENTRY_H */
