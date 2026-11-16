/* T383_camera_manager.h - CameraManager, CameraView, CameraProjection
 * (translation unit T383, unit u092), 0x00207768..0x0020A2E0.
 *
 * The race camera (RaceBase +0x40): up to four views (split screen, i.LINK
 * battle).  Each view (CameraView) owns one of every race camera (onboard,
 * designer, passline, starting, automatic, automount, logger, matrix, zoomup,
 * heli) and points at the one in use.  T383 has the construction, init, the
 * per-frame update (the view mode from the pad, the mode handlers of
 * T386..T391, then the camera's own update), the i.LINK screen wall (the
 * window and the turn of each screen), forwarders to the current view's
 * camera, accessors, and the typeinfo functions of CameraManager and
 * RaceCameraBase.  CameraManager's other methods are defined in T385 (the
 * course demo cameras), T386 (the BGM marker clock), T387..T391 (the mode
 * handlers) and T409 (designer camera changes, cross-fades).
 *
 * Also here: CarProgress (T408, which has no header of its own: CameraManager
 * holds it by value) and RaceCameraBase (T383's typeinfo function only).
 * CameraSet (u093, T388) and CourseCameraData (u092 / u093, T385) are
 * typedefs of CameraView and CourseCamera.
 */
#ifndef GAME_T383_CAMERA_MANAGER_H
#define GAME_T383_CAMERA_MANAGER_H

#include "game/game.h"
#include "game/camera/T410_camera_base.h"          /* base CameraBase (of RaceCameraBase) */
#include "game/camera/T395_step_pair.h"            /* StepPair (held by value) */
#include "engine/pdistd/T531_random.h"               /* Random (held by value) */
#include "game/camera/T384_camera_groove_meter.h"  /* CameraGrooveMeter (held by value) */
#include "game/camera/T396_camera_request.h"       /* CameraRequest (held by value) */
#include "game/camera/T392_camera_target_selector.h"   /* CameraTargetSelector (held by value) */
#include "game/camera/T377_camera_target_info.h"   /* CameraTargetInfo (held by value) */
#include "game/camera/T393_target_chooser.h"       /* TargetChooser (held by value) */
#include "game/camera/T378_onboard_camera.h"       /* the ten cameras (held by value) */
#include "game/camera/T398_designer_camera.h"
#include "game/camera/T399_passline_camera.h"
#include "game/camera/T400_starting_camera.h"
#include "game/camera/T401_automatic_camera.h"
#include "game/camera/T402_automount_camera.h"
#include "game/camera/T403_logger_camera.h"
#include "game/camera/T404_heli_camera.h"
#include "game/camera/T405_matrix_camera.h"
#include "game/camera/T406_zoomup_camera.h"

class CourseEnv;                            /* T178 (pointers only) */
class DynamicsConductor;                    /* T305 */
class RaceEntryBase;                        /* T121 */
class RaceEntryCar;                         /* T121 */
class RaceInput;                            /* T413 */
class RaceOrganization;                     /* T125 */
class VariableCameraBase;                   /* T397 */
class GSBuffer;                             /* T514 */

/* CameraProjection, the projection window CameraManager hands to the cameras'
 * slot 11 (CameraManager +0x04): its own header (T397 includes only that). */
#include "game/camera/T383_camera_projection.h"

#include "game/camera/T408_car_progress.h"     /* CarProgress, CarProgressEntry (T408; defined here before) */

/* The argument of setScreenLayout (RaceLinkBattle, T138: the place of this
 * console's screen in the i.LINK wall).  Not polymorphic.  Size 0x14 (u047:
 * five words). */
class ScreenLayout {
public:
    int modeX;                              /* 0x00 1: nX counted from the right, 2: from the left, else 1 / 1 */
    int nX;                                 /* 0x04 */
    int modeY;                              /* 0x08 1: nY counted from the bottom, 2: from the top */
    int nY;                                 /* 0x0C */
    u32 flags;                              /* 0x10 -> CameraManager::layoutFlags */
};
EGCS_SIZE(ScreenLayout, 0x14);

/* ScreenLayout::flags / CameraManager::layoutFlags (the i.LINK screen wall;
 * u047, u092). */
enum {
    WALL_SHIFT_MASK  = 0x003,               /* 1: shift x by +shift / 2, 2: by -shift / 2 */
    WALL_SHIFT_PLUS  = 0x001,
    WALL_SHIFT_MINUS = 0x002,
    WALL_LEFT_EDGE   = 0x004,               /* the window starts at the screen's left edge (640 more left) */
    WALL_TOP_EDGE    = 0x008,               /* the window starts at its top edge (224 more up) */
    WALL_TURN_MASK   = 0x0F0,
    WALL_TURNED      = 0x010,               /* (flags & 0xF0) == 0x10: a wall turned a step a screen */
    WALL_TURN_45     = 0x100                /* the step is 45 degrees (else 60); the onboard fov 45 */
};

