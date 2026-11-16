/* T349 - the car-to-car pass of the conductor: 0x001EBB60..0x001EC4B0.
 *
 * dc_car_pairs (from DynamicsConductor::update, T317), for every pair of
 * cars that may meet (car_pair_info: both with a controller, the same
 * DYN_FLAG_COUNTED_GROUND, within 100 m and a car height + 5 m of each
 * other): each car's position in the other's frame goes to its computer
 * driver (ai_note_car, T365), a proximity code per ordered pair
 * (DynamicsConductor::near, read by CarPairStates::update), the closing
 * speed when one car runs into the back of the other within 20 m
 * (ai_note_closing), and a slipstream factor per ordered pair
 * (DynamicsConductor::slip); a car's slipTarget is the sum over the others,
 * at most 1. */
#include "game/car/physics/car_motion_internal.h"
#include "game/race/dynamics/T349_car_pairs.h"

// FUNC 0x001EBB60 dc_set_near
void dc_set_near(DynamicsConductor *dc, int car, int other, int code)
{
    dc->near[car][other] = code;
}

/* Also from CarPairStates::update (T360). */
// FUNC 0x001EBB80 dc_get_near
u8 dc_get_near(DynamicsConductor *dc, int car, int other)
{
    return dc->near[car][other];
}

/* From DynamicsConductor::virtual_21. */
// FUNC 0x001EBBA0 dc_clear_near
void dc_clear_near(DynamicsConductor *dc)
{
    memset(dc->near, 0, sizeof(dc->near));
}

/* 0 when the pair cannot meet; else the distance and each car's position
 * in the other's frame (forward / lateral rows of rot). */
