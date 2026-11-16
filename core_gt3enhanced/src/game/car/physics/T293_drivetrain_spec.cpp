/* T293 (0x001E2670..0x001E28A8): the tyre dimensions and the drivetrain
 * layout of a CarSpec.  (T293 has no canonical header: both functions are
 * used inside u079 only, by gear_calc and gear_auto_ratios.) */
#include "game/car/physics/car_input_internal.h"
#include "game/car/physics/T344_drivetrain.h"   /* hybrid_params_init (the enhanced engine) */
#include "game/enhanced.h"                      /* ENH_PHYSICS_* */

/* The tyre of an axle (0 front, 1 rear) in metres: the width, the sidewall
 * height (width x aspect x 0.05) and the radius (rim / 2 + sidewall).  A rear
 * width of 0 counts as 10 mm; the front one has no such default. */
// FUNC 0x001E2670 tyre_dimensions
void tyre_dimensions(CarSpec *spec, int axle, f32 *radius, f32 *width, f32 *sidewall)
{
    u8  rim;
    u8  aspect;
    int widthMm;
    f32 rimRadius;
    f32 widthM;
    f32 side;

    if (axle == 0) {
        widthMm = spec->tyreWidth[0] * 5;
        aspect = spec->tyreAspect[0];
        rim = spec->tyreRim[0];
    } else {
        rim = spec->tyreRim[1];
        aspect = spec->tyreAspect[1];
        widthMm = spec->tyreWidth[1] * 5;
        if (widthMm == 0)
            widthMm = 10;
    }
    rimRadius = (f32)rim * 0.0254f * 0.5f;          /* inches -> m */
    widthM = (f32)widthMm * 0.001f;
    *width = widthM;
    side = widthM * (f32)aspect * 0.05f;
    *sidewall = side;
    *radius = rimRadius + side;
}

/* The drivetrain of a spec: *type (GearCalc.driveType, DRIVE_*), *axle (the
 * driven axle, 0 front / 1 rear), *split and *torque (the 4WD values: the
 * spec's awdSplit and awdParam, x 10 x 9.8 with a default per kind).
 * Returns *type.  For a layout above 4 (and in GT3 for awdType 5 / 6) *type
 * is not written.  The enhanced engine (Lupo 2 L2 0x001E3CE0): awdType 5 is
 * the Honda Dualnote (rear drive, the hybrid's constants set up), 6 the
 * front-primary fixed split DRIVE_AWD_FIXED_FRONT; ENH_PHYSICS_ATTESA
 * retunes awdType 3. */
/* The spec's 4WD parameter, or def when it is 0. */
static inline int awd_param(CarSpec *spec, int def)
{
    int p = spec->awdParam;
    return (p != 0) ? p : def;
}

// FUNC 0x001E2700 drivetrain_layout
u8 drivetrain_layout(CarSpec *spec, u8 *type, u8 *axle, f32 *split, f32 *torque)
{
    int awd;

    *split = 0.0f;
    *torque = 0.0f;
    switch (spec->layout) {
    case LAYOUT_FR:
    case LAYOUT_MR:
    case LAYOUT_RR:
        *type = DRIVE_REAR;
        *axle = 1;
        break;
    case LAYOUT_FF:
        *type = DRIVE_FRONT;
        *axle = 0;
        break;
    case LAYOUT_4WD:
        awd = spec->awdType;            /* read before the stores (retail's order) */
        *split = (f32)spec->awdSplit * 0.01f;
        *axle = 1;
        switch (awd) {
        case 0:
            *type = DRIVE_AWD_FRONT;
            *axle = 0;
            break;
        case 1:
            *type = DRIVE_AWD_CENTRE;
            break;
        case 2:
            *type = DRIVE_AWD_SPLIT;
            if (spec->awdSplit >= 50)
                *split = 1.0f;
            *torque = (f32)(awd_param(spec, 8) * 10) * 9.8f;
            break;
        case 3:
            *type = DRIVE_AWD_SPEED;
            *split *= 0.35999998f;          /* 0x3EB851EB, one ulp under 0.36f */
#if ENH_PHYSICS_ATTESA
            /* Lupo 2: twice the front-transfer gain (200 / 100) and no
             * coupling torque limit (L2 0x001E3DDC..0x001E3E54: its bytes
             * 200, 0; car_centre_coupling's 25 % / 15 % are its other two,
             * physics.md 12) */
            *split = *split * 2.0f;
            *torque = 0.0f;
#else
            *torque = (f32)(awd_param(spec, 15) * 10) * 9.8f;
#endif
            break;
        case 4:
            *type = DRIVE_AWD_STEER;
            if (*split != 0.0f)
                *split = 1.0f / (*split * 100.0f * DEG2RAD);
            *torque = (f32)(awd_param(spec, 15) * 10) * 9.8f;
            break;
        case 5:
            /* The enhanced engine: the Honda Dualnote (docs/gearbox_dualnote.md;
             * Lupo 2 L2 0x001E3ED0: its type 7, routed like the rear drive,
             * and the hybrid's constants, L2 0x001DFFF0).  Rear drive here;
             * RaceCarState.hybrid marks it (DynCar_initialize). */
            *type = DRIVE_REAR;
            hybrid_params_init();
            break;
        case 6:
            /* The enhanced engine: Lupo 2's front-primary fixed split
             * (physics.md 11; L2 0x001E3EF0, its type 9, axle 0) */
            *type = DRIVE_AWD_FIXED_FRONT;
            *axle = 0;
            break;
        case 7:
            *type = DRIVE_AWD_FIXED;
            break;
        }
        break;
    }
    return *type;
}

/* The enhanced engine (new): the Honda Dualnote is DRIVETRAIN layout 2 with
 * sub-type 5 (Lupo 2's own test, docs/gearbox_dualnote.md section 2; Lupo 2
 * marks it with drivetrain type 7 and tests that, L2 0x001E0288).  No GT3
 * DRIVETRAIN row has sub-type 5.  Callers: CarDynamics_init (the mass),
 * DynCar_initialize (RaceCarState.hybrid). */
int drivetrain_is_hybrid(const CarSpec *spec)
{
    return spec->layout == LAYOUT_4WD && spec->awdType == 5;
}
