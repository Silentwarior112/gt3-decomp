/* race_display_parts_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u054,
 * 0x00196910..0x0019A148: translation unit T157 (whole).
 *
 *   T157  The elements of the race head-up display, all derived from
 *         RaceDisplayObjectBase (T156): lap counter, rank, gear (7-segment
 *         font "gearr#1"), digital speedometer, odometer, time / speed
 *         values with a label, the lap time list, timed messages and the
 *         8-line event board, the VSC / TCS / brake / oil lamps, back
 *         panels, the accel / brake bars, the steering indicator, the tyre
 *         wear gauge, the car icon, the replay-mode banner; plus the faded
 *         "gt_logo_mini" watermark (RaceDisplayFader, not polymorphic).
 *         RaceDisplay (T154) holds all of them by value.
 *
 * Every shared class, function and global this unit uses is declared in the
 * canonical headers below.  This header keeps only what is local to the
 * unit: sprintf, T157's own .sdata / .rodata (the font names, the steering
 * marker), and named constants (the alignment values of the T158 helpers,
 * the colours of the display).
 *
 * Offsets in comments are the ORIGINAL ones (egcs / GCC 2.9x ee layout).
 */
#ifndef U054_H
#define U054_H

#include "types.h"
#include "libc.h"                           /* strcpy */
#ifndef BOOT_OLD_GCC                        /* egcs's <new> (T157 includes it first) conflicts with it */
#include "cxxrt.h"                          /* type_info, __rtti_si */
#endif

/* The race display's header first (it includes T157's): retail's T157
 * type names are the display header's in its order (RaceDisplayBase, the
 * parts, UnitArena<64>, RaceCameraBase, the meters, MiniMap: reversed),
 * the parts' own among them (the T154 merge). */
#include "game/race/display/T154_race_display.h"         /* race_display_find_image, RaceDisplayVertex / RaceDisplayUV */
#include "game/race/display/T157_race_display_parts.h"   /* own: the display parts, their __tf / __ti */
#include "game/race/display/T156_race_display_object.h"  /* base RaceDisplayObjectBase, __tf21 / __ti21RaceDisplayObjectBase */
#include "game/race/display/T158_race_display_draw.h"    /* race_draw_image .. race_copy_string */
#include "engine/text/T496_font.h"                 /* Font, the system font g_002F1968 */
#include "engine/text/T499_font_metrics.h"         /* text_char_width / height, text_string_width */
#include "engine/pgl/G001_flare.h"                /* Tex1Image / GsTexRegs, pgl_color*, gfx_*, pgl_*, vu0_*, PGL_* */
#include "game/car/model/T184_car_icon_maker.h"       /* CarIcon */
#include "game/data/messages/T186_msg_race_display.h"     /* g_002C43E8, the race display's words */
#include "engine/gfx/T512_render_context.h"       /* g_002F424C (RenderContext::wide), g_002F4250 (its VramAllocator) */
#include "engine/gfx/T513_vram_allocator.h"       /* VramAllocator::loadImage */
#include "game/data/T518_unit_settings.h"        /* UnitSettings, g_002F59D8 */
#include "libgraph.h"                       /* SCE_GS_PSMCT32 */

extern "C" {
int sprintf(char *buf, const char *fmt, ...);                       /* 0x0026C718 */
}

/* ------------------------------------------------------------------------ */
/* T157 .sdata / .rodata                                                    */
/* ------------------------------------------------------------------------ */
/* Defined in T157_race_display_parts.cpp, each in front of the function that
 * uses it (their place in .sdata / .rodata is retail's): the font names
 * g_00352060 "gearr#1", g_00352088 "speed", g_003520A8 / g_003520B8 "odo#1"
 * (one string), and g_0032E878, the steering wheel's centre marker: one line
 * (0, -1, 0) -> (0, -1.79, 0) in units of 24 (RaceSteeringDisplay::draw).
 * The table's extern declaration gives it external linkage: egcs then writes
 * it where it is defined (a const of internal linkage goes to the end of the
 * unit). */
struct RaceLine { f32 from[3]; f32 to[3]; };
extern const RaceLine g_0032E878[1];        /* 0x0032E878 */

/* The alignment values RACE_ALIGN_* / RACE_WIDE_SHIFT: T158_race_display_draw.h. */

/* Font::setAlign: G001 font_set_align's FONT_ALIGN_* (G001_flare.h; only
 * the horizontal bits are used here). */

/* ------------------------------------------------------------------------ */
/* Colours (GS RGBA: 0xAABBGGRR, alpha 0x80 = opaque)                       */
/* ------------------------------------------------------------------------ */
#define RACE_COLOR_TEXT     0x801880D0u     /* orange (208, 128, 24): the default text colour */
#define RACE_COLOR_RED      0x800000F0u     /* red (240, 0, 0): reverse gear, brake / oil lamps, warnings */
#define RACE_COLOR_BRIGHT_RED 0x800000FFu   /* red (255, 0, 0): the lit shift lamp */
#define RACE_COLOR_GREEN    0x802BCC49u     /* green (73, 204, 43): the VSC lamp, the event board, the throttle */
#define RACE_COLOR_YELLOW   0x8014B8FFu     /* yellow (255, 184, 20): the TCS lamp */
#define RACE_COLOR_GREY     0x80D0D0D0u     /* light grey: messages, the steering marker */
#define RACE_COLOR_DARK     0x80202020u     /* dark grey: empty gauges */
#define RACE_COLOR_WHITE    0x80FFFFFFu     /* opaque white: images unmodulated */
#define RACE_COLOR_BLACK    0x80000000u     /* opaque black: unlit lamps */
#define RACE_COLOR_ODOMETER 0x80C0C062u     /* teal (98, 192, 192) */
#define RACE_COLOR_UNLIT    0x40000000u     /* translucent black: unlit lamps, back panels */
#define RACE_COLOR_SHADOW   0x60000000u     /* the rank's drop shadow */
#define RACE_COLOR_GHOST    0x30000000u     /* the gear's unlit 7-segment cells */

/* Frames per second of the updates: timers move by 1/60 s per frame. */
#define RACE_FRAME_TIME     (1.0f / 60.0f)  /* 0x3C888889 */

#endif /* U054_H */
