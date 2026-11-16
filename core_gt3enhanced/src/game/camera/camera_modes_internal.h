/* camera_modes_internal.h - unit u093 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x0020AB80..0x0020E020: the replay / TV camera director of CameraManager
 * (T383), one translation unit per view mode, and the per-car camera target
 * selector (T392).
 *
 * Translation units (link order):
 *   T409  CameraManager: designer camera changes over the course camera
 *         records, cross-fades, "may the camera cut now?"
 *   T386  CameraManager: the BGM marker clock (cuts are timed to the markers
 *         of the race music)
 *   T387  CameraManager::updateStart, the countdown cameras
 *   T388  CameraManager::updateMode0, view kinds 0 and 3
 *   T389  CameraManager::updateMode2, view kind 2
 *   T390  CameraManager::updateMode1, view kind 1 (a random car, camera and
 *         screen effects at every cut)
 *   T391  CameraManager::updateLogger, the data logger
 *   T392  CameraTargetSelector: the car each car's camera should rather show
 *         together with it
 *
 * The classes and functions are declared in the canonical headers: the
 * CameraManager methods of T386..T391 / T409 in T383_camera_manager.h (a
 * class cannot be reopened), T388_camera_mode0.h (CameraSet = CameraView),
 * T392_camera_target_selector.h.  CameraManager::update (0x00208BE8, T383)
 * calls the mode handlers as (org, dc, input, view) for every view; each
 * view's cameras are in CameraManager::views[view] (CameraView, 0x24C0 bytes).
 *
 * This header keeps only what is local to the unit: two free functions
 * without a header (camera_kind1_alt_style = F_0020B1D8, pickAutoCamera),
 * T390's .data tables,
 * named constants and a small inline helper.
 *
 * egcs ABI reminder: the vptr is a data member placed after the data of the
 * first polymorphic class (RaceBGMBase +0x4, Control +0xC0; CameraBase has no
 * data, so the cameras' and CameraManager's are at +0).  A modern g++ build
 * puts it at +0; the offsets in the comments are the original ones.
 */
#ifndef U093_H
#define U093_H

#include "types.h"

#include "game/camera/T383_camera_manager.h"       /* CameraManager, CameraView, CarProgress, CameraViewKind; the cameras */
#include "game/camera/T388_camera_mode0.h"         /* CameraSet (= CameraView) */
#include "game/camera/T392_camera_target_selector.h"   /* CameraTargetSelector, CameraTargetSlot, carDistanceSq, carRelation */
#include "game/camera/T377_camera_target_info.h"   /* CameraTargetInfo */
#include "game/camera/T393_target_chooser.h"       /* TargetChooser, F_0020E790 */
#include "game/camera/T396_camera_request.h"       /* CameraRequest, F_0020EE28 .. F_0020EE60 */
#include "game/camera/T397_variable_camera_base.h" /* VariableCameraBase */
#include "game/camera/T378_onboard_camera.h"       /* OnBoardCamera */
#include "game/camera/T398_designer_camera.h"      /* DesignerCamera */
#include "game/camera/T380_camera_spring.h"        /* CameraBank (DesignerCamera::bankAnim) */
#include "game/camera/T382_camera_select.h"        /* camera_find_record, camera_find_next_record(_for) */
#include "game/camera/T401_automatic_camera.h"     /* AutomaticCamera */
#include "game/camera/T402_automount_camera.h"     /* AutomountCamera */
#include "game/camera/T405_matrix_camera.h"        /* MatrixCamera */
#include "game/camera/T406_zoomup_camera.h"        /* ZoomupCamera */
#include "engine/pdistd/T531_random.h"               /* Random (CameraManager::rng) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization, getBGM */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post */
#include "game/race/core/T129_race_base.h"            /* RaceBase::replayMode */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase, RaceEntryCar */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState, car_view_x / y / z */
#include "game/sound/T288_car_sound.h"            /* DynCarState::rank (RaceCarState::st) */
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* DynamicsConductor, CarPairStates */
#include "game/race/core/T153_race_course.h"          /* RaceCourse::data */
#include "game/course/T179_course_data.h"          /* CourseData::replay */
#include "game/camera/T172_course_camera.h"        /* CourseCamera, CourseCameraRecord */
#include "game/sound/T175_race_bgm_base.h"        /* RaceBGMBase (the marker channels) */
#include "game/race/core/T413_race_input.h"           /* RaceInput */
#include "engine/input/T474_control.h"              /* Control::getButtonsOn */
#include "game/sound/T176_car_sound.h"            /* g_003521E4 */
/* Not here: T428_game_options.h / T429_game_status.h (GameOptions,
 * getGameStatus): only T390 uses them, and of the unit's originals only
 * T390's has the GameStatus's garage type names (T390 includes them). */

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* The camera buttons: RaceInput's replay bits (Control::getButtonsOn, set in
 * replay modes 1..3 by RaceInput::emulatePressure, u095 RIB_REPLAY_*). */
