/* T393 - TargetChooser (? name), 0x0020E020..0x0020E930: the automatic
 * choice of the car a replay view follows.
 *
 * target (the view's camera target, CameraViewKind::variant): VIEW_TARGET_
 * MANUAL = the view's own car (score 1200), VIEW_TARGET_TOP = the leader,
 * otherwise automatic: every car has a score 0..1200 from its place
 * (g_002DF108) plus random gains, place changes and a bonus for the state of
 * its CameraTargetSelector slot; F_0020E1F0 picks the best car now and then,
 * F_0020E790 takes it when the camera switches.
 *
 * The place of a car is clamped to g_002DF108's index (0 .. TARGET_PLACE_LAST)
 * in place, on the variable itself (retail keeps it in one register through
 * both movz; an inline helper returning the value gives a second one).  The
 * switches have a case VIEW_TARGET_AUTO next to their default: retail tests
 * == 1, then > 1, then == 0, the decision tree of three case values.
 *
 * Retail T393's .data opens with g_002DF108 (defined here); its .rodata
 * (0x0033F460, nothing references it: DATA_START) has <new>'s "bad_alloc"
 * and the type names 14RaceCameraBase, t9UnitArena1Ui64: the file saw
 * RaceCameraBase (T383), defined after UnitArena<64> (T153).
 */
// DATA_START 0x0033F460
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, as retail */
#include "game/race/core/T153_race_course.h"          /* first: the name after all the others */
#include "game/camera/T383_race_camera_base.h"     /* "14RaceCameraBase" (only pure virtuals: no key function) */
#include "game/camera/camera_base_internal.h"
#include "game/camera/T393_target_chooser.h"
#include "engine/pgl/G001_flare.h"                /* gfx_perf_counter0 */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase (count, cars), RaceEntryCar (state) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries) */
#include "game/car/physics/T289_car_mode.h"             /* car_display_level, CAR_SHOW_FULL */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState (st.rank) */
#include "game/camera/T377_camera_target_info.h"   /* CameraTargetInfo (index) */
#include "game/camera/T392_camera_target_selector.h"   /* CameraTargetSelector, CameraTargetSlot */
#include "engine/pdistd/T531_random.h"               /* Random::next / next01 */

/* Per place: the score a car starts with, its chance per frame of +10 (a
 * chance per second / 60: the bits are those of x * (1/60) in float) and the
 * scale of its slot-state bonus. */
RankWeight g_002DF108[6] = {                /* 0x002DF108 */
    { 400.0f, 1.0f * (1.0f / 60.0f), 1.5f },
    { 300.0f, 0.6f * (1.0f / 60.0f), 0.95f },
    { 200.0f, 0.4f * (1.0f / 60.0f), 0.85f },
    { 100.0f, 0.34f * (1.0f / 60.0f), 0.75f },
    { 100.0f, 0.28f * (1.0f / 60.0f), 0.7f },
    { 100.0f, 0.2f * (1.0f / 60.0f), 0.65f },
};

/* Callers: CameraManager::init (T383), F_0020EBA0.  Restarts the scores: car
 * (the car the view follows) is both next and shown; reseeds rng from the
 * clock.  place is one variable for both loops (retail: $t2 in both). */
