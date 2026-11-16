/* race_pod_rules.h - the Toyota Pod race's stop-area rules (the enhanced
 * engine: Lupo 2's DynamicsConductor members L2 0x001F75C8..0x001F7DBC,
 * race_logic.md M2; src/game/race/modes/race_pod.cpp, new code).  The race
 * classes and the runners: race_pod.h, which says what starts the mode.
 *
 * Lupo 2 keeps the rules' state in the conductor (dc +0xF088..+0xF0FC).
 * GT3's conductor has no room for it and is the physics area's, so the state
 * is one object for the conductor the race set the rules up on (g_podRules);
 * every query answers "no Pod race" for any other conductor.  Set up by
 * RaceBase::virtual_9 after the conductor's initialize (Lupo 2: its
 * virtual_20), run once a frame by RaceBase::virtual_29 after the
 * conductor's update (Lupo 2: the per-frame slot of
 * DynamicsConductorSinglePlayer / Battle2P), and asked at every lap end by
 * the conductors' virtual_17 (pod_rules_lap_check: the physics area's hook,
 * ref/enhanced/ported/race.md).  Kept apart from race_pod.h so the AI (T366)
 * and RaceBase (T129) see none of the race classes. */
#ifndef GAME_RACE_POD_RULES_H
#define GAME_RACE_POD_RULES_H

#include "game/game.h"

class DynamicsConductor;                    /* T305 (pointers only) */

/* ======================================================================== */
/* The stop-area rules                                                      */
/* ======================================================================== */

enum {
    POD_RULES_LICENSE_TYPE = 5,             /* RaceRuleOptions.licenseType of a Pod race */
    POD_WHEEL_IN_AREA      = 0x04,          /* RunwayContact.attr2: the road of the stop area */
    POD_PARKED             = 0x22           /* RaceCarState.podParking: the Pod sits (Lupo 2 L2 0x001E1CF0) */
};

/* The stop area: from 608 m of the lap, 33 m long (Lupo 2's .sdata L2
 * 0x0035E010 / 0x0035E012; nothing in Lupo 2 writes them). */
extern s16 g_podStopStart;
extern u8  g_podStopLength;

/* The rules' state (Lupo 2: dc +0xF088 on, +0xF08A stops, +0xF094 / +0xF098
 * the area, +0xF09C / +0xF0BC / +0xF0DC the computer cars' stop points).
 * Not polymorphic. */
class PodRaceRules {
public:
    DynamicsConductor *dc;                  /* the conductor the rules were set up for (NULL: none) */
    u8   on;                                /* 1: the Pod race's rules run */
    u8   stops[8];                          /* the stops done, per car (at most 255) */
    f32  areaStart;                         /* the stop area's lap distance, m */
    f32  areaLength;                        /* m */
    f32  stopX[8];                          /* where each computer car stops (ai_brake_factor, T366) */
    f32  stopY[8];
    f32  stopDist[8];                       /* its lap distance */
};

extern PodRaceRules g_podRules;

/* The rules on when the race's rules are licenseType 5 and the stop area
 * fits into the lap; the stops and stop points cleared.  Lupo 2 L2
 * 0x001F75C8 (DynamicsConductor::virtual_20).  Caller: RaceBase::virtual_9. */
void pod_rules_init(DynamicsConductor *dc);
/* 1 in a Pod race (L2 0x001F7708). */
int  pod_rules_on(DynamicsConductor *dc);
/* The stop area: 1 in a Pod race (L2 0x001F7720). */
int  pod_rules_area(DynamicsConductor *dc, f32 *start, f32 *length);
/* A computer car's stop point (L2 0x001F7788 / 0x001F7840): 1 when set /
 * read. */
int  pod_rules_set_stop_point(DynamicsConductor *dc, int car, f32 x, f32 y, f32 dist);
int  pod_rules_get_stop_point(DynamicsConductor *dc, int car, f32 *x, f32 *y, f32 *dist);
/* The stops car owes: its laps run less its stops (-1: no Pod race; L2
 * 0x001F79D0). */
int  pod_rules_pending(DynamicsConductor *dc, int car);
/* The car's stop is done: "it sat down" for the player's car (not in the
 * 2-player race), then stops + 1 (L2 0x001F7A38). */
int  pod_rules_stop_done(DynamicsConductor *dc, int car);
/* At the end of car's lap (the conductors' virtual_17): 0 when the lap
 * does not count - two stops owed: the car rolls to a stop
 * (car_mode_stop_roll), the lap is taken back and RACE_EVENT_CAR_
 * DISQUALIFIED posted; the caller then skips the lap's processing and sets
 * the Pod's sad emotion (physics).  1 otherwise (L2 0x001F7B20). */
int  pod_rules_lap_check(DynamicsConductor *dc, int car);
/* Once a frame: each car that owes a stop - the messages "stop and pull the
 * side brake" (in the area) / "go back to the stop area" (past it), and its
 * stop done when the Pod sits (L2 0x001F7BF0).  Caller: RaceBase::
 * virtual_29. */
void pod_rules_step(DynamicsConductor *dc);

#endif /* GAME_RACE_POD_RULES_H */
