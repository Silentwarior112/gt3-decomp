/* T303_race_time_events.h - the best lap of a car and the race display
 * events of the times (translation unit T303, unit u082),
 * 0x001EC4B0..0x001ECBA8.
 *
 * The conductors' gate handlers (DynamicsConductor slots 15 / 17 and their
 * overrides) post, through these, the display events of a car's section and
 * lap times (RaceDisplayCheckPointEvent / LapTimeEvent, then a
 * RaceDisplayGeneralTimeEvent with the time), the differences to the race's
 * reference lap (RaceDisplayTimeDiffEvent), the speed (SpeedEvent) and the lap
 * differences (LapDiffEvent), and record the times in the race organization.
 *
 * T303 also emits out-of-line copies of the inline functions of three T154
 * event classes (their classes are T154_race_display.h's, the vtable copies
 * 0x0033C8F8 / 0x0033C918 / 0x0033C938 are unused: the code stores T154's):
 *   RaceDisplayGeneralTimeEvent::getEventType 0x001ECAA0 (0xE; T154 0x001956D8)
 *   RaceDisplayLapDiffEvent::getEventType     0x001ECAF8 (0x10; T154 0x00195730)
 *   RaceDisplayTimeDiffEvent::getEventType    0x001ECB50 (0xF; T154 0x00195788)
 * and of their typeinfo functions (below). */
#ifndef GAME_T303_RACE_TIME_EVENTS_H
#define GAME_T303_RACE_TIME_EVENTS_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */
class DynamicsConductor;                    /* T305_dynamics_conductor.h */
class RaceOrganization;                     /* T125_race_organization.h */
class type_info;

void car_reset_best_lap(RaceCarState *car);                            /* 0x001EC4B0 bestLap = 0x157529FF */
int  car_update_best_lap(RaceCarState *car, u32 t);                    /* 0x001EC4C0 1 when t beats bestLap (unsigned) */
u32  car_best_lap(RaceCarState *car);                                  /* 0x001EC4E0 bestLap */
/* 1 when the car's times are not recorded: no controller, the conductor's
 * mode (a replay), a race preview or the background mode. */
int  race_times_hidden(RaceOrganization *org, DynamicsConductor *dc, RaceCarState *car); /* 0x001EC4E8 */
/* From the slot 15 handlers (lap: their lap - 1; noRecord: st.fault).  The
 * CheckPoint and GeneralTime events, then the organization's section
 * record.  unused4 is passed by every caller and not read. */
void dc_post_section_time(DynamicsConductor *dc, int car, int lap, int section, u32 unused4,
                          u32 time, int noRecord);                     /* 0x001EC520 */
/* From the slot 17 handlers: the car's best lap, the LapTime and GeneralTime
 * events, the entry's lap times, its best lap and the organization's record.
 * unused3 is not read. */
void dc_post_lap_time(DynamicsConductor *dc, int car, int lap, u32 unused3, u32 time,
                      int noRecord);                                   /* 0x001EC620 */
/* FreeRun / TimeTrial: time minus the reference lap's section / total
 * (RaceOrganization +0x8AC), unless the conductor's mode, st.fault or no
 * reference. */
void dc_post_section_diff(DynamicsConductor *dc, int car, int section, u32 time); /* 0x001EC758 */
void dc_post_lap_diff(DynamicsConductor *dc, int car, u32 time);       /* 0x001EC818 */
/* MachineTest slot 18: the entry's top speed (when recorded) and a speed
 * event in the speed unit.  noRecord: the caller's st.fault, last as for
 * dc_post_lap_time; not read.  (u084: speed before noRecord: the caller
 * loads the fault first, egcs computes register arguments last to first.) */
void dc_post_speed(DynamicsConductor *dc, int car, f32 speed, int noRecord); /* 0x001EC8C0 */
/* Empty.  DynamicsConductor::reportGateTimes (T307) calls them with these
 * arguments (u082 declared them (void)). */
void F_001EC960(DynamicsConductor *dc, int car);                       /* 0x001EC960 */
void F_001EC968(DynamicsConductor *dc, int car);                       /* 0x001EC968 */
void F_001EC970(DynamicsConductor *dc, int car);                       /* 0x001EC970 */
void F_001EC978(DynamicsConductor *dc, int car, int diff);             /* 0x001EC978 */
/* A RaceDisplayTimeDiffEvent of diff ms (reportGateTimes; the two are the
 * same code). */
void dc_post_time_diff(DynamicsConductor *dc, int car, int diff);      /* 0x001EC980 */
void dc_post_time_diff2(DynamicsConductor *dc, int car, int diff);     /* 0x001EC9E0 */
/* A RaceDisplayLapDiffEvent of diff laps (recordGate, T308). */
void dc_post_lap_count_diff(DynamicsConductor *dc, int car, int diff); /* 0x001ECA40 */

/* The copies of the T154 classes' typeinfo functions.  They fill the shared
 * __si_type_info words of T154 (0x004B2190, 0x004B21C0, 0x004B2170; base
 * __ti20RaceDisplayDiffEvent 0x004B2180) with this unit's copies of the name
 * strings (0x0033C958 / 0x0033C978 / 0x0033C998).  egcs generates them from
 * the classes, so only the modern build sees these declarations; they must
 * agree with T154's. */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf27RaceDisplayGeneralTimeEvent(void);  /* 0x001ECAA8 (T154 0x001956E0) */
const type_info &__tf23RaceDisplayLapDiffEvent(void);      /* 0x001ECB00 (T154 0x00195738) */
const type_info &__tf24RaceDisplayTimeDiffEvent(void);     /* 0x001ECB58 (T154 0x00195790, T315 0x001F26F8) */
}
#endif

#endif /* GAME_T303_RACE_TIME_EVENTS_H */
