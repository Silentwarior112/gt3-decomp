/* T055_logger.h - the data logger ("menu/logger.mbl"): gt2k_menu::
 * LoggerControl, LoggerRender, LoggerPage, LoggerInput, LoggerData and the
 * recording thread (translation unit T055, unit u018), 0x00134EE8..0x00137468.
 *
 * logger_run (from the race modes, over the race just driven) and
 * logger_main (from main: replays loaded from the memory card) build a
 * LoggerControl on the stack; LoggerControl::run gives it a 0x4000-byte
 * thread stack, a 0x32000-byte work area and the LoggerData (all on its own
 * stack frame) and runs the pages g_0029DE38[state] ("top", then
 * "replay_logger") until state is 4.  While a race is logged,
 * logger_record_thread runs the race (a replay) as fast as it can and
 * samples both cars every `interval' metres of their lap distance (speed,
 * throttle, brake, gear, x / z) into LoggerData; the logger windows (T086 /
 * T087) query the samples and drive the replay (a seek to a logger sector,
 * play, speed, a camera change).
 *
 * CarDynamicsCC (the pass-2 plan's class of this header, u018 / u078) is
 * the replay recorder of a RaceInput (T413 rec.rec, RaceInput +0xCC):
 * +0x30 is AutomobileControlRecord::Recorder::frames and u018's
 * F_001F4C78 is Recorder::frameCount.  It is a typedef in T318.
 * gt2k_menu::MenuItemEntry (u018 / u040) is T229's MenuHook. */
#ifndef GAME_T055_LOGGER_H
#define GAME_T055_LOGGER_H

#include "game/game.h"
#include "game/race/core/T413_race_input.h"           /* base RaceInput */
#include "game/menu/framework/T246_render_base.h"          /* base gt2k_menu::RenderBase */
#include "game/menu/framework/T211_page_manager.h"         /* base gt2k_menu::PageManager */
#include "game/menu/framework/T247_control_base.h"         /* base gt2k_menu::ControlBase */
#include "game/race/dynamics/T318_control_record.h"       /* AutomobileControlRecord::Recorder (CarDynamicsCC) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuHook (MenuItemEntry) */

class GSBuffer;                             /* T514 (references only) */
class RaceOrganization;                     /* T125 (pointers only) */
struct ThreadCtx;                           /* G002 (g_00353220) */

namespace gt2k_menu {
class ButtonList;                           /* T244 (pointers only) */
struct LinkPage;                            /* T241 */
class LoggerProgressBar;                    /* T086 */
class LoggerRoot;                           /* T087 */
class LoggerRaceReplay;                     /* T087 */

/* An entry of the logger's item factory table g_0029DE48 (and of
 * MiniPrizeControlBase's, T104): PageLayout::setHooks takes MenuHook
 * tables.  0x18 bytes: name, create, p8, pC, p10, p14 (u018 / u040: arg0,
 * arg1, unk10, unk14). */
typedef MenuHook MenuItemEntry;
}

/* One sample of a car, taken every `interval' metres of the lap distance.
 * Not polymorphic; 8 bytes. */
struct LoggerSample {
    u16 gear  : 3;                          /* bits 0..2 of the first halfword */
    u16 speed : 13;                         /* bits 3..15: the average speed over the interval x 40 */
    u8  throttle;                           /* 0x2 the average throttle, 0..255 */
    u8  brake;                              /* 0x3 the average brake, 0..255 */
    s16 x;                                  /* 0x4 the car's x at the interval's end (T302 F_001EB400) */
    s16 z;                                  /* 0x6 its z (T302 F_001EB490) */
};
EGCS_SIZE(LoggerSample, 0x8);

/* The samples of the two cars (the player's, then the other one); a stack
 * object of LoggerControl::run, g_00353228 points at it.  The queries of
 * the logger windows (T086 / T087): t, t0, t1 are lap distances in metres,
 * car is 0 or 1. */
