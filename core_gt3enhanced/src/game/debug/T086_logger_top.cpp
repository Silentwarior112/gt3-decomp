/* T086 - the top page of the race logger.  0x0016B290..0x0016C260.
 *
 * gt2k_menu::LoggerTop is the "logger_top_page" item of the logger (T055):
 * the "LoggerMode" list in "RawWindow_TrackSelect" (Replay, Graph, TrackA,
 * TrackB, Exit) sets the page the logger runs next (LoggerControl::state)
 * and what it does once its pages end (logger_set_next); Replay / Graph
 * need a complete recording, TrackA / TrackB a replay that can be loaded
 * (F_00136400).  The two LoggerTrackInfo show the recorded cars (car name,
 * power, time, comment, date, mode, course); the LoggerProgressBar follows
 * the recording thread (T055 writes its current / total).
 *
 * Per class the RawWindow registry boilerplate (as T272's): first its record
 * (name pointer, RawWindowClass, registrar; retail's .sdata order), then the
 * class name (slot 8), the layout factory, create, canCreate, the
 * constructor.  The node tables are defined before the functions that use
 * them (their place in .sdata / .rodata).
 *
 * Includes: retail's .rodata (0x00323938..0x00323EE0) opens with <new>'s
 * "bad_alloc" (unreferenced: DATA_START), then the literals, the three
 * vtables and the type names in the reverse order of the classes'
 * definitions: its own three (LoggerTop, LoggerTrackInfo,
 * LoggerProgressBar), OperatorList, List<OperatorHolder>, OperatorHolder,
 * Cell<OperatorHolder>, RaceCameraBase, UnitArena<64>, SmartIndexT<int>,
 * RegurationResult, Operator (a non-polymorphic base: always last).  No
 * garage (retail's T086 did not see the GameStatus).  So <new>,
 * RegurationResult, T251, T153's UnitArena<64>, RaceCameraBase, the Raw*
 * windows (the operator classes), the other headers (no names), and the
 * logger top classes last.
 */
// DATA_START 0x00323938
#include <new>                              /* (seen: "bad_alloc") */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen: RegurationResult) */
#include "game/common/T251_smart_index.h"          /* (seen: SmartIndexT<int>) */
#define GAME_T153_EMITS_TYPEINFO            /* (seen: UnitArena<64>'s type name) */
#include "game/race/core/T153_race_course.h"
#include "game/camera/T383_race_camera_base.h"     /* (seen: RaceCameraBase) */
#include "game/menu/framework/T264_raw_window.h"           /* gt2k_menu::RawWindow, RawWindowArgs, RAW_EVENT_* */
#include "game/menu/framework/T263_raw_list.h"             /* gt2k_menu::RawListItemFixed, RawList (focusItem) */
#include "game/menu/framework/T265_raw_window_register.h"  /* gt2k_menu::RawWindowClass, RawWindowRegister */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator new / delete) */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase (control1) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (exitRequest, setFreeze) */
#include "game/debug/T055_logger.h"               /* gt2k_menu::LoggerControl (state), logger_is_recorded,
                                               logger_set_progress_bar, logger_progress_done, F_00136400,
                                               logger_get_org, logger_set_next */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryCar (info, ghostLapTime, driverName, date),
                                               RaceEntryBase (cars) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, raceCode, raceDB, courseCode,
                                               race, loggerCreating) */
#include "game/race/core/T129_race_base.h"            /* RaceBase::virtual_26 */
#include "game/race/core/T413_race_input.h"           /* RaceInput::hold */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/car/model/T278_show_room_car.h"        /* ShowRoomCar (SetupShowRoomCar, power) */
#include "game/data/T482_car_equipments.h"       /* CarEquipments::getName1 / getName2 */
#include "game/data/T484_race_database.h"        /* RaceDataBase::getCourseName */
#include "game/data/T489_car_database.h"         /* db_race_name, race_name_split, race_kind_text, RaceKind */
#include "game/common/T494_text_utils.h"           /* format_time */
#include "game/data/T518_unit_settings.h"        /* UnitSettings g_002F59D8 (powerName) */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "runtime/newlib/G007_stdio.h"                /* strcpy */
#include "game/debug/T086_logger_top.h"           /* the logger top classes: the last defined */
#include "game/menu/options/play_list_internal.h"

