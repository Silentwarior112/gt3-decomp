/* T398 - DesignerCamera (0x00201418..0x00202AC0).
 *
 * The course ("designer") cameras of the replay.  Set up (setupOnRecord) with
 * a record of the course's camera file: the record covers [start, end] of
 * the course; its parameter (0..1) is the target's place in it (or the time
 * since the cut when the record has a duration), and the record's tables
 * give the camera position, the look-at point and the zoom at it.
 * Coordinate mode 3 records (and a car in the pit lane) use the
 * OnBoardCamera base instead.  The yaw and pitch towards the target go
 * through two CameraRotDampers, the zoom through a CameraFollow; a history
 * of 128 frames of yaw / zoom is kept.
 *
 * Retail T398's .rodata opens with <new>'s "bad_alloc" (0x0033E3C0, nothing
 * references it: DATA_START) and ends with the type names 14DesignerCamera,
 * t9UnitArena1Ui64: UnitArena<64> (T153) defined before the file's class.
 */
// DATA_START 0x0033E3C0
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, as retail */
#include "game/race/core/T153_race_course.h"          /* first: the name after the file's own */
#include "game/camera/cameras_internal.h"
#include "game/camera/T398_designer_camera.h"

/* RACE_EVENT_ERROR (T122): an error message, arg the string. */

/* The absolute values of the listings are in double (f64 locals: dpcmp
 * with 0, dpsub from 0; the 0.0 is kept in a register for both), as
 * CarTurntable::update's (T106). */

/* CameraRotDamper::getSpeed() (T379) and CameraTargetInfo::getPosition()
 * (T377) are inline accessors: retail keeps &yawDamper in a register for the
 * first (the inline's `this') and computes the three addresses of the second
 * before its loads, passing them on to getCarPosition (the inline's
 * arguments).  The stand-ins designer_damper_speed / designer_target_pos
 * gave the same code (u089 / u091 revisit, 2026-10-10). */

/* The parameter of course position pos in [rec->start, rec->end], clamped
 * to 0..1; *clamped = 1 when it had to be clamped. */
// FUNC 0x00201418 designer_record_param
static f32 designer_record_param(CourseCameraRecord *rec, int *clamped, f32 pos)
{
    f32 param = (pos - rec->start) / (rec->end - rec->start);

    *clamped = 0;
    if (param < 0.0f) {
        param = 0.0f;
        *clamped = 1;
    }
    if (1.0f < param) {
        param = 1.0f;
        *clamped = 1;
    }
    return param;
}

// FUNC 0x00201480 DesignerCamera::DesignerCamera
DesignerCamera::DesignerCamera()
{
    /* implicit: OnBoardCamera, ~OnBoardCamera, CameraRotDamper, ~CameraRotDamper
     * (compiler-generated: bases, members, deleting form) */
    /* Called from CameraView::CameraView (0x00207768, T383).
     * OnBoardCamera::OnBoardCamera(), the vtable, the members (yawDamper and
     * pitchDamper: CameraRotDamper::CameraRotDamper; sway: its Random's
     * inline 0x87654321 into +0xA38), then the inline setType(1).  The
     * landing pads destroy the dampers (CameraRotDamper::~CameraRotDamper)
     * and the base (OnBoardCamera::~OnBoardCamera). */
    setType(1);
}

// FUNC 0x00201530 DesignerCamera::~DesignerCamera
DesignerCamera::~DesignerCamera()
{
    /* implicit: ~OnBoardCamera, ~CameraRotDamper (compiler-generated: bases, members, deleting form) */
    /* the vtable; the members are destroyed (pitchDamper, then yawDamper:
     * CameraRotDamper::~CameraRotDamper with flags 2), then
     * OnBoardCamera::~OnBoardCamera gets the flags */
}

/* Set up for a record: OnBoardCamera::virtual_8 first (the registers are
 * passed through; it uses the target only).  A coordinate mode 3 record
 * sets up the onboard mount (the mount buttons off). */
