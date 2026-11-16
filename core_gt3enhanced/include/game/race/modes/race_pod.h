/* race_pod.h - the Toyota Pod race (the enhanced engine: Lupo 2's RacePOD /
 * RacePOD2P, race_logic.md M2; src/game/race/modes/race_pod.cpp and
 * race_pod_runners.cpp, new code, no GT3 original).
 *
 * The Pod race is an arcade race on a course with a stop area before a
 * jump ramp.  Every lap a car has to stop in that area with all four
 * wheels on its marked surface and make the Pod sit down by pulling the
 * side brake; a car that crosses the line with two stops owed is
 * disqualified.  Computer cars do it too (the AI, T366: race_logic.md A10).
 *
 * What starts it (none of it is in GT3's data, so with GT3's content the
 * mode never runs):
 *   - a race record whose rules have licenseType 5 (RaceRuleOptions, T326;
 *     GT3's records use 1..3), on a course at least 641 m long (the stop area
 *     is 608..641 m: g_podStopStart / g_podStopLength, Lupo 2's .sdata);
 *   - the stop area's road triangles marked with attr2 bit 2 (the wheels'
 *     RunwayContact.attr2 & POD_WHEEL_IN_AREA): the Lupo 2 Pod course;
 *   - a Pod (CarDynamics.specialKind CAR_KIND_POD) to sit down: the parking
 *     counter RaceCarState.podParking (physics) reaches POD_PARKED only for
 *     it, so another car can never complete a stop;
 *   - the arcade menu's Pod entry calling run_pod_race / run_pod_2p_race
 *     (menus area, menus_a.md 9 / 14).
 *
 * The rules: race_pod_rules.h. */
#ifndef GAME_RACE_POD_H
#define GAME_RACE_POD_H

#include "game/game.h"
#include <stddef.h>                         /* size_t */
#include "game/race/modes/T134_race_arcade.h"          /* base RaceArcade */
#include "game/race/modes/T137_race_split_battle.h"    /* base RaceSplitBattle */
#include "game/race/display/T141_race_time_trial_display.h"   /* RacePODDisplay (held by value; hud.md C21) */
#include "game/race/display/T155_race_pod2p_display.h"   /* RacePOD2PDisplay (held by value; hud.md C22) */
#include "game/race/modes/race_pod_rules.h"     /* the stop-area rules */

class RaceBase;                             /* T129 (pointers only) */
struct MenuArgs;                            /* T000 */

/* ======================================================================== */
/* The race classes                                                         */
/* ======================================================================== */

/* Lupo 2's RacePOD ("RacePOD", registry L2 0x002DD520): RaceArcade with its
 * own display (the hud area's RacePODDisplay: the time trial's display with
 * the rules on the event board at the start, and the disqualification
 * message), which becomes the race display (the vtable differs from
 * RaceArcade's only in the destructor and the name).  Size RaceArcade's +
 * the display (Lupo 2 0x3E200). */
class RacePOD : public RaceArcade {
public:
    static void *operator new(size_t size);                 /* the race pool */
    static void  operator delete(void *p, size_t size);
    RacePOD();                                              /* L2 0x0018D498 */
    virtual ~RacePOD();                                     /* L2 0x0018D548 */
    virtual const char *virtual_24();                       /* L2 0x0018D420 "RacePOD" */

    RacePODDisplay   podDisplay;            /* 0x34C00 (L2 at +0x338C0) */
};
/* The enhanced engine: RaceArcade 0x34C00 + RacePODDisplay 0xA91C, rounded
 * up to the class's 64-byte alignment (tools/sizeprobe.py). */
EGCS_SIZE(RacePOD, 0x3F540);

/* Lupo 2's RacePOD2P ("RacePOD2P", registry L2 0x002DD530): the split
 * battle with the Pod's split display (the hud area's RacePOD2PDisplay: two
 * RacePODDisplays whose disqualification messages hold; it ends the race
 * once both players are disqualified), which becomes the race display and
 * the one the battle drives (RaceSplitBattle::podDisplay: Lupo 2 split its
 * battle into RaceSplitBattleBase and RaceSplitBattle with a virtual
 * getDisplay for this).  The vtable differs from RaceSplitBattle's only in
 * the destructor, the name and getDisplay.  Size RaceSplitBattle's + the
 * display (Lupo 2 0x3E480). */
class RacePOD2P : public RaceSplitBattle {
public:
    static void *operator new(size_t size);                 /* the race pool */
    static void  operator delete(void *p, size_t size);
    RacePOD2P();                                            /* L2 0x0018D6A0 */
    virtual ~RacePOD2P();                                   /* L2 0x0018D740 */
    virtual const char *virtual_24();                       /* L2 0x0018D628 "RacePOD2P" */

    RacePOD2PDisplay pod2PDisplay;          /* 0x3F740 (L2 at +0x3E400 - 0x15244) */
};
/* The enhanced engine: RaceSplitBattle 0x3F740 + RacePOD2PDisplay 0x15244,
 * rounded up to 64 bytes: the largest registered race class (g_00351DF0,
 * T000's race work area; the runner keeps it on the 1 MB main stack as GT3's
 * run_split_battle keeps RaceSplitBattle). */
EGCS_SIZE(RacePOD2P, 0x549C0);

/* The registry records "RacePOD" / "RacePOD2P" and their functions (L2
 * 0x0018D430 / 0x0018D468, 0x0018D638 / 0x0018D670). */
RaceBase *RacePOD_create(void);
RaceBase *RacePOD_construct(void *buf, int arg);
RaceBase *RacePOD2P_create(void);
RaceBase *RacePOD2P_construct(void *buf, int arg);

/* ======================================================================== */
/* The runners (app code; the arcade menu's Pod entry calls them)           */
/* ======================================================================== */

/* The Pod race (Lupo 2 L2 0x0012E018): GT3's arcade single race
 * (run_arcade_race, T052) with RacePOD; args->race the Pod race's record,
 * args->course its course.  Returns 0. */
int run_pod_race(MenuArgs *args);
/* The 2-player Pod race (Lupo 2 L2 0x00130CB8): GT3's split battle
 * (run_split_battle, T054) with RacePOD2P, on args->race (Lupo 2's menu
 * passes "APO20001") and args->course.  Returns 0. */
int run_pod_2p_race(MenuArgs *args);

#endif /* GAME_RACE_POD_H */