/* ScreenLayout::modeX / modeY (else the screen at 1 / 1). */
enum {
    WALL_COUNT_FROM_RIGHT  = 1,             /* modeX: nX is tileFromRight */
    WALL_COUNT_FROM_LEFT   = 2,             /* modeX: nX is tileFromLeft */
    WALL_COUNT_FROM_BOTTOM = 1,             /* modeY: nY is tileFromBottom */
    WALL_COUNT_FROM_TOP    = 2              /* modeY: nY is tileFromTop */
};

/* The camera kind of a view (CameraManager +0x96E0 + 8 i).  Not polymorphic.
 * Size 8 (the kinds[] stride).  u093 CameraPlayerMode (view, mode, field_6),
 * u094 ReplayViewState (viewMode, target). */
class CameraViewKind {
public:
    u32 view;                               /* 0x0 (u093) 0 the designer camera, 1 the onboard camera (the view
                                               button toggles it), 2 the logger camera (logger mode); init: 0.
                                               Unsigned (u093 matching): updateLogger's switch on it tests
                                               == 1, < 1 (beqz), == 2 with no range test, and its cycle compares
                                               it with sltiu 3 */
    s16 kind;                               /* 0x4 the view mode 0..3 (GameOptions viewMode, T394; 5: the logger) */
    s16 variant;                            /* 0x6 the camera target (GameOptions target[kind]): 0 the view's car,
                                               1 the leader, 2 automatic (T393); mode 2: 0 = manual (u093) */

    /* u093 matching: canCut and T386 read kinds[i].kind through an inline
     * member (the code builds &kinds[i] whole, (i * 8 + this) + 0x96E0, and
     * loads +4 from it; a direct kinds[i].kind folds 0x96E4 into the load).
     * u094 matching: F_0020EA98 (T394) reads kind and variant so, rebuilding
     * &kinds[view] for each read. */
    s16 getKind() { return kind; }
    s16 getVariant() { return variant; }
    u32 getView() { return view; }         /* u093: updateLogger's (and the mode handlers') view == designer test */
};
EGCS_SIZE(CameraViewKind, 0x8);

/* One view of CameraManager (CameraManager +0x300 + 0x24C0 i; u093:
 * CameraSet).  Not polymorphic.  Size 0x24C0 (the views[] stride).  The
 * cameras are 64-byte aligned (VariableCameraBase::matrix), so the class is
 * too: +0x2D8..+0x2FF and +0x2484..+0x24BF are alignment (the enhanced
 * engine keeps its zoom spring at +0x2484 there).  (u047's
 * "CameraView" at CameraManager +0x4A0 is views[0].targetInfo; its obj134 is
 * camera.) */
class CameraView {
public:
    /* Constructs Random (TargetChooser::rng, +0x2C8) and the ten cameras. */
    CameraView();                           /* 0x00207768 */

    CameraRequest        request;           /* 0x0000 (T396) what changed this frame (u092 state, u093 notice) */
    u8                   unk_014[0xC];      /* 0x0014 (not accessed) */
    CameraTargetSelector selector;          /* 0x0020 (T392) */
    u8                   unk_198[0x8];      /* 0x0198 (not accessed; or the end of selector) */
    CameraTargetInfo     targetInfo;        /* 0x01A0 (T377) what the view looks at (u092 info) */
    TargetChooser        chooser;           /* 0x0274 (T393) (u092 switcher, u093 planner) */
    VariableCameraBase  *camera;            /* 0x02D4 the camera in use (u093 current) */
    /*  0x02D8 .. 0x02FF: alignment */
    OnBoardCamera        onboard;           /* 0x0300 */
    DesignerCamera       designer;          /* 0x0480 */
    PasslineCamera       passline;          /* 0x1940 */
    StartingCamera       starting;          /* 0x1A80 */
    AutomaticCamera      automatic;         /* 0x1BC0 */
    AutomountCamera      automount;         /* 0x1D00 */
    LoggerCamera         logger;            /* 0x1E80 */
    MatrixCamera         matrix;            /* 0x1FC0 */
    ZoomupCamera         zoomup;            /* 0x2140 */
    HeliCamera           heli;              /* 0x22C0 */
    RaceInput           *input;             /* 0x2480 the controller of the view's player (setInput; RaceBase
                                               sets views[0]'s, CameraManager +0x2780) */
    /* The enhanced engine (Lupo 2 view +0x24C4, cameras_sound.md C5), in
     * GT3's alignment after `input' (the size stays 0x24C0): the zoom
     * button's spring (T395): CameraManager::init resets it, update steps
     * it, updateMode0 sets its target and reads it. */
    CameraZoomSpring     zoomSpring;        /* 0x2484 */
};
EGCS_SIZE(CameraView, 0x24C0);

/* RaceCameraBase (typeinfo __tf14RaceCameraBase 0x0020A290, T383): its own
 * header, which the race display's includes too (its type name's place). */
