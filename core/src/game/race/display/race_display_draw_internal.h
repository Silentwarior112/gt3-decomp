/* race_display_draw_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u055,
 * 0x0019A148..0x0019D278: translation units T158, T195, T159, T160.
 *
 *   T158  helpers of the race display: textured / plain rectangles of the
 *         2D prim buffer, the position alignment (left / right / centre,
 *         shifted on the wide screen), the lap-time and speed formatters,
 *         the rank names ("1st".."12th") and their static initializer.
 *   T195  the flare sources of a course (a relocatable section of the course
 *         file): relocation and submission to FlareClass.
 *   T159  FlareClass: lens flares, light glows and road reflections of the
 *         cars' lamps, the course's flare sources and the sun
 *         (race/glare.img, race/flare.bin, race/reflection.img), on G001's
 *         flare library.  The projection uses the VU0 matrix registers
 *         vf28..vf31 (VU0 macro mode).
 *   T160  the meters of the race display: RaceMeterBase's constructor (and
 *         T160's copies of its inline methods), RaceRoundMeterBase /
 *         RaceRoundMeter (round dials: panel, scale marks prebuilt into a
 *         double-buffered strip packet, the needle drawn 8 times with a
 *         sub-pixel jitter), RaceBarMeter (an alpha-tested bar).
 *
 * Every shared class, function and global this unit uses is declared in the
 * canonical headers, which each file includes itself in the order that
 * gives the original's type names and strings (u055's data pass: the
 * original T195 has no .rodata at all, T158 and T159 none of the race
 * display's names; see each file's comment).  This header keeps only what
 * is local to the unit: the T195 records (no other unit has them; car_sound_internal.h
 * has its own copy of FlareSourceData for T179) and named constants.
 * T158 and T159 do not include it.
 *
 * Offsets in comments are the ORIGINAL ones (egcs / GCC 2.9x ee layout).
 */
#ifndef U055_H
#define U055_H

#include "types.h"
#include "game/effects/T159_flare.h"                /* FlareClass::addSource, FlareSource (the T195 records) */

/* ======================================================================== */
/* The race display's colour (T160)                                         */
/* ======================================================================== */

/* The alignment values RACE_ALIGN_* / RACE_WIDE_SHIFT(_INT): T158_race_display_draw.h. */

/* The untextured colour of the race display (GS alpha 0x80 = 1.0). */
#define RACE_COLOR_WHITE    0x80FFFFFF

/* The dials' anti-aliasing passes (T160: g_0032F568, the needle and the
 * scale drawn this many times at 1 / 8 alpha). */
#define RACE_METER_PASSES 8

/* ======================================================================== */
/* T195: the flare section of a course file                                 */
/* ======================================================================== */

/* ? name.  A group of flare sources (variable size). */
struct FlareGroup {
    void submit(FlareClass *fc);            /* 0x0019AC90 */

    int         count;                      /* 0x00 */
    FlareSource src[1];                     /* 0x04 [count] */
};

/* ? name.  The group table of the flare section (pointers relocated in
 * place). */
struct FlareGroupTable {
    void relocate(int delta);               /* 0x0019AB88 */

    int         count;                      /* 0x00 */
    FlareGroup *group[1];                   /* 0x04 [count]; NULL: none */
};

/* ? name.  The flare section of a course file (CourseData +0x74: T179's
 * course_data_relocate relocates it, course_submit_flares submits groups of it). */
struct FlareSourceData {
    void relocate();                        /* 0x0019AB38 */
    void submit(FlareClass *fc, int group); /* 0x0019ABD8 group < 0: all */

    u8               unk_00[4];             /* 0x00 */
    FlareSourceData *base;                  /* 0x04 the address the pointers were made for */
    u8               unk_08[8];             /* 0x08 */
    int              present;               /* 0x10 (? != 0: there is a table) */
    FlareGroupTable *table;                 /* 0x14 */
};

/* (T160's .rodata tables, the needle strip g_0032F250 and the sub-pixel
 * offsets g_0032F568, are defined in T160_race_meters.cpp.) */

#endif /* U055_H */