namespace gt2k_menu {

/* ======================================================================== */
/* LoggerProgressBar                                                        */
/* ======================================================================== */

} /* namespace gt2k_menu */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351A60 = "LoggerProgressBar";               /* 0x00351A60 */
gt2k_menu::RawWindowClass g_00351A68 =                      /* 0x00351A68 */
    { "LoggerProgressBar", gt2k_menu::LoggerProgressBar::create };
gt2k_menu::RawWindowRegister g_00351A70(&g_00351A68);       /* 0x00351A70 */

namespace gt2k_menu {

// FUNC 0x0016B290 gt2k_menu::LoggerProgressBar::virtual_8
const char *LoggerProgressBar::virtual_8()
{
    return g_00351A60;                      /* "LoggerProgressBar" */
}

// FUNC 0x0016B298 gt2k_menu::LoggerProgressBar::factory
RawWindow *LoggerProgressBar::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                      int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_00351A68.create). */
// FUNC 0x0016B2D8 gt2k_menu::LoggerProgressBar::create
RawWindow *LoggerProgressBar::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new LoggerProgressBar(*args);    /* Object::operator new(0x1F4); operator delete
                                                   if the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x0016B350 gt2k_menu::LoggerProgressBar::canCreate
int LoggerProgressBar::canCreate(const RawWindowArgs *args)
{
    return 1;
}

/* Starts hidden and empty (width 0; fullWidth = the layout's width), then
 * registers itself with the logger (which fills it at once if the
 * recording is already complete).  RawWindow's slot 23 is called directly.
 * EH: ~RawWindow() if it throws. */
// FUNC 0x0016B358 gt2k_menu::LoggerProgressBar::LoggerProgressBar
LoggerProgressBar::LoggerProgressBar(const RawWindowArgs &args)
    : RawWindow(args)
{
    current = 0;
    blink = 0;
    total = 1;
    fullWidth = w;
    RawWindow::virtual_23(0);               /* 0x001D7AF0 */
    setWidth(0.0f);
    logger_set_progress_bar(this);
    logger_progress_done();
}

/* Unregisters from the logger first. */
// FUNC 0x0016B3F8 gt2k_menu::LoggerProgressBar::~LoggerProgressBar
LoggerProgressBar::~LoggerProgressBar()
{
    /* implicit: ~RawWindow, operator delete (the base / member destructors,
     * flags & 1) */
    logger_set_progress_bar(NULL);
    /* then ~RawWindow() (0x001D5F18, flags 0); Object::operator delete if
     * flags & 1 */
}

/* current != 0: shown, the bar current / total of fullWidth,
 * "progress_done" once complete, else "progress_processing" blinking (21
 * frames of 30).  current == 0: empty, shown (with "progress_loading"
 * blinking) only while the logged race's organization is creating the
 * logger data. */
// FUNC 0x0016B458 gt2k_menu::LoggerProgressBar::virtual_9
void LoggerProgressBar::virtual_9()
{
    RawWindow *loading    = findChildByFullName("progress_loading");
    RawWindow *processing = findChildByFullName("progress_processing");
    RawWindow *done       = findChildByFullName("progress_done");
    int blinkOn;

    if (++blink >= LOGGER_BLINK_FRAMES)
        blink = 0;
    blinkOn = blink < LOGGER_BLINK_ON;
    loading->virtual_23(0);
    processing->virtual_23(0);
    done->virtual_23(0);

    if (current == 0) {
        RaceOrganization *org = logger_get_org();
        setWidth(0.0f);
        if (org != NULL && org->loggerCreating != 0) {
            virtual_23(1);
            if (blinkOn)
                loading->virtual_23(1);
        } else {
            virtual_23(0);
        }
    } else {
        virtual_23(1);
        setWidth(fullWidth * (f32)current / (f32)total);
        if (current >= total)
            done->virtual_23(1);
        else if (blinkOn)
            processing->virtual_23(1);
    }
}

/* ======================================================================== */
/* LoggerTrackInfo                                                          */
/* ======================================================================== */

} /* namespace gt2k_menu */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351A78 = "LoggerTrackInfo";                 /* 0x00351A78 */
gt2k_menu::RawWindowClass g_00351A80 =                      /* 0x00351A80 */
    { "LoggerTrackInfo", gt2k_menu::LoggerTrackInfo::create };
