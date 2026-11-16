/* T288 - the car sound parameters: 0x001DF4C0..0x001DFF40.
 *
 * car_sound_params_init turns the 0x28-byte car sound block of the tyre
 * sound table into g_0049CFD8 (x 10 km/h to m/s, percentages, ...).  The
 * evaluators below are called by RaceEntryCar::updateSound (T121) and the
 * tyre sound evaluators (T287), and return a volume with a pitch through
 * the pointer; the roles (wind, road, rolling tyre, gear whine, engine
 * levels) are guesses from the formulas.  The crash sound keeps the loudest
 * crash of the frame (cleared by DynamicsConductor::update every frame).
 *
 * Data (retail): .rodata 0x0033C498..0x0033C4B9, nothing the code
 * references: <new>'s "bad_alloc", then UnitArena<64>'s type name (T153
 * with GAME_T153_EMITS_TYPEINFO); .lit4 0x003502C0; .bss 0x0049CFD8, the
 * parameters (a file static: retail has it in .bss). */
// DATA_START 0x0033C498
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* RaceCourse (seen: UnitArena<64>) */
#include "game/car/physics/car_engine_internal.h"
#include "game/sound/T288_car_sound.h"
#include "game/car/physics/T292_gear_calc.h"            /* GearCalc.gearboxType, drivenAxle; GEARBOX_ELECTRIC */
#include "game/enhanced.h"                  /* ENH_SOUND_ENGINE_LOAD, ENH_SOUND_WHINE_REVERSE */

/* The enhanced engine (Lupo 2, cameras_sound.md S6): gearbox type 3's
 * motor is silent below 100 rpm (10.472 rad/s, L2 0x001DE48C); the motor
 * whines of the Dualnote (L2 0x001E11C8) and of type 3 (L2 0x001DFED8). */
#define ELECTRIC_SILENT_SPEED   10.472f     /* rad/s */
#define HYBRID_WHINE_PITCH_K    0.012f      /* per m/s of the front wheels' mean tread speed */
#define HYBRID_WHINE_PITCH_BASE 0.4f
#define HYBRID_WHINE_VOLUME_K   0.25f       /* x the front motor's load */
#define ELECTRIC_WHINE_PITCH_K  0.012f      /* per m/s of the driven wheels' mean tread speed */
#define ELECTRIC_WHINE_PITCH_BASE 0.3f
#define ELECTRIC_WHINE_VOLUME_K 0.2f        /* x the driven wheels' brakes */

/* The car sound parameters, set by car_sound_params_init (.bss). */
static CarSoundParams g_0049CFD8;           /* 0x0049CFD8 */

/* The speeds of the block are in 10 km/h (x 2.777778 = m/s), the pitches of
 * the wind in 100 (an integer pitch), the others x 0.01 / x 0.1. */
// FUNC 0x001DF4C0 car_sound_params_init
void car_sound_params_init(TyreSoundTable *t)
{
    const CarSoundSource *s = &t->src;
    CarSoundParams *p = &g_0049CFD8;

    f32 gain = (f32)s->noiseGain / 10.0f;
    f32 c = ((f32)s->windCoef / 10.0f) / 100.0f;
    p->windGain = gain * c * c;
    p->windSpeedLo = (f32)s->windSpeedLo * 2.777778f;
    p->windSpeedHi = (f32)s->windSpeedHi * 2.777778f;
    p->windPitchLo = (f32)(s->windPitchLo * 100);
    p->windPitchHi = (f32)(s->windPitchHi * 100);
    p->windMax = (f32)s->windMax / 10.0f;
    c = ((f32)s->roadCoef / 10.0f) / 100.0f;
    p->roadGain = gain * c * c;
    p->roadSpeedLo = (f32)s->roadSpeedLo * 2.777778f;
    p->roadSpeedHi = (f32)s->roadSpeedHi * 2.777778f;
    p->roadPitchLo = (f32)s->roadPitchLo / 100.0f;
    p->roadPitchHi = (f32)s->roadPitchHi / 100.0f;
    p->roadMax = (f32)s->roadMax / 10.0f;
    p->rollLo = (f32)s->rollLo / 100.0f;
    p->rollHi = (f32)s->rollHi / 100.0f;
    p->rollGain = (f32)s->rollGain / 100.0f;
    p->rollSpeedLo = (f32)s->rollSpeedLo * 2.777778f;
    p->rollSpeedHi = (f32)s->rollSpeedHi * 2.777778f;
    p->rollPitchLo = (f32)s->rollPitchLo / 100.0f;
    p->rollPitchHi = (f32)s->rollPitchHi / 100.0f;
    p->rollMax = (f32)s->rollMax / 10.0f;
    p->whineLo = (f32)s->whineLo / 100.0f;
    p->whineHi = (f32)s->whineHi / 100.0f;
    p->whineTorqueMax = (f32)s->whineAccel * 9.8f;
    f32 base = (f32)s->whinePitchBase / 10.0f;
    f32 top = (f32)s->whinePitchTop / 10.0f;
    p->whinePitchBase = base;
    /* 1047.1976 rad/s = 10000 rpm */
    p->whinePitchSlope = (top - base) / 1047.1976f;
    p->engine64 = (f32)s->engine1C / 100.0f;
    p->engine68 = (f32)s->engine1D / 100.0f;
    p->engine6C = (f32)s->engine1E / 100.0f;
    p->engine70 = (f32)s->engine1F / 10.0f;
    p->crashGain = (f32)s->crashGain / 100.0f;
}

