/* T087_logger_windows.h - the windows of the logger's replay / graph pages:
 * gt2k_menu::LoggerSector, LoggerMapCourse, LoggerRaceReplay,
 * LoggerReplayControl, LoggerReplayRoot, LoggerGraphMap, LoggerRaceGraph,
 * LoggerGraphRoot, LoggerRoot, LoggerReplay (translation unit
 * T087, 0x0016C260..0x00171968: src/game/debug/T087_logger_windows.cpp; units
 * u038 and u039 had it split at 0x0016E640).
 *
 * The "replay_logger" page of the logger (T055's item factory table
 * g_0029DE48) holds a LoggerReplay; its LoggerRoot has the sector list
 * (LoggerSector "Sectors", one "icon_sector<n>" item per sector both cars
 * recorded) and two modes chosen by LoggerControl::state: the replay (1,
 * LoggerReplayRoot "ReplayRoot": the race view LoggerRaceReplay, the buttons
 * LoggerReplayControl "ReplayCtrl", per car the speed / total / sector /
 * average texts of "ReplayTrackA/B") and the graph (2, LoggerGraphRoot
 * "GraphRoot": LoggerRaceGraph "Graph", the course map
 * "LoggerMapCourse_GraphMode" with "graph_point", the option panel
 * "LoggerGraphOperation").  L2 / R2 (events 10 / 11) switch the mode, L1 /
 * R1 (events 5 / 6 to the sector list) the sector, button 0x10 (event 8)
 * opens the graph options; cancel on the sector list goes back to the
 * logger's "top" page.
 *
 * The logged race is logger_get_org() (T055).  Its race gives the cars'
 * inputs (RaceBase::virtual_26: 0x100 the player's car = car 0 / "A", 1 the
 * other one = car 1 / "B"; the first pass's CarDynamics is RaceInput, see
 * T279_meter_scale.h) and the sector records (RaceMemPool bufs[car].sectors,
 * OneLapGhostInfo, T313).  LoggerRaceGraph draws the throttle / brake bands and
 * the speed curves over the lap distance into a 640 x 268 GS buffer.
 *
 * Per class the RawWindow registry boilerplate (as T272's): slot 8 returns
 * the class name, the 8-word layout factory packs its arguments into a
 * RawWindowArgs and calls create (operator new + the constructor if
 * canCreate(args)), canCreate returns 1 (every class takes the arguments,
 * unused, as T085's: the call keeps $a0, so create copies args to $s1
 * before saving $ra).  The accessors 0x001714A0..0x00171910
 * are inline members, emitted out of line at the end of T087 (it defines
 * the classes' key functions; no callers but getSectorStart / getSectorEnd):
 * in the classes, but LoggerRaceGraph's first five, which T087 defines
 * (out of the class) after their first caller.  Virtual slots keep the
 * RawWindow / RawList names (virtual_N, T264 / T263). */
#ifndef GAME_T087_LOGGER_WINDOWS_H
#define GAME_T087_LOGGER_WINDOWS_H

#include "game/game.h"
#include "engine/pgl/G001_flare.h"                /* PrimVertex (GraphVertex) */
#include "game/menu/framework/T264_raw_window.h"           /* base gt2k_menu::RawWindow */
#include "game/menu/framework/T263_raw_list.h"             /* base gt2k_menu::RawList */

class GSBuffer;                             /* T514 (references only) */
class RaceOrganization;                     /* T125 (pointers only) */
class RaceBase;                             /* T129 */
class RacePS2Base;                          /* T130 */
class RaceMemPool;                          /* T174 */
class RaceEntryCar;                         /* T121 */
class RaceInput;                            /* T413: what RaceBase::virtual_26 returns */
class OneLapGhostInfo;                         /* T313: one sector of a car's lap record */
struct LoggerSample;                        /* T055 */
class LoggerCourseMap;                      /* T088 */

namespace gt2k_menu {
struct RawWindowArgs;                       /* T264 (references / pointers only) */
struct RawWindowClass;                      /* T265 (extern objects only) */
class RawWindowRegister;                    /* T265 (extern objects only) */
struct MenuLayoutItem;                      /* T229 */
class PageLayout;                           /* T229 */
class MenuControl;                          /* T214 */
class LoggerControl;                        /* T055 */
class LoggerSector;
class LoggerReplayControl;
class LoggerReplayRoot;
class LoggerRaceGraph;
class LoggerGraphRoot;
class LoggerRoot;
}

/* u038's / u109's name of G001's PrimVertex (the filled curves, T494's
 * quads). */
typedef PrimVertex GraphVertex;

/* The graph options (T087 .data 0x002AED68, all 1 as built): the option
 * panel "LoggerGraphOperation" ("GraphAccell", "GraphBrake", "GraphVelocity":
 * item 0 on, 1 off; "GraphDisplay": item 0 car A, 1 car B, 2 both) reads and
 * writes them.  Not polymorphic, 0x18 bytes (the throttle colours follow at
 * 0x002AED80). */
struct LoggerGraphOptions {
    int accel;                              /* 0x00 the throttle bands */
    int brake;                              /* 0x04 the brake bands */
    int unk_08;                             /* 0x08 (? not read) */
    int velocity;                           /* 0x0C the speed curves */
    int showCar[2];                         /* 0x10 car A (the player's) / car B is drawn */
};
EGCS_SIZE(LoggerGraphOptions, 0x18);

