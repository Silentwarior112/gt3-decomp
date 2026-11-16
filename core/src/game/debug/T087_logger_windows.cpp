/* T087 - the data logger's replay / graph windows.  0x0016C260..0x00171968
 * (one translation unit; units u038 and u039 had it split at 0x0016E640).
 *
 * The logged race (logger_get_org(), T055): its race gives the two cars'
 * inputs (RaceBase::virtual_26: 0x100 = the player's car, 1 = the other
 * one; RaceInput, the first pass's CarDynamics) and their sector records
 * (virtual_4's pool, OneLapGhostInfo).
 *
 * The "replay_logger" page (T055) holds a LoggerReplay; its LoggerRoot has
 * the sector list (LoggerSector "Sectors") and two modes chosen by
 * LoggerControl::state: the replay (LoggerReplayRoot "ReplayRoot": the race
 * view LoggerRaceReplay, the buttons LoggerReplayControl "ReplayCtrl", the
 * two cars' info panels) and the graph (LoggerGraphRoot "GraphRoot": the
 * graph LoggerRaceGraph, the course map LoggerMapCourse).  L2 / R2 switch the
 * mode, L1 / R1 move the sector list's cursor (its left / right events),
 * Start opens the graph's option panel ("LoggerGraphOperation": the throttle
 * / brake / speed curves on or off, which cars), cancel on the sector list
 * goes back to the logger's "top" page.
 *
 * The graph (LoggerRaceGraph) is drawn into a 640 x 268 GS buffer whose
 * projection has x = the lap distance (left .. right metres around the
 * cursor `position', 640 / zoom wide) and y going up (logger_windows_internal.h): from the
 * bottom the pedal bands of the cars shown, then the speed graph; the
 * sector lines, the cursor with the distance (miles) and the cars' speeds
 * at it (mph).  The speed curves are drawn once into a sub-packet that is
 * sent five times with the drawing offset moved by a pixel between the
 * copies (a thicker line).  The buffer is then drawn on the screen as three
 * sprites: as it is, and a line up and a line down at 1/3 and 1/4 alpha (a
 * vertical blur).  LoggerGraphRoot reads the sticks every frame (the left
 * stick zooms, the right one moves the cursor along the lap and scrolls the
 * graph) and shows per car the speed at the cursor, the lap time, the
 * sector's time and average speed, and the cursor's place on the course map
 * ("graph_point").
 *
 * Every class comes with the RawWindow registry boilerplate (as T272's):
 * its record (the name pointer slot 8 returns, the RawWindowClass {name,
 * create}, the registrar, defined in front of the class's first function:
 * retail's .sdata order), slot 8 (the class name), factory (the 8 words of
 * a layout factory packed into a RawWindowArgs on the stack, field by field;
 * no callers but LoggerReplay's), create (the one in the class's
 * RawWindowClass record: Object::operator new + the constructor if
 * canCreate(args)) and canCreate (1, the arguments unused).  The landing
 * pads only free the object (Object::operator delete) when its constructor
 * throws.  The destructors only run the base's (the egcs deleting form calls
 * Object::operator delete when bit 0 of the flags is set).  egcs builds /
 * destroys the registrars itself (T087_static_init / _dtor).
 */
#include "game/debug/logger_windows_internal.h"

using namespace gt2k_menu;

/* T087's data starts unreferenced: "bad_alloc" (<new>, 0x00323EE0) opens its
 * .rodata, the "" of T157's inline RaceMessageDisplay::clear (logger_windows_internal.h's T154)
 * its .sdata (0x00351AF8, the word the first pass took for a variable). */
// DATA_START 0x00323EE0
// DATA_START 0x00351AF8

/* T087's .data (0x002AED68, before its frame info): the graph options (all
 * on), the colours of the throttle and brake bands and of the speed curves
 * per car (rgba: A red, B blue) and the speed graph's lines per scale. */
LoggerGraphOptions g_002AED68 = { 1, 1, 1, 1, { 1, 1 } };    /* 0x002AED68 */
f32 g_002AED80[2][4] = {                                    /* 0x002AED80 */
    { 0.25f, 1.0f, 0.15f, 1.0f },
    { 0.25f, 1.0f, 0.15f, 1.0f }
};
f32 g_002AEDA0[2][4] = {                                    /* 0x002AEDA0 */
    { 1.0f, 0.25f, 0.15f, 1.0f },
    { 1.0f, 0.25f, 0.15f, 1.0f }
};
f32 g_002AEDC0[2][4] = {                                    /* 0x002AEDC0 */
    { 0.809f, 0.29f, 0.295f, 1.0f },
    { 0.318f, 0.489f, 0.739f, 1.0f }
};
int g_002AEDE0[3] = { 800, 533, 400 };                      /* 0x002AEDE0 */

/* .sdata after the "": the graph of the open LoggerRoot. */
gt2k_menu::LoggerRaceGraph *g_00351AFC = NULL;              /* 0x00351AFC */

/* ======================================================================== */
/* Helpers over the logged race                                             */
/* ======================================================================== */

// FUNC 0x0016C260 logger_graph_options
LoggerGraphOptions *logger_graph_options(void)
{
    return &g_002AED68;
}

/* Car 0 = the player's (input id 0x100), car 1 = the other one (id 1). */
// FUNC 0x0016C270 logger_car_input
RaceInput *logger_car_input(int car)
{
    return logger_get_org()->race->virtual_26(car == 0 ? 0x100 : 1);
}

// FUNC 0x0016C2B8 logger_entry_car
RaceEntryCar *logger_entry_car(int car)
{
    return logger_get_org()->entries->cars[car];
}

// FUNC 0x0016C2F0 logger_race_pool
RaceMemPool *logger_race_pool(void)
{
    return logger_get_org()->race->virtual_4();
}

/* The time of sector `sector' of car `car' (ms): the difference of the two
 * cumulative times (sector 0: its own).  The records through
 * logger_pool_sector (logger_windows_internal.h): retail multiplies sector - 1 by 0x1B00. */
// FUNC 0x0016C328 logger_sector_time
int logger_sector_time(int car, int sector)
{
    if (sector == 0)
        return logger_pool_sector(logger_race_pool(), car, 0)->time;
    OneLapGhostInfo *cur = logger_pool_sector(logger_race_pool(), car, sector);
    OneLapGhostInfo *prev = logger_pool_sector(logger_race_pool(), car, sector - 1);
    return cur->time - prev->time;
}

/* The pool's course map (the call through the vtable is inline, not a call
 * of logger_race_pool). */
// FUNC 0x0016C3C8 logger_course_map
LoggerCourseMap *logger_course_map(void)
{
    return logger_get_org()->race->virtual_4()->courseMap;
}

/* The number of sectors both cars recorded: per car, the records from the
 * first that have a length (at most lastSector + 1); the smaller non-zero
 * count of the two, lastSector + 1 if neither has any.  A `for' with a
 * `break' (retail tests recorded < limit first and the length after). */
// FUNC 0x0016C400 logger_sector_count
int logger_sector_count(void)
{
    RaceMemPool *pool = logger_race_pool();
    int limit = pool->lastSector + 1;
    int result = limit;
    for (int car = 0; car < 2; car++) {
        int recorded;
        for (recorded = 0; recorded < limit; recorded++) {
            OneLapGhostInfo *rec = logger_pool_sector(pool, car, recorded);
            if (rec->length == 0)
                break;
        }
        if (recorded != 0 && recorded < result)
            result = recorded;
    }
    return result;
}

/* ======================================================================== */
/* LoggerSector                                                             */
/* ======================================================================== */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351B00 = "LoggerSector";                    /* 0x00351B00 */
gt2k_menu::RawWindowClass g_00351B08 =                      /* 0x00351B08 */
    { "LoggerSector", gt2k_menu::LoggerSector::create };
gt2k_menu::RawWindowRegister g_00351B10(&g_00351B08);       /* 0x00351B10 */

// FUNC 0x0016C4A8 gt2k_menu::LoggerSector::virtual_8
const char *LoggerSector::virtual_8()
{
    return g_00351B00;      /* "LoggerSector" */
}

/* No callers in the image: the layout factory.  The record is filled field
 * by field: an aggregate initializer makes egcs clear it with memset first
 * (store_constructor counts the class's TYPE_DECLs as fields), which retail
 * does not do. */
// FUNC 0x0016C4B0 gt2k_menu::LoggerSector::factory
RawWindow *LoggerSector::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
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

/* The registry's create function (g_00351B08.create).  canCreate takes the
 * arguments (unused): the call keeps args in $a0, so the copy into $s1 is
 * scheduled before $ra's save, as in retail (T085's form). */
// FUNC 0x0016C4F0 gt2k_menu::LoggerSector::create
RawWindow *LoggerSector::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new LoggerSector(*args);     /* Object::operator new(0x258); Object::operator delete if
                                               the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x0016C568 gt2k_menu::LoggerSector::canCreate
int LoggerSector::canCreate(const RawWindowArgs *args)
{
    return 1;
}

/* A horizontal list (spacing 6) of one "RawListItem" "icon_sector<n>" per
 * sector (n from 1); totalTime[car] ends as the cumulative time of the last
 * sector (a car whose input is held keeps 0).  The counters are unsigned
 * (sltu / sltiu). */
// FUNC 0x0016C570 gt2k_menu::LoggerSector::LoggerSector
LoggerSector::LoggerSector(const RawWindowArgs &args) : RawList(args)
{
    vertical = 0;
    spacing = 6.0f;
    RaceMemPool *pool = logger_get_org()->race->virtual_4();   /* sp+0x40 */
    totalTime[1] = 0;
    totalTime[0] = 0;
    u32 count = (u32)logger_sector_count();
    for (u32 sector = 0; sector < count; sector++) {
        char name[0x40];                    /* sp+0x00 */
        sprintf(name, "icon_sector%d", sector + 1);
        addItem((RawListItem *)createChild("RawListItem", name, &args, 0, 0, 0, 0), 1);
        for (u32 car = 0; car < 2; car++) {
            if (logger_car_input(car)->hold == 0) {
                OneLapGhostInfo *rec = logger_pool_sector(pool, car, sector);
                totalTime[car] = rec->time;
            }
        }
    }
}

// FUNC 0x0016C710 gt2k_menu::LoggerSector::~LoggerSector
LoggerSector::~LoggerSector()
{
    /* RawList's destructor; the deleting form calls Object::operator delete. */
    /* implicit: ~RawList, operator delete (the base / member destructors,
     * flags & 1) */
}

/* The cursor moved to item: the replay seeks to that sector and the graph's
 * cursor goes to it; then RawList's (its result). */
// FUNC 0x0016C768 gt2k_menu::LoggerSector::virtual_28
int LoggerSector::virtual_28(RawListItem *item)
{
    int sector = getIndex(item);
    logger_request_seek(sector);
    graph->setCursorToSector(sector);
    return RawList::virtual_28(item);
}

/* No pad input while the graph's unk_5DC is set. */
// FUNC 0x0016C7C8 gt2k_menu::LoggerSector::virtual_16
int LoggerSector::virtual_16(MenuControl *ctl)
{
    if (graph->unk_5DC != 0)
        return 0;
    return RawWindow::virtual_16(ctl);
}

/* Decide or up (the replay buttons are above the horizontal sector list)
 * hand the menu control to the replay control when it is shown. */
// FUNC 0x0016C7F8 gt2k_menu::LoggerSector::virtual_17
int LoggerSector::virtual_17(RawWindow *sender, int event, int arg)
{
    if ((event == RAW_EVENT_DECIDE || event == RAW_EVENT_UP) && replayControl->isShown() != 0) {
        replayControl->virtual_14(menuControl);
        virtual_14(NULL);
        menu_play_se(MENU_SE_CURSOR);
        return 0;
    }
    return RawList::virtual_17(sender, event, arg);
}

/* ======================================================================== */
/* LoggerMapCourse                                                          */
/* ======================================================================== */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351B18 = "LoggerMapCourse";                 /* 0x00351B18 */
gt2k_menu::RawWindowClass g_00351B20 =                      /* 0x00351B20 */
    { "LoggerMapCourse", gt2k_menu::LoggerMapCourse::create };
gt2k_menu::RawWindowRegister g_00351B28(&g_00351B20);       /* 0x00351B28 */

// FUNC 0x0016C8B8 gt2k_menu::LoggerMapCourse::virtual_8
const char *LoggerMapCourse::virtual_8()
{
    return g_00351B18;      /* "LoggerMapCourse" */
}

/* No callers in the image: the layout factory. */
// FUNC 0x0016C8C0 gt2k_menu::LoggerMapCourse::factory
RawWindow *LoggerMapCourse::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
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

/* The registry's create function (g_00351B20.create). */
// FUNC 0x0016C900 gt2k_menu::LoggerMapCourse::create
RawWindow *LoggerMapCourse::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new LoggerMapCourse(*args);  /* Object::operator new(0x1E8); Object::operator delete if
                                               the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x0016C978 gt2k_menu::LoggerMapCourse::canCreate
int LoggerMapCourse::canCreate(const RawWindowArgs *args)
{
    return 1;
}

/* sectors is not initialised (LoggerRoot::setup sets it). */
// FUNC 0x0016C980 gt2k_menu::LoggerMapCourse::LoggerMapCourse
LoggerMapCourse::LoggerMapCourse(const RawWindowArgs &args) : RawWindow(args)
{
}

// FUNC 0x0016C9B8 gt2k_menu::LoggerMapCourse::~LoggerMapCourse
LoggerMapCourse::~LoggerMapCourse()
{
    /* RawWindow's destructor; the deleting form calls Object::operator delete. */
    /* implicit: ~RawWindow, operator delete (the base / member destructors,
     * flags & 1) */
}

/* The course map in the window's rectangle, the selected sector passed
 * along. */
