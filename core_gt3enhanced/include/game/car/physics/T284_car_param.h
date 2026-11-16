/* T284_car_param.h - the car's physics parameter block CarDynamics built from
 * its spec record (translation unit T284, unit u077), 0x001DB220..0x001DD0C8;
 * SuspParam.
 *
 * CarDynamics_init builds one CarDynamics (T279_meter_scale.h; u077 calls it
 * CarParam, u079 / u081 / u082 CarParams) from the packed CarSpec record
 * (T482): dimensions and weight distribution, the engine (engine_setup, T291)
 * and the gearbox (gear_calc, T292), the six tyre parameter sets with their
 * slip / angle / load curves, the camber curves, aerodynamics, brakes, the
 * driving aids, the steering curves, the suspension (springs, bump / rebound
 * dampers fitted through two points: the 'Front/Bound' and 'Front/Rebound'
 * labels) and finally the top speed.  The `log' argument passed down
 * everywhere is not used by the code (? a debug printer in the original).
 *
 * Curve (T320_record_set.h) is the piecewise-linear table: curveSet sets it
 * up, curveLookup / curveLookupLinear (T321 / T322) evaluate it; every curve
 * of the dynamics is one. */
#ifndef GAME_T284_CAR_PARAM_H
#define GAME_T284_CAR_PARAM_H

#include "game/game.h"
#include "game/common/T320_record_set.h"          /* Curve (held by value by this header's users) */

class CarDynamics;                          /* T279_meter_scale.h (pointers only) */
class TyreData;                             /* T296_car_drivetrain.h: one tyre parameter set (u077 TireParam) */
class RaceRuleOptions;                      /* T326_race_rule_options.h */
class CarCondition;                         /* T280_car_condition.h */
struct CarSpec;                             /* T482_car_equipments.h */
struct CarSpecSuspension;
struct CarSpecTyre;

/* One axle's suspension, CarDynamics +0x2B8 + axle * 0x44 (u076
 * AxleGeometry).  Names: u081 (T298 reads it); the units are the spec
 * record's (CarSpecSuspension) scaled by SuspParam_init / _initStroke.
 * Not polymorphic.  Size 0x44 (the array stride). */
class SuspParam {
public:
    f32 bumpStart;                          /* 0x00 the bump stop works below this stroke */
    f32 strokeMin;                          /* 0x04 */
    f32 strokeMax;                          /* 0x08 beyond it the wheel hangs free */
    f32 gapOffset;                          /* 0x0C tyre radius - axle height + staticStroke (u081: the gap offset) */
    f32 staticStroke;                       /* 0x10 -sprungWeight / 2 / spring (u081: the origin of the camber /
                                               track curves) */
    f32 spring;                             /* 0x14 springRate x 980 x the axle's % */
    f32 bumpOfs;                            /* 0x18 the bump damper, sqrt(-v - bumpOfs) (u077 boundA: damper_fit) */
    f32 bumpBase;                           /* 0x1C (u077 boundB) */
    f32 bumpGain;                           /* 0x20 (u077 boundC) */
    f32 reboundOfs;                         /* 0x24 the rebound damper (u077 reboundA) */
    f32 reboundBase;                        /* 0x28 (u077 reboundB) */
    f32 reboundGain;                        /* 0x2C (u077 reboundC) */
    f32 antiRoll;                           /* 0x30 stabilizer x 10 x 9.8 */
    f32 bumpStop;                           /* 0x34 spec +0x02 x 10000 x 9.8 */
    f32 unsprungMass;                       /* 0x38 spec +0x03 x 0.25 (the mass less 2 x both is the sprung mass) */
    f32 unsprungWeight;                     /* 0x3C unsprungMass x 9.8 (u081 preload: in each wheel's static load) */
    f32 bumpStopDamp;                       /* 0x40 spec +0x0D x 10 x 9.8 */
};
EGCS_SIZE(SuspParam, 0x44);

/* x[i] = kmh[i] km/h in m/s, y[i] = deg5[i] fifths of a degree in radians;
 * then F_001D8DD0(c, n, x, y). */
