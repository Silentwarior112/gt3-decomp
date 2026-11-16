/* T154_race_display.h - RaceDisplay, the head-up display of a race, and its
 * interface RaceDisplayBase (translation unit T154, units u051 / u052; one
 * source file, src/game/race/display/T154_race_display.cpp), 0x001909E0..0x00195B88.
 *
 * RaceDisplay holds every element of the display by value (the T157 parts,
 * the T160..T163 meters, the mini map) and runs them: virtual_0 sets them up
 * for a race, virtual_2 updates them each frame from the car shown and the
 * display events of the race event queue (the poll* methods; the events are
 * T194_race_display_event.h's), virtual_3 draws the enabled ones of the
 * enabled panels (RD_PANEL_*); setLayout places them from the race mode's
 * records of g_002F00C0, the two tachometer layouts are g_002BC080 /
 * g_002BC120.  The display images (race/<country>/display.imgs) and the
 * font font/menu.fnt are shared by the live displays (reference counted).
 * RaceBasicWithRaceDisplay (T132) holds one, RaceSplitDisplay (T155) two;
 * RaceTimeTrialDisplay (T141), RaceFreeRunDisplay (T143) and
 * RaceLicenseDisplay (T147) derive from it.
 *
 * Also here: RaceMiniMap (its methods are T164's; the class is in
 * T164_race_minimap.h, which this header reads where the class was), the
 * vertex / UV records of the display's strips, and RaceCarDynamics, u051's
 * name of the car's RaceCarState (T302).
 *
 * The class set and its order (the T154 merge): egcs writes the type names of
 * the polymorphic classes an object sees without a key function (and of
 * those whose vtables it emits) in the reverse of their definition order, so
 * the include order shows in every includer's .rodata.  Retail's T155, T157,
 * T160 and T164 have, after their own names, MiniMap, OnboardSpeedmeter, the
 * two round tachometers, MeterBase, RaceCameraBase, UnitArena<64>,
 * ValueDisplayBase (T157), RaceDisplayBase; T161 / T162 / T163, which
 * include their meter header first, have MeterBase (and their own) after
 * RaceDisplayBase's; none has the difference events (T194's header, which
 * T154's source includes after this one).  Hence: RaceDisplayBase, the
 * display parts (T157), UnitArena<64> (T153) and RaceCameraBase (T383), the
 * meters (T160 with RaceMeterBase, T161, T163, T162 RaceOnboardSpeedmeter),
 * RaceMiniMap, RaceDisplay.  (Before the merge the meter base and the
 * speedometer were defined here and the events included at the top: the
 * includers' type names were 0x30..0x60 bytes off retail's.)
 *
 * Names: u051 / u052 and the vtable symbols (innerPanel .. getMirrorObject,
 * render_2nd, set, setScale); see ref/pass2/groups/h26/notes.md. */
#ifndef GAME_T154_RACE_DISPLAY_H
#define GAME_T154_RACE_DISPLAY_H

#include "game/game.h"
#include "game/race/display/T156_race_display_object.h"  /* base RaceDisplayObjectBase */

class RaceOrganization;                     /* T125_race_organization.h (pointers / references only) */
class RaceEntryCar;                         /* T121_race_entry.h */
class RaceCarState;                         /* T302_car_state.h */
class GSBuffer;                             /* T514_gs_buffer.h */
struct CarIcon;                             /* T184_car_icon_maker.h */
struct ImagePack;                           /* T506_image_pack.h */
struct Tex1Image;                           /* G001_flare.h */
struct RaceMiniMapData;                     /* T164 (the course file's mini map record; no header) */
class type_info;

/* The typeinfo functions (vtable entry 0), declared before the classes
 * for GCC 14 only: egcs declares a polymorphic class's __tf<mangled> itself
 * when the class is complete and rejects a later extern "C" declaration; an
 * earlier one is the declaration it uses, and egcs then generates no
 * typeinfo function (u045 / u046).  (RaceMeterBase's: T160_race_meters.h;
 * RaceOnboardSpeedmeter's: T162_race_onboard_speedmeter.h; the difference
 * events': T194_race_display_event.h.) */
