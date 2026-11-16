/* race_meters_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u056,
 * 0x0019D278..0x0019F9F8: translation units T161..T166.
 *
 *   T161  the tachometers of the race display: RaceRoundTachometerBase (a
 *         RaceRoundMeter dial plus the transmission mark printed with the
 *         "meter#1" font) and its two forms RaceOnboardTachometer ("rev_"
 *         dials) / RaceRemoteTachometer ("revb_"), and RaceBattleTachometer
 *         (a RaceBarMeter, "meter_mini").  Plus out-of-line copies of
 *         RaceMeterBase's inline members.
 *   T162  RaceOnboardSpeedmeter's constructor and its scale set-up (mph /
 *         km/h dial images), with copies of inline members.
 *   T163  RaceBoostmeter (a RaceRoundMeterBase shown only for turbo cars).
 *   T164  RaceMiniMap: the course map image with a dot for every car, and
 *         the course file's mini map record (RaceMiniMapData).
 *   T165  ParticleManager: a pool of 128 particles (sparks / debris) with
 *         simple physics (drag, gravity, one ground plane with bounce and
 *         friction), drawn as streaks between two frames' screen positions,
 *         coloured by remaining life.
 *   T166  the particle parameters g_002C03B0 (defined there) and their
 *         constructor.
 *
 * Every shared class, function and global is declared by the canonical
 * headers below.  This header keeps only what is local to the unit: the
 * dial tables of T161 / T162, T164's RaceMiniMapData (no other unit reads
 * it; T154 / T179 forward-declare it) and named constants.
 *
 * Only T161..T164 include it (u056 data pass): its race display headers
 * and <new> give their type names, "bad_alloc" and T157's "" (retail's
 * T161..T164 have them).  T165 / T166 include just the headers they need
 * (retail's have no race display class, no "bad_alloc", no .sdata).
 *
 * Offsets in comments are the ORIGINAL ones (egcs / GCC 2.9x ee layout).
 */
#ifndef U056_H
#define U056_H

#include "types.h"
#include "libc.h"                           /* strcpy, strcat, free */
#include "cxxrt.h"                          /* type_info, __rtti_si, __rtti_user */
#include <new>                              /* (the original's objects have its "bad_alloc" string) */
/* T161 / T162 / T163 read their own header here, before the race display's
 * (U056_OWN_HEADER, set by the source): their type names (the T154 merge). */
#ifdef U056_OWN_HEADER
#include U056_OWN_HEADER
#endif

/* own translation units */
#include "game/race/display/T154_race_display.h"         /* RaceMiniMap (T164 defines its methods),
                                               race_display_find_image, race_display_draw_tex; T156,
                                               T160 RaceMeterBase, T161, T162, T163 */
#include "game/race/display/T161_race_tachometer.h"      /* the tachometers */
#include "game/race/display/T162_race_onboard_speedmeter.h"  /* RaceOnboardSpeedmeter (T162 defines its methods) */
#include "game/race/display/T163_race_boostmeter.h"      /* RaceBoostmeter */

/* outside the unit */
#include "game/race/display/T156_race_display_object.h"  /* RaceDisplayObjectBase, __ti21RaceDisplayObjectBase */
#include "game/race/display/T160_race_meters.h"          /* RaceRoundMeterBase, RaceRoundMeter, RaceBarMeter, their __ti */
#include "game/race/display/T158_race_display_draw.h"    /* race_align_pos, race_draw_image_rect */
#include "game/race/core/T121_race_entry.h"           /* RaceEntryBase (rankEnd), RaceEntryCar (rankPrev, number, state) */
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization (entries, course) */
#include "game/race/core/T153_race_course.h"          /* RaceCourse (data) */
#include "game/course/T179_course_data.h"          /* CourseData (miniMap) */
#include "game/car/physics/T289_car_mode.h"             /* car_display_level, CAR_SHOW_* */
#include "game/car/physics/T302_car_state.h"            /* RaceCarState, car_view_x / car_view_z */
#include "engine/text/T496_font.h"                 /* Font::select / setColor / setPos3 / putChar, g_002F1968 */
#include "engine/io/T503_vol_thread.h"           /* file_read_blocking, vol_device_object */
#include "engine/gfx/T512_render_context.h"       /* g_002F424C (the wide screen mode), g_002F4250 (the VRAM) */
#include "engine/gfx/T513_vram_allocator.h"       /* VramAllocator::loadTex1 */
#include "engine/pgl/G001_flare.h"                /* Tex1Image, GsTexRegs, tex1_relocate, vu0_rand, the pgl_* /
                                               gfx_* / vu0_* calls, strip_batch_*, strip_draw, PGL_* */
