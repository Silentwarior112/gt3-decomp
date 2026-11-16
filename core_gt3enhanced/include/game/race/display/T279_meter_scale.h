/* T279_meter_scale.h - CarDynamics, the physics parameter block of a car;
 * ValuateAutomobilePerformance and the meter scales (translation unit T279,
 * unit u076), 0x001D98D0..0x001D9AF0.
 *
 * CarDynamics is built by CarDynamics_init (T284) from the car's CarSpec: it
 * is RaceCarState +0x10 (u076 / u080 / u086 CarDynamics, u077 CarParam, u079 /
 * u081 / u082 CarParams), ShowRoomCar +0x3E4 (T278) and a stack object of
 * ValuateAutomobilePerformance / SelectMeterScale.  Its functions are T284's
 * (CarDynamics_*); T297 / T298 / T300 evaluate it.
 *
 * Not this class: u018, u037, u038, u039, u043, u078 and u083 use the name
 * CarDynamics for what RaceEntryCar +0x20DC / RaceCarState +0xC point at and
 * RaceBase::virtual_26 returns, which is RaceInput (T413_race_input.h): their
 * +0xC4 / +0xCC / +0x12C / +0x140 are RaceInput's rec, rec's recorder,
 * snapshots and hold.
 *
 * Names: u076 (the owner), else u081 (the fullest reader), u077 (the
 * builder), u079, u080, u082, u086; the formulas are CarDynamics_init's. */
#ifndef GAME_T279_METER_SCALE_H
#define GAME_T279_METER_SCALE_H

#include "game/game.h"
#include "game/car/physics/T284_car_param.h"            /* Curve, SuspParam (held by value) */
#include "game/car/physics/T291_engine.h"               /* CarEngine (held by value) */
#include "game/car/physics/T292_gear_calc.h"            /* GearCalc (held by value) */
#include "game/car/physics/T296_car_drivetrain.h"       /* TyreData (held by value) */
#include "game/car/physics/T356_brake_assist.h"         /* TcsParams (held by value) */

class CarEquipments;                        /* T482_car_equipments.h (pointers only) */
class RaceCarState;                         /* T302_car_state.h */

/* Not polymorphic.  Size 0xEB0 (ShowRoomCar: +0x3E4, then the
 * EngineSpecSheet at +0x1294; ValuateAutomobilePerformance's stack: +0x3F0,
 * then +0x12A0; RaceCarState +0x10, then dt at +0xEC0). */