gt2k_menu::RawWindowRegister g_00351A88(&g_00351A80);       /* 0x00351A88 */

namespace gt2k_menu {

// FUNC 0x0016B658 gt2k_menu::LoggerTrackInfo::virtual_8
const char *LoggerTrackInfo::virtual_8()
{
    return g_00351A78;                      /* "LoggerTrackInfo" */
}

// FUNC 0x0016B660 gt2k_menu::LoggerTrackInfo::factory
RawWindow *LoggerTrackInfo::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                    int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_00351A80.create). */
// FUNC 0x0016B6A0 gt2k_menu::LoggerTrackInfo::create
RawWindow *LoggerTrackInfo::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new LoggerTrackInfo(*args);  /* Object::operator new(0x204); operator delete if
                                               the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x0016B718 gt2k_menu::LoggerTrackInfo::canCreate
int LoggerTrackInfo::canCreate(const RawWindowArgs *args)
{
    return 1;
}

// FUNC 0x0016B720 gt2k_menu::LoggerTrackInfo::LoggerTrackInfo
LoggerTrackInfo::LoggerTrackInfo(const RawWindowArgs &args)
    : RawListItemFixed(args)
{
}

// FUNC 0x0016B758 gt2k_menu::LoggerTrackInfo::~LoggerTrackInfo
LoggerTrackInfo::~LoggerTrackInfo()
{
    /* only ~RawListItemFixed() (0x001D40E8, flags 0), then
     * Object::operator delete if flags & 1 */
    /* implicit: ~RawListItemFixed, operator delete (the base / member
     * destructors, flags & 1) */
}

} /* namespace gt2k_menu */

/* The "item_load" node of each car (.sdata). */
const char *g_00351A90[2] = { "gadget_load_a", "gadget_load_b" };       /* 0x00351A90 */

namespace gt2k_menu {

/* Called by the LoggerTop constructor for car 0 ("TrackA") and 1
 * ("TrackB").  "item_load" takes the node "gadget_load_a" / "_b"; without
 * an organization nothing more.  The car's texts (name, power in the user's
 * unit, ghost lap time, driver name as the comment, date dd/mm/yyyy) only
 * when its input is not on hold (? the car has a replay); the mode (the
 * race kind's text, plus the licence test's number) and the course always.
 * Frame: buf sp+0x0 (0x100), the ShowRoomCar sp+0x100 (a 0x1350 slot), rpm
 * sp+0x1450, kind sp+0x1454, index sp+0x1458. */
// FUNC 0x0016B7B0 gt2k_menu::LoggerTrackInfo::setInfo
void LoggerTrackInfo::setInfo(RaceOrganization *org, int car)
{
    char buf[0x100];
    ShowRoomCar showRoom;
    int rpm;
    int kind;
    int index;

    findChildByFullName("item_load")->setNode(g_00351A90[car]);
    if (org == NULL)
        return;

    RaceDataBase *db = org->raceDB;
    if (org->race->virtual_26(car == 0 ? INPUT_ID_PLAYER : INPUT_ID_OTHER)->hold == 0) {
        RaceEntryCar *entry = org->entries->cars[car];
        CarEquipments *equip = &entry->info;

        sprintf(buf, "%s %s", equip->getName1(), equip->getName2());    /* 0x00351A98 */
        setChildText("list_track_car_info", buf);

        showRoom.SetupShowRoomCar(equip, NULL);
        sprintf(buf, "%d%s", showRoom.power(&rpm, 0, 0), g_002F59D8.powerName());  /* 0x00351AA0 */
        setChildText("list_track_power_info", buf);

        format_time(buf, entry->ghostLapTime);
        setChildText("list_track_time_info", buf);

        setChildText("list_track_comment_info", entry->driverName);

        const EntryDate *date = (const EntryDate *)&entry->date;     /* lbu +4, lbu +5, lhu +6 */
        sprintf(buf, "%02d/%02d/%04d", date->day, date->month, date->year);
        setChildText("list_track_date_info", buf);
    }

    const char *race = db_race_name(org->raceCode);
    index = -1;
    race_name_split(race, &kind, &index);
    const char *mode = race_kind_text(kind);
    switch (kind) {
    case RACE_KIND_LICENSE_B:
    case RACE_KIND_LICENSE_A:
    case RACE_KIND_LICENSE_IB:
    case RACE_KIND_LICENSE_IA:
    case RACE_KIND_LICENSE_S:
    case RACE_KIND_LICENSE_R:
        sprintf(buf, "%s%d", mode, index + 1);          /* 0x00351AA8 "License B-" + test */
        break;
    default:
        strcpy(buf, mode);
        break;
    }
    setChildText("list_track_mode_info", buf);

    setChildText("list_track_course_info", db->getCourseName(org->courseCode));
}

/* The text of the descendant `name' (findChildByFullName), if there is one. */
// FUNC 0x0016BA30 gt2k_menu::LoggerTrackInfo::setChildText
void LoggerTrackInfo::setChildText(const char *name, const char *text)
{
    RawWindow *w = findChildByFullName(name);
    if (w != NULL)
        w->setText(text);
}

/* Decide from a child is sent on as the item's own decide (handled);
 * anything else: 1 (no base call). */
// FUNC 0x0016BA68 gt2k_menu::LoggerTrackInfo::virtual_17
int LoggerTrackInfo::virtual_17(RawWindow *sender, int event, int arg)
{
    if (event == RAW_EVENT_DECIDE && sender != this) {
        virtual_19(RAW_EVENT_DECIDE, 0);
        return 0;
    }
    return 1;
}

/* ======================================================================== */
/* LoggerTop                                                                */
/* ======================================================================== */

} /* namespace gt2k_menu */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351AB0 = "LoggerTop";                       /* 0x00351AB0 */
gt2k_menu::RawWindowClass g_00351AB8 =                      /* 0x00351AB8 */
    { "LoggerTop", gt2k_menu::LoggerTop::create };