// FUNC 0x00201590 DesignerCamera::setupOnRecord
void DesignerCamera::setupOnRecord(CameraTargetInfo *target, int innerLapOffset, RaceCourse *course,
                                CourseCameraRecord *data, int number)
{
    int i;

    OnBoardCamera::virtual_8(target);
    zoom[0] = 1.4281480f;                   /* 1 / tan(35 degrees) */
    gamma[0] = 1.0f;
    this->innerLapOffset = innerLapOffset;
    counter = 0;
    subCounter = 0;
    startLap = target->lap;
    giveupMode = 180;
    t = 0.0f;
    cameraData = data;
    this->course = course;
    giveupState = 0;
    rangeOver = 0;
    movingBack = 0;
    forcedOnboard = 0;
    initZoomSet = 0;
    initRotOffsetSet = 0;
    initRotPitch = 0.0f;
    initRotYaw = 0.0f;
    giveupTime = 180;
    targetChangeOk = 1;
    targetChangeTimer = 120;
    initZoom = 1.0f;
    bankAnim.reset(t);
    cameraNumber = number;
    for (i = 0; i < 128; i++) {
        history[i].targetYaw = 0.0f;
        history[i].zoom = 0.0f;
        history[i].yawError = 0.0f;
        history[i].yaw = 0.0f;
        history[i].yawSpeed = 0.0f;
    }
    if (course != NULL && data != NULL) {
        CourseCameraRecord *rec = cameraData;

        flags = rec->flags;
        coordMode = rec->flags & CAMREC_COORD_MASK;
        if (coordMode == CAMREC_COORD_ONBOARD) {
            OnBoardCamera::virtual_8(target);
            setMount(rec->mount);
            changeable = 0;
        }
    }
}

/* Switch to the onboard camera (callers: update for the pit lane,
 * CameraManager::updateDesigner / updateMode2 / updateMode1). */
// FUNC 0x00201708 DesignerCamera::setForceOnboard
void DesignerCamera::setForceOnboard(CameraTargetInfo *target)
{
    OnBoardCamera::virtual_8(target);
    coordMode = CAMREC_COORD_ONBOARD;
    flags |= CAMREC_ZOOM_BY_DIST;           /* (the record's zoom is not used then) */
    forcedOnboard = 1;
    setTrueType(1);
    changeable = 0;
    type = 0;
    giveupTime = 180;
}

/* The target's course position for the record: with innerLapOffset < 0
 * the total position (plus the replay cameras' loop length while the
 * target is still on an earlier lap than at set up), else the position in
 * the lap plus innerLapOffset laps (plus one lap once the lap changed). */
// FUNC 0x00201760 DesignerCamera::coursePosition
f32 DesignerCamera::coursePosition(CameraTargetInfo *target)
{
    f32 extra = 0.0f;
    RaceCourse *c = course;
    int lap = target->lap;

    if (innerLapOffset < 0) {
        if (lap < startLap) {
            int loops = c->getReplayLoopCount();

            extra = (f32)loops * c->getLapLength();
        }
        return target->distance + extra;
    }
    if (startLap != lap)
        extra = c->getLapLength();
    {
        f32 pos = target->lapDistance;

        pos = pos + (f32)innerLapOffset * c->getLapLength();
        return pos + extra;
    }
}

/* The update.  Returns 0 on the first call after set up, else 1.
 *
 * Matching (u090): one variable `param' for the parameter in the record and
 * then the zoom (retail's $f23 for both); dx/dy/dz reused for the vector to
 * the saved point; the mode, kind and final coordMode tests are switches
 * (retail's case trees, the kind one a jump table for 0..5).  99.5%: the
 * reload registers of the spilled notFirst / onCar ($v1 / $a1 against
 * retail's $v0, $a1, $a2) are left, the same with 991111-01; statement
 * orders around them and the history loop's forms change nothing. */