struct LoggerData {
    f32 speedAt(int car, f32 t);            /* 0x00134F68 */
    f32 xBefore(int car, f32 t);            /* 0x00134FF0 */
    f32 zBefore(int car, f32 t);            /* 0x00135060 */
    f32 minSpeed(int car, f32 t0, f32 t1);  /* 0x001350D0 */
    f32 maxSpeed(int car, f32 t0, f32 t1);  /* 0x00135210 */
    f32 avgSpeed(int car, f32 t0, f32 t1);  /* 0x00135350 */
    f32 avgThrottle(int car, f32 t0, f32 t1);   /* 0x001354A0 percent */

    LoggerSample samples[2][0x4000];        /* 0x00000 */
    int          count[2];                  /* 0x40000 the samples taken of each car */
    f32          interval;                  /* 0x40008 metres between two samples: the lap length /
                                               16383, at least 0.5 */
};
EGCS_SIZE(LoggerData, 0x4000C);

namespace gt2k_menu {

/* vtable 0x00316D98 (4 slots: [1] RenderBase::update, [3] GameObject's
 * 0x00101A38).  typeinfo __tfQ29gt2k_menu12LoggerRender 0x00137418 (si,
 * base RenderBase; storage 0x004B1790).  vptr +0x00; size 0x5C
 * (LoggerControl +0x28C). */
class LoggerRender : public RenderBase {
public:
    LoggerRender();                         /* 0x001364B8 window NULL */
    virtual ~LoggerRender();                /* [0] +0x0C 0x001364F0 (RenderBase's destructor expanded) */
    /* The window's race view (LoggerRaceReplay::renderRace, T087) under the
     * menu, then RenderBase::render. */
    virtual void render(GSBuffer &buf);     /* [2] +0x1C 0x00136540 */
    /* Inline in the original: no callers, 0x00137410 is egcs's out-of-line
     * copy; LoggerControl::runPage expands it (retail keeps &render in
     * $s7 and stores through it). */
    void setWindow(LoggerRaceReplay *w) { window = w; }     /* 0x00137410 */

    LoggerRaceReplay *window;               /* 0x58 the replay window drawn over the race (it sets
                                               itself through LoggerControl), NULL none */
};
EGCS_SIZE(gt2k_menu::LoggerRender, 0x5C);

/* vtable 0x00316D80 (2 slots: [0] PageManager::createButtonList
 * overridden, [1] the virtual destructor it adds).  typeinfo
 * __tfQ29gt2k_menu10LoggerPage 0x001373C0 (si, base PageManager; storage
 * 0x004B1780).  vptr +0xB58 (PageManager's); size 0xB5C (LoggerControl
 * +0x2E8). */
class LoggerPage : public PageManager {
public:
    LoggerPage(ControlBase *control);       /* 0x00136590 PageManager(control), the vptr */
    /* No button list: the logger's pages are made of its item factories. */
    virtual ButtonList *createButtonList(LinkPage *page, int index);  /* [0] +0x0C 0x001365F8 NULL */
    virtual ~LoggerPage();                  /* [1] +0x14 0x001365C8 */
};
EGCS_SIZE(gt2k_menu::LoggerPage, 0xB5C);

/* vtable 0x00316D68 (2 slots: [1] ControlBase::waitFrame).  typeinfo
 * __tfQ29gt2k_menu13LoggerControl 0x00137370 (si, base ControlBase;
 * storage 0x004B17A0).  vptr +0x288 (ControlBase's); size 0xE48 (a stack
 * object of logger_run / logger_run_without_race). */
class LoggerControl : public ControlBase {
public:
    /* ControlBase, render, page(this); pageManager = &page; state 0. */
    LoggerControl();                        /* 0x00136600 */
    virtual ~LoggerControl();               /* [0] +0x0C 0x001366B0 */
    void loadPages();                       /* 0x00136710 "menu/logger.mbl" */
    void unloadPages();                     /* 0x00136738 */
    void begin();                           /* 0x00136758 the item factories g_0029DE48 in both layouts */
    void end();                             /* 0x001367F8 */
    /* Runs page `name' until it is left; any page but "top" plays the
     * logged replay behind it (sector seek, play, speed, camera change). */
    void runPage(const char *name);         /* 0x00136838 */
    void mainLoop();                        /* 0x00136BC0 the recording thread, the pages by state */
    void run();                             /* 0x00136C58 the thread stack, work area and LoggerData */
    /* Inline in the original: no callers, 0x00137358..0x00137368 are
     * egcs's out-of-line copies (the logger windows, T086 / T087, expand
     * them).  Declared in the reverse of their address order: egcs writes a
     * class's inline members last-declared first (T055's frame info has
     * setWindow, getState, setState; u018's data pass). */
    void setState(int s) { state = s; }     /* 0x00137368 */
    int  getState() { return state; }       /* 0x00137360 */
    void setWindow(LoggerRaceReplay *w) { render.window = w; }  /* 0x00137358 */