// FUNC 0x0016CA10 gt2k_menu::LoggerMapCourse::virtual_12
void LoggerMapCourse::virtual_12()
{
    LoggerCourseMap *map = logger_course_map();
    f32 left = getScreenLeft();
    f32 top = getScreenTop();
    f32 width = w;
    f32 height = h;
    map->draw(sectors->getIndex(NULL), left, top, width, height);
}

/* ======================================================================== */
/* LoggerRaceReplay                                                         */
/* ======================================================================== */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351B30 = "LoggerRaceReplay";                /* 0x00351B30 */
gt2k_menu::RawWindowClass g_00351B38 =                      /* 0x00351B38 */
    { "LoggerRaceReplay", gt2k_menu::LoggerRaceReplay::create };
gt2k_menu::RawWindowRegister g_00351B40(&g_00351B38);       /* 0x00351B40 */

// FUNC 0x0016CAA8 gt2k_menu::LoggerRaceReplay::virtual_8
const char *LoggerRaceReplay::virtual_8()
{
    return g_00351B30;      /* "LoggerRaceReplay" */
}

/* No callers in the image: the layout factory. */
// FUNC 0x0016CAB0 gt2k_menu::LoggerRaceReplay::factory
RawWindow *LoggerRaceReplay::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
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

/* The registry's create function (g_00351B38.create). */
// FUNC 0x0016CAF0 gt2k_menu::LoggerRaceReplay::create
RawWindow *LoggerRaceReplay::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new LoggerRaceReplay(*args); /* Object::operator new(0x1EC); Object::operator delete if
                                               the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x0016CB68 gt2k_menu::LoggerRaceReplay::canCreate
int LoggerRaceReplay::canCreate(const RawWindowArgs *args)
{
    return 1;
}

/* Becomes the window the LoggerRender draws over the race (args.owner is
 * the LoggerControl: render.window, the store of LoggerControl::setWindow);
 * the logged race. */
// FUNC 0x0016CB70 gt2k_menu::LoggerRaceReplay::LoggerRaceReplay
LoggerRaceReplay::LoggerRaceReplay(const RawWindowArgs &args) : RawWindow(args)
{
    ((LoggerControl *)args.owner)->render.window = this;
    org = logger_get_org();
    race = (RacePS2Base *)org->race;        /* the logger's race is a RaceSolitaire */
}

// FUNC 0x0016CBF8 gt2k_menu::LoggerRaceReplay::~LoggerRaceReplay
LoggerRaceReplay::~LoggerRaceReplay()
{
    /* RawWindow's destructor; the deleting form calls Object::operator delete. */
    /* implicit: ~RawWindow, operator delete (the base / member destructors,
     * flags & 1) */
}

/* Called by LoggerRender::render (T055) before the menu: when shown and the
 * race renders, makes buf the draw buffer, clears it and draws the race view
 * into the rectangle (0, 12, 640, 224) of a copy of buf, the mirror at
 * (260, 10) 120 x 32, zoom 1.3 (RacePS2Base::setMirrorRect, inline in
 * T130_race_ps2_base.h); then the race's virtual_45 (empty in RacePS2Base)
 * on buf. */
// FUNC 0x0016CC50 gt2k_menu::LoggerRaceReplay::renderRace
void LoggerRaceReplay::renderRace(GSBuffer &buf)
{
    if (isShown() == 0)
        return;
    RacePS2Base *r = race;
    if (r->renderEnabled == 0)
        return;
    buf.setDrawBuffer();
    gs_set_clear_color(0);
    gs_clear_buffers(PGL_CLEAR_COLOR);
    {
        /* implicit: GSBuffer::copy (the inline copy constructor) */
        GSBuffer view(buf);                 /* sp+0x00 (inline: copy); ~GSBuffer with flags 2 */
        view.setRect(0, 12, 640, 224);
        r->setMirrorRect(320, 26, 120, 32, 1.3f);      /* (260, 10) 120 x 32 */
        r->drawSubView(&view, 0, 1.0f);
        r->virtual_45(&buf);
    }
}

/* ======================================================================== */
/* LoggerReplayControl                                                      */
/* ======================================================================== */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351B48 = "LoggerReplayControl";             /* 0x00351B48 */
gt2k_menu::RawWindowClass g_00351B50 =                      /* 0x00351B50 */
    { "LoggerReplayControl", gt2k_menu::LoggerReplayControl::create };
gt2k_menu::RawWindowRegister g_00351B58(&g_00351B50);       /* 0x00351B58 */

// FUNC 0x0016CD90 gt2k_menu::LoggerReplayControl::virtual_8
const char *LoggerReplayControl::virtual_8()
{
    return g_00351B48;      /* "LoggerReplayControl" */
}

/* No callers in the image: the layout factory. */
// FUNC 0x0016CD98 gt2k_menu::LoggerReplayControl::factory
RawWindow *LoggerReplayControl::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
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

/* The registry's create function (g_00351B50.create). */
// FUNC 0x0016CDD8 gt2k_menu::LoggerReplayControl::create
RawWindow *LoggerReplayControl::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new LoggerReplayControl(*args);  /* Object::operator new(0x24C); Object::operator delete
                                                   if the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x0016CE50 gt2k_menu::LoggerReplayControl::canCreate
int LoggerReplayControl::canCreate(const RawWindowArgs *args)
{
    return 1;
}

// FUNC 0x0016CE58 gt2k_menu::LoggerReplayControl::LoggerReplayControl
LoggerReplayControl::LoggerReplayControl(const RawWindowArgs &args) : RawList(args)
{
    vertical = 0;
}

// FUNC 0x0016CE90 gt2k_menu::LoggerReplayControl::~LoggerReplayControl
LoggerReplayControl::~LoggerReplayControl()
{
    /* RawList's destructor; the deleting form calls Object::operator delete. */
    /* implicit: ~RawList, operator delete (the base / member destructors,
     * flags & 1) */
}

/* Decide (sound 10), by the button's name: "ReplayStart" plays at the
 * normal speed, "ReplaySlow" at half speed, "ReplayStop" stops,
 * "ReplayCamera" asks for a camera change (logger_request_step); other
 * buttons go to the list.  Cancel stops the replay and goes on as down:
 * unless the replay plays, the menu control goes back to the sector list
 * (below the buttons) when this list has it. */
// FUNC 0x0016CEE8 gt2k_menu::LoggerReplayControl::virtual_17
int LoggerReplayControl::virtual_17(RawWindow *sender, int event, int arg)
{
    switch (event) {
    case RAW_EVENT_DECIDE:
        menu_play_se(MENU_SE_LIST_MOVE);
        if (sender->isNamed("ReplayStart") != 0) {
            logger_set_speed(LOGGER_SPEED_NORMAL);
            logger_set_play(1);
            return 0;
        }
        if (sender->isNamed("ReplaySlow") != 0) {
            logger_set_speed(LOGGER_SPEED_SLOW);
            logger_set_play(1);
            return 0;
        }
        if (sender->isNamed("ReplayStop") != 0) {
            logger_set_play(0);
            return 0;
        }
        if (sender->isNamed("ReplayCamera") != 0) {
            logger_request_step();
            return 0;
        }
        break;
    case RAW_EVENT_CANCEL:
        logger_set_play(0);
        /* fall through */
    case RAW_EVENT_DOWN:
        if (logger_is_playing() != 0)
            return 0;
        if (menuControl == NULL)
            return 0;
        sectors->virtual_14(menuControl);
        virtual_14(NULL);
        menu_play_se(MENU_SE_CURSOR);
        return 0;
    }
    return RawList::virtual_17(sender, event, arg);
}

/* ======================================================================== */
/* LoggerReplayRoot                                                         */
/* ======================================================================== */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351B60 = "LoggerReplayRoot";                /* 0x00351B60 */
gt2k_menu::RawWindowClass g_00351B68 =                      /* 0x00351B68 */
    { "LoggerReplayRoot", gt2k_menu::LoggerReplayRoot::create };
gt2k_menu::RawWindowRegister g_00351B70(&g_00351B68);       /* 0x00351B70 */

/* The two cars' panels of the replay mode (LoggerReplayRoot::setup). */
const char *g_00351B78[2] = { "ReplayTrackA", "ReplayTrackB" };  /* 0x00351B78 */

// FUNC 0x0016D078 gt2k_menu::LoggerReplayRoot::virtual_8
const char *LoggerReplayRoot::virtual_8()
{
    return g_00351B60;      /* "LoggerReplayRoot" */
}

/* No callers in the image: the layout factory. */
// FUNC 0x0016D080 gt2k_menu::LoggerReplayRoot::factory
RawWindow *LoggerReplayRoot::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
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

/* The registry's create function (g_00351B68.create). */
// FUNC 0x0016D0C0 gt2k_menu::LoggerReplayRoot::create
RawWindow *LoggerReplayRoot::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new LoggerReplayRoot(*args); /* Object::operator new(0x20C); Object::operator delete if
                                               the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x0016D138 gt2k_menu::LoggerReplayRoot::canCreate
int LoggerReplayRoot::canCreate(const RawWindowArgs *args)
{
    return 1;
}

/* The menu control and the focus stay with this window (not its
 * children). */
// FUNC 0x0016D140 gt2k_menu::LoggerReplayRoot::LoggerReplayRoot
LoggerReplayRoot::LoggerReplayRoot(const RawWindowArgs &args) : RawWindow(args)
{
    childrenFocusChange = 0;
}

// FUNC 0x0016D178 gt2k_menu::LoggerReplayRoot::~LoggerReplayRoot
LoggerReplayRoot::~LoggerReplayRoot()
{
    /* RawWindow's destructor; the deleting form calls Object::operator delete. */
    /* implicit: ~RawWindow, operator delete (the base / member destructors,
     * flags & 1) */
}

/* Called by LoggerRoot::setup: finds the two panels "ReplayTrackA" /
 * "ReplayTrackB" (g_00351B78) and their four info texts, sets their icon
 * and title images; panel B moves to x = 328. */
// FUNC 0x0016D1D0 gt2k_menu::LoggerReplayRoot::setup
void LoggerReplayRoot::setup(LoggerSector *s)
{
    sectors = s;
    for (int car = 0; car < 2; car++) {
        RawWindow *panel = findChild(g_00351B78[car]);
        speedInfo[car] = panel->findChildByFullName("r_speed_info");
        totalInfo[car] = panel->findChildByFullName("r_total_info");
        sectorInfo[car] = panel->findChildByFullName("r_sector_info");
        averageInfo[car] = panel->findChildByFullName("r_average_info");
        if (car == 0) {
            panel->findChildByFullName("r_icon")->changeImage("icon_trackA", 0, 0);
            panel->findChildByFullName("r_title")->changeImage("title_trackA", 0, 0);
        } else {
            panel->findChildByFullName("r_icon")->changeImage("icon_trackB", 0, 0);
            panel->findChildByFullName("r_title")->changeImage("title_trackB", 0, 0);
            panel->setLeft(328.0f);
        }
    }
}

// FUNC 0x0016D388 gt2k_menu::LoggerReplayRoot::getSectors
LoggerSector *LoggerReplayRoot::getSectors()
{
    return sectors;
}

/* Caller LoggerRoot::start: the menu control goes (back) to the sector list
 * (this window's); the replay stops. */
// FUNC 0x0016D390 gt2k_menu::LoggerReplayRoot::leave
void LoggerReplayRoot::leave()
{
    getSectors()->virtual_14(menuControl);
    logger_set_play(0);
}

/* Caller LoggerRoot::virtual_17: the sector list loses the menu control but
 * keeps the focus look (virtual_20(1)); the replay plays. */
// FUNC 0x0016D3D8 gt2k_menu::LoggerReplayRoot::enterPlaying
void LoggerReplayRoot::enterPlaying()
{
    getSectors()->virtual_14(NULL);
    getSectors()->virtual_20(1);
    logger_set_play(1);
}

/* The same with the replay stopped.  No callers. */
// FUNC 0x0016D440 gt2k_menu::LoggerReplayRoot::enterPaused
void LoggerReplayRoot::enterPaused()
{
    getSectors()->virtual_14(NULL);
    getSectors()->virtual_20(1);
    logger_set_play(0);
}

/* Every frame, per car whose input is not held: the current speed, the
 * total time, the selected sector's time and the average speed over the
 * sector (in the speed unit: "%3d%s", the .sdata string 0x00351B90).  One
 * variable per text window and per speed (one reused `info' gave 93.1%).
 * Left (96.4%, also with 991111-01): global-alloc gives sector $s6 and
 * &g_002F59D8 $fp where retail has $s7 / $s6 (the address's lui lands
 * before the sector copy there); a UnitSettings pointer or reference local
 * (before / after sector, in the loop) is the same or worse. */
// FUNC 0x0016D4A8 gt2k_menu::LoggerReplayRoot::virtual_9
void LoggerReplayRoot::virtual_9()
{
    char text[0x100];                       /* sp+0x00 */
    int sector = sectors->getIndex(NULL);
    for (int car = 0; car < 2; car++) {
        if (logger_car_input(car)->hold != 0)
            continue;

        RaceEntryCar *entry = logger_entry_car(car);
        RawWindow *speedText = speedInfo[car];
        int speed = (int)g_002F59D8.convSpeed(entry->state->speed);
        sprintf(text, "%3d%s", speed, g_002F59D8.speedName());
        speedText->setText(text);

        RawWindow *totalText = totalInfo[car];
        format_time(text, sectors->totalTime[car]);
        totalText->setText(text);

        RawWindow *sectorText = sectorInfo[car];
        format_time(text, logger_sector_time(car, sector));
        sectorText->setText(text);

        RawWindow *averageText = averageInfo[car];
        LoggerData *data = logger_get_data();
        f32 start = graph->getSectorStart(sector);
        f32 end = graph->getSectorEnd(sector);
        int avg = (int)g_002F59D8.convSpeed(data->avgSpeed(car, start, end));
        sprintf(text, "%3d%s", avg, g_002F59D8.speedName());
        averageText->setText(text);
    }
}