// FUNC 0x00201830 DesignerCamera::update
int DesignerCamera::update(RaceOrganization *org, RaceInput *input, CameraTargetInfo *target,
                               CarProgress *progress)
{
    int rangeOverNow = 0;                   /* s0 */
    u8 behindNow = 0;                       /* s1 */
    f32 param;                              /* f23, then the zoom (one variable) */
    int clamped;                            /* sp+0x00, designer_record_param's flag */
    int notFirst;                           /* sp+0x40 */
    f32 bank;                               /* sp+0x44 */
    f32 zoomGain;                           /* sp+0x48 the zoom follower's gain */
    int onCar;                              /* sp+0x4C coordinate mode 1 */
    CourseCameraRecord *rec;                /* s5 */
    int noAim;                              /* s6 !(flags & CAMREC_AIM) */
    int carPoint;                           /* fp flags & CAMREC_CAR_POINT */
    int i;
    u16 effectBits;

    zoomGain = target->getSingleWeight();
    notFirst = 1;
    onCar = 0;
    clamped = 0;
    if (counter == 0) {
        notFirst = 0;
        counter = 1;
    }
    rec = cameraData;
    subCounter = subCounter + 1;
    for (i = 127; i > 0; i--)
        history[i] = history[i - 1];
    noAim = (flags & CAMREC_AIM) == 0;
    carPoint = flags >> 10;                 /* CAMREC_CAR_POINT */
    carPoint &= 1;
    pitForced = 0;
    if (coordMode != CAMREC_COORD_ONBOARD) {
        if (car_in_pit_lane(target->car[0]->state)) {
            setForceOnboard(target);
            doppler = 0;
            pitForced = 1;
        }
    }

    switch (coordMode) {
    case CAMREC_COORD_ONBOARD:
    default:
        /* onboard (2, 3, and below 0): the base camera, the parameter only
         * for the range */
        OnBoardCamera::update(org, input, target, progress);
        param = designer_record_param(rec, &clamped, coursePosition(target));
        rangeOver = (u8)clamped;
        t = param;
        if (!(flags & CAMREC_ZOOM_BY_DIST))
            zoom[0] = interpolate_table(rec->zoom, param);
        break;

    case CAMREC_COORD_CAR:
        onCar = 1;
        /* fall through */
    case CAMREC_COORD_COURSE: {
        f32 lookX, lookY, lookZ;            /* sp+0x04.. the look-at point */
        f32 camX, camY, camZ;               /* f31, f30, f29 the camera */
        f32 dirX, dirY, dirZ;
        f32 pitch, yaw;                     /* f28, f27 */
        f32 targetDist, savedDist;          /* f26, f22 */
        f32 savedX, savedY, savedZ;         /* sp+0x10.. */
        f32 rate;                           /* f24 30 / zoom: the dampers' range */
        int hardCut;                        /* s2 */
        int pitchMode;                      /* s0 */
        int intrWasOn;

        if (!(flags & CAMREC_AIM)) {
            f32 duration = rec->duration;
            f32 frames = (f32)subCounter;

            param = designer_record_param(rec, &clamped, coursePosition(target));
            if (1.0f <= duration) {
                /* by time: 60 frames a second */
                param = frames / (duration * 60.0f);
                if (param < 0.0f) {
                    param = 0.0f;
                    rangeOverNow = 1;
                }
                if (1.0f < param) {
                    param = 1.0f;
                    rangeOverNow = 1;
                }
            } else {
                rangeOverNow = clamped;
            }
            lookX = interpolate_table(rec->target[0], param);
            lookY = interpolate_table(rec->target[1], param);
            lookZ = interpolate_table(rec->target[2], param);
        } else {
            switch (target->mode) {
            case 0:
            case 1:
                /* one car or the pair: the target's (blended) point */
                target->getPosition(&lookX, &lookY, &lookZ);
                if (carPoint)
                    target->getCarPosition(0, &lookX, &lookY, &lookZ);
                behindNow = 1;
                lookY = lookY + 0.4f;
                param = designer_record_param(rec, &clamped, coursePosition(target));
                rangeOverNow = clamped;
                /* movingBack: the car went back along the lap this frame */
                if (!(progress->delta(target->index[0]) < 0.0f))
                    behindNow = 0;
                break;
            default:
                lookX = lookY = lookZ = 0.0f;
                param = 0.0f;
                break;
            }
        }
        if (param < 0.0f) {
            rangeOverNow = 1;
            param = 0.0f;
        }
        if (1.0f < param) {
            param = 1.0f;
            rangeOverNow = 1;
        }
        rangeOver = (u8)rangeOverNow;
        movingBack = behindNow;
        t = param;
        camX = interpolate_table(rec->pos[0], param);
        camY = interpolate_table(rec->pos[1], param);
        camZ = interpolate_table(rec->pos[2], param);
        dirX = lookX - camX;
        dirY = lookY - camY;
        dirZ = lookZ - camZ;
        pitch = camera_pitch_deg(dirX, dirY, dirZ);
        yaw = camera_yaw_deg(dirX, dirY, dirZ);
        bank = bankForcibly + 0.0f;
        if (!noAim)
            bank = bank + bankAnim.update();
        history[0].targetYaw = yaw;
        targetDist = boot_sqrtf_v(dirX * dirX + dirY * dirY + dirZ * dirZ);
        targetChangeOk = 1;
        if (targetChangeTimer < 120) {
            targetChangeOk = 0;
            targetChangeTimer = targetChangeTimer + 1;
        }
        savedDist = targetDist;
        if (target->getSavedPosition(&savedX, &savedY, &savedZ)) {
            f32 toSavedX = savedX - camX;
            f32 toSavedY = savedY - camY;
            f32 toSavedZ = savedZ - camZ;

            savedDist = boot_sqrtf(toSavedX * toSavedX + toSavedY * toSavedY + toSavedZ * toSavedZ);
            if (targetDist < savedDist) {
                /* the target came nearer than the saved point: a change is
                 * allowed again */
                targetChangeOk = 1;
                targetChangeTimer = 0;
            }
        }
        if (flags & CAMREC_ZOOM_BY_DIST)
            param = targetDist * interpolate_table(rec->zoom, 0.0f);
        else
            param = interpolate_table(rec->zoom, param);

        if (!noAim) {
            switch (target->kind) {
            case 1:
            case 2:
                if (target->mode == 1 && !carPoint) {
                    /* two cars: widen the view to hold both */
                    f32 carX, carY, carZ;   /* sp+0x1C.. */
                    f32 yaw0, spread, depth, margin, fov;

                    target->getCarPosition(0, &carX, &carY, &carZ);
                    yaw0 = camera_yaw_deg(carX - camX, carY - camY, carZ - camZ);
                    target->getCarPosition(1, &carX, &carY, &carZ);
                    /* the angle between the two cars, 10% more, in double */
                    {
                        f64 d = angle_diff(yaw0, camera_yaw_deg(carX - camX, carY - camY, carZ - camZ));

                        if (d < 0.0)
                            d = 0.0 - d;
                        spread = d * 1.1f;
                    }
                    depth = targetDist;
                    if (targetDist < savedDist) {
                        depth = targetDist - (savedDist - targetDist) * 60.0f;
                        if (depth < 1.0f)
                            depth = 1.0f;
                    }
                    margin = camera_yaw_deg(5.0f, 0.0f, -depth);
                    {
                        f64 s = spread;
                        f64 m = margin;

                        if (m < 0.0)
                            m = 0.0 - m;
                        spread = s + m;
                    }
                    param = param * 0.5f;
                    fov = 90.0f / param;
                    if (fov < spread) {
                        param = 90.0f / spread;
                        fov = 90.0f / param;
                    }
                    if (70.0f < fov)
                        param = 1.2857143f;    /* 90 / 70 */
                }
                break;
            case 3: {
                f32 fov = 90.0f / param;

                param = 90.0f / (fov + fov + 5.0f);
                fov = 90.0f / param;
                if (70.0f < fov)
                    param = 1.2857143f;        /* 90 / 70 */
                break;
            }
            case 0:
            case 4:
            case 5:
                break;
            }
        }
        {
            f32 zoomMax = rec->zoomMax;
            f32 zoomMin;

            if (zoomMax < param)
                param = zoomMax;
            zoomMin = rec->zoomMin;
            if (param < zoomMin)
                param = zoomMin;
            if (zoomMax < zoomMin)
                org->eventQueue.post(RACE_EVENT_ERROR, (int)"ERROR : zoom min-max value", 0);
        }
        hardCut = 0;
        rate = 30.0f / param;
        pitchMode = 1;
        if (target->kind == 3 || target->kind == 5)
            hardCut = 1;
        if (noAim)
            pitchMode = 0;
        if (notFirst) {
            /* not the first frame: damped */
            f32 yawSpeed;

            param = zoomDamper.update(param, zoomGain);
            rate = 30.0f / param;
            if (!noAim) {
                yaw = yawDamper.update(yaw, rate, 1.0f, hardCut);
                pitch = pitchDamper.update(pitch, rate, 1.0f, hardCut);
            }
            yawSpeed = yawDamper.getSpeed();
            if (30.0f <= yawSpeed)
                yawSpeed = 30.0f;
            if (yawSpeed <= -30.0f)
                yawSpeed = -30.0f;
            sway.update(yawSpeed / 30.0f);
            if (5.0f < rate)
                rate = 5.0f;
            if (!hardCut && !noAim)
                pitch = pitch + sway.get() / 3.0f * rate * yawSpeed / 30.0f;
        } else {
            /* the first frame: the dampers start on the target, coming
             * from the saved point (the last target) */
            f32 fromPitch = pitch;          /* f25 */
            f32 fromYaw = yaw;              /* f21 */

            f32 lastX, lastY, lastZ;        /* sp+0x28.. */

            if (!carPoint && !noAim && target->getSavedPosition(&lastX, &lastY, &lastZ)) {
                dirX = lastX - camX;
                dirY = lastY - camY;
                dirZ = lastZ - camZ;
                fromPitch = camera_pitch_deg(dirX, dirY, dirZ);
                fromYaw = camera_yaw_deg(dirX, dirY, dirZ);
            }
            if (!noAim && initRotOffsetSet) {
                fromPitch = fromPitch + initRotPitch;
                fromYaw = fromYaw + initRotYaw;
            }
            sway.init();
            yaw = yawDamper.start(yaw, fromYaw, rate, 0, 1.0f);
            pitch = pitchDamper.start(pitch, fromPitch, rate, pitchMode, 1.0f);
            if (initZoomSet)
                param = initZoom;
            zoomDamper.reset(param, zoomGain);     /* gain: not read */
        }
        targetLength[0] = targetDist;
        zoom[0] = param;
        history[0].zoom = param;
        gamma[0] = 1.0f;
        /* implicit: (the double arithmetic of this function is fp-bit soft-float:
         * fptodp, dpcmp, dpsub, dpmul, dpadd, dptofp) */
        {
            f64 d = angle_diff(yaw, history[0].targetYaw);

            if (d < 0.0)
                d = 0.0 - d;
            history[0].yawError = d;
        }
        history[0].yaw = yaw;
        history[0].yawSpeed = yawDamper.getSpeed();

        intrWasOn = intr_suspend();
        pgl_push_matrix();
        vu0_load_identity_regs();
        if (onCar) {
            /* mode 1: on top of the onboard view */
            OnBoardCamera::update(org, input, target, progress);
            pgl_load_matrix(matrix[0]);
        }
        vu0_rotate_z(bank);
        vu0_rotate_x(pitch);
        vu0_rotate_y(yaw);
        pgl_translate(-camX, -camY, -camZ);
        pgl_get_matrix(matrix[0]);
        pgl_pop_matrix();
        intr_resume(intrWasOn);
        break;
    }
    }

    effectBits = 0;
    if (0.0f < rec->effect[0])
        effectBits = CAMERA_EFFECT_0;
    if (0.0f < rec->effect[1])
        effectBits |= CAMERA_EFFECT_1;
    if (0.0f < rec->effect[2])
        effectBits |= CAMERA_EFFECT_2;
    effect = (s16)effectBits;
    switch (coordMode) {
    case CAMREC_COORD_COURSE:
        break;
    case CAMREC_COORD_CAR:
    case CAMREC_COORD_ONBOARD:
        if (target->changed != 0)
            doppler = 0;
        break;
    }
    return notFirst;
}

