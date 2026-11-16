/* T360 - CarPairStates, DynamicsConductor +0xBA34 (Lupo 2: the "race
 * watcher", DynamicsConductor::GetRaceWatcher), declared in
 * T305_dynamics_conductor.h (T360 has no header of its own).  For every
 * ordered pair of cars (i, j): a frame counter (0..300, up while car i is
 * in car j's slipstream, dc_get_slip(i, j) > 0, down otherwise) and a state
 * 0..3 that follows the longitudinal zone of j in i's frame (the proximity
 * code dc_get_near(i, j) & 0xF, T349).  A state of the mirrored pair (j, i)
 * blocks or is set by some transitions.  The two queries (no callers) give
 * the strongest relation of a car or the set of cars it relates to; the
 * lap difference (DynamicsConductor::gapNearest) takes part in both. */
/* Retail T360's .rodata (0x0033DC20): <new>'s "bad_alloc" and the type name
 * "t9UnitArena1Ui64" (nothing references them: DATA_START). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/race/dynamics/control_record_internal.h"
#include "game/race/dynamics/T305_dynamics_conductor.h"
// DATA_START 0x0033DC20

// FUNC 0x001F5918 CarPairStates::setOwner
void CarPairStates::setOwner(DynamicsConductor *d)
{
    dc = d;
}

// FUNC 0x001F5920 CarPairStates::reset
void CarPairStates::reset()
{
    int n = dc->carCount;
    int i, j;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            timer[i][j] = 0;
            state[i][j] = PAIR_NONE;
        }
    }
}

// FUNC 0x001F59B0 CarPairStates::update
/* Once a frame (DynamicsConductor::update).  k: the zone of j in i's frame.
 *   0 -> 3 on zone 4, -> 2 on zone 5, -> 1 after 61 frames in j's
 *        slipstream; only while (j, i) is 0
 *   1 -> 3 / 2 on zone 4 / 5 while (j, i) is 0, else -> 0 when the counter
 *        is back to 0
 *   2 -> 3 on zones 3, 4; -> 1 on zones 0, 6; on zones 1, 2 (j is behind i
 *        now) the state passes to (j, i) = 2 and (i, j) -> 0
 *   3 -> on zones 1, 2 as for 2; -> 2 on zones 5, 6
 * timer[i][j] / state[i][j] are written out at each use (retail recomputes
 * the addresses after the calls; pointers taken once made it 77.8%). */
void CarPairStates::update()
{
    int n = dc->carCount;
    int i, j;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            int zone;

            if (i == j)
                continue;
            if (0.0f < dc_get_slip(dc, i, j)) {
                if (timer[i][j] < 300)
                    timer[i][j]++;
            } else {
                if (timer[i][j] > 0)
                    timer[i][j]--;
            }
            zone = dc_get_near(dc, i, j) & 0xF;
            switch (state[i][j]) {
            case PAIR_NONE:
                if (state[j][i] != PAIR_NONE)
                    break;
                if (zone == ZONE_CLOSE_AHEAD)
                    state[i][j] = PAIR_CLOSE;
                else if (zone == ZONE_AHEAD)
                    state[i][j] = PAIR_CHASING;
                else if (timer[i][j] >= 61)
                    state[i][j] = PAIR_FOLLOWING;
                break;
            case PAIR_FOLLOWING:
                if (state[j][i] == PAIR_NONE) {
                    if (zone == ZONE_CLOSE_AHEAD) {
                        state[i][j] = PAIR_CLOSE;
                        break;
                    }
                    if (zone == ZONE_AHEAD) {
                        state[i][j] = PAIR_CHASING;
                        break;
                    }
                }
                if (timer[i][j] == 0)
                    state[i][j] = PAIR_NONE;
                break;
            case PAIR_CHASING:
                if ((u32)(zone - ZONE_CLOSE_BEHIND) < 2) {          /* 3, 4 */
                    state[i][j] = PAIR_CLOSE;
                } else if (zone == ZONE_NONE || zone == ZONE_FAR_AHEAD) {
                    state[i][j] = PAIR_FOLLOWING;
                } else if ((u32)(zone - ZONE_FAR_BEHIND) < 2) {     /* 1, 2 */
                    state[j][i] = PAIR_CHASING;
                    state[i][j] = PAIR_NONE;
                }
                break;
            case PAIR_CLOSE:
                if ((u32)(zone - ZONE_FAR_BEHIND) < 2) {            /* 1, 2 */
                    state[j][i] = PAIR_CHASING;
                    state[i][j] = PAIR_NONE;
                } else if ((u32)(zone - ZONE_AHEAD) < 2) {          /* 5, 6 */
                    state[i][j] = PAIR_CHASING;
                }
                break;
            }
        }
    }
}

// FUNC 0x001F5C58 CarPairStates::strongest
/* No callers.  Over the cars j where (car, j) or (j, car) is 2 or 3: on the
 * same lap 5 (a 3 on either side), 4 ((car, j) = 2), 3 ((j, car) = 2) or 0;
 * on another lap 1 (j is laps ahead) or 2 (j is laps behind).  Returns the
 * largest value and puts its car in *other (-1: none).  value starts at 3
 * (retail keeps the 3 of the compares in the result register); the 2 of
 * the compares and of `laps < 0' stays in $t0 across gapNearest. */
int CarPairStates::strongest(int car, int *other)
{
    int n = dc->carCount;
    int best = 0;
    int bestCar = -1;
    int j;

    for (j = 0; j < n; j++) {
        u8 mine, theirs;
        int laps, value;
        f32 dist;

        if (j == car)
            continue;
        mine = state[car][j];
        theirs = state[j][car];
        if (mine < PAIR_CHASING && theirs < PAIR_CHASING)
            continue;
        laps = dc->gapNearest(car, j, &dist);
        if (laps == 0) {
            value = 3;
            if (mine == PAIR_CLOSE || theirs == PAIR_CLOSE)
                value = 5;
            else if (mine == PAIR_CHASING)
                value = 4;
            else if (theirs != PAIR_CHASING)
                value = 0;
        } else {
            value = (laps < 0) ? 2 : 1;
        }
        if (best < value) {
            bestCar = j;
            best = value & 0xFF;
        }
    }
    *other = bestCar;
    return best;
}

// FUNC 0x001F5DB8 CarPairStates::activeMask
/* No callers.  Bit j for every car j in a state with `car' (either way);
 * flags bit 0: state 1 counts too, bit 1: also the cars on another lap.
 * The flag tests are in the loop: loop.c hoists flags & 2, then flags & 1,
 * to the preheader and spills them (sp+4, sp+8), as retail. */
int CarPairStates::activeMask(int car, u16 flags)
{
    int n = dc->carCount;
    int mask = 0;
    int j;

    for (j = 0; j < n; j++) {
        u8 mine, theirs;

        if (j == car)
            continue;
        mine = state[car][j];
        theirs = state[j][car];
        if (mine == PAIR_NONE && theirs == PAIR_NONE)
            continue;
        if ((flags & 2) == 0) {
            f32 dist;
            if (dc->gapNearest(car, j, &dist) != 0)
                continue;
        }
        if ((flags & 1) == 0) {
            if (mine == PAIR_FOLLOWING)
                mine = PAIR_NONE;
            if (theirs == PAIR_FOLLOWING)
                theirs = PAIR_NONE;
        }
        if (mine == PAIR_NONE && theirs == PAIR_NONE)
            continue;
        mask |= 1 << j;
    }
    return mask;
}