class CarDynamics {
public:
    f32       lenFront;                     /* 0x000 the body ahead of the centre (spec +0x1C mm - originOffset; */
                                            /* a grid start moves the car back by it) */
    f32       lenRear;                      /* 0x004 behind it */
    f32       width;                        /* 0x008 spec +0x20 mm */
    f32       height;                       /* 0x00C spec +0x22 mm */
    f32       frontDist;                    /* 0x010 the centre to the front axle (u082 axleFront) */
    f32       rearDist;                     /* 0x014 the centre to the rear axle (u082 axleRear) */
    f32       wheelbase;                    /* 0x018 spec wheelBase, m (axle 0 at -1/2, axle 1 at +1/2) */
    f32       track;                        /* 0x01C 2 x (halfTrack[0] x frontRatio + halfTrack[1] x rearRatio) */
    f32       rearRatio;                    /* 0x020 1 - frontRatio (u081 axleBlend, u082 frontShare) */
    f32       frontRatio;                   /* 0x024 the front weight share (spec +0x2C %) */
    f32       halfTrack[2];                 /* 0x028 front, rear */
    u8        unk_030;                      /* 0x030 */
    u8        driveClass;                   /* 0x031 0..2 = F_001F5EF0(spec layout, awdType): the index of the */
                                            /* rules' enemy arrays (u082 trans31: to F_001F6380) */
    /* The enhanced engine: the special kind (CAR_KIND_*, below), a byte no
     * GT3 code reads or writes.  Writer: the physics area's CarDynamics_init
     * from CarSpec.specialKind (Lupo 2 L2 0x001DC7A4..0x001DC7AC).  Readers:
     * physics (kind 7's active suspension, the moving part, the Pod's
     * emotions and squat), rendering (the wing, the Pod model), sound (the
     * Pod voice), race (the Pod rules).  Lupo 2 params+0x32 (isPod L2
     * 0x001E1288: params+0x32 == 1). */
    u8        specialKind;                  /* 0x032 */
    u8        unk_033;                      /* 0x033 spec +0x2D (u076: isRacingModified) */
    f32       latTransfer;                  /* 0x034 the load transfer of the lateral acceleration (T298) */
    f32       longTransfer;                 /* 0x038 the load transfer of the longitudinal force (T298) */
    f32       cgHeight;                     /* 0x03C the centre of gravity above the ground */
    f32       axleHeight[2];                /* 0x040 spec rideHeight[0] mm, both (the pitch is atan2([0] - [1], */
                                            /* wheelbase)) */
    f32       mass;                         /* 0x048 kg, with g_002D28B0.driverMass + fuelMass */
    f32       invMass;                      /* 0x04C 1 / mass (an impact x it: a speed change) */
    f32       invSprungMass;                /* 0x050 1 / the sprung mass (u081 heaveInv) */
    f32       originOffset;                 /* 0x054 (rearDist - frontDist) / 2 */
    f32       steerLock;                    /* 0x058 rad, spec +0x35 degrees (u079 / u082 steerMax) */
    Curve     steer0;                       /* 0x05C the lock over the speed, spec steer[0]: the pads' (u079 steerTable[0]) */
    f32       steer0X[6];                   /* 0x068 m/s */
    f32       steer0Y[6];                   /* 0x080 rad */
    Curve     steer1;                       /* 0x098 spec steer[2]: the wheel controller's (u079 steerTable[1]) */
    f32       steer1X[6];                   /* 0x0A4 */
    f32       steer1Y[6];                   /* 0x0BC */
    Curve     steer2;                       /* 0x0D4 spec steer[1]: loose ground (steering class 1: a dirt course, */
                                            /* for the pads only with both front wheels on class-1 surfaces;
                                               u079 steerTable[2]) */
    f32       steer2X[6];                   /* 0x0E0 */
    f32       steer2Y[6];                   /* 0x0F8 */
    f32       steerGain[2];                 /* 0x110 atan2(wb, wb -/+ halfTrack[0]) / (pi / 4): the inner, */
                                            /* outer wheel's share */
    f32       yawInertia;                   /* 0x118 spec +0x2E x 100 (0: 6000) */
    f32       yawInertiaInv;                /* 0x11C */
    f32       pitchFront;                   /* 0x120 frontDist x 2 pi / (spec +0x2F x 100) */
    f32       pitchRear;                    /* 0x124 */
    f32       rollFront;                    /* 0x128 halfTrack[0] x 2 pi / (spec +0x30 x 100) */
    f32       rollRear;                     /* 0x12C */
    f32       unk_130[2];                   /* 0x130 per axle (u081: ? the pitch lever of the drive force) */
    CarEngine engine;                       /* 0x138 (RaceCarState +0x148) */
    GearCalc  gear;                         /* 0x1F8 (u080 Drivetrain; u076 redZone +0x226 = gear +0x2E) */
    SuspParam susp[2];                      /* 0x2B8 front, rear (u076 AxleGeometry) */
    Curve     camberCurve[2];               /* 0x340 per axle: the grip over the camber (u077 c340) */
    f32       camberX[2][4];                /* 0x358 */
    f32       camberY[2][4];                /* 0x378 */
    TyreData  tyre[6];                      /* 0x398 compounds 0..2 front, 3..5 rear (u077 TireParam, 0x1B4) */
    u8        tyreIndex[2][16];             /* 0xDD0 [axle][surface slot] -> tyre[] (spec tyreSet; + 3 at the rear) */
    f32       axleGrip[2];                  /* 0xDF0 (spec +0x36 / +0x37 + 900) / 1000, x the rules' enemy */
                                            /* volume for computer cars */
    f32       maxSpeed;                     /* 0xDF8 the top speed, m/s (u077 topSpeed; CarDynamics_calcTopSpeed) */
    f32       camber[2];                    /* 0xDFC per axle, rad (-spec camber x 0.1 degree; u082 camberOffset) */
    f32       camberGain[2];                /* 0xE04 per axle, per stroke (spec +0x62 degrees) */
    f32       camberGrip[2];                /* 0xE0C camberCurve[axle](camber) (u086: ? a grip factor) */
    f32       swingArm[2];                  /* 0xE14 per axle, spec +0x38 % */
    f32       casterCamber;                 /* 0xE1C sin(spec +0x18 degrees): the camber change with the steering */
    f32       toe[2];                       /* 0xE20 per axle */
    f32       brakeTorque[2];               /* 0xE28 per axle, spec +0xDE x 10 x 9.8 (u080 unkE28) */
    f32       handbrakeTorque;              /* 0xE30 the rear wheels, spec +0xE0 x 10 x 9.8 */
    f32       unk_E34;                      /* 0xE34 mass / (4 brakeTorque[0] / tyre radius) (u086: ? the least */
                                            /* braking distance per (m/s)^2) */
    f32       absK[2];                      /* 0xE38 spec brakeBalance x 0.1 */
    f32       brakeK[2];                    /* 0xE40 spec +0x112 % */
    f32       absGain[2];                   /* 0xE48 tyre radius / brakeTorque (0: no brake) */
    f32       weight;                       /* 0xE50 mass x 9.8 (u086 unkE50) */
    f32       sprungWeight[2];              /* 0xE54 the sprung weight x frontRatio, rearRatio */
    f32       unk_E5C;                      /* 0xE5C spec +0x32 %: the drag share of the area coefficient */
    f32       dragCoef;                     /* 0xE60 the air drag x v^2 (T329 scales it in a slipstream) */
    f32       liftCoef[2];                  /* 0xE64 front (spec aeroF), rear (aeroR), x v^2 */
    f32       wearBase[2];                  /* 0xE6C per axle (u081: ? the tyre wear; g_002D28B0.wearLo / Hi) */
    f32       wearLoad[2];                  /* 0xE74 per axle, per load */
    TcsParams tcs;                          /* 0xE7C the driving aids from spec ascFlags.. (u077 AidParam) */
};
EGCS_SIZE(CarDynamics, 0xEB0);

