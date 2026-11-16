/* T377 - CameraTargetInfo (0x001FED38..0x001FF788).
 *
 * What a camera view looks at (CameraView +0x1A0): one car (mode 0) or the
 * midpoint of two (mode 1).  update() recomputes the point; when the target
 * changes (startBlend, from setCar / setPair) the point blends from the old
 * target to the new one over up to 60 frames along an ease curve (T379).
 * save() keeps a snapshot of the position, which OnBoardCamera uses to get
 * the target's speed (the position change per frame).
 *
 * Retail T377's .rodata (0x0033E008..0x0033E029; nothing references it:
 * DATA_START): "bad_alloc", t9UnitArena1Ui64.
 */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/camera/T377_camera_target_info.h"
#include "game/camera/onboard_camera_internal.h"
// DATA_START 0x0033E008

enum {
    TARGET_ONE_CAR  = 0,                    /* mode / fromMode */
    TARGET_TWO_CARS = 1,
    TARGET_NONE     = -1,                   /* fromMode before the first blend */
    BLEND_MAX_FRAMES = 60
};

// FUNC 0x001FED38 CameraTargetInfo::init
void CameraTargetInfo::init(RaceOrganization *o, int c)
{
    kind = 0;
    variant = 0;
    index[0] = c;
    mode = TARGET_ONE_CAR;
    car[0] = NULL;
    if (o != NULL)
        car[0] = o->entries->cars[c];
    org = o;
    savedIndex[0] = c;
    lap = 1;
    weight = 1.0f;
    fromMode = TARGET_NONE;
    saved = 0;
    changed = 0;
    wide = 0;
    /* one chain: retail stores one $f0 (mtc1 $zero), also curve.init's
     * arguments */
    yaw = pitch = roll = pos[0] = pos[1] = pos[2] = distance = lapDistance = 0.0f;
    blendLeft = 0;
    curve.init(0.0f, 0.0f, 3.0f);
}

// FUNC 0x001FEDF0 TargetPoint::clear
void TargetPoint::clear()
{
    lap = 1;
    x = y = z = distance = lapDistance = 0.0f;
}

/* the lap counter wraps at the course's replay loop count (1..n; 0 before
 * the start) */
// FUNC 0x001FEE18 getCarPoint
void getCarPoint(RaceCarState *s, RaceCourse *course, TargetPoint *out)
{
    int lap = s->st.lap;
    f32 lapLength;

    if (lap > 0)
        lap = (lap - 1) % course->getReplayLoopCount() + 1;
    else
        lap = 0;
    out->x = car_view_x(s, 0.0f);
    out->y = car_view_y(s, 0.0f);
    out->z = car_view_z(s, 0.0f);
    lapLength = course->getLapLength();
    out->distance = lapLength * (f32)lap + car_lap_distance(s, 0.0f);
    out->lapDistance = car_lap_distance(s, 0.0f);
    out->lap = lap;
}

/* the midpoint of a and b; the lap and the distances are a's */
// FUNC 0x001FEF10 getPairPoint
void getPairPoint(RaceCarState *a, RaceCarState *b, RaceCourse *course, TargetPoint *out)
{
    int lap = a->st.lap;
    f32 lapLength;
    f32 va;

    if (lap > 0)
        lap = (lap - 1) % course->getReplayLoopCount() + 1;
    else
        lap = 0;
    lapLength = course->getLapLength();
    out->distance = lapLength * (f32)lap + car_lap_distance(a, 0.0f);
    out->lapDistance = car_lap_distance(a, 0.0f);
    va = car_view_x(a, 0.0f);
    va += car_view_x(b, 0.0f);
    va *= 0.5f;
    out->x = va;
    va = car_view_y(a, 0.0f);
    va += car_view_y(b, 0.0f);
    va *= 0.5f;
    out->y = va;
    va = car_view_z(a, 0.0f);
    va += car_view_z(b, 0.0f);
    va *= 0.5f;
    out->z = va;
    out->lap = lap;
}

// FUNC 0x001FF068 CameraTargetInfo::update
void CameraTargetInfo::update(int advance)
{
    f32 newWeight = 0.0f;
    f32 carRoll = 0.0f;
    f32 carPitch = 0.0f;
    RaceCourse *course = org->course;
    f32 carYaw = 0.0f;
    TargetPoint cur;                        /* sp+0x00 blended */
    TargetPoint raw;                        /* sp+0x20 */
    TargetPoint from;                       /* sp+0x40 the blend's source */
    f32 keep;                               /* the part of the new target */

    cur.clear();
    keep = 1.0f;
    raw.clear();
    if (advance) {
        changed = 0;
        if (blendLeft > 0) {
            blendLeft = blendLeft - 1;
            if (blendLeft == 0)
                wide = 0;
        }
    }
    switch (mode) {
    case TARGET_ONE_CAR: {
        RaceCarState *s = car[0]->state;
        carYaw = car_yaw(s, 0.0f);
        carPitch = car_pitch(s, 0.0f);
        carRoll = car_roll(s, 0.0f);
        getCarPoint(s, course, &cur);
        break;
    }
    case TARGET_TWO_CARS:
        getPairPoint(car[0]->state, car[1]->state, course, &cur);
        break;
    }
    raw = cur;
    if (blendLeft > 0) {
        f32 t;

        /* a fromMode other than 0 / 1 leaves `from' unset (as the original does) */
        switch (fromMode) {
        case TARGET_ONE_CAR:
            getCarPoint(fromCar[0]->state, course, &from);
            break;
        case TARGET_TWO_CARS:
            getPairPoint(fromCar[0]->state, fromCar[1]->state, course, &from);
            break;
        }
        t = curve.eval((f32)blendLeft / (f32)blendTotal);
        keep = 1.0f - t;
        cur.x = cur.x * keep + from.x * t;
        cur.y = cur.y * keep + from.y * t;
        cur.z = cur.z * keep + from.z * t;
    }
    switch (mode) {
    case TARGET_ONE_CAR:
        newWeight = 1.0f;
        if (fromMode == TARGET_TWO_CARS)
            newWeight = keep;
        break;
    case TARGET_TWO_CARS:
        newWeight = 0.0f;
        if (fromMode == TARGET_ONE_CAR)
            newWeight = 1.0f - keep;
        break;
    }
    /* the points in TargetPoint order (retail loads them so) */
    pos[0] = cur.x;
    pos[1] = cur.y;
    pos[2] = cur.z;
    distance = cur.distance;
    lapDistance = cur.lapDistance;
    lap = cur.lap;
    yaw = carYaw;
    pitch = carPitch;
    roll = carRoll;
    weight = newWeight;
    rawPos[0] = raw.x;
    rawPos[1] = raw.y;
    rawPos[2] = raw.z;
    rawDistance = raw.distance;
    rawLapDistance = raw.lapDistance;
}