/* Only RawWindow's. */
// FUNC 0x0016D6A0 gt2k_menu::LoggerReplayRoot::virtual_16
int LoggerReplayRoot::virtual_16(MenuControl *ctl)
{
    return RawWindow::virtual_16(ctl);
}

/* Only RawWindow's (which returns 1). */
// FUNC 0x0016D6C0 gt2k_menu::LoggerReplayRoot::virtual_17
int LoggerReplayRoot::virtual_17(RawWindow *sender, int event, int arg)
{
    return RawWindow::virtual_17(sender, event, arg);
}

/* ======================================================================== */
/* LoggerGraphMap                                                           */
/* ======================================================================== */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351B98 = "LoggerGraphMap";                  /* 0x00351B98 */
gt2k_menu::RawWindowClass g_00351BA0 =                      /* 0x00351BA0 */
    { "LoggerGraphMap", gt2k_menu::LoggerGraphMap::create };
gt2k_menu::RawWindowRegister g_00351BA8(&g_00351BA0);       /* 0x00351BA8 */

// FUNC 0x0016D6E0 gt2k_menu::LoggerGraphMap::virtual_8
const char *LoggerGraphMap::virtual_8()
{
    return g_00351B98;      /* "LoggerGraphMap" */
}

/* No callers in the image: the layout factory. */
// FUNC 0x0016D6E8 gt2k_menu::LoggerGraphMap::factory
RawWindow *LoggerGraphMap::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
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

/* The registry's create function (g_00351BA0.create). */
// FUNC 0x0016D728 gt2k_menu::LoggerGraphMap::create
RawWindow *LoggerGraphMap::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new LoggerGraphMap(*args);   /* Object::operator new(0x1E4); Object::operator delete if
                                               the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x0016D7A0 gt2k_menu::LoggerGraphMap::canCreate
int LoggerGraphMap::canCreate(const RawWindowArgs *args)
{
    return 1;
}

// FUNC 0x0016D7A8 gt2k_menu::LoggerGraphMap::LoggerGraphMap
LoggerGraphMap::LoggerGraphMap(const RawWindowArgs &args) : RawWindow(args)
{
}

// FUNC 0x0016D7E0 gt2k_menu::LoggerGraphMap::~LoggerGraphMap
LoggerGraphMap::~LoggerGraphMap()
{
    /* RawWindow's destructor; the deleting form calls Object::operator delete. */
    /* implicit: ~RawWindow, operator delete (the base / member destructors,
     * flags & 1) */
}

/* ======================================================================== */
/* LoggerRaceGraph                                                          */
/* ======================================================================== */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351BB0 = "LoggerRaceGraph";                 /* 0x00351BB0 */
gt2k_menu::RawWindowClass g_00351BB8 =                      /* 0x00351BB8 */
    { "LoggerRaceGraph", gt2k_menu::LoggerRaceGraph::create };
gt2k_menu::RawWindowRegister g_00351BC0(&g_00351BB8);       /* 0x00351BC0 */

/* Five inline members defined out of the class, here: after
 * LoggerReplayRoot::virtual_9, which calls getSectorStart / getSectorEnd
 * (the later users, sectorLength, virtual_12, setPosition and
 * LoggerGraphRoot::virtual_9, expand them).  egcs writes their copies at the end of the unit, first of
 * the inline members, in the reverse order of these definitions
 * (0x001714A0..0x00171540; the frame info's order). */
// FUNC 0x00171540 gt2k_menu::LoggerRaceGraph::getSectorStart (getSectorStart__Q29gt2k_menu15LoggerRaceGraphi)
inline f32 LoggerRaceGraph::getSectorStart(int i)
{
    return sectorStart[i];
}

/* The end of sector i: the next one's start, the recorded length for the
 * last one. */
// FUNC 0x00171500 gt2k_menu::LoggerRaceGraph::getSectorEnd (getSectorEnd__Q29gt2k_menu15LoggerRaceGraphi)
inline f32 LoggerRaceGraph::getSectorEnd(int i)
{
    if (i != sectorCount - 1)
        return sectorStart[i + 1];
    return logger_get_length();
}

/* (no callers) */
// FUNC 0x001714E0 gt2k_menu::LoggerRaceGraph::F_001714E0 (F_001714E0__Q29gt2k_menu15LoggerRaceGraph)
inline f32 LoggerRaceGraph::F_001714E0()
{
    return left - zoom * 5.0f;
}

/* (no callers) */
// FUNC 0x001714C0 gt2k_menu::LoggerRaceGraph::F_001714C0 (F_001714C0__Q29gt2k_menu15LoggerRaceGraph)
inline f32 LoggerRaceGraph::F_001714C0()
{
    return right + zoom * 5.0f;
}

/* (no callers) three pixels, in metres */
// FUNC 0x001714A0 gt2k_menu::LoggerRaceGraph::F_001714A0 (F_001714A0__Q29gt2k_menu15LoggerRaceGraph)
inline f32 LoggerRaceGraph::F_001714A0()
{
    return 3.0f / zoom;
}

// FUNC 0x0016D838 gt2k_menu::LoggerRaceGraph::virtual_8
const char *LoggerRaceGraph::virtual_8()
{
    return g_00351BB0;      /* "LoggerRaceGraph" */
}

/* No callers in the image: the layout factory. */
// FUNC 0x0016D840 gt2k_menu::LoggerRaceGraph::factory
RawWindow *LoggerRaceGraph::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
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

/* The registry's create function (g_00351BB8.create). */
// FUNC 0x0016D880 gt2k_menu::LoggerRaceGraph::create
RawWindow *LoggerRaceGraph::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new LoggerRaceGraph(*args);  /* Object::operator new(0x5E4); Object::operator delete if
                                               the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x0016D8F8 gt2k_menu::LoggerRaceGraph::canCreate
int LoggerRaceGraph::canCreate(const RawWindowArgs *args)
{
    return 1;
}

// FUNC 0x0016D900 gt2k_menu::LoggerRaceGraph::LoggerRaceGraph
LoggerRaceGraph::LoggerRaceGraph(const RawWindowArgs &args) : RawWindow(args)
{
    unk_5DC = 0;
    lrCancel = 0;
}

// FUNC 0x0016D940 gt2k_menu::LoggerRaceGraph::~LoggerRaceGraph
LoggerRaceGraph::~LoggerRaceGraph()
{
    /* RawWindow's destructor; the deleting form calls Object::operator delete. */
    /* implicit: ~RawWindow, operator delete (the base / member destructors,
     * flags & 1) */
}

/* Called by LoggerRoot::setup.  The cars come from the parent
 * LoggerGraphRoot (the cast is not checked: a NULL parent is dereferenced).
 * The sector starts are the course's boundary distances (sector 0 at 0),
 * each made larger than the previous one by adding the lap length (the
 * boundaries of a course that crosses the start line); length ends with
 * those wraps added.  Zoom 1, speedMax 800 (speedHeight 800); then the
 * cursor goes to sector 0. */
// FUNC 0x0016D998 gt2k_menu::LoggerRaceGraph::setup
void LoggerRaceGraph::setup(LoggerSector *s)
{
    sectors = s;
    /* parentWindow(): dynamic_cast<RawWindow *>(parent), inline. */
    LoggerGraphRoot *root = (LoggerGraphRoot *)parentWindow();
    /* implicit: __dynamic_cast (the inline parentWindow) */
    cars[0] = root->cars[0];
    cars[1] = root->cars[1];
    RaceOrganization *org = root->org;
    f32 lapLength = root->race->org->course->getLapLength();
    f32 prevStart = -1.0f;
    lengthSum[0] = 0;
    lengthSum[1] = 0;
    length = lapLength;
    sectorCount = logger_sector_count();
    f32 wrap = 0.0f;
    for (int i = 0; i < sectorCount; i++) {
        for (int car = 0; car < 2; car++) {
            RaceInput *in = cars[car];
            if (in->hold == 0) {
                OneLapGhostInfo *rec = &logger_input_sectors(in)[i];
                records[i][car] = rec;
                sectorStartPos[i][car] = rec->start;
                lengthSum[car] += rec->length;
            }
        }
        f32 start;
        if (i != 0)
            start = org->getLoggerSectorPosition(i - 1);
        else
            start = 0.0f;
        start += wrap;
        if (start <= prevStart) {
            wrap += length;                 /* (this order: retail adds to start first) */
            start += length;
        }
        sectorStart[i] = start;
        prevStart = start;
    }
    position = 0.0f;
    scrollY = 0.0f;                         /* stored as the word $zero */
    length += wrap;
    zoom = 1.0f;
    speedHeight = 800.0f;
    speedMax = 800;
    setCursorToSector(0);
}

/* The length of sector i (the last one ends at the recorded distance).  No
 * callers. */
// FUNC 0x0016DB90 gt2k_menu::LoggerRaceGraph::sectorLength
f32 LoggerRaceGraph::sectorLength(int i)
{
    /* implicit: logger_get_length (the inline getSectorEnd) */
    return getSectorEnd(i) - getSectorStart(i);
}

// FUNC 0x0016DC00 gt2k_menu::LoggerRaceGraph::setCursorToSector
void LoggerRaceGraph::setCursorToSector(int i)
{
    position = sectorStart[i];
}

/* The values the graphs plot (passed by address to drawCurve /
 * drawFilledCurve): throttle and brake 0..1, gear / 7, the speed in km/h
 * (the sample's m/s x 40). */
// FUNC 0x0016DC18 logger_sample_throttle
f32 logger_sample_throttle(LoggerSample *s)
{
    return (f32)s->throttle / 255.0f;
}

// FUNC 0x0016DC40 logger_sample_brake
f32 logger_sample_brake(LoggerSample *s)
{
    return (f32)s->brake / 255.0f;
}

// FUNC 0x0016DC68 logger_sample_gear
f32 logger_sample_gear(LoggerSample *s)
{
    return (f32)s->gear / 7.0f;
}

// FUNC 0x0016DC98 logger_sample_speed
f32 logger_sample_speed(LoggerSample *s)
{
    return (f32)s->speed / 40.0f * 3.6f;
}

/* Caller drawSpeed.  The curve y0 + get(sample) * scale of car `car' over
 * the visible distance (x = the distance), as line strips of 64 points (a
 * new strip starts at the last point); with more than 1000 samples visible
 * only every stride-th one.  Both ends' distances are computed before the
 * tests; the samples indexed (loop.c's pointer starts as i * 8 + base, as
 * retail).  Left (97.2%): retail computes last - first into $v1 and copies
 * it to $a0 for the stride loop (a span variable, the expression repeated in
 * the loop test, while / for forms: none keeps the copy), and with it one
 * alignment nop and the order of two preheader insns. */
// FUNC 0x0016DCC8 gt2k_menu::LoggerRaceGraph::drawCurve
void LoggerRaceGraph::drawCurve(int car, LoggerSampleFn *get, const f32 *rgba, f32 y0, f32 scale)
{
    f32 pts[LOGGER_STRIP_MAX][2];           /* sp+0x00 */

    if (logger_graph_options()->showCar[car] == 0)
        return;
    if (cars[car]->hold != 0)
        return;
    f32 viewRight = right;
    f32 viewLeft = left;
    LoggerData *data = logger_get_data();
    f32 step = data->interval;
    int count = data->count[car];
    f32 perMetre = 1.0f / step;
    int first = (int)(viewLeft * perMetre) - 1;
    int last = (int)(viewRight * perMetre) + 1;
    if (first <= -1)
        first = 0;
    if (last >= count)
        last = count - 1;
    f32 x = (f32)first * step;
    f32 xl = (f32)last * step;
    if (viewRight < x)
        return;
    if (xl < viewLeft)
        return;
    int span = last - first;
    int stride = 1;
    if (span > LOGGER_MAX_SAMPLES) {
        do {
            stride++;
            step += data->interval;
        } while (span / stride > LOGGER_MAX_SAMPLES);
    }
    pgl_color4fv(rgba);
    int n = 0;
    int i = first;
    LoggerSample *samples = data->samples[car];
    if (i <= last) {
        f32 (*p)[2] = pts;
        do {
            n++;
            f32 y = get(&samples[i]) * scale + y0;
            (*p)[0] = x;
            (*p)[1] = y;
            p++;
            if (n == LOGGER_STRIP_MAX) {
                pgl_draw_line_strip_2d(&pts[0][0], LOGGER_STRIP_MAX);
                n = 1;
                pts[0][1] = y;
                p = &pts[1];
                pts[0][0] = x;
            }
            i += stride;
            x += step;
        } while (i <= last);
    }
    if (n >= 2)
        pgl_draw_line_strip_2d(&pts[0][0], n);
}

/* Caller drawPedals.  The same range as drawCurve, filled down to y0: a
 * strip of (x, y0 + v * scale) / (x, y0) vertex pairs, 64 vertices per
 * strip (the next one restarts with the last pair); the top vertices are
 * coloured with rgb scaled by v * 127 + 128, the bottom ones by 128.  The
 * colour is read after the stride loop (GCSE hoists the loads into both
 * arms, as retail), x is copied ($f28 -> $f20) after the first buffer, and
 * the refill of the buffers is a statement block (do { } while (0)): with
 * it loop.c no longer takes n for an induction variable (retail's mult by
 * 12 in the loop).  Left (97.2%): drawCurve's $v1 -> $a0 copy, and loop.c's
 * second pass hoists stride * 8 (the pointer increment) out of the loop
 * where retail keeps it inside (58 insns in the loop pass here, the move
 * is "not desirable" from 59), which spills it. */
