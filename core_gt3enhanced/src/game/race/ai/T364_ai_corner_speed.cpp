/* T364 (0x001F6678..0x001F6B48): the corner speeds and the corner
 * bookkeeping along the driving line.  A corner is a node of the line
 * (LineNode, T368): the kind of the segment it starts, its radius, its roll
 * (the bank) and its surface. */
/* Retail T364's .rodata (0x0033DCB0): <new>'s "bad_alloc" and the type name
 * "t9UnitArena1Ui64" (nothing references them: DATA_START). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/race/ai/T364_ai_corner_speed.h"
#include "game/race/ai/ai_internal.h"
// DATA_START 0x0033DCB0

/* The speed for node idx of the current line: the grip of the front and
 * rear tyres on its surface (weighted by gripBalance; with tyre wear, by
 * each wheel's wearGrip), times the radius, over the bank, then v =
 * sqrt(grip r / (1 + downforce v^2)) by three fixed-point steps, the
 * driver's scale and offsets; at most the top speed, at least 20 km/h. */

/* The fixed-point steps: an inline helper (its own p': GCSE does not hoist
 * the loads of cornerScale / cornerOffset after it into the loop's
 * preheader, as retail shows), sqrt.s as the volatile asm (retail keeps the
 * multiplications after it). */
static inline f32 ai_grip_speed(AIDriver *ai, f32 grip)
{
    AIParams *p = &ai->params;
    f32 v, div = 1.0f;
    int i;

    for (i = 2; i >= 0; i--) {
        v = boot_sqrtf_v(grip * div);
        div = p->downforce * v * v + 1.0f;
    }
    return v;
}

// FUNC 0x001F6678 ai_corner_speed
f32 ai_corner_speed(AIDriver *ai, int idx)
{
    RaceCarState *car = ai->car;
    CarDynamics *dyn = &car->params;
    DynCarState *st = &car->st;
    LineNode *nodes = ai_corners(ai, ai->line);
    f32 balance = ai->params.gripBalance;
    s8 surface = nodes[idx].surface;
    f32 radius = nodes[idx].radius;
    TyreData *front = CarDynamics_getTyre(dyn, 0, surface);
    TyreData *rear = CarDynamics_getTyre(dyn, 2, surface);
    f32 axleGrip[2];                        /* sp+0x0: front, rear */
    f32 grip, v;
    int wear, i;
    AIParams *p;

    axleGrip[0] = balance * front->surfaceGrip[surface] * front->grip;
    axleGrip[1] = (1.0f - balance) * rear->surfaceGrip[surface] * rear->grip;
    wear = 1;
    if (ai->conductor->wear.x3 == 0.0f)     /* the race has no tyre wear */
        wear = 0;
    if (wear != 0 && !(st->flags & DYN_FLAG_COUNTED_GROUND)) {
        grip = 0.0f;
        for (i = 0; i < 4; i++)
            grip += st->wheel[i].wearGrip * axleGrip[i >> 1] * 0.5f;
    } else {
        grip = axleGrip[0] + axleGrip[1];
    }
    /* A roll of the other sign than the radius raises the speed (grip / (1 -
     * |sin roll|): banked into the turn); a roll of the same sign is
     * ignored. */
    if (0.0f <= radius) {
        grip *= radius;
        if (nodes[idx].roll < 0.0f)
            grip /= sinf(nodes[idx].roll) + 1.0f;
    } else {
        grip *= -radius;
        if (0.0f < nodes[idx].roll)
            grip /= 1.0f - sinf(nodes[idx].roll);
    }
    v = ai_grip_speed(ai, grip);
    p = &ai->params;
    v = p->cornerScale * v
      + (p->cornerOffset + (f32)(ai->speedStep * 10) * KMH_TO_MS
         + (f32)nodes[idx].speedAdj * KMH_TO_MS);
    if (dyn->maxSpeed < v)
        return dyn->maxSpeed;
    if (v < 5.555556f)                      /* 20 km/h */
        v = 5.555556f;
    return v;
}

/* Moves the next corner's kind and speed into the current ones and takes the
 * corner at ai->corner (the last one when it is past the end) as the next. */
// FUNC 0x001F6958 ai_next_corner
void ai_next_corner(AIDriver *ai)
{
    CarDynamics *dyn = &ai->car->params;
    int count = ai_corner_count(ai, ai->line);
    LineNode *nodes = ai_corners(ai, ai->line);

    ai->state = ai->nextKind;
    ai->segSpeed = ai->nextSpeed;
    /* ai->corner read each time (a local made CSE use it for count in the
     * else branch). */
    if (ai->corner != count)
        ai->nextKind = nodes[ai->corner].kind;
    else
        ai->nextKind = nodes[count - 1].kind;
    switch (ai->nextKind) {
    case AI_STRAIGHT:
        ai->nextSpeed = dyn->maxSpeed;
        break;
    case AI_CORNER_POS:
    case AI_CORNER_NEG:
        ai->nextSpeed = ai_corner_speed(ai, (ai->corner != count) ? ai->corner : count - 1);
        break;
    }
}

/* Finds the first corner ahead of the car's course distance, takes the one
 * before it (wrapping) as the next, then steps once: the car is in the
 * previous corner, heading for the found one. */
// FUNC 0x001F6A20 ai_find_corner
void ai_find_corner(AIDriver *ai)
{
    RaceCarState *car = ai->car;
    CarDynamics *dyn = &car->params;
    DynCarState *st = &car->st;
    int count = ai_corner_count(ai, ai->line);
    LineNode *nodes = ai_corners(ai, ai->line);
    f32 dist = st->lapDistance;
    int i, prev;

    ai->corner = count;
    for (i = 0; i < count; i++) {
        if (dist < nodes[i].dist) {
            ai->corner = i;
            break;
        }
    }
    prev = ai->corner + count - 1;
    while (prev >= count)
        prev -= count;
    ai->nextKind = nodes[prev].kind;
    if (ai->nextKind == AI_STRAIGHT)
        ai->nextSpeed = dyn->maxSpeed;
    else
        ai->nextSpeed = ai_corner_speed(ai, prev);
    ai_next_corner(ai);
}
