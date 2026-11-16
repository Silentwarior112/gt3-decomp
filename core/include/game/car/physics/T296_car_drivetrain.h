/* T296_car_drivetrain.h - TyreData and the drivetrain step of a car
 * (translation unit T296, unit u081), 0x001E6C28..0x001E8730.
 *
 * car_drivetrain_step (from F_001F39D8, T317) runs one step of the
 * drivetrain: the brake torques, the rolling resistance, the tyre force
 * limits and the longitudinal / lateral tyre forces, the longitudinal
 * acceleration, the wheel / drive / axle torques of T344, the engine speed
 * and the rpm, the gear (car_gear_select: car_select_gear, T343), the yaw
 * and the lateral acceleration, the velocity.  Also the tyre wear and the
 * tyre check of a frame, and tyre_peak_slip_angle, the slip angle of a
 * tyre's lateral force peak (CarDynamics_init, T284, keeps it in
 * TyreData.peakSlipAngle).
 *
 * TyreData: one tyre parameter set, CarDynamics +0x398 + i x 0x1B4
 * (compounds 0..2 front, 3..5 rear; CarDynamics_getTyre picks one by the
 * axle and the surface).  u077 calls it TireParam, u080 / u086 TyreGrip
 * (T295: a typedef of this).  Layout and names: u077's TyreData_init*
 * functions (T284) built it; u081, u082, u086 read it.
 *
 * CarDynState is u079 / u081's name for DynCarState (T288), the dynamic
 * state at RaceCarState +0xF84 (u081's names are merged there).  It is a
 * typedef here, with no second definition: other headers must not
 * forward-declare `class CarDynState'. */
#ifndef GAME_T296_CAR_DRIVETRAIN_H
#define GAME_T296_CAR_DRIVETRAIN_H

#include "game/game.h"
#include "game/car/physics/T284_car_param.h"            /* Curve (held by value) */
#include "game/sound/T288_car_sound.h"            /* DynCarState (CarDynState) */

class RaceCarState;                         /* T302_car_state.h (pointers only) */
typedef RaceCarState RaceCarDynamics;       /* as T154_race_display.h: u051 / u079 / u081 / u087 name it so */

typedef DynCarState CarDynState;            /* u079, u081 */
EGCS_SIZE(CarDynState, 0x538);

/* Not polymorphic.  Size 0x1B4 (the stride of CarDynamics.tyre; six of
 * them fill +0x398 .. +0xDD0). */
class TyreData {
public:
    f32   grip;                             /* 0x000 slipCurve2(0) x 9.8 x loadCurve(the static load): the grip at rest (u079 load) */
    f32   wearRate;                         /* 0x004 the tyre record +0x6B x 0.01: the compound's wear rate (car_tyre_wear: */
                                            /* the wear and the squeal level are the wheel's capped slip x it) */
    f32   surfaceGrip[16];                  /* 0x008 by surface type; [11] (0x034) is the floor of the wear grip (u082 minWearGrip) */
    Curve latCurve;                         /* 0x048 the lateral force over the slip angle */
    f32   latX[8];                          /* 0x054 rad */
    f32   latY[8];                          /* 0x074 */
    f32   peakSlipAngle;                    /* 0x094 tyre_peak_slip_angle, at most pi / 4 (u086: ? the steering limit) */
    Curve dragCurve;                        /* 0x098 the force along the wheel over the slip angle */
    f32   dragX[6];                         /* 0x0A4 rad */
    f32   dragY[6];                         /* 0x0BC */
    Curve slipCurve;                        /* 0x0D4 the longitudinal force over the slip ratio (curveLookupPair) */
    f32   slipX[12];                        /* 0x0E0 the slip ratios: the negative side stored backwards, then the positive */
    f32   slipY[12];                        /* 0x110 */
    f32   peakSlipNeg;                      /* 0x140 the slip of slipCurve's negative-side peak (u079 / u080 grip: a slip limit) */
    f32   peakSlipPos;                      /* 0x144 the slip of the positive-side peak (u079 unk144) */
    f32   peakPos;                          /* 0x148 the positive-side peak (u086 unk148) */
    Curve slipCurve2;                       /* 0x14C over slipX: the force limit, the friction circle (curveLookupPair) */
    f32   slipY2[12];                       /* 0x158 */
    Curve loadCurve;                        /* 0x188 the grip over the load per tyre width (tyre_load_per_width, T300) */
    f32   loadX[4];                         /* 0x194 x 20 x 9.8 */
    f32   loadY[4];                         /* 0x1A4 */
};
EGCS_SIZE(TyreData, 0x1B4);

/* The angular speed of the driven axle(s) by GearCalc.driveType: the rear,
 * the front, the mean (3) or a blend by awdSplit (7). */
f32  car_driven_axle_speed(RaceCarDynamics *car);   /* 0x001E6C28 */
void car_tyre_slip_ratios(RaceCarDynamics *car);    /* 0x001E7328 tyre_slip_ratio of the 4 wheels */
/* The engine speed through the clutch (engine_update, T338), at most
 * engine.redline + 10 rpm, and the rpm. */
void car_engine_speed(RaceCarDynamics *car);        /* 0x001E7500 */
/* Applies car_select_gear's gear (-1: none); a slipping or locked clutch
 * becomes state 3 for a forward -> forward change, else 2. */
void car_gear_select(RaceCarDynamics *car);         /* 0x001E77E0 */
/* Not T300's car_tyre_wear (0x001E9E80, DynCarState *, CarDynamics *, WearCurve *). */
void car_tyre_wear(RaceCarDynamics *car);           /* 0x001E8190 */
/* If the conductor allows it (slot 38): bit 0 a worn tyre (below the
 * conductor's worn limit), bit 1 F_001D9FE0 (T324) == 4 and a wheel's needsService;
 * either sets DynCarState.flags bit 2 (and bit 3 by the driver). */
void car_tyre_check(RaceCarDynamics *car);          /* 0x001E8340 */
void car_drivetrain_step(RaceCarDynamics *car);     /* 0x001E8598 */
/* The slip angle (0..45 degrees, 1 degree steps) of latCurve's maximum; 90
 * degrees when the curve is never above 0. */
f32  tyre_peak_slip_angle(TyreData *t);             /* 0x001E8670 */

#endif /* GAME_T296_CAR_DRIVETRAIN_H */
