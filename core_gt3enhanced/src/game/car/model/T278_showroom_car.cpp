/* T278 - ShowRoomCar: a car's parameter record (CarSpec), its physics
 * parameter block (CarDynamics) and its engine spec sheet, set up outside a
 * race; the figures the menus show (power, torque, top speed, drive train),
 * the wheel geometry of the menu car models and the settings ranges. */
#include "game/car/physics/car_spec_internal.h"
#include "game/car/model/T278_show_room_car.h"
#include "game/enhanced.h"                  /* ENH_RENDER_SHOWROOM_TABLES */
#include "engine/media/G002_mpeg_frameq.h"  /* the enhanced engine: atan2f, sinf, cosf (restLift) */

#if defined(__GNUC__) && __GNUC__ >= 3
#pragma GCC diagnostic ignored "-Wmultichar"    /* driveName()'s 'FR' .. 'RR' */
#endif

/* The enhanced engine (menus_b.md 18; rendering.md "For other areas"): the
 * Honda Dualnote's motors on the spec sheet.  Lupo 2 recognises the hybrid
 * by its DRIVETRAIN row, the 4WD layout with sub-type 5 (CarSpec +0xB2 = 2,
 * +0xB3 = 5: L2 0x001D9990..0x001D99A0), and adds the power of its three
 * motors, 2 x 20 kW at the front and the 35 kW crank motor (the hybrid
 * parameters' bytes 0 and 1, L2 .sdata 0x0035DFB1, expanded by L2
 * 0x001DFFF0; docs/gearbox/dualnote-hybrid.md), in PS (L2 0x001E1238:
 * (2 b0 + b1) / 0.7355, +102 PS).  No GT3 DRIVETRAIN row has sub-type 5
 * (docs/gearbox_dualnote.md), so no GT3 car gets it.  The physics area's
 * Dualnote port keeps the same parameters (RaceCarState::hybrid). */
#define SHOWROOM_AWD_HYBRID      5          /* CarSpec.awdType of the Dualnote */
#define HYBRID_FRONT_MOTOR_KW    20.0f      /* per front motor (b0) */
#define HYBRID_CRANK_MOTOR_KW    35.0f      /* the crank (IMA) motor (b1) */
#define KW_PER_PS                0.7355f

static inline f32 showroom_hybrid_power(void)
{
    f32 kw = HYBRID_FRONT_MOTOR_KW + HYBRID_FRONT_MOTOR_KW + HYBRID_CRANK_MOTOR_KW;
    return kw / KW_PER_PS;
}

// FUNC 0x001D93C0 ShowRoomCar::SetupShowRoomCar
//   The enhanced engine (Lupo 2's twin L2 0x001D9908): the surface /
//   slipstream tables first (ENH_RENDER_SHOWROOM_TABLES), and the Dualnote's
//   motors on the spec sheet's maximum power.
void ShowRoomCar::SetupShowRoomCar(CarEquipments *equip, CarCondition *cond)
{
    CarCondition fresh;

    if (cond == NULL)
        cond = &fresh;
#if ENH_RENDER_SHOWROOM_TABLES
    /* Lupo 2 (L2 0x001D9934): the rolling resistance curve, the surfaces'
     * resistance and the air density from DynParams before CarDynamics_init
     * computes the top speed with them; GT3 sets them up only in a race
     * (DynamicsConductor::virtual_20), so before the first race its menus
     * use the file's defaults (no rolling resistance, air 0.8 for 1.2) */
    surface_tables_init(0);
#endif
    SetupCarSpec(&spec, equip, 0, 0, 0, "ShowRoomCar::SetupShowRoomCar()");
    CarDynamics_init(&dyn, &spec, 0, NULL, &g_002D2988, cond);   /* 0x001DBFD8 */
    engine_spec_sheet(&dyn.engine, &sheet, cond);
    /* Lupo 2 (L2 0x001D9990..0x001D99A8): the Dualnote's motors */
    if (spec.layout == LAYOUT_4WD && spec.awdType == SHOWROOM_AWD_HYBRID)
        sheet.maxPower += showroom_hybrid_power();
}

/* The CarEquipments constructor already copies the car database's file
 * pointers; they are copied a second time here. */
