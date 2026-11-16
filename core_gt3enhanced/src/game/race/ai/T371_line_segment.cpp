/* T371 - an enemy line at a course distance: the segment containing it,
 * the interpolated point on it, line 5's arc test. */
/* Retail's .rodata of this file: <new>'s "bad_alloc" and UnitArena<64>'s
 * type name, nothing references them (u086's data pass found the seven
 * blocks, ref/data/u087/pending.md). */
// DATA_START 0x0033DDF0
#include <new>
#define GAME_T153_EMITS_TYPEINFO           /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"
#include "game/race/ai/T371_line_segment.h"
#include "game/race/ai/enemy_lines_internal.h"

/* The segment *prev -> *next of `line' containing dist.  1: before the first
 * node, 2: at / after the last one (both: prev = last, next = 0); 3: found,
 * searching outward from node `hint' (hint, hint + 1, hint - 1, hint + 2,
 * ...: step k is added to it when odd, subtracted when even); 0: not found.
 * Callers: line_point_at, line5_arc_at. */
// FUNC 0x001F9F08 line_find_segment
int line_find_segment(EnemyLines *file, int line, int hint, f32 dist, int *next, int *prev)
{
    Line *l = file->line[line];
    LineNode *nodes = l->node;
    int count = l->count;
    int step;

    if (dist < nodes[0].dist) {
        *prev = count - 1;
        *next = 0;
        return 1;
    }
    if (nodes[count - 1].dist <= dist) {
        *prev = count - 1;
        *next = 0;
        return 2;
    }
    for (step = 0; step <= count; step++) {
        int node;
        if ((step & 1) == 0)
            hint -= step;
        else
            hint += step;
        node = hint;
        if (node < 0)
            node += count;
        else if (!(node < count))
            node -= count;
        if (node != 0) {
            int before = node - 1;
            if (!(-1 < before))
                before += count;
            if (nodes[before].dist <= dist && dist < nodes[node].dist) {
                *prev = before;
                *next = node;
                return 3;
            }
        }
    }
    return 0;
}

/* The point of `line' at course distance dist (wrapped once into the lap):
 * along the straight, or along the arc of the segment's first node; the
 * height interpolated.  Returns the segment's first node, -1 if none.
 * Callers: DynamicsConductor::linePointWorld, ai_look_ahead, ai_nearest_line,
 * lines_get_point. */
// FUNC 0x001FA018 line_point_at
int line_point_at(CourseLines *cl, int line, int hint, f32 dist, f32 *x, f32 *y, f32 *height)
{
    f32 t = 0.0f;
    f32 lap = cl->course->getLapLength();    /* 0x0018FA10 */
    EnemyLines *file = cl->data;
    int next = 0;                   /* sp+0x0 */
    int prev = 0;                   /* sp+0x4 */
    LineNode *nodes = file->line[line]->node;
    f32 along, length;

    if (dist < 0.0f)
        dist += lap;
    else if (lap <= dist)
        dist -= lap;

    switch (line_find_segment(file, line, hint, dist, &next, &prev)) {
    case 1:                         /* before the first node: the segment over the start */
        along = (dist + lap) - nodes[prev].dist;
        length = (nodes[next].dist + lap) - nodes[prev].dist;
        break;
    case 2:                         /* after the last node */
        along = dist - nodes[prev].dist;
        length = (nodes[next].dist + lap) - nodes[prev].dist;
        break;
    case 3:
        along = dist - nodes[prev].dist;
        length = nodes[next].dist - nodes[prev].dist;
        break;
    default:
        return -1;
    }
    t = along / length;

    LineNode *from = &nodes[prev];
    f32 fromX = from->x;
    f32 fromY = from->y;
    LineNode *to = &nodes[next];
    f32 toX = to->x;
    f32 toY = to->y;

    if (from->kind == 0) {
        *x = t * (toX - fromX) + fromX;
        *y = t * (toY - fromY) + fromY;
    } else {
        f32 heading = from->angle;
        f32 negSin = -sinf(heading);
        f32 cosH = cosf(heading);
        f32 radius = nodes[prev].radius;
        f32 centreX = fromX - cosH * radius;    /* the centre of the arc */
        f32 centreY = fromY + negSin * radius;
        f32 startAngle = atan2f(centreX - fromX, fromY - centreY);
        f32 endAngle = atan2f(centreX - toX, toY - centreY);

        if (0.0f < radius) {
            if (endAngle < startAngle)
                endAngle += U087_TWO_PI;
        } else {
            radius = -radius;
            if (startAngle < endAngle)
                startAngle += U087_TWO_PI;
        }
        f32 phi = startAngle + t * (endAngle - startAngle);
        *x = centreX - sinf(phi) * radius;
        *y = centreY + cosf(phi) * radius;
    }
    f32 fromHeight = nodes[prev].height;
    *height = t * (nodes[next].height - fromHeight) + fromHeight;
    return prev;
}

/* As line_point_at, straight interpolation only, for a line whose distances
 * run past the lap (line 4): dist below half a lap counts as dist + a lap.
 * Returns the segment's first node, -1 outside the line (with no segment
 * containing dist: the last segment's start, at t = 0).  `unused' is not
 * read.  Callers: DynamicsConductor::linePointWorld, ai_look_ahead. */
// FUNC 0x001FA368 line_point_at_linear
int line_point_at_linear(CourseLines *cl, int line, int unused, f32 *x, f32 *y, f32 *height,
                         f32 dist)
{
    f32 lap = cl->course->getLapLength();    /* 0x0018FA10 */
    f32 t = 0.0f;                   /* (after the call: its 0.0 is the dist < 0 test's) */
    int to = 0;
    Line *l = cl->data->line[line];
    int from = 0;
    int count = l->count;           /* (before nodes: count reuses l's register) */
    LineNode *nodes = l->node;
    int i;

    if (dist < 0.0f)
        dist += lap;
    else if (lap <= dist)
        dist -= lap;
    if (dist < lap * 0.5f)
        dist += lap;
    if (dist < nodes[0].dist)
        return -1;
    if (nodes[count - 1].dist <= dist)
        return -1;
    for (i = 0; i < count - 1; i++) {
        from = i;
        to = i + 1;
        if (nodes[from].dist <= dist && dist < nodes[to].dist) {
            t = (dist - nodes[from].dist) / (nodes[to].dist - nodes[from].dist);
            break;
        }
    }
    LineNode *a = &nodes[from];
    LineNode *b = &nodes[to];
    f32 toX = b->x;
    f32 toY = b->y;
    f32 fromX = a->x;
    f32 fromY = a->y;
    *x = t * (toX - fromX) + fromX;
    *y = t * (toY - fromY) + fromY;
    f32 fromHeight = a->height;
    *height = t * (b->height - fromHeight) + fromHeight;
    return from;
}

/* Is dist on an arc of line 5 (searched from node `hint', from node 0 when
 * hint is past the line's count)?  0 without line 5.  Callers:
 * lines_side_line, lines_side_line_other. */
// FUNC 0x001FA530 line5_arc_at
int line5_arc_at(CourseLines *cl, int hint, f32 dist)
{
    EnemyLines *file = cl->data;
    Line *arcs = file->line[LINE_ARCS];
    int next;                       /* sp+0x0 */
    int prev;                       /* sp+0x4 */

    if (arcs == NULL)
        return 0;
    if (line_find_segment(file, LINE_ARCS, (arcs->count < hint) ? 0 : hint, dist, &next, &prev) == 0)
        return 0;
    return cl->data->line[LINE_ARCS]->node[prev].kind != 0;
}