/* CarSpec.specialKind / CarDynamics.specialKind: Lupo 2's special kinds,
 * set by SetupCarSpec from the car code (physics.md 2; L2 0x001DAC18).
 * 0 for every GT3 car.  3..6 select Lupo 2's bike / trike layouts, which
 * nothing in Lupo 2 writes (physics.md 1: not ported). */
enum {
    CAR_KIND_NONE      = 0,                 /* every other car */
    CAR_KIND_POD       = 1,                 /* pod_tmv_01, the Toyota Pod: emotions, parking, the face */
    CAR_KIND_WING_GTR  = 2,                 /* r35_gtr_tmv_01: a moving rear wing, 80 / 60 km/h */
    CAR_KIND_SL        = 7,                 /* sl500_02, sl55_amg_02: the active suspension (physics.md 26) */
    CAR_KIND_WING_XF   = 8,                 /* crossfire_dmv_02, crossfire_04: a moving rear wing */
    CAR_KIND_WING_CIEN = 9,                 /* cien_dmv_02: a moving rear wing, linear in the speed */
    CAR_KIND_WING_W12  = 10                 /* w12_tmv_01: a moving rear wing */
};

/* mass x 300 / maxPower^2 of the car `equip' in a new condition (SetupCarSpec,
 * CarDynamics_init with g_002D2988, engine_spec_sheet on the stack). */
f32  ValuateAutomobilePerformance(CarEquipments *equip);                    /* 0x001D98D0 */
/* *unit = the speed unit (UnitSettings g_002F59D8.system); *speedScale =
 * the speedometer's end: 2..4 steps of 80 km/h or 50 mph covering 1.1 x
 * maxSpeed; *redZone = gear.revLimit; *tachoScale = the tachometer's end:
 * engine.redline + 1500 rounded down to 1000 rpm. */
void SelectMeterScale(CarDynamics *dyn, int *unit, int *speedScale,
                      int *redZone, int *tachoScale);                       /* 0x001D9978 */
void SelectMeterScale(RaceCarState *car, int *unit, int *speedScale,
                      int *redZone, int *tachoScale);                       /* 0x001D9A10 its CarDynamics (+0x10) */
void SelectMeterScale(CarEquipments *equip, int *unit, int *speedScale,
                      int *redZone, int *tachoScale);                       /* 0x001D9A30 a car built on the stack */

#endif /* GAME_T279_METER_SCALE_H */
