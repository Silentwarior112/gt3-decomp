/* T457_runway.h - Runway (? name, as in u050 / u059): the road surface of the
 * course (translation unit T457, unit u103).
 *
 * The file (CourseData +0x70) is relocated in place (runway_relocate).  The
 * road is cut into sectors of triangles; two xz cell maps give, per cell, the
 * triangles (triMap) or the points (pointMap) of each sector touching it.  A
 * sector lists its neighbours and the lap lines crossing it (the distance
 * along the lap).  Queries take a sector hint: only that sector and its
 * neighbours are searched when the hint is a sector.
 *
 * The file (Runway) and its records are declared here (the plan put Runway,
 * RunwayGrid and RunwayLight in T181_car_lighting.h: u060 was the fullest
 * user; u103's names); the query records RunwayQuery and RunwayContact are
 * in T153_race_course.h (RaceCourse fills them; RunwayContact derives from
 * RunwayPos, which this C header cannot express).
 */
#ifndef GAME_T457_RUNWAY_H
#define GAME_T457_RUNWAY_H

#include "game/game.h"

struct RunwayQuery;                             /* T153_race_course.h, 0x40 bytes */
struct RunwayContact;                           /* T153_race_course.h, 0x34 bytes (4 in a row) */
struct WallSeg;                                 /* T167_rigid_body.h, 0x10 bytes: the walls (+0x3C) */
struct GadgetDesc;                              /* T167_rigid_body.h, 0x20 bytes: the objects (+0x54) */

/* A vertex of a sector, 0x10 bytes.  u and v are members of (anonymous)
 * unions: egcs gives a union member access alias set 0, so retail's stores
 * to the query records (f32) stay ahead of the later u / v loads, which a
 * plain u16 field lets the scheduler and CSE move or reuse (u103 matching:
 * runway_tri_values, runway_query_contacts; the original's exact type is not
 * known). */
typedef struct RunwayVertex {
    f32 x;                                      /* 0x00 */
    f32 y;                                      /* 0x04 */
    f32 z;                                      /* 0x08 */
    union { u16 u; };                           /* 0x0C } values interpolated over the triangle, */
    union { u16 v; };                           /* 0x0E }   65535 = 1.0 (? names) */
} RunwayVertex;
EGCS_SIZE(RunwayVertex, 0x10);

/* A road triangle, 0x10 bytes.  attr / attr2 are union members as
 * RunwayVertex's u / v (alias set 0): runway_query's attribute loads stay
 * after its stores to the query record, as retail's do. */
typedef struct RunwayTri {
    u16 v[3];                                   /* 0x00 vertex indices into the sector's verts */
    u16 unk_06;                                 /* 0x06 */
    union { u16 attr; };                        /* 0x08 (? the surface attribute) */
    u16 unk_0A;                                 /* 0x0A */
    union { u16 attr2; };                       /* 0x0C (? a second attribute) */
    u16 unk_0E;                                 /* 0x0E */
} RunwayTri;
EGCS_SIZE(RunwayTri, 0x10);

/* A lap line across the road, 0x20 bytes. */
typedef struct RunwayLine {
    f32 x0;                                     /* 0x00 one end */
    f32 z0;                                     /* 0x04 */
    f32 x1;                                     /* 0x08 the other end */
    f32 z1;                                     /* 0x0C */
    f32 dist;                                   /* 0x10 the distance along the lap */
    f32 a;                                      /* 0x14 } the line a*x + b*z + c */
    f32 b;                                      /* 0x18 }   (<= 0: past it) */
    f32 c;                                      /* 0x1C } */
} RunwayLine;
EGCS_SIZE(RunwayLine, 0x20);

/* A road sector, 0x40 bytes. */
typedef struct RunwaySector {
    u8            unk_00[4];                    /* 0x00 */
    RunwayVertex *verts;                        /* 0x04 */
    RunwayTri    *tris;                         /* 0x08 */
    u8            unk_0C[4];                    /* 0x0C */
    u16          *lines;                        /* 0x10 indices into the Runway's lines */
    u16           lineCount;                    /* 0x14 */
    u16           neighbourCount;               /* 0x16 } a u16 list (count, items): */
    u16           neighbours[20];               /* 0x18 }   the sectors next to this one */
} RunwaySector;
EGCS_SIZE(RunwaySector, 0x40);