// FUNC 0x001D9458 ShowRoomCar::SetupShowRoomCarByName
int ShowRoomCar::SetupShowRoomCarByName(const char *name)
{
    CarEquipments equip;
    CarCondition  cond;

    equip.copy(g_002EBD88);
    if (equip.setCar(nameToCode(name)) == 0)
        return 0;
    SetupShowRoomCar(&equip, &cond);
    return 1;
}

// FUNC 0x001D9528 ShowRoomCar::steerLock
f32 ShowRoomCar::steerLock()
{
    return dyn.steerLock;
}

// FUNC 0x001D9530 ShowRoomCar::strokeMin
f32 ShowRoomCar::strokeMin(int wheel)
{
    SuspParam *susp = &dyn.susp[wheel >> 1];
    return susp->strokeMin - susp->staticStroke;
}

// FUNC 0x001D9558 ShowRoomCar::strokeMax
f32 ShowRoomCar::strokeMax(int wheel)
{
    SuspParam *susp = &dyn.susp[wheel >> 1];
    return susp->strokeMax - susp->staticStroke;
}

// FUNC 0x001D9580 ShowRoomCar::wheelHeight
f32 ShowRoomCar::wheelHeight(int wheel, f32 steer, f32 stroke)
{
    SuspParam *susp = &dyn.susp[wheel >> 1];
    return (susp->gapOffset - susp->staticStroke) - stroke;
}

// FUNC 0x001D95A8 ShowRoomCar::wheelLateralPos
f32 ShowRoomCar::wheelLateralPos(int wheel, f32 steer, f32 stroke)
{
    return wheel_lateral_pos(&dyn, wheel, steer, stroke);
}

// FUNC 0x001D95C8 ShowRoomCar::wheelAxleX
f32 ShowRoomCar::wheelAxleX(int wheel, f32 steer, f32 stroke)
{
    f32 x = dyn.wheelbase * 0.5f;

    if ((wheel >> 1) == 0)                      /* the front axle */
        x = -x;
    return x;
}

// FUNC 0x001D95F0 ShowRoomCar::wheelSteerAngle
f32 ShowRoomCar::wheelSteerAngle(int wheel, f32 steer, f32 unused)
{
    return wheel_steer_angle(&dyn, wheel, steer, unused);
}

// FUNC 0x001D9610 ShowRoomCar::wheelCamber
f32 ShowRoomCar::wheelCamber(int wheel, f32 steer, f32 stroke)
{
    return wheel_camber(&dyn, wheel, steer, stroke);
}

#if ENH_PHYSICS_REAR_RIDE_HEIGHT
/* The enhanced engine: the header says what.  vu0_rotate_x turns the car's
 * rows so that a point at height y and length z ends at y c - z s: the front
 * axle (z = -wheelbase / 2) rises and the rear one drops by wheelbase / 2 x
 * sin a each, which with tan a = (axleHeight[0] - axleHeight[1]) / wheelbase
 * brings both tyres to the floor (with unequal tyre radii the rear is off
 * by their difference x (1 - cos a): a few micrometres). */
f32 ShowRoomCar::restLift(f32 radius, f32 *pitch)
{
    f32 a = atan2f(dyn.axleHeight[0] - dyn.axleHeight[1], dyn.wheelbase);
    f32 front;

    *pitch = a * (180.0f / 3.14159265f);
    front = wheelHeight(0, 0.0f, 0.0f) * cosf(a) - wheelAxleX(0, 0.0f, 0.0f) * sinf(a);
    return radius - front;
}
#endif

/* The table is a local array initialised from .rodata 0x0033C2F0. */
// FUNC 0x001D9630 ShowRoomCar::driveName
int ShowRoomCar::driveName()
{
    int names[5] = { 'FR', 'FF', '4WD', 'MR', 'RR' };   /* LAYOUT_FR .. LAYOUT_RR */

    return names[spec.layout];
}

/* The spec sheet's rpm is read with lhu (T291 types it s16).  The catalogue
 * figure is read once, into an int (one lhu for the test and the
 * conversion; testing the u16 field compares in HImode, and the conversion
 * then reloads it). */
