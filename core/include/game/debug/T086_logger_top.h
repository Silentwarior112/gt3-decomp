/* T086_logger_top.h - gt2k_menu::LoggerTop, LoggerTrackInfo, LoggerProgressBar:
 * the top page of the race logger (translation unit T086, unit u037),
 * 0x0016B290..0x0016C260.
 *
 * LoggerTop is the "logger_top_page" item of the logger (T055's item factory
 * table g_0029DE48): its "LoggerMode" list in "RawWindow_TrackSelect"
 * (Replay, Graph, TrackA, TrackB, Exit) sets LoggerControl::state and the
 * logger's next action (logger_set_next); Replay / Graph need a complete
 * recording (logger_is_recorded), TrackA / TrackB a car to load
 * (F_00136400).  The two LoggerTrackInfo ("TrackA", "TrackB") show the
 * recorded cars (car name, power, time, comment, date, mode, course); the
 * LoggerProgressBar ("LoggerProgressBar_Creating") follows the recording
 * thread, which writes its current / total (T055 g_00351550).
 *
 * Per class the RawWindow registry boilerplate (as T272's): slot 8 returns
 * the class name, the 8-word layout factory packs its arguments into a
 * RawWindowArgs and calls create (operator new + the constructor if
 * canCreate(args)), canCreate returns 1.  The registrars are built by
 * egcs's static initialiser of T086 (the records are defined there). */
#ifndef GAME_T086_LOGGER_TOP_H
#define GAME_T086_LOGGER_TOP_H

#include "game/game.h"
#include "game/menu/framework/T264_raw_window.h"           /* base gt2k_menu::RawWindow */
#include "game/menu/framework/T263_raw_list.h"             /* base gt2k_menu::RawListItemFixed */

class RaceOrganization;                     /* T125 (pointers only) */

namespace gt2k_menu {
struct RawWindowArgs;                       /* T264 (references / pointers only) */
struct RawWindowClass;                      /* T265 (extern objects only) */
class RawWindowRegister;                    /* T265 (extern objects only) */
struct MenuLayoutItem;                      /* T229 */
class PageLayout;                           /* T229 */
class MenuControl;                          /* T214 */
class LoggerTrackInfo;
class LoggerProgressBar;

/* vtable 0x00323C80 (24 slots: [0], [8], [9] its own, the rest RawWindow's).
 * typeinfo __tfQ29gt2k_menu17LoggerProgressBar 0x0016C210 (si, base
 * RawWindow; __si_type_info 0x004B1C60).  Size 0x1F4 (operator new in
 * create).  The bar of the recording: hidden and empty as built. */
class LoggerProgressBar : public RawWindow {
public:
    /* current 0, total 1, blink 0, fullWidth = the layout's width; hidden
     * (RawWindow::virtual_23(0), called directly), width 0; registers itself
     * with the logger (logger_set_progress_bar), which fills it at once if
     * the recording is complete (logger_progress_done). */
    LoggerProgressBar(const RawWindowArgs &args);   /* 0x0016B358 */
    virtual ~LoggerProgressBar();           /* [0] +0x0C 0x0016B3F8 logger_set_progress_bar(NULL) */
    virtual const char *virtual_8();        /* [8] +0x4C 0x0016B290 g_00351A60 "LoggerProgressBar" */
    /* current != 0: the bar is current / total of fullWidth, "progress_done"
     * once complete, else "progress_processing" blinking (21 frames of 30);
     * current == 0: shown (with "progress_loading" blinking) only while the
     * logged race's organization is creating the logger data. */
    virtual void virtual_9();               /* [9] +0x54 0x0016B458 */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x0016B298 */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x0016B2D8 */
    static int        canCreate(const RawWindowArgs *args);  /* 0x0016B350 1 (args unused) */
    /* Inline members of the original (no callers: the recording thread
     * writes the fields directly).  egcs writes their copies at the end of
     * T086 with the class's typeinfo function, in reverse declaration order:
     * retail's setTotal, setCurrent, __tf (frame info). */
    void setCurrent(int n) { current = n; } /* 0x0016C208 */
    void setTotal(int n) { total = n; }     /* 0x0016C200 */