/* A value the graphs plot, from one sample (passed by address to
 * LoggerRaceGraph::drawCurve / drawFilledCurve). */
typedef f32 LoggerSampleFn(LoggerSample *s);

namespace gt2k_menu {

/* vtable 0x00324910 (33 slots: [0], [8], [16], [17], [28] its own, [21]
 * RawListItem's, the rest RawList's).  typeinfo
 * __tfQ29gt2k_menu12LoggerSector 0x00171918 (si, base RawList;
 * __si_type_info 0x004B1CA0).  Size 0x258 (operator new in create).  The
 * sector list "Sectors". */
class LoggerSector : public RawList {
public:
    /* Horizontal (vertical 0), spacing 6; one "RawListItem"
     * "icon_sector<n>" (n from 1) per sector both cars recorded
     * (logger_sector_count); totalTime[car] = the time of the car's last
     * sector (0 for a car not shown). */
    LoggerSector(const RawWindowArgs &args);    /* 0x0016C570 */
    virtual ~LoggerSector();                /* [0] +0x0C 0x0016C710 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x0016C4A8 g_00351B00 "LoggerSector" */
    virtual int  virtual_16(MenuControl *ctl);  /* [16] +0x8C 0x0016C7C8 RawWindow's, 0 while the graph's
                                               unk_5DC is set */
    /* Events 1 and 3 give the input to the replay control when it is shown
     * (sound 3; 0); the rest RawList's. */
    virtual int  virtual_17(RawWindow *sender, int event, int arg); /* [17] +0x94 0x0016C7F8 */
    /* The cursor moved to item: the replay seeks to that sector
     * (logger_request_seek), the graph's cursor goes to it; then RawList's. */
    virtual int  virtual_28(RawListItem *item); /* [28] +0xEC 0x0016C768 */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x0016C4B0 */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x0016C4F0 */
    static int        canCreate(const RawWindowArgs *args);    /* 0x0016C568 1 (args unused) */
    /* Inline (T087 writes the copies, last declared first, with the class's
     * typeinfo function: the frame info's order). */
    void setReplayControl(LoggerReplayControl *c) { replayControl = c; }   /* 0x00171910 */
    void setGraph(LoggerRaceGraph *g) { graph = g; }                     /* 0x00171908 */
    int  getTotalTime(int car) { return totalTime[car]; }                /* 0x001718F8 */

    LoggerReplayControl *replayControl;     /* 0x248 "ReplayCtrl" (set by LoggerRoot::setup) */
    LoggerRaceGraph     *graph;             /* 0x24C "Graph" (set by LoggerRoot::setup) */
    int                  totalTime[2];      /* 0x250 per car: ms at the end of its last sector */
};
EGCS_SIZE(gt2k_menu::LoggerSector, 0x258);

/* vtable 0x00324848 (24 slots: [0], [8], [12] its own).  typeinfo
 * __tfQ29gt2k_menu15LoggerMapCourse 0x001718A8 (si, base RawWindow;
 * __si_type_info 0x004B1CC0).  Size 0x1E8 (operator new in create).  The
 * course map ("LoggerMapCourse_GraphMode"). */
class LoggerMapCourse : public RawWindow {
public:
    LoggerMapCourse(const RawWindowArgs &args); /* 0x0016C980 sectors is not set (LoggerRoot does) */
    virtual ~LoggerMapCourse();             /* [0] +0x0C 0x0016C9B8 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x0016C8B8 g_00351B18 "LoggerMapCourse" */
    virtual void virtual_12();              /* [12] +0x6C 0x0016CA10 the map in the window's rectangle
                                               (LoggerCourseMap::draw, the selected sector) */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x0016C8C0 */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x0016C900 */
    static int        canCreate(const RawWindowArgs *args);    /* 0x0016C978 1 (args unused) */
    void setSectors(LoggerSector *s) { sectors = s; }  /* 0x001718A0 inline (T087 writes the copy) */

    LoggerSector *sectors;                  /* 0x1E4 */
};
EGCS_SIZE(gt2k_menu::LoggerMapCourse, 0x1E8);

/* vtable 0x00324780 (24 slots: [0], [8] its own).  typeinfo
 * __tfQ29gt2k_menu16LoggerRaceReplay 0x00171850 (si, base RawWindow;
 * __si_type_info 0x004B1CE0).  Size 0x1EC (operator new in create).  The
 * race view of the replay page. */
class LoggerRaceReplay : public RawWindow {
public:
    /* Becomes the window the LoggerRender draws (args.owner is the
     * LoggerControl: render.window = this); org / race of the logged race. */
    LoggerRaceReplay(const RawWindowArgs &args);    /* 0x0016CB70 */
    virtual ~LoggerRaceReplay();            /* [0] +0x0C 0x0016CBF8 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x0016CAA8 g_00351B30 "LoggerRaceReplay" */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x0016CAB0 */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x0016CAF0 */
    static int        canCreate(const RawWindowArgs *args);    /* 0x0016CB68 1 (args unused) */
    /* From LoggerRender::render (T055) before the menu: when shown and the
     * race renders, clears buf and draws the race view (virtual_47) into the
     * rectangle (0, 12, 640, 224) of a copy of buf, the mirror view set to
     * (260, 10, 120, 32) x 1.3 (RacePS2Base::setMirrorRect(320, 26, 120,
     * 32, 1.3), inline); then the race's virtual_45 on buf. */
    void renderRace(GSBuffer &buf);         /* 0x0016CC50 */