// FUNC 0x001D9680 ShowRoomCar::power
int ShowRoomCar::power(int *rpm, int unitTag, int catalogue)
{
    f32 value;
    int catPower = spec.catalogPower;

    if (catalogue != 0 && catPower != 0) {
        *rpm = spec.catalogPowerRpm * 100;
        value = g_002F59D8.convPower((f32)catPower) + 0.1f;
    } else {
        *rpm = (u16)sheet.maxPowerRpm;
        value = g_002F59D8.convPower(sheet.maxPower);
    }
    return (int)value;
}

/* Quirk kept: the computed torque (x 10) goes through convPower, the
 * catalogue one through convTorque. */
// FUNC 0x001D96F8 ShowRoomCar::torque
int ShowRoomCar::torque(int *rpm, int unitTag, int catalogue)
{
    f32 value;
    int catTorque = spec.catalogTorque;

    if (catalogue != 0 && catTorque != 0) {
        *rpm = spec.catalogTorqueRpm * 100;
        value = g_002F59D8.convTorque((f32)catTorque) + 0.1f;
    } else {
        *rpm = (u16)sheet.maxTorqueRpm;
        value = g_002F59D8.convPower(sheet.maxTorque * 10.0f);
    }
    return (int)value;
}

// FUNC 0x001D9778 ShowRoomCar::maxSpeed
int ShowRoomCar::maxSpeed(int unitTag, int catalogue)
{
    if (catalogue != 0 && spec.catalogSpeed != 0)
        return spec.catalogSpeed;
    return (int)(dyn.maxSpeed * 3.6f);          /* m/s -> km/h */
}

// FUNC 0x001D97A8 ShowRoomCar::getCaster
u8 ShowRoomCar::getCaster(s8 *min, s8 *max)
{
    *min = 0;
    *max = 90;
    return spec.caster;
}

// FUNC 0x001D97C0 ShowRoomCar::getCamberGain
s8 ShowRoomCar::getCamberGain(int wheel, s8 *min, s8 *max)
{
    *min = -100;
    *max = 100;
    return spec.camberGain[wheel >> 1];
}

// FUNC 0x001D97E0 ShowRoomCar::getSwingArm
u8 ShowRoomCar::getSwingArm(int wheel, s8 *min, s8 *max)
{
    *min = 0;
    *max = 100;
    return spec.swingArm[wheel >> 1];
}

// FUNC 0x001D9800 ShowRoomCar::getTrackOffset
s8 ShowRoomCar::getTrackOffset(int wheel, s8 *min, s8 *max)
{
    *min = -100;
    *max = 100;
    return spec.trackOffset[wheel >> 1];
}

// FUNC 0x001D9820 ShowRoomCar::getBumpTravel
u8 ShowRoomCar::getBumpTravel(int wheel, u8 *min, u8 *max)
{
    int axle = wheel >> 1;

    *min = 0;
    *max = spec.rideHeight[axle];
    return spec.strokeLimit[axle];
}

// FUNC 0x001D9840 ShowRoomCar::setCaster
void ShowRoomCar::setCaster(u8 v)
{
    spec.caster = v;
}

// FUNC 0x001D9848 ShowRoomCar::setCamberGain
void ShowRoomCar::setCamberGain(int wheel, s8 v)
{
    spec.camberGain[wheel >> 1] = v;
}

// FUNC 0x001D9858 ShowRoomCar::setSwingArm
void ShowRoomCar::setSwingArm(int wheel, u8 v)
{
    spec.swingArm[wheel >> 1] = v;
}

// FUNC 0x001D9868 ShowRoomCar::setTrackOffset
void ShowRoomCar::setTrackOffset(int wheel, s8 v)
{
    spec.trackOffset[wheel >> 1] = v;
}

// FUNC 0x001D9878 ShowRoomCar::setBumpTravel
void ShowRoomCar::setBumpTravel(int wheel, u8 v)
{
    spec.strokeLimit[wheel >> 1] = v;
}

// FUNC 0x001D9888 ShowRoomCar::setupDynamics
void ShowRoomCar::setupDynamics()
{
    CarCondition fresh;

    CarDynamics_init(&dyn, &spec, 0, NULL, &g_002D2988, &fresh);   /* 0x001DBFD8 */
}
