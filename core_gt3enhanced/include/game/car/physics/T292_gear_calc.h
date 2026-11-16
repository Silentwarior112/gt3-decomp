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
    /* The enhanced engine (physics.md 24; GT3 never writes it): 1 for a
     * DRIVE_AWD_SPEED car whose front and rear tyre radii differ; then every
     * conversion between an axle's speed and its wheels' tread speed uses the
     * driven axle's radius (gear_calc, car_wheel_spin, car_drive_torque,
     * car_axle_differentials, DynCar_setInitialSpeed; Lupo 2 Transmission
     * +0x01, L2 0x001E3574). */
    u8  sharedRadius;                       /* 0x01 */
    u8  gearCount;                          /* 0x02 the top gear */
    u8  drivenAxle;                         /* 0x03 0 front, 1 rear (u080: the axle the other copies when the centre locks) */
    f32 awdSplit;                           /* 0x04 (u080: the centre coupling's factor; kind 7: the front share) */
    f32 awdTorque;                          /* 0x08 (u080: the centre lock's torque limit) */
    f32 inertia;                            /* 0x0C the engine's, CarSpec.flywheel x 0.001 */
    u8  diffType[2];                        /* 0x10 per axle: DIFF_* (below) */
    /* The enhanced engine: the gearbox type (GEARBOX_*, below), padding no
     * GT3 code touches (a whole-image scan, docs/gearbox_dualnote.md
     * section 4).  Writer: the physics area's gear_calc / Transmission set-up
     * from CarSpec.gearboxType.  Readers: physics (the 33 type tests), sound
     * (the type-3 whine and engine silence, cameras_sound.md S6), hud (the
     * type-3 tachometer shows the motor rpm: physics feeds it).  Lupo 2
     * Transmission+0x12 (car+0x208+0x12). */
    u8  gearboxType;                        /* 0x12 */
    u8  unk_13;                             /* 0x13 */
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
    DRIVE_AWD_FIXED  = 7,                   /* awdType 7: a fixed share awdSplit to the front */
    /* The enhanced engine: Lupo 2's 4WD sub-type 6 (physics.md 11; its
     * drivetrain type 9, L2 0x001E3EF0): DRIVE_AWD_FIXED with the front as
     * the driven axle (drivenAxle 0), awdSplit x the drive to the rear.
     * Every switch on driveType sends it where DRIVE_AWD_FIXED goes, with the
     * axles' roles swapped where the code names them.  No GT3 DRIVETRAIN row
     * has sub-type 6.  (Lupo 2 renumbered GT3's 7 to 8 and gave 7 to the
     * Dualnote; here GT3's numbers stay and the Dualnote is RaceCarState.
     * hybrid with DRIVE_REAR.) */
    DRIVE_AWD_FIXED_FRONT = 8
};

/* The enhanced engine: the Honda Dualnote is DRIVETRAIN layout 2 (4WD) with
 * sub-type 5 (docs/gearbox_dualnote.md section 2; Lupo 2's own test).
 * drivetrain_layout gives it DRIVE_REAR (Lupo 2's type 7 routes the drive
 * to the rear only); the hybrid system rides on top (T344).  No GT3 row has
 * sub-type 5.  (T293) */
int  drivetrain_is_hybrid(const CarSpec *spec);

/* GearCalc.gearboxType / CarSpec.gearboxType / TransmissionRecord.
 * gearboxType: Lupo 2's gearbox types (docs/gearbox_dualnote.md section 1,
 * physics.md 23).  0 for every GT3 car. */
enum {
    GEARBOX_STOCK      = 0,                 /* GT3's gearbox */
    GEARBOX_CVT        = 1,                 /* R / D only, the ratio slides between 1st and top */
    GEARBOX_CLUTCHLESS = 2,                 /* stepped, no clutch phase: the ratio glides between steps */
    GEARBOX_ELECTRIC   = 3                  /* one ratio, the built-in motor law */
};

/* The enhanced engine: Lupo 2's four gearbox predicates (L2 0x001DF8D8 type
 * 1, 0x001DF8E8 type 2, 0x001DF8F8 type 1 or 2, 0x001DFC90 type 3), on the
 * GearCalc.  All 0 for every GT3 car. */
static inline int gearbox_is_cvt(const GearCalc *g) { return g->gearboxType == GEARBOX_CVT; }
static inline int gearbox_is_clutchless(const GearCalc *g) { return g->gearboxType == GEARBOX_CLUTCHLESS; }
static inline int gearbox_is_ratio_glide(const GearCalc *g)
{
    return (u8)(g->gearboxType - 1) < 2;    /* CVT or clutchless: the ratio factor (RaceCarState.cvtRatio) */
}
static inline int gearbox_is_electric(const GearCalc *g) { return g->gearboxType == GEARBOX_ELECTRIC; }
/* The ratio slot of a gear: a clutchless box keeps gears 2 and up as steps
 * relative to 1st (gear_calc), reached through slot 1 and the ratio factor
 * (Lupo 2 L2 0x001E4604, 0x001FBC34, 0x001E52D0, 0x001FC020). */
static inline int gearbox_ratio_slot(const GearCalc *g, int gear)
{
    if (gearbox_is_clutchless(g) && gear >= 2)
        return 1;
    return gear;
}

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