#include "game/camera/T383_race_camera_base.h"

#ifdef BOOT_OLD_GCC
/* #pragma interface (from here on: egcs applies it to the classes defined
 * after it), #pragma implementation in T383's source: egcs emits the
 * out-of-line copies of CameraManager's inline accessors (0x00209BC8..
 * 0x0020A228, no callers) and its vtable only there, before
 * __tf13CameraManager, as the binary has them.  The classes above stay as
 * they were: RaceCameraBase has no vtable or destructor of its own in the
 * binary, which it would get under the pragma. */
#pragma interface
#endif

/* vtable 0x0033EEB8 (15 slots: [6], [7] CameraBase's).  typeinfo
 * __tf13CameraManager 0x0020A240 (__si_type_info 0x004B2960, base
 * RaceCameraBase).  vptr +0x0000 (CameraBase has no data; u093's +0x973C was
 * wrong: the constructor stores the vtable at +0).  Size 0x9740 (RaceBase
 * +0x40; the data end at 0x9718, then the 64-byte alignment of the views;
 * the enhanced engine's start fields use 0x9718..0x971F of it).
 *
 * view / player arguments index views[]; `current' is the view the
 * forwarders use (setCurrentView before drawing each screen). */
/* The on / off members of CameraManager that egcs saw as bool (4 bytes
 * there, its own alias set; u092 matching: the store order of resetState
 * and update, and demoMode's loads and stores not ordered against the float
 * and pointer accesses round them, show it).  int under GCC 14, as before. */
#ifdef BOOT_OLD_GCC
typedef bool CameraFlag;
#else
typedef int CameraFlag;
#endif

class CameraManager : public RaceCameraBase {
public:
    /* views, rng, groove; then init(NULL, 0). */
    CameraManager();                        /* 0x00207960 */
    virtual ~CameraManager();               /* [0] +0x0C 0x00207BC0 groove, the views (last first), CameraBase's */
    virtual void applyProjection();         /* [1] +0x14 0x00209E58 the projection: the current camera's setProjection(&proj) */
    /* The view: the screen's shift (layoutFlags bits 0-1) and, in a turned
     * wall (layoutFlags & 0xF0 == 0x10), its turn; then the camera's applyView. */
    virtual void applyView();               /* [2] +0x1C 0x002094D0 */
    virtual f32  getPositionX();            /* [3] +0x24 0x00209D58 the current camera's */
    virtual f32  getPositionY();            /* [4] +0x2C 0x00209D10 */
    virtual f32  getPositionZ();            /* [5] +0x34 0x00209CC8 */
    virtual void setNearClip(f32 z);        /* [8] +0x4C 0x00209C08 proj.nearClip */
    virtual void setFarClip(f32 z);         /* [9] +0x54 0x00209C00 proj.farClip (u045: CourseEnv +0x20) */
    virtual f32  virtual_10();              /* [10] +0x5C 0x00209EE8 the current camera's targetLength[0] */
    virtual int  virtual_11();              /* [11] +0x64 0x00209DE8 the current camera's getEffect() */
    /* 1 unless the current view (camera kind 0) already follows car or its
     * onboard camera is in the car (OnBoardCamera F_00200E08). */
    virtual int  virtual_12(RaceOrganization *org, int car);   /* [12] +0x6C 0x002092B0 */
    virtual int  isDrawBackMirror();        /* [13] +0x74 0x00209268 the current camera's */
    /* The current camera's virtual_9 as 0 (0, 1: inside), 1 (2: a mount), 2
     * (outside); RaceEntryCar::updateSound picks its listening mode by it
     * (u042). */
    virtual int  getCameraKind();           /* [14] +0x7C 0x00209420 */

