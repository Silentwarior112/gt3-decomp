/* T335 - the pad's big vibration motor: 0x001DF2B0..0x001DF4C0.
 * Called by the per-car step car_step (T302).
 *
 * Data (retail): .lit4 0x003502AC only; no .rodata ("bad_alloc" and
 * UnitArena<64>'s name at 0x0033C470 / 0x0033C498 are T334's and T288's):
 * no <new>, T153 without its type name. */
#include "game/car/physics/car_engine_internal.h"
#include "game/car/physics/T335_pad_vibration.h"

/* The largest of: the front wheels' slip x drive force and brake force
 * terms (each part at most 0.333), the impact shake st.impact x 8 + 0.25
 * when positive (the collision box's impact, held 24 frames at its peak by
 * car_box_impact_feedback, T374), and the sum over the wheels (none while
 * the car is held) of damperForce / 19600 (at most 1 each) plus, for the
 * front wheels on the kerbs (?) (surface 1), |load / 2940 x speedX / 64|
 * (at most 0.25 each).
 * x 256, at most 127, to vib.big.  ($f0 holds the product at the return; no
 * caller reads it.) */
// FUNC 0x001DF2B0 pad_vibration_update
void pad_vibration_update(RaceCarState *car)
{
    DynCarState *st = &car->st;
    f32 grip = 0.0f;
    f32 damper = 0.0f;
    f32 kerb = 0.0f;
    f32 shake = 0.0f;

    f32 drive = (st->wheel[0].slip * st->wheel[0].slip + st->wheel[1].slip * st->wheel[1].slip) * 0.5f + 0.1f;
    drive *= st->wheel[0].driveForce + st->wheel[1].driveForce;
    if (drive < 0.0f)
        drive = -drive;
    drive = drive * 0.00065039063f;
    if (0.333f < drive)
        drive = 0.333f;
    f32 brake = st->wheel[0].brakeForce + st->wheel[1].brakeForce;
    brake = brake / 19600.0f;
    brake = brake * brake;
    if (0.333f < brake)
        brake = 0.333f;
    f32 impact = st->impact;
    grip = drive + brake;
    if (0.0f < impact)
        shake = impact * 8.0f + 0.25f;

    u8 held = st->stopped;
    for (int i = 0; i < 4; i++) {
        DynWheel *w = &st->wheel[i];
        if (held != 0)
            continue;
        f32 v = w->damperForce;
        if (0.0f < v) {
            v = v / 19600.0f;
            if (1.0f < v)
                v = 1.0f;
            damper += v;
        }
        if (i < 2 && w->surface == SURFACE_KERB) {
            f32 k = (w->load / 2940.0f) * (st->speedX * 0.015625f);
            if (k < 0.0f)
                k = -k;
            if (0.25f < k)
                k = 0.25f;
            kerb += k;
        }
    }
    f32 level = damper + kerb;
    if (level < grip)
        level = grip;
    if (level < shake)
        level = shake;
    int n = (int)(level * 256.0f);
    if (!(n < 0x80))
        n = 0x7F;
    car->vib.big = n;
}
