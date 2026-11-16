/* T328 - the rolling resistance curve g_002D2CA0 / g_002D2CC0 and the 16
 * per-surface entries of g_002D2CE0, from DynParams g_002D28B0; first runs
 * the T329 setup.  Called by DynamicsConductor::virtual_20 (with 0). */
#include "game/car/physics/car_spec_internal.h"
#include "game/car/physics/T328_surface_tables.h"

#define KMH_TO_MPS 0.2777778f                   /* 1 / 3.6 */
#define TWO_PI     6.2831855f

/* T328's .data (0x002D2CA0..0x002D2DD0, then its frame info).  The file's
 * curve: the speeds of DynParams.speed in m/s (x 2 km/h, as
 * surface_tables_init computes them) and values that are not quite
 * DynParams.speedValue / 1000 (0.25 at the end, the code's 0.255);
 * surface_tables_init overwrites both. */
f32 g_002D2CA0[8] = {                       /* 0x002D2CA0 */
    0.0f, 1.1111112f, 5.555556f, 16.111113f, 32.222225f, 55.555557f, 85.55556f, 111.111115f
};
f32 g_002D2CC0[8] = {                       /* 0x002D2CC0 */
    0.0f, 0.0f, 0.01f, 0.015f, 0.032f, 0.072f, 0.144f, 0.25f
};
SurfaceTable g_002D2CE0 = {                 /* 0x002D2CE0 */
    { 8, 0, 0, g_002D2CA0, g_002D2CC0 },    /* resistCurve */
    { 0, 0, 0, 0 }
};
/* The per-surface arrays, zero in the file (separate objects in the
 * original, static members here: T328_surface_tables.h).  Written with an
 * initialiser: egcs writes static data members without one at the end of
 * the file's .data in the reverse of their declaration order (bumpKind
 * first); with one, at the definition.  (Their symbols, the header's asm
 * names, carry the addresses: an address comment here would name the
 * address after the member for tools/pass2_names.py.) */
f32 SurfaceTable::rollingResistance[16] = { 0 };    /* g_002D2CF0 */
f32 SurfaceTable::bumpAmplitude[16] = { 0 };        /* g_002D2D30 */
f32 SurfaceTable::bumpWaveNumber[16] = { 0 };       /* g_002D2D70 */
s8  SurfaceTable::bumpSpeed[16] = { 0 };            /* g_002D2DB0 */
s8  SurfaceTable::bumpKind[16] = { 0 };             /* g_002D2DC0 */

/* The bump amplitudes are read through a local pointer: retail addresses
 * them as &g_002D28B0.bumpAmplitude + i (a register giv loop.c leaves
 * alone) and the other tables relative to that address; read in place, all
 * five combine into one register.  The per-surface arrays are separate
 * objects (T328_surface_tables.h). */
// FUNC 0x001DAE70 surface_tables_init
void surface_tables_init(void *unused)
{
    int count, i;

    slipstream_init();
    count = g_002D28B0.speedCount;
    for (i = 0; i < count; i++) {
        g_002D2CA0[i] = (f32)(g_002D28B0.speed[i] * 2) * KMH_TO_MPS;     /* 2 km/h units */
        g_002D2CC0[i] = (f32)g_002D28B0.speedValue[i] * 0.001f;
    }
    curveSet(&g_002D2CE0.resistCurve, count, g_002D2CA0, g_002D2CC0, 0, 0);
    {
        u8 *amplitude = g_002D28B0.bumpAmplitude;

        for (i = 0; i < 16; i++) {
            g_002D2CF0[i] = (f32)g_002D28B0.rollingResistance[i] * 0.01f;
            g_002D2D30[i] = (f32)amplitude[i] * 0.001f;
            g_002D2D70[i] = TWO_PI / ((f32)g_002D28B0.bumpPeriod[i] * 0.1f);  /* 0.1 s units */
            g_002D2DB0[i] = g_002D28B0.bumpSpeed[i];
            g_002D2DC0[i] = g_002D28B0.bumpKind[i];
        }
    }
}
