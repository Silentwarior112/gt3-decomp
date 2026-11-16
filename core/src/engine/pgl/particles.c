/* particles.c - weather particles of the graphics library (G001),
 * 0x00245E58..0x002466F0.
 *
 * A box of drops (Weather, owned by RacePS2Base: init from virtual_8, update
 * from virtual_28, draw from virtual_44; weather_set from F_001A8678).  The
 * box is wrapped around a point `ahead' units in front of the eye: every
 * drop position is reduced modulo
 * the box size around that point (VU0 vdiv / vftoi0 / vitof0), projected
 * with vf24..vf27, clipped, and drawn as a 6-vertex textured strip whose
 * texture is the frame buffer itself (UV = the screen position, slightly
 * smaller than the XY footprint: a refraction look).
 */
#include "engine/pgl/G001_flare.h"   /* this translation unit (G001) */
#include "vu0asm.h"
#include "engine/pgl/pgl_effects_internal.h"

// FUNC 0x00245E58 weather_drop_init
/* Random drop: pos x, z in [-spreadXZ, spreadXZ], y in +-spreadY/2, w 1;
 * velocity x in +-velXZ, y = velY, z in +-velZ, w 0. */
void weather_drop_init(WeatherDrop *d, f32 spreadXZ, f32 spreadY, f32 velXZ,
                       f32 velY, f32 velZ)
{
    f32 v;

    d->pos[0] = lcg_rand_range(-spreadXZ, spreadXZ);
    d->pos[1] = lcg_rand_range(-spreadY * 0.5f, spreadY * 0.5f);
    d->pos[2] = lcg_rand_range(-spreadXZ, spreadXZ);
    d->pos[3] = 1.0f;
    v = lcg_rand_range(-velXZ, velXZ);
    d->vel[1] = velY;
    d->vel[0] = v;
    d->vel[2] = lcg_rand_range(-velZ, velZ);
    d->vel[3] = 0.0f;            /* sw $zero */
}