    int current;                            /* 0x1E4 the replay frames recorded (0: not started) */
    int total;                              /* 0x1E8 the replay's frame count */
    f32 fullWidth;                          /* 0x1EC the width as built (the full bar) */
    int blink;                              /* 0x1F0 0..29 */
};
EGCS_SIZE(gt2k_menu::LoggerProgressBar, 0x1F4);

/* vtable 0x00323BB8 (24 slots: [0], [8], [17] its own, [21]
 * RawListItemFixed's, the rest RawWindow's).  typeinfo
 * __tfQ29gt2k_menu15LoggerTrackInfo 0x0016C1B0 (si, base RawListItemFixed;
 * __si_type_info 0x004B1C40).  Size 0x204 (operator new in create): no data
 * of its own.  One recorded car of the top page ("TrackA" / "TrackB"). */
class LoggerTrackInfo : public RawListItemFixed {
public:
    LoggerTrackInfo(const RawWindowArgs &args);     /* 0x0016B720 */
    virtual ~LoggerTrackInfo();             /* [0] +0x0C 0x0016B758 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x0016B658 g_00351A78 "LoggerTrackInfo" */
    /* An event 1 from a child is sent on as the item's own event 1 (0);
     * anything else: 1. */
    virtual int  virtual_17(RawWindow *sender, int event, int arg); /* [17] +0x94 0x0016BA68 */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x0016B660 */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x0016B6A0 */
    static int        canCreate(const RawWindowArgs *args);  /* 0x0016B718 1 (args unused) */
    /* "item_load" gets the node "gadget_load_a" / "_b" (g_00351A90[car]);
     * with an organization: the car texts ("list_track_car_info", _power_,
     * _time_, _comment_, _date_) when race->virtual_26(car ? 1 : 0x100)
     * ->hold (RaceInput +0x140) is 0, the mode (race_kind_text, plus the
     * test number for the licence kinds 9..14) and the course always. */
    void setInfo(RaceOrganization *org, int car);           /* 0x0016B7B0 */
    /* The text of the descendant `name' (findChildByFullName), if any. */
    void setChildText(const char *name, const char *text);  /* 0x0016BA30 */
};
EGCS_SIZE(gt2k_menu::LoggerTrackInfo, 0x204);

/* vtable 0x00323AF0 (24 slots: [0], [8], [9], [14], [17] its own, the rest
 * RawWindow's).  typeinfo __tfQ29gt2k_menu9LoggerTop 0x0016C160 (si, base
 * RawWindow; __si_type_info 0x004B1C50).  Size 0x1F8 (operator new in
 * create).  The menu control is the owner (the LoggerControl). */
class LoggerTop : public RawWindow {
public:
    /* blink 0, childrenFocusChange 0; loads "RawWindow_TrackSelect", sets up
     * "TrackA" / "TrackB" with logger_get_org(), takes args.owner as its
     * control (LoggerTop::virtual_14, called directly), then loads
     * "LoggerProgressBar_Creating". */
    LoggerTop(const RawWindowArgs &args);   /* 0x0016BB78 */
    virtual ~LoggerTop();                   /* [0] +0x0C 0x0016BCA0 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x0016BAB0 g_00351AB0 "LoggerTop" */
    /* The focused "LoggerMode" item pulses (x 1.0 .. 2.0 over 30 frames, half
     * as bright if it cannot be chosen); Replay / Graph / TrackA when not
     * focused: 1.0, or 0.5 when they cannot be chosen. */
    virtual void virtual_9();               /* [9] +0x54 0x0016BEC8 */
    virtual void virtual_14(MenuControl *ctl);  /* [14] +0x7C 0x0016BCF8 RawWindow's, and the "LoggerMode"
                                               list gets ctl too */
    /* Event 1 (a "LoggerMode" item): TrackA / TrackB logger_set_next(1 / 2)
     * and state 4 if F_00136400 allows it, Exit as event 2, Replay (state 1)
     * / Graph (state 2) if logger_is_recorded(); a refused choice plays sound
     * 4.  Event 2: logger_set_next(0), state 4.  A chosen item ends the
     * page (pages +0xB4C = 1, setFreeze(0)); 0 handled, 1 otherwise. */
    virtual int  virtual_17(RawWindow *sender, int event, int arg); /* [17] +0x94 0x0016BD50 */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x0016BAB8 (T055's g_0029DE48
                                               "logger_top_page") */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x0016BAF8 */
    static int        canCreate(const RawWindowArgs *args);  /* 0x0016BB70 1 (args unused) */

    RawWindow         *trackSelect;         /* 0x1E4 "RawWindow_TrackSelect" (childrenFocusChange 0) */
    LoggerTrackInfo   *tracks[2];           /* 0x1E8 "TrackA", "TrackB" */
    LoggerProgressBar *progress;            /* 0x1F0 "LoggerProgressBar_Creating" */
    int                blink;               /* 0x1F4 0..29 */
};
EGCS_SIZE(gt2k_menu::LoggerTop, 0x1F8);

} /* namespace gt2k_menu */

/* 0x0016C0E0 (.dtors 0x0028F1F4) / 0x0016C118 (.ctors 0x0028F0B4): egcs's
 * static destructor / initialiser of the three registrars
 * (_GLOBAL_$D$g_00351A60 / _GLOBAL_$I$g_00351A60, T086). */

/* T086's .sdata (defined in T086): per class the name pointer, the
 * RawWindowClass record {name, create} and its registrar (as T272's); the
 * node tables before the functions that use them. */
extern const char *g_00351A60;                  /* 0x00351A60 gp-0x5F90 "LoggerProgressBar" */
extern gt2k_menu::RawWindowClass    g_00351A68; /* 0x00351A68 gp-0x5F88 {name, LoggerProgressBar::create} */
extern gt2k_menu::RawWindowRegister g_00351A70; /* 0x00351A70 gp-0x5F80 its registrar */
extern const char *g_00351A78;                  /* 0x00351A78 gp-0x5F78 "LoggerTrackInfo" */
extern gt2k_menu::RawWindowClass    g_00351A80; /* 0x00351A80 gp-0x5F70 {name, LoggerTrackInfo::create} */
extern gt2k_menu::RawWindowRegister g_00351A88; /* 0x00351A88 gp-0x5F68 its registrar */
extern const char *g_00351A90[2];               /* 0x00351A90 gp-0x5F60 {"gadget_load_a", "gadget_load_b"} */
extern const char *g_00351AB0;                  /* 0x00351AB0 gp-0x5F40 "LoggerTop" */
extern gt2k_menu::RawWindowClass    g_00351AB8; /* 0x00351AB8 gp-0x5F38 {name, LoggerTop::create} */
extern gt2k_menu::RawWindowRegister g_00351AC0; /* 0x00351AC0 gp-0x5F30 its registrar */
extern const char *g_00351AD8[2];               /* 0x00351AD8 gp-0x5F18 {"TrackA", "TrackB"} */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself (and rejects hand
 * declarations of them). */
extern "C" {
void *__tfQ29gt2k_menu9LoggerTop(void);         /* 0x0016C160 */
void *__tfQ29gt2k_menu15LoggerTrackInfo(void);  /* 0x0016C1B0 */
void *__tfQ29gt2k_menu17LoggerProgressBar(void);/* 0x0016C210 */
}
#endif

#endif /* GAME_T086_LOGGER_TOP_H */