    RaceOrganization *org;                  /* 0x1E4 logger_get_org() */
    RacePS2Base      *race;                 /* 0x1E8 org->race */
};
EGCS_SIZE(gt2k_menu::LoggerRaceReplay, 0x1EC);

/* vtable 0x00324670 (33 slots: [0], [8], [17] its own, the rest RawList's).
 * typeinfo __tfQ29gt2k_menu19LoggerReplayControl 0x00171800 (si, base
 * RawList; __si_type_info 0x004B1C70).  Size 0x24C (operator new in
 * create).  The replay buttons ("ReplayCtrl"). */
class LoggerReplayControl : public RawList {
public:
    LoggerReplayControl(const RawWindowArgs &args); /* 0x0016CE58 horizontal (vertical 0) */
    virtual ~LoggerReplayControl();         /* [0] +0x0C 0x0016CE90 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x0016CD90 g_00351B48 "LoggerReplayControl" */
    /* Event 1 (sound 10): "ReplayStart" plays at speed 0, "ReplaySlow" at
     * speed 1, "ReplayStop" stops, "ReplayCamera" logger_request_step; other
     * buttons go to the list.  Event 2 stops the replay, then as event 4:
     * unless the replay plays, the input goes back to the sector list. */
    virtual int  virtual_17(RawWindow *sender, int event, int arg); /* [17] +0x94 0x0016CEE8 */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x0016CD98 */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x0016CDD8 */
    static int        canCreate(const RawWindowArgs *args);    /* 0x0016CE50 1 (args unused) */
    void setSectors(LoggerSector *s) { sectors = s; }  /* 0x001717F8 inline (T087 writes the copy) */

    LoggerSector *sectors;                  /* 0x248 (set by LoggerRoot::setup) */
};
EGCS_SIZE(gt2k_menu::LoggerReplayControl, 0x24C);

/* vtable 0x003245A8 (24 slots: [0], [8], [9], [16], [17] its own).
 * typeinfo __tfQ29gt2k_menu16LoggerReplayRoot 0x001717A8 (si, base
 * RawWindow; __si_type_info 0x004B1CF0).  Size 0x20C (operator new in
 * create).  The replay mode ("ReplayRoot"). */
class LoggerReplayRoot : public RawWindow {
public:
    LoggerReplayRoot(const RawWindowArgs &args);    /* 0x0016D140 childrenFocusChange 0 */
    virtual ~LoggerReplayRoot();            /* [0] +0x0C 0x0016D178 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x0016D078 g_00351B60 "LoggerReplayRoot" */
    /* Per car shown: the current speed, the total time, the selected
     * sector's time and its average speed (the speed unit, "%3d%s"). */
    virtual void virtual_9();               /* [9] +0x54 0x0016D4A8 */
    virtual int  virtual_16(MenuControl *ctl);  /* [16] +0x8C 0x0016D6A0 RawWindow's */
    virtual int  virtual_17(RawWindow *sender, int event, int arg); /* [17] +0x94 0x0016D6C0 RawWindow's */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x0016D080 */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x0016D0C0 */
    static int        canCreate(const RawWindowArgs *args);    /* 0x0016D138 1 (args unused) */
    /* From LoggerRoot::setup: the texts of "ReplayTrackA" / "ReplayTrackB"
     * (g_00351B78), their icon / title pictures; panel B moves to x 328. */
    void setup(LoggerSector *s);            /* 0x0016D1D0 */
    LoggerSector *getSectors();             /* 0x0016D388 */
    void leave();                           /* 0x0016D390 the input back to the sector list; the replay stops */
    void enterPlaying();                    /* 0x0016D3D8 the sector list loses the input (its virtual_20(1));
                                               the replay plays */
    void enterPaused();                     /* 0x0016D440 the same, the replay stopped (no callers) */
    void setGraph(LoggerRaceGraph *g) { graph = g; }   /* 0x001717A0 inline (T087 writes the copy) */

