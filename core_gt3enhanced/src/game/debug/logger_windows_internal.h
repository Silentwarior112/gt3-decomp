/* logger_windows_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u038,
 * 0x0016C260..0x0016E640, second pass; since the merge of T087 also
 * 0x0016E640..0x00171968 (T087's second part, which unit u039 had).
 *
 * Translation unit (src/game/debug/T087_logger_windows.cpp):
 *   T087  the windows of the data logger's replay / graph pages
 *         ("menu/logger.mbl", run by T055's LoggerControl; its
 *         "replay_logger" page): the helpers over the logged race,
 *         gt2k_menu::LoggerSector (the list of the lap's sectors),
 *         LoggerMapCourse (the course map), LoggerRaceReplay (the race view of
 *         the replay page), LoggerReplayControl (the start / slow / stop /
 *         camera buttons), LoggerReplayRoot (the per-car info panels
 *         "ReplayTrackA/B"), LoggerGraphMap, LoggerRaceGraph (the graph and
 *         its drawing), LoggerGraphRoot (the graph mode), LoggerRoot (the
 *         sector list and the two modes), LoggerReplay (the page's window),
 *         the registry data of the ten window classes, the inline accessors
 *         emitted out of line and every class's typeinfo function
 *
 * Every class and function is declared in its canonical header
 * (include/game/debug/T087_logger_windows.h and the headers of the classes it
 * uses); this header adds what only this unit uses: the constants of the
 * logger windows, the accessors of a car's sector records, a stick helper and
 * the typeinfo storage of the ten classes.
 */
#ifndef U038_H
#define U038_H

#include "types.h"
#include <stddef.h>

/* The include order gives retail T087's type names (0x00324A20..
 * 0x00324D28, reverse definition order): its own ten, the OperatorHolder
 * classes (T213 / T263 / T264), RaceMiniMap .. RaceDisplayBase (T154, which
 * nothing here uses: the classes retail's T087 saw), RaceCameraBase (T383),
 * UnitArena<64> (T153, emitting its name), SmartIndexT<int> (T055 / T211),
 * Operator.  T154's T157 puts the "" of RaceMessageDisplay::clear first in
 * T087's .sdata (0x00351AF8). */
#define GAME_T153_EMITS_TYPEINFO

/* outside the unit */
#include "engine/pgl/G001_flare.h"                /* gs_set_clear_color, gs_clear_buffers (PGL_CLEAR_COLOR),
                                               gfx_alloc_tail, pgl_draw_strip, pgl_draw_line_strip_2d,
                                               pgl_draw_line_3f, pack_rgba_floats_to_u32,
                                               pgl_color3f, pgl_color4fv, gfx_enable / gfx_disable
                                               (PGL_CAP_*), gfx_blend_func (PGL_BLEND_SEL),
                                               gfx_*_subpacket, gfx_offset_xy, gfx_tex_param,
                                               pgl_matrix_mode / push / pop, pgl_push_attrib / pop,
                                               pgl_ortho, gs_draw_textured_sprite_strips_f,
                                               vu0_load_identity_regs */
#include "engine/media/G002_mpeg_frameq.h"          /* logf */
#include "game/app/T002_render_thread.h"        /* GameObject (RaceBase's base) */
#include "game/debug/T055_logger.h"               /* LoggerSample, LoggerData, gt2k_menu::LoggerControl
                                               (state, render.window), logger_get_org,
                                               logger_get_length, logger_request_seek,
                                               logger_set_play / _speed, logger_is_playing,
                                               logger_request_step, logger_get_data, logger_set_root */