// FUNC 0x0016DF28 gt2k_menu::LoggerRaceGraph::drawFilledCurve
void LoggerRaceGraph::drawFilledCurve(int car, LoggerSampleFn *get, const f32 *rgba, f32 y0, f32 scale)
{
    if (logger_graph_options()->showCar[car] == 0)
        return;
    if (cars[car]->hold != 0)
        return;
    f32 viewRight = right;
    f32 viewLeft = left;
    LoggerData *data = logger_get_data();
    f32 step = data->interval;
    int count = data->count[car];
    f32 perMetre = 1.0f / step;
    int first = (int)(viewLeft * perMetre) - 1;
    int last = (int)(viewRight * perMetre) + 1;
    if (first <= -1)
        first = 0;
    if (last >= count)
        last = count - 1;
    f32 startX = (f32)first * step;
    f32 xl = (f32)last * step;
    if (viewRight < startX)
        return;
    if (xl < viewLeft)
        return;
    int span = last - first;
    int stride = 1;
    if (span > LOGGER_MAX_SAMPLES) {
        do {
            stride++;
            step += data->interval;
        } while (span / stride > LOGGER_MAX_SAMPLES);
    }
    f32 r = rgba[0];
    f32 g = rgba[1];
    f32 b = rgba[2];
    f32 a = rgba[3];
    u32 baseColor = pack_rgba_floats_to_u32(r, g, b, a, 128.0f);
    int i = first;
    PrimVertex *verts = (PrimVertex *)gfx_alloc_tail(LOGGER_STRIP_VERTS_QWC);
    f32 x = startX;
    u32 *colors = (u32 *)gfx_alloc_tail(LOGGER_STRIP_COLS_QWC);
    int n = 0;
    LoggerSample *samples = data->samples[car];
    if (last >= i) {
        do {
            f32 v = get(&samples[i]);
            f32 y = y0 + v * scale;
            u32 topColor = pack_rgba_floats_to_u32(r, g, b, a, v * 127.0f + 128.0f);
            PrimVertex *vt = &verts[n];
            vt[0].x = x;
            vt[0].y = y;
            vt[0].z = 0.0f;
            vt[1].x = x;
            vt[1].y = y0;
            vt[1].z = 0.0f;
            colors[n] = topColor;
            colors[n + 1] = baseColor;
            n += 2;
            if (n == LOGGER_STRIP_MAX) {
                n = 2;
                pgl_draw_strip(verts, NULL, colors, NULL, LOGGER_STRIP_MAX);
                do {                        /* (a statement block in the original) */
                    verts = (PrimVertex *)gfx_alloc_tail(LOGGER_STRIP_VERTS_QWC);
                    colors = (u32 *)gfx_alloc_tail(LOGGER_STRIP_COLS_QWC);
                } while (0);
                verts[0].y = y;
                colors[0] = topColor;
                verts[0].x = x;
                verts[0].z = 0.0f;
                verts[1].x = x;
                verts[1].y = y0;
                verts[1].z = 0.0f;
                colors[1] = baseColor;
            }
            i += stride;
            x += step;
        } while (last >= i);
    }
    if (n >= 4)
        pgl_draw_strip(verts, NULL, colors, NULL, n);
}

/* Caller LoggerRaceGraph::virtual_12 (the pedal bands: maxSpeed 100, h 50;
 * the speed curves: speedMax + 25, speedHeight + 25).  The float y0 comes
 * first: virtual_12 sets $f12 before $a1 (the EE ABI hides the order
 * otherwise; virtual_12 is exact only so).  Vertical lines every
 * 1/64 mile but every 4th (those are drawDistanceScale's), grey 0.25, over
 * y0..y0 + h; then horizontal lines for v = 0, 25, ... up to maxSpeed at
 * y0 + h * v / maxSpeed * 1.60934 (v in mph on the km/h scale of the speed
 * curves; the pedal bands get the factor too, and the loop bound compares
 * the mph value with maxSpeed): white at 0, grey 0.5 every 100, else
 * 0.25.  Retail reloads the loops' constants every pass: GCSE inserts
 * maxSpeed < 0 (the second loop's entry test, mph = 0 propagated) and the
 * (f32)maxSpeed conversion right after each loop's NOTE_INSN_LOOP_BEG, and
 * loop.c then takes both loops for phony (-dL); a scaleMax local, a
 * `maxSpeed >= 0' test or goto loops all differ. */
// FUNC 0x0016E2A0 gt2k_menu::LoggerRaceGraph::drawSpeedGrid
void LoggerRaceGraph::drawSpeedGrid(f32 y0, int maxSpeed, f32 h)
{
    f32 y1 = y0 + h;
    f32 viewLeft = left;
    f32 viewRight = right;
    int last = (int)(viewRight / LOGGER_MILE_64TH) + 1;
    int first = (int)(viewLeft / LOGGER_MILE_64TH) - 1;
    pgl_color3f(0.25f, 0.25f, 0.25f);
    for (int i = first; i <= last; i++) {
        if ((i & 3) != 0) {
            f32 x = (f32)i * LOGGER_MILE_64TH;
            pgl_draw_line_3f(x, y0, 0.0f, x, y1, 0.0f);
        }
    }
    int mph = 0;
    if (mph <= maxSpeed) {
        do {
            f32 y = y0 + h * (f32)mph / (f32)maxSpeed * LOGGER_KMH_PER_MPH;
            f32 grey;
            if (mph % 100 == 0) {
                grey = 0.5f;
                if (mph == 0)
                    grey = 1.0f;
            } else {
                grey = 0.25f;
            }
            pgl_color3f(grey, grey, grey);
            mph += 25;
            pgl_draw_line_3f(viewLeft, y, 0.0f, viewRight, y, 0.0f);
        } while (mph <= maxSpeed);
    }
}

/* Caller LoggerRaceGraph::virtual_12.  A grey line from y 268 to 0 every
 * 1/16 mile (x truncated to an int); every 16th (each mile) the label
 * " %dml" (x / 1600: miles, 1600 m to the mile; the .sdata string
 * 0x00351BC8) in white at (x, y - 8) with the "sw_12*50" style. */
// FUNC 0x0016E468 gt2k_menu::LoggerRaceGraph::drawDistanceScale
void LoggerRaceGraph::drawDistanceScale(f32 y)
{
    int first = (int)(left / LOGGER_MILE_16TH) - 1;
    int last = (int)(right / LOGGER_MILE_16TH) + 1;
    g_002F1968.setStyle("sw_12*50");
    g_002F1968.setAlign(0);
    for (int i = first; i <= last; i++) {
        int x = (int)((f32)i * LOGGER_MILE_64TH * 4.0f);
        pgl_color3f(0.5f, 0.5f, 0.5f);
        f32 fx = (f32)x;
        pgl_draw_line_3f(fx, LOGGER_GRAPH_LINES, 0.0f, fx, 0.0f, 0.0f);
        if ((i & 0xF) == 0) {
            pgl_color3f(1.0f, 1.0f, 1.0f);
            g_002F1968.setPos3(fx, y - 8.0f, 0.0f);
            g_002F1968.print(" %dml", x / 1600);
        }
    }
}