// FUNC 0x00245F30 weather_draw_quad
/* Projects (x, y, z, 1) with vf24..vf27 and clip-tests it against w
 * (vclipw; a point behind the eye, w < 0, gets its flags mirrored).  Any
 * flag set: returns 0.  Else the screen position (Q = 1/w, min vf21,
 * vf22 + p * vf23) is the centre of a 16-qword GIF packet:
 *   GIFtag A+D x2: TEXFLUSH 1, RGBAQ 0x80808080
 *   GIFtag EOP, PRIM 0x154 (tri strip, TME, ABE, FST), regs UV / XYZF2, 6 loops
 * with the XYZ corners at +-(0.58 size, 0.5 size) and +-1.16 size on x,
 * clamped on the low side to the screen edge vf2 = (centre.x - scale.x,
 * centre.y + scale.y), and the UV corners at +-(0.58 size, 0.45 size) and
 * +-0.928 size around the same point in frame-buffer coordinates, clamped
 * at 0.  (Vertex 4's y is clamped with the x edge, vf2x: as in the binary.)
 * Returns 1.
 * The egcs form is the original's: one small asm per VU0 step, the GPR
 * operands left to the compiler (retail's `addiu $v1,$v0,off' in front of
 * every sqc2, pextlw on compiler registers), the vnop / cfc2 asm not
 * volatile (the clip[3] load is scheduled into it), the packet words
 * written quadword by quadword.  97.2%: left are the registers of the
 * vertex pextlw operands (retail converts size * 0.58 into $a0 once per
 * XYZ / UV block and the other half into the output's $v1; here the
 * conversion is shared by all blocks and the output gets its own register)
 * and $a2 for z in the first pextlw.  Tried: SF operands (reloads), "0"
 * matching operands, *(u32 *)& conversions, a u32 per block, block-local
 * u128 temporaries, do { } while (0) blocks, 48 operand orders of the
 * first asm.  The GCC 14 form keeps the same instructions in larger asm
 * blocks on fixed registers. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
int weather_draw_quad(f32 x, f32 y, f32 z, f32 size)
{
    s32 clip[4] __attribute__((aligned(16)));   /* sp+0x00 the clip-space point */
    u32 clipFlags;
    u32 *p;
    f32 hx, hy, uy;
    u128 v, t;

    /* (x, y, z, 1.0) */
    __asm__ volatile("pextlw %1, %4, %2\n\tpextlw %0, %5, %3"
                     : "=r"(t), "=r"(v)
                     : "r"(u113_f2u(x)), "r"(u113_f2u(y)), "r"(u113_f2u(z)), "r"(0x3F800000));
    __asm__ volatile("pextlw %0, %1, %2" : "=r"(v) : "r"(t), "r"(v));
    __asm__ volatile("qmtc2.ni %0, $vf1" : : "r"(v));
    __asm__ volatile(
        "vmulax.xyzw  " VU0_ACC ", $vf24, $vf1x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf25, $vf1y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf26, $vf1z\n\t"
        "vmaddw.xyzw  $vf1, $vf27, $vf0w\n\t"
        ".word        0x4BE109FF");                 /* vclipw.xyzw $vf1, $vf1w */
    __asm__ volatile("sqc2 $vf1, 0(%0)" : : "r"(clip) : "memory");
    __asm__ volatile("vnop\n\tvnop\n\tvnop");
    __asm__("vnop\n\tcfc2.ni %0, $vi18" : "=r"(clipFlags));     /* the clip flags */
    if (clip[3] < 0)                            /* w < 0: flag = NOT its opposite (+x = !-x ...) */
        clipFlags = ((clipFlags >> 1) | 0x2A) ^ ((clipFlags << 1) | 0x15);
    if ((clipFlags & 0x3F) != 0)
        return 0;

    __asm__ volatile(
        "lqc2         $vf3, 0xB80(%0)\n\t"      /* GsContext.viewScale */
        "lqc2         $vf4, 0xB90(%0)\n\t"      /* GsContext.viewCenter */
        "vdiv         " VU0_Q ", $vf0w, $vf1w\n\t"
        "vmini.xyzw   $vf1, $vf1, $vf21\n\t"    /* limits */
        "vwaitq\n\t"
        "vmulq.xyzw   $vf1, $vf1, " VU0_Q "\n\t"
        "vmulaw.xyzw  " VU0_ACC ", $vf22, $vf0w\n\t"
        "vmadd.xyzw   $vf1, $vf1, $vf23"
        : : "r"(GS_CTX) : "memory");
    p = gfx_open_direct(0x10);
    p[0x0] = 2;                     /* GIFtag NLOOP 2, A+D */
    p[0x1] = 0x10000000;
    p[0x2] = 0xEE;
    p[0x3] = 0;
    p[0xC] = 0x8006;                /* GIFtag NLOOP 6, EOP, PRIM 0x154, UV + XYZF2 */
    p[0xD] = 0x20AA4000;
    p[0xE] = 0x43;
    p[0xF] = 0;
    p[0x4] = 1;                     /* TEXFLUSH */
    p[0x5] = 0;
    p[0x6] = 0x3F;
    p[0x7] = 0;
    p[0x8] = 0x80808080;            /* RGBAQ */
    p[0x9] = 0;
    p[0xA] = 1;
    p[0xB] = 0;

    /* XYZF2 of the 6 vertices */
    __asm__ volatile("vsub.x $vf2, $vf4, $vf3\n\tvadd.y $vf2, $vf4, $vf3");
    hx = size * 0.58f;
    hy = size * 0.5f;
    __asm__ volatile("pextlw %0, %1, %2" : "=r"(v) : "r"(u113_f2u(hy)), "r"(u113_f2u(hx)));
    __asm__ volatile("qmtc2.ni %0, $vf5" : : "r"(v));
    __asm__ volatile("vadd.xyzw $vf6, $vf1, $vf5\n\tvftoi4.xyzw $vf6, $vf6");
    __asm__ volatile("sqc2 $vf6, 0(%0)" : : "r"(p + 0x14) : "memory");
    __asm__ volatile("vsub.xyzw $vf6, $vf1, $vf5\n\tvmax.xy $vf6, $vf6, $vf2\n\tvftoi4.xyzw $vf6, $vf6");
    __asm__ volatile("sqc2 $vf6, 0(%0)" : : "r"(p + 0x3C) : "memory");
    hy = -hy;
    __asm__ volatile("pextlw %0, %1, %2" : "=r"(v) : "r"(u113_f2u(hy)), "r"(u113_f2u(hx)));
    __asm__ volatile("qmtc2.ni %0, $vf5" : : "r"(v));
    __asm__ volatile("vadd.xyzw $vf6, $vf1, $vf5\n\tvmaxx.y $vf6, $vf6, $vf2x\n\tvftoi4.xyzw $vf6, $vf6");
    __asm__ volatile("sqc2 $vf6, 0(%0)" : : "r"(p + 0x34) : "memory");
    __asm__ volatile("vsub.xyzw $vf6, $vf1, $vf5\n\tvmax.x $vf6, $vf6, $vf2\n\tvftoi4.xyzw $vf6, $vf6");
    __asm__ volatile("sqc2 $vf6, 0(%0)" : : "r"(p + 0x1C) : "memory");
    hy = hx + hx;
    __asm__ volatile("pextlw %0, $0, %1" : "=r"(v) : "r"(u113_f2u(hy)));
    __asm__ volatile("qmtc2.ni %0, $vf5" : : "r"(v));
    __asm__ volatile("vadd.xyzw $vf6, $vf1, $vf5\n\tvftoi4.xyzw $vf6, $vf6");
    __asm__ volatile("sqc2 $vf6, 0(%0)" : : "r"(p + 0x24) : "memory");
    __asm__ volatile("vsub.xyzw $vf6, $vf1, $vf5\n\tvmax.x $vf6, $vf6, $vf2\n\tvftoi4.xyzw $vf6, $vf6");
    __asm__ volatile("sqc2 $vf6, 0(%0)" : : "r"(p + 0x2C) : "memory");

    /* UV of the 6 vertices: the same point in frame-buffer coordinates */
    __asm__ volatile("vsub.xyzw $vf1, $vf1, $vf4\n\tvadd.x $vf1, $vf1, $vf3\n\tvsub.y $vf1, $vf1, $vf3");
    uy = size * 0.45f;
    __asm__ volatile("pextlw %0, %1, %2" : "=r"(v) : "r"(u113_f2u(uy)), "r"(u113_f2u(hx)));
    __asm__ volatile("qmtc2.ni %0, $vf5" : : "r"(v));
    __asm__ volatile("vadd.xyzw $vf6, $vf1, $vf5\n\tvftoi4.xyzw $vf6, $vf6");
    __asm__ volatile("sqc2 $vf6, 0(%0)" : : "r"(p + 0x10) : "memory");
    __asm__ volatile("vsub.xyzw $vf6, $vf1, $vf5\n\tvmaxx.xy $vf6, $vf6, $vf0x\n\tvftoi4.xyzw $vf6, $vf6");
    __asm__ volatile("sqc2 $vf6, 0(%0)" : : "r"(p + 0x38) : "memory");
    uy = -uy;
    __asm__ volatile("pextlw %0, %1, %2" : "=r"(v) : "r"(u113_f2u(uy)), "r"(u113_f2u(hx)));
    __asm__ volatile("qmtc2.ni %0, $vf5" : : "r"(v));
    __asm__ volatile("vadd.xyzw $vf6, $vf1, $vf5\n\tvmaxx.y $vf6, $vf6, $vf0x\n\tvftoi4.xyzw $vf6, $vf6");
    __asm__ volatile("sqc2 $vf6, 0(%0)" : : "r"(p + 0x30) : "memory");
    __asm__ volatile("vsub.xyzw $vf6, $vf1, $vf5\n\tvmaxx.x $vf6, $vf6, $vf0x\n\tvftoi4.xyzw $vf6, $vf6");
    __asm__ volatile("sqc2 $vf6, 0(%0)" : : "r"(p + 0x18) : "memory");
    hx = hx * 1.6f;
    __asm__ volatile("qmtc2.ni %0, $vf5" : : "r"(u113_f2u(hx)));     /* (1.6 hx, 0, ..): no pextlw */
    __asm__ volatile("vadd.xyzw $vf6, $vf1, $vf5\n\tvftoi4.xyzw $vf6, $vf6");
    __asm__ volatile("sqc2 $vf6, 0(%0)" : : "r"(p + 0x20) : "memory");
    __asm__ volatile("vsub.xyzw $vf6, $vf1, $vf5\n\tvmaxx.x $vf6, $vf6, $vf0x\n\tvftoi4.xyzw $vf6, $vf6");
    __asm__ volatile("sqc2 $vf6, 0(%0)" : : "r"(p + 0x28) : "memory");
    return 1;
}
#else
int weather_draw_quad(f32 x, f32 y, f32 z, f32 size)
{
    s32 clip[4] __attribute__((aligned(16)));   /* sp+0x00 the clip-space point */
    u32 clipFlags;
    u32 flags;
    u32 *p;
    f32 hx, hy, uy;

    __asm__ volatile(
        "pextlw       $3, %3, %1\n\t"           /* (x, z) */
        "pextlw       $2, %4, %2\n\t"           /* (y, 1.0) */
        "pextlw       $3, $2, $3\n\t"           /* (x, y, z, 1.0) */
        "qmtc2.ni     $3, $vf1\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf24, $vf1x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf25, $vf1y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf26, $vf1z\n\t"
        "vmaddw.xyzw  $vf1, $vf27, $vf0w\n\t"
        ".word        0x4BE109FF\n\t"           /* vclipw.xyzw $vf1, $vf1w */
        "sqc2         $vf1, 0(%5)\n\t"
        "vnop\n\t"
        "vnop\n\t"
        "vnop\n\t"
        "vnop\n\t"
        "cfc2.ni      %0, $vi18\n\t"           /* the clip flags */
        : "=r"(clipFlags)
        : "r"(u113_f2u(x)), "r"(u113_f2u(y)), "r"(u113_f2u(z)),
          "r"(0x3F800000), "r"(clip)
        : "$2", "$3", "memory");
    flags = clipFlags & 0x3F;
    if (clip[3] < 0)                            /* w < 0: flag = NOT its opposite (+x = !-x ...) */
        flags = (((clipFlags >> 1) | 0x2A) ^ ((clipFlags << 1) | 0x15)) & 0x3F;
    if (flags != 0)
        return 0;

    __asm__ volatile(
        "lqc2         $vf3, 0xB80(%0)\n\t"      /* GsContext.viewScale */
        "lqc2         $vf4, 0xB90(%0)\n\t"      /* GsContext.viewCenter */
        "vdiv         " VU0_Q ", $vf0w, $vf1w\n\t"
        "vmini.xyzw   $vf1, $vf1, $vf21\n\t"    /* limits */
        "vwaitq\n\t"
        "vmulq.xyzw   $vf1, $vf1, " VU0_Q "\n\t"
        "vmulaw.xyzw  " VU0_ACC ", $vf22, $vf0w\n\t"
        "vmadd.xyzw   $vf1, $vf1, $vf23\n\t"
        : : "r"(GS_CTX) : "memory");
    p = gfx_open_direct(0x10);
    p[0x0] = 2;                     /* GIFtag NLOOP 2, A+D */
    p[0x1] = 0x10000000;
    p[0x2] = 0xEE;
    p[0xC] = 0x8006;                /* GIFtag NLOOP 6, EOP, PRIM 0x154, UV + XYZF2 */
    p[0xD] = 0x20AA4000;
    p[0xE] = 0x43;
    p[0x6] = 0x3F;                  /* TEXFLUSH */
    p[0x8] = 0x80808080;            /* RGBAQ */
    p[0xA] = 1;                     /* (RGBAQ register number) */
    p[0x3] = 0;
    p[0xF] = 0;
    p[0x4] = 1;                     /* TEXFLUSH data */
    p[0x5] = 0;
    p[0x7] = 0;
    p[0x9] = 0;
    p[0xB] = 0;

    /* XYZF2 of the 6 vertices */
    hx = size * 0.58f;
    hy = size * 0.5f;
    __asm__ volatile(
        "vsub.x       $vf2, $vf4, $vf3\n\t"
        "vadd.y       $vf2, $vf4, $vf3\n\t"
        "pextlw       $3, %1, %0\n\t"           /* (hx, hy) */
        "qmtc2.ni     $3, $vf5\n\t"
        "vadd.xyzw    $vf6, $vf1, $vf5\n\t"
        "vftoi4.xyzw  $vf6, $vf6\n\t"
        "sqc2         $vf6, 0x50(%2)\n\t"
        "vsub.xyzw    $vf6, $vf1, $vf5\n\t"
        "vmax.xy      $vf6, $vf6, $vf2\n\t"
        "vftoi4.xyzw  $vf6, $vf6\n\t"
        "sqc2         $vf6, 0xF0(%2)\n\t"
        : : "r"(u113_f2u(hx)), "r"(u113_f2u(hy)), "r"(p) : "$3", "memory");
    hy = -hy;
    __asm__ volatile(
        "pextlw       $3, %1, %0\n\t"           /* (hx, -hy) */
        "qmtc2.ni     $3, $vf5\n\t"
        "vadd.xyzw    $vf6, $vf1, $vf5\n\t"
        "vmaxx.y      $vf6, $vf6, $vf2x\n\t"
        "vftoi4.xyzw  $vf6, $vf6\n\t"
        "sqc2         $vf6, 0xD0(%2)\n\t"
        "vsub.xyzw    $vf6, $vf1, $vf5\n\t"
        "vmax.x       $vf6, $vf6, $vf2\n\t"
        "vftoi4.xyzw  $vf6, $vf6\n\t"
        "sqc2         $vf6, 0x70(%2)\n\t"
        : : "r"(u113_f2u(hx)), "r"(u113_f2u(hy)), "r"(p) : "$3", "memory");
    hy = hx + hx;
    __asm__ volatile(
        "pextlw       $3, $0, %0\n\t"           /* (2 hx, 0) */
        "qmtc2.ni     $3, $vf5\n\t"
        "vadd.xyzw    $vf6, $vf1, $vf5\n\t"
        "vftoi4.xyzw  $vf6, $vf6\n\t"
        "sqc2         $vf6, 0x90(%1)\n\t"
        "vsub.xyzw    $vf6, $vf1, $vf5\n\t"
        "vmax.x       $vf6, $vf6, $vf2\n\t"
        "vftoi4.xyzw  $vf6, $vf6\n\t"
        "sqc2         $vf6, 0xB0(%1)\n\t"
        : : "r"(u113_f2u(hy)), "r"(p) : "$3", "memory");

    /* UV of the 6 vertices: the same point in frame-buffer coordinates */
    uy = size * 0.45f;
    __asm__ volatile(
        "vsub.xyzw    $vf1, $vf1, $vf4\n\t"
        "vadd.x       $vf1, $vf1, $vf3\n\t"
        "vsub.y       $vf1, $vf1, $vf3\n\t"
        "pextlw       $3, %1, %0\n\t"           /* (hx, uy) */
        "qmtc2.ni     $3, $vf5\n\t"
        "vadd.xyzw    $vf6, $vf1, $vf5\n\t"
        "vftoi4.xyzw  $vf6, $vf6\n\t"
        "sqc2         $vf6, 0x40(%2)\n\t"
        "vsub.xyzw    $vf6, $vf1, $vf5\n\t"
        "vmaxx.xy     $vf6, $vf6, $vf0x\n\t"
        "vftoi4.xyzw  $vf6, $vf6\n\t"
        "sqc2         $vf6, 0xE0(%2)\n\t"
        : : "r"(u113_f2u(hx)), "r"(u113_f2u(uy)), "r"(p) : "$3", "memory");
    uy = -uy;
    __asm__ volatile(
        "pextlw       $3, %1, %0\n\t"           /* (hx, -uy) */
        "qmtc2.ni     $3, $vf5\n\t"
        "vadd.xyzw    $vf6, $vf1, $vf5\n\t"
        "vmaxx.y      $vf6, $vf6, $vf0x\n\t"
        "vftoi4.xyzw  $vf6, $vf6\n\t"
        "sqc2         $vf6, 0xC0(%2)\n\t"
        "vsub.xyzw    $vf6, $vf1, $vf5\n\t"
        "vmaxx.x      $vf6, $vf6, $vf0x\n\t"
        "vftoi4.xyzw  $vf6, $vf6\n\t"
        "sqc2         $vf6, 0x60(%2)\n\t"
        : : "r"(u113_f2u(hx)), "r"(u113_f2u(uy)), "r"(p) : "$3", "memory");
    hx = hx * 1.6f;
    __asm__ volatile(
        "qmtc2.ni     %0, $vf5\n\t"             /* (1.6 hx, 0, ..): no pextlw */
        "vadd.xyzw    $vf6, $vf1, $vf5\n\t"
        "vftoi4.xyzw  $vf6, $vf6\n\t"
        "sqc2         $vf6, 0x80(%1)\n\t"
        "vsub.xyzw    $vf6, $vf1, $vf5\n\t"
        "vmaxx.x      $vf6, $vf6, $vf0x\n\t"
        "vftoi4.xyzw  $vf6, $vf6\n\t"
        "sqc2         $vf6, 0xA0(%1)\n\t"
        : : "r"(u113_f2u(hx)), "r"(p) : "memory");
    return 1;
}
#endif