#include "game/race/core/T153_race_course.h"          /* RaceCourse::getLapLength (UnitArena<64>'s name) */
#include "game/camera/T383_race_camera_base.h"     /* (RaceCameraBase's name) */
#include "game/race/display/T154_race_display.h"         /* (the race displays' names; "" in .sdata) */
#include "game/debug/T088_logger_course_map.h"    /* LoggerCourseMap (draw, toMap, width, height) */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase (cars), RaceEntryCar (state) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, course, race, getLoggerSectorPosition) */
#include "game/race/core/T129_race_base.h"            /* RaceBase (org, renderEnabled, virtual_4, virtual_26) */
#include "game/race/core/T130_race_ps2_base.h"        /* RacePS2Base (setMirrorRect, drawSubView, virtual_45) */
#include "game/race/core/T174_race_mem_pool.h"        /* RaceMemPool (bufs, lastSector, courseMap) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (exitRequest, setFreeze) */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item::setText */
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl (l1 .. r2; Control's type,
                                               getButtonsOn, getAnalog) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (parent, operator new / delete) */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase::control1 */
#include "game/menu/framework/T263_raw_list.h"             /* gt2k_menu::RawListItem, RawList (getIndex, virtual_31 / 32,
                                               moveLoop, keepSubListIndex, scroll, scrollCenter) */
#include "game/menu/framework/T264_raw_window.h"           /* gt2k_menu::RawWindow, RawWindowArgs, RAW_EVENT_* */
#include "game/menu/framework/T265_raw_window_register.h"  /* gt2k_menu::RawWindowClass, RawWindowRegister */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState::speed */
#include "game/race/dynamics/T313_dc_free_run.h"          /* OneLapGhostInfo */
#include "game/race/core/T413_race_input.h"           /* RaceInput (hold, snapshots; the first pass's CarDynamics) */
#include "engine/input/T474_control.h"              /* CONTROL_START */
#include "game/common/T494_text_utils.h"           /* format_time */
#include "engine/text/T496_font.h"                 /* Font, the system font g_002F1968 */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer (copy, setRect, setField, allocVram,
                                               setDrawBuffer, setTexture) */
#include "game/data/T518_unit_settings.h"        /* UnitSettings (convSpeed, speedName), g_002F59D8 */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si, type_info */
#endif
#include <new>                              /* T087 included it: its inline bad_alloc::what() puts
                                               "bad_alloc" first in T087's .rodata (0x00323EE0);
                                               after T543: egcs's <new> declares the EH runtime its
                                               own way */

/* own translation unit, after the rest: egcs writes the type names of a
 * unit's classes in the reverse order of their definitions, and retail's
 * ten come first, right after the vtables (0x00324A20; the menu, camera and
 * page classes the other headers bring follow them).  T265 comes before it:
 * T087_logger_windows.h declares the registrars and class records extern
 * with the types forward-declared, and egcs puts an extern in small data
 * only when its size is known there (retail reaches them $gp-relative). */
#include "game/debug/T087_logger_windows.h"

/* ======================================================================== */
/* Constants of the logger windows                                          */
/* ======================================================================== */

/* The events of the logger windows after T264's RAW_EVENT_* (1..6). */
enum {
    LOGGER_EVENT_GRAPH_OPTIONS = 8,         /* Start (LoggerRoot::virtual_16): the graph's option panel */
    LOGGER_EVENT_REPLAY_PLAY   = 9,         /* the replay plays (LoggerReplayRoot::enterPlaying); (? no
                                               virtual_19(9, ..) in the image) */
    LOGGER_EVENT_MODE_L2       = 10,        /* L2: replay <-> graph */
    LOGGER_EVENT_MODE_R2       = 11         /* R2: the same */
};

/* The menu sound effects the logger windows play: T233's MENU_SE_CANCEL (the page
 * is left), MENU_SE_CURSOR (the input moved to another window, the cursor / the
 * speed scale moved), MENU_SE_REFUSED (the speed scale is at its end),
 * MENU_SE_LIST_MOVE (a replay button was pressed, an option panel opens, the
 * mode changes). */

