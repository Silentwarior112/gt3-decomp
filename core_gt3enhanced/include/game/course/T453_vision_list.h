/* T453_vision_list.h - VisionList: the course's visibility grid (translation
 * unit T453, unit u102).
 *
 * A grid of nx * nz cells over the course (x, z); each cell is a
 * NULL-terminated list of points.  A point carries the list of the model
 * parts visible from there ({n, part[n]}: u059 / u075 draw those parts of the
 * course model).  The file stores offsets relative to `base'; vision_relocate
 * turns them into pointers (always, even when base is the address).
 */
#ifndef GAME_T453_VISION_LIST_H
#define GAME_T453_VISION_LIST_H

#include "game/game.h"

/* A point of the grid, 0x10 bytes (vision_nearest_index: the byte
 * difference / 16). */
typedef struct VisionPoint {
    u16 *parts;                                 /* 0x00 the parts visible from here: {n, part[n]} */
    f32  x;                                     /* 0x04 */
    f32  y;                                     /* 0x08 */
    f32  z;                                     /* 0x0C */
} VisionPoint;
EGCS_SIZE(VisionPoint, 0x10);

typedef struct VisionList {
    u32                  magic;                 /* 0x00 */
    struct VisionList   *base;                  /* 0x04 the address the offsets are for */
    u8                   unk_08[8];             /* 0x08 */
    u16                  nx;                    /* 0x10 */
    u16                  nz;                    /* 0x12 */
    VisionPoint       ***cells;                 /* 0x14 [nz][nx] NULL-terminated point lists */
    f32                  minX;                  /* 0x18 */
    f32                  minZ;                  /* 0x1C */
    u8                   unk_20[8];             /* 0x20 */
    f32                  scaleX;                /* 0x28 1 / width */
    f32                  scaleZ;                /* 0x2C 1 / depth */
    int                  nPoints;               /* 0x30 */
    VisionPoint         *points;                /* 0x34 [nPoints] */
} VisionList;

void         vision_relocate(VisionList *v);                            /* 0x00226860 */
/* The point of the cell under (x, z) nearest to (x, y, z); the cell's
 * first point when none is nearer than FLT_MAX. */
VisionPoint *vision_nearest(VisionList *v, f32 x, f32 y, f32 z);       /* 0x00226950 */
VisionPoint *vision_point(VisionList *v, int i);                       /* 0x00226A60 &points[i] */
u32          vision_nearest_index(VisionList *v, f32 x, f32 y, f32 z); /* 0x00226A70 */

#endif /* GAME_T453_VISION_LIST_H */