gt2k_menu::RawWindowRegister g_00351AC0(&g_00351AB8);       /* 0x00351AC0 */

namespace gt2k_menu {

// FUNC 0x0016BAB0 gt2k_menu::LoggerTop::virtual_8
const char *LoggerTop::virtual_8()
{
    return g_00351AB0;                      /* "LoggerTop" */
}

/* T055_static_init puts it in the logger's item factory table
 * (g_0029DE48 "logger_top_page"). */
// FUNC 0x0016BAB8 gt2k_menu::LoggerTop::factory
RawWindow *LoggerTop::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_00351AB8.create). */
// FUNC 0x0016BAF8 gt2k_menu::LoggerTop::create
RawWindow *LoggerTop::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new LoggerTop(*args);    /* Object::operator new(0x1F8); operator delete if
                                           the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x0016BB70 gt2k_menu::LoggerTop::canCreate
int LoggerTop::canCreate(const RawWindowArgs *args)
{
    return 1;
}

} /* namespace gt2k_menu */

/* The names of the two LoggerTrackInfo (.sdata). */
const char *g_00351AD8[2] = { "TrackA", "TrackB" };                     /* 0x00351AD8 */

namespace gt2k_menu {

/* The menu control is the owner's (a LoggerControl) pad 1, control1 at its
 * +0; LoggerTop::virtual_14 is called directly.  EH: ~RawWindow() if it
 * throws. */
// FUNC 0x0016BB78 gt2k_menu::LoggerTop::LoggerTop
LoggerTop::LoggerTop(const RawWindowArgs &args)
    : RawWindow(args)
{
    int i;

    blink = 0;
    childrenFocusChange = 0;
    trackSelect = loadChild("RawWindow_TrackSelect", &args, NULL);
    trackSelect->childrenFocusChange = 0;

    RaceOrganization *org = logger_get_org();
    for (i = 0; i < 2; i++) {
        tracks[i] = static_cast<LoggerTrackInfo *>(findChild(g_00351AD8[i]));    /* "TrackA", "TrackB" */
        tracks[i]->setInfo(org, i);
    }
    LoggerTop::virtual_14(&args.owner->control1);
    progress = static_cast<LoggerProgressBar *>(loadChild("LoggerProgressBar_Creating", &args, NULL));
}

// FUNC 0x0016BCA0 gt2k_menu::LoggerTop::~LoggerTop
LoggerTop::~LoggerTop()
{
    /* only ~RawWindow() (0x001D5F18, flags 0), then Object::operator delete
     * if flags & 1 */
    /* implicit: ~RawWindow, operator delete (the base / member destructors,
     * flags & 1) */
}

/* RawWindow's (called directly); the "LoggerMode" list gets the control
 * too. */
// FUNC 0x0016BCF8 gt2k_menu::LoggerTop::virtual_14
void LoggerTop::virtual_14(MenuControl *ctl)
{
    RawWindow::virtual_14(ctl);             /* 0x001D6678 */
    trackSelect->findChild("LoggerMode")->virtual_14(ctl);
}

/* Decide on a "LoggerMode" item: TrackA / TrackB end the logger's pages to
 * load a replay (logger_set_next 1 / 2) if F_00136400 allows it; Exit is
 * cancel; Replay / Graph run the replay page in that mode if the recording
 * is complete.  Cancel: the logger quits.  A refused choice plays
 * MENU_SE_REFUSED (handled), an unknown item or event returns 1.  A choice
 * made leaves the page (exitRequest, setFreeze(0)).  The control is the
 * LoggerControl's control1 (its +0).
 * Retail has one copy each of the cancel code (Exit jumps into it), of the
 * page exit and of `return 1' (after the exit, before the epilogue): the
 * labels below.  Written with the exit in each case, egcs's CSE keeps the
 * switch's constant 1 in $s3 for the exitRequest stores; with the cancel
 * code copied into the Exit branch, the (use (const_int 0)) egcs leaves
 * after the cancel call stops the cross-jump (90.0%). */
// FUNC 0x0016BD50 gt2k_menu::LoggerTop::virtual_17
int LoggerTop::virtual_17(RawWindow *sender, int event, int arg)
{
    LoggerControl *logger = (LoggerControl *)menuControl;

    switch (event) {
    case RAW_EVENT_DECIDE:
        if (sender->isNamed("TrackA")) {
            if (F_00136400(0) == 0) {
                menu_play_se(MENU_SE_REFUSED);
                return 0;
            }
            logger->state = LOGGER_STATE_END;
            logger_set_next(LOGGER_NEXT_LOAD);
        } else if (sender->isNamed("TrackB")) {
            if (F_00136400(1) == 0) {
                menu_play_se(MENU_SE_REFUSED);
                return 0;
            }
            logger->state = LOGGER_STATE_END;
            logger_set_next(LOGGER_NEXT_LOAD_OTHER);
        } else if (sender->isNamed("Exit")) {
            goto cancel;                    /* as cancel */
        } else {
            if (logger_is_recorded() == 0) {
                menu_play_se(MENU_SE_REFUSED);
                return 0;
            }
            if (sender->isNamed("Replay"))
                logger->state = event;      /* LOGGER_STATE_REPLAY: the register holding event */
            else if (sender->isNamed("Graph"))
                logger->state = LOGGER_STATE_GRAPH;
            else
                return 1;
        }
        menu_play_se(MENU_SE_CONFIRM);
        goto leave;
    case RAW_EVENT_CANCEL:
    cancel:                                 /* Exit too: one copy in retail */
        logger_set_next(LOGGER_NEXT_QUIT);
        logger->state = LOGGER_STATE_END;
        menu_play_se(MENU_SE_CANCEL);
    leave:                                  /* a choice made: the page ends */
        pages->exitRequest = 1;
        pages->setFreeze(0);
        return 0;
    }
    return 1;
}

/* The focused "LoggerMode" item pulses (x 1.0 .. 2.0 over 30 frames, half
 * as bright if it cannot be chosen); Replay / Graph when not focused are
 * drawn at 1.0, or 0.5 if they cannot be chosen; TrackA when not focused
 * only at 0.5 when it cannot be chosen.  TrackB and Exit are not set here
 * when they are not focused. */
// FUNC 0x0016BEC8 gt2k_menu::LoggerTop::virtual_9
void LoggerTop::virtual_9()
{
    RawWindow *modeList = findChild("LoggerMode");
    RawWindow *item;
    int replayUnfocused, graphUnfocused, trackAUnfocused;
    f32 scale, brightness;
    int frames = LOGGER_BLINK_FRAMES;       /* retail converts the 30 at run time (li 30; cvt.s.w) */

    if (++blink >= LOGGER_BLINK_FRAMES)
        blink = 0;
    RawWindow *focused = static_cast<RawList *>(modeList)->focusItem;
    scale = (f32)blink / (f32)frames + 1.0f;
    replayUnfocused = 1;
    graphUnfocused = 1;
    trackAUnfocused = 1;

    if (focused->isNamed("Replay")) {
        if (logger_is_recorded() == 0)
            scale *= 0.5f;
        replayUnfocused = 0;
    }
    if (focused->isNamed("Graph")) {
        if (logger_is_recorded() == 0)
            scale *= 0.5f;
        graphUnfocused = 0;
    }
    if (focused->isNamed("TrackA")) {
        if (F_00136400(0) == 0)
            scale *= 0.5f;
        trackAUnfocused = 0;
    }
    focused->applyColourScale(scale);

    if (replayUnfocused) {
        item = modeList->findChild("Replay");
        brightness = 1.0f;
        if (logger_is_recorded() == 0)
            brightness = 0.5f;
        item->applyColourScale(brightness);
    }
    if (graphUnfocused) {
        item = modeList->findChild("Graph");
        brightness = 1.0f;
        if (logger_is_recorded() == 0)
            brightness = 0.5f;
        item->applyColourScale(brightness);
    }
    if (trackAUnfocused) {
        item = modeList->findChild("TrackA");
        if (F_00136400(0) == 0)
            item->applyColourScale(0.5f);
    }
}

} /* namespace gt2k_menu */

