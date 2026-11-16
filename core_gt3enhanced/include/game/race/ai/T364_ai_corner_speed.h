/* T364_ai_corner_speed.h - the computer driver's corner speeds and the
 * corner bookkeeping along its line (translation unit T364, unit u086),
 * 0x001F6678..0x001F6B48.
 *
 * A corner is a node of the enemy line (LineNode, T368): its kind, radius,
 * bank (roll) and surface give the speed v = sqrt(grip x radius / (1 +
 * downforce v^2)), the driver's cornerScale / cornerOffset, speedStep and the
 * node's speedAdj added; at most the top speed, at least 20 km/h. */
#ifndef GAME_T364_AI_CORNER_SPEED_H
#define GAME_T364_AI_CORNER_SPEED_H

#include "game/game.h"

struct AIDriver;                            /* T361_drive_params.h (pointers only) */

/* The speed for node idx of the current line: the front / rear tyre grip on
 * its surface (by gripBalance; with tyre wear, by each wheel's grip), the
 * radius, the bank, the downforce. */
f32  ai_corner_speed(struct AIDriver *ai, int idx);                             /* 0x001F6678 */
/* The next corner's kind / speed become the current ones; the corner at
 * ai->corner (the last one past the end) becomes the next. */
void ai_next_corner(struct AIDriver *ai);                                       /* 0x001F6958 */
/* The first corner ahead of the car's course distance, the one before it
 * (wrapping) as the next, then ai_next_corner (the line changes, T367;
 * DynamicsConductorBattle2P::virtual_17, T309). */
void ai_find_corner(struct AIDriver *ai);                                       /* 0x001F6A20 */

#endif /* GAME_T364_AI_CORNER_SPEED_H */
