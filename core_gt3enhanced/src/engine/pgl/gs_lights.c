/* gs_lights.c - OpenGL-like material / light setters of the
 * graphics library (G001), 0x0025ABD0..0x0025B0C8, and the 4x4 matrix copy
 * at its end.
 *
 * gs_store_material_vec / gs_set_material_param are the same machine code
 * as the bootstrap's (gs_state.cpp, addresses in the comments).  The two
 * light setters (caller pgl_lightfv) mirror glLightf / glLightfv with the
 * library's GL-like pnames (PGL_LIGHT_*; the material kinds use the same
 * numbering); light vectors go through the VU0 modelview (inline asm: VU0
 * macro code).
 */
#include "engine/pgl/G001_flare.h"
#include "vu0asm.h"
#include "engine/pgl/gs_driver_internal.h"

/* GsLight.spotCutoff of a light that is not a spot light; the shininess /
 * spot exponent range. */
#define PGL_SPOT_CUTOFF_NONE 180.0f
#define PGL_EXPONENT_MAX     128.0f

// FUNC 0x0025ABD0 gs_store_material_vec
// Stores a 4-float vector (one 128-bit register in the binary, written with
// sq) into material[face]: kind 0 ambient, 1 diffuse, 2 diffuse and
// ambient, 3 emission (+0x30), 4 specular (+0x20) (jump table @0x00345760;
// kind >= 5 stores nothing); always lightFlags |= 4.  (bootstrap 0x0100EF50)
void gs_store_material_vec(GsContext *ctx, int face, u32 kind, GsVec4Arg v)
{
    switch (kind) {
    case PGL_MAT_AMBIENT:
        *(GsVec4Arg *)&ctx->material[face].ambient = v;
        break;
    case PGL_MAT_DIFFUSE:
        *(GsVec4Arg *)&ctx->material[face].diffuse = v;
        break;
    case PGL_MAT_AMBIENT_AND_DIFFUSE:   /* (emitted diffuse first, ambient in the delay slot) */
        *(GsVec4Arg *)&ctx->material[face].ambient = v;
        *(GsVec4Arg *)&ctx->material[face].diffuse = v;
        break;
    case PGL_MAT_SPECULAR:
        *(GsVec4Arg *)&ctx->material[face].specular = v;
        break;
    case PGL_MAT_EMISSION:
        *(GsVec4Arg *)&ctx->material[face].emission = v;
        break;
    }
    ctx->lightFlags |= PGL_LF_MATERIAL;
}

// FUNC 0x0025AC70 gs_set_material_param
// glMaterial-like setter.  kind 11 (shininess): value (unsigned) -> float
// clamped to [0, 128], stored with its encoding as shininess of face 0
// (faceMask bit 0) and/or face 1 (bit 1), the pow table pointer cleared
// (x44.powTable: float_pow_encoded is used until a table is set), lightFlags
// |= 4.  Other kinds: the RGBA bytes of value -> {r,g,b,a} / 255 (VU0:
// pextlb/pextlh, qmtc2, vitof0, vmulx by 0x3B808081 = 1/255, qmfc2 - VU
// float rounding may differ in the last bit from the C) stored via
// gs_store_material_vec.  Quirk: both the bit-0 and the bit-1 call pass face
// 0, so face 1 never gets vectors.  Callers pgl_shade_vertices (the colour
// material of the CPU lighting) and pgl_color's tail call (with
// colorMaterial).  (bootstrap 0x0100EFF0)
void gs_set_material_param(int faceMask, int kind, u32 value)
{
    GsContext *ctx = GS_SPR_CTX;
    GsVec4Arg v;

    if (kind == PGL_MAT_SHININESS) {
        f32 raw = (f32)value;                               /* u32 -> float */
        f32 shininess = ee_minf(ee_maxf(raw, 0.0f), PGL_EXPONENT_MAX);
        u32 enc = float_to_gs_param_encode(shininess);
        if (faceMask & PGL_FACE_FRONT) {
            ctx->material[0].shininess = shininess;
            ctx->material[0].x44.powTable = NULL;
            ctx->material[0].shininessEnc = enc;
        }
        if (faceMask & PGL_FACE_BACK) {
            ctx->material[1].shininess = shininess;
            ctx->material[1].shininessEnc = enc;
            ctx->material[1].x44.powTable = NULL;
        }
        ctx->lightFlags |= PGL_LF_MATERIAL;
        return;
    }

#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    __asm__ __volatile__(
        "pextlb     %0, $0, %1\n\t"
        "pextlh     %0, $0, %0\n\t"
        "vnop\n\t"
        "qmtc2.ni   %0, $vf1\n\t"
        "vitof0.xyzw $vf1, $vf1\n\t"
        "lui        %0, 0x3B80\n\t"
        "ori        %0, %0, 0x8081\n\t"
        "qmtc2.ni   %0, $vf2\n\t"
        "vmulx.xyzw $vf1, $vf1, $vf2x\n\t"
        "qmfc2.ni   %0, $vf1"
        : "=&r"(v) : "r"(value));
