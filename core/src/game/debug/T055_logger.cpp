/* T055 - the data logger: gt2k_menu::LoggerControl, LoggerRender,
 * LoggerPage, LoggerInput, the recording thread and the replay controls.
 * 0x00134EE8..0x00137468.
 *
 * How it runs.  logger_run (from the race modes, over the race just
 * driven) or logger_main (from main: replays loaded from the memory card)
 * build a LoggerControl on the stack; LoggerControl::run gives it a 0x4000-
 * byte thread stack, a 0x32000-byte work area and the LoggerData (all on its
 * own stack frame) and runs the pages g_0029DE38[state] ("top", then
 * "replay_logger") until state is 4.  While a race is set, mainLoop starts
 * logger_record_thread: it runs the race (a replay) as fast as it can and
 * samples both cars (the player, entry 0, and entry 1 - or entry 0 again
 * when the second input is held, i.e. there is no second car) every
 * `interval' metres of their lap distance: the average speed (x 40, 13
 * bits), throttle and brake (0..255) over the interval (integrals of the
 * lines between two frames, segment_area), the gear, and the position
 * interpolated at the interval's end (line_at).  interval = lap length /
 * 16383, at least 0.5 m, so that a lap fits the 0x4000 samples.  When the
 * replay ends the speed, throttle and brake are smoothed (1-2-1).  The
 * "replay_logger" pages then play the replay under the control of the T086
 * / T087 windows: a seek to a logger sector (logger_request_seek), play,
 * speed, a camera change (logger_request_step); a sector's end stops the
 * play (or, with g_0035158C, plays the sector again).
 *
 * Retail's data (u018's data pass): .rodata 0x00316CC8..0x00317120,
 * "bad_alloc" (<new>, read first), the literals, the four vtables, then the
 * type names LoggerInput, LoggerControl, LoggerPage, LoggerRender,
 * OperatorList .. Cell<OperatorHolder>, RaceMiniMap .. RaceDisplayBase,
 * RaceCameraBase, RegurationResult, RecordsMenuArg, the garage wheel's
 * four, SmartIndexT<int>, the garage car's four, UnitArena<64>, Operator:
 * the reverse of the class definitions, which the includes below give
 * (SmartIndexT<int> between the car list and the wheel: T251 read between
 * T429_garage_car_list.h and the GameStatus, an order only this original
 * has; no memory card classes: the entry and load-info headers only).
 * .sdata 0x00351540: T157's "" (through T154), then the variables below.
 * .sbss 0x00353210..0x0035322C: the file statics. */
#include <new>                              /* first: "bad_alloc" */
#include "types.h"
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name (retail has it, defined first) */
#include "game/race/core/T153_race_course.h"          /* RaceCourse::getLapLength */
#include "game/save/T429_garage_car_list.h"      /* (no use: the garage car's type names) */
#include "game/common/T251_smart_index.h"          /* (no use: SmartIndexT<int>, instantiated here) */
#include "game/save/T429_game_status.h"          /* (no use: the garage wheel's type names) */
#include "game/menu/records/T089_records_menu_args.h"    /* (no use: RecordsMenuArg's name) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (no use: RegurationResult's name) */
#include "game/camera/T383_race_camera_base.h"     /* (no use: RaceCameraBase) */
#include "game/race/display/T154_race_display.h"         /* (no use: the race displays; T157's "" opens the .sdata) */
#include "game/menu/framework/T216_menu_transitive.h"      /* (no use: the menu classes) */
#include "game/debug/T055_logger.h"
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "engine/media/G002_mpeg_frameq.h"          /* intr_suspend, ThreadCtx (complete before g_00353220:
                                               8 bytes, $gp-relative), worker_thread_* */
#include "game/app/T007_game_init.h"            /* race_begin, race_end */
#include "game/menu/memorycard/T068_memorycard_entry.h"     /* F_0014AE00, gt2k_menu::MEMCARD_LOGGER_LOAD */
#include "game/menu/memorycard/T068_memorycard_load_info.h" /* gt2k_menu::MemorycardLoadInfo */
#include "game/debug/T086_logger_top.h"           /* gt2k_menu::LoggerTop (factory), LoggerProgressBar */
#include "game/debug/T087_logger_windows.h"       /* gt2k_menu::LoggerReplay (factory), LoggerRaceReplay, LoggerRoot */
#include "game/debug/T088_logger_course_map.h"    /* LoggerCourseMap::relocate */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase, RaceEntryCar, RaceDate */
#include "game/race/core/T123_race_setup.h"           /* race_organization_create */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization */
#include "game/race/core/T129_race_base.h"            /* RaceBase, race_destroy, g_00351DF0 */
#include "game/race/core/T130_race_ps2_base.h"        /* RacePS2Base (T173 RaceSmoke::reset) */
#include "game/race/modes/T139_race_solitaire.h"       /* RaceSolitaire */
#include "game/race/core/T149_race_replay_information.h"  /* RaceReplayInformation */
#include "game/race/core/T174_race_mem_pool.h"        /* RaceMemPool */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push, menu_heap_pop */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuHook, MenuHookFn */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset, g_0035274C, g_00352750 */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState, car_view_x, car_view_z */
#include "game/race/dynamics/T305_dynamics_conductor.h"   /* DynamicsConductor (replaySpeed) */
#include "game/race/dynamics/T318_control_record.h"       /* AutomobileControlRecord::Recorder */
#include "game/camera/T383_camera_manager.h"       /* CameraManager (views[0].input, loggerSeek, update) */
#include "game/race/core/T413_race_input.h"           /* RaceInput */
#include "game/sound/T464_bgm.h"                  /* bgm_stop */
#include "game/data/T489_car_database.h"         /* db_course_name, db_race_name, race_name_split */
#include "engine/io/T503_vol_thread.h"           /* file_read_blocking, vol_device_object */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "game/debug/logger_internal.h"

// DATA_START 0x00316CC8
// DATA_START 0x00351540

using namespace gt2k_menu;

/* max.s / min.s.  Every clamp of this file is a max.s / min.s in retail,
 * and they behave like volatile asm statements, not like egcs's MAX_EXPR /
 * MIN_EXPR (the `>?' / `<?' operators, or -ffast-math ternaries): they stay
 * where the source has them relative to stores and calls (record's min.s
 * after each sample store, the max.s first in the queries) and are never in
 * a delay slot (none of the image's 166 max.s / min.s is).  The original's
 * helpers were inline asm, then; with them speedAt / xBefore / zBefore are
 * exact, with `>?' 76%.  The divisions by constants stay div.s (no
 * -ffast-math).  GCC 14: the C ternaries (equal results; max.s / min.s
 * differ only for -0.0 against 0.0). */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