    /*            ControlBase                  0x000 .. 0x28C (vptr +0x288) */
    LoggerRender  render;                   /* 0x28C */
    LoggerPage    page;                     /* 0x2E8 */
    int           state;                    /* 0xE44 the page to run next (g_0029DE38: 0 "top", 1..3
                                               "replay_logger"); 4 ends the logger */
};
EGCS_SIZE(gt2k_menu::LoggerControl, 0xE48);

/* LoggerInput is defined after LoggerControl (runPage's stack object): egcs
 * emits a unit's vtables and typeinfo names in the reverse order of the
 * class definitions, and retail T055's .rodata has LoggerInput's first
 * (vtable 0x00316D08, name 0x00316DC0), then LoggerControl's, LoggerPage's
 * and LoggerRender's. */
/* vtable 0x00316D08 (11 slots: only the destructor is its own; [1..8]
 * Control's, [9] RaceInput::emulatePressure, [10] RaceInput::getControllerConfig).
 * typeinfo __tfQ29gt2k_menu11LoggerInput 0x00137308 (si, base RaceInput;
 * storage 0x004B17B0).  vptr +0xC0 (Control's); size 0x148 (as RaceInput).
 * The camera input the logger gives the replay for one frame
 * (CameraManager +0x2780, a stack object of LoggerControl::runPage). */
class LoggerInput : public RaceInput {
public:
    LoggerInput();                          /* 0x00134EE8 RaceInput(), the vptr */
    virtual ~LoggerInput();                 /* [0] +0x0C 0x00134F20 */
    /* For the replay's camera change (logger_request_step): the camera
     * button RIB_REPLAY_CROSS (0x400000) pressed this frame - curButtons
     * {all, 0x400000}, prevButtons {all, 0} - so that the camera handlers
     * (T391) step the view kind.  (The name does not fit: pressCameraButton
     * would; see ref/pass2/units/u018/notes.md.) */
    void setNeutral();                      /* 0x00134F48 */
};
EGCS_SIZE(gt2k_menu::LoggerInput, 0x148);

} /* namespace gt2k_menu */

/* LoggerControl::state: the page the logger runs next (g_0029DE38: 0
 * "top", 1..3 "replay_logger"; 3 is not set by any page); LOGGER_STATE_END
 * ends the logger. */
enum {
    LOGGER_STATE_TOP    = 0,                /* the "top" page */
    LOGGER_STATE_REPLAY = 1,                /* "replay_logger" in the replay mode (LoggerReplayRoot) */
    LOGGER_STATE_GRAPH  = 2,                /* "replay_logger" in the graph mode (LoggerGraphRoot) */
    LOGGER_STATE_END    = 4                 /* ends the logger */
};