namespace gt2k_menu {

/* Caller virtual_12.  A white line at the bands' common edge y; "0%" there
 * (below it with both cars, above it with car A alone, 8 below with car B
 * alone), "100%" near the far edge of each band shown (car A's above y,
 * car B's below), 16 pixels from the left of the view. */
// FUNC 0x0016E640 gt2k_menu::LoggerRaceGraph::drawPedalScale
void LoggerRaceGraph::drawPedalScale(f32 y, int showA, int showB)
{
    f32 viewLeft = left;
    f32 viewRight = right;
    f32 x = viewLeft + 16.0f / zoom;
    f32 zeroY;

    pgl_color3f(1.0f, 1.0f, 1.0f);
    pgl_draw_line_3f(viewLeft, y, 0.0f, viewRight, y, 0.0f);
    g_002F1968.setStyle("sw_12*50");
    g_002F1968.setAlign(FONT_ALIGN_LEFT);
    pgl_color3f(1.0f, 1.0f, 1.0f);

    zeroY = 0.0f;
    if (showA != 0)
        zeroY = showB != 0 ? y - 4.0f : y + 4.0f;
    else if (showB != 0)
        zeroY = y - 8.0f;
    g_002F1968.setPos3(x, zeroY, 0.0f);
    g_002F1968.print("%s", "0%");
    if (showA != 0) {
        g_002F1968.setPos3(x, (y + LOGGER_BAND_LINES) - 8.0f, 0.0f);
        g_002F1968.print("%s", "100%");
    }
    if (showB != 0) {
        g_002F1968.setPos3(x, (y - LOGGER_BAND_LINES) + 4.0f, 0.0f);
        g_002F1968.print("%s", "100%");
    }
}

/* Caller virtual_12.  "<n>mph" every 100 mph, at the height of n mph on the
 * km/h scale of the speed graph (n x 1.60934 km/h, as drawSpeedGrid's
 * lines).  The loop bound compares the mph value with speedMax (km/h): the
 * labels go on above the graph's top (quirk kept). */
// FUNC 0x0016E830 gt2k_menu::LoggerRaceGraph::drawSpeedScale
void LoggerRaceGraph::drawSpeedScale(f32 y)
{
    int mph = 0;
    f32 x = left + 16.0f / zoom;

    pgl_color3f(1.0f, 1.0f, 1.0f);
    g_002F1968.setStyle("sw_12*50");
    g_002F1968.setAlign(FONT_ALIGN_LEFT);
    for (; speedMax >= mph; mph += 100) {
        g_002F1968.setPos3(x, y + speedHeight * (f32)mph / (f32)speedMax * LOGGER_KMH_PER_MPH + 4.0f, 0.0f);
        g_002F1968.print("%dmph", mph);
    }
}

/* The cursor's mark: a filled triangle with its base at y0 from
 * x - halfWidth to x + halfWidth and its apex at (x, y1). */
// FUNC 0x0016E968 gt2k_menu::logger_draw_mark
void logger_draw_mark(f32 x, f32 halfWidth, f32 y0, f32 y1)
{
    PrimVertex *tri = (PrimVertex *)gfx_alloc_tail(3);

    tri[0].x = x + halfWidth;
    tri[0].y = y0;
    tri[0].z = 0.0f;                        /* the z words are stored as $zero */
    tri[1].x = x - halfWidth;
    tri[1].y = y0;
    tri[1].z = 0.0f;
    tri[2].x = x;
    tri[2].y = y1;
    tri[2].z = 0.0f;
    pgl_draw_strip(tri, NULL, NULL, NULL, 3);
}

/* The leader line of a speed label: from (*x, *y) 15 * dx across and
 * 30 * dy up (dx: a pixel in metres, dy: +-1 line), then 30 * dx across;
 * (*x, *y) becomes its end, where the label goes. */
// FUNC 0x0016E9F8 gt2k_menu::logger_draw_leader
void logger_draw_leader(f32 *x, f32 *y, f32 dx, f32 dy)
{
    f32 x0 = *x;
    f32 y0 = *y;
    f32 x1 = x0 + dx * 15.0f;
    f32 y1 = y0 + dy * 30.0f;

    pgl_draw_line_3f(x0, y0, 0.0f, x1, y1, 0.0f);
    *x = x1 + dx * 30.0f;
    *y = y1;
    pgl_draw_line_3f(x1, y1, 0.0f, *x, y1, 0.0f);
}

/* Caller virtual_12; y is the bottom of the speed graph (0 km/h), the texts
 * use it clamped at the buffer's bottom.  The sector starts (orange lines,
 * their numbers from 1) and the lap's end; with antialiasing: the cursor's
 * distance ("%.3tml": thousandths of a mile), the cars' speeds at the
 * cursor ("%.1tmph": tenths) at the end of leader lines from the curves -
 * the faster car's goes up, the other one's down unless it would leave the
 * buffer, car A's to the left (right-aligned text) when both cars are
 * shown -, then the cursor itself: a red band two pixels wide over the
 * whole height and its mark above the zero line.
 * Matching (91.2%, register allocation): retail keeps y in $f29 and spills
 * showB (sp+0x10); here y is spilled.  Retail compares labelY - 30 with a
 * 0.0 made on the spot and keeps the zero of setPos3 / dxA < 0 in $f26
 * (the 1.0's register); egcs here gives the three zeros one register,
 * which costs dxB $f21 and y its register.  Tried: declaration orders,
 * textY as a ternary / if-else / self-test, the band corners as
 * variables (retail computes them after the loop: $f30 / $f31). */
// FUNC 0x0016EAC8 gt2k_menu::LoggerRaceGraph::drawCursor
void LoggerRaceGraph::drawCursor(f32 y, int showA, int showB)
{
    f32 textY = y;
    f32 pixel = 1.0f / zoom;                /* one pixel, in metres */
    f32 center = (left + right) * 0.5f;     /* the cursor (position) */
    f32 upA, upB, dxA, dxB;
    f32 linesPerKmh, speedA, speedB;
    LoggerData *data;
    PrimVertex *band;
    int i;

    if (y < 0.0f)
        textY = 0.0f;
    g_002F1968.setStyle("sw_12");
    g_002F1968.setAlign(FONT_ALIGN_LEFT);
    pgl_color3f(0.9f, 0.5f, 0.1f);
    for (i = 0; i < sectorCount; i++) {
        f32 x = sectorStart[i];
        pgl_draw_line_3f(x, LOGGER_GRAPH_LINES, 0.0f, x, 0.0f, 0.0f);
        g_002F1968.setPos3(x + (pixel + pixel), textY + 3.0f, 0.0f);
        g_002F1968.print("%d", i + 1);
    }
    pgl_draw_line_3f(length, LOGGER_GRAPH_LINES, 0.0f, length, 0.0f, 0.0f);

    upA = -1.0f;                            /* the leaders go down ... */
    upB = -1.0f;                            /* (sp+0x18) */
    gfx_enable(PGL_CAP_ANTIALIAS);
    gfx_disable(PGL_CAP_BLEND);
    gfx_blend_func(LOGGER_BLEND_ALPHA, 1.0f);
    pgl_color3f(0.9f, 0.9f, 0.9f);
    g_002F1968.setPos3(center + pixel * 10.0f, textY + 3.0f, 0.0f);
    g_002F1968.print("%.3tml", (int)(center * 1000.0f * LOGGER_MILES_PER_METRE));

    data = logger_get_data();
    linesPerKmh = speedHeight / (f32)speedMax;
    speedA = data->speedAt(0, center) * LOGGER_KMH_PER_MPS;
    speedB = data->speedAt(1, center) * LOGGER_KMH_PER_MPS;
    dxB = 1.0f / zoom;
    dxA = dxB;
    if (showA != 0) {
        if (showB != 0) {
            /* ... but the faster car's goes up; car A's goes left */
            if (speedB < speedA)
                upA = 1.0f;
            else
                upB = 1.0f;
            dxA = -dxB;
        }
    }
    if (showA != 0) {
        f32 labelX = center;                /* sp+0x0 */
        f32 labelY = speedA * linesPerKmh + y;  /* sp+0x4, on the curve */
        if (labelY - 30.0f < 0.0f)
            upA = 1.0f;
        pgl_color4fv(g_002AEDC0[0]);
        logger_draw_leader(&labelX, &labelY, dxA, upA);
        if (dxA < 0.0f)
            g_002F1968.setAlign(FONT_ALIGN_RIGHT);
        g_002F1968.setPos3(labelX, labelY, 0.0f);
        g_002F1968.print("%.1tmph", (int)(speedA * LOGGER_MPH_PER_KMH * 10.0f));
        g_002F1968.setAlign(FONT_ALIGN_LEFT);
    }
    if (showB != 0) {
        f32 labelX = center;                /* sp+0x8 */
        f32 labelY = speedB * linesPerKmh + y;  /* sp+0xC */
        if (labelY - 30.0f < 0.0f)
            upB = 1.0f;
        pgl_color4fv(g_002AEDC0[1]);
        logger_draw_leader(&labelX, &labelY, dxB, upB);
        g_002F1968.setPos3(labelX, labelY, 0.0f);
        g_002F1968.print("%.1tmph", (int)(speedB * LOGGER_MPH_PER_KMH * 10.0f));
        g_002F1968.setAlign(FONT_ALIGN_LEFT);
    }

    pgl_color3f(1.0f, 0.0f, 0.0f);
    band = (PrimVertex *)gfx_alloc_tail(3); /* a strip of 4 */
    band[0].x = center - pixel;
    band[0].y = LOGGER_GRAPH_LINES;
    band[0].z = 0.0f;
    band[1].x = center - pixel;
    band[1].y = 0.0f;
    band[1].z = 0.0f;
    band[2].x = center + pixel;
    band[2].y = LOGGER_GRAPH_LINES;
    band[2].z = 0.0f;
    band[3].x = center + pixel;
    band[3].y = 0.0f;
    band[3].z = 0.0f;
    pgl_draw_strip(band, NULL, NULL, NULL, 4);
    logger_draw_mark(center, pixel * 8.0f, textY + 11.0f, textY + 1.0f);
    gfx_disable(PGL_CAP_ANTIALIAS);
}

/* Caller virtual_12.  "A" / "B" big, in the car's colour at 0.4 alpha,
 * centred 64 pixels from the left of the view, 25 lines above y (the middle
 * of the car's band). */
// FUNC 0x0016F028 gt2k_menu::LoggerRaceGraph::drawCarLabel
void LoggerRaceGraph::drawCarLabel(int car, f32 y)
{
    const f32 *rgba = g_002AEDC0[car];
    f32 x = left + 64.0f / zoom;

    g_002F1968.setStyle("sw_20*200,150");
    g_002F1968.setAlign(FONT_ALIGN_CENTER);
    g_002F1968.setPos3(x, y + 25.0f, 0.0f);
    g_002F1968.setColor(rgba[0], rgba[1], rgba[2], 0.4f);
    g_002F1968.print("%c", "AB"[car]);
    g_002F1968.setAlign(FONT_ALIGN_LEFT);
}

/* Caller virtual_12.  The throttle and brake bands of a car (as the options
 * enable them), half transparent, filled from the common edge y up (car A)
 * or down (car B). */
// FUNC 0x0016F120 gt2k_menu::LoggerRaceGraph::drawPedals
void LoggerRaceGraph::drawPedals(int car, f32 y)
{
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(LOGGER_BLEND_FIX, 0.5f);
    if (logger_graph_options()->accel != 0) {
        f32 h = car == 0 ? LOGGER_BAND_LINES : -LOGGER_BAND_LINES;     /* up / down */
        drawFilledCurve(car, logger_sample_throttle, g_002AED80[car], y, h);
    }
    if (logger_graph_options()->brake != 0) {
        f32 h = car == 0 ? LOGGER_BAND_LINES : -LOGGER_BAND_LINES;
        drawFilledCurve(car, logger_sample_brake, g_002AEDA0[car], y, h);
    }
    gfx_disable(PGL_CAP_BLEND);
}

/* Caller virtual_12.  The speed curve of a car (if the options enable it),
 * from y up, antialiased: drawn once into a sub-packet, then the packet is
 * sent five times, the drawing offset moved by (+1, +1), (-1, +1), (+1, -1),
 * (-1, -1) pixels between the copies (gfx_offset_xy adds: the copies land
 * at (0, 0), (1, 1), (0, 2), (1, 1), (0, 0)) and by (0, 0) after the last. */
// FUNC 0x0016F218 gt2k_menu::LoggerRaceGraph::drawSpeed
void LoggerRaceGraph::drawSpeed(int car, f32 y)
{
    if (logger_graph_options()->velocity != 0) {
        f32 linesPerKmh = speedHeight / (f32)speedMax;
        GsSubPacketHeader *curve;

        gfx_enable(PGL_CAP_ANTIALIAS);
        gfx_disable(PGL_CAP_BLEND);
        gfx_blend_func(LOGGER_BLEND_ALPHA, 1.0f);
        curve = gfx_begin_subpacket();
        drawCurve(car, logger_sample_speed, g_002AEDC0[car], y, linesPerKmh);
        gfx_close_subpacket();
        gfx_disable(PGL_CAP_ANTIALIAS);
        gfx_call_subpacket(curve);
        gfx_offset_xy(1.0f, 1.0f);
        gfx_call_subpacket(curve);
        gfx_offset_xy(-1.0f, 1.0f);
        gfx_call_subpacket(curve);
        gfx_offset_xy(1.0f, -1.0f);
        gfx_call_subpacket(curve);
        gfx_offset_xy(-1.0f, -1.0f);
        gfx_call_subpacket(curve);
        gfx_offset_xy(0.0f, 0.0f);
    }
}

/* Slot 12 (draw).  The window, then the graph: the view around the cursor
 * (left / right) and the sectors in it; a 640 x 268 GS buffer (PSMCT32,
 * its own VRAM; ~GSBuffer frees it, also when an exception passes) becomes
 * the draw buffer, cleared, with the projection x left .. right, y 0 .. 268
 * upwards; from the bottom (scrolled down by scrollY): the bands of the
 * cars shown (car B's, then car A's, with their labels and grids) if there
 * are throttle or brake curves to draw, then the speed graph (its grid, the
 * distance scale, the pedal scale and bands, the speed scale, the speed
 * curves, the cursor).  The GS state restored, the buffer is drawn as three
 * sprites (point sampled): as it is, then at 1/3 alpha a line up and at
 * 1/4 a line down.  (drawPedalScale, drawCursor and drawSpeedGrid take the
 * float first: retail sets $f12 before the ints.) */
// FUNC 0x0016F368 gt2k_menu::LoggerRaceGraph::virtual_12
void LoggerRaceGraph::virtual_12()
{
    int i;
    int showA, showB, shown, accel, brake, pedals;
    f32 bandY, top, speedY;                 /* speedY: the bottom of the speed graph */

    RawWindow::virtual_12();                /* 0x001D6350 */
    left = position - 320.0f / zoom;        /* half the buffer each side */
    right = left + LOGGER_GRAPH_W / zoom;
    for (i = 0; i < sectorCount; i++) {
        f32 start = getSectorStart(i);
        f32 end = getSectorEnd(i);          /* implicit: logger_get_length (inline getSectorEnd) */
        if (end < left || right < start)
            sectorVisible[i] = 0;
        else
            sectorVisible[i] = 1;
    }

    GSBuffer buf;                           /* sp+0x00 */
    buf.width = 640;
    buf.height = 268;
    buf.psm = 0;                            /* PSMCT32 */
    buf.setField(PGL_FIELD_FRAME);
    buf.allocVram();
    pgl_push_attrib(LOGGER_GRAPH_ATTRIBS);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_push_matrix();
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    pgl_push_matrix();
    buf.setDrawBuffer();
    gs_set_clear_color(0);
    gs_clear_buffers(PGL_CLEAR_COLOR);
    gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_ANTIALIAS);
    gfx_disable(PGL_CAP_BLEND);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_disable(PGL_CAP_CULL_FACE);
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    vu0_load_identity_regs();
    pgl_ortho(left, right, 0.0f, LOGGER_GRAPH_LINES, -100.0f, 100.0f);
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    vu0_load_identity_regs();

    showA = 0;
    if (cars[0]->hold == 0)
        showA = logger_graph_options()->showCar[0] != 0;
    showB = 0;
    if (cars[1]->hold == 0)
        showB = logger_graph_options()->showCar[1] != 0;
    shown = showA + showB;
    accel = logger_graph_options()->accel;
    brake = logger_graph_options()->brake;
    pedals = 0;
    if (shown != 0 && (accel != 0 || brake != 0))
        pedals = 1;

    bandY = 0.0f;                           /* the bands' common edge */
    top = -scrollY;                         /* the bottom of what comes next */
    if (pedals != 0) {
        top += LOGGER_BAND_MARGIN;
        if (showB != 0) {
            drawCarLabel(1, top);
            drawSpeedGrid(top, LOGGER_BAND_GRID_MAX, LOGGER_BAND_LINES);
            top += LOGGER_BAND_LINES;
        }
        bandY = top;
        if (showA != 0) {
            drawCarLabel(0, bandY);
            drawSpeedGrid(bandY, LOGGER_BAND_GRID_MAX, LOGGER_BAND_LINES);
            top = bandY + LOGGER_BAND_LINES;
        }
    }
    speedY = top + LOGGER_GRAPH_GAP;
    drawSpeedGrid(speedY, speedMax + LOGGER_SPEED_GRID_OVER, speedHeight + 25.0f);
    drawDistanceScale(speedY);
    if (pedals != 0) {
        drawPedalScale(bandY, showA, showB);
        if (showA != 0)
            drawPedals(0, bandY);
        if (showB != 0)
            drawPedals(1, bandY);
    }
    drawSpeedScale(speedY);
    if (showB != 0)
        drawSpeed(1, speedY);
    if (showA != 0)
        drawSpeed(0, speedY);
    drawCursor(speedY, showA, showB);

    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_pop_matrix();
    pgl_matrix_mode(PGL_MTX_MODELVIEW);
    pgl_pop_matrix();
    pgl_pop_attrib();
    buf.setTexture();
    gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_NEAREST);
    gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_NEAREST);
    pgl_color3f(1.0f, 1.0f, 1.0f);
    gs_draw_textured_sprite_strips_f(0.0f, 0.0f, LOGGER_GRAPH_W, LOGGER_GRAPH_LINES,
                                     0.0f, 0.0f, (f32)buf.width, (f32)buf.height);
    gfx_enable(PGL_CAP_BLEND);
    gfx_blend_func(LOGGER_BLEND_FIX, 0.33333334f);
    gs_draw_textured_sprite_strips_f(0.0f, 0.0f, LOGGER_GRAPH_W, 267.0f,
                                     0.0f, 1.0f, (f32)buf.width, (f32)buf.height);
    gfx_blend_func(LOGGER_BLEND_FIX, 0.25f);
    gs_draw_textured_sprite_strips_f(0.0f, 1.0f, LOGGER_GRAPH_W, LOGGER_GRAPH_LINES,
                                     0.0f, 0.0f, (f32)buf.width, (f32)buf.height - 1.0f);
    gfx_disable(PGL_CAP_BLEND);
    /* ~GSBuffer (0x00244428, flags 2) */
}

/* Caller LoggerGraphRoot::virtual_16.  Moves the cursor to pos; when it
 * enters another sector the sector list's cursor follows (sound 3).  A pos
 * outside every sector moves only the cursor. */
// FUNC 0x0016F8D0 gt2k_menu::LoggerRaceGraph::setPosition
void LoggerRaceGraph::setPosition(f32 pos)
{
    int current = sectors->getIndex(NULL);
    int i;

    for (i = 0; i < sectorCount; i++) {
        /* implicit: logger_get_length (inline getSectorEnd) */
        if (getSectorStart(i) <= pos && pos < getSectorEnd(i))
            break;
    }
    if (i != sectorCount && i != current) {
        menu_play_se(MENU_SE_CURSOR);
        sectors->virtual_32(i);
    }
    position = pos;
}

/* Caller LoggerGraphRoot::virtual_16 (the scroll limit).  The lines
 * virtual_12 draws: the bands (4 + 50 per car shown) if any, 10, the speed
 * graph. */