    LoggerSector    *sectors;               /* 0x1E4 */
    RawWindow       *speedInfo[2];          /* 0x1E8 "r_speed_info" of ReplayTrackA / B */
    RawWindow       *totalInfo[2];          /* 0x1F0 "r_total_info" */
    RawWindow       *sectorInfo[2];         /* 0x1F8 "r_sector_info" */
    RawWindow       *averageInfo[2];        /* 0x200 "r_average_info" */
    LoggerRaceGraph *graph;                 /* 0x208 (set by LoggerRoot::setup: the sector bounds) */
};
EGCS_SIZE(gt2k_menu::LoggerReplayRoot, 0x20C);

/* vtable 0x003244E0 (24 slots: [0], [8] its own).  typeinfo
 * __tfQ29gt2k_menu14LoggerGraphMap 0x00171750 (si, base RawWindow;
 * __si_type_info 0x004B1C90).  Size 0x1E4 (operator new in create): no data
 * of its own. */
class LoggerGraphMap : public RawWindow {
public:
    LoggerGraphMap(const RawWindowArgs &args);  /* 0x0016D7A8 */
    virtual ~LoggerGraphMap();              /* [0] +0x0C 0x0016D7E0 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x0016D6E0 g_00351B98 "LoggerGraphMap" */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x0016D6E8 */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x0016D728 */
    static int        canCreate(const RawWindowArgs *args);    /* 0x0016D7A0 1 (args unused) */
};
EGCS_SIZE(gt2k_menu::LoggerGraphMap, 0x1E4);

/* vtable 0x00324418 (24 slots: [0], [8], [12] its own).  typeinfo
 * __tfQ29gt2k_menu15LoggerRaceGraph 0x00171700 (si, base RawWindow;
 * __si_type_info 0x004B1D00).  Size 0x5E4 (operator new in create).  The
 * graph ("Graph" of "GraphRoot"): x is the lap distance in metres (the view
 * left .. right is 640 / zoom wide around position), y the lines of the
 * 268-line buffer, upwards (pgl_ortho bottom 0, top 268; logger_windows_internal.h), the content
 * scrolled down by scrollY.  From the bottom: the throttle / brake bands of
 * the shown cars (50 lines each, from 4: car B's, filled down from the
 * common edge, then car A's, filled up), 10 lines, then the speed graph
 * (km/h upwards, speedHeight lines for speedMax).  The accessor names
 * getPositionX / getZoom / getMaxPositionX are index.txt's Lupo 2 matches;
 * they do not fit the fields they read (scrollY, position, zoom). */
class LoggerRaceGraph : public RawWindow {
public:
    LoggerRaceGraph(const RawWindowArgs &args); /* 0x0016D900 unk_5DC, lrCancel 0 */
    virtual ~LoggerRaceGraph();             /* [0] +0x0C 0x0016D940 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x0016D838 g_00351BB0 "LoggerRaceGraph" */
    /* The window (RawWindow::virtual_12), then the graph: the view, the
     * visible sectors, the bands and curves into a 640 x 268 GS buffer
     * (orthographic: x left .. right, y 0 .. 268 upwards), drawn as three
     * sprites (the 2nd / 3rd a line up / down at 1/3 / 1/4 alpha). */
    virtual void virtual_12();              /* [12] +0x6C 0x0016F368 */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x0016D840 */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x0016D880 */
    static int        canCreate(const RawWindowArgs *args);    /* 0x0016D8F8 1 (args unused) */
    /* From LoggerRoot::setup: the cars of the parent LoggerGraphRoot (not
     * checked), the sector records, the sector starts (the course's
     * boundaries, RaceOrganization::getLoggerSectorPosition, each made larger than the
     * previous by adding the lap length; length gets those wraps); zoom 1,
     * speedMax 800, speedHeight 800, the cursor on sector 0. */
    void setup(LoggerSector *s);            /* 0x0016D998 */
    f32  sectorLength(int i);               /* 0x0016DB90 (no callers) */
    void setCursorToSector(int i);          /* 0x0016DC00 position = sectorStart[i] */
    /* The curve y0 + get(sample) * scale of car `car' over the view, as line
     * strips of 64 points (every stride-th sample beyond 1000). */
    void drawCurve(int car, LoggerSampleFn *get, const f32 *rgba, f32 y0, f32 scale);       /* 0x0016DCC8 */
    /* The same filled down to y0 (strips, the top colour scaled by the value). */
    void drawFilledCurve(int car, LoggerSampleFn *get, const f32 *rgba, f32 y0, f32 scale); /* 0x0016DF28 */
    /* A grid over y0 .. y0 + h: vertical lines every 1/64 mile (but every
     * 4th), lines every 25 up to maxSpeed (100 for the pedal bands) at
     * y0 + h * v / maxSpeed * 1.60934 (v in mph on the km/h scale; the
     * pedal bands get the factor too). */
    void drawSpeedGrid(f32 y0, int maxSpeed, f32 h);        /* 0x0016E2A0 */
    void drawDistanceScale(f32 y);          /* 0x0016E468 a line every 1/16 mile, " %dml" every mile */
    /* drawSpeedGrid / drawPedalScale / drawCursor take the float first:
     * virtual_12 sets $f12 before $a1 / $a2 (the EE ABI hides the order
     * otherwise). */
    void drawPedalScale(f32 y, int showA, int showB);       /* 0x0016E640 "0%" / "100%" of the bands */
    void drawSpeedScale(f32 y);             /* 0x0016E830 "%dmph" every 100 */
    /* The sector lines and numbers, the cursor's distance and the cars'
     * speeds at it on leader lines, the red cursor band and its mark. */
    void drawCursor(f32 y, int showA, int showB);           /* 0x0016EAC8 */
    void drawCarLabel(int car, f32 y);      /* 0x0016F028 "A" / "B" in the car's colour */
    void drawPedals(int car, f32 y);        /* 0x0016F120 the throttle / brake bands (as enabled) */
    void drawSpeed(int car, f32 y);         /* 0x0016F218 the speed curve, five times (a sub-packet; the
                                               drawing offset moved a pixel between the copies) */
    /* The cursor to pos; the sector list follows into another sector
     * (sound 3, virtual_32). */
    void setPosition(f32 pos);              /* 0x0016F8D0 */
    f32  getContentHeight();                /* 0x0016F9C8 the bands (if any), 10, speedHeight */
    /* Inline members defined out of the class, in T087 after
     * LoggerReplayRoot::virtual_9 (which calls getSectorStart / getSectorEnd;
     * the later users expand them): egcs writes these copies first of the
     * end-of-unit group (0x001714A0..0x00171550). */
    f32  F_001714A0();                      /* 0x001714A0 3 / zoom */
    f32  F_001714C0();                      /* 0x001714C0 right + zoom * 5 */
    f32  F_001714E0();                      /* 0x001714E0 left - zoom * 5 */
    f32  getSectorEnd(int i);               /* 0x00171500 the next start, logger_get_length() for the last */
    f32  getSectorStart(int i);             /* 0x00171540 */
    /* Inline (T087 writes the copies, last declared first, with the class's
     * typeinfo function).  The names getPositionX / getZoom /
     * getMaxPositionX are Lupo 2's (see above). */
    f32  getMaxPositionX() { return zoom; }             /* 0x001716F8 */
    void setZoom(f32 z) { zoom = z; }                   /* 0x001716F0 */
    f32  getLength() { return length; }                 /* 0x001716E8 */
    f32  getZoom() { return position; }                 /* 0x001716E0 */
    f32  getPositionX() { return scrollY; }             /* 0x001716D8 */
    void setScrollY(f32 y) { scrollY = y; }             /* 0x001716D0 */
    void setSpeedMax(int v) { speedMax = v; }           /* 0x001716C8 */
    int  getSpeedMax() { return speedMax; }             /* 0x001716C0 */
    void setSpeedHeight(f32 h) { speedHeight = h; }     /* 0x001716B8 */
    f32  getSpeedHeight() { return speedHeight; }       /* 0x001716B0 */
    int  F_001716A8() { return unk_5DC; }               /* 0x001716A8 */
    void F_001716A0(int v) { unk_5DC = v; }             /* 0x001716A0 */
    int  getLRCancel() { return lrCancel; }             /* 0x00171698 */
    void setLRCancel(bool c) { lrCancel = c; }          /* 0x00171690 a word store of the bool */

