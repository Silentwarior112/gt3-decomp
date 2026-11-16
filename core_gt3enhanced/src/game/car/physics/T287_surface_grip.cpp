/* T287 (0x001DE3B8..0x001DE5E0): the tyre sound of a wheel by surface
 * class.  The race sound code (RaceEntryCar::updateSound, T121) calls all
 * seven in turn: when the wheel's surface type is in the evaluator's class,
 * *volume = volumeScale x the wheel's squeal volume and *pitch = pitchScale
 * x its squeal pitch (tyre_sound_roll_0789: the rolling sound of car_sound_tyre_roll);
 * otherwise both are left alone.  The wheel is taken through a DynCarState
 * pointer (retail adds the index to car, then the wheel offset 0xFE4). */
#include "game/car/physics/T287_surface_grip.h"
#include "game/car/physics/car_param_internal.h"

// FUNC 0x001DE3B8 tyre_sound_squeal_0789
/* surfaces 0, 7, 8, 9 */
void tyre_sound_squeal_0789(RaceCarState *car, int wheel, f32 volumeScale, f32 pitchScale,
                f32 *volume, f32 *pitch)
{
    DynCarState *st = &car->st;
    DynWheel *w = &st->wheel[wheel];

    switch (w->surface) {
    case 0:
    case 7:
    case 8:
    case 9:
        *volume = volumeScale * w->squealVolume;
        *pitch = pitchScale * w->squealPitch;
        break;
    }
}

// FUNC 0x001DE410 tyre_sound_roll_0789
/* surfaces 0, 7, 8, 9: the rolling tyre sound */
void tyre_sound_roll_0789(RaceCarState *car, int wheel, f32 volumeScale, f32 pitchScale,
                f32 *volume, f32 *pitch)
{
    f32 rollPitch;

    switch (car->st.wheel[wheel].surface) {
    case 0:
    case 7:
    case 8:
    case 9:
        *volume = volumeScale * car_sound_tyre_roll(car, wheel, &rollPitch);
        *pitch = pitchScale * rollPitch;
        break;
    }
}

// FUNC 0x001DE4A0 tyre_sound_squeal_6_12_14
/* surfaces 6, 12, 14 */
void tyre_sound_squeal_6_12_14(RaceCarState *car, int wheel, f32 volumeScale, f32 pitchScale,
                f32 *volume, f32 *pitch)
{
    DynCarState *st = &car->st;
    DynWheel *w = &st->wheel[wheel];

    switch (w->surface) {
    case 6:
    case 12:
    case 14:
        *volume = volumeScale * w->squealVolume;
        *pitch = pitchScale * w->squealPitch;
        break;
    }
}

// FUNC 0x001DE508 tyre_sound_squeal_5
/* surface 5 */
void tyre_sound_squeal_5(RaceCarState *car, int wheel, f32 volumeScale, f32 pitchScale,
                f32 *volume, f32 *pitch)
{
    DynCarState *st = &car->st;
    DynWheel *w = &st->wheel[wheel];

    if (w->surface == 5) {
        *volume = volumeScale * w->squealVolume;
        *pitch = pitchScale * w->squealPitch;
    }
}

// FUNC 0x001DE548 tyre_sound_none
/* no surface class: empty */
void tyre_sound_none(RaceCarState *car, int wheel, f32 volumeScale, f32 pitchScale,
                f32 *volume, f32 *pitch)
{
}

// FUNC 0x001DE550 tyre_sound_squeal_1_13
/* surfaces 1, 13 */
void tyre_sound_squeal_1_13(RaceCarState *car, int wheel, f32 volumeScale, f32 pitchScale,
                f32 *volume, f32 *pitch)
{
    DynCarState *st = &car->st;
    DynWheel *w = &st->wheel[wheel];

    if (w->surface == 1 || w->surface == 13) {
        *volume = volumeScale * w->squealVolume;
        *pitch = pitchScale * w->squealPitch;
    }
}

// FUNC 0x001DE598 tyre_sound_squeal_2_4
/* surfaces 2, 3, 4 */
void tyre_sound_squeal_2_4(RaceCarState *car, int wheel, f32 volumeScale, f32 pitchScale,
                f32 *volume, f32 *pitch)
{
    DynCarState *st = &car->st;
    DynWheel *w = &st->wheel[wheel];

    switch (w->surface) {
    case 2:
    case 3:
    case 4:
        *volume = volumeScale * w->squealVolume;
        *pitch = pitchScale * w->squealPitch;
        break;
    }
}