// FUNC 0x0016F9C8 gt2k_menu::LoggerRaceGraph::getContentHeight
f32 LoggerRaceGraph::getContentHeight()
{
    int showA, showB, shown, accel, brake, pedals;
    f32 lines;

    showA = 0;
    if (cars[0]->hold == 0)
        showA = logger_graph_options()->showCar[0] != 0;
    showB = 0;
    if (cars[1]->hold == 0)
        showB = logger_graph_options()->showCar[1] != 0;
    shown = showA + showB;
    accel = logger_graph_options()->accel;
    brake = logger_graph_options()->brake;
    pedals = 0;
    if (shown != 0 && (accel != 0 || brake != 0))
        pedals = 1;
    lines = 0.0f;
    if (pedals != 0)
        lines = (f32)shown * LOGGER_BAND_LINES + LOGGER_BAND_MARGIN;
    lines += LOGGER_GRAPH_GAP;
    return lines + speedHeight;
}

/* ======================================================================== */
/* LoggerGraphRoot                                                          */
/* ======================================================================== */

} /* namespace gt2k_menu */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351C1C = "LoggerGraphRoot";                 /* 0x00351C1C */
gt2k_menu::RawWindowClass g_00351C20 =                      /* 0x00351C20 */
    { "LoggerGraphRoot", gt2k_menu::LoggerGraphRoot::create };
gt2k_menu::RawWindowRegister g_00351C28(&g_00351C20);       /* 0x00351C28 */

namespace gt2k_menu {

// FUNC 0x0016FAA8 gt2k_menu::LoggerGraphRoot::virtual_8
const char *LoggerGraphRoot::virtual_8()
{
    return g_00351C1C;                      /* "LoggerGraphRoot" */
}

/* No callers in the image: the layout factory. */
// FUNC 0x0016FAB0 gt2k_menu::LoggerGraphRoot::factory
RawWindow *LoggerGraphRoot::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
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

/* The registry's create function (g_00351C20.create). */
// FUNC 0x0016FAF0 gt2k_menu::LoggerGraphRoot::create
RawWindow *LoggerGraphRoot::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new LoggerGraphRoot(*args);  /* Object::operator new(0x230); Object::operator delete if
                                               the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x0016FB68 gt2k_menu::LoggerGraphRoot::canCreate
int LoggerGraphRoot::canCreate(const RawWindowArgs *args)
{
    return 1;
}

/* The logged race (organization, race, its memory pool, the inputs of the
 * two cars) and the option panel, loaded from the layout
 * "LoggerGraphOperation" (its "GraphOperation" list does not keep the
 * sub-list index and wraps around).  The children do not take the focus
 * with it; scale 0. */
// FUNC 0x0016FB70 gt2k_menu::LoggerGraphRoot::LoggerGraphRoot
LoggerGraphRoot::LoggerGraphRoot(const RawWindowArgs &args)
    : RawWindow(args)
{
    RawList *list;

    org = logger_get_org();
    race = org->race;
    pool = race->virtual_4();
    cars[0] = race->virtual_26(LOGGER_INPUT_CAR_A);
    cars[1] = race->virtual_26(LOGGER_INPUT_CAR_B);
    operation = loadChild("LoggerGraphOperation", &args, NULL);
    list = (RawList *)operation->findChild("GraphOperation");
    list->keepSubListIndex = 0;
    list->moveLoop = 1;
    graphOperation = list;
    childrenFocusChange = 0;
    scale = 0;
}

// FUNC 0x0016FC88 gt2k_menu::LoggerGraphRoot::~LoggerGraphRoot
LoggerGraphRoot::~LoggerGraphRoot()
{
    /* RawWindow's destructor; the deleting form calls Object::operator delete. */
    /* implicit: ~RawWindow, operator delete (the base / member destructors,
     * flags & 1) */
}

} /* namespace gt2k_menu */

/* The two cars' panels of the graph mode (LoggerGraphRoot::setup). */
const char *g_00351C30[2] = { "GraphTrackA", "GraphTrackB" };    /* 0x00351C30 */

namespace gt2k_menu {

/* From LoggerRoot::setup.  The texts of both cars in "GraphTrackA" /
 * "GraphTrackB" (g_00351C30), their icon and title pictures (car B's box
 * moved to x 328), "graph_point" (the cursor on the course map). */
// FUNC 0x0016FCE0 gt2k_menu::LoggerGraphRoot::setup
void LoggerGraphRoot::setup(LoggerSector *s)
{
    int car;

    sectors = s;
    for (car = 0; car < 2; car++) {
        RawWindow *track = findChild(g_00351C30[car]);
        speedInfo[car] = track->findChildByFullName("r_speed_info");
        totalInfo[car] = track->findChildByFullName("r_total_info");
        sectorInfo[car] = track->findChildByFullName("r_sector_info");
        averageInfo[car] = track->findChildByFullName("r_average_info");
        if (car == 0) {
            track->findChildByFullName("r_icon")->changeImage("icon_trackA", 0, 0);
            track->findChildByFullName("r_title")->changeImage("title_trackA", 0, 0);
        } else {
            track->findChildByFullName("r_icon")->changeImage("icon_trackB", 0, 0);
            track->findChildByFullName("r_title")->changeImage("title_trackB", 0, 0);
            track->setLeft(328.0f);
        }
    }
    graphPoint = findChildByFullName("graph_point");
}

/* The parent LoggerRoot's sector list. */
// FUNC 0x0016FEA8 gt2k_menu::LoggerGraphRoot::getRootSectors
LoggerSector *LoggerGraphRoot::getRootSectors()
{
    /* parentWindow(): dynamic_cast<RawWindow *>(parent), inline; the result
     * is not checked */
    /* implicit: __dynamic_cast (the inline parentWindow) */
    return ((LoggerRoot *)parentWindow())->sectors;
}

/* From LoggerRoot::start and virtual_17.  Closes the option panel: the menu
 * control goes back to the sector list and the graph, the panel is hidden
 * and loses it. */
// FUNC 0x0016FF00 gt2k_menu::LoggerGraphRoot::closeOperation
void LoggerGraphRoot::closeOperation()
{
    getRootSectors()->virtual_14(menuControl);
    findChild("Graph")->virtual_14(menuControl);
    operation->virtual_23(0);
    operation->virtual_14(NULL);
}

/* From LoggerRoot::virtual_17 (Start in the graph mode).  The sector list
 * loses the menu control but keeps its focus look; the panel is centred on
 * the screen, shown, takes the menu control, its cursor on the first row;
 * the rows show the options: "GraphAccell" / "GraphBrake" / "GraphVelocity"
 * item 0 on, 1 off; "GraphDisplay" item 0 car A, 1 car B, 2 both (none
 * shown gives -1). */
// FUNC 0x0016FFA0 gt2k_menu::LoggerGraphRoot::openOperation
void LoggerGraphRoot::openOperation()
{
    LoggerGraphOptions *opt;
    int display = 0;
    /* An exact form, not the original: retail keeps 1 in $s3 from the
     * prologue for the four if-converted values below (movn / movz), so the
     * original read them from a variable holding 1; a literal 1 gives
     * store-flags (sltu / sltiu) instead.  The values are the same. */
    int on = 1;

    getRootSectors()->virtual_14(NULL);
    getRootSectors()->virtual_20(1);
    operation->setLeft((LOGGER_GRAPH_W - operation->w) * 0.5f);
    operation->virtual_23(1);
    operation->virtual_14(menuControl);
    graphOperation->virtual_31(0);
    opt = logger_graph_options();
    if (opt->showCar[0] != 0)
        display = on;
    if (opt->showCar[1] != 0)
        display += 2;
    ((RawList *)graphOperation->findChild("GraphAccell"))->virtual_31(opt->accel != 0 ? 0 : on);
    ((RawList *)graphOperation->findChild("GraphBrake"))->virtual_31(opt->brake != 0 ? 0 : on);
    ((RawList *)graphOperation->findChild("GraphVelocity"))->virtual_31(opt->velocity != 0 ? 0 : on);
    ((RawList *)graphOperation->findChild("GraphDisplay"))->virtual_31(display - 1);
}

/* Slot 9 (once a frame).  The graph's speed scale (g_002AEDE0[scale]); per
 * car whose input is not held: the speed at the cursor and the average
 * speed over the selected sector (in the speed unit: "%3d%s"), the lap time
 * and the sector's time; then "graph_point" on the course map: car A's
 * position at the cursor (LoggerCourseMap::toMap, from the map's centre)
 * scaled from the map's pixels to its window, centred on that point.  The
 * speed uses the "Graph" child's cursor, the rest this->graph (the same
 * window).
 * Matching (83.0%): inside the loop retail keeps the "%3d%s" address in
 * $fp for both sprintf calls and g_002F59D8's in $s5 for three of its four
 * uses (the fourth rebuilt from the shared lui), so the "Graph" child goes
 * to the stack (sp+0x10C); egcs here unifies g_002F59D8's address (loop.c
 * then hoists it) but not the string's (PRE merges only the two lui
 * halves, combine folds each addiu into $a1), and keeps the child in $fp.
 * Tried: the conversion nested in sprintf, speed / km/h locals, inline
 * getSectorStart / getSectorEnd helpers (fresh `this'); the reference
 * compiler 991111-01 gives the same code. */
// FUNC 0x00170170 gt2k_menu::LoggerGraphRoot::virtual_9
void LoggerGraphRoot::virtual_9()
{
    char text[0x100];                       /* sp+0x00 */
    LoggerData *data = logger_get_data();
    LoggerRaceGraph *g = (LoggerRaceGraph *)findChild("Graph");
    int sector, car;

    g->speedHeight = (f32)g_002AEDE0[scale];
    sector = sectors->getIndex(NULL);
    for (car = 0; car < 2; car++) {
        if (logger_car_input(car)->hold != 0)
            continue;
        {
            RawWindow *info = speedInfo[car];
            int speed = (int)g_002F59D8.convSpeed(data->speedAt(car, g->position));
            sprintf(text, "%3d%s", speed, g_002F59D8.speedName());
            info->setText(text);
        }
        {
            RawWindow *info = totalInfo[car];
            format_time(text, sectors->totalTime[car]);
            info->setText(text);
        }
        {
            RawWindow *info = sectorInfo[car];
            format_time(text, logger_sector_time(car, sector));
            info->setText(text);
        }
        {
            RawWindow *info = averageInfo[car];
            LoggerData *samples = logger_get_data();
            f32 start = graph->getSectorStart(sector);
            /* implicit: logger_get_length (inline getSectorEnd) */
            f32 end = graph->getSectorEnd(sector);
            int speed = (int)g_002F59D8.convSpeed(samples->avgSpeed(car, start, end));
            sprintf(text, "%3d%s", speed, g_002F59D8.speedName());
            info->setText(text);
        }
    }

    {
        LoggerData *samples = logger_get_data();
        LoggerCourseMap *map = logger_course_map();
        f32 pos = graph->position;
        f32 mapX, mapY;                     /* sp+0x100, sp+0x104 */
        RawWindow *mapWindow;

        mapX = samples->xBefore(0, pos);
        mapY = samples->zBefore(0, pos);
        map->toMap(&mapX, &mapY, mapX, mapY);
        /* parentWindow(): dynamic_cast<RawWindow *>(parent), inline; the
         * result is not checked */
        mapWindow = graphPoint->parentWindow();
        /* implicit: __dynamic_cast (the inline parentWindow) */
        mapX += (f32)map->width * 0.5f;
        mapY += (f32)map->height * 0.5f;
        mapX *= mapWindow->w / (f32)map->width;
        mapY *= mapWindow->h / (f32)map->height;
        mapX -= graphPoint->w * 0.5f;
        mapY -= graphPoint->h * 0.5f;
        graphPoint->setLeft(mapX);
        graphPoint->setTop(mapY);
    }
}

/* Slot 16 (input).  The graph's unk_5DC and lrCancel are cleared.  While
 * the option panel is shown the options are taken from its rows, 0.  Else,
 * if this window is shown and the pad has sticks (PS type 5, 7, 8; the
 * values through the curve g_003416F8): the left stick's y zooms (the step
 * grows with log10(zoom); 0.4 .. 10), the right stick's x moves the cursor
 * along the lap (8 pixels a frame at full tilt; past an end it wraps to the
 * other), its y scrolls the graph (4 lines a frame, 0 .. the content above
 * the buffer + 16); a zoom move sets lrCancel.  Then RawWindow's. */
// FUNC 0x001705E0 gt2k_menu::LoggerGraphRoot::virtual_16
int LoggerGraphRoot::virtual_16(MenuControl *ctl)
{
    LoggerRaceGraph *g = (LoggerRaceGraph *)findChild("Graph");

    g->lrCancel = 0;
    g->unk_5DC = 0;
    if (operation->isShown() != 0) {
        LoggerGraphOptions *opt = logger_graph_options();
        int display;

        opt->accel = ((RawList *)graphOperation->findChild("GraphAccell"))->getIndex(NULL) == 0;
        opt->brake = ((RawList *)graphOperation->findChild("GraphBrake"))->getIndex(NULL) == 0;
        opt->velocity = ((RawList *)graphOperation->findChild("GraphVelocity"))->getIndex(NULL) == 0;
        display = ((RawList *)graphOperation->findChild("GraphDisplay"))->getIndex(NULL) + 1;
        opt->showCar[0] = display & 1;
        opt->showCar[1] = (display >> 1) & 1;
        return 0;
    }
    if (isShown() != 0) {
        switch (ctl->rawPSType()) {
        case PS_PAD_ANALOG:
        case PS_PAD_DUALSHOCK:
        case PS_PAD_DUALSHOCK2: {
            f32 zoomIn = logger_stick(ctl, LOGGER_AXIS_ZOOM);
            f32 move = logger_stick(ctl, LOGGER_AXIS_MOVE);
            f32 scroll = logger_stick(ctl, LOGGER_AXIS_SCROLL);
            f32 zoom = g->zoom;
            f32 v, maxScroll;               /* v: the new position, then the new scrollY */

            zoom = zoom + zoomIn * 0.1f * (logf(zoom) / LOGGER_LN_10 + 1.2f);
            if (zoom < LOGGER_ZOOM_MIN)
                zoom = LOGGER_ZOOM_MIN;
            else if (LOGGER_ZOOM_MAX < zoom)
                zoom = LOGGER_ZOOM_MAX;
            v = g->position;
            v += move / zoom * 8.0f;
            g->zoom = zoom;
            if (v < 0.0f)
                v = g->length;
            else if (g->length < v)
                v = 0.0f;
            g->setPosition(v);

            maxScroll = (g->getContentHeight() - LOGGER_GRAPH_LINES) + 16.0f;
            if (maxScroll < 0.0f)
                maxScroll = 0.0f;
            v = g->scrollY;
            v -= scroll * 4.0f;
            if (v < 0.0f)
                v = 0.0f;
            else if (maxScroll < v)
                v = maxScroll;
            g->scrollY = v;
            if (zoomIn != 0.0f)
                g->lrCancel = 1;
            break;
        }
        }
    }
    return RawWindow::virtual_16(ctl);      /* 0x001D65A0 */
}

/* Slot 17 (events).  Nothing while the zoom stick moves (lrCancel: 0).
 * Decide is swallowed; cancel closes the option panel if it has the menu
 * control (sound 1); up / down from this window step the speed scale
 * (sound 3, or 4 at the end).  The rest RawWindow's. */
// FUNC 0x00170968 gt2k_menu::LoggerGraphRoot::virtual_17
int LoggerGraphRoot::virtual_17(RawWindow *sender, int event, int arg)
{
    if (((LoggerRaceGraph *)findChild("Graph"))->lrCancel != 0)
        return 0;
    switch (event) {
    case RAW_EVENT_DECIDE:
        return 0;
    case RAW_EVENT_CANCEL:
        if (operation->menuControl != NULL) {
            closeOperation();
            menu_play_se(MENU_SE_CANCEL);
        }
        return 0;
    case RAW_EVENT_UP:
        if (sender != this)
            break;
        if (scale >= LOGGER_SCALE_LAST) {   /* (read through sender, == this) */
            menu_play_se(MENU_SE_REFUSED);
        } else {
            scale++;
            menu_play_se(MENU_SE_CURSOR);
        }
        return 0;
    case RAW_EVENT_DOWN:
        if (sender != this)
            break;
        if (scale == 0) {
            menu_play_se(MENU_SE_REFUSED);
        } else {
            scale--;
            menu_play_se(MENU_SE_CURSOR);
        }
        return 0;
    }
    return RawWindow::virtual_17(sender, event, arg); /* 0x001D6BD0 */
}

/* ======================================================================== */
/* LoggerRoot                                                               */
/* ======================================================================== */

} /* namespace gt2k_menu */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351C40 = "LoggerRoot";                      /* 0x00351C40 */
gt2k_menu::RawWindowClass g_00351C48 =                      /* 0x00351C48 */
    { "LoggerRoot", gt2k_menu::LoggerRoot::create };