void curve_setup_kmh_deg(Curve *c, f32 *x, f32 *y, u8 n, u8 *kmh, u8 *deg5);   /* 0x001DB220 */
/* Steps the speed from 0 to maxSpeed (topSpeed / 500 + 1 km/h steps),
 * shifting up at the shift rpm, and keeps the last speed where the drive
 * force exceeds the resistance (rolling: g_002D2CE0 x rollingResistance[0];
 * drag; lift).  cond may be NULL (engine_torque_at). */
void CarDynamics_calcTopSpeed(CarDynamics *p, CarCondition *cond);           /* 0x001DB2C8 (u077 CarParam_calcTopSpeed) */
/* maxSpeed = the speed at the rev limit in top gear, unkE34, then
 * CarDynamics_calcTopSpeed. */
void CarDynamics_initTopSpeed(CarDynamics *p, CarCondition *cond, void *log); /* 0x001DB498 */
f32  inv_percent(u8 pct);                                                    /* 0x001DB510 100 / pct */
/* One damper record: slow and fast coefficient (% x scale), slow and fast
 * force (x 5 x 9.8). */
void damper_unpack(f32 *slowK, f32 *slowF, f32 *fastK, f32 *fastF,
                   u8 a, u8 b, u8 c, u8 d, f32 scale);                       /* 0x001DB538 */
/* Fits the damper curve (*a, *b, *c) through (x1, y1) and (x2, y2); all
 * three stay 0 if the points give no valid curve.  label: "Front/Bound",
 * "Front/Rebound" (not used). */
void damper_fit(f32 *a, f32 *b, f32 *c, void *log, const char *label,
                f32 x1, f32 y1, f32 x2, f32 y2);                             /* 0x001DB5C0 */
void SuspParam_init(SuspParam *s, CarSpecSuspension *ss, u8 pct, void *log); /* 0x001DB6B8 */
void SuspParam_initStroke(CarDynamics *p, SuspParam *s, CarSpec *spec,
                          int axle, void *log);                              /* 0x001DB890 */
/* The tyre parameter set of a wheel (0..3, axle = wheel >> 1) on surface
 * slot `slot': &p->tyre[p->tyreIndex[wheel >> 1][slot]]. */
TyreData *CarDynamics_getTyre(CarDynamics *p, int wheel, int slot);          /* 0x001DB9B8 (u077 CarParam_getTire) */
/* The slip curves (TyreData +0xD4 / +0x14C over +0xE0): the negative side
 * stored backwards, then the positive side; the centre point is shared. */
void TyreData_initSlipCurves(TyreData *t, CarSpecTyre *ts);                  /* 0x001DB9E0 (u077 TireParam_*) */
void TyreData_initAngleCurves(TyreData *t, CarSpecTyre *ts);                 /* 0x001DBC50 curves +0x48 and +0x98 */
/* The grip per surface (+0x08..), the load curve (+0x188); `log' (a0) is
 * not used. */
void TyreData_initLoadCurve(void *log, TyreData *t, CarSpecTyre *ts, f32 scale); /* 0x001DBE40 */
/* Builds the parameter block from the spec record.  driver is the car's
 * DynCarState.driver: 2 (a computer car) scales axleGrip[] by the rules'
 * enemyMuVolumeF / R [driveClass]; rules->powerLimit x 10 goes to
 * engine_setup.  rules: DynamicsConductor +0xE340, or g_002D2988 outside a
 * race.  The air drag area is multiplied by g_00352920 / 2 (T329's .sdata:
 * 0.8 in the file, 1.2 once slipstream_init has run; ? the air density).  cond
 * (may be NULL) to the top speed search.  Returns 0.  (u076
 * declares it as the method CarDynamics::F_001DBFD8.) */
int  CarDynamics_init(CarDynamics *p, CarSpec *spec, int driver, void *log,
                      RaceRuleOptions *rules, CarCondition *cond);           /* 0x001DBFD8 (u077 CarParam_init) */

#endif /* GAME_T284_CAR_PARAM_H */
