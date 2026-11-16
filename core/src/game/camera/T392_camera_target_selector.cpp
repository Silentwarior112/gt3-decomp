/* T392_camera_target_selector.cpp - CameraTargetSelector (0x0020D5E8..0x0020E020).
 *
 * One per camera view (CameraView +0x20).  For every car i it keeps a slot
 * with the car its camera should rather show together with it: a car within
 * 30 m that it is battling with or close to.  CameraManager::update
 * (0x00208BE8) calls update() once a frame per view (outside the countdown);
 * the mode handlers read slot[target].getTarget() as the second car of the
 * target info and react to slot[target].changed; the target chooser (T393)
 * reads the slot states and the rank tables.
 *
 * Relations (relation[i][j]): carRelation() = the conductor's pair state of
 * (i, j) (CarPairStates, T305) when it is 1..3, else 1 when the cars are
 * within 10 m.  Slot states: 1 candidate (not active yet), 2 battle, 3 near
 * (? names).
 */
/* Retail T392's .rodata (0x0033F420): <new>'s "bad_alloc", the type names
 * "14RaceCameraBase", "t9UnitArena1Ui64" (nothing references them:
 * DATA_START).  Its switches are decision trees, no jump table. */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/camera/camera_modes_internal.h"
#include "game/camera/T392_camera_target_selector.h"   /* T392 */
// DATA_START 0x0033F420

/* CameraTargetSlot::state */
enum {
    SLOT_NONE      = 0,
    SLOT_CANDIDATE = 1,                     /* a car within 20 m in a relation (not active: getTarget -1) */
    SLOT_BATTLE    = 2,
    SLOT_NEAR      = 3
};

/* carRelation's values (relation[][] adds -1 for i == j) */
enum {
    REL_NONE = 0,
    REL_NEAR = 1                            /* within 10 m (or the pair state 1); 2 / 3: the pair state */
};

// FUNC 0x0020D5E8 carDistanceSq
//   The squared distance between cars a and b (their view positions at
//   t = 0).
f32 carDistanceSq(RaceEntryBase *entries, int a, int b)
{
    RaceCarState *carA = entries->cars[a]->state;
    RaceCarState *carB = entries->cars[b]->state;
    f32 dx = car_view_x(carA, 0.0f) - car_view_x(carB, 0.0f);
    f32 dy = car_view_y(carA, 0.0f) - car_view_y(carB, 0.0f);
    f32 dz = car_view_z(carA, 0.0f) - car_view_z(carB, 0.0f);

    return dx * dx + dy * dy + dz * dz;
}

// FUNC 0x0020D6C8 carRelation
//   The pair state of (a, b) when it is 1..3, else 1 when the cars are less
//   than 10 m apart, else 0.
int carRelation(RaceEntryBase *entries, CarPairStates *pairs, int a, int b)
{
    int relation;
    int state;

    if (carDistanceSq(entries, a, b) < 100.0f)
        relation = REL_NEAR;
    else
        relation = REL_NONE;
    state = pairs->state[a][b];
    if (state < 4) {
        if (state > 0)
            relation = state;
    }
    return relation;
}

// FUNC 0x0020D748 CameraTargetSlot::reset
void CameraTargetSlot::reset()
{
    prevTarget = -1;
    target = -1;
    age = 0;
    timer = 0;
    prevState = SLOT_NONE;
    state = SLOT_NONE;
    active = 0;
    changed = 0;
}

// FUNC 0x0020D770 CameraTargetSlot::savePrevious
void CameraTargetSlot::savePrevious()
{
    prevTarget = target;
    prevState = state;
    age = 0;
}

// FUNC 0x0020D788 CameraTargetSlot::tick
void CameraTargetSlot::tick()
{
    age = age + 1;
    if (age >= 30001)
        age = 30000;
}

// FUNC 0x0020D7B8 CameraTargetSlot::clear
void CameraTargetSlot::clear()
{
    target = -1;
    timer = 0;
    state = SLOT_NONE;
    active = 0;
}

// FUNC 0x0020D7D0 CameraTargetSlot::setCandidate
void CameraTargetSlot::setCandidate(int car)
{
    target = car;
    state = SLOT_CANDIDATE;
    timer = 0;
    active = 0;
}

// FUNC 0x0020D7E8 CameraTargetSlot::setBattle
void CameraTargetSlot::setBattle(int car)
{
    target = car;
    state = SLOT_BATTLE;
    timer = 0;
    active = 1;
}

// FUNC 0x0020D808 CameraTargetSlot::setNear
void CameraTargetSlot::setNear(int car)
{
    target = car;
    state = SLOT_NEAR;
    timer = 0;
    active = 1;
}

// FUNC 0x0020D828 CameraTargetSlot::getTarget
//   The target car, or -1 while the slot is not active.
int CameraTargetSlot::getTarget()
{
    int car = -1;

    if (active != 0)
        car = target;
    return car;
}

// FUNC 0x0020D840 CameraTargetSelector::reset
//   CameraManager::init (0x00208320): every slot reset, the rank tables -1,
//   holdTimer 40.
void CameraTargetSelector::reset()
{
    int i;

    for (i = 6; i >= 0; i--) {
        slot[i].reset();
        holdTimer[i] = 40;
        carAtRank[i] = -1;
        prevCarAtRank[i] = -1;
        rankOfCar[i] = -1;
        prevRankOfCar[i] = -1;
    }
}