gt2k_menu::RawWindowRegister g_00351C50(&g_00351C48);       /* 0x00351C50 */

namespace gt2k_menu {

// FUNC 0x00170A90 gt2k_menu::LoggerRoot::virtual_8
const char *LoggerRoot::virtual_8()
{
    return g_00351C40;                      /* "LoggerRoot" */
}

/* No callers in the image: the layout factory. */
// FUNC 0x00170A98 gt2k_menu::LoggerRoot::factory
RawWindow *LoggerRoot::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
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

/* The registry's create function (g_00351C48.create). */
// FUNC 0x00170AD8 gt2k_menu::LoggerRoot::create
RawWindow *LoggerRoot::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new LoggerRoot(*args);       /* Object::operator new(0x1F8); Object::operator delete if
                                               the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x00170B50 gt2k_menu::LoggerRoot::canCreate
int LoggerRoot::canCreate(const RawWindowArgs *args)
{
    return 1;
}

/* Registers itself as the logger's root window (T055); the owner is the
 * LoggerControl. */
// FUNC 0x00170B58 gt2k_menu::LoggerRoot::LoggerRoot
LoggerRoot::LoggerRoot(const RawWindowArgs &args)
    : RawWindow(args)
{
    logger_set_root(this);
    control = static_cast<LoggerControl *>(args.owner);
}

// FUNC 0x00170BD8 gt2k_menu::LoggerRoot::~LoggerRoot
LoggerRoot::~LoggerRoot()
{
    /* implicit: ~RawWindow, operator delete (the base / member destructors,
     * flags & 1) */
    logger_set_root(NULL);
    /* then RawWindow's destructor; the deleting form calls Object::operator
     * delete */
}

/* From LoggerReplay's constructor.  Wires the children together: the
 * sector list (scrolled inside "SectorBox", wrapping, the cursor centred;
 * at 0, 0), the graph mode (its texts; its map and graph get the sector
 * list, the graph becomes g_00351AFC and is set up), the replay mode (the
 * graph's sector bounds; set up) and its buttons; then the window of the
 * current mode is shown. */
// FUNC 0x00170C38 gt2k_menu::LoggerRoot::setup
void LoggerRoot::setup()
{
    RawWindow *box = findChild("SectorBox");
    LoggerSector *list = (LoggerSector *)findChild("Sectors");
    LoggerRaceGraph *g;
    LoggerReplayRoot *replay;
    LoggerReplayControl *buttons;

    list->scroll = box;
    sectors = list;
    list->scrollCenter = 1;
    list->moveLoop = 1;
    list->setLeft(0.0f);
    sectors->setTop(0.0f);

    graphRoot = (LoggerGraphRoot *)findChild("GraphRoot");
    graphRoot->setup(sectors);
    ((LoggerMapCourse *)graphRoot->findChildByFullName("LoggerMapCourse_GraphMode"))->sectors = sectors;
    g = (LoggerRaceGraph *)graphRoot->findChild("Graph");
    graphRoot->graph = g;
    g_00351AFC = g;
    g->setup(sectors);
    sectors->graph = g;

    replay = (LoggerReplayRoot *)findChild("ReplayRoot");
    replay->graph = g;
    replayRoot = replay;
    replay->setup(sectors);

    buttons = (LoggerReplayControl *)findChild("ReplayCtrl");
    buttons->sectors = sectors;
    sectors->replayControl = buttons;
    showMode();
}

/* Shows the window of the current mode (replay or graph), hides the
 * other. */
// FUNC 0x00170D70 gt2k_menu::LoggerRoot::showMode
void LoggerRoot::showMode()
{
    replayRoot->virtual_23(0);
    graphRoot->virtual_23(0);
    switch (control->state) {
    case LOGGER_STATE_REPLAY:
        replayRoot->virtual_23(1);
        break;
    case LOGGER_STATE_GRAPH:
        graphRoot->virtual_23(1);
        break;
    }
}

/* From LoggerReplay's constructor: unlocked, the menu control to the sector
 * list (the replay stopped), the option panel closed. */
// FUNC 0x00170E30 gt2k_menu::LoggerRoot::start
void LoggerRoot::start()
{
    locked = 0;
    replayRoot->leave();
    graphRoot->closeOperation();
}

/* Slot 16 (input).  1 when not shown.  Start sends LOGGER_EVENT_GRAPH_OPTIONS
 * (8), L1 / R1 the sector list's left / right events (its cursor moves), L2
 * / R2 events 10 / 11 (the mode); the result of the event is returned.
 * Else RawWindow's. */
// FUNC 0x00170E68 gt2k_menu::LoggerRoot::virtual_16
int LoggerRoot::virtual_16(MenuControl *ctl)
{
    if (isShown() == 0)
        return 1;
    if (ctl->getButtonsOn() & CONTROL_START)
        return virtual_19(LOGGER_EVENT_GRAPH_OPTIONS, 0);
    if (ctl->l1 != 0)
        return sectors->virtual_19(RAW_EVENT_LEFT, 0);
    if (ctl->r1 != 0)
        return sectors->virtual_19(RAW_EVENT_RIGHT, 0);
    if (ctl->l2 != 0)
        return virtual_19(LOGGER_EVENT_MODE_L2, 0);
    if (ctl->r2 != 0)
        return virtual_19(LOGGER_EVENT_MODE_R2, 0);
    return RawWindow::virtual_16(ctl);      /* 0x001D65A0 */
}

/* L2 / R2 (events 10 / 11): replay <-> graph (to the graph the replay
 * stops and the sector list takes the menu control from "ReplayCtrl"),
 * sound 10 if the mode changed, then the window of the mode.  Inline in the
 * original (as an inline member): retail's switch tests 10 and 11 as two
 * case labels whose identical code jump2 cross-jumped into one block. */
static inline void logger_root_switch_mode(LoggerRoot *root)
{
    int changed = 1;

    switch (root->control->state) {
    case LOGGER_STATE_REPLAY:
        root->control->state = LOGGER_STATE_GRAPH;
        logger_set_play(0);
        root->sectors->virtual_14(root->menuControl);
        root->replayRoot->findChild("ReplayCtrl")->virtual_14(NULL);
        break;
    case LOGGER_STATE_GRAPH:
        root->control->state = LOGGER_STATE_REPLAY;
        break;
    default:
        changed = 0;
        break;
    }
    if (changed != 0)
        menu_play_se(MENU_SE_LIST_MOVE);
    root->showMode();
}

/* Slot 17 (events), unless locked.  8: the graph's option panel (in the
 * graph mode; the sound plays in either mode); 9: the replay plays (in the
 * replay mode); 10 / 11: replay <-> graph (to the graph: the replay stops,
 * the sector list takes the menu control from "ReplayCtrl"), sound 10 if
 * the mode changed.  The rest RawWindow's. */
// FUNC 0x00170FA8 gt2k_menu::LoggerRoot::virtual_17
int LoggerRoot::virtual_17(RawWindow *sender, int event, int arg)
{
    if (locked == 0) {
        switch (event) {
        case LOGGER_EVENT_GRAPH_OPTIONS:
            menu_play_se(MENU_SE_LIST_MOVE);
            if (control->state == LOGGER_STATE_GRAPH)
                graphRoot->openOperation();
            return 0;
        case LOGGER_EVENT_REPLAY_PLAY:
            if (control->state == LOGGER_STATE_REPLAY) {
                menu_play_se(MENU_SE_LIST_MOVE);
                replayRoot->enterPlaying();
            }
            return 0;
        case LOGGER_EVENT_MODE_L2:
            logger_root_switch_mode(this);
            return 0;
        case LOGGER_EVENT_MODE_R2:
            logger_root_switch_mode(this);
            /* implicit: logger_set_play, findChild, showMode (the inlined
             * logger_root_switch_mode) */
            return 0;
        }
    }
    return RawWindow::virtual_17(sender, event, arg); /* 0x001D6BD0 */
}

/* ======================================================================== */
/* LoggerReplay                                                             */
/* ======================================================================== */

} /* namespace gt2k_menu */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351C60 = "LoggerReplay";                    /* 0x00351C60 */
gt2k_menu::RawWindowClass g_00351C68 =                      /* 0x00351C68 */
    { "LoggerReplay", gt2k_menu::LoggerReplay::create };
gt2k_menu::RawWindowRegister g_00351C70(&g_00351C68);       /* 0x00351C70 */

namespace gt2k_menu {

// FUNC 0x00171110 gt2k_menu::LoggerReplay::virtual_8
const char *LoggerReplay::virtual_8()
{
    return g_00351C60;                      /* "LoggerReplay" */
}

/* T055's item factory "replay_logger" (g_0029DE48). */
// FUNC 0x00171118 gt2k_menu::LoggerReplay::factory
RawWindow *LoggerReplay::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
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

/* The registry's create function (g_00351C68.create). */
// FUNC 0x00171158 gt2k_menu::LoggerReplay::create
RawWindow *LoggerReplay::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new LoggerReplay(*args);     /* Object::operator new(0x1E8); Object::operator delete if
                                               the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x001711D0 gt2k_menu::LoggerReplay::canCreate
int LoggerReplay::canCreate(const RawWindowArgs *args)
{
    return 1;
}

/* No graph yet (g_00351AFC); loads the "LoggerRoot" layout as its child and
 * sets it up, takes the owner's control1 (the LoggerControl's, its +0: the
 * pointer is passed unchanged) as its menu control (RawWindow::virtual_14
 * called directly) and starts the root. */
// FUNC 0x001711D8 gt2k_menu::LoggerReplay::LoggerReplay
LoggerReplay::LoggerReplay(const RawWindowArgs &args)
    : RawWindow(args)
{
    g_00351AFC = NULL;
    root = (LoggerRoot *)loadChild("LoggerRoot", &args, NULL);
    root->setup();
    RawWindow::virtual_14(&args.owner->control1);   /* 0x001D6678 */
    root->start();
}

// FUNC 0x00171280 gt2k_menu::LoggerReplay::~LoggerReplay
LoggerReplay::~LoggerReplay()
{
    /* RawWindow's destructor; the deleting form calls Object::operator delete. */
    /* implicit: ~RawWindow, operator delete (the base / member destructors,
     * flags & 1) */
}

/* Slot 17: cancel from (inside) the sector list leaves the page for the
 * logger's "top" page: state 0, sound 1, no replay window to draw
 * (render.window: LoggerControl::setWindow(NULL), inline), the page manager
 * exits (exitRequest, setFreeze(0)); 0.  Every other event: 1. */
// FUNC 0x001712D8 gt2k_menu::LoggerReplay::virtual_17
int LoggerReplay::virtual_17(RawWindow *sender, int event, int arg)
{
    LoggerControl *logger = logger_control_of(menuControl);

    if (event == RAW_EVENT_CANCEL && sender->isInClass("LoggerSector") != 0) {
        logger->state = LOGGER_STATE_TOP;
        menu_play_se(MENU_SE_CANCEL);
        logger->render.window = NULL;
        pages->exitRequest = 1;
        pages->setFreeze(0);
        return 0;
    }
    return 1;
}

} /* namespace gt2k_menu */

