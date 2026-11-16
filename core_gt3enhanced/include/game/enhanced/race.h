/* enhanced/race.h - the race area's switches (race/core, race/modes,
 * race/ai, race/pause, engine/input): 1 = Lupo 2, 0 = GT3
 * (game/enhanced.h).  Survey: ref/enhanced/survey/race_logic.md (entry
 * numbers below), backend.md B3. */
#ifndef GAME_ENHANCED_RACE_H
#define GAME_ENHANCED_RACE_H

#define ENH_AI_TORQUE_HANDICAP      0   /* ai_brake_factor: a throttle scale < 1 as a share of the net engine
                                           torque (race_logic.md A1) */
#define ENH_AI_CAUTION              0   /* ai_note_closing: full caution from 5 km/h closing, half under 5 s,
                                           quarter under 4 m (race_logic.md A2) */
#define ENH_AI_TRAFFIC              0   /* ai_avoid_traffic: 180 frames between line changes, none while
                                           blending, calmer pit approach (race_logic.md A3) */
#define ENH_AI_START_LOOKAHEAD      0   /* computer cars in a standing / slow start look up to 200 m ahead for
                                           5 s (race_logic.md A4) */
#define ENH_AI_PIT_EXIT             0   /* pit-exit look-ahead 5 m below 30 km/h (race_logic.md A5) */
#define ENH_AI_RECOVERY             0   /* back up when 60 deg off the line's own direction (race_logic.md A6) */
#define ENH_AI_SLIP_STEER           0   /* the yaw-rate gain eased by the front slip (RaceCarState.frontSlip)
                                           (race_logic.md A7) */
#define ENH_AI_DRIVER_SPREAD        0   /* the per-car spread of computer drivers in every race, not only in
                                           conductor mode 2 (race_logic.md A8 a) */
#define ENH_AI_SKILL_AS_GRIP        0   /* rules.enemySkillC2 as the computer cars' grip handicap (physics), no
                                           longer as the AI's km/h offset; never both (race_logic.md A8 b +
                                           physics.md 13) */
#define ENH_AI_COOL_DOWN            0   /* ai_reduce_pace: throttle x0.5 (at least 0.4), corners x0.5, braking
                                           x0.8 (race_logic.md A9) */
#define ENH_RACE_GHOST_CAR          1   /* the best-lap ghost keeps the car, condition and aids of its lap
                                           (race_logic.md M1; GT3's ghost file format unchanged) */
#define ENH_RACE_TIME_UP_ENDS       0   /* RACE_EVENT_TIME_UP ends every race (RaceBase::virtual_27)
                                           (race_logic.md C1) */
#define ENH_RACE_TIME_LIMIT_GRACE   0   /* timeRemain = timeLimit x 60 + 59 (race_logic.md C2) */
#define ENH_RACE_AFTERIMAGE         1   /* the replay afterimage keeps its field mode (race_logic.md D1) */
#define ENH_INPUT_WHEEL_MENU_STEER  1   /* menu left / right from the wheel's position, repeat from its
                                           deflection (backend.md B3) */

#endif /* GAME_ENHANCED_RACE_H */