/* ? name.  An xz cell map.  Cell (i, j) (i: x interval, j: z interval) is a
 * u16 stream of entries {n, sector, item[n]} ended by n == 0. */
typedef struct RunwayCellMap {
    u16 **cells;                                /* 0x00 (nx - 1) * (nz - 1), row j = z interval */
    s16   nx;                                   /* 0x04 the number of x bounds */
    s16   nz;                                   /* 0x06 the number of z bounds */
    f32   bounds[1];                            /* 0x08 nx x bounds (ascending), then nz z bounds */
} RunwayCellMap;

/* ? name.  A record of the marker table, 8 bytes. */
typedef struct RunwayMarker {
    u16 type;                                   /* 0x00 (2 is the one counted) */
    u16 key;                                    /* 0x02 */
    f32 value;                                  /* 0x04 */
} RunwayMarker;
EGCS_SIZE(RunwayMarker, 0x8);

/* ? name (u050).  A record of the start grid (Runway +0x2C), 0x10 bytes:
 * RaceCourse::getGridPosition reads it as one quadword (T153). */
typedef struct RunwayGrid {
    f32 x;                                      /* 0x00 */
    f32 y;                                      /* 0x04 */
    f32 z;                                      /* 0x08 */
    f32 angle;                                  /* 0x0C radians (getGridAngle adds pi) */
} RunwayGrid;
EGCS_SIZE(RunwayGrid, 0x10);

/* ? name (u060).  A light of the course along the road (Runway +0x5C),
 * 0x20 bytes; CarRenderCtx::setup (T181) weighs them. */
typedef struct RunwayLight {
    int type;                                   /* 0x00 its CourseEnv light (0..3) */
    f32 dir[3];                                 /* 0x04 the spot direction */
    f32 pos[3];                                 /* 0x10 the position */
    f32 radius;                                 /* 0x1C 0: a directional light (weight 1) */
} RunwayLight;
EGCS_SIZE(RunwayLight, 0x20);

/* Runway (? name, u050 / u059 / u103 "runway"): the header of the file
 * (CourseData +0x70).  Its size is not known (it holds at least the 0x60
 * bytes below); unknown words are left as they are. */
typedef struct Runway {
    u8             unk_00[4];                   /* 0x00 */
    struct Runway *self;                        /* 0x04 the address the pointers are for (runway_relocate) */
    u8             unk_08[0x10];                /* 0x08 */
    /* lap distances wrap at it (u050 f18, RaceCourse::getLapLength) */
    f32            lapLength;                   /* 0x18 */
    /* u050 / u085 f1C: RaceCourse::F_00190018, DynamicsConductor::virtual_30 */
    f32            unk_1C;                      /* 0x1C */
    u8             looped;                      /* 0x20 the last lap line is followed by the first */
    /* u045 envmapAlloc: then the cars' environment map gets its own VRAM */
    u8             hasLights;                   /* 0x21 the course has runway lights (u060) */
    u16            markerCount;                 /* 0x22 */
    RunwayMarker  *markers;                     /* 0x24 (u043: {u16 type; u16 key; f32 value}) */
    u8             unk_28[4];                   /* 0x28 */
    RunwayGrid    *grid;                        /* 0x2C the start grid (u050) */
    u32            sectorCount;                 /* 0x30 */
    RunwaySector  *sectors;                     /* 0x34 */
    u8             unk_38[4];                   /* 0x38 */
    /* the wall polylines (T167's WallSeg, 16 bytes: what a box corner
     * touched, saved as an index by car_box_save, u087; the gadgets'
     * walls, u057) */
    struct WallSeg *walls;                      /* 0x3C */
    int            lineCount;                   /* 0x40 */
    RunwayLine    *lines;                       /* 0x44 the lap lines */
    RunwayCellMap *triMap;                      /* 0x48 items: triangle indices of the sector */
    RunwayCellMap *pointMap;                    /* 0x4C items: indices into walls */
    u32            objectCount;                 /* 0x50 */
    struct GadgetDesc *objects;                 /* 0x54 the course's rigid bodies (u050; RaceCourse::setupRigidBodies) */
    int            lightCount;                  /* 0x58 (u103 count58) */
    RunwayLight   *lights;                      /* 0x5C (u103 data5C; relocated when lightCount != 0) */
} Runway;