    /* ---- T383 ---- */
    /* The screen effects, the cross-fade, the demo and marker state reset;
     * soundLevel / mode1Style from the game options (0..1). */
    void  resetState();                     /* 0x00207CC0 */
    void  updateWindowCentred();            /* 0x00207DE0 updateWindow, then left / right -320 / 320 */
    void  updateWindow();                   /* 0x00207E20 the window of this screen in a wall of 640 x 448 screens */
    /* Where this screen sits around the wall's centre (updateWindow: tile 1
     * of a direction is the screen next to the centre, so with an even
     * count the four are the 2 x 2 screens round it, with an odd count or
     * one screen they are all the centre screen).  RaceLinkBattle shows the
     * RaceDisplay panel of that corner on that screen (setTopLeftPanel ..
     * setCenterPanel); T131 draws the mirror on the isCenterScreen one.
     * bool (u047 matching): RaceLinkBattle::virtual_8 passes the results to
     * the bool setXxxPanel parameters with no `sltu' conversion; their own
     * code is the same as with int. */
    bool  isCenterTopLeft();                /* 0x002080E0 tileFromRight == 1 && tileFromBottom == 1 */
    bool  isCenterTopRight();               /* 0x00208110 tileFromLeft == 1 && tileFromBottom == 1 */
    bool  isCenterBottomLeft();             /* 0x00208140 tileFromRight == 1 && tileFromTop == 1 */
    bool  isCenterBottomRight();            /* 0x00208170 tileFromLeft == 1 && tileFromTop == 1 */
    bool  isCenterScreen();                 /* 0x002081A0 the same code as isCenterBottomRight: the
                                               centre screen (its bottom right one for an even count) */
    void  setScreenLayout(const ScreenLayout *l);  /* 0x002081D0 (flags 0x100: the onboard field of view 45) */
    /* Also run by the constructor with org NULL.  Every view gets the kind /
     * variant of the game options (the target kept when bit `kind' (sic, not
     * bit `variant') of T394's targets is set; a demo replay: view 0 random,
     * F_0020E930) and its parts and cameras initialised on car `car'; with a
     * race, the cameras get the course data. */
    void  init(RaceOrganization *org, int car);    /* 0x00208320 */
    void  setViewCount(int n);              /* 0x00208B48 */
    void  setCurrentView(int view);         /* 0x00208B58 */
    /* RacePS2Base's colour correction pass (pfx_color_correction): out[0..1]
     * lo / hi (correction.cur, or levelLo / levelHi), out[2..4] r, g, b
     * (env +0x980..+0x988, or colour), out[5] saturation (env +0x994, or
     * saturation). */
    void  getColourCorrection(const CourseEnv *env, f32 *out);    /* 0x00208B68 */
    /* Per frame (RaceBase): the demo cameras alone when on; else the marker
     * clock, progress, and per view the kind (options / logger), the target
     * selector and chooser, the pad's kind change (race event 0x15), the mode
     * handler of the kind, the camera's update; then the markers. */
    void  update(RaceOrganization *org, DynamicsConductor *dc);   /* 0x00208BE8 */
    /* The GS state of 2D drawing: blending on, no depth test / lighting /
     * culling, a 640 x 448 orthographic projection, an identity modelview
     * (u092: ?).  RacePS2Base::drawView (T131) passes its org and buf, which
     * are not read (u045). */
    void  begin2D(RaceOrganization *org, GSBuffer *buf);   /* 0x00209130 */
    int   isOwnCar(RaceEntryCar *car);      /* 0x002091E0 the current view (mode 0) looks from car */
    int   F_00209220();                     /* 0x00209220 the current camera's virtual_9 */
    int   F_00209370(RaceOrganization *org, RaceEntryCar *car);   /* 0x00209370 virtual_12 for a car pointer */
    f32   F_00209498();                     /* 0x00209498 frameEffectLevel clamped to 0..1 */
    /* The camera switches of the mode handlers (org and input are not used). */
    int   F_00209660(int view, RaceOrganization *org, RaceInput *input);  /* 0x00209660 camera = &starting; 0 */
    int   F_00209680(int view, RaceOrganization *org, RaceInput *input);  /* 0x00209680 camera = &passline; 0 */
    int   F_002096A0(int view, RaceOrganization *org, RaceInput *input);  /* 0x002096A0 camera = &logger; 0 */
    int   F_002096C0(int view, RaceOrganization *org, RaceInput *input, int reinit);  /* 0x002096C0 camera = &onboard
                                                                                    (set up again if reinit); 0 */
    /* A random one of cam's mounts that look towards car (two or more; else
     * mount 0); u093 passes a DesignerCamera. */
    void  F_00209738(RaceOrganization *org, int car, OnBoardCamera *cam);    /* 0x00209738 */
    void  F_00209808(int mask, OnBoardCamera *cam); /* 0x00209808 a random one of the mounts in the directions mask
                                                       (OnBoardCamera::findMountsByDirection) */
    int   isWatchable(RaceEntryCar *car);   /* 0x002098D8 car_display_level(car->state) == CAR_SHOW_FULL (T289) */
    /* The next car by rank that isWatchable accepts (wraps; car if none). */
    RaceEntryCar *nextWatchableByRank(RaceEntryBase *entries, RaceEntryCar *car);  /* 0x00209900 */
    RaceEntryCar *prevWatchableByRank(RaceEntryBase *entries, RaceEntryCar *car);  /* 0x002099A0 the same backwards */
    int   nextWatchableEntry(RaceOrganization *org, int start);    /* 0x00209A40 the first accepted entry from start (wraps) */
    int   prevWatchableEntry(RaceOrganization *org, int start);    /* 0x00209B00 the same backwards */
    s16   F_00209BC8(int view);             /* 0x00209BC8 views[view].targetInfo.index[0] */
    CameraView *F_00209BE0();               /* 0x00209BE0 &views[current] */
    f32  *F_00209C10();                     /* 0x00209C10 the current camera's matrix */
    f32   F_00209C38(int i);                /* 0x00209C38 the current camera's position[i][2] */
    f32   F_00209C68(int i);                /* 0x00209C68 position[i][1] */
    f32   F_00209C98(int i);                /* 0x00209C98 position[i][0] */
    RaceEntryCar *F_00209DA0();             /* 0x00209DA0 the current view's targetInfo.car[0] */
    f32   F_00209DC0();                     /* 0x00209DC0 the current camera's gamma[0] */
    int   F_00209E30();                     /* 0x00209E30 the current camera's doppler */
    int   isTargetDistEnable();             /* 0x00209EA0 the current camera's */
    s16   F_00209F10();                     /* 0x00209F10 the current view's targetInfo.index[0] */
    void  setTargetCar(int car);            /* 0x00209F30 the current view follows car (setCar(car, 0)) */
    f32   F_00209F68();                     /* 0x00209F68 shift */
    void  F_00209F78(f32 v);                /* 0x00209F78 shift = v */
    u32   F_00209F88();                     /* 0x00209F88 layoutFlags & 3 */
    u32   getTileFromTop();                 /* 0x00209FA0 tileFromTop */
    u32   getTileFromBottom();              /* 0x00209FB0 tileFromBottom */
    u32   getTileFromLeft();                /* 0x00209FC0 tileFromLeft */
    u32   getTileFromRight();               /* 0x00209FD0 tileFromRight */
    void  setInput(int view, RaceInput *input);    /* 0x00209FE0 */
    int   getCurrentView();                 /* 0x00209FF8 */
    int   getViewCount();                   /* 0x0020A008 */
    f32   F_0020A018();                     /* 0x0020A018 crossFadeTime */
    int   F_0020A020();                     /* 0x0020A020 crossFade */
    void  F_0020A028(f32 v);                /* 0x0020A028 frameEffectLevel = v */
    int   F_0020A030();                     /* 0x0020A030 frameEffect */
    int   F_0020A038();                     /* 0x0020A038 levelOverride || colourOverride */
    void  F_0020A060(int v);                /* 0x0020A060 forcedTarget = v */
    void  F_0020A068();                     /* 0x0020A068 loggerSeek = 1 */
    s16   getViewVariant(int view);         /* 0x0020A078 kinds[view].variant */
    s16   getViewKind(int view);            /* 0x0020A090 kinds[view].kind */
    VariableCameraBase *getCamera(int view);        /* 0x0020A0A8 */
    void  setCamera(int view, VariableCameraBase *cam);    /* 0x0020A0C0 */
    RaceInput *getInput(int view);          /* 0x0020A0D8 */
    CameraView *getView(int view);          /* 0x0020A0F0 */
    TargetChooser *getSwitcher(int view);   /* 0x0020A108 &views[view].chooser */
    CameraTargetSelector *getSelector(int view);    /* 0x0020A120 */
    CameraTargetInfo *getTargetInfo(int view);      /* 0x0020A138 */
    MatrixCamera *getMatrixCamera(int view);        /* 0x0020A150 */
    HeliCamera *getHeliCamera(int view);            /* 0x0020A168 */
    LoggerCamera *getLoggerCamera(int view);        /* 0x0020A180 */
    ZoomupCamera *getZoomupCamera(int view);        /* 0x0020A198 */
    AutomountCamera *getAutomountCamera(int view);  /* 0x0020A1B0 */
    AutomaticCamera *getAutomaticCamera(int view);  /* 0x0020A1C8 */
    StartingCamera *getStartingCamera(int view);    /* 0x0020A1E0 */
    PasslineCamera *getPasslineCamera(int view);    /* 0x0020A1F8 */
    DesignerCamera *getDesignerCamera(int view);    /* 0x0020A210 */
    OnBoardCamera *getOnBoardCamera(int view);      /* 0x0020A228 */