using namespace gt2k_menu;

/* ======================================================================== */
/* Static destructor / constructor, inline functions, RTTI                  */
/* ======================================================================== */

/* Compiler-generated: egcs's static destructor and initialiser of the three
 * registrars above (named after the unit's first global), emitted here,
 * after the last out-of-line function. */
// FUNC 0x0016C0E0 T086_static_dtor (_GLOBAL_$D$g_00351A60)
//   (.dtors entry 0x0028F1F4) ~RawWindowRegister (flags 2) of the
//   registrars, in reverse order.
/* implicit: ~RawWindowRegister */
// FUNC 0x0016C118 T086_static_init (_GLOBAL_$I$g_00351A60)
//   (.ctors entry 0x0028F0B4) RawWindowRegister(&class) of the three.
/* implicit: RawWindowRegister */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo functions. */
// FUNC 0x0016C160 __tfQ29gt2k_menu9LoggerTop
extern "C" void *__tfQ29gt2k_menu9LoggerTop(void)
{
    if (__tiQ29gt2k_menu9LoggerTop[0] == 0) {
        __tfQ29gt2k_menu9RawWindow();
        __rtti_si(__tiQ29gt2k_menu9LoggerTop, "Q29gt2k_menu9LoggerTop",
                  (const type_info *)__tiQ29gt2k_menu9RawWindow);
    }
    return __tiQ29gt2k_menu9LoggerTop;
}