#ifndef BOOT_OLD_GCC
extern "C" {
/* No vtable of its own: emitted wherever needed (an identical copy without
 * callers at 0x00196450, T155). */
const type_info &__tf15RaceDisplayBase(void);               /* 0x00195B48 __user_type_info 0x003532E8 */
/* (__tf11RaceMiniMap: T164_race_minimap.h) */
const type_info &__tf11RaceDisplay(void);                   /* 0x00195978 */
}
#endif

/* u051 / u052 / u079 / u081 / u087's name of the car (RaceEntryCar +0x18):
 * the same 0x16DC-byte object as RaceCarState (T302). */
typedef RaceCarState RaceCarDynamics;

/* ======================================================================== */
/* The strips of the display (G001 F_002501A8)                              */
/* ======================================================================== */

/* A vertex of the strips (12 bytes; z is stored as an int 0). */
struct RaceDisplayVertex {
    f32 x;                                  /* 0x0 */
    f32 y;                                  /* 0x4 */
    int z;                                  /* 0x8 */
};
EGCS_SIZE(RaceDisplayVertex, 0xC);

/* A texture coordinate pair (8 bytes). */
struct RaceDisplayUV {
    f32 u;                                  /* 0x0 */
    f32 v;                                  /* 0x4 */
};
EGCS_SIZE(RaceDisplayUV, 0x8);

/* ======================================================================== */
/* RaceDisplayBase                                                          */
/* ======================================================================== */

/* The interface of a race display: RaceBasic +0xC888 points to one,
 * RaceDisplay or RaceSplitDisplay (T155), whose typeinfo base it is
 * ("15RaceDisplayBase").  No data: vptr +0.  Its 16 slots are pure (no
 * vtable of its own is emitted); no virtual destructor (RaceDisplay's is its
 * slot 21, RaceSplitDisplay's its slot 16).  Slot names and signatures of
 * RaceDisplay's vtable 0x0032E1D8.  Size 4.  (h24 declared it in T133.) */
class RaceDisplayBase {
public:
    /* Set-up for a race: the organization, the mode id, the replay mode. */
    virtual void virtual_0(RaceOrganization *org, int raceMode, int replay) = 0;  /* [ 0] +0x0C */
    virtual void virtual_1() = 0;                           /* [ 1] +0x14 (RaceBasic::virtual_35) */
    virtual void virtual_2() = 0;                           /* [ 2] +0x1C the per-frame update */
    virtual void virtual_3(GSBuffer &buf) = 0;              /* [ 3] +0x24 the draw */
    virtual void innerPanel(bool on) = 0;                   /* [ 4] +0x2C */
    virtual bool innerPanel() const = 0;                    /* [ 5] +0x34 */
    virtual void setOrganization(RaceOrganization &org) = 0;/* [ 6] +0x3C */
    virtual void setPlayerCarNumber(int n) = 0;             /* [ 7] +0x44 */
    virtual void setTargetCarNumber(int n) = 0;             /* [ 8] +0x4C the car the view follows */
    virtual void virtual_9(int mount) = 0;                  /* [ 9] +0x54 the view camera's getMount() (slot 13; -1 none) */
    virtual void setTopLeftPanel(bool on) = 0;              /* [10] +0x5C */
    virtual void setTopRightPanel(bool on) = 0;             /* [11] +0x64 */
    virtual void setBottomLeftPanel(bool on) = 0;           /* [12] +0x6C */
    virtual void setBottomRightPanel(bool on) = 0;          /* [13] +0x74 */
    virtual void setCenterPanel(bool on) = 0;               /* [14] +0x7C */
    virtual void *getMirrorObject() = 0;                    /* [15] +0x84 */
};
EGCS_SIZE(RaceDisplayBase, 4);

/* ======================================================================== */
/* T154 data and free functions                                             */
/* ======================================================================== */