/* Volume: windGain x the drag share (params.unk_E5C) x speedX^2 x (1 - the
 * slipstream blend), at most windMax.  Pitch (an integer): windPitchLo..Hi
 * over |speedX| windSpeedLo..Hi. */
// FUNC 0x001DF840 car_sound_wind
f32 car_sound_wind(RaceCarState *car, u16 *pitch)
{
    const CarSoundParams *p = &g_0049CFD8;
    DynCarState *st = &car->st;
    f32 v = st->speedX;
    f32 vol = p->windGain * car->params.unk_E5C * v * v;
    vol *= 1.0f - st->slipBlend;
    f32 speed = v;
    if (speed < 0.0f)
        speed = -speed;
    f32 f;
    if (speed < p->windSpeedLo)
        f = p->windPitchLo;
    else if (p->windSpeedHi < speed)
        f = p->windPitchHi;
    else
        f = p->windPitchLo + ((speed - p->windSpeedLo) * (p->windPitchHi - p->windPitchLo))
                             / (p->windSpeedHi - p->windSpeedLo);
    *pitch = (int)f;
    if (p->windMax < vol)
        vol = p->windMax;
    return vol;
}

/* As car_sound_wind with the road set and a float pitch. */
// FUNC 0x001DF910 car_sound_road
f32 car_sound_road(RaceCarState *car, f32 *pitch)
{
    const CarSoundParams *p = &g_0049CFD8;
    DynCarState *st = &car->st;
    f32 v = st->speedX;
    f32 vol = p->roadGain * car->params.unk_E5C * v * v;
    vol *= 1.0f - st->slipBlend;
    f32 speed = v;
    if (speed < 0.0f)
        speed = -speed;
    f32 f;
    if (speed < p->roadSpeedLo)
        f = p->roadPitchLo;
    else if (p->roadSpeedHi < speed)
        f = p->roadPitchHi;
    else
        f = p->roadPitchLo + ((speed - p->roadSpeedLo) * (p->roadPitchHi - p->roadPitchLo))
                             / (p->roadSpeedHi - p->roadSpeedLo);
    *pitch = f;
    if (p->roadMax < vol)
        vol = p->roadMax;
    return vol;
}

/* One wheel: silent (pitch rollPitchLo) unless it has had load lately and
 * its surface is 0, 1, 7, 8 or 9.  Volume (|spin| / 100)^2 x rollGain x a
 * factor rollLo..rollHi over the axle's tyre width 0.135..0.355 m; pitch
 * rollPitchLo..Hi over |spin| rollSpeedLo..Hi.  Callers: tyre_sound_roll_0789 (T287). */
