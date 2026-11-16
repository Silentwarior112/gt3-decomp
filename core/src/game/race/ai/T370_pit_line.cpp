/* T370 - line 4 of the enemy lines, the pit lane: node values and the pose
 * of a pit slot (counted from the lane's end). */
/* Retail's .rodata of this file: <new>'s "bad_alloc" and UnitArena<64>'s
 * type name, nothing references them (u086's data pass found the seven
 * blocks, ref/data/u087/pending.md). */
// DATA_START 0x0033DDC8
#include <new>
#define GAME_T153_EMITS_TYPEINFO           /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"
#include "game/race/ai/T370_pit_line.h"
#include "game/race/ai/enemy_lines_internal.h"

/* The node of slot `slot': count - (slot + 4); 0 without line 4.
 * Callers: DynamicsConductor::pitSlotNode, pit_slot_pose. */
// FUNC 0x001F9D90 pit_slot_node
int pit_slot_node(CourseLines *cl, int slot)
{
    Line *pit = cl->data->line[LINE_PIT];

    if (pit == NULL)
        return 0;
    int back = slot + 4;            /* (in place, fold makes it (count - 4) - slot) */
    return pit->count - back;
}

/* Node i's course distance, i < 0 counted from the end (no test for line 4).
 * Caller: DynamicsConductor::pitGateDistance. */
// FUNC 0x001F9DB8 pit_node_dist_wrapped
f32 pit_node_dist_wrapped(CourseLines *cl, int i)
{
    Line *pit = cl->data->line[LINE_PIT];
    int count = pit->count;         /* read first: retail's movn */

    if (i < 0)
        i += count;
    return pit->node[i].dist;
}

/* Caller: DynamicsConductor::pitBoxDistance. */
// FUNC 0x001F9DF0 pit_node_dist
f32 pit_node_dist(CourseLines *cl, int i)
{
    return cl->data->line[LINE_PIT]->node[i].dist;
}

/* Caller: DynamicsConductor::pitNodeAngle. */
// FUNC 0x001F9E10 pit_node_angle
f32 pit_node_angle(CourseLines *cl, int i)
{
    return cl->data->line[LINE_PIT]->node[i].angle;
}

/* The pose of slot `slot': the node's position moved 0.5 m along its
 * heading (forwards is (-sin, cos) of it, as lines_get_point computes it),
 * its height, pitch / roll / heading and sector.  Caller: DynCar_initialize
 * (start types 6 / 7). */
// FUNC 0x001F9E30 pit_slot_pose
void pit_slot_pose(CourseLines *cl, int slot, f32 *pos, f32 *rot, int *sector)
{
    int node = pit_slot_node(cl, slot);
    LineNode *at = &cl->data->line[LINE_PIT]->node[node];

    pos[0] = at->x;
    pos[1] = at->y;
    pos[2] = at->height;
    rot[0] = at->pitch;
    rot[1] = at->roll;
    rot[2] = at->angle;
    *sector = at->sector;
    pos[0] = pos[0] - sinf(rot[2]) * 0.5f;     /* (rot[2]: the stored $f12 passed on) */
    pos[1] = pos[1] + cosf(rot[2]) * 0.5f;
}