    LoggerSector *sectors;                  /* 0x1E4 */
    RaceInput    *cars[2];                  /* 0x1E8 LoggerGraphRoot::cars */
    OneLapGhostInfo *records[40][2];           /* 0x1F0 per sector and car (u039 void *sectorRecord) */
    f32           sectorStart[40];          /* 0x330 the lap distance each sector starts at */
    int           sectorStartPos[40][2];    /* 0x3D0 records[i][car]->start (OneLapGhostInfo: the logger
                                               position at the start) */
    int           lengthSum[2];             /* 0x510 the sum of records[i][car]->length */
    int           sectorCount;              /* 0x518 logger_sector_count() */
    f32           length;                   /* 0x51C the lap length plus the wraps of the sector starts;
                                               position wraps at it */
    f32           zoom;                     /* 0x520 0.4 .. 10 (pixels per metre) */
    f32           position;                 /* 0x524 the cursor (the centre of the view), metres */
    f32           scrollY;                  /* 0x528 lines scrolled (the left stick) */
    bool          sectorVisible[40];        /* 0x52C (virtual_12; written only).  bool: egcs's is 4
                                               bytes, and its own alias set lets virtual_12 keep
                                               sectorCount in a register across the stores */
    f32           left;                     /* 0x5CC the view, metres (virtual_12) */
    f32           right;                    /* 0x5D0 */
    int           speedMax;                 /* 0x5D4 the speed at the top of the speed graph (800) */
    f32           speedHeight;              /* 0x5D8 its lines (g_002AEDE0[LoggerGraphRoot::scale]) */
    int           unk_5DC;                  /* 0x5DC non-zero: LoggerSector takes no input; cleared every
                                               frame by LoggerGraphRoot::virtual_16 */
    int           lrCancel;                 /* 0x5E0 set while the zoom stick moves: LoggerGraphRoot
                                               swallows the events */
};
EGCS_SIZE(gt2k_menu::LoggerRaceGraph, 0x5E4);

/* vtable 0x00324350 (24 slots: [0], [8], [9], [16], [17] its own).
 * typeinfo __tfQ29gt2k_menu15LoggerGraphRoot 0x00171640 (si, base
 * RawWindow; __si_type_info 0x004B1C80).  Size 0x230 (operator new in
 * create).  The graph mode ("GraphRoot"). */
class LoggerGraphRoot : public RawWindow {
public:
    /* The logged race (org, race, pool, the two cars) and the option panel
     * "LoggerGraphOperation" (its "GraphOperation" list: keepSubListIndex
     * 0, moveLoop 1); childrenFocusChange 0, scale 0. */
    LoggerGraphRoot(const RawWindowArgs &args); /* 0x0016FB70 */
    virtual ~LoggerGraphRoot();             /* [0] +0x0C 0x0016FC88 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x0016FAA8 g_00351C1C "LoggerGraphRoot" */
    /* Every frame: the speed scale, per car shown the speed at the cursor,
     * the lap time, the sector's time and average speed; "graph_point" on
     * the course map at car A's position (LoggerCourseMap::toMap). */
    virtual void virtual_9();               /* [9] +0x54 0x00170170 */
    /* While the option panel is shown: the options from its lists, 0.
     * Else with an analog pad (type 5, 7, 8, curve g_003416F8): axis 3
     * zooms, axis 0 moves the cursor (wrapping), axis 1 scrolls; a zoom
     * move sets lrCancel.  Then RawWindow's. */
    virtual int  virtual_16(MenuControl *ctl);  /* [16] +0x8C 0x001705E0 */
    /* Nothing while the zoom moves.  1 swallowed, 2 closes the option panel
     * (if it has the input), 3 / 4 from this window step scale (0..2). */
    virtual int  virtual_17(RawWindow *sender, int event, int arg); /* [17] +0x94 0x00170968 */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x0016FAB0 */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x0016FAF0 */
    static int        canCreate(const RawWindowArgs *args);    /* 0x0016FB68 1 (args unused) */
    /* The texts of "GraphTrackA" / "GraphTrackB" (g_00351C30), their icon /
     * title pictures (B's box to x 328), "graph_point". */
    void setup(LoggerSector *s);            /* 0x0016FCE0 */
    LoggerSector *getRootSectors();         /* 0x0016FEA8 the parent LoggerRoot's (not checked) */
    void closeOperation();                  /* 0x0016FF00 the input back to the sector list and "Graph" */
    void openOperation();                   /* 0x0016FFA0 the panel centred, with the current options */
    /* Inline (T087 writes the copies, last declared first, with the class's
     * typeinfo function). */
    RaceOrganization *getRaceOrganization() { return org; }     /* 0x00171638 */
    RaceBase *getOrganization() { return race; }        /* 0x00171630 (Lupo 2 name; reads race) */
    RaceMemPool *getRace() { return pool; }             /* 0x00171628 (Lupo 2 name; reads pool) */
    RaceInput *getCar(int car) { return cars[car]; }    /* 0x00171618 */
    void setGraph(LoggerRaceGraph *g) { graph = g; }    /* 0x00171610 */