// FUNC 0x001DF9D8 car_sound_tyre_roll
f32 car_sound_tyre_roll(RaceCarState *car, int wheel, f32 *pitch)
{
    const CarSoundParams *p = &g_0049CFD8;
    CarDynamics *params = &car->params;
    DynCarState *st = &car->st;
    DynWheel *w = &st->wheel[wheel];
    int rolls = 1;

    if (w->contactTimer == 0) {
        rolls = 0;
    } else {
        switch (w->surface) {
        case 0: case 1: case 7: case 8: case 9:
            break;
        default:
            rolls = 0;
            break;
        }
    }
    if (rolls == 0) {
        *pitch = p->rollPitchLo;
        return 0.0f;
    }

    f32 widthK = params->gear.tyreWidth[wheel >> 1];
    if (widthK < 0.135f)
        widthK = p->rollLo;
    else if (0.35500002f < widthK)
        widthK = p->rollHi;
    else
        widthK = p->rollLo + ((widthK - 0.135f) * (p->rollHi - p->rollLo)) / 0.22000001f;

    f32 speed = w->spin;
    if (speed < 0.0f)
        speed = -speed;
    f32 vol = speed / 100.0f;
    vol = vol * vol * p->rollGain * widthK;
    f32 f;
    if (speed < p->rollSpeedLo)
        f = p->rollPitchLo;
    else if (p->rollSpeedHi < speed)
        f = p->rollPitchHi;
    else
        f = p->rollPitchLo + ((speed - p->rollSpeedLo) * (p->rollPitchHi - p->rollPitchLo))
                             / (p->rollSpeedHi - p->rollSpeedLo);
    *pitch = f;
    if (p->rollMax < vol)
        vol = p->rollMax;
    return vol;
}

/* The enhanced engine (Lupo 2 L2 0x001E11C8, cameras_sound.md S6): the
 * Dualnote's front motor whine.  Pitch 0.4 + 0.012 x the front wheels' mean
 * |tread speed|; volume 0.25 x the front motor's load, which the Dualnote's
 * drivetrain leaves in clutchTorqueAbs (as GT3's gear whine reads it;
 * INTERFACES.md).  Caller: car_sound_gear_whine, for a hybrid car. */
static f32 car_sound_hybrid_whine(RaceCarState *car, f32 *pitch)
{
    DynCarState *st = &car->st;
    f32 sum = 0.0f;

    for (int i = 0; i < 2; i++) {
        f32 spin = st->wheel[i].spin;
        if (spin < 0.0f)
            spin = -spin;
        sum = sum + spin;
    }
    *pitch = sum * 0.5f * HYBRID_WHINE_PITCH_K + HYBRID_WHINE_PITCH_BASE;
    return st->clutchTorqueAbs * HYBRID_WHINE_VOLUME_K;
}

/* The enhanced engine (Lupo 2 L2 0x001DFED8, cameras_sound.md S6): gearbox
 * type 3's motor whine, heard when braking with the clutch closed (else
 * volume 0, pitch 1).  Pitch 0.3 + 0.012 x the driven wheels' mean |tread
 * speed|; volume 0.2 x the two wheels' brakes.  Lupo 2 finds the driven
 * axle's wheels through its vehicle layouts (L2 0x001DB870); for a car they
 * are 2 x axle and the one beside it.  Caller: car_sound_gear_whine. */
static f32 car_sound_electric_whine(RaceCarState *car, f32 *pitch)
{
    DynCarState *st = &car->st;
    DynWheel *w0, *w1;
    f32 speed;

    if (st->brake == 0.0f || st->clutch == 0.0f) {
        *pitch = 1.0f;
        return 0.0f;
    }
    w0 = &st->wheel[2 * car->params.gear.drivenAxle];
    w1 = w0 + 1;
    speed = (w0->spin + w1->spin) * 0.5f;
    if (speed < 0.0f)
        speed = -speed;
    *pitch = speed * ELECTRIC_WHINE_PITCH_K + ELECTRIC_WHINE_PITCH_BASE;
    return (w0->brake + w1->brake) * ELECTRIC_WHINE_VOLUME_K;
}

/* Only for a car with gearWhineOn and the clutch not free (clutchState !=
 * 0; else volume 0, pitch 1): volume whineLo..Hi by |clutchTorqueAbs| (at
 * most whineTorqueMax, a torque: x 9.8) x ratio[gear] / (whineTorqueMax x
 * ratio[1]), i.e. the torque through the gearbox against 1st gear's; pitch
 * whinePitchBase + |the driven axle's speed| x whinePitchSlope.
 * The enhanced engine (Lupo 2 L2 0x001DE2C0): a Dualnote (RaceCarState::
 * hybrid) and gearbox type 3 have their motor whines instead (S6, no
 * switch: GT3's cars are neither); ENH_SOUND_WHINE_REVERSE: the torque
 * share as |share| (L2 0x001DE3E4), so the reverse gear's negative ratio
 * gives the forward gears' whine, not a quieter, inverted one. */
