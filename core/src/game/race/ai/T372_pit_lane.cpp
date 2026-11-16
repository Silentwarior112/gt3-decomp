/* T372 - line 4 of the enemy lines, the pit lane: is a course distance
 * inside it. */
/* Retail's .rodata of this file: <new>'s "bad_alloc" and UnitArena<64>'s
 * type name, nothing references them (u086's data pass found the seven
 * blocks, ref/data/u087/pending.md). */
// DATA_START 0x0033DE18
#include <new>
#define GAME_T153_EMITS_TYPEINFO           /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"
#include "game/race/ai/T372_pit_lane.h"
#include "game/race/ai/enemy_lines_internal.h"

/* dist (below half a lap: + a lap) strictly between node margin + 1 and node
 * count - 2 - margin of line 4; 0 without line 4 or with fewer than 6 nodes.
 * Callers: DynamicsConductor::chargePenaltyBySurface (0x001DA020),
 * DynamicsConductor::IsInsidePitRedArea (margin 1) / IsInsidePitYellowArea
 * (margin 0). */
// FUNC 0x001FA5B0 pit_lane_contains
int pit_lane_contains(CourseLines *cl, f32 dist, int margin)
{
    Line *pit = cl->data->line[LINE_PIT];

    if (pit == NULL)
        return 0;
    int count = pit->count;
    if (count < 6)
        return 0;
    LineNode *nodes = pit->node;
    f32 lap = cl->course->getLapLength();    /* 0x0018FA10 */
    if (dist < lap * 0.5f)
        dist += lap;
    if (nodes[margin + 1].dist < dist) {
        if (dist < nodes[(count - 2) - margin].dist)
            return 1;
    }
    return 0;
}