/* The replay speeds of logger_set_speed (T055 g_0029DE28 {1, 0.5, 0.25, 0}). */
enum {
    LOGGER_SPEED_NORMAL = 0,                /* x 1 */
    LOGGER_SPEED_SLOW   = 1                 /* x 0.5 */
};

/* The input ids (RaceEntryInformation::inputId, RaceBase::virtual_26) of
 * the two logged cars: car 0 / "A" the player's, car 1 / "B" the other one
 * (logger_car_input). */
enum {
    LOGGER_INPUT_CAR_A = 0x100,
    LOGGER_INPUT_CAR_B = 1
};

/* Font::setAlign: G001 font_set_align's FONT_ALIGN_* (G001_flare.h). */

/* The analog channels (Control::getAnalog, T474's ControlChannel)
 * LoggerGraphRoot::virtual_16 reads; the PS pad types it reads them of are
 * T474's PS_PAD_ANALOG .. PS_PAD_DUALSHOCK2. */
enum {
    LOGGER_AXIS_MOVE   = CHAN_RSTICK_X,     /* right stick x: the cursor along the lap */
    LOGGER_AXIS_SCROLL = CHAN_RSTICK_Y,     /* right stick y: the graph up / down */
    LOGGER_AXIS_ZOOM   = CHAN_LSTICK_Y      /* left stick y: the zoom */
};

/* The graph's off-screen buffer (LoggerRaceGraph::virtual_12): 640 x 268,
 * with an orthographic projection whose y goes UP (pgl_ortho bottom 0, top
 * 268; the menus use bottom 448, top 0): line 0 is the bottom of the
 * window.  Its contents from the bottom (scrolled down by scrollY): car B's
 * pedal band, car A's (50 lines each, after 4 lines), 10 lines, the speed
 * graph (km/h upwards, speedHeight lines for speedMax). */
#define LOGGER_GRAPH_W       640.0f
#define LOGGER_GRAPH_LINES   268.0f
#define LOGGER_BAND_LINES    50.0f          /* a car's throttle / brake band */
#define LOGGER_BAND_MARGIN   4.0f           /* below the bands */
#define LOGGER_GRAPH_GAP     10.0f          /* between the bands and the speed graph */
#define LOGGER_BAND_GRID_MAX 100            /* drawSpeedGrid's "maxSpeed" of a band (percent) */
#define LOGGER_SPEED_GRID_OVER 25           /* the speed grid goes 25 above speedMax */

/* The longest line strip / triangle strip the graph draws at once (the
 * primitive buffers: 64 vertices = 0x30 quadwords, 64 colours = 0x10). */
enum {
    LOGGER_STRIP_MAX       = 64,
    LOGGER_STRIP_VERTS_QWC = 0x30,          /* 64 x 12-byte PrimVertex */
    LOGGER_STRIP_COLS_QWC  = 0x10,          /* 64 x u32 colour */
    LOGGER_MAX_SAMPLES     = 1000           /* drawn per curve: beyond, every stride-th sample */
};

/* The distances of the graph's grid and scale and the units: the logger
 * works in metres and km/h, the labels of the US build in miles and mph. */
#define LOGGER_MILE_64TH       25.145937f     /* 1609.34 m / 64 */
#define LOGGER_MILE_16TH       100.58375f     /* 1609.34 m / 16 */
#define LOGGER_KMH_PER_MPS     3.6f
#define LOGGER_MPH_PER_KMH     0.621372759f   /* 0x3F1F1249 */
#define LOGGER_MILES_PER_METRE 0.000621372717f /* 0x3A22E39E */
#define LOGGER_KMH_PER_MPH     1.60934f       /* 0x3FCDFEDA */
#define LOGGER_LN_10           2.30259f       /* ln 10: logf(x) / LOGGER_LN_10 = log10(x) */

/* The zoom of the graph (pixels per metre). */
#define LOGGER_ZOOM_MIN      0.4f
#define LOGGER_ZOOM_MAX      10.0f