// FUNC 0x002461D8 weather_init
/* Defaults (size 8, box 32 x 8, ahead 16, velocity +-0.5 / 0.01 / +-0.5)
 * and a random start for every drop (last to first). */
void weather_init(Weather *w, WeatherDrop *drops, int count)
{
    int i;

    /* in field order (the scheduler gives retail's store order) */
    w->center[0] = 0.0f;            /* sw $zero */
    w->center[1] = 0.0f;            /* sw $zero */
    w->center[2] = 0.0f;            /* sw $zero */
    w->center[3] = 0.0f;            /* sw $zero */
    w->size = 8.0f;
    w->spreadXZ = 32.0f;
    w->spreadY = 8.0f;
    w->ahead = 16.0f;
    w->velXZ = 0.5f;
    w->velY = 0.01f;
    w->velZ = 0.5f;
    w->drops = drops;
    w->count = count;
    for (i = count - 1; i >= 0; i--)
        weather_drop_init(&w->drops[i], w->spreadXZ, w->spreadY, w->velXZ,
                          w->velY, w->velZ);
}

// FUNC 0x00246290 weather_set
/* New parameters (velZ = velXZ) and a fresh random start for every drop. */
void weather_set(Weather *w, f32 size, f32 spreadXZ, f32 spreadY, f32 ahead,
                 f32 velXZ, f32 velY)
{
    int i;

    /* in field order, as weather_init */
    w->center[0] = 0.0f;            /* sw $zero */
    w->center[1] = 0.0f;            /* sw $zero */
    w->center[2] = 0.0f;            /* sw $zero */
    w->center[3] = 0.0f;            /* sw $zero */
    w->size = size;
    w->spreadXZ = spreadXZ;
    w->spreadY = spreadY;
    w->ahead = ahead;
    w->velXZ = velXZ;
    w->velY = velY;
    w->velZ = velXZ;
    for (i = w->count - 1; i >= 0; i--)
        weather_drop_init(&w->drops[i], w->spreadXZ, w->spreadY, w->velXZ,
                          w->velY, w->velZ);
}