#include "engine/media/G002_mpeg_frameq.h"          /* cosf, sinf */
#include "game/enhanced.h"                          /* ENH_HUD_* (ref/enhanced/survey/hud.md) */

/* ======================================================================== */
/* Named constants                                                          */
/* ======================================================================== */

/* The colours of the dials: METER_*_COLOR (T160; setPanelColor /
 * setScaleColor are inline there, the stores are expanded here). */

/* The transmission mark, a glyph of the "meter#1" font.  transmission is
 * RaceEntryCar::transmission (1: manual, T121; RaceDisplay::setupCarMeters
 * copies it). */
#define TACHO_MARK_AUTOMATIC    0x3E        /* transmission 0 (? an "AT" glyph) */
#define TACHO_MARK_MANUAL       0x3D        /* otherwise (? "MT") */

/* The race display's alignment values RACE_ALIGN_*: T158_race_display_draw.h. */

/* The colours of the mini map's dots (pgl_color). */
#define MINIMAP_DOT_COLOR       0x80FFFFFFu /* opaque white (also the map image) */
#define MINIMAP_DOT_FADED       0x40FFFFFFu /* half transparent: CAR_SHOW_MAP / CAR_SHOW_GHOST */

/* ======================================================================== */
/* T161 / T162: the dial tables (.data of the unit; no other unit reads    */
/* them)                                                                    */
/* ======================================================================== */

/* One step of the tachometer dial: the first entry whose maxRpm is >= the
 * car's rev limit gives the dial image (prefix + suffix) and the rpm per
 * scale mark. */
struct TachoScale {
    f32         maxRpm;                     /* 0x0 */
    f32         step;                       /* 0x4 */
    const char *suffix;                     /* 0x8 */
};
EGCS_SIZE(TachoScale, 0xC);
/* 0x0032F5B8: {6000, 500, "06"} {7000, 500, "07"} {8000, 500, "08"}
 * {9000, 500, "09"} {10000, 500, "10"} {11000, 500, "11"}
 * {12000, 500, "12"} {18000, 1000, "18"} */
extern const TachoScale g_0032F5B8[8];

/* A speedometer dial: the first entry whose maxSpeed is >= the car's top of
 * scale gives the dial image. */
struct SpeedScale {
    f32         maxSpeed;                   /* 0x0 */
    const char *image;                      /* 0x4 */
};
EGCS_SIZE(SpeedScale, 0x8);
/* 0x0032F998 (mph): {120, "speed_120mp"} {160, "speed_160mp"}
 * {200, "speed_200mp"} {FLT_MAX, "speed_200mp"} */
extern const SpeedScale g_0032F998[4];
/* 0x0032F9E8 (km/h): {180, "speed_180km"} {240, "speed_240km"}
 * {320, "speed_320km"} {FLT_MAX, "speed_320km"} */
extern const SpeedScale g_0032F9E8[4];

/* ======================================================================== */
/* T164: RaceMiniMapData                                                    */
/* ======================================================================== */

/* ? name.  The mini map record of the course file (CourseData +0x84,
 * relocated by course_data_relocate through relocate()).  Only +0x18..+0x2B are
 * read. */
struct RaceMiniMapData {
    u8         unk_00[0x18];                /* 0x00 */
    f32        originX;                     /* 0x18 the world point at the map's centre */
    f32        originZ;                     /* 0x1C */
    f32        scale;                       /* 0x20 world -> map pixels */
    f32        angle;                       /* 0x24 degrees (the map's rotation) */
    Tex1Image *image;                       /* 0x28 the map (an offset from this record until relocate) */

    void relocate();                                        /* 0x0019E230 ? name */
    void project(f32 *outX, f32 *outY, f32 x, f32 z);       /* 0x0019E258 ? name */
};

#endif /* U056_H */
