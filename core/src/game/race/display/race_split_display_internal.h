/* race_split_display_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u052,
 * 0x00193040..0x00196490, second pass: the second part of translation unit
 * T154 (whose source is u051's since the T154 merge: src/u051/
 * T154_race_display.cpp, 0x00193040..0x00195B88 included) and T155
 * (include/game/race/display/T155_race_split_display.h).
 *
 *   T155  RaceSplitDisplay: two RaceDisplays, one per screen.
 *
 * Every class is defined in its canonical header (include/game/).  The
 * include set is part of the match: retail's T155 .rodata has, after its
 * vtable, "16RaceSplitDisplay" and then the race display header's type names
 * only (MiniMap .. RaceDisplayBase; no display events, no garage lists), so
 * T155 sees the display's header and its own, nothing else.
 */
#ifndef U052_H
#define U052_H

#include "types.h"
#ifndef BOOT_OLD_GCC                        /* egcs's <new> (T155) conflicts with it */
#include "cxxrt.h"                          /* __rtti_si, __rtti_user */
#endif

/* own translation unit */
#include "game/race/display/T154_race_display.h"         /* RaceDisplayBase, RaceDisplay (held by value) */
#include "game/race/display/T155_race_split_display.h"   /* RaceSplitDisplay */

#endif /* U052_H */
