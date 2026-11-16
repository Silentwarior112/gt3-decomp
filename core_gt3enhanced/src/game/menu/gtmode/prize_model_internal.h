/* prize_model_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u103,
 * 0x00227378..0x0022AA78.
 *
 * Translation units (link order):
 *   T457  Runway (? name, as in u050 / u059): the course's road surface file
 *         (CourseData +0x70).  Sectors of road triangles, two xz cell maps
 *         over them (triangles / points), the lap lines with their distance
 *         along the lap, a marker table.  Point queries: the road height and
 *         plane under a point, the interpolated vertex values, the distance
 *         along the lap, four-wheel contacts, area walks with a callback.
 *   T458  CourseSound (? name, as in u059): the course's ambient sound file
 *         (CourseData +0x8C).  Five groups of sound sources; for a listener
 *         (position, side axis, velocity) each group gives left / right /
 *         pitch: distance attenuation (1 / (d / 32) beyond 32 m), a pan from
 *         the side axis (sin of the quarter turn) and a Doppler factor with
 *         5.524167 = 331.45 / 60, the speed of sound in m per 1/60 s.  The
 *         sources of group 3 fade with the cars on their section of the lap.
 *   T459  PrizeModel (? name, as in u019): the .prm trophy model file: two
 *         animated matrices (key frames), lights and a planar shadow per
 *         light, the embedded ModelSet (T451).
 *   T460  PrizeBase (RTTI), PrizeModelSetContext (RTTI), the prize camera.
 *   T461  Matrix interpolation: the rotation part by a quaternion (axis /
 *         angle of the relative rotation), the rest linearly.
 *
 * Canonical headers (include/game/): each file includes those it needs
 * itself, in the order that gives retail's type names (u103's data pass:
 * ref/data/u103/notes.md); this header has only the unit's local helpers:
 * the min / max / relocation helpers, the prize part numbers, T459's two
 * .rodata tables.
 */
#ifndef U103_H
#define U103_H

#include "types.h"
#include <stddef.h>

/* EE min.s / max.s as the original had them: volatile asm statements (a
 * scheduling barrier, never in a delay slot), as u018's T055 found: the
 * code before a min / max stays before it, the code after it after it
 * (runway_tri_values, runway_tri_height).  GCC 14: the C ternaries (max.s /
 * min.s differ only for -0.0 against 0.0). */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
static inline f32 u103_min(f32 a, f32 b)
{
    f32 r;
    __asm__ __volatile__("min.s %0,%1,%2" : "=f"(r) : "f"(a), "f"(b));
    return r;
}
static inline f32 u103_max(f32 a, f32 b)
{
    f32 r;
    __asm__ __volatile__("max.s %0,%1,%2" : "=f"(r) : "f"(a), "f"(b));
    return r;
}
#else
static inline f32 u103_min(f32 a, f32 b) { return a < b ? a : b; }
static inline f32 u103_max(f32 a, f32 b) { return a > b ? a : b; }
#endif

/* EE rsqrt.s fd, fs, ft: fs / sqrt(ft), a COP1 instruction egcs never
 * emits: a volatile asm, as the original's (u032's icon_rsqrt, u055,
 * u056, u057).  GCC 14: the division. */
static inline f32 u103_rsqrt(f32 num, f32 sq)
{
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    f32 r;

    __asm__ __volatile__("rsqrt.s %0, %1, %2" : "=f"(r) : "f"(num), "f"(sq));
    return r;
#else
    return num / boot_sqrtf(sq);
#endif
}

/* Relocates a pointer field of a file loaded at another address: the file
 * stores it for the address `self' says. */
template <class T> static inline void u103_reloc(T *&field, int delta)
{
    field = (T *)((u8 *)field + delta);
}

/* The same as a statement macro: a do { } while (0) block, which egcs keeps
 * as a scheduling barrier (runway_map_relocate's cell loop is exact only
 * with it). */
#define U103_RELOC(field, delta)                                              \
    do { (field) = (__typeof__(field))((u8 *)(field) + (delta)); } while (0)

/* gfx_enable / gfx_disable PGL_CAP_CPU_LIGHTING (GsContext.lightFlags 0x80)
 * and PGL_CAP_LIGHT0 + i, the pgl_lightfv / pgl_light_modelfv pnames
 * PGL_LIGHT_* / PGL_LIGHTMODEL_*: G001_flare.h. */

/* The parts of a prize's ModelSet that PrizeBase::draw draws
 * (modelset_draw_part): the trophy, and the part drawn flattened for each
 * light's shadow. */
enum {
    PRIZE_PART_MODEL  = 0,
    PRIZE_PART_SHADOW = 1                   /* (? a dedicated shadow mesh) */
};

/* T459 .rodata (defined there; external linkage, so egcs writes them at
 * their definitions, before T459's type name): the light model's ambient
 * colour {0, 0, 0, 1} and the 8 light ids PGL_CAP_LIGHT0 + 0..7 (101..108)
 * of prize_model_lights. */
extern const f32 g_003413C0[4];             /* 0x003413C0 */
extern const int g_003413D0[8];             /* 0x003413D0 */

#endif /* U103_H */
