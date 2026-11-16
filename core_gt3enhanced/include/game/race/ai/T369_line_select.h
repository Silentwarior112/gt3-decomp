/* T369_line_select.h - choosing an enemy line (translation unit T369, unit
 * u087), 0x001F9BF8..0x001F9D90.
 *
 * lines_select gives the first existing line of a line kind's fallback list
 * (g_002DA508, u087: kind -> up to 6 lines, -1 ends; kind >= 8 is taken as
 * 6):  0 {-1}  1 {1, 6, 2}  2 {2, 6, 1}  3 {3}  4 {4}  5 {-1}  6 {6, 1, 2}
 * 7 {7, 6}.  The side lines of a kind are 1 and 2 (around the racing line 6);
 * which one is "this side" flips on line 5's arcs (T371 line5_arc_at).  The
 * computer driver uses them to move out of the way of other cars (T365). */
#ifndef GAME_T369_LINE_SELECT_H
#define GAME_T369_LINE_SELECT_H

#include "game/game.h"

struct CourseLines;                         /* T368_enemy_lines.h (pointers only) */
struct EnemyLines;                          /* T368_enemy_lines.h */

/* The line to use for `kind': the first line of its list that exists; -1 if
 * none (or no line file). */
int  lines_select(struct CourseLines *cl, u32 kind);                            /* 0x001F9BF8 */
/* kind 1 or 6: line 2 if it exists; else -1. */
int  lines_side_line_2(struct EnemyLines *d, int kind);                         /* 0x001F9C80 */
/* kind 2 or 6: line 1 if it exists; else -1. */
int  lines_side_line_1(struct EnemyLines *d, int kind);                         /* 0x001F9CB0 */
/* The side line of `kind' at course distance dist (line 5 searched from node
 * `hint'): lines_side_line_2 off line 5's arcs, lines_side_line_1 on them.
 * u086: the line to the - side (traffic zone 1 free). */
int  lines_side_line(struct CourseLines *cl, int kind, int hint, f32 dist);     /* 0x001F9CE0 */
/* As lines_side_line, the other way round (u086: the + side). */
int  lines_side_line_other(struct CourseLines *cl, int kind, int hint, f32 dist); /* 0x001F9D38 */

#endif /* GAME_T369_LINE_SELECT_H */
