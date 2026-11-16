/* T367 (0x001F8638..0x001F8BA0): the computer driver's set-up, the driving
 * line tables and line changes, and the revving at the start. */
/* Retail T367's .rodata (0x0033DD50): <new>'s "bad_alloc" and the type name
 * "t9UnitArena1Ui64" (nothing references them: DATA_START).  Its .data: the
 * line table g_002DA030 before the frame info. */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/race/ai/T367_ai_line.h"
#include "game/race/ai/ai_internal.h"
// DATA_START 0x0033DD50

/* The links: car, conductor, lines = &conductor->lines.  car_restore (T351)
 * calls it to re-link a restored snapshot. */
// FUNC 0x001F8638 ai_init
void ai_init(AIDriver *ai, DynamicsConductor *dc, RaceCarState *car)
{
    ai->car = car;
    ai->conductor = dc;
    ai->lines = &dc->lines;
}

/* car_init's set-up of the driver, after the car's memset: only ai_init.
 * (Whether it was AIDriver's constructor cannot be told from the code: the
 * wrapper is all there is, and car_init calls it like a function.) */
// FUNC 0x001F8650 ai_construct
void ai_construct(AIDriver *ai, DynamicsConductor *dc, RaceCarState *car)
{
    ai_init(ai, dc, car);
}

/* The lines ai_nearest_line tries, in order.  In retail's .data (not
 * const), T367's only variable. */
s32 g_002DA030[3] = { 6, 2, 1 };            /* 0x002DA030 */

/* Of the lines 6, 2, 1 that exist, the one whose point at the car's course
 * distance (searched from node 0) is nearest to the car, under 100 m; else
 * the racing line. */
// FUNC 0x001F8670 ai_nearest_line
int ai_nearest_line(AIDriver *ai)
{
    int best = LINE_RACING;
    const s32 *tryLine = g_002DA030;
    int left = 2;
    f32 bestDist = 100.0f;
    DynCarState *st = &ai->car->st;
    f32 point[3];                           /* sp+0x0: x, y, height */

    do {
        int want = *tryLine;
        int got = lines_select(ai->lines, want);

        if (got == want) {
            f32 d;

            line_point_at(ai->lines, got, 0, st->lapDistance, &point[0], &point[1], &point[2]);
            point[0] = point[0] - st->pos[0];
            point[1] = point[1] - st->pos[1];
            d = boot_sqrtf(point[0] * point[0] + point[1] * point[1]);
            if (d < bestDist) {
                bestDist = d;
                best = got;
            }
        }
        left--;
        tryLine++;
    } while (left >= 0);
    return best;
}

/* Reset for a start: line `line' (-1: the nearest), the corner speeds the
 * top speed, the counters, the zones and the gear cleared.  Called by
 * DynCar_initialize with 6, 7 or -1. */
// FUNC 0x001F8780 ai_reset
void ai_reset(AIDriver *ai, int line)
{
    int ln;
    int i;

    ai->recover = AI_ON_LINE;
    ai->nextSpeed = ai->segSpeed = ai->car->params.maxSpeed;
    if (line == -1)
        ln = ai_nearest_line(ai);
    else
        ln = lines_select(ai->lines, line);
    ai_set_line(ai, lines_select(ai->lines, ln));
    ai->inPitLane = 0;
    ai->holdCount = 0;
    ai->speedStep = 0;
    ai->unk_99 = 0;
    F_001927C8();
    for (i = 0; i < 8; i++)
        ai->traffic[i] = 0;
    ai->caution = 0.0f;
    ai->lineTimer = 0;
    ai_clear_gear(ai);
}

/* No callers. */
// FUNC 0x001F8838 ai_on_line4
int ai_on_line4(AIDriver *ai)
{
    return ai->line == LINE_PIT_LANE;
}

// FUNC 0x001F8848 ai_corner_count
int ai_corner_count(AIDriver *ai, int line)
{
    return ai->lines->data->line[line]->count;
}

// FUNC 0x001F8868 ai_corners
LineNode *ai_corners(AIDriver *ai, int line)
{
    return ai->lines->data->line[line]->node;
}