// FUNC 0x0016C1B0 __tfQ29gt2k_menu15LoggerTrackInfo
extern "C" void *__tfQ29gt2k_menu15LoggerTrackInfo(void)
{
    if (__tiQ29gt2k_menu15LoggerTrackInfo[0] == 0) {
        __tfQ29gt2k_menu16RawListItemFixed();
        __rtti_si(__tiQ29gt2k_menu15LoggerTrackInfo, "Q29gt2k_menu15LoggerTrackInfo",
                  (const type_info *)__tiQ29gt2k_menu16RawListItemFixed);
    }
    return __tiQ29gt2k_menu15LoggerTrackInfo;
}
#endif

// FUNC 0x0016C200 gt2k_menu::LoggerProgressBar::setTotal
/* inline in game/debug/T086_logger_top.h; egcs emits this copy (no callers: the
 * recording thread writes the fields directly): total = n */

// FUNC 0x0016C208 gt2k_menu::LoggerProgressBar::setCurrent
/* inline in game/debug/T086_logger_top.h; egcs emits this copy (no callers):
 * current = n */

#ifndef BOOT_OLD_GCC
// FUNC 0x0016C210 __tfQ29gt2k_menu17LoggerProgressBar
extern "C" void *__tfQ29gt2k_menu17LoggerProgressBar(void)
{
    if (__tiQ29gt2k_menu17LoggerProgressBar[0] == 0) {
        __tfQ29gt2k_menu9RawWindow();
        __rtti_si(__tiQ29gt2k_menu17LoggerProgressBar, "Q29gt2k_menu17LoggerProgressBar",
                  (const type_info *)__tiQ29gt2k_menu9RawWindow);
    }
    return __tiQ29gt2k_menu17LoggerProgressBar;
}
#endif
