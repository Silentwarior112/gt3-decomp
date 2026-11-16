/* T324_dynamics_conductor_rules.h - the rule queries of the DynamicsConductor
 * (translation unit T324, unit u076), 0x001D9FB8..0x001DA1B8.
 *
 * T324 defines DynamicsConductor members, which T305_dynamics_conductor.h
 * declares in the class:
 *   virtual_34 .. virtual_37   0x001D9FB8 / FC0 / FC8 / FD0  return 0 (36 / 37:
 *                              the course-out levels of all four wheels on
 *                              counted ground / off the course)
 *   isPitEnabled               0x001D9FD8 returns 1 (0 in License, FreeRun,
 *                              TimeTrial): the race has pit stops;
 *                              DYN_FLAG_COUNTED_GROUND and car_tyre_check
 *                              are on
 *   chargePenaltyBySurface(RaceCarState *car, int countedWheels,
 *                          int offCourseWheels) [39]
 *       0x001DA020 the course-out rule.  car_ground_contact (T298) passes
 *       the wheels (0..4) on ground whose RunwayContact.attr2 bit 0 is set
 *       and the wheels on an off-course surface code (g_002D53D8: 2, 3, 4,
 *       6).  All four wheels on counted ground (inside the pit lane only)
 *       / off the course count at the level of slot 36 / 37 (1 counts up
 *       DynCarState.courseOut, 2 sets it to 0xFF); any count sets
 *       DynCarState.fault = FAULT_COURSE_OUT.  Skipped when fault is set or
 *       the car's driver is not the pad.
 *   issueCrashEvent(RaceCarState *car, int kind, int arg, f32 force) [40]
 *       0x001DA128 empty: the collision rule.  car_report_crash (T288)
 *       calls it with its four arguments (a1..a3, $f12); u076 declared
 *       (car, kind, f32 impact), u078 (car, kind, int arg).
 *   F_001DA130(RaceCarState *car, int kind, int arg, f32 force) 0x001DA130
 *       (non-virtual) the collision rule of the License / TimeTrial slot 40:
 *       kind 0 (a wall, T376) is a fault when force x invMass > 10, kind 2
 *       (a loose course object, T167) always, kind 1 (another car, T375)
 *       never; DynCarState.fault |= 1.  Only for carIndex 0 and while the car
 *       has not finished; arg is not read.
 * The counted wheels count only inside the pit lane
 * (pit_lane_contains(&lines, st.lapDistance, 0), T372).
 */
#ifndef GAME_T324_DYNAMICS_CONDUCTOR_RULES_H
#define GAME_T324_DYNAMICS_CONDUCTOR_RULES_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */

/* The car's conductor's virtual_34, 0 while the car's driver is not the pad
 * (DynCarState.driver != 0).  car_tyre_check (T296) compares it with 4. */
int F_001D9FE0(RaceCarState *car);          /* 0x001D9FE0 */

#endif /* GAME_T324_DYNAMICS_CONDUCTOR_RULES_H */