// FUNC 0x00202430 DesignerCamera::applyMountView
void DesignerCamera::applyMountView(RaceEntryCar *car)
{
    OnBoardCamera::applyMountView(car);
}

// FUNC 0x00202450 DesignerCamera::virtual_9
int DesignerCamera::virtual_9()
{
    int r = 3;

    if (coordMode == CAMREC_COORD_ONBOARD)
        r = OnBoardCamera::virtual_9();
    return r;
}

// FUNC 0x00202488 DesignerCamera::getEffect
int DesignerCamera::getEffect()
{
    return effect;
}

// FUNC 0x00202490 DesignerCamera::getFadeOutTime
f32 DesignerCamera::getFadeOutTime()
{
    CourseCameraRecord *rec = cameraData;
    f32 r = 0.0f;

    if (rec != NULL) {
        r = 1.0f;
        if (coordMode != CAMREC_COORD_ONBOARD)
            r = rec->fadeOutTime;
    }
    return r;
}

// FUNC 0x002024C0 DesignerCamera::getMount
int DesignerCamera::getMount()
{
    int r = -1;

    if (coordMode == CAMREC_COORD_ONBOARD)
        r = mount;
    return r;
}

// FUNC 0x002024E0 DesignerCamera::isTargetDistEnable
int DesignerCamera::isTargetDistEnable()
{
    return coordMode != CAMREC_COORD_ONBOARD;
}

