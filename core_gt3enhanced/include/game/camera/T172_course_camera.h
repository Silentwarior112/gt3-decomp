/* T172_course_camera.h - the camera file of a course: CourseCamera,
 * CourseCameraRecord, CourseCameraZone and the piecewise linear tables of
 * the records (translation unit T172, unit u058), 0x001A3EA8..0x001A45A8.
 *
 * A camera file ("cameras/start.cam", "cameras/attach.cam" of T153, the
 * replay cameras CourseData +0x88) is one relocatable block: its offsets
 * are relative to `base', the address it was built at; relocate() moves
 * them to the block's address.  It holds two tables of 0x80-byte camera
 * records sorted by their start on the course (records: the replay
 * cameras; records2: the demo cameras of T385) and a table of 0x20-byte
 * zones.  The course position wraps by whole laps (loopLength).  Users: the
 * replay cameras (DesignerCamera T398, PasslineCamera T399, AutomountCamera,
 * HeliCamera T404; u090 / u091 read the record tables through
 * interpolate_table), T153's wrappers, T382, T385 (u092 / u093's
 * CourseCameraData is CourseCamera: T385's typedef).
 */
#ifndef GAME_T172_COURSE_CAMERA_H
#define GAME_T172_COURSE_CAMERA_H

#include "game/game.h"

/* A point of an InterpTable. */
struct InterpPoint {
    f32 x;                                  /* 0x0 */
    f32 y;                                  /* 0x4 */
};
EGCS_SIZE(InterpPoint, 0x8);

/* A piecewise linear table: n points, x ascending (in the camera file). */
struct InterpTable {
    int         n;                          /* 0x0 */
    InterpPoint pts[1];                     /* 0x4 [n] */
};

/* The y of x on t: a binary search for the first point with x < pts[i].x,
 * then the segment (i - 1, i), clamped to the last segment at the top; not
 * clamped at the bottom (x < pts[0].x uses the word before the table: the
 * callers keep x in range).  Users: u090, u091. */
f32 interpolate_table(const InterpTable *t, f32 x);         /* 0x001A3EA8 */

/* ? names (u090's; u058 had ptrs[13]).  A camera record, 0x80 bytes: its
 * course range and 13 tables (pointers into the file, relocated with it). */
class CourseCameraRecord {
public:
    /* Adds delta to the 13 table pointers +0x30..+0x60. */
    void relocate(int delta);               /* 0x001A3F58 */

    u32          flags;                     /* 0x00 bits 0-1: the coordinate mode (0 course coordinates,
                                               1 the same on top of the onboard view, relative to the
                                               car; 2 the onboard update without a mount; 3 the record's
                                               onboard mount); 0xC: aims at the target; 0x10: the zoom
                                               scales with the distance; 0x400: DesignerCamera aims at
                                               car 0's own position instead of the blended target (no
                                               two-car framing, no start from the saved position;
                                               isNeedReset returns the target's `changed', getChangeLevel
                                               offers a change near the record's end) (u090) */
    f32          start;                     /* 0x04 the record's course range */
    f32          end;                       /* 0x08 */
    f32          duration;                  /* 0x0C seconds; >= 1: moves by time (u090) */
    u8           unk_10[8];                 /* 0x10 */
    int          mount;                     /* 0x18 the onboard mount (mode 3) */
    f32          effect[3];                 /* 0x1C > 0: effect bits 0x2000, 0x4000, 0x8000 */
    f32          zoomMin;                   /* 0x28 */
    f32          zoomMax;                   /* 0x2C */
    InterpTable *table30;                   /* 0x30 */
    InterpTable *zoom;                      /* 0x34 over the range */
    InterpTable *table38[2];                /* 0x38 */
    InterpTable *pos[3];                    /* 0x40 camera x, y, z (u091 x / y / z) */
    InterpTable *table4C[3];                /* 0x4C */
    InterpTable *target[3];                 /* 0x58 look-at x, y, z */
    f32          fadeOutTime;               /* 0x64 */
    u8           unk_68[0x18];              /* 0x68 */
};
EGCS_SIZE(CourseCameraRecord, 0x80);

/* A zone of the course, 0x20 bytes: a range and a flag mask. */
struct CourseCameraZone {
    /* [a, b] (either order) meets [lo, hi] and mask has a bit of m.  The
     * mask is the last parameter: zoneInRange sets $f12 / $f13 first and
     * $a1 last (egcs loads arguments in order; the registers are the same
     * in any order). */
    int overlaps(f32 lo, f32 hi, u32 m);    /* 0x001A4438 */

    f32 a;                                  /* 0x00 */
    f32 b;                                  /* 0x04 */
    u32 mask;                               /* 0x08 */
    u8  unk_0C[0x14];                       /* 0x0C */
};
EGCS_SIZE(CourseCameraZone, 0x20);

/* ? name (u050's; u092 / u093 CourseCameraData).  The head of a loaded
 * camera file.  Not polymorphic. */
class CourseCamera {
public:
    /* delta = this - base: the three tables and every record's tables
     * move by it; base = this.  Callers: RaceCourse (T153, after loading),
     * CourseData's relocation (T179). */
    void  relocate();                       /* 0x001A4008 */
    /* length x the whole laps the records cover: (int)(end / length) - 1 -
     * (int)(records[0].start / length). */
    f32   loopLength();                     /* 0x001A40D8 */
    int   loopCount();                      /* 0x001A4128 the same laps, at least 1 */
    /* pos < 0 or pos < end: pos; else pos - L * (int)(L / pos), L =
     * loopLength() (sic: L / pos). */
    f32   wrap(f32 pos);                    /* 0x001A4178 */
    /* The last record starting at or before pos; -1 before the first. */
    int   find(f32 pos);                    /* 0x001A41F0 */
    /* The records around pos (wrapped) that have started and not ended,
     * going on into the next lap: at most max of them into out; returns
     * how many (T153's collectReplayCameras). */
    int   collect(CourseCameraRecord **out, int max, f32 pos);  /* 0x001A4258 */
    /* (No callers.) */
    CourseCameraZone *findZone(f32 pos);    /* 0x001A4378 */
    /* 1 if a zone with a bit of mask meets [lo, hi] (a range past the lap
     * length is also tried as [0, hi - length]).  Caller: AutomountCamera
     * (u090).  The mask is the last parameter, as overlaps' (the prologue
     * copies $f12 / $f13 before $a1; the registers are the same). */
    int   zoneInRange(f32 lo, f32 hi, u32 mask);    /* 0x001A4490 */

    u8                  unk_00[4];          /* 0x00 */
    void               *base;               /* 0x04 the address the offsets are relative to */
    u8                  unk_08[0x10];       /* 0x08 */
    f32                 length;             /* 0x18 the lap length */
    f32                 end;                /* 0x1C */
    u8                  unk_20[4];          /* 0x20 */
    int                 count;              /* 0x24 (u092 count24) */
    CourseCameraRecord *records;            /* 0x28 sorted by start (u092 rec28, u093 positions) */
    int                 count2;             /* 0x2C (u092 count2C) */
    CourseCameraRecord *records2;           /* 0x30 the demo camera records (u092 rec30; T385) */
    u32                 zoneCount;          /* 0x34 */
    CourseCameraZone   *zones;              /* 0x38 */
};

#endif /* GAME_T172_COURSE_CAMERA_H */
