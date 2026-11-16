/* u089 - GT3 main executable, 0x001FED38..0x00201418, second pass.
 *
 *   T377  CameraTargetInfo: what a camera view looks at - one car, or the
 *         midpoint of two - with timed, eased blends from the previous
 *         target and a saved snapshot (include/game/camera/T377_camera_target_info.h)
 *   T378  OnBoardCamera: the cameras mounted on the followed car (the
 *         camera mounts of the car file), with head sway / shake springs,
 *         look-back, mount cycling and the "off the course / inside another
 *         car's box" checks; CameraShake, its random head shake
 *         (include/game/camera/T378_onboard_camera.h).  DesignerCamera (T398)
 *         derives from OnBoardCamera.
 *
 * Every shared class is defined in its canonical header; this header adds
 * the outside headers the sources need and what only this unit uses: the
 * camera mount record of the car file, T377's target point and T378's box
 * helpers, T378's .rodata tables and the button bits it reads.
 */
#ifndef U089_H
#define U089_H

#include "types.h"
#include "libc.h"                           /* memset */
#ifndef BOOT_OLD_GCC                        /* egcs's <new> (T377, T378) conflicts with it */
#include "cxxrt.h"                          /* __rtti_si (GCC 14's __tf13OnBoardCamera) */
#endif

/* own translation units (T377 brings T379 CameraCurve; T378 brings T397
 * VariableCameraBase / T410 CameraBase, T380 CameraSpring, T531 Random) */
#include "game/camera/T377_camera_target_info.h"
#include "game/camera/T378_onboard_camera.h"

/* outside the unit.  No header here may emit a type name or a literal
 * (tools/hdrnames.py --each): retail T377's .rodata is "bad_alloc" and
 * UnitArena<64>'s name only, T378's adds its own and RaceCameraBase's, which
 * T378 reads itself before its class (T383_race_camera_base.h; not the
 * camera manager's header, which defines OnBoardCamera before it). */
#include "game/camera/T379_camera_rot_damper.h"    /* CameraCurve, angle_diff, angle_wrap360 */
#include "game/camera/T382_camera_select.h"        /* camera_yaw_deg */
#include "game/camera/T383_camera_projection.h"    /* CameraProjection (slot 11; CarProgress: pointers only) */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase, RaceEntryCar (T180 RaceCarModel model) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, course) */
#include "game/race/core/T153_race_course.h"          /* RaceCourse: getLapLength, getReplayLoopCount, getPlane */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState and its interpolated accessors (car_view_x ...) */
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* DynamicsConductor::boxes (T167 CarBoxList, T376 CarBox) */
#include "game/race/core/T413_race_input.h"           /* RaceInput (T474 Control): the buttons, replayMode */
#include "game/car/model/T455_car_model_info.h"       /* CarModelInfo: mountCount / mounts */
#include "game/car/model/T456_car_model_file.h"       /* CarModelFile: info */
#include "engine/pgl/G001_flare.h"                /* pgl_* / vu0_*: the VU0 matrix stack */
#include "engine/media/G002_mpeg_frameq.h"          /* intr_suspend, tanf */

/* ======================================================================== */
/* T377 - the target point                                                  */
/* ======================================================================== */

/* ? name.  A point of the target, 0x18 bytes (stack locals of
 * CameraTargetInfo::update). */
struct TargetPoint {
    f32 x, y, z;                            /* 0x00 the camera world (car_view_x / y / z) */
    f32 distance;                           /* 0x0C lap x lap length + lapDistance */
    f32 lapDistance;                        /* 0x10 car_lap_distance */
    int lap;                                /* 0x14 the lap, wrapped to 1..getReplayLoopCount (0 before the start) */

    void clear();                           /* 0x001FEDF0 lap = 1, the rest 0 */
};
/* the point of one car; of the midpoint of two (lap and distance: of a) */
void getCarPoint(RaceCarState *car, RaceCourse *course, TargetPoint *out);                  /* 0x001FEE18 */
void getPairPoint(RaceCarState *a, RaceCarState *b, RaceCourse *course, TargetPoint *out); /* 0x001FEF10 */

/* ======================================================================== */
/* T378 - the camera mounts of the car file                                 */
/* ======================================================================== */

/* ? name.  One camera mount of a car file, 0x40 bytes (CarModelInfo +0x6C,
 * T455 forward-declares it; no other unit reads the records). */
