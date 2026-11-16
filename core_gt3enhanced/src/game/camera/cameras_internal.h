/* cameras_internal.h - unit u090 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x00201418..0x00204B10.
 *
 * The replay / course cameras of the race (link order), each with its
 * canonical header:
 *   T398  DesignerCamera   the course cameras of the replay: a record of the
 *                          course's camera file, damped yaw / pitch, a bank
 *                          animation, an onboard fallback.
 *                          game/camera/T398_designer_camera.h
 *   T399  PasslineCamera   a fixed course camera from the first record of
 *                          "cameras/start.cam", at timeRange in the record.
 *                          game/camera/T399_passline_camera.h
 *   T400  StartingCamera   PasslineCamera driven by the start countdown,
 *                          composed with the target's position / angles.
 *                          game/camera/T400_starting_camera.h
 *   T401  AutomaticCamera  a random still camera ahead of the car (on the
 *                          road plane) or beside it.
 *                          game/camera/T401_automatic_camera.h
 *   T402  AutomountCamera  a random mount around the car with wobbling
 *                          angles (and CameraWobble).
 *                          game/camera/T402_automount_camera.h
 *   T403  LoggerCamera     a fixed chase camera behind the car.
 *                          game/camera/T403_logger_camera.h
 *
 * All derive from VariableCameraBase (T397) : CameraBase (T410).  Common
 * pattern: the view matrix (VariableCameraBase::matrix) is built with the VU0
 * matrix helpers of G001 on a pushed matrix level, between intr_suspend() and
 * intr_resume() (G002).
 *
 * Every class, function and global the unit uses is declared in a canonical
 * header; this header only includes them.  Offsets in comments are the
 * original (egcs) ones.
 *
 * Data (the data pass, ref/data/u090/notes.md): every retail file's .rodata
 * opens with <new>'s "bad_alloc" and ends with UnitArena<64>'s type name
 * (T401..T403: "14RaceCameraBase" before it).  Each source includes <new>
 * first (anchored with DATA_START), then T153_race_course.h under
 * GAME_T153_EMITS_TYPEINFO (and T401..T403 T383_race_camera_base.h) before
 * this header: the names come in reverse definition order.
 */
#ifndef U090_H
#define U090_H

#include "types.h"
#include "eekernel.h"
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                              /* type_info, __rtti_si (GCC 14's typeinfo copies; egcs's
                                                   <new>, which the sources include first, conflicts
                                                   with it) */
#endif

/* own translation units */
#include "game/camera/T398_designer_camera.h"          /* DesignerCamera, DesignerCameraFrame */
#include "game/camera/T399_passline_camera.h"          /* PasslineCamera */
#include "game/camera/T400_starting_camera.h"          /* StartingCamera */
#include "game/camera/T401_automatic_camera.h"         /* AutomaticCamera */
#include "game/camera/T402_automount_camera.h"         /* AutomountCamera, CameraWobble */
#include "game/camera/T403_logger_camera.h"            /* LoggerCamera */

/* outside the unit */
#include "game/camera/T397_variable_camera_base.h"     /* VariableCameraBase (the base of every camera here) */
#include "game/camera/T377_camera_target_info.h"       /* CameraTargetInfo (the target of a view) */
#include "game/camera/T378_onboard_camera.h"           /* OnBoardCamera (DesignerCamera's base) */
#include "game/camera/T379_camera_rot_damper.h"        /* CameraRotDamper, CameraSway, angle_diff */
#include "game/camera/T380_camera_spring.h"            /* CameraFollow, CameraBank */
#include "game/camera/T382_camera_select.h"            /* camera_pitch_deg, camera_yaw_deg */
#include "game/camera/T383_camera_projection.h"        /* CameraProjection (not the camera manager's
                                                   header: it puts "14RaceCameraBase" into .rodata) */
#include "game/camera/T408_car_progress.h"             /* CarProgress */
#include "game/camera/T172_course_camera.h"            /* CourseCamera, CourseCameraRecord, interpolate_table */
#include "game/race/core/T153_race_course.h"              /* RaceCourse, g_00351F94 ("cameras/start.cam") */
#include "game/course/T179_course_data.h"              /* CourseData (replay) */
#include "game/race/core/T125_race_organization.h"        /* RaceOrganization (course, countDown, eventQueue) */
#include "game/race/core/T122_race_event_queue.h"         /* RaceEventQueue::post */
#include "game/race/core/T121_race_entry.h"               /* RaceEntryCar (state) */
#include "game/car/physics/T302_car_state.h"                /* RaceCarState (params), car_view_* / car_pitch / car_yaw */
#include "game/race/dynamics/T350_pit_lane.h"                 /* car_in_pit_lane */
#include "game/race/core/T413_race_input.h"               /* RaceInput (: Control, getButtonsOn) */
#include "engine/pdistd/T531_random.h"                   /* Random */
#include "engine/pgl/G001_flare.h"                    /* pgl_* / vu0_* matrix helpers, vu0_atan2f, gfx_perf_counter0 */
#include "engine/media/G002_mpeg_frameq.h"              /* intr_suspend, intr_resume, cosf, sinf, tanf */

/* ======================================================================== */
/* Unit-local constants                                                     */
/* ======================================================================== */

/* CourseCameraRecord::flags as the course cameras read them (T398, T399;
 * DesignerCamera keeps a copy in its own flags). */
enum {
    CAMREC_COORD_MASK   = 0x003,            /* the coordinate mode, CAMREC_COORD_* */
    CAMREC_AIM          = 0x00C,            /* aims at the target (else the record's look-at tables) */
    CAMREC_ZOOM_BY_DIST = 0x010,            /* zoom = the zoom table at 0 x the distance (DesignerCamera
                                               sets it in its copy when forced onboard: the record's
                                               zoom is then not used) */
    CAMREC_CAR_POINT    = 0x400             /* ? aims at car 0's own position: no two-car framing, no
                                               start from the saved position; isNeedReset follows the
                                               target's change flag */
};

/* The coordinate modes (CAMREC_COORD_MASK). */
enum {
    CAMREC_COORD_COURSE  = 0,               /* a camera on the course (world coordinates) */
    CAMREC_COORD_CAR     = 1,               /* the same on top of the onboard view (relative to the car) */
    CAMREC_COORD_ONBOARD = 3                /* the car's onboard mount `mount' (2: ? the onboard update
                                               without setting a mount) */
};

/* VariableCameraBase::effect: bit i + 13 when the record's effect[i] > 0
 * (what the effects are is not seen here). */
enum {
    CAMERA_EFFECT_0 = 0x2000,
    CAMERA_EFFECT_1 = 0x4000,
    CAMERA_EFFECT_2 = 0x8000
};

#endif /* U090_H */