// FUNC 0x001FF380 CameraTargetInfo::save
void CameraTargetInfo::save()
{
    int i;

    saved = 1;
    for (i = 0; i < 7; i++)
        savedIndex[i] = index[i];
    savedMode = mode;
    savedPos[0] = pos[0];
    savedPos[1] = pos[1];
    savedPos[2] = pos[2];
}

// FUNC 0x001FF3D8 CameraTargetInfo::finishBlend
void CameraTargetInfo::finishBlend()
{
    blendLeft = 0;
    update(0);
}

/* the current target becomes the blend's source; frames clamped to 60
 * (blendTotal: 1 for a negative count, 0 for 0) */
// FUNC 0x001FF400 CameraTargetInfo::startBlend
void CameraTargetInfo::startBlend(int c, int frames)
{
    wide = 0;
    if (saved == 1 && index[0] != c) {
        changed = 1;                        /* retail stores the lh of saved (CSE: it is 1) */
        saved = 0;
    }
    switch (mode) {
    case TARGET_ONE_CAR:
        fromMode = TARGET_ONE_CAR;
        fromCar[0] = car[0];
        break;
    case TARGET_TWO_CARS:
        fromMode = mode;
        fromCar[0] = car[0];
        fromCar[1] = car[1];
        break;
    }
    if (!(frames < BLEND_MAX_FRAMES + 1))
        frames = BLEND_MAX_FRAMES;
    blendLeft = frames;
    if (!(frames > -1))
        frames = 1;
    blendTotal = frames;
}

// FUNC 0x001FF490 CameraTargetInfo::setCar
int CameraTargetInfo::setCar(int c, int frames)
{
    int wasBlending = blendLeft > 0;

    startBlend(c, frames);
    index[0] = c;
    mode = TARGET_ONE_CAR;
    car[0] = org->entries->cars[c];
    return wasBlending;
}

/* two cars out of range: car 0 alone (the count is compared signed) */
// FUNC 0x001FF4F8 CameraTargetInfo::setPair
int CameraTargetInfo::setPair(int a, int b, int frames)
{
    int wasBlending = blendLeft > 0;
    int count = org->entries->count;

    startBlend(a, frames);
    if (a >= count || b >= count) {
        setCar(0, frames);
        return wasBlending;
    }
    mode = TARGET_TWO_CARS;
    index[0] = a;
    index[1] = b;
    /* retail reloads org->entries after the car[0] store: car[] is a union
     * member (alias set 0, see the header) */
    car[0] = org->entries->cars[a];
    car[1] = org->entries->cars[b];
    return wasBlending;
}

// FUNC 0x001FF5D8 CameraTargetInfo::refresh
void CameraTargetInfo::refresh()
{
    update(0);
}

/* 1 - the blend's remaining fraction while a single car is followed */
// FUNC 0x001FF5F8 CameraTargetInfo::getSingleWeight
f32 CameraTargetInfo::getSingleWeight()
{
    f32 w = 1.0f;

    if (mode == TARGET_ONE_CAR && blendLeft > 0)
        w = 1.0f - (f32)blendLeft / (f32)blendTotal;
    return w;
}

// FUNC 0x001FF648 CameraTargetInfo::getSecondCar
int CameraTargetInfo::getSecondCar()
{
    int second = -1;

    if (mode == TARGET_TWO_CARS)
        second = index[1];
    return second;
}

// FUNC 0x001FF668 CameraTargetInfo::getSavedPosition
int CameraTargetInfo::getSavedPosition(f32 *x, f32 *y, f32 *z)
{
    if (saved != 1)
        return 0;
    *x = savedPos[0];
    *y = savedPos[1];
    *z = savedPos[2];
    return 1;
}

// FUNC 0x001FF698 CameraTargetInfo::getCarPosition
void CameraTargetInfo::getCarPosition(int k, f32 *x, f32 *y, f32 *z)
{
    RaceCarState *s = car[k]->state;

    *x = car_view_x(s, 0.0f);
    *y = car_view_y(s, 0.0f);
    *z = car_view_z(s, 0.0f);
}

/* no callers */
// FUNC 0x001FF728 CameraTargetInfo::isTarget
int CameraTargetInfo::isTarget(int c)
{
    int hit = 0;

    switch (mode) {
    case TARGET_ONE_CAR:
        hit = c == index[0];
        break;
    case TARGET_TWO_CARS:
        /* the 1s come from mode's register (CSE: mode is 1 here) */
        if (c == index[0])
            hit = 1;
        if (c == index[1])
            hit = 1;
        break;
    }
    return hit;
}
