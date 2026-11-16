/* T372_pit_lane.h - line 4 of the enemy lines, the pit lane: is a course
 * distance inside it (translation unit T372, unit u087),
 * 0x001FA5B0..0x001FA680.
 *
 * Callers: DynamicsConductor::chargePenaltyBySurface (slot 39, 0x001DA020,
 * T324) and DynamicsConductor::IsInsidePitRedArea / IsInsidePitYellowArea
 * (0x001ED8D8 / 0x001ED900, T305). */
#ifndef GAME_T372_PIT_LANE_H
#define GAME_T372_PIT_LANE_H

#include "game/game.h"

struct CourseLines;                         /* T368_enemy_lines.h (pointers only) */

/* dist (below half a lap: + a lap) strictly between node margin + 1 and node
 * count - 2 - margin of line 4; 0 without line 4 or with fewer than 6 nodes.
 * The float before the int: both retail callers load $f12 before $a1 (the
 * argument registers are set in parameter order; u083's IsInsidePit*Area are
 * exact only so).  The registers are the same either way (EABI: a0, f12,
 * a1), so the function's own code does not change. */
int  pit_lane_contains(struct CourseLines *cl, f32 dist, int margin);           /* 0x001FA5B0 */

#endif /* GAME_T372_PIT_LANE_H */