/* The callbacks of the area walks. */
typedef void (*RunwayTriFunc)(const RunwayTri *t, const RunwayVertex *verts, void *ctx);
typedef void (*RunwayPointFunc)(const struct WallSeg *p, void *ctx);   /* a pointMap item: a wall vertex */

/* Cell maps and triangles */
void  runway_map_relocate(RunwayCellMap *m, int delta);                 /* 0x00227378 */
/* The interval of v in bounds[0..n): 0x80000001 below the first,
 * 0x7FFFFFFE at or above the last. */
int   runway_find_interval(const f32 *bounds, int n, f32 v);            /* 0x002273F0 */
u16  *runway_map_cell(const RunwayCellMap *m, int i, int j);            /* 0x00227478 */
u16  *runway_map_find(const RunwayCellMap *m, f32 x, f32 z);            /* 0x002274A0 NULL: outside */
int   runway_map_clamp_x(const RunwayCellMap *m, int i);                /* 0x00227558 */
int   runway_map_clamp_z(const RunwayCellMap *m, int j);                /* 0x00227578 */
int   runway_tri_contains(const RunwayTri *t, const RunwayVertex *verts, f32 x, f32 z); /* 0x00227598 */
f32   runway_tri_height(const RunwayTri *t, const RunwayVertex *verts, f32 x, f32 z);   /* 0x00227670 */
int   runway_list_contains(const u16 *list, int item);                  /* 0x00227758 list: {count, items} */
/* q->u / q->v at the query point and vertex 0's raw values (lapLength is
 * passed but not used). */
void  runway_tri_values(const RunwaySector *s, struct RunwayQuery *q, const RunwayTri *t,
                        f32 lapLength);                                 /* 0x00227790 */

/* The file */
void  runway_relocate(struct Runway *r);                                /* 0x00227980 */
/* The distance along the lap at (q->x, q->z) into q->lapDist (+0x0C, 0 when
 * not found); q->sector is the hint, not changed.  1: found. */
int   runway_lap_distance(struct Runway *r, struct RunwayQuery *q);     /* 0x00227AA0 */
int   runway_sector_lap_distance(struct Runway *r, struct RunwayQuery *q, u32 sector); /* 0x00227BA8 */
/* The road under (q->x, q->z), the highest triangle of its cell: the height
 * into q->y, the sector, attr / attr2, the vertex values, the triangle and
 * the unit plane.  0: no road (q->sector = -1, the rest unchanged). */
int   runway_query(struct Runway *r, struct RunwayQuery *q);            /* 0x00227D78 */
/* The road under 4 contacts in a row (the wheels; c[0].sector is the hint):
 * height, sector, vertex values, the unit plane, offRoad, attr / attr2. */
void  runway_query_contacts(struct Runway *r, struct RunwayContact *c); /* 0x002280C8 */
/* cb(triangle, its sector's vertices, ctx) for the triangles listed in the
 * triMap cells over (x0, z0)..(x1, z1) widened by 8 (only the hint sector
 * and its neighbours when sector >= 0); a triangle in several cells is
 * passed several times.  runway_for_each_point: the same over the pointMap. */
void  runway_for_each_tri(struct Runway *r, RunwayTriFunc cb, void *ctx, int sector,
                          f32 x0, f32 z0, f32 x1, f32 z1);              /* 0x002285B8 */
void  runway_for_each_point(struct Runway *r, RunwayPointFunc cb, void *ctx, int sector,
                            f32 x0, f32 z0, f32 x1, f32 z1);            /* 0x00228848 */
/* The markers of type 2 with this key: their number; the value of the n-th. */
int   runway_marker_count(struct Runway *r, int key);                   /* 0x00228AB0 */
f32   runway_marker_value(struct Runway *r, int n, int key);            /* 0x00228B00 */
/* 1: other is in the neighbour list of sector. */
int   runway_is_neighbour(struct Runway *r, int sector, int other);     /* 0x00228B68 */

#endif /* GAME_T457_RUNWAY_H */
