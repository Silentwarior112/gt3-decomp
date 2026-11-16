/* T356 (0x001E2CC8..0x001E32D8): the brake assist.  Wheels are FL, FR, RL,
 * RR: side 0 the even wheels, side 1 the odd ones.  `inner' is side 0 when
 * the yaw rate is positive, else side 1; `outer' the other (? the inner and
 * outer wheels of the turn: so the aid brakes the outer wheels against an
 * oversteer and the inner ones against an understeer, as a stability control
 * does). */
/* Retail T356's .rodata (0x0033C638): "bad_alloc", then UnitArena<64>'s
 * type name (nothing references either: DATA_START); the original saw the
 * race headers (its code asks the conductor, DynamicsConductor::isDirtCourse). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/car/physics/car_input_internal.h"
#include "game/car/physics/T356_brake_assist.h"
// DATA_START 0x0033C638

// FUNC 0x001E2CC8 wheel_add_brake
void wheel_add_brake(CarWheelState *w, f32 v)
{
    w->brake += v;
    if (w->brake > 1.0f)
        w->brake = 1.0f;
}

/* != 0 when the brake (+0x30) or the ABS (+0x31) part of the block is on:
 * the two bytes read as one halfword (lhu). */
// FUNC 0x001E2CF8 tcs_brake_or_abs
int tcs_brake_or_abs(TcsParams *t)
{
    return *(u16 *)&t->brakeOn != 0;
}

/* slipLimit[4]: the wheels' ABS slip limits (car_brake_aids), scaled here by
 * the axle's brakeK; a released wheel's limit is scaled with its brake.  With `enable':
 *   off the brake pedal: the oversteer brakes the outer wheels (brakeGain /
 *   brakeSlip, the "working" assist), the understeer with the throttle off
 *   the inner ones (brakeGain2 / brakeSlip2);
 *   on the brake pedal: the oversteer releases the inner wheels' brakes,
 *   the understeer the outer ones' (1 - (slip - 0.1) x absGain(2)).
 * DynCarState.brakeAssist: 2 when the first kind worked, else 1 when the
 * block has the brake or the ABS part. */
// FUNC 0x001E2D08 tcs_brakes
void tcs_brakes(CarDynState *st, CarParams *p, TcsParams *t, f32 *slipLimit, int enable)
{
    int i;
    int j;
    int working;
    int inner;
    int outer;
    f32 under;
    f32 over;
    f32 v;
    f32 k;

    for (i = 0; i < 4; i++)
        slipLimit[i] *= p->brakeK[i >> 1];

    if (enable == 0)
        return;
    working = 0;

    inner = 1;
    under = st->understeer;
    over = st->oversteer;
    if (st->yawRate > 0.0f)
        inner = 0;
    outer = 1 - inner;

    if (st->brake == 0.0f) {
        if (t->brakeOn != 0) {
            v = (over - t->brakeSlip) * t->brakeGain;
            if (v > 0.0f) {
                working = 1;
                if (t->brakeFront != 0)
                    wheel_add_brake(&st->wheel[outer], v);
                if (t->brakeRear != 0)
                    wheel_add_brake(&st->wheel[outer + 2], v);
            }
        }
        if (st->throttle == 0.0f && t->brakeOn2 != 0) {
            v = (under - t->brakeSlip2) * t->brakeGain2;
            if (v > 0.0f) {
                if (t->brakeFront2 != 0)
                    wheel_add_brake(&st->wheel[inner], v);
                if (t->brakeRear2 != 0)
                    wheel_add_brake(&st->wheel[inner + 2], v);
            }
        }
    } else {
        if (t->absOn != 0) {
            k = 1.0f - (over - 0.1f) * t->absGain;
            if (k < 0.0f)
                k = 0.0f;
            if (k < 1.0f) {
                working = 1;
                if (t->absFront != 0) {
                    slipLimit[inner] *= k;
                    st->wheel[inner].brake *= k;
                }
                if (t->absRear != 0) {
                    slipLimit[inner + 2] *= k;
                    st->wheel[inner + 2].brake *= k;
                }
            }
        }
        if (t->absOn2 != 0) {
            k = 1.0f - (under - 0.1f) * t->absGain2;
            if (k < 0.0f)
                k = 0.0f;
            if (k < 1.0f) {
                if (t->absFront2 != 0) {
                    slipLimit[outer] *= k;
                    st->wheel[outer].brake *= k;
                }
                if (t->absRear2 != 0) {
                    slipLimit[outer + 2] *= k;
                    st->wheel[outer + 2].brake *= k;
                }
            }
        }
    }

    if (working != 0)
        st->brakeAssist = 2;
    else if (tcs_brake_or_abs(t) != 0)
        st->brakeAssist = 1;
}

/* The brake aids of a car with the block t: on a dirt course (course kind
 * 2) the brake balance with the steering (g_002D28B0 brakeAssistAngle /
 * Front / Rear), the brake assist (tcs_brakes), then the ABS of every
 * braked wheel: its brake x grip x absGain, cut as its slip passes the
 * tyre's positive peak slip (x absK x slipGrip), never to exactly 0
 * (0.001). */
// FUNC 0x001E3068 car_brake_aids
void car_brake_aids(RaceCarDynamics *car, TcsParams *t, int enable)
{
    CarParams   *p = &car->params;
    CarDynState *st = &car->st;
    f32 slipLimit[4];               /* sp+0x00 */
    int i;
    int j;

    if (car->dc->isDirtCourse() != 0 && g_002D28B0.brakeAssistAngle != 0) {
        f32 share = st->steer;          /* the steering's share of the full angle (one variable: retail's f8) */
        f32 fullAngle = (f32)g_002D28B0.brakeAssistAngle * DEG2RAD;
        f32 rearK, frontK;

        if (share < 0.0f)
            share = -share;
        if (fullAngle <= share)
            share = 1.0f;
        else
            share /= fullAngle;
        frontK = 1.0f - share * ((f32)g_002D28B0.brakeAssistFront * 0.01f);
        rearK = 1.0f - share * ((f32)g_002D28B0.brakeAssistRear * 0.01f);
        st->wheel[0].brake *= frontK;
        st->wheel[1].brake *= frontK;
        st->wheel[2].brake *= rearK;
        st->wheel[3].brake *= rearK;
    }

    /* (99.3%: &p->absGain / &p->absK, hoisted, swap $s5 / $s6 under this
     * compiler build; exact with 991111-01) */
    for (i = 0; i < 4; i++)
        slipLimit[i] = CarDynamics_getTyre(p, i, &st->wheel[i])->peakSlipPos;

    tcs_brakes(st, p, t, slipLimit, enable);

    for (j = 0; j < 4; j++) {
        CarWheelState *w = &st->wheel[j];
        f32 brake = w->brake;
        f32 gain = w->grip * p->absGain[j >> 1];
        f32 k = p->absK[j >> 1];

        if (brake != 0.0f) {
            f32 v;
            k *= w->slipGrip;
            v = gain * (1.0f - (w->slip - slipLimit[j]) * k);
            if (v < 0.0f)
                v = 0.0f;
            else if (v > 1.0f)
                v = 1.0f;
            v *= brake;
            if (v == 0.0f)
                v = 0.001f;
            w->brake = v;
        }
    }
}