/* The shared resources of the race displays (T154 .sdata / .sbss; u051). */
extern ImagePack  *g_00351FA4;              /* 0x00351FA4 gp-0x5A4C race/<country>/display.imgs */
extern int         g_00351FA8;              /* 0x00351FA8 gp-0x5A48 the font/menu.fnt handle, -1: none */
extern int         g_00351FAC;              /* 0x00351FAC gp-0x5A44 the live RaceDisplays */
extern int         g_00351FB0;              /* 0x00351FB0 gp-0x5A40 the start countdown runs */
extern int         g_00351FB4;              /* 0x00351FB4 gp-0x5A3C the start event (3) came this frame */
/* The history of event 0x16 (the replay record is full): bit 0 this frame,
 * bit 1 the frame before, ...: race_display_poll_events shifts it left each
 * frame and sets bit 0 when it takes an event 0x16 (0x00192938);
 * RaceDisplay::virtual_0 clears it; RaceFreeRun::onExit (0x001897D8, T142),
 * RaceMachineTest::onExit (0x0018CFB0, T148) set it to 3 (seen) with
 * RaceDisplay::ackReplayRecorded (inline, copy 0x00195820).  Read with lbu / lb
 * (u048 / u049: "a request to the race loop"; the plan had put it in
 * T140). */
extern u8          g_00351FB8;              /* 0x00351FB8 gp-0x5A38 */
extern const char *g_00351FBC;              /* 0x00351FBC gp-0x5A34 "font/menu.fnt" */

/* An entry of the meter layouts (T154 .bss, built by the static initializer
 * initializer of T154, 0x00195230): {member, region, alignment}, ended by a
 * null member.  Defined in T154 with initializers: egcs builds them at run
 * time (the pointers to members are not constant to it), each entry
 * memset to zero, then its fields. */
class RaceDisplay;
struct RaceMeterLayout {
    /* egcs stores a pointer to a data member as its offset + 1 (0: the
     * null pointer); layoutMeters reads it again for each use, as
     * this->*member does. */
    RaceDisplayObjectBase RaceDisplay::*member; /* 0x0 */
    s16 x;                                  /* 0x4 */
    s16 y;                                  /* 0x6 */
    s16 w;                                  /* 0x8 */
    s16 h;                                  /* 0xA */
    u8  alignX;                             /* 0xC (u052 alignH) */
    u8  alignY;                             /* 0xD (u052 alignV) */
};
EGCS_SIZE(RaceMeterLayout, 0x10);
extern RaceMeterLayout g_002BC080[10];      /* 0x002BC080 layout 0: the onboard tachometer, the speedometer */
extern RaceMeterLayout g_002BC120[9];       /* 0x002BC120 layout 1: the remote tachometer */

/* The texture coordinates of the four corners of a w x h image in its
 * power-of-two texture, from the prim buffer. */
RaceDisplayUV *race_display_tex_uvs(int w, int h);          /* 0x001922F8 */
/* tex centred on (x, y) at its own size (CLAMP MAXU / MAXV + 1). */
void race_display_draw_tex(Tex1Image *tex, f32 x, f32 y);   /* 0x001923F8 */
void race_display_draw_rect(int x, int y, int w, int h);    /* 0x00192528 untextured; no callers */
/* F_001927C0 .. F_001927D8, the four empty functions (T367 calls three):
 * T154_race_display_entry.h (u086's data pass). */
#include "game/race/display/T154_race_display_entry.h"
/* Every frame before the displays (the races): the countdown event (2; the
 * beep 0x14 at each second below 4 s), the start event (3; sound 0x15,
 * g_00351FB4) and the events 0x16 (g_00351FB8). */
void race_display_poll_events(RaceOrganization *org);       /* 0x00192880 */
/* Every frame after the displays: drops the display events left (0xB, 0xC,
 * 0xE, 0x15, 0xF, 0x10, 0x12, 5). */
void race_display_drop_events(RaceOrganization *org);       /* 0x00192970 */
/* o by the current layout record (location, size, alignment), enabled;
 * nothing for NULL. */
void race_display_apply_layout(RaceDisplayObjectBase *o);   /* 0x00195168 */
/* The named image of the display images (image_pack_find(g_00351FA4,
 * name); u051 F_00195208, u052 int). */
Tex1Image *race_display_find_image(const char *name);       /* 0x00195208 */
/* (T154's static initializer, .ctors entry 0x0028F130, is egcs's: the two
 * meter layouts.  0x001957E0 / 0x00195820, g_00351FA4 / g_00351FB8 = 3:
 * RaceDisplay's static inline members getImages / ackReplayRecorded.) */

