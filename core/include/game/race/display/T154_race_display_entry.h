/* T154_race_display_entry.h - the four empty functions of the race display
 * (translation unit T154, units u051 / u052; the classes are in
 * T154_race_display.h).
 *
 * In their own header since u086's data pass (2026-10-17): the computer
 * driver's line code (T367) calls three of them, and retail's T367 did not
 * see the race display classes (its .rodata 0x0033DD50 is <new>'s
 * "bad_alloc" and UnitArena<64>'s name only; T154_race_display.h gives every
 * includer the meters' and RaceDisplayBase's type names and T157's "").
 * T154_race_display.h includes this header where it declared them. */
#ifndef GAME_T154_RACE_DISPLAY_ENTRY_H
#define GAME_T154_RACE_DISPLAY_ENTRY_H

#include "game/game.h"

void F_001927C0(void);                      /* 0x001927C0 empty, no callers */
void F_001927C8(void);                      /* 0x001927C8 empty (T367 0x001F8780) */
void F_001927D0(int a);                     /* 0x001927D0 empty (T367 0x001F8888: a signed byte) */
/* Empty (T367 0x001F88F8, $a0 set to the value at the call; u051 defined
 * it (void)). */
void F_001927D8(int a);                     /* 0x001927D8 */

#endif /* GAME_T154_RACE_DISPLAY_ENTRY_H */
