/* T453_vision_list.cpp - VisionList, the course's visibility grid (unit
 * u102).
 *
 * A grid of nx * nz cells over the course (x, z); each cell is a
 * NULL-terminated list of points (VisionPoint: the parts visible from there
 * and x, y, z).  vision_nearest finds the point of the cell under (x, z)
 * nearest to (x, y, z).
 */
#include "engine/gfx/model_set_internal.h"
#include "game/course/T453_vision_list.h"

/* Relocates in place.  Unlike the model files, cells and points are
 * relocated without a NULL test, and the delta is applied even when 0.
 * (delta + p: U102_RELOC_DP.) */
// FUNC 0x00226860 vision_relocate
void vision_relocate(VisionList *v)
{
    int nx = v->nx;
    int nz = v->nz;
    int delta = (u8 *)v - (u8 *)v->base;
    int cell, left;
    VisionPoint *pt;

    v->base = v;
    U102_RELOC_DP(v->cells, delta);
    for (cell = 0; cell < nx * nz; cell++) {
        if (v->cells[cell] != NULL) {
            VisionPoint **list;
            U102_RELOC_DP(v->cells[cell], delta);
            for (list = v->cells[cell]; *list != NULL; list++)
                U102_RELOC_DP(*list, delta);
        }
    }
    U102_RELOC_DP(v->points, delta);
    pt = v->points;
    for (left = v->nPoints; left > 0; left--) {
        U102_RELOC_DP(pt->parts, delta);
        pt++;
    }
}

/* The point of the cell under (x, z) nearest to (x, y, z) (the first point
 * of the cell if none is nearer than FLT_MAX).
 * The loop is hand-tuned with inline asm in the original: each step
 * prefetches the next point (pref, twice), sums the squares on the FPU
 * accumulator (mula.s / madda.s / madd.s, which egcs never emits) and keeps
 * the minimum with min.s; the better test is on the bits of
 * bestDist - dist (> 0 as an integer).  The asm statements are volatile
 * (barriers), so the C between them stays where it is written: retail
 * steps the list between the mula.s and the madda.s.  The next point is
 * read into its own variable at the end of the loop (retail tests it and
 * copies it to pt in the delay slot).  GCC 14 computes the same in C. */
// FUNC 0x00226950 vision_nearest
VisionPoint *vision_nearest(VisionList *v, f32 x, f32 y, f32 z)
{
    int nx = v->nx;
    int nz = v->nz;
    int ix = (int)((x - v->minX) * (f32)nx * v->scaleX);
    int iz = (int)((z - v->minZ) * (f32)nz * v->scaleZ);
    VisionPoint **list;
    VisionPoint *best, *pt;
    f32 bestDist;

    if (ix < 0)
        ix = 0;
    else if (nx - 1 < ix)
        ix = nx - 1;
    if (iz < 0)
        iz = 0;
    else if (nz - 1 < iz)
        iz = nz - 1;

    bestDist = 3.40282347e+38f;             /* [0x00352AF4] FLT_MAX */
    list = v->cells[iz * nx + ix];
    best = list[0];
    pt = best;
    if (best != NULL) for (;;) {
        union { f32 f; s32 i; } diff;
        f32 dx = pt->x - x;
        f32 dy = pt->y - y;
        f32 dz = pt->z - z;
        f32 dist;
        VisionPoint *next = list[1];        /* may be NULL (pref does not fault) */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
        __asm__ volatile("pref 0, 0x4(%0)" : : "r"(next));
        __asm__ volatile("mula.s %0, %0" : : "f"(dx));
        list++;
        __asm__ volatile("madda.s %0, %0" : : "f"(dy));
        __asm__ volatile("pref 0, 0xC(%0)" : : "r"(next));
        __asm__ volatile("madd.s %0, %1, %1" : "=f"(dist) : "f"(dz));
        diff.f = bestDist - dist;
        __asm__ volatile("min.s %0, %1, %2" : "=f"(bestDist) : "f"(bestDist), "f"(dist));
#else
#ifdef BOOT_OLD_GCC
        /* egcs has no __builtin_prefetch: the instruction itself */
        __asm__("pref 0, 0x4(%0)" : : "r"(next));
        list++;
        __asm__("pref 0, 0xC(%0)" : : "r"(next));
#else
        __builtin_prefetch((u8 *)next + 0x4);
        list++;
        __builtin_prefetch((u8 *)next + 0xC);
#endif
        dist = dx * dx + dy * dy + dz * dz;
        diff.f = bestDist - dist;
        bestDist = bestDist < dist ? bestDist : dist;
#endif
        {
            int better = diff.i > 0;
            if (better)
                best = pt;
        }
        {
            VisionPoint *next = *list;
            pt = next;
            if (next == NULL)
                break;
        }
    }
    return best;
}

// FUNC 0x00226A60 vision_point
VisionPoint *vision_point(VisionList *v, int i)
{
    return &v->points[i];
}

/* (srl: an unsigned byte difference / 16) */
// FUNC 0x00226A70 vision_nearest_index
u32 vision_nearest_index(VisionList *v, f32 x, f32 y, f32 z)
{
    VisionPoint *pt = vision_nearest(v, x, y, z);
    return (u32)((u8 *)pt - (u8 *)v->points) >> 4;
}