/* ======================================================================== */
/* The elements RaceDisplay holds                                           */
/* ======================================================================== */

#include "game/race/core/T127_race_records.h"         /* SpeedRecord (held by value) */
#include "game/race/display/T157_race_display_parts.h"   /* the display parts (held by value) */
/* UnitArena<64> (T153) and RaceCameraBase (T383): retail's type names have
 * them here, after the display parts' and before the meters' (see the top).
 * T153's template typeinfo stays declared for egcs (no name written) unless
 * GAME_T153_EMITS_TYPEINFO is set when T153 is first read. */
#define GAME_T153_EMITS_TYPEINFO
#include "game/race/core/T153_race_course.h"          /* UnitArena<64> */
/* RaceOrganization: retail T125 (its key function's file, the only object
 * with its type name) has it between UnitArena<64>'s and RaceCameraBase's
 * names (0x00327840; u043's data pass).  No other includer's object changes
 * (it emits nothing of its own here). */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization */
#include "game/camera/T383_race_camera_base.h"     /* RaceCameraBase */
#include "game/race/display/T160_race_meters.h"          /* base RaceMeterBase, RaceRoundMeter */
#include "game/race/display/T161_race_tachometer.h"      /* the tachometers (held by value) */
#include "game/race/display/T163_race_boostmeter.h"      /* RaceBoostmeter (held by value) */
#include "game/race/display/T162_race_onboard_speedmeter.h"  /* RaceOnboardSpeedmeter (held by value) */

/* ======================================================================== */
/* RaceMiniMap                                                              */
/* ======================================================================== */

/* The course map with a dot per car (RaceDisplay +0x20): its own header
 * (u059's data pass: retail's T179 saw it without the rest of this
 * header), read here, where the class was defined. */
#include "game/race/display/T164_race_minimap.h"         /* RaceMiniMap (held by value) */

/* ======================================================================== */
/* RaceDisplay                                                              */
/* ======================================================================== */

/* panels bits (setTopLeftPanel .. setCenterPanel) */
#define RD_PANEL_TOP_LEFT       0x01        /* the map, the rank */
#define RD_PANEL_TOP_RIGHT      0x02        /* the times */
#define RD_PANEL_BOTTOM_LEFT    0x04        /* tyre wear, accel / brake, steering */
#define RD_PANEL_BOTTOM_RIGHT   0x08        /* the car name, the second event board */
#define RD_PANEL_CENTER         0x10        /* the meters, the messages */

/* vtable 0x0032E1D8 (22 slots: [16..21] new), typeinfo __tf11RaceDisplay
 * 0x00195978 (si, base RaceDisplayBase).  vptr +0 (RaceDisplayBase's);
 * 0xA47C bytes (RaceSplitDisplay +4 / +0xA480; RaceBasicWithRaceDisplay
 * +0xC8C0, rounded up there to 0xA480).  The elements are constructed in
 * order and destroyed in reverse (their destructors are expanded in
 * ~RaceDisplay). */
