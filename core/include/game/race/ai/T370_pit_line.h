/* T370_pit_line.h - line 4 of the enemy lines, the pit lane: node values and
 * the pose of a pit slot (translation unit T370, unit u087),
 * 0x001F9D90..0x001F9F08.
 *
 * A slot is counted from the lane's end: slot i is node count - (i + 4).
 * DynamicsConductor's pit gates / boxes (pitSlotNode .. pitNodeAngle, T317)
 * read the node distances and headings; the car set-up (F_001DDAD0, T330)
 * places a car at a slot (start types 6 / 7). */
#ifndef GAME_T370_PIT_LINE_H
#define GAME_T370_PIT_LINE_H

#include "game/game.h"

struct CourseLines;                         /* T368_enemy_lines.h (pointers only) */

/* The node of slot `slot': count - (slot + 4); 0 without line 4. */
int  pit_slot_node(struct CourseLines *cl, int slot);                           /* 0x001F9D90 */
/* Node i's course distance, i < 0 counted from the end. */
f32  pit_node_dist_wrapped(struct CourseLines *cl, int i);                      /* 0x001F9DB8 */
f32  pit_node_dist(struct CourseLines *cl, int i);                              /* 0x001F9DF0 */
/* Node i's heading, rad. */
f32  pit_node_angle(struct CourseLines *cl, int i);                             /* 0x001F9E10 */
/* The pose of slot `slot': pos[3] the node's x, y moved 0.5 m forwards along
 * its heading ((-sin, +cos) of it, the lines' forward direction:
 * lines_get_point) and its height; rot[3] pitch, roll, heading; *sector the
 * node's runway sector. */
void pit_slot_pose(struct CourseLines *cl, int slot, f32 *pos, f32 *rot, int *sector); /* 0x001F9E30 */

#endif /* GAME_T370_PIT_LINE_H */