/* g_00351544, what the logger does when its pages end (logger_set_next;
 * LoggerTop's Exit / TrackA / TrackB). */
enum {
    LOGGER_NEXT_QUIT       = 0,             /* leave the logger */
    LOGGER_NEXT_LOAD       = 1,             /* "TrackA": load a replay to log (logger_main) */
    LOGGER_NEXT_LOAD_OTHER = 2              /* "TrackB": load the other car's replay, log again
                                               (logger_run) */
};

/* Recording (the thread and its control).  The thread runs the logged
 * race frame after frame until stopped or until the race signals its end;
 * the progress bar gets the replay's frames / frameCount() every frame.
 * arg is not used. */
void logger_record_thread(void *arg);       /* 0x00135788 */
void logger_record_start(void);             /* 0x001362A8 stops a running one, then starts it (priority 10) */
void logger_record_join(void);              /* 0x001362F8 */
void logger_record_stop(void);              /* 0x00136330 */
int  logger_is_recorded(void);              /* 0x00136358 1 once the recording is complete */
void logger_set_root(gt2k_menu::LoggerRoot *root);  /* 0x00136368 (LoggerRoot's constructor / destructor) */
gt2k_menu::LoggerRoot *logger_get_root(void);       /* 0x00136428 (no callers) */
void logger_set_progress_bar(gt2k_menu::LoggerProgressBar *bar); /* 0x00136370 (T086) */
void logger_progress_done(void);            /* 0x00136378 the bar 1 / 1 once complete */
/* The replay controls.  A seek is to a logger sector (RaceSolitaire::
 * seekRecords); a sector's end stops the play (with g_0035158C: plays the
 * sector again). */
void logger_request_seek(int sector);       /* 0x001363C0 */
int  logger_take_seek(void);                /* 0x001363C8 the request (-1 none), cleared */
/* Whether a replay can be loaded (LoggerTop): the other car's ("TrackB",
 * otherCar 1) when a race is logged, else only while logger_main runs
 * (g_00351548; "TrackA", the replay to log). */
int  F_00136400(int otherCar);              /* 0x00136400 */
RaceOrganization *logger_get_org(void);     /* 0x00136420 the logged race's, NULL none */
f32  logger_get_length(void);               /* 0x00136430 the recorded distance (the graphs' range) */
void logger_reset_replay(void);             /* 0x00136438 */
void logger_set_play(int on);               /* 0x00136450 */
int  logger_is_playing(void);               /* 0x00136460 */
void logger_set_loop(int on);               /* 0x00136468 (no callers) */
void logger_set_speed(int index);           /* 0x00136470 g_0029DE28[index] */
/* "ReplayCamera": a camera change on the next frame (runPage: the
 * LoggerInput's camera button, the cameras alone while paused).  Not a
 * single step: the names are the first pass's (notes.md). */
void logger_request_step(void);             /* 0x00136478 */
int  logger_take_step(void);                /* 0x00136488 the request, cleared */
LoggerData *logger_get_data(void);          /* 0x00136C50 */
/* The entry points.  logger_run: over a race (the race modes of T052 /
 * T053, logger_main; unused is not read), again with the other car's
 * replay from the memory card when the pages ask for it; 1.
 * logger_run_without_race: the top page only (to load a replay); 1.
 * logger_main: from main, replays from the memory card become the logged
 * race. */
int  logger_run(RaceOrganization *org, int unused);  /* 0x00136CF0 */
int  logger_run_without_race(void);         /* 0x00136F18 */
void logger_main(void);                     /* 0x00136FC0 */
void logger_set_next(int next);             /* 0x001372A0 g_00351544 (LoggerTop) */
void T055_static_init(void);                /* 0x001372A8 .ctors entry 0x0028F064: g_0029DE48 */

