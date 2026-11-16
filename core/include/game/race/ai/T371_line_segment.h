/* T371_line_segment.h - an enemy line at a course distance: the segment
 * containing it, the interpolated point, line 5's arc test (translation unit
 * T371, unit u087), 0x001F9F08..0x001FA5B0.
 *
 * The computer driver's look-ahead point (T366 ai_look_ahead) and the
 * DynamicsConductor's grid / line queries (linePointWorld, T317) use
 * line_point_at, and line_point_at_linear for line 4 (the pit lane, whose
 * distances run on past the lap). */
#ifndef GAME_T371_LINE_SEGMENT_H
#define GAME_T371_LINE_SEGMENT_H

#include "game/game.h"

struct CourseLines;                         /* T368_enemy_lines.h (pointers only) */
struct EnemyLines;                          /* T368_enemy_lines.h */

/* The segment *prev -> *next of `line' containing dist: 1 before the first
 * node, 2 at / after the last one (both: prev = last, next = 0), 3 found
 * (searched outward from node `hint'), 0 not found.  (dist before the
 * pointers: EABI gives the same registers; the callers' argument order is
 * retail's only this way.) */
int  line_find_segment(struct EnemyLines *d, int line, int hint, f32 dist, int *next,
                       int *prev);                                              /* 0x001F9F08 */
/* The point of `line' at course distance dist (wrapped once into the lap):
 * along the straight or along the arc of the segment's first node, the
 * height interpolated.  Returns the segment's first node, -1 if none.
 * (dist before the pointers, as line_find_segment: lines_get_point's
 * argument order is retail's only this way; no other caller changes.) */
int  line_point_at(struct CourseLines *cl, int line, int hint, f32 dist, f32 *x, f32 *y,
                   f32 *height);                                                /* 0x001FA018 */
/* As line_point_at, straight interpolation only, dist below half a lap
 * counting a lap further (line 4).  `unused' is not read.  Returns the
 * segment's first node, -1 outside the line. */
int  line_point_at_linear(struct CourseLines *cl, int line, int unused, f32 *x, f32 *y, f32 *height,
                          f32 dist);                                            /* 0x001FA368 */
/* Is dist on an arc of line 5 (searched from node `hint')?  0 without line 5. */
int  line5_arc_at(struct CourseLines *cl, int hint, f32 dist);                  /* 0x001FA530 */

#endif /* GAME_T371_LINE_SEGMENT_H */