// FUNC 0x00246320 weather_update
void weather_update(Weather *w)
{
    int i;

    for (i = w->count - 1; i >= 0; i--) {
        w->drops[i].pos[0] += w->drops[i].vel[0];   /* indexed in place: addu idx, base */
        w->drops[i].pos[1] += w->drops[i].vel[1];
        w->drops[i].pos[2] += w->drops[i].vel[2];
    }
}

// FUNC 0x00246380 weather_draw
/* centre = inverse(current matrix) * (0, 0, -ahead, 1): the point `ahead'
 * in front of the eye (vu0_invert_rigid inside a push / pop); no depth
 * writes; texture = the draw buffer (tgtA, no mipmaps, region-clamped to
 * its size); vf24..vf27 = the projection matrix (the current matrix
 * vf28..vf31 when matrixMode is the projection, else its parked copy at VU0
 * qword 8); period = 2 spreadXZ / m00, drop size = size * sqrt(m00) (m00 =
 * the matrix's x scale); a 50 % blend (PGL_BLEND_FIX, 0.5); a GIF packet
 * setting RGBAQ; then every drop, last first; depth writes back on and the
 * texture state reset.
 * Retail loads the context pointer twice (lui / ori again for matrixMode:
 * GS_SPR_CTX, an asm egcs cannot fold), pads the else label of the matrix
 * choice to 8 bytes (the if / else sits in a do { } while (0): a statement
 * macro), and has the sqrt.s with no errno check in its source position
 * between the period and the size products (a volatile sqrt.s asm, as
 * u113_sqrtf). */
