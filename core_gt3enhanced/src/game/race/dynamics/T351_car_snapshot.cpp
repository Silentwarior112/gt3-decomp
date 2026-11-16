/* T351 (0x001EE288..0x001EE638): a snapshot of a RaceCarState.  The car is
 * copied whole into / out of a byte buffer (0x16DC bytes, memcpy: ldl / ldr
 * pairs when either pointer is not 8-aligned, the last word lwl / lwr); a
 * restore keeps the car's four links and points the curves of its parameter
 * block (RaceCarState::params, CarDynamics) back into itself. */
#include "game/race/dynamics/dynamics_conductor_internal.h"
#include "game/race/dynamics/T351_car_snapshot.h"

// FUNC 0x001EE288 car_relink_curves
/* Each Curve gets its x / y pointing at the arrays that follow it in the
 * block: the engine's torque curve, steer0..2, the five curves of each of
 * the six TyreData (slipCurve2 shares slipCurve's x) and camberCurve[2], in
 * the block's order.  The engine through a pointer (retail stores its two
 * through engine + 0x138). */
void car_relink_curves(CarDynamics *params)
{
    u32 i;
    CarEngine *e = &params->engine;

    e->torqueCurve.x = e->rpm;
    e->torqueCurve.y = e->torque;
    params->steer0.x = params->steer0X;
    params->steer0.y = params->steer0Y;
    params->steer1.x = params->steer1X;
    params->steer1.y = params->steer1Y;
    params->steer2.x = params->steer2X;
    params->steer2.y = params->steer2Y;
    for (i = 0; i < 6; i++) {
        TyreData *t = &params->tyre[i];

        t->latCurve.x = t->latX;
        t->latCurve.y = t->latY;
        t->dragCurve.x = t->dragX;
        t->dragCurve.y = t->dragY;
        t->slipCurve.x = t->slipX;
        t->slipCurve.y = t->slipY;
        t->slipCurve2.x = t->slipX;         /* the same x array */
        t->slipCurve2.y = t->slipY2;
        t->loadCurve.x = t->loadX;
        t->loadCurve.y = t->loadY;
    }
    for (i = 0; i < 2; i++) {
        params->camberCurve[i].x = params->camberX[i];
        params->camberCurve[i].y = params->camberY[i];
    }
}

// FUNC 0x001EE3A0 car_links_save
/* Member by member: retail copies four words with lw / sw (a struct copy of
 * the 4-aligned record gives ldl / ldr pairs). */
void car_links_save(CarLinks *src, CarLinks *dst)
{
    dst->org = src->org;
    dst->dc = src->dc;
    dst->course = src->course;
    dst->input = src->input;
}

// FUNC 0x001EE3C8 car_links_restore
void car_links_restore(CarLinks *dst, CarLinks *src)
{
    dst->org = src->org;
    dst->dc = src->dc;
    dst->course = src->course;
    dst->input = src->input;
}

// FUNC 0x001EE3F0 car_copy
/* The car into a snapshot buffer (OneLapGhostInfo::car).  A byte copy: the
 * buffer's alignment is 1 (retail copies the last word with lwl / lwr; a
 * RaceCarState struct copy gives lw / sw). */
void car_copy(u8 *dst, const RaceCarState *src)
{
    /* The enhanced engine: GT3's 0x16DC bytes only, the snapshot's room
     * (OneLapGhostInfo::car); the fields after them are not snapshotted. */
    memcpy(dst, src, RACE_CAR_STATE_GT3_SIZE);
}

// FUNC 0x001EE4D8 car_restore
/* The snapshot over the car, keeping the car's links (org, dc, course,
 * input: the CarLinks at the head of RaceCarState), then the curves
 * relinked and the computer driver bound to the car again. */
void car_restore(RaceCarState *car, const u8 *snap)
{
    CarLinks links;

    car_links_save((CarLinks *)&car->org, &links);
    memcpy(car, snap, RACE_CAR_STATE_GT3_SIZE);    /* (the enhanced engine: GT3's bytes, see car_copy) */
    car_links_restore((CarLinks *)&car->org, &links);
    car_relink_curves(&car->params);
    ai_init(&car->ai, car->dc, car);
}
