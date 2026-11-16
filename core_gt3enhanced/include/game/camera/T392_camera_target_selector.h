/* T392_camera_target_selector.h - CameraTargetSelector (translation unit
 * T392, unit u093), 0x0020D5E8..0x0020E020.
 *
 * ? names.  One per camera view (CameraView +0x20, T383).  For every car i
 * it keeps a slot with the car its camera should rather show together with
 * it: a car within 30 m that it is battling with or close to, and the rank
 * tables of this frame and the last.  CameraManager::update calls update()
 * once a frame per view; the mode handlers (T388..T391) read
 * slot[target].getTarget() as the second car of the target info and react to
 * slot[target].changed; the replay target chooser (T393) reads the slots'
 * states and the rank tables (u094 called this object RaceEvents).
 *
 * Relations (relation[i][j]): carRelation = the conductor's pair state of
 * (i, j) when it is 1..3 (CarPairStates, T305), else 1 when the cars are
 * within 10 m.  Slot states: 1 candidate (not active yet), 2 battle, 3 near.
 */
#ifndef GAME_T392_CAMERA_TARGET_SELECTOR_H
#define GAME_T392_CAMERA_TARGET_SELECTOR_H

#include "game/game.h"

class CarPairStates;                        /* T305 (pointers only): DynamicsConductor +0xBA34 */
class DynamicsConductor;                    /* T305 */
class RaceEntryBase;                        /* T121 */
class RaceOrganization;                     /* T125 */

/* One car's slot.  Not polymorphic.  Size 0xC (the slot[] stride). */
class CameraTargetSlot {
public:
    void reset();                           /* 0x0020D748 everything 0, the targets -1 */
    void savePrevious();                    /* 0x0020D770 prevTarget / prevState = target / state, age 0 */
    void tick();                            /* 0x0020D788 age + 1 (at most 30000) */
    void clear();                           /* 0x0020D7B8 not active, no target, state 0 */
    void setCandidate(int car);             /* 0x0020D7D0 state 1 (not active) */
    void setBattle(int car);                /* 0x0020D7E8 state 2 */
    void setNear(int car);                  /* 0x0020D808 state 3 */
    int  getTarget();                       /* 0x0020D828 the target, -1 while not active */
    /* The mode handlers' "the slot changed this frame" test (u093 matching:
     * the code computes the flag, lh / xori 1 / bnez, as an inline's value;
     * slot->changed == 1 in place gives li 1 / bne). */
    int  isChanged() { return changed == 1; }

    s16 active;                             /* 0x00 getTarget() is -1 while 0 */
    s16 changed;                            /* 0x02 set by update() when the slot changed this frame */
    s8  target;                             /* 0x04 the car to look at, -1: none */
    s8  state;                              /* 0x05 0 none, 1 candidate, 2 battle, 3 near (? names; T393
                                               gives 1 / 2 / 3 bonuses of 100 / 400 / 200) */
    s16 timer;                              /* 0x06 frames in the state */
    s8  prevTarget;                         /* 0x08 */
    s8  prevState;                          /* 0x09 */
    s16 age;                                /* 0x0A frames since the last change (at most 30000) */
};
EGCS_SIZE(CameraTargetSlot, 0xC);

/* Not polymorphic.  Declared size 0x178, the end of the data (u093's).  In
 * CameraView the next member is at +0x1A0 (+0x20 + 0x180), and nothing reads
 * or writes CameraView +0x198..+0x19F, so 0x180 is possible too; CameraView
 * declares the 8 bytes as its own unk_198. */
class CameraTargetSelector {
public:
    void reset();                           /* 0x0020D840 (CameraManager::init) slots reset, ranks -1, holdTimer 40 */
    /* The best-placed car j within 20 m with relation[car][j] >= minRel; else
     * the worst-placed car j within 20 m with the highest relation[j][car] >=
     * minRelBack; -1: none. */
    int  findTarget(RaceEntryBase *entries, int car, int minRel, int minRelBack);  /* 0x0020D900 */
    /* Once a frame: ages the slots, rebuilds the relation and rank tables,
     * moves every slot through its states.  locked: the current camera does
     * not allow a target change (!isEnableTargetChange()). */
    void update(RaceOrganization *org, DynamicsConductor *dc, int locked);         /* 0x0020DAC0 */

    CameraTargetSlot slot[7];               /* 0x000 */
    u8  unk_054[0xC];                       /* 0x054 (not accessed; ? an eighth slot) */
    int holdTimer[7];                       /* 0x060 frames the slot is left alone (40 after reset) */
    int relation[7][7];                     /* 0x07C [i][j]: -1 i == j, 0 none, 1 near, 2 / 3 the pair state */
    s16 carAtRank[7];                       /* 0x140 the car at each place (0 = leading), -1 */
    s16 rankOfCar[7];                       /* 0x14E the place of each car (0-based), -1 */
    s16 prevCarAtRank[7];                   /* 0x15C last frame's carAtRank */
    s16 prevRankOfCar[7];                   /* 0x16A last frame's rankOfCar (-1: none) */
};
EGCS_SIZE(CameraTargetSelector, 0x178);

f32 carDistanceSq(RaceEntryBase *entries, int a, int b);    /* 0x0020D5E8 the squared distance of cars a and b (t = 0) */
int carRelation(RaceEntryBase *entries, CarPairStates *pairs, int a, int b);  /* 0x0020D6C8 pairs->state[a][b] if 1..3,
                                                                                 else 1 within 10 m, else 0 */

#endif /* GAME_T392_CAMERA_TARGET_SELECTOR_H */