void weather_draw(Weather *w, u32 addr)
{
    f32 eye[4] __attribute__((aligned(16)));   /* sp+0x00 */
    GsContext *ctx = GS_SPR_CTX;
    f32 period, size, m00;
    u32 *p;
    int i;

    eye[3] = 1.0f;
    eye[0] = 0.0f;            /* sw $zero */
    eye[1] = 0.0f;            /* sw $zero */
    eye[2] = -w->ahead;
    pgl_push_matrix();
    vu0_invert_rigid();
    vu0_apply_matrix(w->center, eye);
    pgl_pop_matrix();
    gfx_depth_mask(0);
    gfx_reset_texture_state_spr();
    gfx_set_texture_buffer(0, ctx->tgtA.base << 6, ctx->tgtA.psm, ctx->tgtA.width);
    gfx_set_texture_size(ctx->tgtA.width, ctx->tgtA.height);
    gfx_tex_param(PGL_TEXPARAM_MXL, 0);
    gfx_tex_param(PGL_TEXPARAM_WMS, PGL_WRAP_REGION_CLAMP);
    gfx_tex_param(PGL_TEXPARAM_WMT, PGL_WRAP_REGION_CLAMP);
    gfx_set_texture_region(0, 0, ctx->tgtA.width);      /* u */
    gfx_set_texture_region(1, 0, ctx->tgtA.height);     /* v */
    gs_flush_dirty_regs();

    size = w->size;
    do {
        if (GS_SPR_CTX->matrixMode == PGL_MTX_PROJECTION) {
            __asm__ volatile(
                "vmove.xyzw   $vf24, $vf28\n\t"
                "vmove.xyzw   $vf25, $vf29\n\t"
                "vmove.xyzw   $vf26, $vf30\n\t"
                "vmove.xyzw   $vf27, $vf31\n\t");
        } else {
            __asm__ volatile(
                "ctc2.i       %0, $vi10\n\t"
                "vlqi.xyzw    $vf24, ($vi10++)\n\t"
                "vlqi.xyzw    $vf25, ($vi10++)\n\t"
                "vlqi.xyzw    $vf26, ($vi10++)\n\t"
                "vlqi.xyzw    $vf27, ($vi10++)\n\t"
                "vnop\n\t"
                "vnop\n\t"
                : : "r"(PGL_VU0_PARKED(PGL_MTX_PROJECTION)));
        }
    } while (0);
    __asm__ volatile("qmfc2.ni %0, $vf24" : "=r"(m00));
    period = (1.0f / m00) * (w->spreadXZ + w->spreadXZ);
    size = size * u113_sqrtf(m00);
    gfx_blend_func(PGL_BLEND_FIX, 0.5f);
    gs_flush_dirty_regs();
    p = gfx_open_direct(2);
    p[0] = 0x8001;                  /* GIFtag NLOOP 1, EOP, A+D */
    p[1] = 0x10000000;
    p[2] = 0xE;
    p[3] = 0;
    p += 4;
    p[0] = 0x80808080;              /* RGBAQ = 0x0000000180808080 */
    p[1] = 1;
    p[2] = 1;
    p[3] = 0;
    vu0_load_screen_regs();
    __asm__ volatile("vnop");
    for (i = w->count - 1; i >= 0; i--)
        weather_draw_drop(&w->drops[i], w, period, w->spreadY, size);
    gfx_depth_mask(1);
    gfx_reset_texture_state_spr();
}

