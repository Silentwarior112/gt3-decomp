/* camera_base_internal.h - unit u094 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x0020E020..0x002104E8, second pass.
 *
 * Translation units (address order):
 *   T393  TargetChooser (? name): the automatic choice of the car a replay
 *         view follows (CameraView +0x274, T383).  Every car has a score: its
 *         place gives it a base score and a chance per frame to gain 10;
 *         gaining / losing places adds 150 / takes 100; the state of its
 *         CameraTargetSelector slot (T392) adds a bonus.  After 240 frames
 *         the chance to switch grows to 1 over 1560 frames (tested every 30
 *         frames); the car with the best score that is drawn in full becomes
 *         `next' (include/game/camera/T393_target_chooser.h)
 *   T394  the replay view modes (traditional, avantgarde, broadcast,
 *         crsline): the targets each one allows (ReplayViewInfo), the random
 *         choice of a mode for a demo replay, and applying a mode and a
 *         target to a view of CameraManager (T394_replay_view_mode.h)
 *   T395  StepPair (? name): the colour correction's lo / hi of
 *         CameraManager, the first stepping towards its goal by 0.01 and
 *         capped at 0.3 (T395_step_pair.h)
 *   T396  CameraRequest (? name): what changed in a view this frame
 *         (CameraView +0x000): four flags with their arguments
 *         (T396_camera_request.h)
 *   T410  CameraBase: the abstract camera (no data, vptr +0): slot 1 sets the
 *         projection, slot 2 the view, slots 6 / 7 return them as a matrix
 *         (T410_camera_base.h)
 *   T411  MenuCarCamera: the menu camera that orbits a car (the showroom, the
 *         garage, ...), built from MotionValue springs, its presets read from
 *         the showroom records (T215) by its mode (T411_menu_car_camera.h)
 *   T412  SimpleCameraBase: a camera with a translation, a rotation and a
 *         frustum window that can be split into n x n tiles
 *         (T412_simple_camera_base.h)
 *
 * Every class is defined in its canonical header (include/game/).  Data (the
 * data pass, ref/data/u094/notes.md): each source includes the canonical
 * headers it needs itself, in the order that gives retail's type names (the
 * classes a file sees decide which type names its .rodata holds; T393..T395
 * open theirs with <new>'s "bad_alloc", T393 / T394 end with RaceCameraBase's
 * and UnitArena<64>'s names, T394 has the garage names before them, T395
 * UnitArena<64>'s only, T396 none; T410..T412 their vtable and own name).
 * This header has only the named constants several sources use.
 */
#ifndef U094_H
#define U094_H

#include "types.h"
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info, __rtti_user, __rtti_si (GCC 14's typeinfo
                                               copies; egcs's <new>, which T393..T395 include
                                               first, conflicts with it) */
#endif

/* ======================================================================== */
/* The replay views (T393, T394)                                             */
/* ======================================================================== */

/* The view modes (CameraViewKind::kind, GameOptions viewMode): ReplayOption's
 * "traditional" .. "crs" (T196) and T157's "replay_mode_*" images. */
enum {
    VIEW_MODE_TRADITIONAL = 0,              /* the view's own car only */
    VIEW_MODE_AVANTGARDE  = 1,              /* automatic only; its own BGM */
    VIEW_MODE_BROADCAST   = 2,              /* every target (TargetChooser picks the car) */
    VIEW_MODE_CRSLINE     = 3,              /* the view's own car only */
    VIEW_MODE_COUNT       = 4               /* the entries of g_002DF228 */
};

/* The camera targets (CameraViewKind::variant, GameOptions target[mode]):
 * ReplayOption's "manual", "top", "auto". */
enum {
    VIEW_TARGET_MANUAL = 0,                 /* the view's own car */
    VIEW_TARGET_TOP    = 1,                 /* the car in the lead */
    VIEW_TARGET_AUTO   = 2,                 /* chosen by TargetChooser */
    VIEW_TARGET_COUNT  = 3
};

/* CameraTargetSlot::state (T392; ? names): the bonus a state gives a car's
 * score, and the scale of the other cars' scores when the camera switches to
 * a car in that state. */
enum {
    TARGET_SLOT_CANDIDATE = 1,              /* bonus 100, scale 0.98 */
    TARGET_SLOT_BATTLE    = 2,              /* bonus 400, scale 0.84 */
    TARGET_SLOT_NEAR      = 3               /* bonus 200, scale 0.95 */
};

#define TARGET_SCORE_MAX    1200.0f         /* a score is kept in 0..1200; a forced target has 1200 */
#define TARGET_PLACE_LAST   5               /* g_002DF108 has six places: 6th and below share the last */
#define TARGET_CARS         7               /* TargetChooser::car[] */

/* ======================================================================== */
/* MenuCarCamera (T411)                                                      */
/* ======================================================================== */

/* The showroom record values MenuCarCamera's presets read (T215
 * ShowroomData::values). */
enum {
    SHOWROOM_INTEREST      = 0x34,          /* setInterest: x, y, z */
    SHOWROOM_INTEREST_GOAL = 0x38,          /* setInterestGoal: x, y, z, spring, damping */
    SHOWROOM_ORBIT         = 0x3D,          /* setOrbit: pitch, yaw, distance */
    SHOWROOM_ORBIT_MOTION  = 0x41,          /* setOrbitMotion: 6 values */
    SHOWROOM_SHIFT         = 0x47,          /* setShift: x, y; then setTilt from 0x49 */
    SHOWROOM_FOV           = 0x4B           /* the vertical field of view, degrees */
};

/* The window of the menu camera: 640 x 448 around the centre, near 0.1,
 * far 1000. */
#define MENU_CAMERA_HALF_W  320.0f
#define MENU_CAMERA_HALF_H  224.0f
#define MENU_CAMERA_NEAR    0.1f
#define MENU_CAMERA_FAR     1000.0f

/* MenuCarCamera::mode 2 (a showroom record shared with mode 1, showroom_index)
 * reads the presets mirrored in x. */
#define MENU_CAMERA_MODE_MIRRORED   2

#endif /* U094_H */
