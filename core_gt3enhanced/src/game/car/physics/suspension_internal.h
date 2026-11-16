/* suspension_internal.h - unit u081 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001E6C28..0x001E9E80, second pass.
 *
 * Translation units (link order):
 *   T296  the drivetrain step of a car (car_drivetrain_step): brakes, clutch
 *         state, slip ratios, tyre force limits and forces, the longitudinal
 *         acceleration, the engine speed and rpm, the gear, the yaw and the
 *         lateral acceleration, the velocity; tyre wear and the tyre check;
 *         the rolling resistance; the peak of a tyre's lateral force curve
 *         (include/game/car/physics/T296_car_drivetrain.h)
 *   T346  the DynamicsConductor's car boxes: the cars' position / velocity /
 *         yaw to the collision boxes and back (T346_conductor_car_boxes.h)
 *   T297  the steering angle of each wheel (T297_wheel_steer.h)
 *   T347  the surface type of a ground contact (T347_ground_surface.h)
 *   T298  the ground contact (4 contacts, the surface bumps), the suspension
 *         (spring, bump stop, damper, anti-roll bar), camber and track
 *         change, the pit stop jack, the body's pitch / roll / heave motion,
 *         the ground penetration correction (T298_suspension_body.h)
 *   T299  the wheel positions relative to the body (T299_wheel_offsets.h)
 *   T348  the wheel speed factors (T348_wheel_speed_factor.h)
 *
 * Every class is defined in its canonical header (include/game/).  The car
 * is RaceCarState (T302; u051 / u079's name RaceCarDynamics is its typedef):
 * +0x10 its CarDynamics (CarParams), +0xEC0 the frame time, +0xF84 its
 * DynCarState (CarDynState), +0x14BC the four RunwayContacts (GroundQuery).
 * The car world is z-up.  This header adds the outside headers the sources
 * need and what only this unit uses: the prototypes of the functions that
 * no canonical header declares (they are called only inside u081) and T347's
 * surface table g_002D53D8.
 *
 * Data (the data pass): no header here emits a type name or literal (the
 * race display header, T154_race_display.h, is not included: retail's u081
 * objects have none of its names; the RaceCarDynamics typedef comes from
 * the units' own headers).  T296, T346, T297, T347, T298 and T299 include
 * <new> and T153_race_course.h (emitting UnitArena<64>'s name) themselves,
 * first, as their originals did; T348's original has no .rodata.
 */
#ifndef U081_H
#define U081_H

#include "types.h"                          /* boot_sqrtf (sqrt.s) */
#include "game/enhanced.h"                  /* ENH_PHYSICS_*, ENH_AI_SKILL_AS_GRIP (the enhanced engine) */

/* own translation units */
#include "game/car/physics/T296_car_drivetrain.h"       /* TyreData, CarDynState */
#include "game/car/physics/T346_conductor_car_boxes.h"
#include "game/car/physics/T297_wheel_steer.h"
#include "game/car/physics/T347_ground_surface.h"
#include "game/car/physics/T298_suspension_body.h"      /* CarParams */
#include "game/car/physics/T299_wheel_offsets.h"
#include "game/car/physics/T348_wheel_speed_factor.h"

/* outside the unit */
#include "engine/media/G002_mpeg_frameq.h"          /* atan2f, cosf, sinf */
#include "game/race/core/T153_race_course.h"          /* RaceCourse::getContacts, RunwayContact */
#include "game/race/display/T279_meter_scale.h"          /* CarDynamics */
#include "game/car/physics/T284_car_param.h"            /* SuspParam, CarDynamics_getTyre */
#include "game/sound/T288_car_sound.h"            /* DynCarState, TyreForce */
#include "game/car/physics/T292_gear_calc.h"            /* GearCalc, EngineCurve (CarEngine) */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState */
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* DynamicsConductor (getCar, pitNodeAngle, wear, boxes, mode) */
#include "game/common/T319_angle_math.h"           /* eulerToAxes, angleDiff */
#include "game/common/T320_record_set.h"           /* Curve */
#include "game/common/T322_curve_linear.h"         /* curveLookupLinear */
#include "game/common/T323_curve_pair.h"           /* curveLookupPair */
#include "game/race/dynamics/T324_dynamics_conductor_rules.h" /* F_001D9FE0 */
#include "game/car/physics/T325_misc.h"                 /* g_002D28B0 (DynParams) */
#include "game/car/physics/T327_wear_curve.h"           /* WearCurve (DynamicsConductor::wear) */
#include "game/car/physics/T328_surface_tables.h"       /* g_002D2CE0 (SurfaceTable) */
#include "game/car/physics/T330_dyn_car_init.h"         /* GroundQuery (a typedef of RunwayContact) */
#include "game/car/physics/T338_engine_output.h"        /* engine_update */
#include "game/car/physics/T342_car_input.h"            /* CarCtrlOut */
#include "game/car/physics/T343_gear_select.h"          /* car_set_gear, car_select_gear (GearInput) */
#include "game/car/physics/T344_drivetrain.h"           /* car_wheel_torques .. car_axle_differentials */
#include "game/car/physics/T356_brake_assist.h"         /* CarWheelState (a typedef of DynWheel) */
#include "game/car/physics/T376_car_box_walls.h"        /* CarBox */

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

#define U081_PI         3.1415927f
#define U081_HALF_PI    1.5707964f
#define U081_QUARTER_PI 0.7853982f
#define U081_TWO_PI     6.2831855f
#define U081_DEG        0.017453292f        /* 1 degree in rad */
#define U081_KMH        0.2777778f          /* 1 km/h in m/s */
#define U081_RPM        0.10471976f         /* 1 rpm in rad/s (2 pi / 60) */
#define U081_G          9.8f                /* m/s^2 */