class RaceDisplay : public RaceDisplayBase {
public:
    /* The members; org, icons and activeTacho NULL; panels 0x1F, inner 1;
     * loadResources() for the first display alive. */
    RaceDisplay();                          /* 0x001909E0 */
    /* The set-up for a race: the organization, the race mode (the split
     * battles 7 / 8 by the screen mode; a race of 10 laps or more or with a
     * limitTime (RaceEvent) shown as RACE_MODE_ENDULANCE, except free
     * practice), the meters, the record times, the mode message, the title
     * (its timing by DynamicsConductor::getStartingFormat). */
    virtual void virtual_0(RaceOrganization *org, int raceMode, int replay);  /* [ 0] +0x0C 0x001918D8 */
    virtual void virtual_1();               /* [ 1] +0x14 0x00192120 empty */
    virtual void virtual_2();               /* [ 2] +0x1C 0x00192A58 the per-frame update */
    virtual void virtual_3(GSBuffer &buf);  /* [ 3] +0x24 0x001934B8 the draw */
    /* [4] .. [8], [10] .. [15]: inline, below */
    /* on: the view camera's car mount (VariableCameraBase::getMount, -1
     * none; u046): mount 0 variant 0, else 1. */
    virtual void virtual_9(int mount);      /* [ 9] +0x54 0x00192128 layoutVariant = mount != 0 */
    /* A car finished: once, for car myCar of this display, finished = 1 and
     * the events 0x14 (0x13) and 8; in the split screen battles (display
     * modes 7, 8) "PLAYER <n> WIN!" when that car is first; otherwise
     * put_finish_display() when finishDisplay. */
    virtual void virtual_16(RaceOrganization *org, int car, int myCar);   /* [16] +0x8C 0x00193DA8 */
    virtual void virtual_17();              /* [17] +0x94 0x001947E0 empty */
    virtual void put_start_display();       /* [18] +0x9C 0x001947E8 live only: "@start" for 1 s */
    virtual void put_finish_display();      /* [19] +0xA4 0x00194838 "@finish" */
    /* The lap time of the car shown on the event board in the big font
     * (pollLapTimeEvents; RaceLicenseDisplay's override forwards it only
     * in replays).  (u052 virtual_20.) */
    virtual void showLapTime(int time);     /* [20] +0xAC 0x001944A0 */
    /* The last display alive releases the shared resources. */
    virtual ~RaceDisplay();                 /* [21] +0xB4 0x00191300 */

    /* u051 */
    void loadResources();                   /* 0x00191830 display.imgs and font/menu.fnt (once) */
    void releaseResources();                /* 0x001918A0 */
    /* displayMode from raceMode and replay (RACE_MODE_REPLAY / _DEMO). */
    void updateDisplayMode();               /* 0x00192010 */
    void setDisplayCar(int car);            /* 0x00192140 setupCarMeters when it changes */
    /* The tachometers' and the speedometer's scales, the boost meter shown
     * for a turbo car. */
    void setupCarMeters(RaceEntryCar *car); /* 0x00192190 */
    void drawCountdown(int frames);         /* 0x001925E8 "@count<n>" below 4 s (live) */
    void drawTimeLimit(int seconds);        /* 0x00192678 "Time Limit", "GAME OVER" */
    /* The car's warning: 1, 2 "WRONG WAY", 4, 5, 7..10, 12 "AUTO DRIVE".
     * An int (the caller passes DynCarState::msgKind, a byte; retail's
     * function does not narrow it again). */
    void showWarning(int warning);          /* 0x001927E0 */

    /* u052 */
    /* The current lap, the total and the best lap of the car shown. */
    void updateLapTimes(RaceEntryCar *car); /* 0x00193040 */
    int  findCarByRank(int rank);           /* 0x001930D0 the car of rank, -1: none */
    /* The pending display events of a class (RaceDisplayEventBase::peek)
     * for car `car', to the event board / the time displays. */
    void pollLapTimeEvents(RaceOrganization *org, int car);     /* 0x00193118 */
    void pollCheckPointEvents(RaceOrganization *org, int car);  /* 0x00193260 */
    /* The first event 0x15, the replay view mode change (CameraManager::
     * update T383 / updateMode1 T390: arg (style << 16) | (mode << 8) |
     * view): onViewModeChanged(mode, style) when its view is this display's
     * number, else posted again for the other display.  (u052 pollViewModeEvent.) */
    void pollViewModeEvent();               /* 0x001933D0 */
    void F_00193468();                      /* 0x00193468 empty, no callers (arguments unknown) */
    void F_00193470();                      /* 0x00193470 empty, no callers (arguments unknown) */
    void onViewModeChanged(int mode, int style);    /* 0x00193478 empty (u052 onViewModeChanged) */
    const char *F_00193480(int kind);       /* 0x00193480 1 "SOFT", 2 "HARD", else ""; no callers */
    void pollLapDiffEvents(RaceOrganization *org, int car);     /* 0x00193F58 */
    void pollTimeDiffEvents(RaceOrganization *org, int car);    /* 0x00194010 */
    void pollSpeedEvents(RaceOrganization *org, int car);       /* 0x00194110 */
    void pollInformationEvents(RaceOrganization *org);          /* 0x001941C0 */
    void showLeaderIcon();                  /* 0x001943E8 */
    void showCarIcon(int car);              /* 0x00194420 carIcon0 */
    void showLapDiff(int laps);             /* 0x00194500 "+<n> Laps" */
    void showTimeDiff(int diff, int iconCar);   /* 0x001945B0 */
    void showSpeed(f32 speed);              /* 0x001946E0 */
    void showReplayRecorded();              /* 0x00194788 (a fresh event 0x16, live only) */
    /* The objects of the display mode's layout of this variant (its
     * racedetail.db records): 1 nothing to do (loaded) or done, 0 no
     * layout. */
    int  setLayout(int variant);            /* 0x00194888 */
    void disableAll();                      /* 0x00194BF8 */
    void layoutTimeDisplay(int variant);    /* 0x00194DF0 setLayout's record type 11 */
    void layoutMeters(int kind);            /* 0x00194FF8 tachometer layout 0 (onboard) / 1 (remote) */