enum {
    CAMBTN_RIGHT   = 0x00040000,            /* mode 1: mode1Style + 1.1 */
    CAMBTN_LEFT    = 0x00080000,            /* mode 1: mode1Style - 1.1 */
    CAMBTN_UP      = 0x00100000,            /* the car ahead (CameraManager::prevWatchableByRank); mode 1: soundLevel + 0.1 */
    CAMBTN_DOWN    = 0x00200000,            /* the car behind (nextWatchableByRank); mode 1: soundLevel - 0.1 */
    CAMBTN_CROSS   = 0x00400000,            /* the view: designer <-> onboard (logger: also the logger camera) */
    CAMBTN_CROSS2  = 0x02000000,            /* cross again (mode 1: cut) */
    CAMBTN_CIRCLE  = 0x04000000,            /* mode 1: cut */
    CAMBTN_R1      = 0x08000000,            /* mode 1: cut */
    CAMBTN_L1      = 0x10000000             /* mode 1: cut */
};
#define CAMBTN_UP_DOWN    (CAMBTN_UP | CAMBTN_DOWN)
#define CAMBTN_LEFT_RIGHT (CAMBTN_RIGHT | CAMBTN_LEFT)

/* The enhanced engine (Lupo 2, cameras_sound.md C5): the TV camera's zoom
 * button, read held (Control::getButtons) by updateMode0.  Lupo 2 sets
 * 0x00800000 for the pad's triangle in replay modes 1..3 (L2 0x0021D1CC),
 * having moved GT3's view mode bits (CAMBTN_VIEWMODE_*, T383: 0x00800000 /
 * 0x01000000) and cross2 .. L1 one bit up.  GT3's replay word keeps its
 * bits and has 0x40000000 free, so the port uses that one: the race input
 * (RaceInput::emulatePressure, T413) is to set it for the triangle (and a
 * wheel button) in replay modes 1..3; until it does, the zoom stays at 1. */
#define CAMBTN_ZOOM       0x40000000

/* CameraViewKind::view */
enum {
    VIEW_DESIGNER = 0,                      /* the designer (course) camera, or what updateDesigner falls back to */
    VIEW_ONBOARD  = 1,                      /* the onboard camera */
    VIEW_LOGGER   = 2                       /* the logger camera (updateLogger only) */
};

/* updateDesigner's flags.  Any flag also cross-fades into a new record. */
enum {
    DESIGNER_FORCE = 1,                     /* take the record at the target even when it is the same one */
    DESIGNER_RESET = 2,                     /* set the current record up again */
    DESIGNER_FADE  = 4                      /* (no meaning of its own: every mode handler passes it) */
};

/* VariableCameraBase::type / trueType of a DesignerCamera: setType(1) in its
 * constructor; setForceOnboard sets type 0 (its OnBoardCamera base draws) and
 * trueType 1. */
enum {
    CAMTYPE_ONBOARD  = 0,
    CAMTYPE_DESIGNER = 1
};

/* The race events these handlers post (RaceEventQueue::post, T122):
 * RACE_EVENT_VIEW_MODE, arg (sub << 16) | (kind << 8) | view (T383; mode 1:
 * sub = the style 1 / 2); RACE_EVENT_ERROR, arg a const char * ("TEST : ...";
 * T131 also posts one). */

/* ======================================================================== */
/* Helpers                                                                  */
/* ======================================================================== */

/* The number of a course camera record: the code computes (rec - records) >> 7
 * (CourseCameraRecord is 0x80 bytes) and passes it to setupOnRecord as is (an
 * int; the owner declares s16, note h21). */
static inline int camera_record_number(RaceCourse *course, CourseCameraRecord *rec)
{
    return rec - course->data->replay->records;
}

/* "The view's camera is a designer camera forced onboard" (trueType 1, type
 * 0: DesignerCamera::setForceOnboard) is written in the handlers as nested
 * ifs, trueType (+5) first: as one `&&' egcs merges the two adjacent byte
 * compares into a single ld + mask (fold_truthop), which retail does not. */

/* ======================================================================== */
/* Free functions without a header                                          */
/* ======================================================================== */

/* T386 (F_0020B1D8): view kind 1 in its other style (mode1Style >= 0.5:
 * the cuts follow BGM marker channel 4 instead of 3 / 2 / 1). */
int camera_kind1_alt_style(int kind, f32 style);            /* 0x0020B1D8 */
/* T390: the camera of a mode-1 cut from a random r in [0, 1): 0 automatic,
 * 1 automount, 2 matrix, 3 zoomup, 4 designer (onboard). */
int pickAutoCamera(int counting, f32 nextMarker, f32 r);    /* 0x0020C510 */

/* ======================================================================== */
/* Data (T390 .data)                                                        */
/* ======================================================================== */

/* Writable, not const: the tables are in .data (T390's, before its frame
 * info at 0x002DEDB4), and updateMode1 loads the colour row's floats between
 * its stores to colour[] (a const table's loads would move ahead of the
 * stores: egcs marks them unchanging).  Defined in T390_camera_mode1.cpp. */

/* The colours of mode 1's colour override: red, blue, cyan, grey .3, white. */
extern f32 g_002DED30[5][3];                /* 0x002DED30 */
/* pickAutoCamera's tables: the first i with r < table[i].  5 floats each
 * (the loop reads 5; each is followed by a word of 0 or the next table). */
extern f32 g_002DED70[5];                   /* 0x002DED70 {0, 0, 0, 0, 1} during the countdown: the designer */
extern f32 g_002DED88[5];                   /* 0x002DED88 {.35, .6, .7, .85, 1} the next marker is < 3 s away */
extern f32 g_002DEDA0[5];                   /* 0x002DEDA0 {0, .3, .5, .7, 1} 3 s or more */

#endif /* U093_H */
