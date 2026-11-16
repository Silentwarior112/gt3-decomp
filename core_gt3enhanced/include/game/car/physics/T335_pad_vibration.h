/* T335_pad_vibration.h - DynWheel (translation unit T335, unit u078),
 * 0x001DF2B0..0x001DF4C0.
 *
 * T335 drives the pad's big vibration motor of a car (pad_vibration_update,
 * called by the per-car step car_step, T302): the largest of the front
 * wheels' slip x drive / brake force terms, the lift-off shake and the
 * wheels' damper forces (the bumps, kerbs on surface 1), x 256, at most 127,
 * to RaceCarState's PadVibration.big.  pad_vibration_send (T336) sends it.
 *
 * DynWheel: one wheel of the dynamics state (DynCarState +0x60, 4 of them:
 * FL FR RL RR; the plan puts it here).  u079 / u080 / u081 / u086 call the
 * same record CarWheelState (T356: a typedef of this).  Names: u081 (the
 * code of T296..T299 that fills most of it), else u082, u080, u078; see
 * ref/pass2/groups/h18/notes.md.  u078's names: groundZ = y, offX / offY =
 * offX / offZ (course axes), velX / velY = v4C / v50, slip = f68,
 * damperForce = f74, latForce = f80, driveForce = f88, brakeForce = f8C. */
#ifndef GAME_T335_PAD_VIBRATION_H
#define GAME_T335_PAD_VIBRATION_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */

/* One wheel of the dynamics state.  Not polymorphic.  Size 0xA4 (the
 * stride of DynCarState.wheel). */
class DynWheel {
public:
    f32 groundZ;                            /* 0x00 the ground height under the wheel (car_ground_contact) */
    f32 groundZ2;                           /* 0x04 the same (u082: the contact height) */
    f32 load;                               /* 0x08 0: in the air */
    f32 steer;                              /* 0x0C rad, the wheel's steering angle (T297) */
    f32 camber;                             /* 0x10 (wheel_camber, T298) */
    f32 lateralPos;                         /* 0x14 (? the contact point's y: wheel_lateral_pos, T298) */
    f32 stroke;                             /* 0x18 the suspension travel */
    f32 strokeSpeed;                        /* 0x1C */
    f32 spin;                               /* 0x20 the tread speed, m/s: GearCalc.tyreRadius x axleSpeed (T344) */
    u8  surface;                            /* 0x24 the surface type (ground_surface_type); F_001DB9B8 picks the tyre by it */
    u8  b25;                                /* 0x25 the ground contact's +0x32 (car_ground_contact) */
    u8  b26;                                /* 0x26 the ground contact's +0x33: bit 0 counted ground, bit 1 remapped */
    /* The enhanced engine: the lateral force use x 128 (car_tyre_lat_forces;
     * car_brake_aids' ABS blend of compound flag A reads it; Lupo 2's wheel
     * +0x27, physics.md 8).  GT3 never used the byte. */
    u8  latUse;                             /* 0x27 */
    u8  needsService;                       /* 0x28 read by car_tyre_check (T296), the pit box (T350); no source sets it */
    u8  b29;                                /* 0x29 */
    u8  contactTimer;                       /* 0x2A 6 after a frame with load, counts down (T335) */
    u8  unk_2B;                             /* 0x2B */
    f32 smoke;                              /* 0x2C 0..1 the tyre smoke (T333) */
    f32 squealVolume;                       /* 0x30 (T333; u081: the slip that makes the tyre wear / squeal) */
    f32 squealPitch;                        /* 0x34 (T333) */
    f32 rotation;                           /* 0x38 rad, kept within +-2 pi */
    f32 offX;                               /* 0x3C the wheel's offset from the body, world axes (T299) */
    f32 offY;                               /* 0x40 */
    f32 offZ;                               /* 0x44 */
    f32 slipGrip;                           /* 0x48 slip_grip_factor (0x001E6FE0, T296) */
    f32 velX;                               /* 0x4C the wheel's velocity, body axes: forward */
    f32 velY;                               /* 0x50 lateral */
    f32 grip;                               /* 0x54 load x the tyre's load / camber curves x the factors (T300) */
    f32 wearGrip;                           /* 0x58 the grip factor of the tyre wear */
    u8  squeal;                             /* 0x5C 0..255 (? the tyre sound level) */
    s8  wearLevel;                          /* 0x5D -0x80 unworn .. 0x7F worn out (?) */
    u8  unk_5E;                             /* 0x5E */
    u8  flags;                              /* 0x5F WHEEL_* (below) */
    f32 bump;                               /* 0x60 the surface bump height */
    f32 forceMax;                           /* 0x64 the friction circle (F_001D9098) */
    f32 slip;                               /* 0x68 the slip ratio (tyre_slip_ratio, T296) */
    f32 speedFactor;                        /* 0x6C min(|vel| x 0.02, 1): scales the bumps (T348) */
    f32 springForce;                        /* 0x70 */
    f32 damperForce;                        /* 0x74 */
    f32 velAngle;                           /* 0x78 atan2(-velY, velX) when moving (T300) */
    f32 lowSpeedFactor;                     /* 0x7C 0 below 1 km/h .. 1 from 10 km/h (T300): scales the tyre forces */
    f32 latForce;                           /* 0x80 */
    f32 dragForce;                          /* 0x84 */
    f32 driveForce;                         /* 0x88 */
    f32 brakeForce;                         /* 0x8C */
    f32 brake;                              /* 0x90 0..1 (car_set_wheel_brakes, T302, sets the 4) */
    f32 brakeHeat;                          /* 0x94 0..1, up with brakeTorque x tyreRadius x brake x |spin| (T295) */
    f32 shake;                              /* 0x98 -1..1 (T345) */
    f32 shakeVel;                           /* 0x9C (T345) */
    f32 wear;                               /* 0xA0 (? the tyre wear) */
};
EGCS_SIZE(DynWheel, 0xA4);

/* DynWheel.flags (u080, u081). */
enum {
    WHEEL_AIRBORNE = 0x01,                  /* no load (car_brake_torques, T296) */
    WHEEL_BRAKED   = 0x02,                  /* a brake torque on the wheel (car_brake_torques) */
    WHEEL_COUPLED  = 0x04,                  /* driven through the AWD coupling / centre (T344) */
    WHEEL_DRIVEN   = 0x08                   /* driven (T344) */
};

void pad_vibration_update(RaceCarState *car);   /* 0x001DF2B0 */

#endif /* GAME_T335_PAD_VIBRATION_H */