/* speedStep += delta within -2..2 (0 at the bound), then the corner speeds
 * again and the (empty) race display function F_001927D0.  No callers
 * (? a debug adjustment: +-10 km/h a step on every corner speed). */
// FUNC 0x001F8888 ai_step_speed
int ai_step_speed(AIDriver *ai, int delta)
{
    s8 step;

    ai->speedStep += delta;
    step = ai->speedStep;
    if (step >= 3) {
        ai->speedStep = 2;
        return 0;
    }
    if (step < -2) {
        ai->speedStep = -2;
        return 0;
    }
    ai_find_corner(ai);
    F_001927D0(ai->speedStep);
    return 1;
}

/* unk_99 += delta within -2..2 (0 at the bound), then the (empty) race
 * display function F_001927D8 with the new value.  No callers, and nothing reads unk_99:
 * the twin of ai_step_speed for a value that is not used.  The argument:
 * the value is computed into $a0 and `ai' is moved out of $a0 to $a2 to make
 * room for it, as F_001927D0(int) gets speedStep. */
// FUNC 0x001F88F8 ai_step_unk99
int ai_step_unk99(AIDriver *ai, int delta)
{
    s8 value;

    ai->unk_99 += delta;
    value = ai->unk_99;
    if (value >= 3) {
        ai->unk_99 = 2;
        return 0;
    }
    if (value < -2) {
        ai->unk_99 = -2;
        return 0;
    }
    F_001927D8(value);
    return 1;
}

/* Switches to `line' at once (no blend). */
// FUNC 0x001F8958 ai_set_line
void ai_set_line(AIDriver *ai, int line)
{
    ai->line = line;
    ai->prevLine = line;
    ai->lineBlend = 0.0f;
    ai_find_corner(ai);
}

/* Moves to `line' (or its fallback, lines_select); 0 when there is none.
 * now == 0: blended from the old line (lineBlend 1 -> 0 over a second). */
// FUNC 0x001F8980 ai_change_line
int ai_change_line(AIDriver *ai, int line, int now)
{
    int ln = lines_select(ai->lines, line);

    if (ln == -1)
        return 0;
    if (now == 0) {
        ai->prevLine = ai->line;
        ai->line = ln;
        ai->lineBlend = 1.0f;
        ai_find_corner(ai);
    } else {
        ai_set_line(ai, ln);
    }
    return 1;
}

/* The throttle before the start (count: DynamicsConductor::getCountDown or
 * the run state's pitTimer, car_input_ai, T358).  From 13 on: revving
 * between 3/4 and 15/16 of the rev limit (faster for the lower ranks); under
 * 13: holding the rev limit. */
// FUNC 0x001F8A00 ai_rev_engine
void ai_rev_engine(AIDriver *ai, int count)
{
    RaceCarState *car = ai->car;
    DynCarState *st = &car->st;
    CarDynamics *dyn = &car->params;
    f32 rank = (f32)car->st.rank;

    st->clutch = 0.0f;
    if (!(count < 13)) {
        u8 rising = st->revving;
        f32 dt = car->dt;

        if (rising == 0) {
            if (st->rpm < dyn->gear.revLimit * 3 / 4) {
                st->revving = 1;
                rising = 1;
            }
        } else if (!(st->rpm < dyn->gear.revLimit * 15 / 16)) {
            st->revving = 0;
            rising = 0;
        }
        if (rising != 0) {
            ai->throttle = ai->throttle + dt * (5.0f - rank * 0.2f);
            if (1.0f < ai->throttle)
                ai->throttle = 1.0f;
        } else {
            ai->throttle = ai->throttle - dt * (10.0f - rank * 0.4f);
            if (ai->throttle < 0.0f)
                ai->throttle = 0.0f;
        }
    } else {
        /* 0.625 at the limit, minus twice the overshoot over the redline. */
        f32 throttle = (0.625f - rank * 0.002f)
                     - 2.0f * ((f32)(st->rpm - dyn->gear.revLimit) / (f32)dyn->engine.redline);

        ai->throttle = throttle;
        if (1.0f < throttle)
            ai->throttle = 1.0f;
        else if (throttle < 0.0f)
            ai->throttle = 0.0f;
    }
}
