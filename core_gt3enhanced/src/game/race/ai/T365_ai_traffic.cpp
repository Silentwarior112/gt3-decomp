/* T365 (0x001F6B48..0x001F7078): the cars around the computer driver.
 * dc_car_pairs (T349) clears the zones, then reports each nearby car
 * (ai_note_car, ai_note_closing); ai_drive acts on them (ai_avoid_traffic). */
/* Retail T365's .rodata (0x0033DCD8): <new>'s "bad_alloc" and the type name
 * "t9UnitArena1Ui64" (nothing references them: DATA_START). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/race/ai/T365_ai_traffic.h"
#include "game/race/ai/ai_internal.h"
#include "game/enhanced.h"                  /* ENH_AI_CAUTION, ENH_AI_TRAFFIC */
// DATA_START 0x0033DCD8

/* The zones, AIDriver.traffic[] ("-" / "+": the sign of `side'), */
enum {
    ZONE_AHEAD       = 0,
    ZONE_AHEAD_NEG   = 1,                   /* ahead, to the - side */
    ZONE_AHEAD_POS   = 2,
    ZONE_BESIDE_NEG  = 3,
    ZONE_BESIDE_POS  = 4,
    ZONE_BEHIND_NEG  = 5,
    ZONE_BEHIND_POS  = 6,
    ZONE_BEHIND      = 7
};
/* and their values: */
enum {
    ZONE_LEANING     = 1,                   /* a car in the middle zone leans to this side */
    ZONE_TAKEN       = 2                    /* a car in the zone */
};

/* The extra distance before the braking point that counts as "near it"
 * (ai_brake_factor). */
// FUNC 0x001F6B48 ai_brake_margin
f32 ai_brake_margin(AIDriver *ai)
{
    return 50.0f;
}

// FUNC 0x001F6B58 ai_clear_traffic
void ai_clear_traffic(AIDriver *ai)
{
    s8 *zone = ai->traffic;
    int i;

    ai->caution = 0.0f;
    for (i = 0; i < 8; i++)                 /* reversed by loop.c */
        zone[i] = 0;
}

/* One car at `dist' (ignored beyond 20 m), `along' ahead (+) / behind (-)
 * and `side' across: ahead beyond `front', behind beyond `rear', else beside;
 * a side zone beyond +-`width', else the middle zone and the side it leans
 * to. */
// FUNC 0x001F6B90 ai_note_car
void ai_note_car(AIDriver *ai, f32 dist, f32 along, f32 side,
                 f32 front, f32 rear, f32 width)
{
    if (20.0f < dist)
        return;
    if (front < along) {
        if (side < -width) {
            ai->traffic[ZONE_AHEAD_NEG] = ZONE_TAKEN;
            return;
        }
        if (width < side) {
            ai->traffic[ZONE_AHEAD_POS] = ZONE_TAKEN;
            return;
        }
        ai->traffic[ZONE_AHEAD] = ZONE_TAKEN;
        if (0.0f < side)
            ai->traffic[ZONE_AHEAD_POS] = ZONE_LEANING;
        else
            ai->traffic[ZONE_AHEAD_NEG] = ZONE_LEANING;
        return;
    }
    if (along < -rear) {
        if (side < -width) {
            ai->traffic[ZONE_BEHIND_NEG] = ZONE_TAKEN;
            return;
        }
        if (width < side) {
            ai->traffic[ZONE_BEHIND_POS] = ZONE_TAKEN;
            return;
        }
        ai->traffic[ZONE_BEHIND] = ZONE_TAKEN;
        if (0.0f < side)
            ai->traffic[ZONE_BEHIND_POS] = ZONE_LEANING;
        else
            ai->traffic[ZONE_BEHIND_NEG] = ZONE_LEANING;
        return;
    }
    if (side < 0.0f) {
        ai->traffic[ZONE_BESIDE_NEG] = ZONE_TAKEN;
        if (0.0f < along)
            ai->traffic[ZONE_AHEAD_NEG] = ZONE_LEANING;
        else
            ai->traffic[ZONE_BEHIND_NEG] = ZONE_LEANING;
    } else {
        ai->traffic[ZONE_BESIDE_POS] = ZONE_TAKEN;
        if (0.0f < along)
            ai->traffic[ZONE_AHEAD_POS] = ZONE_LEANING;
        else
            ai->traffic[ZONE_BEHIND_POS] = ZONE_LEANING;
    }
}

/* The caution for a car `gap' m ahead closing at `closing' m/s: 1 above
 * 10 m/s, 0.5 under 3 s to contact (closing above 1 km/h), 0.25 under 2 m;
 * the largest of the frame is kept.
 * The enhanced engine (race_logic.md A2, ENH_AI_CAUTION; tuned together
 * with A3 and physics.md 27, which also reports cars that are not
 * closing): 1 above 5 km/h, 0.5 under 5 s to contact (closing above
 * 0.1 km/h), 0.25 under 4 m (Lupo 2 L2 0x002007A8). */
// FUNC 0x001F6CC0 ai_note_closing
void ai_note_closing(AIDriver *ai, f32 gap, f32 closing)
{
    f32 caution = 0.0f;

#if ENH_AI_CAUTION
    if (1.388889f < closing)                /* Lupo 2: 5 km/h (gp 0x0035BB00) */
        caution = 1.0f;
    else if (0.02777778f < closing && gap / closing < 5.0f) /* 0.1 km/h, 5 s */
        caution = 0.5f;
    if (caution == 0.0f && gap < 4.0f)      /* 4 m */
        caution = 0.25f;
#else
    if (10.0f < closing)
        caution = 1.0f;
    else if (0.2777778f < closing && gap / closing < 3.0f)
        caution = 0.5f;
    if (caution == 0.0f && gap < 2.0f)
        caution = 0.25f;
#endif
    if (ai->caution < caution)
        ai->caution = caution;
}

