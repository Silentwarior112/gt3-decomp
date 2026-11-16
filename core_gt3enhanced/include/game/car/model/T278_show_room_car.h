/* T278_show_room_car.h - ShowRoomCar, a car set up outside a race: its
 * parameter record, its physics parameter block and its engine spec sheet
 * (translation unit T278, unit u076), 0x001D93C0..0x001D98D0.
 *
 * SetupShowRoomCar ("ShowRoomCar::SetupShowRoomCar()", the caller string it
 * passes to SetupCarSpec, T282) builds the three parts from a CarEquipments
 * and a CarCondition: SetupCarSpec, CarDynamics_init (T284) with the default
 * rules g_002D2988 (T281), engine_spec_sheet (T291).  The menus read it: the
 * power / torque / top speed figures (catalogue or computed, in the user's
 * units), the drive train, and the wheel geometry the car models are posed
 * with (the five wheel accessors: T107, T180, T184, T245's CarRender).
 *
 * Users: a stack object of 0x1350 bytes in u015, u037, u043, u050, u061,
 * u108 (describe), u109; T232's MenuCarSpec::car (its first member).  Lupo 2
 * has a class of this name (RaceBikeModel::renderTireForIcon(ShowRoomCar &)).
 *
 * The setting getters / setters (0x001D97A8..0x001D9878) have no callers;
 * they are named after what CarDynamics_init (T284, u077) makes of the
 * bytes they reach. */
#ifndef GAME_T278_SHOW_ROOM_CAR_H
#define GAME_T278_SHOW_ROOM_CAR_H

#include "game/game.h"
#include "game/data/T482_car_equipments.h"       /* CarSpec (held by value), CarEquipments */
#include "game/race/display/T279_meter_scale.h"          /* CarDynamics (held by value) */
#include "game/car/physics/T291_engine.h"               /* EngineSpecSheet (held by value) */
#include "game/enhanced.h"                              /* ENH_PHYSICS_REAR_RIDE_HEIGHT (restLift) */

class CarCondition;                         /* T280_car_condition.h (pointers only) */

/* Not polymorphic, no constructor (the callers' stack space is used as it
 * is).  Size 0x1344 (CarSpec 0x3E2 + 2 bytes of alignment, CarDynamics
 * 0xEB0, EngineSpecSheet 0xB0; the users' 0x1350-byte slots, MenuCarSpec's
 * CarEquipments at +0x1348). */
class ShowRoomCar {
public:
    /* cond NULL: a new condition (a CarCondition on the stack). */
    void SetupShowRoomCar(CarEquipments *equip, CarCondition *cond);    /* 0x001D93C0 */
    /* The car `name' (nameToCode) on a CarEquipments of its own, in a new
     * condition.  0 if the database has no such car (no callers). */
    int  SetupShowRoomCarByName(const char *name);                      /* 0x001D9458 */