    RawWindow        *operation;            /* 0x1E4 "LoggerGraphOperation" (the option panel) */
    RawList          *graphOperation;       /* 0x1E8 its "GraphOperation" list */
    LoggerSector     *sectors;              /* 0x1EC */
    LoggerRaceGraph  *graph;                /* 0x1F0 "Graph" (set by LoggerRoot::setup) */
    RaceOrganization *org;                  /* 0x1F4 logger_get_org() */
    RaceBase         *race;                 /* 0x1F8 org->race */
    RaceMemPool      *pool;                 /* 0x1FC race->virtual_4() */
    RaceInput        *cars[2];              /* 0x200 race->virtual_26(0x100), race->virtual_26(1) */
    RawWindow        *speedInfo[2];         /* 0x208 "r_speed_info" */
    RawWindow        *totalInfo[2];         /* 0x210 "r_total_info" */
    RawWindow        *sectorInfo[2];        /* 0x218 "r_sector_info" */
    RawWindow        *averageInfo[2];       /* 0x220 "r_average_info" */
    RawWindow        *graphPoint;           /* 0x228 "graph_point" (on the course map) */
    int               scale;                /* 0x22C 0..2: speedHeight = g_002AEDE0[scale] */
};
EGCS_SIZE(gt2k_menu::LoggerGraphRoot, 0x230);

/* vtable 0x00324288 (24 slots: [0], [8], [16], [17] its own).  typeinfo
 * __tfQ29gt2k_menu10LoggerRoot 0x001715C0 (si, base RawWindow;
 * __si_type_info 0x004B1CD0).  Size 0x1F8 (operator new in create).  The
 * "LoggerRoot" window of the page: the sector list and the two modes. */
class LoggerRoot : public RawWindow {
public:
    LoggerRoot(const RawWindowArgs &args);  /* 0x00170B58 logger_set_root(this); control = args.owner */
    virtual ~LoggerRoot();                  /* [0] +0x0C 0x00170BD8 logger_set_root(NULL) */
    virtual const char *virtual_8();        /* [8] +0x4C 0x00170A90 g_00351C40 "LoggerRoot" */
    /* Start (CONTROL_START, pressed) sends event 8, L1 / R1 the sector
     * list's RAW_EVENT_LEFT / RIGHT (5 / 6: its cursor), L2 / R2 10 / 11;
     * the event's result.  1 when not shown. */
    virtual int  virtual_16(MenuControl *ctl);  /* [16] +0x8C 0x00170E68 */
    /* Unless locked: 8 the graph's option panel (graph mode), 9 the
     * replay's enterPlaying (replay mode), 10 / 11 replay <-> graph. */
    virtual int  virtual_17(RawWindow *sender, int event, int arg); /* [17] +0x94 0x00170FA8 */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x00170A98 */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x00170AD8 */
    static int        canCreate(const RawWindowArgs *args);    /* 0x00170B50 1 (args unused) */
    /* Wires the children: the sector list (scroll "SectorBox", moveLoop,
     * scrollCenter, at 0, 0), the graph mode (map, graph: g_00351AFC), the
     * replay mode and its buttons; showMode. */
    void setup();                           /* 0x00170C38 */
    void showMode();                        /* 0x00170D70 the window of control->state (1 replay, 2 graph) */
    void start();                           /* 0x00170E30 locked 0, replayRoot->leave(), closeOperation */
    /* Inline (T087 writes the copies, last declared first, with the class's
     * typeinfo function). */
    int  isLocked() { return locked; }                  /* 0x001715B8 */
    LoggerSector *getSectorList() { return sectors; }   /* 0x001715B0 */
    LoggerReplayRoot *getSectors() { return replayRoot; }   /* 0x001715A8 (Lupo 2 name; reads replayRoot) */
    LoggerGraphRoot *getGraphRoot() { return graphRoot; }   /* 0x001715A0 */