/* no callers */
// FUNC 0x002024F0 DesignerCamera::isCourseCoord
int DesignerCamera::isCourseCoord()
{
    return coordMode == CAMREC_COORD_COURSE;
}

/* The change level of getChangeLevel (0 stay, 1 may change, 2 change):
 * levels 0, 3, 5 only follow the range; otherwise, for a CAMREC_CAR_POINT
 * record that is not forced onboard, the end of the range the car runs to
 * (the parameter past 0.9, or under 0.1 when it goes backwards: movingBack)
 * also offers a change. */
// FUNC 0x00202500 designer_change_level
static int designer_change_level(int carPoint, int over, f32 param, int level, int isBehind, int forced)
{
    int r = 0;

    switch (level) {
    case 0:
    case 3:
    case 5:
        r = over ? 2 : 0;
        break;
    default:
        if (carPoint != 0 && forced == 0) {
            if (isBehind) {
                if (param < 0.1f)
                    r = 1;
            } else if (0.9f < param)
                r = 1;
            if (over)
                r = 2;
        } else
            r = over != 0;
        break;
    }
    return r;
}

/* By giveupState (set with VariableCameraBase::setGiveup): 0: the level
 * of designer_change_level; 1: 2 once giveupMode ran out, 0 before; 2: a
 * CAMREC_CAR_POINT record that is not forced onboard as 0 (not forced),
 * the others 0 while giveupMode runs, then as 0; 3: as 1 while the range
 * is not over, else as 0. */