static inline f32 logger_maxf(f32 a, f32 b)
{
    f32 r;
    __asm__ volatile("max.s %0,%1,%2" : "=f"(r) : "f"(a), "f"(b));
    return r;
}
static inline f32 logger_minf(f32 a, f32 b)
{
    f32 r;
    __asm__ volatile("min.s %0,%1,%2" : "=f"(r) : "f"(a), "f"(b));
    return r;
}
#define LOGGER_MAXF(a, b) logger_maxf(a, b)
#define LOGGER_MINF(a, b) logger_minf(a, b)
#else
#define LOGGER_MAXF(a, b) ((a) > (b) ? (a) : (b))
#define LOGGER_MINF(a, b) ((a) < (b) ? (a) : (b))
#endif

/* Swaps two floats (a statement macro: its do-while's loop notes give the
 * alignment nop retail has after the swap of the queries). */
#define LOGGER_SWAPF(a, b)  do { f32 swap_ = (a); (a) = (b); (b) = swap_; } while (0)

/* The 64-byte aligned start of a buffer.  logger_main evaluates it at each
 * use (retail recomputes the `and' after the join, CSE'd within a block). */
#define LOGGER_ALIGN64(p)   ((void *)(((int)(p) + 0x3F) & ~0x3F))

/* ------------------------------------------------------------------------ */
/* T055's .sdata (0x00351544..0x00351574; the "" before it, 0x00351540, is a */
/* header inline's string): defined here, ahead of logger_load_course_map, */
/* whose "logger/" follows them (0x00351578).                               */
/* ------------------------------------------------------------------------ */
int g_00351544 = 0;                         /* 0x00351544 */
int g_00351548 = 0;                         /* 0x00351548 */
LoggerRoot *g_0035154C = NULL;              /* 0x0035154C */
LoggerProgressBar *g_00351550 = NULL;       /* 0x00351550 */
void *g_00351554 = NULL;                    /* 0x00351554 */
int g_00351558 = 0;                         /* 0x00351558 */
int g_0035155C = 0;                         /* 0x0035155C */
volatile int g_00351560 = 0;                /* 0x00351560 */
volatile int g_00351564 = 0;                /* 0x00351564 */
f32 g_00351568 = 0.0f;                      /* 0x00351568 */
int g_0035156C = -1;                        /* 0x0035156C */
int g_00351570 = -1;                        /* 0x00351570 */
int g_00351574 = 0;                         /* 0x00351574 */

/* ------------------------------------------------------------------------ */
/* T055's .sbss (0x00353210..0x0035322C): file statics (egcs writes a C++  */
/* global without an initialiser into .sdata; retail has these in .sbss),   */
/* emitted at the end of the object in definition order.                    */
/* ------------------------------------------------------------------------ */
static RaceOrganization *g_00353210;        /* 0x00353210 gp-0x47E0 the logged race's, NULL none */
static void *g_00353214;                    /* 0x00353214 gp-0x47DC logger_main's race buffer */
static void *g_00353218;                    /* 0x00353218 gp-0x47D8 LoggerControl::run's work area */
static int  g_0035321C;                     /* 0x0035321C gp-0x47D4 its size (0x32000) */
static ThreadCtx g_00353220;                /* 0x00353220 gp-0x47D0 the recording thread */
static LoggerData *g_00353228;              /* 0x00353228 gp-0x47C8 LoggerControl::run's LoggerData */