    /* ---- T385: the course demo cameras (view 0's designer camera) ---- */
    int   startDemoCameras(RaceOrganization *org);  /* 0x0020A9C0 0: the course has no demo records */
    void  updateDemoCameras(RaceOrganization *org, DynamicsConductor *dc);   /* 0x0020AA90 (dc not used) */

    /* ---- T409 ---- */
    void  setCrossFadeFrom(VariableCameraBase *cam);    /* 0x0020AB80 over cam's fade-out time, at most 4 */
    int   canCut(int player, RaceOrganization *org, int strict);    /* 0x0020ABF0 player's camera may change now */
    /* player's designer camera over the course camera records; flags 1
     * force, 2 reset, any: cross-fade on a move. */
    int   updateDesigner(int player, RaceOrganization *org, RaceInput *input, int flags);  /* 0x0020AD18 */

    /* ---- T386: the BGM marker clock (before / after the views) ---- */
    void  updateMarkerTime(RaceOrganization *org);  /* 0x0020B200 nextMarkerTime */
    void  updateMarkers(RaceOrganization *org);     /* 0x0020B380 changeRequest, the marker flags */

    /* ---- T387..T391: the mode handlers; 1 when the camera changed (dc is
     * not used) ---- */
    int   updateStart(RaceOrganization *org, DynamicsConductor *dc, RaceInput *input, int player);  /* 0x0020B788 the
                                                                                 countdown (the input's mode 0) */
    int   updateMode0(RaceOrganization *org, DynamicsConductor *dc, RaceInput *input, int player);  /* 0x0020B8E8 kinds 0, 3 */
    int   updateMode2(RaceOrganization *org, DynamicsConductor *dc, RaceInput *input, int player);  /* 0x0020BE58 kind 2 */
    int   updateMode1(RaceOrganization *org, DynamicsConductor *dc, RaceInput *input, int player);  /* 0x0020C580 kind 1 */
    int   updateLogger(RaceOrganization *org, DynamicsConductor *dc, RaceInput *input, int player); /* 0x0020D208 logger */