// FUNC 0x00202588 DesignerCamera::getChangeLevel
int DesignerCamera::getChangeLevel(RaceOrganization &org, int level)
{
    f32 param = t;
    u8 over = rangeOver;
    u8 forced = forcedOnboard;
    int carPoint;
    int r = 0;

    carPoint = flags >> 10;                 /* CAMREC_CAR_POINT (as update) */
    carPoint &= 1;

    if (cameraData == NULL)
        return 2;
    if (forced != 0 && isViewBlocked(&org) != 0)
        return 2;
    switch (giveupState) {
    case 2:
        if (carPoint != 0 && forced == 0) {
            r = designer_change_level(carPoint, over, param, level, movingBack, 0);
            break;
        }
        if (giveupMode != 0)
            break;
        r = designer_change_level(carPoint, over, param, level, movingBack, forced);
        break;
    case 1:
        r = 2;
        if (giveupMode != 0)
            r = 0;
        break;
    case 3:
        r = (giveupMode != 0) ? 0 : 2;
        if (over == 0)
            break;
        /* fall through */
    case 0:
        r = designer_change_level(carPoint, over, param, level, movingBack, forced);
        break;
    }
    return r;
}

// FUNC 0x002026E0 DesignerCamera::isEnableChanging
int DesignerCamera::isEnableChanging()
{
    return giveupTime < 1;
}

/* 1 when the target left the record's range (only when its car changed);
 * forced onboard / CAMREC_CAR_POINT records return the change flag
 * itself. */
// FUNC 0x002026F0 DesignerCamera::isNeedReset
int DesignerCamera::isNeedReset(RaceOrganization &org, CameraTargetInfo &target)
{
    int r = 0;

    if (forcedOnboard != 0 || (flags & CAMREC_CAR_POINT))
        r = target.changed;
    else if (target.changed != 0) {
        CourseCameraRecord *rec = cameraData;
        f32 pos = target.distance;

        if (pos < rec->start || rec->end < pos)
            r = 1;
    }
    return r;
}

// FUNC 0x00202758 DesignerCamera::disableCrossOver
int DesignerCamera::disableCrossOver()
{
    return pitForced;
}

/* 1 when the target's raw (unblended) point is outside the frame: the
 * damped heading more than twice the half field of view from it, or the
 * pitch more than 1.5 times.  Caller: CameraManager::updateMode2 (T389). */