    int               locked;               /* 0x1E4 non-zero: the events go to RawWindow */
    LoggerControl    *control;              /* 0x1E8 args.owner */
    LoggerSector     *sectors;              /* 0x1EC "Sectors" */
    LoggerReplayRoot *replayRoot;           /* 0x1F0 "ReplayRoot" */
    LoggerGraphRoot  *graphRoot;            /* 0x1F4 "GraphRoot" */
};
EGCS_SIZE(gt2k_menu::LoggerRoot, 0x1F8);

/* vtable 0x003241C0 (24 slots: [0], [8], [17] its own).  typeinfo
 * __tfQ29gt2k_menu12LoggerReplay 0x00171550 (si, base RawWindow;
 * __si_type_info 0x004B1CB0).  Size 0x1E8 (operator new in create).  The
 * window of the "replay_logger" pages. */
class LoggerReplay : public RawWindow {
public:
    /* g_00351AFC NULL; loads "LoggerRoot" as its child, setup; takes
     * args.owner as its control (RawWindow::virtual_14, called directly);
     * root->start(). */
    LoggerReplay(const RawWindowArgs &args);    /* 0x001711D8 */
    virtual ~LoggerReplay();                /* [0] +0x0C 0x00171280 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x00171110 g_00351C60 "LoggerReplay" */
    /* Cancel (2) from inside a LoggerSector: the "top" page (state 0, sound
     * 1, render.window NULL, pages +0xB4C = 1, setFreeze(0)), 0; else 1. */
    virtual int  virtual_17(RawWindow *sender, int event, int arg); /* [17] +0x94 0x001712D8 */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x00171118 (T055's g_0029DE48
                                               "replay_logger") */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x00171158 */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001711D0 1 (args unused) */

    LoggerRoot *root;                       /* 0x1E4 */
};
EGCS_SIZE(gt2k_menu::LoggerReplay, 0x1E8);

/* LoggerRaceGraph's drawing helpers (no `this'). */
void logger_draw_mark(f32 x, f32 halfWidth, f32 y0, f32 y1);    /* 0x0016E968 the cursor mark: a
                                               triangle, apex (x, y1), base y0 */
void logger_draw_leader(f32 *x, f32 *y, f32 dx, f32 dy);        /* 0x0016E9F8 a speed label's leader
                                               line; (*x, *y) becomes its end */

} /* namespace gt2k_menu */

/* The helpers over the logged race (logger_get_org(), T055).  car: 0 the
 * player's (input id 0x100), 1 the other one (id 1). */
LoggerGraphOptions *logger_graph_options(void);     /* 0x0016C260 &g_002AED68 */
RaceInput    *logger_car_input(int car);            /* 0x0016C270 race->virtual_26(car ? 1 : 0x100)
                                                      (first pass: logger_car_dynamics) */
RaceEntryCar *logger_entry_car(int car);            /* 0x0016C2B8 entries->cars[car] */
RaceMemPool  *logger_race_pool(void);               /* 0x0016C2F0 race->virtual_4() */
/* The car's time in sector `sector' (ms): the difference of the cumulative
 * times (sector 0: its own). */
int           logger_sector_time(int car, int sector);  /* 0x0016C328 */
LoggerCourseMap *logger_course_map(void);           /* 0x0016C3C8 the pool's courseMap */
/* The sectors both cars recorded: the smaller non-zero count of the two
 * (records with a length, at most lastSector + 1); lastSector + 1 if
 * neither has any. */
int           logger_sector_count(void);            /* 0x0016C400 */
/* The values the graphs plot: throttle and brake 0..1, gear / 7, km/h. */
f32 logger_sample_throttle(LoggerSample *s);        /* 0x0016DC18 */
f32 logger_sample_brake(LoggerSample *s);           /* 0x0016DC40 */
f32 logger_sample_gear(LoggerSample *s);            /* 0x0016DC68 */
f32 logger_sample_speed(LoggerSample *s);           /* 0x0016DC98 */

/* (The static destructor 0x00171358 / initialiser 0x001713E8 of the ten
 * registrars, .dtors 0x0028F1F8 / .ctors 0x0028F0B8, are egcs's
 * _GLOBAL_$D$ / _GLOBAL_$I$ of T087: not declared.) */

/* T087's .data. */
extern LoggerGraphOptions g_002AED68;       /* 0x002AED68 */
extern f32 g_002AED80[2][4];                /* 0x002AED80 the throttle colours of car A / B (rgba) */
extern f32 g_002AEDA0[2][4];                /* 0x002AEDA0 the brake colours */
extern f32 g_002AEDC0[2][4];                /* 0x002AEDC0 the car colours (A red, B blue) */
extern int g_002AEDE0[3];                   /* 0x002AEDE0 speedHeight per scale {800, 533, 400} */

/* The analog stick curve g_003416F8 (LoggerGraphRoot, u065, u095, u105):
 * its own header (T214's original did not see the logger windows). */
#include "engine/input/T472_analog_curve.h"

/* T087's .sdata: per class the name pointer, the RawWindowClass record
 * {name, create} and its registrar (as T272's; defined in T087's source,
 * each in front of its class's first function; egcs's static constructor
 * builds the registrars; the first pass's 0x18-byte WindowClassInfo
 * g_00351B00 .. is these three plus the padding word). */
