/* T292_gear_calc.h - GearCalc, the drivetrain values of a car, and the gear
 * chart (translation unit T292, unit u079), 0x001E1B28..0x001E2670.
 *
 * gear_calc derives the drivetrain block from a CarSpec (the parameter block
 * CarEquipments::getSpec builds, T482): the layout (drivetrain_layout, T293),
 * the differentials (the letters "frnmhvyt" or an index 0..7 per axle), the
 * gear x final ratios, the tyre dimensions (tyre_dimensions, T293), the
 * inertias and the shift points.  CarDynamics_init (T284) builds it into
 * CarDynamics +0x1F8.  gear_chart_of_spec / gear_chart_with_sheet fill the gear chart of the
 * setting screen (GearChartData, T235; GearChart, T237) from a spec, the
 * latter with the engine's power.
 *
 * EngineCurve is u079 / u081's name for CarEngine (T291): the same 0xC0
 * bytes at CarDynamics +0x138 (u079 rpmMax / rpmMin = CarEngine.redline /
 * idle).  It is a typedef here, with no second definition: other headers
 * must not forward-declare `class EngineCurve'. */
#ifndef GAME_T292_GEAR_CALC_H
#define GAME_T292_GEAR_CALC_H

#include "game/game.h"
#include "game/car/physics/T291_engine.h"               /* CarEngine (EngineCurve) */

struct CarSpec;                             /* T482_car_equipments.h (pointers only) */
struct GearChartData;                       /* T235_color_box.h */
class CarCondition;                         /* T280_car_condition.h */

typedef CarEngine EngineCurve;              /* u079, u081 */
EGCS_SIZE(EngineCurve, 0xC0);

/* The drivetrain block (CarDynamics +0x1F8; a stack object of gear_chart_of_spec).
 * u080 calls it Drivetrain, u077 GearboxParam.  Gear 0 is the reverse,
 * 1..gearCount the forward gears; axle 0 is the front.  Not polymorphic.
 * Size 0xC0 (SuspParam follows at CarDynamics +0x2B8). */
class GearCalc {
public:
    u8  driveType;                          /* 0x00 DRIVE_* (below; drivetrain_layout, T293) */
    u8  unk_01;                             /* 0x01 */
    u8  gearCount;                          /* 0x02 the top gear */
    u8  drivenAxle;                         /* 0x03 0 front, 1 rear (u080: the axle the other copies when the centre locks) */
    f32 awdSplit;                           /* 0x04 (u080: the centre coupling's factor; kind 7: the front share) */
    f32 awdTorque;                          /* 0x08 (u080: the centre lock's torque limit) */
    f32 inertia;                            /* 0x0C the engine's, CarSpec.flywheel x 0.001 */
    u8  diffType[2];                        /* 0x10 per axle: DIFF_* (below) */
    u8  unk_12[2];                          /* 0x12 */
    f32 diffA[2];                           /* 0x14 per axle (u080: the LSD's initial torque) */
    f32 diffB[2];                           /* 0x1C (u080: the LSD's accel factor) */
    f32 diffC[2];                           /* 0x24 (u080: the LSD's decel factor) */
    u16 idleRpm;                            /* 0x2C CarSpec.idleRpm x 10, at least the engine's idle + 500 */
    u16 revLimit;                           /* 0x2E CarSpec.revLimit x 100 (u076 / u080: redZone) */
    u16 shiftRpm[6];                        /* 0x30 [i - 1]: the rpm after the shift i -> i + 1 */
    f32 ratio[8];                           /* 0x3C gear x final ([0] negative: the reverse) */
    f32 ratioPerRadius[8];                  /* 0x5C ratio / the driven tyre's radius: engine rad/s per m/s */
    f32 tyreRadius[2];                      /* 0x7C m, per axle */
    f32 tyreRadiusInv[2];                   /* 0x84 */
    f32 tyreWidth[2];                       /* 0x8C m (u081: a lever of the drive force) */
    f32 tyreSidewall[2];                    /* 0x94 m */
    f32 clutchTorque;                       /* 0x9C CarSpec.unkCD x 9.8 (u080: ? the clutch torque of clutch state 3) */
    f32 invInertia[2];                      /* 0xA0 per axle: tyreRadius / the axle's inertia (CarSpec.unkD2 x 0.01) */
    f32 axleInertia[2];                     /* 0xA8 per axle: CarSpec.unkCE x 0.01 x final + 2 x the axle's inertia */
    f32 invAxleInertia;                     /* 0xB0 1 / (axleInertia[0] + axleInertia[1]) */
    f32 inertiaInv;                         /* 0xB4 1 / inertia */
    f32 axleMass[2];                        /* 0xB8 per axle: the axle's inertia / tyreRadius^2, an equivalent mass */
};
EGCS_SIZE(GearCalc, 0xC0);

