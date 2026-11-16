/* T366_ai_drive.h - the computer driver's driving: the braking point, the
 * throttle for a target speed, the look-ahead point, the steering, the
 * recovery (AIDriver.recover 1 backing up, 2 turning round forward) and the
 * copy of the outputs into the car (translation unit T366, unit u086),
 * 0x001F7078..0x001F8638.
 *
 * Called by car_input_ai (T358: ai_drive, ai_idle, ai_rev_engine) and
 * car_input_ai_apply (T342: ai_apply_outputs).  The run state's mode byte
 * (DynCarState.ctrlMode +0x534, set by T341's requests) steers it: 7 stop,
 * 5 / 6 stop with the steering, 3 (at the pit box) a 5 m look-ahead, 4
 * (leaving the pits) with inPitLane 20 m.
 *
 * CarRunState: u076 / u080 / u083 / u084 / u086's name for the run state of
 * a car (RaceCarState +0xF84, 0x538 bytes), which T288 defines as
 * DynCarState with the names of all its users merged.  It is a typedef here,
 * with no second definition: other headers must not forward-declare
 * `class CarRunState'. */
#ifndef GAME_T366_AI_DRIVE_H
#define GAME_T366_AI_DRIVE_H

#include "game/game.h"
#include "game/sound/T288_car_sound.h"            /* DynCarState (CarRunState) */

class DynCarState;                          /* (defined in T288) */
typedef DynCarState CarRunState;            /* u076, u080, u083, u084, u086 */

struct AIDriver;                            /* T361_drive_params.h (pointers only) */
struct AIParams;                            /* T362_ai_params.h */
struct CourseLines;                         /* T368_enemy_lines.h */

/* The corner index, 0 past the last one. */
int  ai_corner_index(int idx, int count);                                       /* 0x001F7078 */
/* Steps ai->corner along the course distance (wrapping at half a lap), then,
 * heading for a slower corner, compares the braking distance (v^2 - vt^2) /
 * (2 mu g), adjusted for the pitch, the node's brakeAdj, brakeScale and the
 * caution, with the distance to the corner: past it the state becomes
 * braking (3 / 4); near it brakeState 1 and, with brakeScale > 1, a throttle
 * factor running down to 0.  Returns that factor x throttleScale (not on
 * line 3; 1 on line 4). */
f32  ai_brake_factor(AIDriver *ai);                                             /* 0x001F7088 */
/* base + gain x (speed - target), the difference held to +-10 km/h; 0..1. */
f32  ai_speed_throttle(f32 speed, f32 target, f32 base, f32 gain);              /* 0x001F7520 */
/* The throttle that holds target (aiming 2 km/h under it); full brake when
 * it is 0 and the car is faster; shiftTimer 12 for a part throttle. */
void ai_hold_speed(AIDriver *ai, f32 target, f32 gain);                         /* 0x001F7590 */
/* The point dist m ahead on `line' (line_point_at; line 4
 * line_point_at_linear), pushed 1 m further at a time (up to 9 m) until it is
 * at least 5 m from the car.  Returns the distance used; *dx, *dy: the point
 * relative to the car.  line, corner: int (retail takes them unmasked). */
f32  ai_look_ahead(CourseLines *lines, DynCarState *run, int line, int corner,
                   f32 *dx, f32 *dy, f32 dist);                                 /* 0x001F7658 */
/* Turns ai->steer toward target at params.steerRate rad/s. */
void ai_steer_toward(AIDriver *ai, f32 target);                                 /* 0x001F77B8 */
/* No throttle, full brake (not in mode 5) until the car is still (under
 * 0.25 km/h and 3 deg/s), then car_mode_still (T341: mode 8). */
void ai_stop(AIDriver *ai, f32 steer);                                          /* 0x001F7810 */
/* The outputs cleared, the steering back to 0. */
void ai_idle(AIDriver *ai);                                                     /* 0x001F7900 */
/* The look-ahead distance: lookNear at rest up to lookFar at 1 / lookInvSpeed. */
f32  ai_look_distance(AIParams *p, f32 speed);                                  /* 0x001F7948 */
/* Clears the outputs (the steering from the car's), runs lineBlend down by
 * the frame time. */
void ai_reset_outputs(AIDriver *ai);                                            /* 0x001F79A0 */
/* Copies the outputs into the car's run state (car_set_brake for the
 * brake); in a corner (state 1 / 2) the throttle moves by at most the frame
 * time a frame. */
void ai_apply_outputs(AIDriver *ai);                                            /* 0x001F79F0 */
/* A frame of driving: the look-ahead point (blended with the old line after a
 * change), forward or reverse and the recovery state from the heading error,
 * the steering toward the yaw rate that reaches the point (limited by the
 * tyres' slip angle), the throttle of the state (full, corner, braking, the
 * pit lane) x ai_brake_factor, then the traffic (ai_avoid_traffic) from
 * (rank x 100 + 5000) ms of the conductor's clock on (clock / 3: the clock
 * counts 1/3 ms). */
void ai_drive(AIDriver *ai);                                                    /* 0x001F7AE0 */

#endif /* GAME_T366_AI_DRIVE_H */
