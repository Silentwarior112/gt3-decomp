/* T368_enemy_lines.h - the course's enemy lines, the driving lines of the
 * computer cars (translation unit T368, unit u087), 0x001F8BA0..0x001F9BF8.
 *
 * The line file is CourseData +0x7C (RaceCourse::getEnemyLine, Lupo 2).
 * CourseLines (DynamicsConductor +0xBBA8, before the CarBoxList at +0xBBB4)
 * points at it.  lines_prepare converts the loaded file once (relocation,
 * fixed point to float) and fills every node's runway sector, course distance
 * and road height / pitch / roll / surface; the rest are ground queries along
 * a line, the point of a line at a course distance and a corner entry search.
 * T369 picks a line, T370 / T372 handle line 4 (the pit lane), T371 a line's
 * segment at a course distance.  The computer driver (T362..T367) follows
 * the lines.
 *
 * Lines: 6 the racing line, 1 / 2 its side lines, 3 the way into the pit
 * lane, 4 the pit lane (its distances run on past the lap), 5 (? only its
 * arcs are used: T369's side test), 7 (? falls back to 6); 0 unused.
 *
 * Other units' names for these records: u086 Corner (LineNode), LineData
 * (Line), LineSet (EnemyLines); CourseLines is u083's CourseGates and the
 * first 0xC bytes of u085's CourseGrid (0x2798 = CourseLines + CarBoxList).
 * The world of the lines is (x, y) horizontal, z up; the runway queries take
 * (x, height, -y). */
#ifndef GAME_T368_ENEMY_LINES_H
#define GAME_T368_ENEMY_LINES_H

#include "game/game.h"

class RaceCourse;                           /* T153_race_course.h (pointers only) */

/* One node of a line, 0x28 bytes.  In the file x, y, radius and angle are
 * ints (1/4096 units, angle 4096 = a turn); lines_prepare converts them.
 * The computer driver treats a node as the start of a corner (u086). */
struct LineNode {
    u8  kind;                               /* 0x00 the segment to the next node: 0 straight, */
                                            /* 1 arc (radius > 0), 2 arc (radius < 0); line 5: 1 any arc */
    s8  brakeAdj;                           /* 0x01 the braking distance to it x (1 + brakeAdj / 100) (u086) */
    s8  speedAdj;                           /* 0x02 km/h added to its corner speed (u086) */
    u8  unk_03;                             /* 0x03 */
    f32 x;                                  /* 0x04 (file: -x x 4096) */
    f32 y;                                  /* 0x08 (file: -y x 4096) */
    f32 height;                             /* 0x0C the road height */
    s16 sector;                             /* 0x10 the runway sector (cached; 0: not yet) */
    s8  surface;                            /* 0x12 the surface type (F_001E8A48; u086 reads it with lb) */
    u8  attr;                               /* 0x13 the contact's +0x32 byte (always 0: */
                                            /* RaceCourse::getContact clears it) */
    f32 dist;                               /* 0x14 the course distance (line 4: + a lap past the line) */
    f32 radius;                             /* 0x18 the arc radius (signed) */
    f32 pitch;                              /* 0x1C rad (u086: the braking distance x (1 - sin pitch)) */
    f32 roll;                               /* 0x20 rad (u086: the bank of the corner) */
    f32 angle;                              /* 0x24 the heading, rad (file: (a - pi) x 4096 / 2pi); */
                                            /* forwards is (-sin, cos) of it */
};
EGCS_SIZE(LineNode, 0x28);

/* A line: the node count and the nodes. */
struct Line {
    int      count;                         /* 0x00 */
    LineNode node[1];                       /* 0x04 [count] */
};

/* The enemy line file (CourseData +0x7C). */
struct EnemyLines {
    int   state;                            /* 0x00 > 0: as loaded (lines_prepare negates it) */
    int   unk_04;                           /* 0x04 */
    Line *line[8];                          /* 0x08 offsets from the file start until prepared, */
                                            /* then pointers (NULL for an empty line) */
};
EGCS_SIZE(EnemyLines, 0x28);

/* DynamicsConductor +0xBBA8, 0xC bytes (the CarBoxList follows at +0xBBB4).
 * Set up by lines_init (DynamicsConductor::initialize, slot 22). */
struct CourseLines {
    /* Inline: DynamicsConductor's constructor (0x001F2A10, T317) stores
     * lines.data = 0 itself (0x001F2A7C). */
    CourseLines() : data(0) {}
    /* Inline (u085 matching: DynamicsConductor::hasPitLane keeps &lines in
     * a register and reads +8). */
    int hasPitLane() { return hasPit; }

    RaceCourse *course;                     /* 0x0 */
    EnemyLines *data;                       /* 0x4 course->data->enemyLine (u086: lines) */
    int         hasPit;                     /* 0x8 lines 3 and 4 both exist */
};
EGCS_SIZE(CourseLines, 0xC);

/* The runway sector under (x, y), searched from `sector'; -1 off the road. */
int  lines_find_sector(RaceCourse *course, int sector, f32 x, f32 y);           /* 0x001F8BA0 */
/* The sector at the end of segment n-1 -> n, followed along it (straight:
 * 10 m steps, arc: 10 degree steps); sets nodes[n-1].kind of an arc. */
int  lines_trace_segment(RaceCourse *course, LineNode *nodes, int n, int sector, int kind); /* 0x001F8BF8 */
/* The road height at (x, y) probed from z; *miss: off the road (then 0 and
 * *sector unchanged).  (The point before the pointers: EABI gives the same
 * registers, and the callers' argument order is retail's only this way.) */
f32  lines_ground_height(RaceCourse *course, f32 x, f32 y, f32 z, int *sector, int *miss); /* 0x001F8EC8 */
/* The road under a node: height, surface, attr, flag (contact +0x33 bit 0),
 * pitch / roll from four points 0.5 m around it.  0: off the road. */
int  lines_ground_at(RaceCourse *course, f32 *height, f32 *pitch, f32 *roll, int *surface,
                     int *attr, int *flag, int sector, f32 x, f32 y, f32 angle); /* 0x001F8F40 */
/* The sector (and height) at (x, y) from the racing line's node sectors,
 * last node first (for line 4's first node).  0: none. */
int  lines_find_sector_on_racing_line(CourseLines *cl, int *sector, f32 *height, f32 x, f32 y); /* 0x001F91E8 */
/* Prepares the loaded lines (once: state > 0); returns 1. */
int  lines_prepare(CourseLines *cl);                                            /* 0x001F92F8 */
/* On the racing line: the course distance (- a lap) of the last node where
 * a tight arc (|radius| <= maxRadius) starts, at most dist + a lap; dist if
 * none (DynamicsConductor::virtual_25). */
f32  lines_corner_entry_before(CourseLines *cl, f32 dist, f32 maxRadius);       /* 0x001F9708 */
/* The point of line `kind' (lines_select) at course distance dist: x, y,
 * the road height, the heading towards the point 10 m on, the sector;
 * clamp: not before the last node - a lap.  0: no such line. */
int  lines_get_point(CourseLines *cl, u32 kind, int *sector, f32 *x, f32 *y, f32 *height,
                     f32 *angle, int clamp, f32 dist);                          /* 0x001F9880 */
/* course, data = course->data->enemyLine, lines_prepare, hasPit. */
void lines_init(CourseLines *cl, RaceCourse *course);                           /* 0x001F9B98 */

#endif /* GAME_T368_ENEMY_LINES_H */