// FUNC 0x0020D900 CameraTargetSelector::findTarget
//   The best-placed car j within 20 m with relation[car][j] >= minRel; if
//   there is none, the worst-placed car j within 20 m with the highest
//   relation[j][car] >= minRelBack.  -1 if none.
int CameraTargetSelector::findTarget(RaceEntryBase *entries, int car, int minRel, int minRelBack)
{
    int carCount = entries->count;     /* compared signed */
    int best = -1;
    int bestRank = 7;
    int rank;
    int j;

    for (j = 0; j < carCount; j++) {
        if (relation[car][j] >= minRel) {
            rank = rankOfCar[j];
            if (rank < bestRank && carDistanceSq(entries, car, j) < 400.0f) {
                best = j;
                bestRank = rank;
            }
        }
    }
    if (best < 0) {
        int bestRelation;
        bestRank = -1;
        bestRelation = 0;
        for (j = 0; j < carCount; j++) {
            int rel = relation[j][car];
            if (rel >= minRelBack) {
                rank = rankOfCar[j];
                if (bestRank < rank && rel >= bestRelation) {
                    if (carDistanceSq(entries, car, j) < 400.0f) {
                        best = j;
                        bestRank = rank;
                        bestRelation = rel;
                    }
                }
            }
        }
    }
    return best;
}

// FUNC 0x0020DAC0 CameraTargetSelector::update
//   Once a frame: ages the slots, rebuilds the relation and rank tables, then
//   moves every car's slot through its states.  A slot held (holdTimer, set
//   by the mode handlers when they retarget) is left alone.  locked: the
//   current camera does not allow a target change (!isEnableTargetChange()):
//   no new candidates, no switch to another battle, a candidate is dropped.
//   changed = 1 when the slot's shown car changes.
void CameraTargetSelector::update(RaceOrganization *org, DynamicsConductor *dc, int locked)
{
    RaceEntryBase *entries = org->entries;
    CarPairStates *pairs = &dc->pairs;
    int carCount = entries->count;     /* compared signed */
    int i;
    int j;
    int k;                                  /* the slot loop's own index (s5; i is s1) */

    for (i = 6; i >= 0; i--) {
        CameraTargetSlot *s = &slot[i];
        s->tick();
        s->changed = 0;
    }

    for (i = 0; i < carCount; i++) {
        for (j = 0; j < carCount; j++) {
            int rel = -1;
            if (i != j) {
                switch (carRelation(entries, pairs, i, j)) {
                case 1:
                    rel = 1;
                    break;
                case 2:
                    rel = 2;
                    break;
                case 3:
                    rel = 3;
                    break;
                default:
                    rel = 0;
                    break;
                }
            }
            relation[i][j] = rel;
        }
    }

    for (i = 0; i < 7; i++) {
        prevCarAtRank[i] = carAtRank[i];
        prevRankOfCar[i] = rankOfCar[i];
        carAtRank[i] = -1;
        rankOfCar[i] = -1;
    }
    for (i = 0; i < carCount; i++) {
        int rank = org->entries->cars[i]->state->st.rank - 1;  /* 1-based in the car */
        carAtRank[rank] = i;
        rankOfCar[i] = rank;
    }

    for (k = 0; k < carCount; k++) {
        CameraTargetSlot *s = &slot[k];
        int other = s->target;
        int held = 0;
        int found;                          /* findTarget's car (one variable for both calls: s2) */

        if (holdTimer[k] > 0) {
            held = 1;
            holdTimer[k] = holdTimer[k] - 1;
        }

        if (other >= 0) {
            int rel;
            int state;

            if (held)
                continue;
            found = findTarget(entries, k, 2, 1);  /* a car in a pair state 2 / 3 */
            rel = carRelation(entries, pairs, k, other);
            if (rel == REL_NONE)
                rel = carRelation(entries, pairs, other, k);
            state = s->state;

            switch (state) {
            case SLOT_CANDIDATE:
                if (locked) {
                    s->savePrevious();
                    s->clear();
                    s->changed = 1;
                } else if (found >= 0 && other != found) {
                    s->savePrevious();
                    s->setBattle(found);
                    s->changed = 1;
                } else if (rel == REL_NEAR) {
                    /* near for 2 s: active */
                    s->timer = s->timer + 1;
                    if (s->timer >= 121) {
                        s->savePrevious();
                        s->setNear(other);
                        s->changed = 1;
                    }
                } else if (rel > 0) {
                    if (rel < 4) {
                        s->savePrevious();
                        s->setBattle(other);
                        s->changed = 1;
                    } else
                        goto drop;
                } else {
                    goto drop;
                }
                break;

            case SLOT_BATTLE:
                if (carDistanceSq(entries, k, other) > 900.0f)
                    goto drop;
                if (s->timer < 240) {
                    s->timer = s->timer + 1;
                } else if (!((u32)(rel - 2) < 2)) {
                    /* 4 s on, once out of the pair states 2 / 3: near */
                    s->savePrevious();
                    s->setNear(other);
                }
                break;

            case SLOT_NEAR:
                if (carDistanceSq(entries, k, other) > 900.0f)
                    goto drop;
                if (locked)
                    break;
                if (found >= 0 && other != found) {
                    s->savePrevious();
                    s->setBattle(found);
                    s->changed = 1;
                    break;
                }
                s->timer = s->timer + 1;
                if (s->timer >= 241) {
            drop:
                    /* more than 30 m apart, out of any relation, or near too long */
                    s->savePrevious();
                    s->clear();
                    s->changed = 1;
                    break;
                }
                if ((u32)(rel - 2) < 2) {
                    s->savePrevious();
                    s->setBattle(other);
                }
                break;

            default:
                s->clear();
                break;
            }
        } else {
            /* no target: after 240 frames look for a candidate */
            if (held)
                continue;
            if (s->timer < 240) {
                s->timer = s->timer + 1;
            } else if (!locked) {
                found = findTarget(entries, k, 1, 1);
                if (found >= 0) {
                    s->savePrevious();
                    s->setCandidate(found);
                }
            }
        }
    }
}