// FUNC 0x0020E020 F_0020E020
void F_0020E020(TargetChooser *tc, RaceOrganization *org, int car, int target)
{
    int i;
    int place;

    u32 seed = gfx_perf_counter0();
    tc->next = car;
    tc->rng.seed = seed;
    tc->shown = car;
    tc->pending = 0;
    tc->frames = 0;
    for (i = 0; i < TARGET_CARS; i++) {
        tc->car[i].score = 0;
        tc->car[i].bonus = 0;
        tc->car[i].hold = 0;
    }
    if (org == NULL)
        return;

    int count = org->entries->count;   /* compared signed */
    switch (target) {
    case VIEW_TARGET_TOP:
        /* the leader (1-based rank 1) */
        for (i = 0; i < count; i++) {
            place = org->entries->cars[i]->state->st.rank - 1;
            if (place < 0)
                place = 0;
            if (place > TARGET_PLACE_LAST)
                place = TARGET_PLACE_LAST;
            if (place == 0)
                tc->car[i].score = TARGET_SCORE_MAX;
        }
        break;
    case VIEW_TARGET_MANUAL:
        /* the view's own car */
        for (i = 0; i < count; i++) {
            if (i == car)
                tc->car[i].score = TARGET_SCORE_MAX;
        }
        break;
    case VIEW_TARGET_AUTO:
    default:
        /* automatic: the base score of each place */
        for (i = 0; i < count; i++) {
            place = org->entries->cars[i]->state->st.rank - 1;
            if (place < 0)
                place = 0;
            if (place > TARGET_PLACE_LAST)
                place = TARGET_PLACE_LAST;
            tc->car[i].score = g_002DF108[place].score;
        }
        break;
    }
}

/* Caller: CameraManager::update (T383), once a frame per view.  info is the
 * view's target info (index[0] = the car followed), sel its target selector
 * (the slots' states and this frame's / the last frame's places).
 *
 * The last loop's "drawn in full" flag is the parameter target, reused
 * (target is not read after the switch, so it computes the same): retail
 * keeps target, `change' and that flag in $s0; a local of its own gets a
 * caller-saved register (97.1%), `change' reused gets $s4 (99.2%).  An exact
 * form, not the original as far as known (the original may have had one
 * variable of another name). */
// FUNC 0x0020E1F0 F_0020E1F0
void F_0020E1F0(TargetChooser *tc, RaceOrganization *org, CameraTargetInfo *info,
                CameraTargetSelector *sel, int target)
{
    Random *rng = &tc->rng;
    int followed = info->index[0];
    int count = org->entries->count;   /* compared signed */
    int partner = sel->slot[followed].getTarget();
    int i;

    tc->frames = tc->frames + 1;
    switch (target) {
    case VIEW_TARGET_TOP:
        for (i = 0; i < count; i++) {
            int rank = sel->rankOfCar[i];   /* retail reads it before the stores */
            tc->car[i].score = 0;
            tc->car[i].bonus = 0;
            if (rank == 0)
                tc->car[i].score = TARGET_SCORE_MAX;
        }
        break;
    case VIEW_TARGET_MANUAL:
        for (i = 0; i < count; i++) {
            tc->car[i].score = 0;
            tc->car[i].bonus = 0;
            if (i == followed)
                tc->car[i].score = TARGET_SCORE_MAX;
        }
        break;
    case VIEW_TARGET_AUTO:
    default:
        /* the car followed and its slot's target lose 1/30 a frame, the
         * others gain 10 with the chance of their place (next() is drawn
         * and dropped first) */
        for (i = 0; i < count; i++) {
            int place = sel->rankOfCar[i];
            if (place < 0)
                place = 0;
            if (place > TARGET_PLACE_LAST)
                place = TARGET_PLACE_LAST;
            if (followed == i || partner == i) {
                tc->car[i].score = tc->car[i].score - 0.033333335f;
            } else {
                rng->next();
                if (rng->next01() < g_002DF108[place].chance)
                    tc->car[i].score = tc->car[i].score + 10.0f;
            }
        }
        for (; i < TARGET_CARS; i++) {
            tc->car[i].score = 0;
            tc->car[i].bonus = 0;
        }
        /* places gained / lost since the last frame (hold only counts down:
         * nothing here sets it) */
        for (i = 0; i < count; i++) {
            int before = sel->prevRankOfCar[i];
            int now = sel->rankOfCar[i];
            if (before >= 0) {
                if (tc->car[i].hold > 0) {
                    tc->car[i].hold = tc->car[i].hold - 1;
                } else if (now < before) {
                    tc->car[i].score = tc->car[i].score + 150.0f;
                } else if (before < now) {
                    tc->car[i].score = tc->car[i].score - 100.0f;
                }
            }
        }
        for (i = 0; i < count; i++) {
            if (tc->car[i].score < 0.0f)
                tc->car[i].score = 0;
            if (tc->car[i].score > TARGET_SCORE_MAX)
                tc->car[i].score = TARGET_SCORE_MAX;
        }
        /* the bonus of the slot's state, scaled by the place */
        for (i = 0; i < count; i++) {
            int place = sel->rankOfCar[i];
            if (place < 0)
                place = 0;
            if (place > TARGET_PLACE_LAST)
                place = TARGET_PLACE_LAST;
            f32 bonus = 0.0f;
            switch (sel->slot[i].state) {
            case TARGET_SLOT_CANDIDATE:
                bonus = 100.0f;
                break;
            case TARGET_SLOT_BATTLE:
                bonus = 400.0f;
                break;
            case TARGET_SLOT_NEAR:
                bonus = 200.0f;
                break;
            }
            tc->car[i].bonus = bonus * g_002DF108[place].scale;
        }
        break;
    }

    /* Time to look for another car?  Not before 240 frames; then a chance,
     * tested every 30 frames, that grows to 1 at 1800 frames (at once
     * after that).  The chance is converted, then divided, in one variable
     * (retail: cvt.s.w and div.s both in $f20). */
    int change = 0;
    if (tc->pending == 0) {
        int frames = tc->frames;
        f32 chance = (f32)(frames - 240);
        chance = chance / 1560.0f;
        if (chance < 0.0f)
            chance = 0.0f;
        if (chance > 1.0f) {
            chance = 1.0f;
            change = 1;
        }
        if (frames % 30 == 0) {
            f32 draw = rng->next01();
            if (draw >= 1.0f)
                draw = 0.0f;
            if (draw < chance)
                change = 1;
        }
    }
    if (change) {
        int best = tc->next;
        f32 bestScore = tc->car[best].score + tc->car[best].bonus;
        for (i = 0; i < count; i++) {
            RaceEntryCar *entry = org->entries->cars[i];
            target = car_display_level(entry->state) == CAR_SHOW_FULL;   /* drawn in full */
            f32 score = tc->car[i].score + tc->car[i].bonus;
            if (bestScore < score && target) {
                bestScore = score;
                best = i;
            }
        }
        if (best != tc->next) {
            tc->next = best;
            tc->pending = 1;
        }
    }
}