// FUNC 0x00202760 DesignerCamera::isTargetOutOfFrame
int DesignerCamera::isTargetOutOfFrame(CameraTargetInfo *target)
{
    CourseCameraRecord *rec = cameraData;
    int out;
    f32 pitch, yaw, zm, x, y, z, dx, dy, dz;

    if (rec == NULL)
        return 0;
    if (counter == 0)
        return 0;
    if (forcedOnboard != 0)
        return 0;
    if (coordMode == CAMREC_COORD_ONBOARD)
        return 0;
    out = 0;
    pitch = pitchDamper.angle;
    x = target->rawPos[0];
    y = target->rawPos[1];
    z = target->rawPos[2];
    yaw = yawDamper.angle;
    zm = zoom[0];
    dx = x - interpolate_table(rec->pos[0], t);
    dy = y - interpolate_table(rec->pos[1], t);
    dz = z - interpolate_table(rec->pos[2], t);
    /* the angle errors, in double */
    /* implicit: (in double: fp-bit fptodp, dpcmp, dpsub, dptofp) */
    {
        f64 d = angle_diff(pitch, camera_pitch_deg(dx, dy, dz));

        if (d < 0.0)
            d = 0.0 - d;
        pitch = d;
    }
    {
        f64 d = angle_diff(yaw, camera_yaw_deg(dx, dy, dz));

        if (d < 0.0)
            d = 0.0 - d;
        yaw = d;
    }
    /* zm: half the field of view */
    if (0.0f < zm)
        zm = 90.0f / zm;
    else
        zm = 1.0f;
    if (zm + zm < yaw)
        out = 1;
    if (zm * 1.5f < pitch)
        out = 1;
    return out;
}

/* the projection: the focal length scaled by the zoom, the near clip by
 * 0.700208 x the zoom */
// FUNC 0x00202960 DesignerCamera::setProjection
void DesignerCamera::setProjection(const CameraProjection *proj)
{
    f32 zm = zoom[0];

    pgl_frustum(proj->left, proj->right, proj->bottom, proj->top,
                proj->nearClip * 0.70020753f * zm, proj->farClip, proj->focal * zm);
    setLodBias(zoom[0]);
}

/* the current matrix times the view */
// FUNC 0x002029C0 DesignerCamera::applyView
void DesignerCamera::applyView()
{
    pgl_mult_matrix(matrix[0]);
}

// FUNC 0x002029E0 DesignerCamera::setInitBankAnim(float)
void DesignerCamera::setInitBankAnim(f32 bank)
{
    bankAnim.reset(bank);
}

// FUNC 0x00202A00 DesignerCamera::setInitRotOffset(float, float)
void DesignerCamera::setInitRotOffset(f32 pitch, f32 yaw)
{
    initRotYaw = yaw;
    initRotPitch = pitch;
    initRotOffsetSet = 1;
}

// FUNC 0x00202A18 DesignerCamera::setInitZoom(float)
void DesignerCamera::setInitZoom(f32 z)
{
    initZoom = z;
    initZoomSet = 1;
}

// FUNC 0x00202A28 DesignerCamera::getCoordMode(void)
int DesignerCamera::getCoordMode()
{
    return coordMode;
}

// FUNC 0x00202A30 DesignerCamera::getCameraNumber(void)
s16 DesignerCamera::getCameraNumber()
{
    return cameraNumber;
}

// FUNC 0x00202A38 DesignerCamera::getCameraData(void)
CourseCameraRecord *DesignerCamera::getCameraData()
{
    return cameraData;
}

// FUNC 0x00202A40 DesignerCamera::isForceOnboard(void)
u8 DesignerCamera::isForceOnboard()
{
    return forcedOnboard;
}

// FUNC 0x00202A48 DesignerCamera::isDrawBackMirror(void)
int DesignerCamera::isDrawBackMirror()
{
    return 0;
}

// FUNC 0x00202A50 DesignerCamera::isRangeOver(void)
int DesignerCamera::isRangeOver()
{
    return rangeOver;
}

// FUNC 0x00202A58 DesignerCamera::isEnableTargetChange(void)
int DesignerCamera::isEnableTargetChange()
{
    return targetChangeOk != 0;
}

// FUNC 0x00202A68 DesignerCamera::getInnerLapOffset(void)
int DesignerCamera::getInnerLapOffset()
{
    return innerLapOffset;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00202A70 __tf14DesignerCamera
extern "C" void *__tf14DesignerCamera(void)
{
    if (__ti14DesignerCamera[0] == 0) {
        __tf13OnBoardCamera();
        __rtti_si(__ti14DesignerCamera, "14DesignerCamera",
                  (const type_info *)__ti13OnBoardCamera);
    }
    return __ti14DesignerCamera;
}
#endif
