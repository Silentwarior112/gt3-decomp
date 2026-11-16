/* T328_surface_tables.h - the rolling resistance curve and the per-surface
 * table g_002D2CE0 (translation unit T328, unit u076), 0x001DAE70..0x001DB020.
 *
 * surface_tables_init (DynamicsConductor::virtual_20, with 0) first runs the T329
 * setup (slipstream_init), then fills the speed curve g_002D2CA0 / g_002D2CC0 and
 * the 16 per-surface entries of g_002D2CE0 from g_002D28B0 (DynParams, T283).
 * Readers: CarDynamics_calcTopSpeed (T284: the curve and rollingResistance[0]),
 * T300 (u081: the resistance and the bumps by surface type). */
#ifndef GAME_T328_SURFACE_TABLES_H
#define GAME_T328_SURFACE_TABLES_H

#include "game/game.h"
#include "game/car/physics/T284_car_param.h"            /* Curve (held by value) */

/* Names: u081's.  Not polymorphic.  Size 0x10 (the curve and 4 bytes, 0
 * in the file, before the next array's 16-byte alignment).
 *
 * The per-surface arrays are objects of their own in the original (T328's
 * .data 0x002D2CF0..0x002D2DD0): surface_tables_init addresses each with its
 * own lui / addiu and keeps the loop index for the two byte arrays, where
 * members of one object share a base (CSE's related values) and the loop
 * is reversed (ref/matching/u076/notes.md).  They are static members here,
 * bound to their original names, so the units that read them as members of
 * g_002D2CE0 (u077, u081) keep their source. */
class SurfaceTable {
public:
    Curve resistCurve;                      /* 0x00 the rolling resistance over the speed (g_002D2CA0 -> */
                                            /* g_002D2CC0; u077 used g_002D2CE0 as this Curve) */
    u8    unk_0C[4];                        /* 0x0C (0 in the file) */

    static f32 rollingResistance[16] __asm__("g_002D2CF0"); /* 0x002D2CF0 by surface type, DynParams.rollingResistance / 100 */
    static f32 bumpAmplitude[16]     __asm__("g_002D2D30"); /* 0x002D2D30 by surface type (0: flat), DynParams.bumpAmplitude / 1000 */
    static f32 bumpWaveNumber[16]    __asm__("g_002D2D70"); /* 0x002D2D70 2 pi / (DynParams.bumpPeriod x 0.1 s) (u076 omega) */
    static s8  bumpSpeed[16]         __asm__("g_002D2DB0"); /* 0x002D2DB0 non-zero: the bump grows with the wheel speed (lb) */
    static s8  bumpKind[16]          __asm__("g_002D2DC0"); /* 0x002D2DC0 the bump wave kind (F_001E8AE0; lb) */
};
EGCS_SIZE(SurfaceTable, 0x10);

extern f32          g_002D2CA0[8];          /* 0x002D2CA0 the curve's speeds, m/s (DynParams.speed x 2 km/h) */
extern f32          g_002D2CC0[8];          /* 0x002D2CC0 its values (DynParams.speedValue / 1000) */
extern SurfaceTable g_002D2CE0;             /* 0x002D2CE0 */

/* The arrays by their original names (u077 f32 g_002D2CF0, u081 f32[]
 * g_002D2CF0 / g_002D2D30 / g_002D2D70 and s8[] g_002D2DB0 / g_002D2DC0). */
#define g_002D2CF0 (SurfaceTable::rollingResistance)  /* 0x002D2CF0 */
#define g_002D2D30 (SurfaceTable::bumpAmplitude)      /* 0x002D2D30 */
#define g_002D2D70 (SurfaceTable::bumpWaveNumber)     /* 0x002D2D70 */
#define g_002D2DB0 (SurfaceTable::bumpSpeed)          /* 0x002D2DB0 */
#define g_002D2DC0 (SurfaceTable::bumpKind)           /* 0x002D2DC0 */

void surface_tables_init(void *unused);              /* 0x001DAE70 the argument is not read */

#endif /* GAME_T328_SURFACE_TABLES_H */
