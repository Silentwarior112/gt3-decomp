/* enhanced/physics.h - the physics area's switches (car/physics,
 * race/dynamics, T482): 1 = Lupo 2, 0 = GT3 (game/enhanced.h).  Survey:
 * ref/enhanced/survey/physics.md (entry numbers below).
 *
 * Entry 13 (the computer cars' grip handicap) shares ENH_AI_SKILL_AS_GRIP
 * with race_logic.md A8 (game/enhanced/race.h): one switch, so the rules'
 * enemySkillC2 is used either as Lupo 2's grip handicap or as GT3's km/h
 * offset, never both. */
#ifndef GAME_ENHANCED_PHYSICS_H
#define GAME_ENHANCED_PHYSICS_H

#define ENH_PHYSICS_PITCH_INERTIA        0   /* pitch inertia = yaw inertia x 1.35 in SetupCarSpec (physics.md 3) */
#define ENH_PHYSICS_REAR_ABS             0   /* rear brakeK default 50: the rear ABS slip limit halved (physics.md 4) */
#define ENH_PHYSICS_DYNPARAMS            0   /* DynParams retune: -50 kg, pitch moment x2, rolling resistance x3 on
                                                surface 0, wear floor 110, oversteer 7 deg / gain 60 (physics.md 5) */
#define ENH_PHYSICS_REAR_RIDE_HEIGHT     1   /* the rear axle uses rideHeight[1] (GT3 bug: [0] for both) (physics.md 6) */
#define ENH_PHYSICS_TYRE_REF_GRIP        0   /* TyreData.grip at the real static load, after the mass (GT3 bug) (physics.md 7) */
#define ENH_PHYSICS_AUTO_DOWNSHIFT       0   /* the automatic's 2->1 point averaged with the idle rpm (physics.md 9) */
#define ENH_PHYSICS_FIRST_GEAR_THRESHOLD 0   /* car_gear_for_speed: 1st up to the shift limit - 2000 rpm (GT3 - 500)
                                                (physics.md 10) */
#define ENH_PHYSICS_ATTESA               0   /* awdType 3 retune: gain x2, no torque limit, 0.15 at the limit, front
                                                free on the overrun, front torque in the lock test (physics.md 12) */
#define ENH_PHYSICS_ROLL_RATE            0   /* body roll rate clamped to 4 pi rad/s (GT3 pi / 2) (physics.md 14) */
#define ENH_PHYSICS_UNDERSTEER_BRAKE     0   /* the ASM's understeer brake works with the throttle open; aid flags
                                                (physics.md 15) */
#define ENH_PHYSICS_AI_OVERSTEER         1   /* computer cars' oversteer estimate 0 below 25 km/h (physics.md 16) */
#define ENH_PHYSICS_PAD_STEERING         0   /* pads: surface steering class everywhere, slower steering speeds
                                                (physics.md 17 a, b) */
#define ENH_PHYSICS_WHEEL_THROTTLE       0   /* wheel throttle through response curve 3, 0.7 u^8 + 0.3 u (GT3 mode 1,
                                                50) (physics.md 18) */
#define ENH_PHYSICS_FORCE_FEEDBACK       1   /* the wheel force feedback: two parameter sets, slip / load term,
                                                centring under the computer (physics.md 20) */
#define ENH_PHYSICS_CATCHUP_CAP          0   /* the slow-car boost's time scale capped at x1.3 (physics.md 21) */
#define ENH_PHYSICS_DISPLAY_FLAGS        1   /* computer cars set their aid flags; their steering display from the
                                                steering angle (physics.md 22 a, b) */
#define ENH_PHYSICS_ATTESA_RADII         1   /* awdType 3 with unequal tyre radii: the driven axle's radius for both
                                                (physics.md 24) */
#define ENH_PHYSICS_PAIR_CONTACT         0   /* car_pair_contact without its closing test: caution 0.25 within 4 m
                                                (physics.md 27; tuned with race_logic.md A2) */
#define ENH_PHYSICS_TIMED_END_AT_LEADER  1   /* after the time limit a car finishes only once the leader has
                                                (DynamicsConductor::virtual_17) (physics.md 29) */

/* Found during the port (not in the survey; ref/enhanced/ported/physics.md
 * 30): Lupo 2's car_drive_torque (L2 0x001FBEC8..0x001FC338). */
#define ENH_PHYSICS_AWD_COUPLING         0   /* 4WD SPLIT / SPEED / STEER / FIXED: the coupling torque flows from the
                                                faster axle to the slower one (GT3: a fixed signed share), and a
                                                locked centre lets go when the strain exceeds that torque (GT3: the
                                                coupling limit awdTorque) (physics.md 30) */

#endif /* GAME_ENHANCED_PHYSICS_H */