// FUNC 0x001DFB58 car_sound_gear_whine
f32 car_sound_gear_whine(RaceCarState *car, f32 *pitch)
{
    const CarSoundParams *p = &g_0049CFD8;
    DynCarState *st = &car->st;

    if (car->hybrid)                        /* Lupo 2: the Dualnote (L2 0x001DE2F0) */
        return car_sound_hybrid_whine(car, pitch);
    if (car->params.gear.gearboxType == GEARBOX_ELECTRIC)   /* Lupo 2: type 3 (L2 0x001DE32C) */
        return car_sound_electric_whine(car, pitch);

    if (car->gearWhineOn == 0 || st->clutchState == 0) {
        *pitch = 1.0f;
        return 0.0f;
    }
    f32 vol = st->clutchTorqueAbs;
    f32 max = p->whineTorqueMax;
    if (vol < 0.0f)
        vol = -vol;
    if (max < vol)
        vol = max;
    vol = (vol * car->params.gear.ratio[st->gear]) / (max * car->params.gear.ratio[1]);
#if ENH_SOUND_WHINE_REVERSE
    if (vol < 0.0f)                         /* Lupo 2 (L2 0x001DE3E4) */
        vol = -vol;
#endif
    vol = p->whineLo + (p->whineHi - p->whineLo) * vol;
    f32 axle = car_driven_axle_speed(car);
    if (axle < 0.0f)
        axle = -axle;
    *pitch = p->whinePitchBase + axle * p->whinePitchSlope;
    return vol;
}

/* Per frame (car_step): the three engine sound levels.  [1] = the average
 * of itself and (1 + 3 t) x 32 x engine68 (t = turboThrottle, at most 1),
 * [0] = the average of itself and (|t - 0.375| + 0.375) x (1 with the clutch
 * locked, else 0.5) x 127 x engine64, [2] (the turbo's, engine_update)
 * decays by engine6C; each at most 127.  The blow-off level blowOffLevel decays
 * by engine70.
 * The enhanced engine (Lupo 2 L2 0x001DE458):
 * - gearbox type 3: both levels 0 while the motor turns below 100 rpm (S6,
 *   no switch);
 * - ENH_SOUND_ENGINE_LOAD (cameras_sound.md S2): the break point is the
 *   throttle at which the engine gives no net torque at this speed, r =
 *   frictionTorque / (frictionTorque + engineTorque) (engine_output: load x
 *   (torque + friction) - friction), not 0.375; [1] = (0.4 + 0.6 (1 - (1 -
 *   x)^16)) x 127 x engine68 with x = max(0, t - r) / (1 - r), louder at
 *   idle and full as soon as the engine pulls.  Lupo 2 does not guard the
 *   divisions: here r is 0.375 while both torques are 0 (the frames before
 *   the first step), and x is 0 when t <= r. */