    /*  vptr                                       0x0000 */
    CameraProjection  proj;                 /* 0x0004 */
    StepPair          correction;           /* 0x0020 (T395) the colour correction's lo / hi of the course
                                               (CourseEnv +0x98C / +0x990; u092 fog) */
    u8                unk_03C[0x200];       /* 0x003C */
    /* The colour correction overrides (replays: either one turns the pass on;
     * mode 1 picks them at a cut). */
    CameraFlag        levelOverride;        /* 0x023C 1: levelLo / levelHi instead of the course's (u092 fogOverride) */
    f32               levelLo;              /* 0x0240 0 */
    f32               levelHi;              /* 0x0244 1.0 */
    int               colourOverride;       /* 0x0248 1: colour / saturation instead of the course's (u093 tint) */
    f32               colour[3];            /* 0x024C r, g, b (0; mode 1: a tint of g_002DED30) */
    f32               saturation;           /* 0x0258 1.0 (u093 tintLevel) */
    CameraFlag        frameEffect;          /* 0x025C RacePS2Base: gfx_frame_effect(5, min(F_00209498(), 0.8)) */
    f32               frameEffectLevel;     /* 0x0260 (u093: 0.5..2.0, fades by 0.02 while frameEffectFading) */
    CameraFlag        frameEffectFading;    /* 0x0264 */
    CameraFlag        crossFade;            /* 0x0268 RacePS2Base captures an afterimage and fades it (u045) */
    f32               crossFadeTime;        /* 0x026C seconds: 0.5 each frame (update), 2.0 at a demo change */
    CameraFlag        loggerSeek;           /* 0x0270 the logger sets 1 after a seek (F_0020A068) */
    int               forcedTarget;         /* 0x0274 the car to look at next (logger), -1: none */
    CarProgress       progress;             /* 0x0278 (T408) */
    /*  0x02D8 .. 0x02FF: alignment */
    CameraView        views[4];             /* 0x0300 */
    int               frame;                /* 0x9600 frames of update */
    s16               targetReset[4];       /* 0x9604 per view: re-target request (init and F_0020EBA0: 1) */
    int               autoTimer;            /* 0x960C frames since the last automatic change (modes 0 / 3) */
#ifdef BOOT_OLD_GCC
    /* One byte (sb / lbu) that is no char: the course demo flag was a bool
     * bit-field (u092 matching: update's demoMode load is not ordered after
     * the float store before it, nor startDemoCameras' store before the
     * course data load after it; a char access would be). */
    CameraFlag        demoMode : 8;         /* 0x9610 the course demo cameras run (T385) */
#else
    u8                demoMode;             /* (the same byte) */
#endif
    int               demoTimer;            /* 0x9614 frames on the current demo record */
    int               demoIndex;            /* 0x9618 the current demo record */
    Random            rng;                  /* 0x961C */
    int               changeRequest;        /* 0x9620 the cut request of the BGM markers (1..4; init 1) */
    f32               nextMarkerTime;       /* 0x9624 seconds to the next marker that can cut (init 4.0) */
    s8                marker1Enabled;       /* 0x9628 markers 6 / 7 set it, 4 / 5 clear it: gates marker 1's cut (T386) */
    s8                marker2Random;        /* 0x9629 5 / 7 set, 4 / 6 clear: marker 2 then disarms itself with 50% */
    s8                marker1Armed;         /* 0x962A marker channel 1 may cut (u093 flag_962A) */
    s8                unk_962B;             /* 0x962B only ever cleared */
    s8                marker2Armed;         /* 0x962C marker channel 2 may cut (u093 flag_962C; resetState 1) */
    s8                soundLevelApply;      /* 0x962D soundLevel is to be applied (set by markers 8 / 9, cleared every frame) */
    s8                soundLevelFull;       /* 0x962E 1: apply 1.0 instead of soundLevel (marker 8, resetState) */
    /*  0x962F: alignment */
    f32               soundLevel;           /* 0x9630 GameOptions replaySoundLevel, 0..1: the car sound factor
                                               g_003521E4 (T176) of mode 1 (up / down by 0.1) (u092 replaySeLevel) */
    f32               mode1Style;           /* 0x9634 GameOptions +0x118, 0..1: >= 0.5 the other style of mode 1
                                               (T386 F_0020B1D8) */
    int               markerTimeout;        /* 0x9638 frames without a near marker before a cut (60; 120) */
    CameraGrooveMeter groove;               /* 0x963C (T384) */
    int               current;              /* 0x96D8 the view the forwarders use */
    int               viewCount;            /* 0x96DC the number of views (2 in a split battle) */
    CameraViewKind    kinds[4];             /* 0x96E0 */
    /* i.LINK wall: this screen's place counted out from the wall's centre
     * (updateWindow; 1: next to the centre, or the centre screen; 0: the
     * other one of the pair is used).  tileFromRight n: the n-th column left
     * of the centre (the window n - 1 screens further left), tileFromLeft:
     * right of it, tileFromBottom: above it, tileFromTop: below it. */
    u32               tileFromRight;        /* 0x9700 */
    u32               tileFromLeft;         /* 0x9704 */
    u32               tileFromTop;          /* 0x9708 */
    u32               tileFromBottom;       /* 0x970C */
    u32               layoutFlags;          /* 0x9710 ScreenLayout flags: bits 0-1 shift x by +-shift / 2, 2: 640 more
                                               left, 3: 224 more up, 4-7 == 1: a wall turned 60 (0x100: 45) degrees
                                               a screen */
    f32               shift;                /* 0x9714 0.2 (the enhanced engine: 0.063, Lupo 2's StereoLRdistance,
                                               cameras_sound.md C8) */
    /* The enhanced engine (Lupo 2, cameras_sound.md C6), in GT3's alignment
     * after the data (the size stays 0x9740): view mode 0's race start
     * sequence (updateMode0, T388). */
    int               startClock;           /* 0x9718 frames since the countdown ended (update: +1 a frame while
                                               org->countDown is 0, at most 60000; init 0; Lupo 2 +0x9504) */
    CameraFlag        startShown;           /* 0x971C the starting camera ran last frame (updateMode0; init 0;
                                               Lupo 2 +0x9500) */
    /*  0x9720 .. 0x973F: alignment */
};
EGCS_SIZE(CameraManager, 0x9740);