    /* The accessors, inline: egcs emits their out-of-line copies with
     * RaceDisplay's vtable, in T154 (the object of the key function
     * virtual_0), in the reverse of this order: 0x001957E0..0x00195970,
     * right before __tf11RaceDisplay.  The races call them on their display
     * (RaceArcade::virtual_43's setCarIcons, RaceMachineTest::onExit's
     * setFinishDisplay: retail keeps the display's address as their
     * `this'); the copies have no callers but the vtables. */
    virtual void setOrganization(RaceOrganization &o) { org = &o; }  /* [ 6] +0x3C 0x00195970 */
    RaceOrganization *getOrganization() { return org; }     /* 0x00195968 */
    virtual void innerPanel(bool on) { inner = on; }        /* [ 4] +0x2C 0x00195958 */
    virtual bool innerPanel() const { return inner; }       /* [ 5] +0x34 0x00195948 */
    /* The panel switches: the bit set or cleared (the others kept, the
     * byte sign-extended on the way: retail's sll / sra). */
    virtual void setTopLeftPanel(bool on) {                 /* [10] +0x5C 0x00195920 */
        s8 others = panels & ~RD_PANEL_TOP_LEFT;
        panels = others | (on ? RD_PANEL_TOP_LEFT : 0);
    }
    virtual void setTopRightPanel(bool on) {                /* [11] +0x64 0x001958F8 */
        s8 others = panels & ~RD_PANEL_TOP_RIGHT;
        panels = others | (on ? RD_PANEL_TOP_RIGHT : 0);
    }
    virtual void setBottomLeftPanel(bool on) {              /* [12] +0x6C 0x001958D0 */
        s8 others = panels & ~RD_PANEL_BOTTOM_LEFT;
        panels = others | (on ? RD_PANEL_BOTTOM_LEFT : 0);
    }
    virtual void setBottomRightPanel(bool on) {             /* [13] +0x74 0x001958A8 */
        s8 others = panels & ~RD_PANEL_BOTTOM_RIGHT;
        panels = others | (on ? RD_PANEL_BOTTOM_RIGHT : 0);
    }
    virtual void setCenterPanel(bool on) {                  /* [14] +0x7C 0x00195880 */
        s8 others = panels & ~RD_PANEL_CENTER;
        panels = others | (on ? RD_PANEL_CENTER : 0);
    }
    s8 getPanels() { return (s8)panels; }                   /* 0x00195870 (lb) */
    virtual void setPlayerCarNumber(int n) { playerCar = n; }   /* [ 7] +0x44 0x00195860 */
    virtual void setTargetCarNumber(int n) { targetCar = n; }   /* [ 8] +0x4C 0x00195850 */
    void setLive(bool on) { live = on; }                    /* 0x00195840 */
    bool isLive() { return live; }                          /* 0x00195830 */
    /* The event 0x16 (the replay record is full) counts as seen in the frame
     * before, so virtual_2 shows no "replay recording finished"
     * (g_00351FB8 = 3; RaceFreeRun::onExit, RaceMachineTest::onExit). */
    static void ackReplayRecorded() { g_00351FB8 = 3; }     /* 0x00195820 */
    void setFinishDisplay(bool on) { finishDisplay = on; }  /* 0x00195810 */
    /* The screen of a split display (RaceSplitDisplay's constructor). */
    void setDisplayNumber(int n) { displayNumber = n; }     /* 0x00195800 */
    virtual void *getMirrorObject() { return &mirror; }     /* [15] +0x84 0x001957F0 */
    void setCarIcons(CarIcon *i) { icons = i; }             /* 0x001957E8 */
    static ImagePack *getImages() { return g_00351FA4; }    /* 0x001957E0 the display images */

