/* camera_motion_internal.h - unit u091 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x00204B10..0x00207768, second pass: race cameras and their helpers.
 *
 * Translation units (link order):
 *   T404  HeliCamera: an AutomountCamera that follows its car along a course
 *         camera record (the "helicopter" replay view)
 *         (include/game/camera/T404_heli_camera.h)
 *   T405  MatrixCamera: a randomised sweep around the car (T405_matrix_camera.h)
 *   T406  ZoomupCamera: a randomised close-up with a speed-dependent shake
 *         (T406_zoomup_camera.h)
 *   T379  CameraRotDamper, the angle helpers (wrap to +-180 / 0..360) and
 *         CameraCurve, a cubic ease (T379_camera_rot_damper.h)
 *   T380  CameraSpring, a damped spring (T380_camera_spring.h)
 *   T407  CameraFollow / CameraBank, two small dampers of DesignerCamera
 *         (no header of its own: T380_camera_spring.h)
 *   T381  CameraSway, a random target chased by a spring (no header of its
 *         own: T379_camera_rot_damper.h)
 *   T408  CarProgress, the lap distance of every car frame to frame
 *         (T408_car_progress.h; CameraManager +0x278)
 *   T382  the pitch / yaw of a direction and the course camera record pickers
 *         (T382_camera_select.h)
 *
 * Every class is defined in its canonical header.  The cameras derive from
 * VariableCameraBase (T397; CameraBase T410) and look at a CameraTargetInfo
 * (T377); the car is RaceCarState (T302: its CarDynamics extents, its
 * DynCarState lap, the interpolated view position / yaw).  This header adds
 * the outside headers the sources need and what only this unit uses: the
 * literal-pool constants and the camera numbers the code tests.
 *
 * Not included here: the camera manager's header (T383).  With it egcs writes
 * "14RaceCameraBase" in front of each object's own type name, which retail
 * has after it (MatrixCamera's .rodata: the jump table, the vtable, then
 * "12MatrixCamera" at 0x0033EC80); T382 / T408 take CarProgress from
 * T408_car_progress.h.  The sources that saw RaceCameraBase (T404..T406,
 * T408, T382) read <new>, T153 (emitting UnitArena<64>'s name) and
 * T383_race_camera_base.h themselves, before their own header; T407 <new>
 * and T153 (retail's names per file: the data pass, ref/data/u091/notes.md).
 */
#ifndef U091_H
#define U091_H

#include "types.h"                          /* boot_sqrtf (sqrt.s) */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* __rtti_si, __rtti_user, type_info (GCC 14's typeinfo
                                               copies; egcs's <new>, read first by most sources,
                                               conflicts with it) */
#endif

/* own translation units */
#include "game/camera/T379_camera_rot_damper.h"    /* CameraRotDamper, CameraCurve, CameraSway, angle_* */
#include "game/camera/T380_camera_spring.h"        /* CameraSpring, CameraFollow, CameraBank */
#include "game/camera/T382_camera_select.h"        /* camera_pitch_deg .. camera_find_next_record_for */
#include "game/camera/T404_heli_camera.h"          /* HeliCamera */
#include "game/camera/T405_matrix_camera.h"        /* MatrixCamera */
#include "game/camera/T406_zoomup_camera.h"        /* ZoomupCamera */

/* outside the unit */
#include "engine/pgl/G001_flare.h"                /* pgl_* matrix stack, vu0_rotate_*, vu0_atan2f, gfx_perf_counter0 */
#include "engine/media/G002_mpeg_frameq.h"          /* cosf, tanf, intr_suspend */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar (state), RaceEntryBase (count, cars) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, course) */
#include "game/race/core/T153_race_course.h"          /* RaceCourse (getLapLength, getPlane, collectReplayCameras,
                                               getReplayLoopCount) */
#include "game/camera/T172_course_camera.h"        /* CourseCameraRecord, interpolate_table */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState, car_view_x / y / z, car_yaw, car_lap_distance */
#include "game/camera/T377_camera_target_info.h"   /* CameraTargetInfo */
#include "game/camera/T383_camera_projection.h"    /* CameraProjection (the cameras' setProjection) */
#include "game/camera/T378_onboard_camera.h"       /* CameraShake (ZoomupCamera's wobbles) */
#include "game/camera/T397_variable_camera_base.h" /* VariableCameraBase, __tf18VariableCameraBase */
#include "game/camera/T402_automount_camera.h"     /* AutomountCamera, __tf15AutomountCamera */
#include "engine/pdistd/T531_random.h"               /* Random */

/* ======================================================================== */
/* Constants (the literal pool)                                             */
/* ======================================================================== */

#define U091_PI          3.1415927f         /* 0x40490FDB */
#define U091_TWO_PI      6.2831855f         /* 0x40C90FDB */
#define U091_RAD_TO_DEG  57.29578f          /* 0x42652EE1: 180 / pi */
#define U091_FPS         60.0f              /* the cameras step 60 times a second */
#define U091_FRAME_TIME  0.016666668f       /* 0x3C888889: 1 / 60 s */

/* VariableCameraBase::type of the cameras of this unit (T397: 5 automount /
 * matrix / zoom-up, 6 heli); the constructors set type and trueType (an
 * inlined setType). */
enum {
    CAMERA_TYPE_MOUNT = 5,                  /* AutomountCamera, MatrixCamera, ZoomupCamera */
    CAMERA_TYPE_HELI  = 6                   /* HeliCamera */
};

/* The results of getChangeLevel as CameraManager::canCut (T409) reads them:
 * 2 the camera may be cut at once; 1 it may be cut away from a music
 * marker; 0 it asks for nothing (strict callers never cut it). */
enum {
    CHANGE_LEVEL_NONE = 0,
    CHANGE_LEVEL_SOFT = 1,
    CHANGE_LEVEL_NOW  = 2
};

/* VariableCameraBase::giveupState as HeliCamera::getChangeLevel reads it (?
 * names: after what each state does there; giveupMode is the frame timer). */
enum {
    GIVEUP_ON_RANGE       = 0,              /* change once the target leaves the record */
    GIVEUP_ON_TIME        = 1,              /* change once giveupMode runs out */
    GIVEUP_TIME_THEN_RANGE = 2,             /* after giveupMode, as GIVEUP_ON_RANGE */
    GIVEUP_RANGE_OR_TIME  = 3               /* as GIVEUP_ON_RANGE, and at the latest when giveupMode runs out */
};

#endif /* U091_H */