/* GearCalc.driveType: DRIVE_* (T292); DynCarState.clutchState / flags:
 * CLUTCH_* / DYN_FLAG_* (T288); DynWheel.flags: WHEEL_* (T335). */

/* RunwayContact.attr2 */
enum {
    ATTR2_COUNTED  = 0x01,                  /* counted by the conductor (chargePenaltyBySurface) */
    ATTR2_REMAP    = 0x02                   /* the surface code is remapped (ground_surface_type) */
};

/* ======================================================================== */
/* T346: the car state block of a CarBox                                    */
/* ======================================================================== */

/* CarBox +0x454..+0x477 (statePos, stateVel, stateYaw, stateYawRate, sector:
 * T374's in / out record) as one record.  Retail reaches it through its
 * address (box + 0x454 in a register, the later fields at offsets from it):
 * the original's state member through an inline accessor.  A view of the
 * canonical CarBox's fields, same layout (u057's CarBoxState is the same
 * view without the sector). */
struct CarBoxCarState {
    f32 pos[3];                             /* 0x00 statePos */
    f32 vel[3];                             /* 0x0C stateVel */
    f32 yaw;                                /* 0x18 stateYaw */
    f32 yawRate;                            /* 0x1C stateYawRate */
    int sector;                             /* 0x20 sector */
};
static inline CarBoxCarState *car_box_car_state(CarBox *box)
{
    return (CarBoxCarState *)box->statePos;
}

/* ======================================================================== */
/* T347's .data                                                             */
/* ======================================================================== */

/* 0x002D53D8 by surface code (RunwayContact.attr): 1 for codes 2, 3, 4 and 6,
 * the surfaces off the course (car_ground_contact counts the wheels on them
 * for chargePenaltyBySurface's course-out rule), else 0.  16 words, then
 * T347's .eh_frame.  Defined in T347. */
extern int g_002D53D8[16];

/* ======================================================================== */
/* Functions of this unit that no canonical header declares                 */
/* ======================================================================== */
/* T296 */
f32  car_engine_speed_from_wheels(RaceCarDynamics *car);                /* 0x001E6CA8 */
void car_update_rpm(RaceCarDynamics *car);                              /* 0x001E6D00 */
void car_clutch_state(RaceCarDynamics *car);                            /* 0x001E6D48 */
void car_brake_torques(RaceCarDynamics *car);                           /* 0x001E6EC8 */
f32  slip_grip_factor(int kind, f32 speed);                             /* 0x001E6FE0 */
/* The enhanced engine: + the wheel's tyre (its compound flags; Lupo 2 L2
 * 0x001E4BA8 takes it the same way). */
void tyre_slip_ratio(TyreForce *tf, CarWheelState *w, f32 groundSpeed, TyreData *tyre); /* 0x001E7068 */
void car_tyre_force_limits(RaceCarDynamics *car);                       /* 0x001E73C0 */
void car_tyre_long_forces(RaceCarDynamics *car);                        /* 0x001E7450 */
void car_long_accel(RaceCarDynamics *car);                              /* 0x001E7848 */
f32  slip_angle_fold(f32 *sign, f32 angle);                             /* 0x001E7A68 */
f32  wheel_low_speed_side_force(CarWheelState *w);                      /* 0x001E7AD8 */
void car_tyre_lat_forces(RaceCarDynamics *car);                         /* 0x001E7C08 */
void car_yaw_lateral(RaceCarDynamics *car, int airborne);               /* 0x001E7D90 */
void car_integrate_velocity(RaceCarDynamics *car, int airborne);        /* 0x001E8008 */
void car_rolling_resistance(RaceCarDynamics *car);                      /* 0x001E84D8 */
/* T347 */
int  ground_is_off_course(GroundQuery *q);                              /* 0x001E8AC0 */
/* T298 */
f32  surface_bump_wave(int kind, f32 x, f32 z, f32 waveNumber);         /* 0x001E8AE0 */
f32  susp_damper_force(SuspParam *s, f32 speed);                        /* 0x001E8DD0 */
/* The enhanced engine: the car and the spring rate passed (Lupo 2 L2
 * 0x001E7420: the Pod's squat, the SLs' active suspension). */
f32  susp_wheel_force(RaceCarDynamics *car, CarWheelState *w, SuspParam *s, int wheel, f32 gap,
                      f32 springRate);                                  /* 0x001E8E58 */
f32  sl_active_suspension_force(RaceCarDynamics *car, int wheel, f32 stroke);  /* L2 0x001E2280 */
f32  pod_squat_spring_rate(RaceCarDynamics *car, int wheel, f32 rate);          /* L2 0x001E1D00 */
f32  susp_antiroll_force(CarWheelState *a, CarWheelState *b, SuspParam *s); /* 0x001E8FD0 */
void car_wheel_gaps(CarDynState *st, CarParams *p, f32 groundCos);      /* 0x001E9010 */
int  car_wheel_ground_bits(CarDynState *st, CarParams *p, int bits);    /* 0x001E90A0 */
f32  pit_jack_lift(int timer, int *phase);                              /* 0x001E90F8 */
int  car_hold_lift(CarDynState *st, f32 lift);                          /* 0x001E9160 */
int  car_pit_jack(RaceCarDynamics *car);                                /* 0x001E9528 */
void car_drive_pitch_heave(RaceCarDynamics *car, f32 *pitch, f32 *heave); /* 0x001E9640 */
/* T299 */
void car_wheel_heights(RaceCarDynamics *car);                           /* 0x001E9CE8 */

#endif /* U081_H */