/* CameraManager's accessors (and the virtual getPosition*, virtual_10 / 11,
 * applyProjection, isTargetDistEnable) are inline: none has a caller, the
 * users expand them (u092).  Defined in the reverse order of their
 * addresses: egcs emits the #pragma implementation copies in reverse
 * definition order. */
inline OnBoardCamera *CameraManager::getOnBoardCamera(int view) { return &views[view].onboard; }  /* 0x0020A228 */
inline DesignerCamera *CameraManager::getDesignerCamera(int view) { return &views[view].designer; }  /* 0x0020A210 */
inline PasslineCamera *CameraManager::getPasslineCamera(int view) { return &views[view].passline; }  /* 0x0020A1F8 */
inline StartingCamera *CameraManager::getStartingCamera(int view) { return &views[view].starting; }  /* 0x0020A1E0 */
inline AutomaticCamera *CameraManager::getAutomaticCamera(int view) { return &views[view].automatic; }  /* 0x0020A1C8 */
inline AutomountCamera *CameraManager::getAutomountCamera(int view) { return &views[view].automount; }  /* 0x0020A1B0 */
inline ZoomupCamera *CameraManager::getZoomupCamera(int view) { return &views[view].zoomup; }  /* 0x0020A198 */
inline LoggerCamera *CameraManager::getLoggerCamera(int view) { return &views[view].logger; }  /* 0x0020A180 */
inline HeliCamera *CameraManager::getHeliCamera(int view) { return &views[view].heli; }  /* 0x0020A168 */
inline MatrixCamera *CameraManager::getMatrixCamera(int view) { return &views[view].matrix; }  /* 0x0020A150 */
inline CameraTargetInfo *CameraManager::getTargetInfo(int view) { return &views[view].targetInfo; }  /* 0x0020A138 */
inline CameraTargetSelector *CameraManager::getSelector(int view) { return &views[view].selector; }  /* 0x0020A120 */
inline TargetChooser *CameraManager::getSwitcher(int view) { return &views[view].chooser; }  /* 0x0020A108 */
inline CameraView *CameraManager::getView(int view) { return &views[view]; }  /* 0x0020A0F0 */
inline RaceInput *CameraManager::getInput(int view) { return views[view].input; }  /* 0x0020A0D8 */
inline void CameraManager::setCamera(int view, VariableCameraBase *cam) { views[view].camera = cam; }  /* 0x0020A0C0 */
inline VariableCameraBase *CameraManager::getCamera(int view) { return views[view].camera; }  /* 0x0020A0A8 */
inline s16 CameraManager::getViewKind(int view) { return kinds[view].getKind(); }  /* 0x0020A090 */
inline s16 CameraManager::getViewVariant(int view) { return kinds[view].getVariant(); }  /* 0x0020A078 */
inline void CameraManager::F_0020A068() { loggerSeek = 1; }  /* 0x0020A068 */
inline void CameraManager::F_0020A060(int v) { forcedTarget = v; }  /* 0x0020A060 */
/* 0x0020A038 */
inline int CameraManager::F_0020A038()
{
    int result = 0;

    if (levelOverride != 0 || colourOverride != 0)
        result = 1;
    return result;
}
inline int CameraManager::F_0020A030() { return frameEffect; }  /* 0x0020A030 */
inline void CameraManager::F_0020A028(f32 v) { frameEffectLevel = v; }  /* 0x0020A028 */
inline int CameraManager::F_0020A020() { return crossFade; }  /* 0x0020A020 */
inline f32 CameraManager::F_0020A018() { return crossFadeTime; }  /* 0x0020A018 */
inline int CameraManager::getViewCount() { return viewCount; }  /* 0x0020A008 */
inline int CameraManager::getCurrentView() { return current; }  /* 0x00209FF8 */
inline void CameraManager::setInput(int view, RaceInput *input) { views[view].input = input; }  /* 0x00209FE0 */
inline u32 CameraManager::getTileFromRight() { return tileFromRight; }  /* 0x00209FD0 */
inline u32 CameraManager::getTileFromLeft() { return tileFromLeft; }  /* 0x00209FC0 */
inline u32 CameraManager::getTileFromBottom() { return tileFromBottom; }  /* 0x00209FB0 */
inline u32 CameraManager::getTileFromTop() { return tileFromTop; }  /* 0x00209FA0 */
inline u32 CameraManager::F_00209F88() { return layoutFlags & WALL_SHIFT_MASK; }  /* 0x00209F88 */
inline void CameraManager::F_00209F78(f32 v) { shift = v; }  /* 0x00209F78 */
inline f32 CameraManager::F_00209F68() { return shift; }  /* 0x00209F68 */
inline void CameraManager::setTargetCar(int car) { views[current].targetInfo.setCar(car, 0); }  /* 0x00209F30 */
inline s16 CameraManager::F_00209F10() { return views[current].targetInfo.index[0]; }  /* 0x00209F10 */
inline f32 CameraManager::virtual_10() { return views[current].camera->targetLength[0]; }  /* 0x00209EE8 */
inline int CameraManager::isTargetDistEnable() { return views[current].camera->isTargetDistEnable(); }  /* 0x00209EA0 */
inline void CameraManager::applyProjection() { views[current].camera->setProjection(&proj); }  /* 0x00209E58 */
inline int CameraManager::F_00209E30() { return views[current].camera->doppler; }  /* 0x00209E30 */
inline int CameraManager::virtual_11() { return views[current].camera->getEffect(); }  /* 0x00209DE8 */
inline f32 CameraManager::F_00209DC0() { return views[current].camera->gamma[0]; }  /* 0x00209DC0 */
inline RaceEntryCar *CameraManager::F_00209DA0() { return views[current].targetInfo.car[0]; }  /* 0x00209DA0 */
inline f32 CameraManager::getPositionX() { return views[current].camera->getPositionX(); }  /* 0x00209D58 */
inline f32 CameraManager::getPositionY() { return views[current].camera->getPositionY(); }  /* 0x00209D10 */
inline f32 CameraManager::getPositionZ() { return views[current].camera->getPositionZ(); }  /* 0x00209CC8 */
inline f32 CameraManager::F_00209C98(int i) { return views[current].camera->position[i][0]; }  /* 0x00209C98 */
inline f32 CameraManager::F_00209C68(int i) { return views[current].camera->position[i][1]; }  /* 0x00209C68 */
inline f32 CameraManager::F_00209C38(int i) { return views[current].camera->position[i][2]; }  /* 0x00209C38 */
inline f32 *CameraManager::F_00209C10() { return &views[current].camera->matrix[0][0]; }  /* 0x00209C10 */
inline void CameraManager::setNearClip(f32 z) { proj.nearClip = z; }  /* 0x00209C08 */
inline void CameraManager::setFarClip(f32 z) { proj.farClip = z; }  /* 0x00209C00 */
inline CameraView *CameraManager::F_00209BE0() { return &views[current]; }  /* 0x00209BE0 */
inline s16 CameraManager::F_00209BC8(int view) { return views[view].targetInfo.index[0]; }  /* 0x00209BC8 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself (and rejects hand declarations) */
extern "C" {
void *__tf13CameraManager(void);            /* 0x0020A240 */
void *__tf14RaceCameraBase(void);           /* 0x0020A290 */
extern u32 __ti13CameraManager[3];          /* 0x004B2960 its __si_type_info (only T383 uses it) */
extern u32 __ti14RaceCameraBase[3];         /* 0x004B1200 its __si_type_info (only T383 uses it) */
}
#endif

#endif /* GAME_T383_CAMERA_MANAGER_H */