/* Called by ai_drive while driving forward.  On the pit approach (line 3) it
 * only slows behind cars beside and ahead on the same side; line 4 does
 * nothing.  Otherwise, with a car closing in (caution) it changes to a free
 * side line (preferring the side of the next corner's kind, not while
 * braking toward that side) and waits 30 frames; else it lifts by the
 * caution, goes back to the racing line when nobody is around, or moves away
 * from a car beside it.  No line change near the braking point or while
 * holding a corner speed (brakeState 1, 2, 5).
 *
 * The enhanced engine (race_logic.md A3, ENH_AI_TRAFFIC; Lupo 2 L2
 * 0x00200848): after a caution change the next one waits 180 frames; no
 * line change while the last one is still blending in (lineBlend > 0);
 * while the line timer runs (and after a caution change) only the caution
 * is dropped, the return to the racing line and the move away from a car
 * beside still run; on the pit approach a car under 30 km/h holds the line
 * timer instead of halving its throttle. */
// FUNC 0x001F6D70 ai_avoid_traffic
void ai_avoid_traffic(AIDriver *ai)
{
    CourseLines *lines = ai->lines;
    DynCarState *st = &ai->car->st;
    int mayChange, side;
    f32 caution;
    s8 neg, pos;
    int both, any;
    u8 line;

    if (ai->lineTimer > 0)
        ai->lineTimer--;
    mayChange = 1;
    if (ai->line == LINE_PIT_LANE)
        return;
    if (ai->line == LINE_PIT_ENTRY) {
        if ((ai->traffic[ZONE_BESIDE_NEG] != 0 && ai->traffic[ZONE_AHEAD_NEG] != 0)
            || (ai->traffic[ZONE_BESIDE_POS] != 0 && ai->traffic[ZONE_AHEAD_POS] != 0)) {
#if ENH_AI_TRAFFIC
            /* Lupo 2: crawling (under 30 km/h) the timer is held instead (L2 0x002008E4) */
            if (st->speedX < 8.333334f)
                ai->lineTimer++;
            else
#endif
            ai->throttle *= 0.5f;
        }
        mayChange = 0;
    }
    if (ai->brakeState > BRAKE_STATE_NONE
        && (ai->brakeState < BRAKE_STATE_FULL_NEG || ai->brakeState == BRAKE_STATE_PART))
        mayChange = 0;
    caution = ai->caution;
#if ENH_AI_TRAFFIC
    /* Lupo 2: the running timer only drops the caution (L2 0x00200944), and
     * no change while the last one blends in (L2 0x00200954) */
    if (ai->lineTimer > 0)
        caution = 0.0f;
    if (0.0f < ai->lineBlend)
        mayChange = 0;
#else
    if (ai->lineTimer > 0)
        return;
#endif

    if (mayChange != 0 && 0.0f < caution) {
        int negLine, posLine, pick;
        u8 cur;

        posLine = -1;
        cur = ai->line;
        negLine = -1;
        if (ai->traffic[ZONE_AHEAD_NEG] == 0)
            negLine = lines_side_line(lines, cur, ai->corner, st->lapDistance);
        if (ai->traffic[ZONE_AHEAD_POS] == 0)
            posLine = lines_side_line_other(lines, cur, ai->corner, st->lapDistance);
        /* Once through (each way out breaks): retail has the loop's layout,
         * the last choice in front of the rotated test, the constants 1, 2
         * and 3 hoisted. */
        pick = -1;
        while (pick < 0) {
            if (ai->nextKind == AI_CORNER_POS) {
                if (negLine >= 0) {
                    pick = negLine;
                    break;
                }
            } else if (ai->nextKind == AI_CORNER_NEG) {
                if (posLine >= 0) {
                    pick = posLine;
                    break;
                }
            }
            if (negLine >= 0 && ai->brakeState != BRAKE_STATE_FULL_NEG) {
                pick = negLine;
                break;
            }
            if (posLine >= 0 && ai->brakeState != BRAKE_STATE_FULL_POS)
                pick = posLine;
            break;
        }
        if (pick >= 0) {
            ai_change_line(ai, pick, 0);
#if ENH_AI_TRAFFIC
            /* Lupo 2: 180 frames, no lift, and on (L2 0x00200A58) */
            ai->lineTimer = 180;
            caution = 0.0f;
#else
            ai->lineTimer = 30;
            return;
#endif
        }
    }

    ai->throttle *= 1.0f - caution;
    if (mayChange == 0)
        return;
    if (ai->traffic[ZONE_AHEAD] + ai->traffic[ZONE_AHEAD_NEG] + ai->traffic[ZONE_AHEAD_POS]
        + ai->traffic[ZONE_BESIDE_NEG] + ai->traffic[ZONE_BESIDE_POS] == 0) {
        ai_change_line(ai, LINE_RACING, 0);
        return;
    }
    /* A car beside on one side only: away from it. */
    neg = ai->traffic[ZONE_BESIDE_NEG];
    pos = ai->traffic[ZONE_BESIDE_POS];
    both = neg != 0 && pos != 0;
    any = neg != 0 || pos != 0;
    if (!any)
        return;
    if (both)
        return;
    line = ai->line;
    if (neg != 0 && ai->brakeState != BRAKE_STATE_FULL_POS) {
        side = lines_side_line_other(lines, line, ai->corner, st->lapDistance);
        if (side != -1) {
            ai_change_line(ai, side, 0);
            return;
        }
    }
    if (pos != 0 && ai->brakeState != BRAKE_STATE_FULL_NEG) {
        side = lines_side_line(lines, line, ai->corner, st->lapDistance);
        if (side != -1)
            ai_change_line(ai, side, 0);
    }
}