    /*  vptr                                   0x0000 */
    int                   raceMode;         /* 0x0004 the id of the race mode */
    int                   displayMode;      /* 0x0008 the mode shown (raceMode, REPLAY / DEMO; 7, 8: the split
                                               screen battles) */
    int                   replay;           /* 0x000C 0 live, 1 / 3 replay, 2 demo (u049 mode) */
    int                   layoutVariant;    /* 0x0010 virtual_9 (u051 unk10) */
    int                   currentVariant;   /* 0x0014 the variant setLayout loaded, -1 (u051 unk14) */
    RaceOrganization     *org;              /* 0x0018 */
    CarIcon              *icons;            /* 0x001C the cars' icons (the race modes' virtual_43; u051 unk1C,
                                               u047 f1C) */
    RaceMiniMap           miniMap;          /* 0x0020 */
    RaceDisplayFader      fader;            /* 0x0050 the "gt_logo_mini" watermark */
    RaceLapDisplay        lapDisplay;       /* 0x0060 */
    RaceRankDisplay       rankDisplay;      /* 0x0084 */
    RaceLapTimesDisplay   lapTimes;         /* 0x00A4 */
    RaceTimeDisplay       totalTime;        /* 0x00EC "Total" / "Time" (u051 / u052 time0EC) */
    RaceTimeDisplay       bestTime;         /* 0x0134 "Best" (time134) */
    RaceTimeDisplay       recordTime;       /* 0x017C "Record" / "Record Lap" / "Fastest"; org +0x8BC
                                               (time17C) */
    RaceTimeDisplay       lastTime;         /* 0x01C4 "Last" (time1C4) */
    RaceTimeDisplay       lapTime;          /* 0x020C "Lap" / "Time": the current lap (time20C) */
    RaceTimeDisplay       recordTimes[3];   /* 0x0254 the 1st..3rd place records ("mark_1st".."mark_3rd") */
    RaceSpeedDisplay      recordSpeed;      /* 0x032C "Record": org +0x8A4 (speed32C) */
    RaceSpeedDisplay      topSpeedDisplay;  /* 0x0374 "Best": topSpeed */
    RaceEventDisplay      events;           /* 0x03BC the event board (u048 RaceMessageBoard messages) */
    RaceEventDisplay      events2;          /* 0x0838 the second event board (setLayout type 25; events838) */
    RaceMessageDisplay    warningMessage;   /* 0x0CB4 "WRONG WAY" / "AUTO DRIVE" */
    RaceMessageDisplay    carName;          /* 0x0D3C the maker and model of the car shown */
    RaceReplayModeDisplay replayMode;       /* 0x0DC4 */
    RaceMessageDisplay    messageDF0;       /* 0x0DF0 */
    RaceMeterBase        *activeTacho;      /* 0x0E78 the tachometer of the layout */
    RaceOnboardTachometer onboardTacho;     /* 0x0E7C */
    RaceRemoteTachometer  remoteTacho;      /* 0x3F3C */
    RaceBattleTachometer  battleTacho;      /* 0x6FFC */
    RaceOnboardSpeedmeter speedometer;      /* 0x7040 */
    RaceBoostmeter        boostMeter;       /* 0xA0BC */
    RaceOdometer          odometer;         /* 0xA10C */
    RaceOdometer          tripMeter;        /* 0xA134 4 digits */
    RaceIndicators        indicators;       /* 0xA15C */
    RaceDigitalSpeedmeter digitalSpeed;     /* 0xA17C */
    RaceGearDisplay       gearDisplay;      /* 0xA1A0 */
    RaceBackPanel         backPanel;        /* 0xA1D0 */
    RaceMessageDisplay    messageA1EC;      /* 0xA1EC */
    RaceABMonitor         abMonitor;        /* 0xA274 the accel / brake monitor */
    RaceSteeringDisplay   steering;         /* 0xA29C */
    RaceTireWearDisplay   tireWear;         /* 0xA2BC */
    RaceMessageDisplay    title;            /* 0xA2EC the course name, "@count%d", "@start", "@finish",
                                               "PLAYER %d WIN!" */
    RaceMessageDisplay    modeMessage;      /* 0xA374 "QUALIFY" / "DEMONSTRATION" / "LIVE" / "REPLAY" */
    RaceDisplayObjectBase mirror;           /* 0xA3FC getMirrorObject (u051 objectA3FC) */
    RaceCarIconDisplay    carIcon0;         /* 0xA414 next to recordTime */
    RaceCarIconDisplay    carIcon1;         /* 0xA43C next to the time difference */
    SpeedRecord           topSpeed;         /* 0xA464 */
    s8                    displayNumber;    /* 0xA468 the screen of a split display (RaceSplitDisplay: 0, 1;
                                               u051 unkA468): the view of the camera's event 0x15 */
    s8                    targetCar;        /* 0xA469 the car shown (< 0: car 0) */
    s8                    displayCar;       /* 0xA46A the car the meters are set up for, -1: none */
    s8                    playerCar;        /* 0xA46B (u051 unkA46B) */
    u8                    panels;           /* 0xA46C RD_PANEL_* (0x1F) */
    s8                    leaderCar;        /* 0xA46D findCarByRank(1), -1 (u051 / u052 unkA46D) */
    u8                    splitReplay;      /* 0xA46E the game option +0x50 when the mode was chosen */
    /* inner, finished, live: 8-bit bool bit-fields (egcs's bool is 4 bytes):
     * the getters return the byte without normalizing it (lbu only) and
     * the stores do not alias other loads (a u8's would: char aliases
     * everything), as u052's isLive, innerPanel, virtual_16 show. */
    bool                  inner : 8;          /* 0xA46F innerPanel, 1 (u051 unkA46F) */
    u8                    unkA470;          /* 0xA470 cleared by virtual_0 (u049 fA470: RaceLicenseDisplay) */
    bool                  finished : 8;       /* 0xA471 set by virtual_16 (u051 unkA471) */
    u8                    hasTireWear;      /* 0xA472 DynamicsConductor +0x9C != 0 (u051 unkA472) */
    bool                  live : 8;           /* 0xA473 "LIVE" instead of "REPLAY" (u047 fA473) */
    u8                    qualify;          /* 0xA474 the race mode is RACE_MODE_FREE_PRACTICE */
    u8                    endurance;        /* 0xA475 shown as RACE_MODE_ENDULANCE */
    u8                    unkA476;          /* 0xA476 cleared by virtual_0 */
    u8                    noLapDisplay;     /* 0xA477 a replay of race mode 4 or 9 (u051 unkA477) */
    u8                    finishDisplay;    /* 0xA478 1: virtual_16 shows put_finish_display (u051 unkA478,
                                               u049 fA478) */
};
EGCS_SIZE(RaceDisplay, 0xA47C);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
/* The type_info nodes: a __user_type_info {vptr, name}, a __si_type_info
 * {vptr, name, base}.  (u051..u056 / u082 / u084 g_<address>.  The plan
 * also put __ti11RaceDisplay in T140 and __ti14RaceRoundMeter here; the
 * latter is declared with its class in T160, as RaceMeterBase's 0x004B11D0;
 * RaceOnboardSpeedmeter's 0x004B1360 with its class in T162.) */
extern "C" {
extern u32 __ti15RaceDisplayBase[2];            /* 0x003532E8 gp-0x4708 */
extern u32 __ti11RaceMiniMap[3];                /* 0x004B12A0 */
extern u32 __ti11RaceDisplay[3];                /* 0x004B21D0 */
}
#endif

#endif /* GAME_T154_RACE_DISPLAY_H */