// FUNC 0x001DFC60 car_sound_engine_levels
void car_sound_engine_levels(RaceCarState *car)
{
    const CarSoundParams *p = &g_0049CFD8;
    DynCarState *st = &car->st;

    if (car->params.gear.gearboxType == GEARBOX_ELECTRIC && st->engineSpeed < ELECTRIC_SILENT_SPEED) {
        st->engineSnd[0] = 0;               /* Lupo 2: the motor at rest is silent (L2 0x001DE478..0x001DE4A4) */
        st->engineSnd[1] = 0;
        return;
    }

    f32 t = st->turboThrottle;
    if (1.0f < t)
        t = 1.0f;
#if ENH_SOUND_ENGINE_LOAD
    /* Lupo 2 (L2 0x001DE4A8..0x001DE5D0) */
    f32 torques = st->frictionTorque + st->engineTorque;
    f32 r = 0.375f;
    if (torques != 0.0f)
        r = st->frictionTorque / torques;
    f32 x = t - r;
    f32 d = x;
    if (d < 0.0f)
        d = -d;
    d = d + r;
    if (st->clutchState != 1)               /* 1: locked */
        d = d * 0.5f;
    int level0 = (int)(d * 127.0f * p->engine64);
    if (!(level0 < 0x80))
        level0 = 0x7F;
    st->engineSnd[0] = (st->engineSnd[0] + level0) / 2;
    f32 pull = 0.0f;
    if (0.0f < x)
        pull = x / (1.0f - r);
    f32 rest = 1.0f - pull;
    rest = rest * rest;                     /* (1 - x)^16 */
    rest = rest * rest;
    rest = rest * rest;
    rest = rest * rest;
    int level1 = (int)(((1.0f - rest) * 0.6f + 0.4f) * 127.0f * p->engine68);
    if (!(level1 < 0x80))
        level1 = 0x7F;
    st->engineSnd[1] = (st->engineSnd[1] + level1) / 2;
#else
    int level1 = (int)((t * 3.0f + 1.0f) * 32.0f * p->engine68);
    if (!(level1 < 0x80))
        level1 = 0x7F;
    f32 d = t - 0.375f;
    if (d < 0.0f)
        d = -d;
    d = d + 0.375f;
    f32 k = 1.0f;
    if (st->clutchState != 1)               /* 1: locked */
        k = 0.5f;
    int level0 = (int)(d * k * 127.0f * p->engine64);
    if (!(level0 < 0x80))
        level0 = 0x7F;
    st->engineSnd[1] = (st->engineSnd[1] + level1) / 2;
    st->engineSnd[0] = (st->engineSnd[0] + level0) / 2;
#endif
    int level2 = (int)((f32)st->engineSnd[2] * p->engine6C);
    if (!(level2 < 0x80))
        level2 = 0x7F;
    st->engineSnd[2] = level2;
    st->blowOffLevel = st->blowOffLevel * p->engine70;
}

/* Callers: DynamicsConductor::update (every frame). */
// FUNC 0x001DFD90 car_crash_sound_clear
void car_crash_sound_clear(RaceCarState *car)
{
    car->crashLevel = 0.0f;
}

/* kind -1: nothing; 0 (a wall): force / 50000, 1 (a car): force x 2, 2 (a
 * course object): |force / 50|, else 0; at most 1, below 0.2 is 0.  Kept
 * (with kind, arg) when louder than the frame's level so far.  Callers:
 * car_report_crash, car_box_impact_feedback (T374). */
// FUNC 0x001DFD98 car_crash_sound_add
void car_crash_sound_add(RaceCarState *car, f32 force, int kind, int arg)
{
    if (kind == CRASH_KIND_NONE)
        return;
    f32 level = 0.0f;
    switch (kind) {
    case CRASH_KIND_WALL:
        level = force / 50000.0f;
        break;
    case CRASH_KIND_CAR:
        level = force + force;
        break;
    case CRASH_KIND_GADGET:
        level = force / 50.0f;
        if (level < 0.0f)
            level = -level;
        break;
    }
    if (1.0f < level)
        level = 1.0f;
    if (level < 0.2f)
        level = 0.0f;
    if (car->crashLevel < level) {
        car->crashLevel = level;
        car->crashKind = kind;
        car->crashArg = arg;
    }
}

/* Pitch 1, kind / arg of the loudest crash; returns its level x crashGain.
 * Callers: RaceEntryCar::updateSound (T121). */
// FUNC 0x001DFE70 car_crash_sound_get
f32 car_crash_sound_get(RaceCarState *car, f32 *pitch, int *kind, int *arg)
{
    if (pitch != NULL)
        *pitch = 1.0f;
    if (kind != NULL)
        *kind = car->crashKind;
    if (arg != NULL)
        *arg = car->crashArg;
    return car->crashLevel * g_0049CFD8.crashGain;
}

/* Tells the conductor (slot 40, issueCrashEvent; force stays in $f12), then
 * the crash sound.  Callers: F_001A10F8 (T167), car_box_respond_car (T375),
 * car_box_wall_vertex_impulse and car_box_wall_edge_impulse (T376). */
// FUNC 0x001DFEB8 car_report_crash
void car_report_crash(RaceCarState *car, int kind, int arg, f32 force)
{
    car->dc->issueCrashEvent(car, kind, arg, force);
    car_crash_sound_add(car, force, kind, arg);
}