/* LoggerGraphRoot::scale: 0 .. 2, the index of g_002AEDE0 {800, 533, 400}
 * (the speed graph's lines). */
#define LOGGER_SCALE_LAST    2

/* The blend modes of the graph (gfx_blend_func): colour = (Cs - Cd) * C + Cd
 * with C = the source alpha (0x44) or the FIX factor (0x64). */
#define LOGGER_BLEND_ALPHA   PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD)
#define LOGGER_BLEND_FIX     PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_FIX, PGL_SEL_CD)

/* The GS state virtual_12 saves around the graph's buffer (pgl_push_attrib,
 * 0x887). */
#define LOGGER_GRAPH_ATTRIBS (PGL_ATTR_DRAW_BUFFER | PGL_ATTR_DEPTH | PGL_ATTR_TARGET_B | \
                              PGL_ATTR_VIEWPORT | PGL_ATTR_SCISSOR)

/* ======================================================================== */
/* Helpers                                                                  */
/* ======================================================================== */

/* A car's sector records: its RaceInput's snapshots (+0x12C), an array of
 * OneLapGhostInfo (T313; u078 has the same accessor). */
static inline OneLapGhostInfo *logger_input_sectors(RaceInput *in)
{
    return in->snapshots;
}

/* Car `car''s record of sector i in the pool: an inline accessor in the
 * original (its arguments are fresh pseudos: retail logger_sector_time
 * multiplies sector - 1 by 0x1B00 instead of folding it into the offset, and
 * the walks put the record base first in the address sum). */
static inline OneLapGhostInfo *logger_pool_sector(RaceMemPool *pool, int car, int i)
{
    return &pool->bufs[car].sectors[i];
}

/* One stick axis of pad through the stick curve g_003416F8 (0x80 centre),
 * as -1 .. 1 (the expression the listing repeats for each axis). */
static inline f32 logger_stick(gt2k_menu::MenuControl *pad, int ch)
{
    return (f32)(g_003416F8[pad->getAnalog(ch) & 0xFF] - 0x80) / 127.5f;
}

/* The menu control of the logger windows is the LoggerControl's control1
 * (ControlBase +0, the same address): LoggerReplay's constructor hands it on
 * (virtual_14), LoggerReplay::virtual_17 reads the LoggerControl back
 * through it. */
static inline gt2k_menu::LoggerControl *logger_control_of(gt2k_menu::MenuControl *ctl)
{
    return (gt2k_menu::LoggerControl *)ctl;
}

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
/* The __si_type_info storage {name, vptr, base} the __tf functions fill
 * (GCC 14 only: egcs generates the typeinfo functions and their storage
 * itself). */
#ifndef BOOT_OLD_GCC
extern "C" {
extern u32 __tiQ29gt2k_menu19LoggerReplayControl[3]; /* 0x004B1C70 */
extern u32 __tiQ29gt2k_menu15LoggerGraphRoot[3];    /* 0x004B1C80 */
extern u32 __tiQ29gt2k_menu14LoggerGraphMap[3];     /* 0x004B1C90 */
extern u32 __tiQ29gt2k_menu12LoggerSector[3];       /* 0x004B1CA0 */
extern u32 __tiQ29gt2k_menu12LoggerReplay[3];       /* 0x004B1CB0 */
extern u32 __tiQ29gt2k_menu15LoggerMapCourse[3];    /* 0x004B1CC0 */
extern u32 __tiQ29gt2k_menu10LoggerRoot[3];         /* 0x004B1CD0 */
extern u32 __tiQ29gt2k_menu16LoggerRaceReplay[3];   /* 0x004B1CE0 */
extern u32 __tiQ29gt2k_menu16LoggerReplayRoot[3];   /* 0x004B1CF0 */
extern u32 __tiQ29gt2k_menu15LoggerRaceGraph[3];    /* 0x004B1D00 */
}
#endif /* !BOOT_OLD_GCC */

#endif /* U038_H */
