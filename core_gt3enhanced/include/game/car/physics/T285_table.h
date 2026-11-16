/* T285_table.h - one lookup in a float table of 16 columns (translation unit
 * T285, unit u077), 0x001DE050..0x001DE070.
 *
 * Its caller is RaceEntryCar::updateSound (u042): the rows are its
 * listening modes (race_entry_internal.h LISTEN_*), the columns the sounds of the frame
 * (race_entry_internal.h SND_GAIN_*, 15 used).  The table is T285's .data (0x002D33A8 up
 * to its .eh_frame at 0x002D34E8: 5 rows); the first row starts {3.0, 2.5,
 * 0.5, 0.2, 0.8, 0.3, 1.4, 0.6}. */
#ifndef GAME_T285_TABLE_H
#define GAME_T285_TABLE_H

#include "game/game.h"

extern f32 g_002D33A8[5][16];               /* 0x002D33A8 */

f32 F_001DE050(int row, int col);           /* 0x001DE050 g_002D33A8[row][col]: a sound's gain */

#endif /* GAME_T285_TABLE_H */