#else
    {
        const f32 k = U118_F_1_255;
        v.x = (f32)(s32)(value & 0xFF) * k;
        v.y = (f32)(s32)((value >> 8) & 0xFF) * k;
        v.z = (f32)(s32)((value >> 16) & 0xFF) * k;
        v.w = (f32)(s32)((value >> 24) & 0xFF) * k;
    }
#endif
    if (faceMask & PGL_FACE_FRONT)
        gs_store_material_vec(ctx, 0, (u32)kind, v);
    if (faceMask & PGL_FACE_BACK)
        gs_store_material_vec(ctx, 0, (u32)kind, v);        /* face 0 again (quirk) */
}

// FUNC 0x0025ADB0 gs_set_light_param_f
/* glLightf mirror (jump table @0x00345780 for pname 6..10; others ignored;
 * ctx is not read, and unlike the vector setter no lightFlags change):
 *   PGL_LIGHT_SPOT_EXPONENT  clamped to [0, 128], with its encoding
 *                            (spotExpEnc); the pow table pointer cleared;
 *   PGL_LIGHT_SPOT_CUTOFF    unless it is 180.0 (no spot light),
 *                            spotCosCutoff = pgl_cos_deg(v);
 *   PGL_LIGHT_CONST_ATTEN / _LINEAR_ATTEN / _QUAD_ATTEN. */
void gs_set_light_param_f(GsContext *ctx, GsLight *l, int pname, f32 v)
{
    switch (pname) {
    case PGL_LIGHT_SPOT_EXPONENT: {
        f32 exponent = ee_minf(ee_maxf(v, 0.0f), PGL_EXPONENT_MAX);

        l->spotTable = NULL;
        l->spotExponent = exponent;
        l->spotExpEnc = float_to_gs_param_encode(exponent);
        break;
    }
    case PGL_LIGHT_SPOT_CUTOFF:
        l->spotCutoff = v;
        if (v != PGL_SPOT_CUTOFF_NONE)
            l->spotCosCutoff = pgl_cos_deg(v);
        break;
    case PGL_LIGHT_CONST_ATTEN:
        l->constAtten = v;
        break;
    case PGL_LIGHT_LINEAR_ATTEN:
        l->linearAtten = v;
        break;
    case PGL_LIGHT_QUAD_ATTEN:
        l->quadAtten = v;
        break;
    }
}

/* vf2 = *v (vnop; lq; qmtc2.ni). */
static inline void u118_vf2_load(const GsVec4 *v)
{
    __asm__ __volatile__(
        "vnop\n\t"
        "lq          $2, 0(%0)\n\t"
        "qmtc2.ni    $2, $vf2"
        : : "r"(v) : "$2", "memory");
}

