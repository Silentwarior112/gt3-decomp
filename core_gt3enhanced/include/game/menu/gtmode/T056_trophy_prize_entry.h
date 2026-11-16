/* T056_trophy_prize_entry.h - trophy_prize_screen, the entry point of the
 * mini prize screens after a race (translation unit T056, unit u019; the
 * controller classes are in T056_gtmode_trophy_control.h, which includes
 * this header where it declared the function).
 *
 * In its own header since u016's data pass (2026-10-17): retail T052's
 * .rodata has none of the type names of the trophy screens
 * (MiniPrizeControlBase, ArcadeTrophyControl, GTmodeTrophyControl,
 * ConsolationControl, ControlBaseWithPageManager, the PrizeCounter items):
 * its caller saw the function without the classes (egcs writes the type
 * name of every class it sees whose virtual functions are all inline). */
#ifndef GAME_T056_TROPHY_PRIZE_ENTRY_H
#define GAME_T056_TROPHY_PRIZE_ENTRY_H

#include "game/game.h"

/* After a race: 1 if a screen was shown.  type: the race's trophy type
 * 1..8; rank: 0 first; consolation != 0: the consolation screen.  Callers:
 * the race modes of T052 / T053. */
int  trophy_prize_screen(int prize, int rank, int type, int consolation);  /* 0x00137DC0 */

#endif /* GAME_T056_TROPHY_PRIZE_ENTRY_H */