/* T055's own helpers. */
f32  line_at(f32 x0, f32 x1, f32 y0, f32 y1, f32 x);                 /* 0x00135600 */
/* The area under the segment (x0, y0)-(x1, y1) clipped to [lo, hi]. */
f32  segment_area(f32 x0, f32 x1, f32 y0, f32 y1, f32 lo, f32 hi);   /* 0x00135638 */
void logger_load_course_map(void);          /* 0x001356F8 "logger/<course>" into the race's work buffer */

/* T055's data. */
extern f32  g_0029DE28[4];                  /* 0x0029DE28 the replay speeds {1, 0.5, 0.25, 0} */
extern const char *g_0029DE38[4];           /* 0x0029DE38 the logger pages {"top", "replay_logger" x 3} */
extern gt2k_menu::MenuItemEntry g_0029DE48[2]; /* 0x0029DE48 "logger_top_page", "replay_logger" */
extern int  g_00351544;                     /* 0x00351544 gp-0x64AC what the logger does next: 0 quit,
                                               1 load a replay, 2 load the other car */
extern int  g_00351548;                     /* 0x00351548 gp-0x64A8 1 while logger_main runs */
extern gt2k_menu::LoggerRoot *g_0035154C;   /* 0x0035154C gp-0x64A4 the LoggerRoot window */
extern gt2k_menu::LoggerProgressBar *g_00351550; /* 0x00351550 gp-0x64A0 the recording's bar, NULL none */
extern void *g_00351554;                    /* 0x00351554 gp-0x649C the recording thread's stack */
extern int  g_00351558;                     /* 0x00351558 gp-0x6498 its size (0x4000) */
extern int  g_0035155C;                     /* 0x0035155C gp-0x6494 recording threads started, not joined */
extern volatile int g_00351560;             /* 0x00351560 gp-0x6490 non-zero: the recording stops (volatile:
                                               shared with the thread; retail keeps its accesses out of
                                               delay slots) */
extern volatile int g_00351564;             /* 0x00351564 gp-0x648C 1 once the recording is complete (volatile:
                                               set by the thread) */
extern f32  g_00351568;                     /* 0x00351568 gp-0x6488 the recorded distance of car 0 */
extern int  g_0035156C;                     /* 0x0035156C gp-0x6484 (-1) the sector play resumes from */
extern int  g_00351570;                     /* 0x00351570 gp-0x6480 (-1) a seek request (sector) */
extern int  g_00351574;                     /* 0x00351574 gp-0x647C the sector of the last seek */
extern int  g_00351580;                     /* 0x00351580 gp-0x6470 non-zero: the replay plays */
extern int  g_00351584;                     /* 0x00351584 gp-0x646C a camera change request */
extern int  g_00351588;                     /* 0x00351588 gp-0x6468 a copy of g_00351580 (never read) */
extern int  g_0035158C;                     /* 0x0035158C gp-0x6464 non-zero: at a sector's end play goes
                                               on, the sector again (loop; logger_set_loop) */
extern int  g_00351590;                     /* 0x00351590 gp-0x6460 the replay speed (g_0029DE28 index) */
extern int  g_0035159C;                     /* 0x0035159C gp-0x6454 2, the entries of g_0029DE48 */
/* T055's .sbss 0x00353210..0x0035322C (the logged race, logger_main's race
 * buffer, the work area and its size, the recording thread, the LoggerData)
 * are file statics of T055 (u018's data pass: egcs writes a C++ global
 * without an initialiser into .sdata, only statics go to .sbss). */

/* __ti9RaceInput (0x004B2990, planned here as well) is declared in
 * T413_race_input.h. */
#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu11LoggerInput(void);      /* 0x00137308 */
void *__tfQ29gt2k_menu13LoggerControl(void);    /* 0x00137370 */
void *__tfQ29gt2k_menu10LoggerPage(void);       /* 0x001373C0 */
void *__tfQ29gt2k_menu12LoggerRender(void);     /* 0x00137418 */
}
#endif

#endif /* GAME_T055_LOGGER_H */