/* vf2 *= 1 / |vf2.xyz| (w scaled as well). */
static inline void u118_vf2_normalize(void)
{
    __asm__ __volatile__(
        "vmul.xyzw   $vf1, $vf2, $vf2\n\t"
        "vaddy.x     $vf1, $vf1, $vf1y\n\t"
        "vaddz.x     $vf1, $vf1, $vf1z\n\t"
        "vrsqrt      " VU0_Q ", $vf0w, $vf1x\n\t"
        "vwaitq\n\t"
        "vmulq.xyzw  $vf2, $vf2, " VU0_Q
        : : : "memory");
}

/* vf20..vf23 = the modelview: the current matrix vf28..vf31 when it is the
 * current mode (matrixMode PGL_MTX_MODELVIEW), else where it is parked, VU0
 * data memory qwords 4..7. */
#define VU0_MODELVIEW_QW    4               /* 4 + 4 * PGL_MTX_MODELVIEW */

/* (The vf28 copy is a statement macro in the binary: the `then' block of
 * the matrixMode test sits in its own loop notes - egcs then aligns the
 * `else' label, the nop in front of the ctc2.i.) */
#define U118_MODELVIEW_FROM_VF28()  do { u118_modelview_from_vf28(); } while (0)

static inline void u118_modelview_from_vf28(void)
{
    __asm__ __volatile__(
        "vmove.xyzw  $vf20, $vf28\n\t"
        "vmove.xyzw  $vf21, $vf29\n\t"
        "vmove.xyzw  $vf22, $vf30\n\t"
        "vmove.xyzw  $vf23, $vf31"
        : : : "memory");
}

static inline void u118_modelview_from_vu0(void)
{
    __asm__ __volatile__(
        ".set push\n\t"
        ".set noreorder\n\t"
        "ctc2.i      %0, $vi10\n\t"
        "vlqi.xyzw   $vf20, ($vi10++)\n\t"
        "vlqi.xyzw   $vf21, ($vi10++)\n\t"
        "vlqi.xyzw   $vf22, ($vi10++)\n\t"
        "vlqi.xyzw   $vf23, ($vi10++)\n\t"
        "vnop\n\t"
        "vnop\n\t"
        ".set pop"
        : : "r"(VU0_MODELVIEW_QW) : "memory");
}

/* *dst = vf2 times vf20..vf23 (w included), sqc2 (a memory operand: the
 * binary stores at l + offset, no address register). */
static inline void u118_vf2_transform_store(GsVec4 *dst)
{
    __asm__ __volatile__(
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf2x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf2y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf2z\n\t"
        "vmaddw.xyzw  $vf2, $vf23, $vf2w\n\t"
        "sqc2         $vf2, %0"
        : "=m"(*dst) : : "memory");
}

// FUNC 0x0025AE60 gs_set_light_param_v
/* glLightfv mirror (v in one 128-bit register, copied to the stack; jump
 * table @0x003457A0 for pname 0..13):
 *   PGL_LIGHT_AMBIENT / _DIFFUSE / _SPECULAR: stored as given;
 *   PGL_LIGHT_POSITION: normalised when w == 0 (directional), times the
 *     modelview (eye coordinates, as glLightfv);
 *   PGL_LIGHT_SPOT_DIRECTION: normalised, times the modelview;
 *   2, 3, 6..12 and >= 14: nothing.
 * Then, if the light is enabled, ctx->lightFlags |= 1. */