extern const char *g_00351B00;                  /* 0x00351B00 gp-0x5EF0 "LoggerSector" */
extern gt2k_menu::RawWindowClass    g_00351B08; /* 0x00351B08 gp-0x5EE8 {name, LoggerSector::create} */
extern gt2k_menu::RawWindowRegister g_00351B10; /* 0x00351B10 gp-0x5EE0 its registrar */
extern const char *g_00351B18;                  /* 0x00351B18 gp-0x5ED8 "LoggerMapCourse" */
extern gt2k_menu::RawWindowClass    g_00351B20; /* 0x00351B20 gp-0x5ED0 {name, LoggerMapCourse::create} */
extern gt2k_menu::RawWindowRegister g_00351B28; /* 0x00351B28 gp-0x5EC8 its registrar */
extern const char *g_00351B30;                  /* 0x00351B30 gp-0x5EC0 "LoggerRaceReplay" */
extern gt2k_menu::RawWindowClass    g_00351B38; /* 0x00351B38 gp-0x5EB8 {name, LoggerRaceReplay::create} */
extern gt2k_menu::RawWindowRegister g_00351B40; /* 0x00351B40 gp-0x5EB0 its registrar */
extern const char *g_00351B48;                  /* 0x00351B48 gp-0x5EA8 "LoggerReplayControl" */
extern gt2k_menu::RawWindowClass    g_00351B50; /* 0x00351B50 gp-0x5EA0 {name, LoggerReplayControl::create} */
extern gt2k_menu::RawWindowRegister g_00351B58; /* 0x00351B58 gp-0x5E98 its registrar */
extern const char *g_00351B60;                  /* 0x00351B60 gp-0x5E90 "LoggerReplayRoot" */
extern gt2k_menu::RawWindowClass    g_00351B68; /* 0x00351B68 gp-0x5E88 {name, LoggerReplayRoot::create} */
extern gt2k_menu::RawWindowRegister g_00351B70; /* 0x00351B70 gp-0x5E80 its registrar */
extern const char *g_00351B78[2];               /* 0x00351B78 gp-0x5E78 {"ReplayTrackA", "ReplayTrackB"} */
extern const char *g_00351B98;                  /* 0x00351B98 gp-0x5E58 "LoggerGraphMap" */
extern gt2k_menu::RawWindowClass    g_00351BA0; /* 0x00351BA0 gp-0x5E50 {name, LoggerGraphMap::create} */
extern gt2k_menu::RawWindowRegister g_00351BA8; /* 0x00351BA8 gp-0x5E48 its registrar */
extern const char *g_00351BB0;                  /* 0x00351BB0 gp-0x5E40 "LoggerRaceGraph" */
extern gt2k_menu::RawWindowClass    g_00351BB8; /* 0x00351BB8 gp-0x5E38 {name, LoggerRaceGraph::create} */
extern gt2k_menu::RawWindowRegister g_00351BC0; /* 0x00351BC0 gp-0x5E30 its registrar */
extern gt2k_menu::LoggerRaceGraph *g_00351AFC;  /* 0x00351AFC gp-0x5EF4 the graph of the open LoggerRoot
                                                   (written only) */
extern const char *g_00351C1C;                  /* 0x00351C1C gp-0x5DD4 "LoggerGraphRoot" */
extern gt2k_menu::RawWindowClass    g_00351C20; /* 0x00351C20 gp-0x5DD0 {name, LoggerGraphRoot::create} */
extern gt2k_menu::RawWindowRegister g_00351C28; /* 0x00351C28 gp-0x5DC8 its registrar */
extern const char *g_00351C30[2];               /* 0x00351C30 gp-0x5DC0 {"GraphTrackA", "GraphTrackB"} */
extern const char *g_00351C40;                  /* 0x00351C40 gp-0x5DB0 "LoggerRoot" */
extern gt2k_menu::RawWindowClass    g_00351C48; /* 0x00351C48 gp-0x5DA8 {name, LoggerRoot::create} */
extern gt2k_menu::RawWindowRegister g_00351C50; /* 0x00351C50 gp-0x5DA0 its registrar */
extern const char *g_00351C60;                  /* 0x00351C60 gp-0x5D90 "LoggerReplay" */
extern gt2k_menu::RawWindowClass    g_00351C68; /* 0x00351C68 gp-0x5D88 {name, LoggerReplay::create} */
extern gt2k_menu::RawWindowRegister g_00351C70; /* 0x00351C70 gp-0x5D80 its registrar */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself (and rejects hand
 * declarations of them). */
extern "C" {
void *__tfQ29gt2k_menu12LoggerReplay(void);         /* 0x00171550 */
void *__tfQ29gt2k_menu10LoggerRoot(void);           /* 0x001715C0 */
void *__tfQ29gt2k_menu15LoggerGraphRoot(void);      /* 0x00171640 */
void *__tfQ29gt2k_menu15LoggerRaceGraph(void);      /* 0x00171700 */
void *__tfQ29gt2k_menu14LoggerGraphMap(void);       /* 0x00171750 */
void *__tfQ29gt2k_menu16LoggerReplayRoot(void);     /* 0x001717A8 */
void *__tfQ29gt2k_menu19LoggerReplayControl(void);  /* 0x00171800 */
void *__tfQ29gt2k_menu16LoggerRaceReplay(void);     /* 0x00171850 */
void *__tfQ29gt2k_menu15LoggerMapCourse(void);      /* 0x001718A8 */
void *__tfQ29gt2k_menu12LoggerSector(void);         /* 0x00171918 */
}
#endif

#endif /* GAME_T087_LOGGER_WINDOWS_H */