/* VU0 wrap of d into [0, period): d - (f32)(int)(d / period) * period
 * (vdiv, vmulq, vftoi0 / vitof0, vmul, vsub), + period when d < 0, read
 * back from vf3. */
static inline f32 u113_wrap(f32 d, f32 period)
{
    f32 r;

    /* one asm per transfer: retail moves each float through $v0 right in
     * front of its qmtc2 */
    __asm__ volatile("qmtc2.ni     %0, $vf2" : : "r"(period));
    __asm__ volatile("vdiv         " VU0_Q ", $vf0w, $vf2x");
    __asm__ volatile("qmtc2.ni     %0, $vf1" : : "r"(d));
    __asm__ volatile(
        "vwaitq\n\t"
        "vmulq.xyzw   $vf3, $vf1, " VU0_Q "\n\t"
        "vftoi0.xyzw  $vf3, $vf3\n\t"
        "vitof0.xyzw  $vf3, $vf3\n\t"
        "vmul.xyzw    $vf3, $vf3, $vf2\n\t"
        "vsub.xyzw    $vf3, $vf1, $vf3\n\t");
    if (d < 0.0f)
        __asm__ volatile("vadd.xyzw $vf3, $vf3, $vf2");
    __asm__ volatile("qmfc2.ni %0, $vf3" : "=r"(r));
    return r;
}