struct CameraMount {
    f32 x, y, z;                            /* 0x00 the camera point on the car */
    f32 rotX;                               /* 0x0C degrees */
    f32 rotY;                               /* 0x10 degrees (getMountDirection's heading) */
    f32 rotZ;                               /* 0x14 degrees */
    f32 zoom;                               /* 0x18 the mount's 1 / tan(half the field of view) */
    int flags;                              /* 0x1C MOUNT_* */
    u8  unk_20[0x20];                       /* 0x20 */
};
EGCS_SIZE(CameraMount, 0x40);

/* CameraMount::flags */
enum {
    MOUNT_DRAW_INSIDE = 0x2,                /* mountFlag2, virtual_9's 1 (? the car's inside is drawn) */
    MOUNT_NO_SHAKE    = 0x4,                /* no head shake, no springs */
    MOUNT_SELECTABLE  = 0x8                 /* offered by the mount buttons (stepMount's flags) */
};

/* The mounts with a meaning of their own.  0 and 1 are the two fixed views
 * (g_0033E370 without car mounts: (0, -0.8, 0) and (0, -1.5, -6)): they
 * turn with the car, 0 sways with the wheels, 1 leaves the car's lag yaw /
 * pitch and its roll out.  The others take the mount's own rotation and the
 * head shake. */
enum {
    MOUNT_SWAY           = 0,
    MOUNT_CHASE          = 1,
    MOUNT_FIRST_OWN      = 2,               /* findMountsToward / ByDirection search from here */
    MOUNT_FIRST_CHECKED  = 5,               /* isViewBlocked tests the mounts from here */
    MOUNT_MIN_SELECTABLE = 6                /* fewer mounts: the replay buttons keep mount 0 */
};

/* getMountDirection's mask (g_002DB7B0 by 45-degree sector of the heading) */
enum {
    MOUNT_DIR_FRONT   = 1,                  /* 0 degrees (and 45, 315) */
    MOUNT_DIR_REAR    = 2,                  /* 180 (and 135, 225) */
    MOUNT_DIR_SIDE270 = 4,                  /* 270 (and 225, 315) */
    MOUNT_DIR_SIDE90  = 8                   /* 90 (and 45, 135) */
};

/* RaceInput's button word (T413 emulatePressure; u095's RIB_* names) */
enum {
    ONBOARD_BTN_LOOK_BACK = 0x10000,        /* function 7 (RIB_FUNC7), held */
    ONBOARD_BTN_VIEW      = 0x20000,        /* function 8, pressed: mount 0 <-> 1 (replayMode 0) */
    ONBOARD_BTN_RIGHT     = 0x40000,        /* RIB_REPLAY_RIGHT (replayMode 1..3): stepMount(-1) */
    ONBOARD_BTN_LEFT      = 0x80000         /* RIB_REPLAY_LEFT: stepMount(+1) */
};

/* the box tests: the camera point against six planes (x, y, z, w) */
int isInsidePlanes(const f32 (*planes)[4], const f32 *p);   /* 0x00200950 */
f32 dot3(const f32 *a, const f32 *b);                       /* 0x002009C0 */
int isMountLocked(int mount, int flags);                    /* 0x00201148 (always 0) */

/* ? name.  A name of a camera mount (the index into the car file's mounts),
 * 8 bytes: T378's unreferenced table g_0033E100. */
struct CameraMountName {
    int         id;                         /* 0x0 the mount index; -1 ends the table */
    const char *name;                       /* 0x4 */
};

/* T378 .rodata / .sdata (defined in T378; nothing else reads them) */
extern const CameraMountName g_0033E100[21]; /* 0x0033E100 mounts 0..19 ("Driver", "Chase", "reserved", ...), { -1, "" } */
extern int g_00352A0C;                      /* 0x00352A0C 20 (? the table's count; nothing reads it) */
extern const f32 g_0033E1A8[6][4];          /* 0x0033E1A8 the box face normals: -z, -x, +z, +x, +y, -y */
extern const f32 g_0033E208[6][4];          /* 0x0033E208 the same, isInsideCar's copy */
/* g_0033E370 (mounts 0..4 of a car without mounts) is a file static of T378. */
/* T378's .data (between u087's 0x002DA508 and u093's 0x002DED30): the
 * direction mask of each 45-degree sector, 1, 9, 8, 10, 2, 6, 4, 5, 1
 * (MOUNT_DIR_*), and a tenth entry 9 (never read: the sector is clamped to
 * 8).  64-bit entries (ld, then the low word sign-extended: an EE `long').
 * Not const: retail has it in .data. */
extern s64 g_002DB7B0[10];                  /* 0x002DB7B0 */

#endif /* U089_H */