    f32  steerLock();                       /* 0x001D9528 dyn.steerLock, rad */
    /* The stroke range of a wheel (axle = wheel >> 1) from its static
     * position: susp.strokeMin / strokeMax - staticStroke. */
    f32  strokeMin(int wheel);              /* 0x001D9530 */
    f32  strokeMax(int wheel);              /* 0x001D9558 */
    /* The wheel accessors of the menu car models (every caller passes its
     * wheel 0..3 and the same steer and stroke, 0). */
    /* susp.gapOffset - staticStroke - stroke: the wheel centre's height
     * (tyre radius - axle height - stroke); steer is not read. */
    f32  wheelHeight(int wheel, f32 steer, f32 stroke);                 /* 0x001D9580 (F_001D9580) */
    f32  wheelLateralPos(int wheel, f32 steer, f32 stroke);             /* 0x001D95A8 wheel_lateral_pos (F_001D95A8) */
    /* -wheelbase / 2 for the front wheels, + for the rear; steer, stroke
     * are not read (as car_wheel_axle_x, T302). */
    f32  wheelAxleX(int wheel, f32 steer, f32 stroke);                  /* 0x001D95C8 (F_001D95C8; u076 axlePosition) */
    f32  wheelSteerAngle(int wheel, f32 steer, f32 unused);             /* 0x001D95F0 wheel_steer_angle (F_001D95F0) */
    f32  wheelCamber(int wheel, f32 steer, f32 stroke);                 /* 0x001D9610 wheel_camber (F_001D9610) */
#if ENH_PHYSICS_REAR_RIDE_HEIGHT
    /* The enhanced engine (from the PC port, its ROADMAP 68): the menu cars'
     * pose with each axle at its own ride height.  The menus lift the body
     * until the front tyre touches the floor and draw it level, which fits
     * only when the two axle heights are equal (GT3: rideHeight[0] for
     * both); with the rear's own the rear tyres sank into the floor or
     * floated over it.  *pitch: the body's pitch in degrees for vu0_rotate_x
     * in the car's frame, atan2(axleHeight[0] - axleHeight[1], wheelbase)
     * as DynCar_place (T330) starts a race car; returned: the lift that
     * keeps the front tyre (radius) on the floor once pitched. */
    f32  restLift(f32 radius, f32 *pitch);
#endif
    /* The drive train's name as a multi-character constant: 'FR', 'FF',
     * '4WD', 'MR', 'RR' by spec.layout (the bytes "RF\0\0" .. in memory). */
    int  driveName();                                                   /* 0x001D9630 */
    /* The power in the user's unit (UnitSettings g_002F59D8), its rpm in
     * *rpm: the catalogue figures (+ 0.1, then truncated) when `catalogue'
     * and the car has them, else the spec sheet's.  unitTag ('ps') is not
     * read. */
    int  power(int *rpm, int unitTag, int catalogue);                   /* 0x001D9680 */
    /* As power for the torque, in tenths (the callers' unitTag is 'dkgm');
     * the computed torque (x 10) goes through convPower, not convTorque (a
     * quirk of the original). */
    int  torque(int *rpm, int unitTag, int catalogue);                  /* 0x001D96F8 */
    /* km/h: the catalogue figure, else dyn.maxSpeed x 3.6 ('kmph' from the
     * caller; unitTag is not read). */
    int  maxSpeed(int unitTag, int catalogue);                          /* 0x001D9778 */

    /* The settings with their ranges (no callers).  The value, *min, *max. */
    u8   getCaster(s8 *min, s8 *max);                       /* 0x001D97A8 spec.caster, 0..90 (? the caster angle, degrees:
                                                               CarDynamics.casterCamber = sin of it) */
    s8   getCamberGain(int wheel, s8 *min, s8 *max);        /* 0x001D97C0 spec.camberGain[axle], -100..100 (CarDynamics.camberGain,
                                                               degrees per stroke) */
    u8   getSwingArm(int wheel, s8 *min, s8 *max);          /* 0x001D97E0 spec.swingArm[axle], 0..100 % (CarDynamics.swingArm) */
    s8   getTrackOffset(int wheel, s8 *min, s8 *max);       /* 0x001D9800 spec.trackOffset[axle], -100..100 mm (halfTrack = (tread
                                                               - 2 x it) / 2) */
    u8   getBumpTravel(int wheel, u8 *min, u8 *max);        /* 0x001D9820 spec.strokeLimit[axle], 0..spec.rideHeight[axle] mm (the
                                                               stroke limit of SuspParam_initStroke) */
    void setCaster(u8 v);                                   /* 0x001D9840 */
    void setCamberGain(int wheel, s8 v);                    /* 0x001D9848 */
    void setSwingArm(int wheel, u8 v);                      /* 0x001D9858 */
    void setTrackOffset(int wheel, s8 v);                   /* 0x001D9868 */
    void setBumpTravel(int wheel, u8 v);                    /* 0x001D9878 */
    /* CarDynamics_init again from spec (after a setting changed), for a car
     * in a new condition; the spec sheet is not rebuilt (no callers). */
    void setupDynamics();                                   /* 0x001D9888 */

    CarSpec         spec;                   /* 0x0000 SetupCarSpec */
    CarDynamics     dyn;                    /* 0x03E4 CarDynamics_init */
    EngineSpecSheet sheet;                  /* 0x1294 engine_spec_sheet (u076 engine, h17 EngineOutput) */
};
EGCS_SIZE(ShowRoomCar, 0x1344);

#endif /* GAME_T278_SHOW_ROOM_CAR_H */
