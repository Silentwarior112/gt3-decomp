/* T060_trophy_get_entry.h - trophy_get, the championship cup screen's entry
 * point (translation unit T060, unit u020; the controller class is in
 * T060_trophy_get.h, which includes this header where it declared the
 * function).
 *
 * In its own header since u017's data pass (2026-10-17): of the originals,
 * T056..T058, T060, T099, T104 have TrophyGetControl's type name (and those
 * of its bases), the caller T053 has none of them: it saw trophy_get
 * without the classes (egcs writes the type name of every class it sees
 * whose virtual functions are all inline). */
#ifndef GAME_T060_TROPHY_GET_ENTRY_H
#define GAME_T060_TROPHY_GET_ENTRY_H

#include "game/game.h"

/* 0 if the menu heap cannot be allocated, else 1.  trophy: the cup 0..9, -1
 * a random one.  Caller: T053's run_championship (trophy -1). */
int  trophy_get(int prize, int trophy);     /* 0x0013ADC8 */

#endif /* GAME_T060_TROPHY_GET_ENTRY_H */
