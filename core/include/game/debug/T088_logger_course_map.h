/* T088_logger_course_map.h - LoggerCourseMap, the race logger's course map,
 * (translation unit T088, unit u039), 0x00171968..0x00171CA8.
 *
 * logger_load_course_map (T055) reads "logger/<course>" into the race
 * pool's 0x20000-byte buffer (RaceMemPool::courseMap, T174) and relocates
 * it; LoggerMapCourse::virtual_12 (T087) draws it as a textured quad and
 * LoggerGraphRoot::virtual_9 (T087) places the cursor point on it with
 * toMap.  The first pass called the three from outside as F_00171968 (u018)
 * and F_001719A0(map, ...) (u038): `this' is the map. */
#ifndef GAME_T088_LOGGER_COURSE_MAP_H
#define GAME_T088_LOGGER_COURSE_MAP_H

#include "game/game.h"
#include "engine/pgl/G001_flare.h"                /* PrimUV (its users draw the map with it) */

struct Tex1Image;                           /* G001 (pointers only) */

/* (? name) The course map file as loaded: it was written with its own
 * address at +0x04 and holds a Tex1Image at the offset +0x30.  Not
 * polymorphic; only the header is declared (the picture follows). */
class LoggerCourseMap {
public:
    /* Moves the file's pointers by (where it is - where it was written):
     * base = this, tex, then tex1_relocate(tex).  Caller: T055. */
    void relocate();                        /* 0x00171968 */
    /* The picture (picture / CLUT index + 1 of its texture) as a w x h quad
     * at x, y, opaque white; the texture coordinates cover width x height of
     * the power-of-two texture.  Caller: LoggerMapCourse::virtual_12 (index:
     * the selected sector). */
    void draw(int index, f32 x, f32 y, f32 w, f32 h);   /* 0x001719A0 */
    /* World (x, z) -> map pixels from the map's centre: rotated by rotation
     * degrees, minus the origin, times scale. */
    void toMap(f32 *mapX, f32 *mapY, f32 x, f32 z);     /* 0x00171BD0 */

    u32        unk_00;                      /* 0x00 */
    void      *base;                        /* 0x04 the address the file was written at */
    u8         unk_08[0x8];                 /* 0x08 */
    u32        width;                       /* 0x10 the picture, pixels */
    u32        height;                      /* 0x14 */
    f32        originX;                     /* 0x18 subtracted from the rotated x */
    f32        originZ;                     /* 0x1C subtracted from the rotated z */
    f32        scale;                       /* 0x20 pixels per metre */
    f32        rotation;                    /* 0x24 degrees */
    u8         unk_28[0x8];                 /* 0x28 */
    Tex1Image *tex;                         /* 0x30 the picture (an offset in the file until relocate) */
};

#endif /* GAME_T088_LOGGER_COURSE_MAP_H */