// FUNC 0x001EBBC8 car_pair_info
int car_pair_info(CarPairInfo *out, RaceCarState *a, RaceCarState *b)
{
    DynCarState *sa = &a->st;
    DynCarState *sb = &b->st;
    f32 d[3];                                   /* b - a, world axes */
    int i;

    if (car_no_controller(a))
        return 0;
    if (car_no_controller(b))
        return 0;
    if ((sa->flags & DYN_FLAG_COUNTED_GROUND) != (sb->flags & DYN_FLAG_COUNTED_GROUND))
        return 0;
    for (i = 0; i < 3; i++)
        d[i] = sb->pos[i] - sa->pos[i];
    f32 dist = boot_sqrtf_v(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    out->dist = dist;
    if (100.0f < dist)
        return 0;
    f32 heightB = b->params.height;
    f32 heightA = a->params.height;
    if (d[2] < -(heightB + 5.0f) || heightA + 5.0f < d[2])
        return 0;
    out->aFwd = out->aSide = out->bFwd = out->bSide = 0.0f;
    for (i = 0; i < 3; i++) {
        f32 v = d[i];
        out->aFwd = out->aFwd + sa->rot[0][i] * v;
        out->aSide = out->aSide + sa->rot[1][i] * v;
        out->bFwd = out->bFwd - sb->rot[0][i] * v;
        out->bSide = out->bSide - sb->rot[1][i] * v;
    }
    return 1;
}

/* b at (fwd, side) in a's frame.  For a car not driven by the pad or not
 * in CTRL_DRIVER: 1 when b is ahead with a gap (fwd - b's rear length
 * - a's front length > 0), within the mean width sideways, and a closes on
 * it (the relative velocity, each scaled by its car's time scale, along
 * a's forward row is < 0); *gap, *closing. */
// FUNC 0x001EBDA0 car_pair_contact
int car_pair_contact(RaceCarState *a, RaceCarState *b, f32 *gap, f32 *closing, f32 fwd, f32 side)
{
    DynCarState *sb = &b->st;
    CarParams *pb = &b->params;
    DynCarState *sa = &a->st;
    f32 lenRear = pb->lenRear;
    f32 lenFront = a->params.lenFront;
    f32 widthA = a->params.width;
    f32 widthB = pb->width;

    *gap = 0.0f;
    *closing = 0.0f;
    if (sa->driver == DRIVER_PAD && sa->ctrlMode == CTRL_DRIVER)
        return 0;
    f32 g = fwd - lenRear - lenFront;
    if (g <= 0.0f)
        return 0;
    f32 halfWidth = (widthA + widthB) * 0.5f;
    if (halfWidth < side)
        return 0;
    if (side < -halfWidth)
        return 0;
    f32 relSpeed = 0.0f;
    for (int i = 0; i < 3; i++)
        relSpeed += sa->rot[0][i] * (sb->timeScale * sb->vel[i] - sa->timeScale * sa->vel[i]);
    if (0.0f <= relSpeed)
        return 0;
    *gap = g;
    *closing = -relSpeed;
    return 1;
}

// FUNC 0x001EBE98 dc_set_slip
void dc_set_slip(DynamicsConductor *dc, int car, int other, f32 v)
{
    dc->slip[car][other] = v;
}

/* Also from CarPairStates::update (T360). */
// FUNC 0x001EBEB8 dc_get_slip
f32 dc_get_slip(DynamicsConductor *dc, int car, int other)
{
    return dc->slip[car][other];
}

/* From DynamicsConductor::virtual_21. */
// FUNC 0x001EBED8 dc_clear_slip
void dc_clear_slip(DynamicsConductor *dc)
{
    memset(dc->slip, 0, sizeof(dc->slip));
}

/* a at (fwd, side) in b's frame.  When b runs faster than 100 km/h and a
 * is behind it by less than b's speed in metres and within b's width
 * sideways: (speed + fwd) / speed, 1 right behind b.  a is not read. */
// FUNC 0x001EBF00 car_slipstream
f32 car_slipstream(RaceCarState *a, RaceCarState *b, f32 fwd, f32 side)
{
    f32 speed = b->st.speedX;
    f32 factor = 0.0f;
    f32 width = b->params.width;

    if (KMH_100 < speed && fwd < 0.0f && -speed < fwd && -width < side && side < width)
        factor = (speed + fwd) / speed;
    return factor;
}

/* 0 beyond 50 m; else a longitudinal zone in bits 0..3 (6 far ahead ..
 * 1 far behind) and a lateral zone in bits 4..6 (1 far to one side .. 7 far
 * to the other side), from the gaps and the width. */
// FUNC 0x001EBF68 car_near_code
int car_near_code(f32 dist, f32 fwd, f32 side, f32 frontGap, f32 rearGap, f32 width)
{
    int zone;

    if (50.0f < dist)
        return 0;
    if (frontGap * 4.0f < fwd)
        zone = 6;
    else if (frontGap < fwd)
        zone = 5;
    else if (0.0f < fwd)
        zone = 4;
    else if (-rearGap <= fwd)
        zone = 3;
    else if (!(-rearGap * 4.0f <= fwd))     /* retail: c.le.s; bc1t over zone = 1 (zone = 2 in the slot) */
        zone = 1;
    else
        zone = 2;

    if (side < -width * 4.0f)
        return zone | 0x10;
    if (side < -width)
        return zone | 0x20;
    if (side < -width * 0.5f)
        return zone | 0x30;
    if (side <= width * 0.5f)
        return zone | 0x40;
    if (side <= width)
        return zone | 0x50;
    if (side <= width * 4.0f)
        return zone | 0x60;
    return zone | 0x70;
}

// FUNC 0x001EC098 dc_car_pairs
void dc_car_pairs(DynamicsConductor *dc)
{
    int i;

    for (i = 0; i < dc->carCount; i++)
        ai_clear_traffic(&dc->getCar(i)->ai);

    for (i = 0; i < dc->carCount; i++) {
        for (int j = i + 1; j < dc->carCount; j++) {
            RaceCarState *a = dc->getCar(i);
            RaceCarState *b = dc->getCar(j);
            CarPairInfo pair;
            if (car_pair_info(&pair, a, b) == 0) {
                dc_set_slip(dc, i, j, 0.0f);
                dc_set_slip(dc, j, i, 0.0f);
                dc_set_near(dc, i, j, 0);
                dc_set_near(dc, j, i, 0);
                continue;
            }
            CarParams *pa = &a->params;
            CarParams *pb = &b->params;
            f32 frontA = pa->lenFront;
            f32 rearA = pa->lenRear;
            f32 frontB = pb->lenFront;
            f32 rearB = pb->lenRear;
            f32 width = (pa->width + pb->width) * 0.5f;
            f32 frontGap = frontA + rearB;
            f32 rearGap = rearA + frontB;
            ai_note_car(&a->ai, pair.dist, pair.aFwd, pair.aSide, frontGap, rearGap, width);
            ai_note_car(&b->ai, pair.dist, pair.bFwd, pair.bSide, rearGap, frontGap, width);
            dc_set_near(dc, i, j, car_near_code(pair.dist, pair.aFwd, pair.aSide, frontGap, rearGap, width));
            dc_set_near(dc, j, i, car_near_code(pair.dist, pair.bFwd, pair.bSide, rearGap, frontGap, width));
            if (pair.dist < 20.0f) {
                f32 gap, closing;
                if (car_pair_contact(a, b, &gap, &closing, pair.aFwd, pair.aSide))
                    ai_note_closing(&a->ai, gap, closing);
                if (car_pair_contact(b, a, &gap, &closing, pair.bFwd, pair.bSide))
                    ai_note_closing(&b->ai, gap, closing);
            }
            dc_set_slip(dc, i, j, car_slipstream(a, b, pair.bFwd, pair.bSide));
            dc_set_slip(dc, j, i, car_slipstream(b, a, pair.aFwd, pair.aSide));
        }
    }

    for (i = 0; i < dc->carCount; i++) {
        f32 sum = 0.0f;
        for (int j = 0; j < dc->carCount; j++)
            sum += dc_get_slip(dc, i, j);
        if (1.0f < sum)
            sum = 1.0f;
        dc->getCar(i)->st.slipTarget = sum;
    }
}
