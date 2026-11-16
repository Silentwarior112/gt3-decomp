/* T038_gtmode_main.h - gtmode_main, the GT mode as main() runs it
 * (translation unit T038, unit u013), 0x001274E0..0x00127790. */
#ifndef GAME_T038_GTMODE_MAIN_H
#define GAME_T038_GTMODE_MAIN_H

#include "game/game.h"

/* Runs a GtmodeControl (T043, on the stack, over a 4 MB menu heap, T204)
 * until its pages leave the GT mode; returns the last run()'s result.  Two
 * MenuStatus::nextMode values come back here and restart the GT mode: 0x1D
 * "go_trade" (the memory card screen, T068 mode 0x1D, loads a guest garage
 * into the GameStatusHolder's status, T004; if it did, GtmodeControl::
 * runTrade on a new heap) and 0x1E "go_save_in_home" (T068 mode 0x19 saves
 * the game with a 13 MB work buffer, then MenuStatus::setPage("hm/hm", 4)).
 * The GtmodeControl's implicit destructor is expanded here. */
int gtmode_main(void);                      /* 0x001274E0 (callers: main, T000) */

#endif /* GAME_T038_GTMODE_MAIN_H */