/* ======================================================================== */
/* Static destructor / constructor                                          */
/* ======================================================================== */

/* Compiler-generated: egcs's static destructor and initialiser of the ten
 * registrars above (named after the unit's first global, g_002AED68),
 * emitted after the last function, before the inline members and typeinfo
 * functions below. */
// FUNC 0x00171358 T087_static_dtor (_GLOBAL_$D$g_002AED68)
//   (.dtors entry 0x0028F1F8) ~RawWindowRegister (0x001D8238, flags 2) of
//   the registrars, in reverse order.
/* implicit: ~RawWindowRegister */
// FUNC 0x001713E8 T087_static_init (_GLOBAL_$I$g_002AED68)
//   (.ctors entry 0x0028F0B8) RawWindowRegister(&class) (0x001D8220) of the
//   ten, in definition order.
/* implicit: RawWindowRegister */

/* ======================================================================== */
/* Inline members emitted out of line, typeinfo functions                   */
/* ======================================================================== */
/* egcs writes them at the end of the unit (it defines the ten classes' key
 * functions): first LoggerRaceGraph's five inline members defined out of
 * the class (above, before LoggerRaceGraph::virtual_8: 0x001714A0 ..
 * 0x00171540), then per class, the last defined first, its inline members
 * (in the class, game/debug/T087_logger_windows.h: the last declared first) and
 * its typeinfo function. */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base RawWindow). */
// FUNC 0x00171550 __tfQ29gt2k_menu12LoggerReplay
extern "C" void *__tfQ29gt2k_menu12LoggerReplay(void)
{
    if (__tiQ29gt2k_menu12LoggerReplay[0] == 0) {
        __tfQ29gt2k_menu9RawWindow();
        __rtti_si(__tiQ29gt2k_menu12LoggerReplay, "Q29gt2k_menu12LoggerReplay",
                  (const type_info *)__tiQ29gt2k_menu9RawWindow);
    }
    return __tiQ29gt2k_menu12LoggerReplay;
}
#endif

// FUNC 0x001715A0 gt2k_menu::LoggerRoot::getGraphRoot (getGraphRoot__Q29gt2k_menu10LoggerRoot)
// FUNC 0x001715A8 gt2k_menu::LoggerRoot::getSectors (getSectors__Q29gt2k_menu10LoggerRoot)
/* (the Lupo 2 name; the field is the replay mode's window) */
// FUNC 0x001715B0 gt2k_menu::LoggerRoot::getSectorList (getSectorList__Q29gt2k_menu10LoggerRoot)
// FUNC 0x001715B8 gt2k_menu::LoggerRoot::isLocked (isLocked__Q29gt2k_menu10LoggerRoot)
/* inline in game/debug/T087_logger_windows.h; egcs emits these copies */

#ifndef BOOT_OLD_GCC
// FUNC 0x001715C0 __tfQ29gt2k_menu10LoggerRoot
extern "C" void *__tfQ29gt2k_menu10LoggerRoot(void)
{
    if (__tiQ29gt2k_menu10LoggerRoot[0] == 0) {
        __tfQ29gt2k_menu9RawWindow();
        __rtti_si(__tiQ29gt2k_menu10LoggerRoot, "Q29gt2k_menu10LoggerRoot",
                  (const type_info *)__tiQ29gt2k_menu9RawWindow);
    }
    return __tiQ29gt2k_menu10LoggerRoot;
}
#endif

// FUNC 0x00171610 gt2k_menu::LoggerGraphRoot::setGraph (setGraph__Q29gt2k_menu15LoggerGraphRootPQ29gt2k_menu15LoggerRaceGraph)
// FUNC 0x00171618 gt2k_menu::LoggerGraphRoot::getCar (getCar__Q29gt2k_menu15LoggerGraphRooti)
// FUNC 0x00171628 gt2k_menu::LoggerGraphRoot::getRace (getRace__Q29gt2k_menu15LoggerGraphRoot)
/* (the Lupo 2 name; the field is the race's memory pool) */
// FUNC 0x00171630 gt2k_menu::LoggerGraphRoot::getOrganization (getOrganization__Q29gt2k_menu15LoggerGraphRoot)
/* (the Lupo 2 name; the field is the race) */
// FUNC 0x00171638 gt2k_menu::LoggerGraphRoot::getRaceOrganization (getRaceOrganization__Q29gt2k_menu15LoggerGraphRoot)
/* inline in game/debug/T087_logger_windows.h; egcs emits these copies */

#ifndef BOOT_OLD_GCC
// FUNC 0x00171640 __tfQ29gt2k_menu15LoggerGraphRoot
extern "C" void *__tfQ29gt2k_menu15LoggerGraphRoot(void)
{
    if (__tiQ29gt2k_menu15LoggerGraphRoot[0] == 0) {
        __tfQ29gt2k_menu9RawWindow();
        __rtti_si(__tiQ29gt2k_menu15LoggerGraphRoot, "Q29gt2k_menu15LoggerGraphRoot",
                  (const type_info *)__tiQ29gt2k_menu9RawWindow);
    }
    return __tiQ29gt2k_menu15LoggerGraphRoot;
}
#endif

// FUNC 0x00171690 gt2k_menu::LoggerRaceGraph::setLRCancel (setLRCancel__Q29gt2k_menu15LoggerRaceGraphb)
/* (a word store of the bool) */
// FUNC 0x00171698 gt2k_menu::LoggerRaceGraph::getLRCancel (getLRCancel__Q29gt2k_menu15LoggerRaceGraph)
// FUNC 0x001716A0 gt2k_menu::LoggerRaceGraph::F_001716A0 (F_001716A0__Q29gt2k_menu15LoggerRaceGraphi)
// FUNC 0x001716A8 gt2k_menu::LoggerRaceGraph::F_001716A8 (F_001716A8__Q29gt2k_menu15LoggerRaceGraph)
// FUNC 0x001716B0 gt2k_menu::LoggerRaceGraph::getSpeedHeight (getSpeedHeight__Q29gt2k_menu15LoggerRaceGraph)
// FUNC 0x001716B8 gt2k_menu::LoggerRaceGraph::setSpeedHeight (setSpeedHeight__Q29gt2k_menu15LoggerRaceGraphf)
// FUNC 0x001716C0 gt2k_menu::LoggerRaceGraph::getSpeedMax (getSpeedMax__Q29gt2k_menu15LoggerRaceGraph)
// FUNC 0x001716C8 gt2k_menu::LoggerRaceGraph::setSpeedMax (setSpeedMax__Q29gt2k_menu15LoggerRaceGraphi)
// FUNC 0x001716D0 gt2k_menu::LoggerRaceGraph::setScrollY (setScrollY__Q29gt2k_menu15LoggerRaceGraphf)
// FUNC 0x001716D8 gt2k_menu::LoggerRaceGraph::getPositionX (getPositionX__Q29gt2k_menu15LoggerRaceGraph)
/* (the Lupo 2 name; reads scrollY, +0x528) */
// FUNC 0x001716E0 gt2k_menu::LoggerRaceGraph::getZoom (getZoom__Q29gt2k_menu15LoggerRaceGraph)
/* (the Lupo 2 name; reads position, +0x524) */
// FUNC 0x001716E8 gt2k_menu::LoggerRaceGraph::getLength (getLength__Q29gt2k_menu15LoggerRaceGraph)
// FUNC 0x001716F0 gt2k_menu::LoggerRaceGraph::setZoom (setZoom__Q29gt2k_menu15LoggerRaceGraphf)
// FUNC 0x001716F8 gt2k_menu::LoggerRaceGraph::getMaxPositionX (getMaxPositionX__Q29gt2k_menu15LoggerRaceGraph)
/* (the Lupo 2 name; reads zoom, +0x520) */
/* inline in game/debug/T087_logger_windows.h; egcs emits these copies */

#ifndef BOOT_OLD_GCC
// FUNC 0x00171700 __tfQ29gt2k_menu15LoggerRaceGraph
extern "C" void *__tfQ29gt2k_menu15LoggerRaceGraph(void)
{
    if (__tiQ29gt2k_menu15LoggerRaceGraph[0] == 0) {
        __tfQ29gt2k_menu9RawWindow();
        __rtti_si(__tiQ29gt2k_menu15LoggerRaceGraph, "Q29gt2k_menu15LoggerRaceGraph",
                  (const type_info *)__tiQ29gt2k_menu9RawWindow);
    }
    return __tiQ29gt2k_menu15LoggerRaceGraph;
}

// FUNC 0x00171750 __tfQ29gt2k_menu14LoggerGraphMap
extern "C" void *__tfQ29gt2k_menu14LoggerGraphMap(void)
{
    if (__tiQ29gt2k_menu14LoggerGraphMap[0] == 0) {
        __tfQ29gt2k_menu9RawWindow();
        __rtti_si(__tiQ29gt2k_menu14LoggerGraphMap, "Q29gt2k_menu14LoggerGraphMap",
                  (const type_info *)__tiQ29gt2k_menu9RawWindow);
    }
    return __tiQ29gt2k_menu14LoggerGraphMap;
}
#endif

// FUNC 0x001717A0 gt2k_menu::LoggerReplayRoot::setGraph (setGraph__Q29gt2k_menu16LoggerReplayRootPQ29gt2k_menu15LoggerRaceGraph)
/* inline in game/debug/T087_logger_windows.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x001717A8 __tfQ29gt2k_menu16LoggerReplayRoot
extern "C" void *__tfQ29gt2k_menu16LoggerReplayRoot(void)
{
    if (__tiQ29gt2k_menu16LoggerReplayRoot[0] == 0) {
        __tfQ29gt2k_menu9RawWindow();
        __rtti_si(__tiQ29gt2k_menu16LoggerReplayRoot, "Q29gt2k_menu16LoggerReplayRoot",
                  (const type_info *)__tiQ29gt2k_menu9RawWindow);
    }
    return __tiQ29gt2k_menu16LoggerReplayRoot;
}
#endif

// FUNC 0x001717F8 gt2k_menu::LoggerReplayControl::setSectors (setSectors__Q29gt2k_menu19LoggerReplayControlPQ29gt2k_menu12LoggerSector)
/* inline in game/debug/T087_logger_windows.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC
/* (__si_type_info, base RawList) */
// FUNC 0x00171800 __tfQ29gt2k_menu19LoggerReplayControl
extern "C" void *__tfQ29gt2k_menu19LoggerReplayControl(void)
{
    if (__tiQ29gt2k_menu19LoggerReplayControl[0] == 0) {
        __tfQ29gt2k_menu7RawList();
        __rtti_si(__tiQ29gt2k_menu19LoggerReplayControl, "Q29gt2k_menu19LoggerReplayControl",
                  (const type_info *)__tiQ29gt2k_menu7RawList);
    }
    return __tiQ29gt2k_menu19LoggerReplayControl;
}

// FUNC 0x00171850 __tfQ29gt2k_menu16LoggerRaceReplay
extern "C" void *__tfQ29gt2k_menu16LoggerRaceReplay(void)
{
    if (__tiQ29gt2k_menu16LoggerRaceReplay[0] == 0) {
        __tfQ29gt2k_menu9RawWindow();
        __rtti_si(__tiQ29gt2k_menu16LoggerRaceReplay, "Q29gt2k_menu16LoggerRaceReplay",
                  (const type_info *)__tiQ29gt2k_menu9RawWindow);
    }
    return __tiQ29gt2k_menu16LoggerRaceReplay;
}
#endif

// FUNC 0x001718A0 gt2k_menu::LoggerMapCourse::setSectors (setSectors__Q29gt2k_menu15LoggerMapCoursePQ29gt2k_menu12LoggerSector)
/* inline in game/debug/T087_logger_windows.h; egcs emits this copy */

#ifndef BOOT_OLD_GCC
// FUNC 0x001718A8 __tfQ29gt2k_menu15LoggerMapCourse
extern "C" void *__tfQ29gt2k_menu15LoggerMapCourse(void)
{
    if (__tiQ29gt2k_menu15LoggerMapCourse[0] == 0) {
        __tfQ29gt2k_menu9RawWindow();
        __rtti_si(__tiQ29gt2k_menu15LoggerMapCourse, "Q29gt2k_menu15LoggerMapCourse",
                  (const type_info *)__tiQ29gt2k_menu9RawWindow);
    }
    return __tiQ29gt2k_menu15LoggerMapCourse;
}
#endif

// FUNC 0x001718F8 gt2k_menu::LoggerSector::getTotalTime (getTotalTime__Q29gt2k_menu12LoggerSectori)
// FUNC 0x00171908 gt2k_menu::LoggerSector::setGraph (setGraph__Q29gt2k_menu12LoggerSectorPQ29gt2k_menu15LoggerRaceGraph)
// FUNC 0x00171910 gt2k_menu::LoggerSector::setReplayControl (setReplayControl__Q29gt2k_menu12LoggerSectorPQ29gt2k_menu19LoggerReplayControl)
/* inline in game/debug/T087_logger_windows.h; egcs emits these copies */

#ifndef BOOT_OLD_GCC
/* (__si_type_info, base RawList) */
// FUNC 0x00171918 __tfQ29gt2k_menu12LoggerSector
extern "C" void *__tfQ29gt2k_menu12LoggerSector(void)
{
    if (__tiQ29gt2k_menu12LoggerSector[0] == 0) {
        __tfQ29gt2k_menu7RawList();
        __rtti_si(__tiQ29gt2k_menu12LoggerSector, "Q29gt2k_menu12LoggerSector",
                  (const type_info *)__tiQ29gt2k_menu7RawList);
    }
    return __tiQ29gt2k_menu12LoggerSector;
}
#endif