// FUNC 0x002465B8 weather_draw_drop
/* The drop's position wrapped into the box around w->center: x and z
 * modulo `period`, y modulo min(periodY, period), each centred (- half the
 * period); then weather_draw_quad (a tail jump: void here; retail's j
 * leaves weather_draw_quad's result in $v0, which the only caller,
 * weather_draw, ignores).
 * 83%: the code and its order are retail's; left are FPU registers (retail
 * keeps the difference apart from the loaded position, $f1 / $f0, and the
 * centre in $f2; here the difference reuses the position's register).
 * Tried: the wrap open-coded with named differences, a pointer result,
 * the centre in locals, seven statement orders of the half / centring. */
void weather_draw_drop(WeatherDrop *d, Weather *w, f32 period, f32 periodY,
                       f32 size)
{
    f32 x, y, z;
    f32 half;
    f32 py;

    x = w->center[0] + u113_wrap(d->pos[0] - w->center[0], period);
    half = period * 0.5f;
    x = x - half;
    z = w->center[2] + u113_wrap(d->pos[2] - w->center[2], period);
    z = z - half;
    py = period;
    if (periodY < period)
        py = periodY;
    y = w->center[1] + u113_wrap(d->pos[1] - w->center[1], py) - py * 0.5f;
    weather_draw_quad(x, y, z, size);
}