/* GearCalc.driveType (drivetrain_layout, T293: the spec's layout and
 * awdType; u079 DRIVE_4WD_*, u081 DRIVE_MEAN / BLEND). */
enum {
    DRIVE_REAR       = 0,                   /* FR, MR, RR */
    DRIVE_FRONT      = 1,                   /* FF */
    DRIVE_AWD_FRONT  = 2,                   /* awdType 0: the front driven, the rear through a coupling
                                               (awdSplit x the axle speed difference) */
    DRIVE_AWD_CENTRE = 3,                   /* awdType 1: a centre differential (the mean of the axles) */
    DRIVE_AWD_SPLIT  = 4,                   /* awdType 2: the rear driven, awdSplit x the torque to the front
                                               (a coupling torque awdParam, default 8 x 10 x 9.8) */
    DRIVE_AWD_SPEED  = 5,                   /* awdType 3: the front share by the axle speed difference */
    DRIVE_AWD_STEER  = 6,                   /* awdType 4: the front share falls with the steering */
    DRIVE_AWD_FIXED  = 7                    /* awdType 7: a fixed share awdSplit to the front */
};

/* GearCalc.diffType[axle] (the spec's letter in "frnmhvyt"; u080). */
enum {
    DIFF_NONE    = 0,                       /* 'f' not driven: each wheel on its own torque */
    DIFF_LOCKED  = 1,                       /* 'r' both wheels at the axle's speed */
    DIFF_OPEN    = 2,                       /* 'n' */
    DIFF_LSD     = 3,                       /* 'm', 'h': a torque-sensing LSD (diffA the initial torque,
                                               diffB the accel factor, diffC the decel factor) */
    DIFF_LSD2    = 4,
    DIFF_VISCOUS = 5,                       /* 'v' diffB x min(dv^2, diffA) */
    DIFF_AYC     = 6,                       /* 'y', 't': open, with the active yaw control (? ; its strength
                                               diffA) */
    DIFF_AYC2    = 7
};

/* a3 goes to gear_auto_apply (not read there). */
void gear_calc(CarSpec *spec, GearCalc *g, EngineCurve *e, int a3);        /* 0x001E1C98 */
/* The gear chart of a spec: engine_setup and gear_calc on the stack, then
 * gear_chart_fill.  cond is not read: both callers (T237) pass $a2 = 0, and
 * the function builds a fresh CarCondition it does not use (? the original
 * selects it for a NULL cond, as gear_chart_with_sheet does; the select is optimised
 * away, the constructor call stays). */
void gear_chart_of_spec(CarSpec *spec, GearChartData *out, CarCondition *cond = 0); /* 0x001E2538 */
/* The same, and engine_spec_sheet (cond, a fresh condition when NULL) into
 * the whole EngineSpecSheet (T291, 0xB0 bytes; the first pass had `f32
 * *power', its first member maxPower); returns (int) maxPower.  No
 * callers. */
int  gear_chart_with_sheet(CarSpec *spec, GearChartData *out, EngineSpecSheet *sheet, CarCondition *cond); /* 0x001E25B0 */

#endif /* GAME_T292_GEAR_CALC_H */