void gs_set_light_param_v(GsContext *ctx, GsLight *l, u32 pname, GsVec4Arg v)
{
    GsVec4 param;

    *(GsVec4Arg *)&param = v;                           /* sq $a3, 0($sp) */
    switch (pname) {
    case PGL_LIGHT_AMBIENT:                             /* (lq / sq) */
        *(GsVec4Arg *)&l->ambient = *(GsVec4Arg *)&param;
        break;
    case PGL_LIGHT_DIFFUSE:
        *(GsVec4Arg *)&l->diffuse = *(GsVec4Arg *)&param;
        break;
    case PGL_LIGHT_SPECULAR:
        *(GsVec4Arg *)&l->specular = *(GsVec4Arg *)&param;
        break;
    case PGL_LIGHT_POSITION:
        u118_vf2_load(&param);
        do {                                            /* (a statement macro: the join label aligned) */
            if (param.w == 0.0f)
                u118_vf2_normalize();
        } while (0);
        if (ctx->matrixMode == PGL_MTX_MODELVIEW)
            U118_MODELVIEW_FROM_VF28();
        else
            u118_modelview_from_vu0();
        u118_vf2_transform_store(&l->position);
        break;
    case PGL_LIGHT_SPOT_DIRECTION:
        u118_vf2_load(&param);
        u118_vf2_normalize();
        if (ctx->matrixMode == PGL_MTX_MODELVIEW)
            U118_MODELVIEW_FROM_VF28();
        else
            u118_modelview_from_vu0();
        u118_vf2_transform_store(&l->spotDir);
        break;
    }
    if (l->enable)
        ctx->lightFlags |= PGL_LF_LIGHTS;
}

/* SDR / SDL (little endian) of one doubleword at an arbitrary address. */
static inline void u118_sdr(u8 *a, u64 v)
{
    u32 k, n = 8 - ((u32)a & 7);

    for (k = 0; k < n; k++)
        a[k] = (u8)(v >> (8 * k));
}

static inline void u118_sdl(u8 *a, u64 v)
{
    u32 k, n = ((u32)a & 7) + 1;
    u8 *base = (u8 *)((u32)a & ~7u);

    for (k = 0; k < n; k++)
        base[k] = (u8)(v >> (8 * (8 - n + k)));
}

// FUNC 0x0025AFE8 mat44_copy
/* The function list's start is its unaligned-source path 0x0025AFE8 (since
 * the 2026-10-16 regen; it was counted into gs_set_light_param_v); the entry
 * point is 0x0025B03C, reached only by pgl_set_normal_matrix's tail jump.
 * The C here is not the original: hand-written: four
 * lq / sq when 16-byte aligned, else ldr/ldl + pcpyld, qword halves via
 * pcpyud + sdr/sdl.  Two slips in the unaligned paths, reproduced:
 *   - load: the pcpyld of rows 1..3 takes its low half from $a4 (row 0)
 *     instead of the doubleword it just loaded, so those rows get row 0's
 *     x, y;
 *   - store: row 1's low-half sdl is at +0xA7 instead of +0x17: it writes
 *     into the doubleword at dst + 0xA0 and leaves the end of
 *     dst + 0x10..0x17 unwritten. */
void mat44_copy(void *dst, const void *src)
{
    const U118U64U *s = (const U118U64U *)src;
    U118U64U *d = (U118U64U *)dst;
    u64 r[8];
    int i;

    if (((u32)src & 0xF) != 0) {
        r[0] = s[0].v;
        r[1] = s[1].v;
        r[2] = r[0];                                    /* slip: s[2] loaded but unused */
        r[3] = s[3].v;
        r[4] = r[0];                                    /* slip */
        r[5] = s[5].v;
        r[6] = r[0];                                    /* slip */
        r[7] = s[7].v;
    } else {
        for (i = 0; i < 8; i++)
            r[i] = ((const u64a *)src)[i];              /* lq x4 */
    }
    if (((u32)dst & 0xF) == 0) {
        for (i = 0; i < 8; i++)
            ((u64a *)dst)[i] = r[i];                    /* sq x4 */
    } else {
        d[0].v = r[0];
        d[1].v = r[1];
        u118_sdr((u8 *)dst + 0x10, r[2]);
        u118_sdl((u8 *)dst + 0xA7, r[2]);               /* slip: +0x17 meant */
        d[3].v = r[3];
        d[4].v = r[4];
        d[5].v = r[5];
        d[6].v = r[6];
        d[7].v = r[7];
    }
}