/* ------------------------------------------------------------------------ */
/* LoggerInput                                                              */
/* ------------------------------------------------------------------------ */
namespace gt2k_menu {

/* RaceInput(), then the vptr (+0xC0). */
// FUNC 0x00134EE8 gt2k_menu::LoggerInput::LoggerInput
LoggerInput::LoggerInput()
    : RaceInput()
{
}

/* Slot 0.  The vptr, then RaceInput's destructor (~RaceInput, tail call)
 * with this destructor's flags. */
// FUNC 0x00134F20 gt2k_menu::LoggerInput::~LoggerInput
LoggerInput::~LoggerInput()
{
    /* the vptr, then ~RaceInput (a tail call) with these flags */
    /* implicit: ~RaceInput (base destructor) */
}

/* For the replay's camera change: the camera button RIB_REPLAY_CROSS
 * (0x400000, u095) pressed this frame - curButtons {mask all, 0x400000},
 * prevButtons {mask all, 0} - so that getButtonsOn() reports it and the
 * camera mode handlers (T391 updateLogger) step the view kind (designer,
 * onboard, logger camera). */
// FUNC 0x00134F48 gt2k_menu::LoggerInput::setNeutral
void LoggerInput::setNeutral()
{
    prevButtons.mask    = 0xFFFFFFFF;
    prevButtons.buttons = 0;
    curButtons.mask     = 0xFFFFFFFF;
    curButtons.buttons  = 0x400000;
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* LoggerData: the queries of the logger windows (T086 / T087)              */
/* ------------------------------------------------------------------------ */

/* The speed of the sample at distance t (clamped to the recorded range). */
// FUNC 0x00134F68 LoggerData::speedAt
f32 LoggerData::speedAt(int car, f32 t)
{
    t = LOGGER_MAXF(t, 0.0f);
    f32 end = (f32)(count[car] - 1) * interval;
    t = LOGGER_MINF(t, end);
    int index = (int)(t / interval);
    return (f32)samples[car][index].speed / 40.0f;
}

/* The x of the sample one interval before t (clamped to the range). */
// FUNC 0x00134FF0 LoggerData::xBefore
f32 LoggerData::xBefore(int car, f32 t)
{
    t = t - interval;
    t = LOGGER_MAXF(t, 0.0f);
    f32 end = (f32)(count[car] - 1) * interval;
    t = LOGGER_MINF(t, end);
    int index = (int)(t / interval);
    return (f32)samples[car][index].x;
}

/* The z of the sample one interval before t (clamped to the range). */
// FUNC 0x00135060 LoggerData::zBefore
f32 LoggerData::zBefore(int car, f32 t)
{
    t = t - interval;
    t = LOGGER_MAXF(t, 0.0f);
    f32 end = (f32)(count[car] - 1) * interval;
    t = LOGGER_MINF(t, end);
    int index = (int)(t / interval);
    return (f32)samples[car][index].z;
}

/* The four statistics below take [t0, t1] in either order, return 0 when
 * it is outside the recording, and keep 5 m from both of its ends:
 * samples (int)(max(t0, 5) / interval) .. (int)(min(t1, end - 5) / interval).
 * No callers in the binary for minSpeed / maxSpeed. */
// FUNC 0x001350D0 LoggerData::minSpeed
f32 LoggerData::minSpeed(int car, f32 t0, f32 t1)
{
    if (t1 < t0)
        LOGGER_SWAPF(t0, t1);
    f32 step = interval;
    f32 hi = (f32)(count[car] - 1) * step - 5.0f;
    if (hi < t0 || t1 < 0.0f)
        return 0.0f;
    int first = (int)(LOGGER_MAXF(t0, 5.0f) / step);
    int last  = (int)(LOGGER_MINF(t1, hi) / step);
    LoggerSample *s = samples[car];
    u32 best = s[first].speed;
    for (int i = first + 1; i <= last; i++) {
        u32 speed = s[i].speed;
        if (speed < best)
            best = speed;
    }
    return (f32)best / 40.0f;
}

// FUNC 0x00135210 LoggerData::maxSpeed
f32 LoggerData::maxSpeed(int car, f32 t0, f32 t1)
{
    if (t1 < t0)
        LOGGER_SWAPF(t0, t1);
    f32 step = interval;
    f32 hi = (f32)(count[car] - 1) * step - 5.0f;
    if (hi < t0 || t1 < 0.0f)
        return 0.0f;
    int first = (int)(LOGGER_MAXF(t0, 5.0f) / step);
    int last  = (int)(LOGGER_MINF(t1, hi) / step);
    LoggerSample *s = samples[car];
    u32 best = s[first].speed;
    for (int i = first + 1; i <= last; i++) {
        u32 speed = s[i].speed;
        if (best < speed)
            best = speed;
    }
    return (f32)best / 40.0f;
}

// FUNC 0x00135350 LoggerData::avgSpeed
f32 LoggerData::avgSpeed(int car, f32 t0, f32 t1)
{
    if (t1 < t0)
        LOGGER_SWAPF(t0, t1);
    f32 step = interval;
    f32 hi = (f32)(count[car] - 1) * step - 5.0f;
    if (hi < t0 || t1 < 0.0f)
        return 0.0f;
    int first = (int)(LOGGER_MAXF(t0, 5.0f) / step);
    int last  = (int)(LOGGER_MINF(t1, hi) / step);
    LoggerSample *s = samples[car];
    u32 sum = 0;
    for (int i = first; i <= last; i++)
        sum += s[i].speed;
    return (f32)sum / ((f32)(last - first + 1) * 40.0f);
}

/* The average throttle in percent (0..255 -> 0..100, at most 100). */
// FUNC 0x001354A0 LoggerData::avgThrottle
f32 LoggerData::avgThrottle(int car, f32 t0, f32 t1)
{
    if (t1 < t0)
        LOGGER_SWAPF(t0, t1);
    f32 step = interval;
    f32 hi = (f32)(count[car] - 1) * step - 5.0f;
    if (hi < t0 || t1 < 0.0f)
        return 0.0f;
    int first = (int)(LOGGER_MAXF(t0, 5.0f) / step);
    int last  = (int)(LOGGER_MINF(t1, hi) / step);
    LoggerSample *s = samples[car];
    u32 sum = 0;
    for (int i = first; i <= last; i++)
        sum += s[i].throttle;
    f32 percent = (f32)sum / ((f32)(last - first + 1) * 2.55f) + 0.0005f;
    return LOGGER_MINF(percent, 100.0f);
}

/* ------------------------------------------------------------------------ */
/* The recording                                                            */
/* ------------------------------------------------------------------------ */

/* The line through (x0, y0) and (x1, y1), at x. */
// FUNC 0x00135600 line_at
f32 line_at(f32 x0, f32 x1, f32 y0, f32 y1, f32 x)
{
    f32 inv = 1.0f / (x1 - x0);
    return ((y1 - y0) * x + (y0 * x1 - y1 * x0)) * inv;
}

/* The area under the segment (x0, y0)-(x1, y1) between lo and hi (the
 * segment clipped to [lo, hi]; 0 when they do not overlap). */
// FUNC 0x00135638 segment_area
f32 segment_area(f32 x0, f32 x1, f32 y0, f32 y1, f32 lo, f32 hi)
{
    if (x1 < lo || hi < x0)
        return 0.0f;
    if (x0 < lo || hi < x1) {
        f32 invDx  = 1.0f / (x1 - x0);
        f32 offset = y0 * x1 - y1 * x0;
        f32 dy     = y1 - y0;
        if (x0 < lo) {
            y0 = (dy * lo + offset) * invDx;
            x0 = lo;
        }
        if (hi < x1) {
            y1 = (dy * hi + offset) * invDx;
            x1 = hi;
        }
    }
    return (y0 + y1) * (x1 - x0) * 0.5f;
}

/* Reads "logger/<course name>" (the course map) into the race pool's
 * course map buffer (0x20000 bytes, unpacked) and relocates it.  The 8-byte
 * copy of "logger/" (from .sdata, one ld / sd) is strcpy expanded inline. */
// FUNC 0x001356F8 logger_load_course_map
void logger_load_course_map(void)
{
    char path[0x100];
    RaceOrganization *org = logger_get_org();
    RaceMemPool *pool = org->race->virtual_4();
    strcpy(path, "logger/");
    strcat(path, db_course_name(org->courseCode));
    LoggerCourseMap *map = (LoggerCourseMap *)file_read_blocking(path, pool->courseMap, 0x20000, 1,
                                                                 &vol_device_object);
    map->relocate();
}

/* (? name) The recording state of one car: a stack object of
 * logger_record_thread (car 0 at sp+0x00, car 1 at sp+0x50; 0x44 bytes),
 * its methods expanded inline there twice (no type of the binary: it may
 * as well be plain locals of a loop the compiler unrolled). */
struct LoggerRecorder {
    f32           last;                     /* 0x00 the furthest distance reached */
    f32           sum[3];                   /* 0x04 integrals of speed, throttle, brake over the current interval */
    f32           prevDist;                 /* 0x10 the distance of the previous frame */
    f32           prev[3];                  /* 0x14 speed, throttle, brake at prevDist */
    f32           prevX;                    /* 0x20 */
    f32           prevZ;                    /* 0x24 */
    LoggerSample *samples;                  /* 0x28 LoggerData::samples[car] */
    int          *count;                    /* 0x2C &LoggerData::count[car] */
    f32           interval;                 /* 0x30 */
    f32           invInterval;              /* 0x34 1 / interval */
    f32           lapLength;                /* 0x38 */
    RaceCarState *car;                      /* 0x3C */
    int           first;                    /* 0x40 1 until the first frame is seen */

    void record();
    void smooth();
};

/* One frame of the car (called while *count < 0x4000).  The distance is the
 * lap distance plus (lap - 1) lap lengths; it never goes back (a smaller
 * one counts as `last'), the first one is at most 0.  Every interval the
 * frame's segments complete is written out as a sample (the float to
 * unsigned conversions are libgcc's fptoui calls); when the buffer becomes
 * full the previous-frame values are not updated any more. */
inline void LoggerRecorder::record()
{
    s16 lap      = car->st.lap;
    f32 dist     = car->st.lapDistance;
    u8  gear     = car->st.gear;
    f32 speed    = car->speed;
    f32 throttle = car->st.throttle;
    f32 brake    = car->st.brake;
    f32 x = car_view_x(car, 0.0f);
    f32 z = car_view_z(car, 0.0f);
    dist += (f32)(lap - 1) * lapLength;
    if (first != 0) {
        if (0.0f < dist)
            dist = 0.0f;
        last = dist;
        prevDist = dist;
        first = 0;
    } else if (dist <= last) {
        dist = last;
    } else {
        last = dist;
        f32 lo = (f32)*count * interval;
        f32 hi = lo + interval;
        sum[0] += segment_area(prevDist, dist, prev[0], speed, lo, hi);
        sum[1] += segment_area(prevDist, dist, prev[1], throttle, lo, hi);
        sum[2] += segment_area(prevDist, dist, prev[2], brake, lo, hi);
        while (hi <= dist) {
            f32 avgSpeed    = sum[0] * 40.0f * invInterval + 0.5f;
            f32 avgThrottle = sum[1] * 255.0f * invInterval + 0.5f;
            f32 avgBrake    = sum[2] * 255.0f * invInterval + 0.5f;
            f32 endX = line_at(prevDist, dist, prevX, x, hi);
            f32 endZ = line_at(prevDist, dist, prevZ, z, hi);
            samples[*count].gear = gear;
            avgSpeed = LOGGER_MINF(avgSpeed, 8191.0f);
            samples[*count].speed = (u32)avgSpeed;                /* fptoui */
            avgThrottle = LOGGER_MINF(avgThrottle, 255.0f);
            samples[*count].throttle = (u32)avgThrottle;          /* fptoui */
            avgBrake = LOGGER_MINF(avgBrake, 255.0f);
            samples[*count].brake = (u32)avgBrake;                /* fptoui */
            samples[*count].x = (int)endX;
            samples[*count].z = (int)endZ;
            (*count)++;
            if (*count >= 0x4000)
                return;
            hi += interval;
            lo += interval;
            sum[0] = segment_area(prevDist, dist, prev[0], speed, lo, hi);
            sum[1] = segment_area(prevDist, dist, prev[1], throttle, lo, hi);
            sum[2] = segment_area(prevDist, dist, prev[2], brake, lo, hi);
        }
    }
    prevDist = dist;
    prev[0] = speed;
    prev[1] = throttle;
    prev[2] = brake;
    prevX = x;
    prevZ = z;
}

/* The 1-2-1 filter on the speed, throttle and brake of samples
 * 1 .. count - 2 (the left neighbour is the unfiltered value). */
inline void LoggerRecorder::smooth()
{
    u32 leftSpeed    = samples[0].speed;
    u32 leftThrottle = samples[0].throttle;
    u32 leftBrake    = samples[0].brake;
    for (int i = 1; i < *count - 1; i++) {
        LoggerSample *s = &samples[i];
        LoggerSample *right = s + 1;
        u32 speed    = s->speed;
        u32 throttle = s->throttle;
        s->speed = (leftSpeed + speed * 2 + right->speed) >> 2;
        u32 brake = s->brake;
        samples[i].throttle = (leftThrottle + throttle * 2 + right->throttle) >> 2;
        samples[i].brake = (leftBrake + brake * 2 + right->brake) >> 2;
        leftSpeed    = speed;
        leftThrottle = throttle;
        leftBrake    = brake;
    }
}

/* The recording thread (started by logger_record_start; `arg' is not
 * used).  The logged race (a RaceSolitaire: logger_run dynamic_casts it,
 * logger_main builds one from the replay; slot 56 is RaceSolitaire's) is
 * started as a replay with its pool's sectors cleared, the course map
 * loaded, then run frame after frame (RaceBase::update) until stopped
 * (g_00351560) or until the race signals its end (takeSignal); each frame
 * both cars are recorded (rec0 / rec1 .record(): segment_area, line_at,
 * car_view_x / car_view_z, fptoui) and the progress bar, if any, gets the
 * position / frame count of the main input's replay record
 * (virtual_26(0x100)->rec.rec, Recorder::frameCount) with interrupts off.
 * After a complete run the samples are smoothed, g_00351564 = 1 and
 * g_00351568 = the first car's distance.  The thread ends with
 * worker_thread_exit. */
// FUNC 0x00135788 logger_record_thread
void logger_record_thread(void *arg)
{
    RaceOrganization *org = logger_get_org();
    RaceSolitaire *race = (RaceSolitaire *)org->race;
    org->loggerCreating = 1;
    RaceEntryBase *entries = org->entries;
    race->virtual_8(1);
    race->virtual_56();
    logger_load_course_map();
    LoggerData *data = logger_get_data();
    f32 lapLength = race->org->course->getLapLength();
    f32 interval = lapLength / 16383.0f;
    interval = LOGGER_MAXF(interval, 0.5f);

    LoggerRecorder rec0;                    /* sp+0x00 */
    LoggerRecorder rec1;                    /* sp+0x50 */
    data->interval = interval;
    data->count[0] = 0;
    rec0.first = 1;
    rec0.car = entries->cars[0]->state;
    rec0.invInterval = 1.0f / interval;
    rec0.samples = data->samples[0];
    rec0.count = &data->count[0];
    rec0.interval = interval;
    rec0.lapLength = lapLength;
    rec0.sum[0] = 0.0f;
    rec0.sum[1] = 0.0f;
    rec0.sum[2] = 0.0f;
    /* Input 1 held: no second car (the replay of one car); entry 0 again. */
    RaceEntryCar *other;
    if (race->virtual_26(1)->hold != 0)
        other = entries->cars[0];
    else
        other = entries->cars[1];
    rec1.samples = data->samples[1];
    rec1.lapLength = lapLength;
    rec1.car = other->state;
    rec1.invInterval = 1.0f / interval;
    int *count1 = &data->count[1];
    *count1 = 0;
    rec1.first = 1;
    rec1.count = count1;
    rec1.interval = interval;
    rec1.sum[0] = 0.0f;
    rec1.sum[1] = 0.0f;
    rec1.sum[2] = 0.0f;

    /* Each frame recN.record() (inline): car_view_x / car_view_z, segment_area,
     * line_at, the fptoui conversions. */
    /* implicit: car_view_x, car_view_z, segment_area, line_at, fptoui (the inline LoggerRecorder::record) */
    while (g_00351560 == 0 && race->takeSignal() != RACE_SIGNAL_END) {
        int wasEnabled = intr_suspend();
        if (g_00351550 != NULL) {
            AutomobileControlRecord::Recorder *recorder = &race->virtual_26(0x100)->rec.rec;
            g_00351550->current = recorder->frames;
            g_00351550->total = recorder->frameCount();
        }
        if (wasEnabled)
            __asm__ volatile("ei");
        race->update();
        if (*rec0.count < 0x4000)
            rec0.record();
        if (*rec1.count < 0x4000)
            rec1.record();
    }
    race->virtual_35();
    org->loggerCreating = 0;
    if (g_00351560 == 0) {
        rec0.smooth();
        rec1.smooth();
        g_00351564 = 1;
        g_00351568 = rec0.last;
        logger_progress_done();
    }
    worker_thread_exit(&g_00353220);
}

/* Stops a running recording, then starts a new one: priority 10, the
 * stack set by LoggerControl::run. */
// FUNC 0x001362A8 logger_record_start
void logger_record_start(void)
{
    logger_record_stop();
    g_00351560 = 0;
    g_0035155C += 1;
    g_00351564 = 0;
    worker_thread_start(&g_00353220, logger_record_thread, NULL, 10, g_00351554, g_00351558);
}

/* Waits for the end of the recording thread, if one was started. */
// FUNC 0x001362F8 logger_record_join
void logger_record_join(void)
{
    if (g_0035155C != 0) {
        worker_thread_join(&g_00353220);
        g_0035155C -= 1;
    }
}

// FUNC 0x00136330 logger_record_stop
void logger_record_stop(void)
{
    g_00351560 = 1;
    logger_record_join();
}

/* Callers: gt2k_menu::LoggerTop::virtual_17 / virtual_9 (T086). */
// FUNC 0x00136358 logger_is_recorded
int logger_is_recorded(void)
{
    return g_00351564;
}

/* Called by the LoggerRoot constructor / destructor (T087). */
// FUNC 0x00136368 logger_set_root
void logger_set_root(LoggerRoot *root)
{
    g_0035154C = root;
}

/* Called by the LoggerProgressBar constructor / destructor (T086). */
// FUNC 0x00136370 logger_set_progress_bar
void logger_set_progress_bar(LoggerProgressBar *bar)
{
    g_00351550 = bar;
}

/* Once the recording is complete, the progress bar (if any) shows 1 / 1;
 * interrupts off. */
// FUNC 0x00136378 logger_progress_done
void logger_progress_done(void)
{
    int wasEnabled = intr_suspend();
    if (g_00351564 != 0 && g_00351550 != NULL) {
        g_00351550->current = 1;
        g_00351550->total = 1;
    }
    if (wasEnabled)
        __asm__ volatile("ei");
}

/* A seek to logger sector `sector' (RaceSolitaire::seekRecords).  Callers:
 * LoggerControl::runPage, gt2k_menu::LoggerSector::virtual_28. */
// FUNC 0x001363C0 logger_request_seek
void logger_request_seek(int sector)
{
    g_00351570 = sector;
}

/* The seek request (-1 none), cleared; interrupts off. */
// FUNC 0x001363C8 logger_take_seek
int logger_take_seek(void)
{
    int wasEnabled = intr_suspend();
    int sector = g_00351570;
    g_00351570 = -1;
    if (wasEnabled)
        __asm__ volatile("ei");
    return sector;
}

/* Whether a replay can be loaded: the other car's (otherCar != 0, LoggerTop
 * "TrackB": logger_run loads it) when a race is logged, else only while
 * logger_main runs (g_00351548; "TrackA": the replay to log).  Callers:
 * gt2k_menu::LoggerTop::virtual_17 / 9. */
// FUNC 0x00136400 F_00136400
int F_00136400(int otherCar)
{
    if (otherCar != 0 && g_00353210 != NULL)
        return 1;
    return g_00351548;
}

// FUNC 0x00136420 logger_get_org
RaceOrganization *logger_get_org(void)
{
    return g_00353210;
}

/* No callers. */
// FUNC 0x00136428 logger_get_root
LoggerRoot *logger_get_root(void)
{
    return g_0035154C;
}

/* The recorded distance (the graphs' range). */
// FUNC 0x00136430 logger_get_length
f32 logger_get_length(void)
{
    return g_00351568;
}

/* The replay controls' .sdata (0x00351580..0x00351590, after "logger/"). */
int g_00351580 = 0;                         /* 0x00351580 */
int g_00351584 = 0;                         /* 0x00351584 */
int g_00351588 = 0;                         /* 0x00351588 */
int g_0035158C = 0;                         /* 0x0035158C */
int g_00351590 = 0;                         /* 0x00351590 */

// FUNC 0x00136438 logger_reset_replay
void logger_reset_replay(void)
{
    g_00351580 = 0;
    g_00351584 = 0;
    g_0035158C = 0;
    g_00351590 = 0;
}

/* Callers: gt2k_menu::LoggerReplayControl::virtual_17, LoggerRoot::virtual_17, ... */
// FUNC 0x00136450 logger_set_play
void logger_set_play(int on)
{
    g_00351580 = on;
    g_00351588 = on;
}

// FUNC 0x00136460 logger_is_playing
int logger_is_playing(void)
{
    return g_00351580;
}

/* No callers. */
// FUNC 0x00136468 logger_set_loop
void logger_set_loop(int on)
{
    g_0035158C = on;
}

// FUNC 0x00136470 logger_set_speed
void logger_set_speed(int index)
{
    g_00351590 = index;
}

/* "ReplayCamera" (LoggerReplayControl::virtual_17): a camera change for
 * the next frame (runPage presses the camera button of its LoggerInput). */
// FUNC 0x00136478 logger_request_step
void logger_request_step(void)
{
    g_00351584 = 1;
}

/* The camera change request, cleared; interrupts off. */
// FUNC 0x00136488 logger_take_step
int logger_take_step(void)
{
    int wasEnabled = intr_suspend();
    int request = g_00351584;
    g_00351584 = 0;
    if (wasEnabled)
        __asm__ volatile("ei");
    return request;
}

/* ------------------------------------------------------------------------ */
/* LoggerRender, LoggerPage, LoggerControl                                  */
/* ------------------------------------------------------------------------ */
namespace gt2k_menu {

/* RenderBase(), window = NULL, the vptr. */
// FUNC 0x001364B8 gt2k_menu::LoggerRender::LoggerRender
LoggerRender::LoggerRender()
    : RenderBase()
{
    window = NULL;
}

/* Slot 0.  The vptr, the implicit RenderBase destructor expanded
 * (~RootObject(root, 2)), then GameObject::~GameObject(this, flags), which
 * does the delete. */
// FUNC 0x001364F0 gt2k_menu::LoggerRender::~LoggerRender
LoggerRender::~LoggerRender()
{
    /* the vptr, ~RootObject(&root, 2), GameObject::~GameObject(this, flags) */
    /* implicit: ~RootObject (member destructor), ~GameObject (base destructor) */
}

/* Slot 2.  No screen clear; the window's race view (the replay page's
 * LoggerRaceReplay) under the menu. */
// FUNC 0x00136540 gt2k_menu::LoggerRender::render
void LoggerRender::render(GSBuffer &buf)
{
    g_00352750 = 0;
    g_0035274C = 0;
    if (window != NULL)
        window->renderRace(buf);
    RenderBase::render(buf);
}

/* PageManager(control), then the vptr (+0xB58). */
// FUNC 0x00136590 gt2k_menu::LoggerPage::LoggerPage
LoggerPage::LoggerPage(ControlBase *control)
    : PageManager(control)
{
}

/* Slot 1: the vptr; PageManager has nothing to destroy; operator delete
 * when flags & 1. */
// FUNC 0x001365C8 gt2k_menu::LoggerPage::~LoggerPage
LoggerPage::~LoggerPage()
{
    /* the vptr; operator delete(this) when flags & 1 */
    /* implicit: operator delete (deleting destructor) */
}

/* Slot 0 (PageManager::createButtonList, 0x001B70F8, overridden): no button
 * list; the logger's pages are made of its item factories. */
// FUNC 0x001365F8 gt2k_menu::LoggerPage::createButtonList
ButtonList *LoggerPage::createButtonList(LinkPage *page, int index)
{
    return NULL;
}

/* ControlBase, the vptr, render (LoggerRender()), page (LoggerPage(this));
 * pageManager = &page; state = 0.  The landing pads destroy render
 * (~LoggerRender, flags 0) and the ControlBase (~ControlBase, flags 0). */
// FUNC 0x00136600 gt2k_menu::LoggerControl::LoggerControl
LoggerControl::LoggerControl()
    : ControlBase(), render(), page(this)
{
    /* implicit: LoggerRender, LoggerPage (member constructors), ~LoggerRender (exception cleanup) */
    pageManager = &page;
    state = LOGGER_STATE_TOP;
}

/* Slot 0.  The vptr, ~LoggerPage(&page, 2), ~LoggerRender(&render, 2), then
 * ~ControlBase with this destructor's flags. */
// FUNC 0x001366B0 gt2k_menu::LoggerControl::~LoggerControl
LoggerControl::~LoggerControl()
{
    /* the vptr, ~LoggerPage(&page, 2), ~LoggerRender(&render, 2), ~ControlBase(this, flags) */
    /* implicit: ~LoggerPage, ~LoggerRender (member destructors), ~ControlBase (base destructor) */
}

// FUNC 0x00136710 gt2k_menu::LoggerControl::loadPages
void LoggerControl::loadPages()
{
    page.loadFile("menu/logger.mbl");
}

// FUNC 0x00136738 gt2k_menu::LoggerControl::unloadPages
void LoggerControl::unloadPages()
{
    page.unloadFile();
}

/* T055's .data (0x0029DE28..0x0029DE78): the replay speeds, the pages by
 * state and the item factory table (filled by T055_static_init).  The page
 * names come here: "top" in .sdata after the replay controls (0x00351598)
 * and "replay_logger" in .rodata after "menu/logger.mbl" (0x00316CE8), then
 * the table's entry count (0x0035159C). */
} /* namespace gt2k_menu */

f32 g_0029DE28[4] = { 1.0f, 0.5f, 0.25f, 0.0f };   /* 0x0029DE28 */
const char *g_0029DE38[4] = { "top", "replay_logger", "replay_logger", "replay_logger" };  /* 0x0029DE38 */
MenuItemEntry g_0029DE48[2] = { { NULL } };         /* 0x0029DE48 */
int g_0035159C = sizeof(g_0029DE48) / sizeof(g_0029DE48[0]);   /* 0x0035159C */

namespace gt2k_menu {

/* The pads attached and one frame of the ControlBase, the renderer on
 * screen, the object tree and the logger's item factories (g_0029DE48,
 * g_0035159C = 2 of them) in both layouts; no screen clear. */
// FUNC 0x00136758 gt2k_menu::LoggerControl::begin
void LoggerControl::begin()
{
    attach();
    waitFrame();
    render.attach();
    page.initialize(&render.root, NULL);
    int hookCount = g_0035159C;
    page.common.setHooks(g_0029DE48, hookCount, this);
    page.own.setHooks(g_0029DE48, hookCount, this);
    g_00352750 = 0;
    g_0035274C = 0;
}

// FUNC 0x001367F8 gt2k_menu::LoggerControl::end
void LoggerControl::end()
{
    page.finalize();
    gfx_vsync(2);
    render.detach();
    detach();
}

/* Runs page `name' until the page manager exits (the render's mask off
 * meanwhile, on once it is left).  Any page but "top" plays the replay of
 * the logged race behind it (it must be set: a RaceSolitaire, see
 * logger_record_thread): the race is restarted (virtual_8(1)) with a seek
 * to sector 0 pending; then every frame, not rendered meanwhile
 * (renderEnabled 0):
 *   - sector = the seek request; when there is none and the replay plays,
 *     the sector play resumes from (taken);
 *   - a frame of the race first when it asked to end (endRequested), so
 *     that it ends;
 *   - when the race signalled its end it is restarted (virtual_42); then,
 *     without a request, the sector of the last seek again;
 *   - a seek (seekRecords, the smoke reset) remembers the sector;
 *   - while playing, after a seek, or for a camera change: one frame with
 *     a LoggerInput (replay mode 1) as the input of camera view 0, at the
 *     replay speed g_0029DE28[speed] (DynamicsConductor::replaySpeed, and
 *     / 60 as RaceSolitaire::timeStep), the cameras cut after a seek
 *     (loggerSeek); a camera change without play or seek runs only the
 *     cameras (CameraManager::update) with the camera button pressed
 *     (setNeutral); at a sector's end (loggerSector) play will resume from
 *     the last seek, and stops unless g_0035158C.
 * The LoggerInput is destroyed with flags 2 (~LoggerInput, also by its
 * landing pad). */
// FUNC 0x00136838 gt2k_menu::LoggerControl::runPage
void LoggerControl::runPage(const char *name)
{
    RaceOrganization *org = logger_get_org();
    RaceSolitaire *race = org != NULL ? (RaceSolitaire *)org->race : NULL;
    int replay = strcmp(name, "top") != 0;
    if (replay) {
        race->virtual_8(1);
        logger_request_seek(0);
        g_00351574 = 0;
        g_0035156C = -1;
    }
    render.setWindow(NULL);
    page.setPage(name, NULL);
    page.startPage(NULL);
    page.setFreeze(1);
    render.setMask(false);
    while (page.exited == 0) {
        page.update();
        if (replay) {
            race->renderEnabled = 0;
            int sector = logger_take_seek();
            if (sector < 0 && g_00351580 != 0) {
                sector = g_0035156C;
                g_0035156C = -1;
            }
            if (race->endRequested != 0)
                race->update();
            if (race->virtual_42() != 0 && sector < 0)
                sector = g_00351574;
            if (sector >= 0) {
                race->seekRecords(sector);
                race->smoke.reset();
                g_00351574 = sector;
                g_0035156C = -1;
            }
            int cameraChange = logger_take_step();
            if (g_00351580 != 0 || sector >= 0 || cameraChange != 0) {
                LoggerInput input;          /* sp+0x00 */
                input.setReplayMode(1);
                race->virtual_6()->views[0].input = &input;
                if (cameraChange != 0)
                    input.setNeutral();
                if (g_00351580 != 0 || sector >= 0)
                    cameraChange = 0;
                f32 speed = g_0029DE28[g_00351590];
                race->dc->replaySpeed = speed;
                race->timeStep = speed * (1.0f / 60.0f);
                if (sector >= 0)
                    race->virtual_6()->loggerSeek = 1;
                if (cameraChange == 0)
                    race->update();
                else
                    race->virtual_6()->update(race->org, race->dc);
                race->virtual_6()->views[0].input = NULL;
                if (org->loggerSector != 0) {
                    g_0035156C = g_00351574;
                    if (g_0035158C == 0)
                        g_00351580 = 0;
                }
            }
            race->renderEnabled = 1;
        }
        waitFrame();
    }
    render.setMask(true);
    page.closeAndExit();
    if (replay)
        race->virtual_35();
}

/* With a logged race, the recording thread runs during the pages; the pages
 * follow `state' (set by the pages, LoggerTop) until it is
 * LOGGER_STATE_END. */
// FUNC 0x00136BC0 gt2k_menu::LoggerControl::mainLoop
void LoggerControl::mainLoop()
{
    if (logger_get_org() != NULL)
        logger_record_start();
    state = LOGGER_STATE_TOP;
    while (state != LOGGER_STATE_END)
        runPage(g_0029DE38[state]);
    if (logger_get_org() != NULL)
        logger_record_stop();
}

} /* namespace gt2k_menu */

/* Callers: logger_record_thread and the graph windows (T087). */
// FUNC 0x00136C50 logger_get_data
LoggerData *logger_get_data(void)
{
    return g_00353228;
}

namespace gt2k_menu {

/* The logger's memory is this frame (0x76070 bytes): the recording thread's
 * stack (0x4000 bytes, 64-byte aligned), a 0x32000-byte work area (its
 * address and size are stored, never read) and the LoggerData. */
// FUNC 0x00136C58 gt2k_menu::LoggerControl::run
void LoggerControl::run()
{
    u8 stack[LOGGER_THREAD_STACK + 0x40];  /* sp+0x00000 */
    u8 work[LOGGER_WORK_SIZE];              /* sp+0x04040 */
    LoggerData data;                        /* sp+0x36040 */
    g_00351554 = (void *)(((int)stack + 0x3F) & ~0x3F);
    g_00351558 = LOGGER_THREAD_STACK;
    g_00353218 = work;
    g_0035321C = LOGGER_WORK_SIZE;
    g_00353228 = &data;
    loadPages();
    begin();
    mainLoop();
    end();
    unloadPages();
}

} /* namespace gt2k_menu */

/* The data logger over a race (callers: the race modes of T052 / T053 and
 * logger_main; `unused' is not read).  The organization's logger mode is 1
 * meanwhile.  Each pass: the replay controls reset, the menu draw state
 * reset, a menu heap over the race's pool (RaceSolitaire::virtual_4, the
 * race found by dynamic_cast), a LoggerControl run (~LoggerControl with
 * flags 2).  When the pages asked for the other car's data (g_00351544 ==
 * LOGGER_NEXT_LOAD_OTHER), a replay is loaded from the memory card (the
 * kind and index of the race's name, race_name_split, select the files)
 * into the race (virtual_59(1, ...): the second car's record) for entry 1
 * (its ghost lap time, driver name and date from the file), and the logger
 * runs again.  Returns 1.
 * The frame (0x42850 bytes): sp+0x10 the LoggerControl, whose slot the load
 * info shares; sp+0xE60 the replay buffer; sp+0x427E0 kind, sp+0x427E4
 * index. */
// FUNC 0x00136CF0 logger_run
int logger_run(RaceOrganization *org, int unused)
{
    g_00353210 = org;
    org->setLoggerMode(1);
    for (;;) {
        logger_reset_replay();
        menu_draw_reset();
        RaceSolitaire *race = dynamic_cast<RaceSolitaire *>(org->race);
        RaceMemPool *pool = race->virtual_4();
        menu_heap_push(pool->heap, pool->heapSize);
        {
            LoggerControl control;
            control.run();
            gfx_vsync(1);
        }
        menu_heap_pop();
        if (g_00351544 != LOGGER_NEXT_LOAD_OTHER)
            break;
        /* Declared after the LoggerControl's block: the frame puts the
         * control first (sp+0x10, its slot reused by info), then raw
         * (sp+0xE60), then kind / index (sp+0x427E0), as retail. */
        u8 raw[0x41980];
        RaceOrganization *raceOrg = race->org;
        int kind = 0;
        int index;
        race_name_split(db_race_name(raceOrg->raceCode), &kind, &index);
        MemorycardLoadInfo info;
        if (F_0014AE00(MEMCARD_LOGGER_LOAD, raceOrg->courseCode, 0, LOGGER_ALIGN64(raw), NULL, 0,
                       kind, index, &info) != 0) {
            race->virtual_59(1, (RaceInfoBlock *)LOGGER_ALIGN64(raw));
            RaceEntryCar *car = raceOrg->entries->cars[1];
            /* +0x08: the file's time (the replay / best lap thumbnails share the word) */
            car->ghostLapTime = info.thumbnail.bestlap.time;
            car->setDriverName(info.thumbnail.comment);
            car->setDate((const RaceDate *)&info.date);     /* the same 8-byte layout */
        }
    }
    org->setLoggerMode(0);
    return 1;
}

/* The logger with no race (its top page only, to load a replay): a
 * 0x20000-byte menu heap from memalign.  Returns 1. */
// FUNC 0x00136F18 logger_run_without_race
int logger_run_without_race(void)
{
    g_00353210 = NULL;
    menu_draw_reset();
    void *heap = memalign(0x40, 0x20000);
    menu_heap_push(heap, 0x20000);
    {
        LoggerControl control;              /* sp+0x00 (~LoggerControl, flags 2) */
        control.run();
        gfx_vsync(1);
    }
    menu_heap_pop();
    free(heap);
    return 1;
}

/* The data logger from main: a race session (race_begin(0x20000, 2) ..
 * race_end) in which the logger runs, first without a race; each time its
 * pages ask for a replay (g_00351544 == LOGGER_NEXT_LOAD) one is loaded from
 * the memory card and becomes the logged race: the old race and its
 * organization are destroyed (delete org, race_destroy), a
 * RaceReplayInformation of the data (destroyed with flags 2) builds the
 * organization (race_organization_create) and the race (createRace, in an
 * alloca of the largest race class size), the race takes the replay
 * (virtual_59(0, ...)), logger mode 1, and the file's info goes to entry 0.
 * LOGGER_NEXT_LOAD_OTHER runs the logger again; anything else ends it.  The
 * frame (0x41B00 bytes, $fp): fp+0x10 the replay buffer (64-byte aligned
 * inside), fp+0x41990 the load info, fp+0x41A00 the RaceReplayInformation. */
// FUNC 0x00136FC0 logger_main
void logger_main(void)
{
    u8 raw[0x41980];
    RaceSolitaire *race = NULL;
    g_00351564 = 0;
    g_00351548 = 1;
    g_00353210 = NULL;
    int raceSize = (g_00351DF0 + 0x3F) & ~0x3F;
    void *raceMem = __builtin_alloca(raceSize);
    g_00353214 = raceMem;
    bgm_stop();
    logger_reset_replay();
    race_begin(0x20000, 2);
    /* The pages' request is a switch with an explicit QUIT case (retail's
     * decision tree: == 1, < 2, == 2); anything else leaves the loop. */
    for (;;) {
        if (race == NULL)
            logger_run_without_race();
        else
            logger_run(race->org, 0);
        switch (g_00351544) {
        case LOGGER_NEXT_LOAD: {
            MemorycardLoadInfo info;
            if (F_0014AE00(MEMCARD_LOGGER_LOAD, 0, 0, LOGGER_ALIGN64(raw), NULL, 0, 0, 0,
                           &info) != 0) {
                if (race != NULL) {
                    if (race->org != NULL)
                        delete race->org;   /* virtual, flags 3 */
                    race_destroy(race, 0);
                }
                RaceReplayInformation replayInfo((RaceInfoBlock *)LOGGER_ALIGN64(raw));
                RaceOrganization *org = race_organization_create(&replayInfo);
                /* The replay's race class: a RaceSolitaire mode (virtual_59). */
                race = (RaceSolitaire *)replayInfo.createRace(raceMem, raceSize);
                org->race = race;
                race->org = org;
                race->virtual_59(0, (RaceInfoBlock *)LOGGER_ALIGN64(raw));
                org->setLoggerMode(1);
                RaceEntryCar *car = org->entries->cars[0];
                car->ghostLapTime = info.thumbnail.bestlap.time;
                car->setDriverName(info.thumbnail.comment);
                car->setDate((const RaceDate *)&info.date);
            }
            continue;
        }
        case LOGGER_NEXT_LOAD_OTHER:
            continue;
        case LOGGER_NEXT_QUIT:
            break;
        }
        break;
    }
    if (race != NULL) {
        if (race->org != NULL)
            delete race->org;               /* virtual, flags 3 */
        race_destroy(race, 0);
    }
    race_end();
    g_00351548 = 0;
}

/* Caller: gt2k_menu::LoggerTop::virtual_17 (with state = 4). */
// FUNC 0x001372A0 logger_set_next
void logger_set_next(int next)
{
    g_00351544 = next;
}

/* One table entry: the inline constructor of the original's MenuHook (u006's
 * form, as T104's and u019's static initialisers). */
static inline void set_entry(MenuHook *e, const char *name, MenuHookFn *create,
                             int p8, int pC, int p10, int p14)
{
    e->name   = name;
    e->create = create;
    e->p8     = p8;
    e->pC     = pC;
    e->p10    = p10;
    e->p14    = p14;
}

/* One element as egcs's build_vec_init expands it: the constructor on a
 * copy of the cursor (`base'), then the cursor and the count of the elements
 * left (`left', dead) stepped. */
#define HOOK(name, create, p8, pC, p10, p14)                                      { MenuHook *e = base; set_entry(e, name, create, p8, pC, p10, p14); }       base++;                                                                       left--

/* Static initialiser (.ctors entry 0x0028F064): the logger's two item
 * factories (the RawWindow factories take the hook's arguments with other
 * types: cast).  Compiler-generated in the original (g_0029DE48 defined
 * with MenuHook objects built by a constructor, filled at run time by
 * build_vec_init; .data stays zero, as above); written out the way
 * build_vec_init expands it (HOOK): an exact form, not the original; it
 * stores the same two entries. */
// FUNC 0x001372A8 T055_static_init
void T055_static_init(void)
{
    MenuHook *base = g_0029DE48;
    int left = 1;                           /* build_vec_init's iterator: elements - 1 */

    HOOK("logger_top_page", (MenuHookFn *)LoggerTop::factory,    0, 0, 0, 0);
    HOOK("replay_logger",   (MenuHookFn *)LoggerReplay::factory, 0, 0, 0, 0);
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00137308 __tfQ29gt2k_menu11LoggerInput
extern "C" void *__tfQ29gt2k_menu11LoggerInput(void)
{
    if (__tiQ29gt2k_menu11LoggerInput[0] == 0) {
        __tf9RaceInput();
        __rtti_si(__tiQ29gt2k_menu11LoggerInput, "Q29gt2k_menu11LoggerInput",
                  (const type_info *)__ti9RaceInput);
    }
    return __tiQ29gt2k_menu11LoggerInput;
}
#endif

namespace gt2k_menu {

// FUNC 0x00137358 gt2k_menu::LoggerControl::setWindow
/* inline in game/debug/T055_logger.h; egcs emits this copy (no callers) */

// FUNC 0x00137360 gt2k_menu::LoggerControl::getState
/* inline in game/debug/T055_logger.h; egcs emits this copy (no callers) */

// FUNC 0x00137368 gt2k_menu::LoggerControl::setState
/* inline in game/debug/T055_logger.h; egcs emits this copy (no callers) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00137370 __tfQ29gt2k_menu13LoggerControl
extern "C" void *__tfQ29gt2k_menu13LoggerControl(void)
{
    if (__tiQ29gt2k_menu13LoggerControl[0] == 0) {
        __tfQ29gt2k_menu11ControlBase();
        __rtti_si(__tiQ29gt2k_menu13LoggerControl, "Q29gt2k_menu13LoggerControl",
                  (const type_info *)__tiQ29gt2k_menu11ControlBase);
    }
    return __tiQ29gt2k_menu13LoggerControl;
}

/* Compiler-generated typeinfo function. */
// FUNC 0x001373C0 __tfQ29gt2k_menu10LoggerPage
extern "C" void *__tfQ29gt2k_menu10LoggerPage(void)
{
    if (__tiQ29gt2k_menu10LoggerPage[0] == 0) {
        __tfQ29gt2k_menu11PageManager();
        __rtti_si(__tiQ29gt2k_menu10LoggerPage, "Q29gt2k_menu10LoggerPage",
                  (const type_info *)__tiQ29gt2k_menu11PageManager);
    }
    return __tiQ29gt2k_menu10LoggerPage;
}
#endif

namespace gt2k_menu {

// FUNC 0x00137410 gt2k_menu::LoggerRender::setWindow
/* inline in game/debug/T055_logger.h; egcs emits this copy (no callers) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00137418 __tfQ29gt2k_menu12LoggerRender
extern "C" void *__tfQ29gt2k_menu12LoggerRender(void)
{
    if (__tiQ29gt2k_menu12LoggerRender[0] == 0) {
        __tfQ29gt2k_menu10RenderBase();
        __rtti_si(__tiQ29gt2k_menu12LoggerRender, "Q29gt2k_menu12LoggerRender",
                  (const type_info *)__tiQ29gt2k_menu10RenderBase);
    }
    return __tiQ29gt2k_menu12LoggerRender;
}
#endif