/* Caller: CameraManager::updateMode2 (T389), when the view switches to next:
 * next is shown from now on (+200), the others are scaled by the state of
 * next's slot, the car left restarts from the base score of its place.
 * tc->next is read once for the store to shown and the score (retail: one
 * lbu, sign-extended for the index); pending's store does not order the
 * score's load and store (TargetChooser::pending, a bool bit-field). */
// FUNC 0x0020E790 F_0020E790
void F_0020E790(TargetChooser *tc, RaceOrganization *org, CameraTargetSelector *sel)
{
    int left = tc->shown;
    int count = org->entries->count;   /* compared signed */
    int place = sel->rankOfCar[left];
    int i;

    tc->shown = tc->next;
    if (place < 0)
        place = 0;
    if (place > TARGET_PLACE_LAST)
        place = TARGET_PLACE_LAST;
    tc->pending = 0;
    tc->frames = 0;

    f32 scale = 1.0f;
    tc->car[tc->next].score = tc->car[tc->next].score + 200.0f;
    switch (sel->slot[tc->next].state) {
    case TARGET_SLOT_CANDIDATE:
        scale = 0.98f;
        break;
    case TARGET_SLOT_BATTLE:
        scale = 0.84f;
        break;
    case TARGET_SLOT_NEAR:
        scale = 0.95f;
        break;
    }
    for (i = 0; i < count; i++) {
        if (i != tc->next)
            tc->car[i].score = tc->car[i].score * scale;
    }
    tc->car[left].score = g_002DF108[place].score;
    for (i = 0; i < count; i++) {
        if (tc->car[i].score < 0.0f)
            tc->car[i].score = 0;
        if (tc->car[i].score > TARGET_SCORE_MAX)
            tc->car[i].score = TARGET_SCORE_MAX;
    }
}
