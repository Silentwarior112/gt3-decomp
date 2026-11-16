/* camera_manager_internal.h - unit u092 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x00207768..0x0020AB80.
 *
 * Translation units (link order):
 *   T383  CameraManager (RaceBase +0x40): up to four views, each a CameraView
 *         with one of every race camera; init, the per-frame update, the
 *         i.LINK screen wall, the forwarders to the current camera, the
 *         accessors and the typeinfo functions of CameraManager and
 *         RaceCameraBase.
 *   T384  CameraGrooveMeter (CameraManager +0x963C) and the arc helpers.
 *   T385  the course demo cameras (CameraManager::startDemoCameras /
 *         updateDemoCameras).
 *
 * The classes and functions are declared in the canonical headers
 * include/game/T383..T385 (CameraManager, CameraView, CameraProjection,
 * CarProgress, ScreenLayout, CameraViewKind, RaceCameraBase,
 * CameraGrooveMeter) and in the headers of the units they use; each source
 * includes the ones it needs itself, in the order that gives the original's
 * type names (the data pass, ref/data/u092/notes.md).  This header keeps
 * only what is local to the unit: named constants for the values the code
 * tests.
 *
 * egcs ABI reminder: the vptr is a data member placed after the data of the
 * first polymorphic class (RaceInput / Control: +0xC0, CameraGrooveMeter:
 * +0x98).  A modern g++ build puts it at +0x00; the offsets in the comments
 * are the original ones.
 */
#ifndef U092_H
#define U092_H

#include "types.h"
#ifndef BOOT_OLD_GCC                            /* (egcs's <new>, which T383 / T385 read, conflicts with it) */
#include "cxxrt.h"                              /* __rtti_si, __rtti_user (the GCC 14 __tf copies) */
#endif
#include <stddef.h>

/* ======================================================================== */
/* T383                                                                     */
/* ======================================================================== */

/* The view modes of a CameraView (CameraViewKind::kind; GameOptions
 * viewMode; T394's table and ReplayOption's "display" row, T196; the images
 * "replay_mode_traditional" .. "replay_mode_crsline", T157). */
enum {
    VIEWMODE_TRADITIONAL = 0,                   /* updateMode0 */
    VIEWMODE_AVANTGARDE  = 1,                   /* updateMode1 */
    VIEWMODE_BROADCAST   = 2,                   /* updateMode2 */
    VIEWMODE_CRS         = 3,                   /* updateMode0 too */
    VIEWMODE_COUNT       = 4,                   /* the modes the pad steps through (and T394's table) */
    VIEWMODE_LOGGER      = 5                    /* the data logger (RaceOrganization::loggerMode): updateLogger */
};

/* RaceInput::replayMode as update() reads it (u089 / u095: 0 live, else the
 * playback of a replay). */
enum {
    INPUT_LIVE = 0                              /* the race itself: updateStart, the onboard mounts saved */
    /* 1..3: a replay: the view modes; outside 0..3 no mode handler runs */
};

/* The bits of RaceInput::getButtonsOn() (Control slot 5) that step the view
 * mode of a replay (u093's CAMBTN_* are the neighbouring bits).  ? which pad
 * buttons: RaceInput::emulatePressure (T413, u095) does not set them, so
 * another input class does (? the logger / link battle inputs). */
enum {
    CAMBTN_VIEWMODE_PREV = 0x00800000,          /* the previous view mode (3 after 0) */
    CAMBTN_VIEWMODE_NEXT = 0x01000000           /* the next one (0 after 3) */
};

/* The race event a view mode change posts: RACE_EVENT_VIEW_MODE (T122), arg =
 * sub << 16 | mode << 8 | view; RaceDisplay::pollViewModeEvent (T154, u052) takes
 * it for the display of `view' (? name: the replay mode image).  u093's
 * updateMode1 posts it too.
 * Its sub value for VIEWMODE_AVANTGARDE (0 for the other modes):
 * CameraManager::mode1Style < 0.5 gives 1, else 2. */
enum {
    VIEWMODE_SUB_NONE    = 0,
    VIEWMODE_SUB_STYLE_A = 1,                   /* mode1Style < 0.5 */
    VIEWMODE_SUB_STYLE_B = 2
};

/* ScreenLayout / CameraManager::layoutFlags: WALL_* (T383). */

/* The screen of the i.LINK wall (640 x 448; the window's centre at 0, 0). */
#define SCREEN_W        640.0f
#define SCREEN_H        448.0f
#define SCREEN_HALF_W   320.0f
#define SCREEN_HALF_H   224.0f

/* The onboard mount searches of the mode handlers (OnBoardCamera, T378):
 * the flags passed as anyFlags / allFlags (? meaning: bit 3 marks the
 * mounts the mount buttons step through, bit 1 is mountFlag2). */
enum {
    MOUNT_SEARCH_MAX          = 16,             /* the size of the result array */
    MOUNT_TOWARD_ANY_FLAGS    = 9,
    MOUNT_TOWARD_ALL_FLAGS    = 2,
    MOUNT_BY_DIR_ANY_FLAGS    = 1,
    MOUNT_BY_DIR_ALL_FLAGS    = 2
};

#endif /* U092_H */
