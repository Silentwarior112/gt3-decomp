/* T451_model_set.cpp - ModelSet, the "GTM1" model (unit u102).
 *
 * A ModelSet holds parts (command streams), prebuilt packets, materials,
 * textures (with optional texture sets: variations of the whole table),
 * one bounding sphere per part and an AnimSet (T452).
 */
#include "engine/gfx/model_set_internal.h"
#include "vu0asm.h"
#include "engine/gfx/T451_model_set.h"

/* .sdata (0x00352AD4..0x00352AF4), all explicitly initialised: retail has
 * the zero words among the others, in address order (an uninitialised C++
 * global would come after them, at the end of the file's output). */
ModelSet::Callback *g_00352AD4 = NULL;      /* 0x00352AD4 the callback (NULL: none) */
int  g_00352AD8 = 0;                        /* 0x00352AD8 the frustum test also culls on clip flag 0x10 */
int  g_00352ADC = 1;                        /* 0x00352ADC the depth test allowed: modelset_set_state5 */
f32  g_00352AE0 = 200.0f;                   /* 0x00352AE0 model LOD size */
int  g_00352AE4 = 0;                        /* 0x00352AE4 texture sets by LOD on */
f32  g_00352AE8 = 400.0f;                   /* 0x00352AE8 texture set LOD size */
int  g_00352AEC = -1;                       /* 0x00352AEC the forced texture set, < 0: by size */
int  g_00352AF0 = 0;                        /* 0x00352AF0 packets drawn (a counter) */

/* .sbss: file statics */
static f32 g_00353258;                      /* 0x00353258 gp-0x4798 projection scale (pixels per unit at w 1) */
static f32 g_0035325C;                      /* 0x0035325C gp-0x4794 morph position, 0..1 */

/* Each loop has its own index (retail keeps them in different registers).
 * A material's offset (+0x48) is relocated with a plain statement through
 * a pointer view: GsMaterial.x48 is a union (alias set 0), and retail keeps
 * the table pointer and the count in registers across the store. */
// FUNC 0x00224600 modelset_relocate
void modelset_relocate(ModelSet *m)
{
    int delta = (u8 *)m - (u8 *)m->base;

    if (delta == 0)
        return;
    m->base = m;
    if (m->parts != NULL) {
        int i;
        U102_RELOC_STMT(m->parts, delta);
        for (i = 0; i < m->nParts; i++) {
            if (m->parts[i] != NULL)
                U102_RELOC_STMT(m->parts[i], delta);
        }
    }
    if (m->packets != NULL) {
        int i;
        U102_RELOC_STMT(m->packets, delta);
        for (i = 0; i < m->nPackets; i++) {
            if (m->packets[i] != NULL) {
                U102_RELOC_STMT(m->packets[i], delta);
                vif_model_relocate(m->packets[i]);
            }
        }
    }
    if (m->materials != NULL) {
        int i;
        U102_RELOC_STMT(m->materials, delta);
        for (i = 0; i < m->nMaterials; i++) {
            void **ofs = &m->materials[i].x48.offset;
            if (*ofs != NULL)
                *ofs = (u8 *)*ofs + delta;
        }
    }
    if (m->tex != NULL) {
        int i;
        U102_RELOC_STMT(m->tex, delta);
        for (i = 0; i < m->nTex; i++) {
            if (m->tex[i] != NULL) {
                U102_RELOC_STMT(m->tex[i], delta);
                tex1_relocate(m->tex[i]);
            }
        }
    }
    if (m->texSets != NULL) {
        int i;
        U102_RELOC_STMT(m->texSets, delta);
        for (i = 0; i < m->nTexSets; i++) {
            if (m->texSets[i] != NULL) {
                U102_RELOC_STMT(m->texSets[i], delta);
                /* a set that is the base table is not relocated twice */
                if (m->texSets[i] != m->tex) {
                    int j;
                    for (j = 0; j < m->nTex; j++) {
                        if (m->texSets[i][j] != NULL) {
                            U102_RELOC_STMT(m->texSets[i][j], delta);
                            tex1_relocate(m->texSets[i][j]);
                        }
                    }
                }
            }
        }
    }
    if (m->bounds != NULL)
        U102_RELOC_STMT(m->bounds, delta);
    if (m->materialSets != NULL) {
        int i;
        U102_RELOC_STMT(m->materialSets, delta);
        for (i = 0; i < m->nMaterialSets; i++) {
            if (m->materialSets[i] != NULL)
                U102_RELOC_STMT(m->materialSets[i], delta);
        }
    }
    if (m->anim != NULL) {
        U102_RELOC_STMT(m->anim, delta);
        animset_relocate(m->anim, delta);
    }
}

/* The four CLUT animation walks: the base texture table (NULL entries
 * skipped), then each texture set that is not the base table (entries NOT
 * checked for NULL). */

// FUNC 0x002248D0 modelset_reset_anims
void modelset_reset_anims(ModelSet *m)
{
    int i, j;

    if (m == NULL)
        return;
    for (i = 0; i < m->nTex; i++) {
        if (m->tex[i] != NULL)
            clut_anim_reset_all(m->tex[i]);
    }
    if (m->texSets != NULL) {
        for (i = 0; i < m->nTexSets; i++) {
            Tex1Image **set = m->texSets[i];
            if (set != NULL && set != m->tex) {
                for (j = 0; j < m->nTex; j++)
                    clut_anim_reset_all(m->texSets[i][j]);
            }
        }
    }
    if (m->anim != NULL)
        animset_reset(m->anim);
}

// FUNC 0x002249F8 modelset_step_anims
void modelset_step_anims(ModelSet *m, f32 dt, int id)
{
    int i, j;

    if (m == NULL)
        return;
    for (i = 0; i < m->nTex; i++) {
        if (m->tex[i] != NULL)
            clut_anim_step(m->tex[i], dt, id);
    }
    if (m->texSets != NULL) {
        for (i = 0; i < m->nTexSets; i++) {
            Tex1Image **set = m->texSets[i];
            if (set != NULL && set != m->tex) {
                for (j = 0; j < m->nTex; j++)
                    clut_anim_step(m->texSets[i][j], dt, id);
            }
        }
    }
    /* the tracks only step with "all" (id < 0) */
    if (m->anim != NULL && id < 0)
        animset_step(m->anim, dt);
}

// FUNC 0x00224B48 modelset_apply_clut_anims
void modelset_apply_clut_anims(ModelSet *m)
{
    int i, j;

    if (m == NULL)
        return;
    for (i = 0; i < m->nTex; i++) {
        if (m->tex[i] != NULL)
            clut_anim_apply_all(m->tex[i]);
    }
    if (m->texSets != NULL) {
        for (i = 0; i < m->nTexSets; i++) {
            Tex1Image **set = m->texSets[i];
            if (set != NULL && set != m->tex) {
                for (j = 0; j < m->nTex; j++)
                    clut_anim_apply_all(m->texSets[i][j]);
            }
        }
    }
}

// FUNC 0x00224C60 modelset_release_clut_anims
void modelset_release_clut_anims(ModelSet *m, int id)
{
    int i, j;

    if (m == NULL)
        return;
    for (i = 0; i < m->nTex; i++) {
        if (m->tex[i] != NULL)
            clut_anim_release(m->tex[i], id);
    }
    if (m->texSets != NULL) {
        for (i = 0; i < m->nTexSets; i++) {
            Tex1Image **set = m->texSets[i];
            if (set != NULL && set != m->tex) {
                for (j = 0; j < m->nTex; j++)
                    clut_anim_release(m->texSets[i][j], id);
            }
        }
    }
}

/* ------------------------------------------------------------------------ */
/* Level of detail                                                          */
/* ------------------------------------------------------------------------ */

/* The w (depth) of the sphere centre through the combined matrix vf24..vf27
 * (vu0_sync_state rebuilds it first), at least 0.0001.  The centre may be
 * unaligned (ldl / ldr, lwl / lwr): it is also read inline from the command
 * streams.
 * The original is VU0 macro code: the centre goes to vf1 through the
 * 128-bit GPRs (ldl / ldr + lwl / lwr + pcpyld + qmtc2.ni, whose w lane is
 * the sign extension of z: unused, vmaddw takes vf0w) and w comes back with
 * qmfc2.ni + pcpyud + dsrl32 + mtc1; the qmtc2.ni is a statement of its own
 * (reorg puts it in the delay slot of the call).  The clamp is an asm
 * max.s too: retail computes it before the epilogue, where sched2 would
 * hoist the ld $ra over an RTL max.s (no latency is charged into an asm).
 * GCC 14 goes through an aligned buffer (lqc2 / sqc2; same value). */
// FUNC 0x00224D88 modelset_sphere_depth
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
f32 modelset_sphere_depth(const void *centre)
{
    u128 xy, v, r;
    f32 w, d;

    __asm__ volatile("vnop");
    __asm__ volatile(
        "ldl          %1, 7+%2\n\t"
        "ldr          %1, %2\n\t"
        "lwl          %0, 11+%2\n\t"
        "lwr          %0, 8+%2\n\t"
        "pcpyld       %0, %0, %1\n\t"
        : "=&r"(v), "=&r"(xy) : "m"(*(const ModelSphere *)centre));
    __asm__ volatile("qmtc2.ni     %0, $vf1" : : "r"(v));
    vu0_sync_state();
    __asm__ volatile(
        "vmulax.xyzw  " VU0_ACC ", $vf24, $vf1x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf25, $vf1y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf26, $vf1z\n\t"
        "vmaddw.xyzw  $vf1, $vf27, $vf0w\n\t"
        "qmfc2.ni     %1, $vf1\n\t"
        "pcpyud       %1, %1, %1\n\t"
        "dsrl32       %1, %1, 0\n\t"
        "mtc1         %1, %0\n\t"
        : "=f"(w), "=&r"(r));
    __asm__ volatile("max.s %0, %1, %2" : "=f"(d) : "f"(w), "f"(0.0001f));
    return d;
}
#else
f32 modelset_sphere_depth(const void *centre)
{
    const u8 *src = (const u8 *)centre;
    U102Vec pos;
    f32 w;

    __asm__ volatile("vnop");
    pos.f[0] = u102_rd_f32(src + 0x0);
    pos.f[1] = u102_rd_f32(src + 0x4);
    pos.f[2] = u102_rd_f32(src + 0x8);
    /* the z word is loaded sign-extended into a 64-bit register and pcpyld
     * puts its upper half in lane w; never read (vmaddw takes vf0w) */
    pos.i[3] = pos.i[2] >> 31;
    __asm__ volatile("lqc2         $vf1, 0(%0)" : : "r"(&pos) : "memory");
    vu0_sync_state();
    __asm__ volatile(
        "vmulax.xyzw  " VU0_ACC ", $vf24, $vf1x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf25, $vf1y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf26, $vf1z\n\t"
        "vmaddw.xyzw  $vf1, $vf27, $vf0w\n\t"
        "sqc2         $vf1, 0(%0)\n\t"
        : : "r"(&pos) : "memory");
    w = pos.f[3];
    return w > 0.0001f ? w : 0.0001f;   /* max.s */
}
#endif

/* The projected size of a sphere (unaligned: inline in a command stream). */
// FUNC 0x00224DE8 modelset_sphere_size
f32 modelset_sphere_size(const u8 *sphere)
{
    f32 depth = modelset_sphere_depth(sphere);
    return g_00353258 * u102_rd_f32(sphere + 0xC) / depth;
}

/* The default LOD for a projected size: 0 above g_00352AE0 (200), 1 above
 * half of it, 2 above a quarter, else 3. */
// FUNC 0x00224E30 modelset_default_lod
int modelset_default_lod(f32 size)
{
    f32 lodSize = g_00352AE0;

    if (lodSize < size)
        return 0;
    if (lodSize * 0.5f < size)
        return 1;
    if (lodSize * 0.25f < size)
        return 2;
    return 3;
}

/* The texture set for a part's bounding sphere: as modelset_default_lod with
 * g_00352AE8 (400). */
// FUNC 0x00224E88 modelset_tex_lod
int modelset_tex_lod(const ModelSphere *s)
{
    f32 depth = modelset_sphere_depth(s);
    f32 size = g_00353258 * s->r / depth;
    f32 lodSize = g_00352AE8;

    if (lodSize < size)
        return 0;
    if (lodSize * 0.5f < size)
        return 1;
    if (lodSize * 0.25f < size)
        return 2;
    return 3;
}

/* ------------------------------------------------------------------------ */
/* Drawing                                                                  */
/* ------------------------------------------------------------------------ */

/* The model render states: depth test (if allowed), alpha test on (alpha
 * > 0x20), destination alpha test off, normal alpha blending, no depth
 * offset, depth and colour writes on, Z greater, and m's material table
 * with texmat_select as the packets' texture hook. */
// FUNC 0x00224F18 modelset_begin_draw
void modelset_begin_draw(ModelSet *m)
{
    if (g_00352ADC != 0)
        gfx_enable(PGL_CAP_DEPTH_TEST);
    else
        gfx_disable(PGL_CAP_DEPTH_TEST);
    gfx_enable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_enable(PGL_CAP_CULL_FACE);
    gfx_blend_func(PGL_BLEND_ALPHA, 0.0f);
    gfx_set_depth_offset(0.0f);
    gfx_depth_mask(1);
    gfx_set_frame_write_mask(0xFFFFFFFF);
    gfx_depth_func(PGL_DEPTHFUNC_LEQUAL);
    gfx_alpha_func_raw(PGL_ATEST_GREATER, 0x20);
    gfx_dest_alpha_func(PGL_ATEST_GEQUAL, 0.5f);
    gfx_alpha_fail(PGL_AFAIL_KEEP);
    matsel_set_table(m->materials);
    pgl_set_part_tex_func(texmat_select);
}

/* (m is unused) */
// FUNC 0x00224FE8 modelset_end_draw
void modelset_end_draw(ModelSet *m)
{
    gfx_enable(PGL_CAP_VU1_PROG_33E);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    gfx_disable(PGL_CAP_DEST_ALPHA);
    gfx_depth_mask(1);
    gfx_set_frame_write_mask(0xFFFFFFFF);
}

// FUNC 0x00225028 modelset_draw_all
void modelset_draw_all(ModelSet *m)
{
    int part;

    if (m == NULL)
        return;
    modelset_begin_draw(m);
    for (part = 0; part < m->nParts; part++)
        modelset_run_part(m, part);
    modelset_end_draw(m);
}

// FUNC 0x00225090 modelset_draw_part
void modelset_draw_part(ModelSet *m, int part)
{
    modelset_begin_draw(m);
    modelset_run_part(m, part);
    modelset_end_draw(m);
}

/* The command interpreter of a part: a stream of u8 opcodes with inline
 * operands, 0 ends it.  Handled here (the rest: modelset_exec):
 *   0x02 u8 / 0x03 u16    draw that part (modelset_draw_part: recursive,
 *                         with the render states set again)
 *   0x04 u8 / 0x05 u16    kick packet n
 *   0x35 u16 base, u16 n  kick packet base + k, k = morph position * n,
 *                         the remainder to pgl_set_morph_weight (GsContext.morphWeight,
 *                         sent to VU1 with the lighting: ? the weight
 *                         towards the next packet)
 *   0x34 f32              modelset_set_morph
 *   0x0A u8 n, n * 3 f32, s16 skip   frustum test of n points: all outside
 *                         one plane (or, with g_00352AD8, any beyond the far
 *                         plane, clip flag 0x10): skip forward; else the
 *                         clipping program (PGL_CAP_VU1_PROG_33E) on only if a point is
 *                         beyond the guard band
 *   0x0B                  PGL_CAP_VU1_PROG_33E on
 *   0x2B u8 / 0x2C u16    texture n (of the texture set chosen by LOD)
 *   0x21 u32              set the fog colour; 0x22 save it; 0x23 restore it
 * The fog colour saved by 0x22 starts as the one at entry. */
// FUNC 0x002250D8 modelset_run_part
void modelset_run_part(ModelSet *m, int part)
{
    u8  *p;
    u8   op;
    int  texSet;
    u32  savedFog;
    int  idx;

    if (m == NULL || part >= m->nParts)
        return;
    p = m->parts[part];
    if (p == NULL)
        return;

    texSet = -1;
    if (g_00352AE4 != 0) {
        if (m->texSets != NULL && g_00352AEC < 0)
            texSet = modelset_tex_lod(&m->bounds[part]);
        else
            texSet = g_00352AEC;
        if (!(texSet < m->nTexSets))
            texSet = m->nTexSets - 1;
    }
    savedFog = gs_get_fog_color();

    op = *p++;
    while (op != 0) {
        switch (op) {
        case 0x02: {
            int sub = p[0];
            p += 1;
            modelset_draw_part(m, sub);
            break;
        }
        case 0x03: {
            int sub = U102_GET_U16(p);
            modelset_draw_part(m, sub);
            break;
        }
        case 0x04: {
            int packet = p[0];
            p += 1;
            model_draw(m->packets[packet]);
            g_00352AF0++;
            break;
        }
        case 0x05: {
            int packet = U102_GET_U16(p);
            model_draw(m->packets[packet]);
            g_00352AF0++;
            break;
        }
        case 0x35: {
            int first = U102_GET_U16(p);
            int count = U102_GET_U16(p);
            int k;
            f32 pos;
            pos = g_0035325C * (f32)count;
            k = (int)pos;
            if (!(k < count))
                k = count - 1;
            pgl_set_morph_weight(pos - (f32)k);
            model_draw(m->packets[first + k]);
            g_00352AF0++;
            break;
        }
        case 0x34: {
            f32 t = U102_GET_F32(p);
            modelset_set_morph(t);
            break;
        }
        case 0x0A: {
            /* allOut and anyOut are set before the call (retail keeps both
             * in s-registers); the loop counts up (loop.c turns it into a
             * count-down copy of n) */
            int n = p[0];
            int allOut, anyOut, k;
            allOut = 0x3F;
            anyOut = 0;
            vu0_load_screen_regs();     /* vf20 = GsContext.vBB0 {0.5, 0.5, 1, 1}: the guard band */
            p += 1;
            __asm__ volatile("vnop");
            for (k = 0; k < n; k++) {
                U102Vec clip, guard;
                int flags, clipFlags, clipW, guardW;
                /* vf1 = vf24..vf27 * (x, y, z, 1); vf2 = vf1 * vf20; the
                 * clip flags of both against their w (vi18: vf1 in bits
                 * 6..11, vf2 in bits 0..5) */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
                /* VU0 macro code, one instruction per statement (the
                 * statement count is visible in the register choice); the
                 * point goes to vf1 through the 128-bit GPRs (its w lane is
                 * z's sign extension, unused: vmaddw takes vf0w).  The nop
                 * between the first two vnops is the original's. */
                u128 v, xy;
                __asm__ volatile(
                    "ldl          %1, 7+%2\n\t"
                    "ldr          %1, %2\n\t"
                    "lwl          %0, 11+%2\n\t"
                    "lwr          %0, 8+%2\n\t"
                    "pcpyld       %0, %0, %1\n\t"
                    "qmtc2.ni     %0, $vf1\n\t"
                    : "=&r"(v), "=&r"(xy) : "m"(*(const ModelSphere *)p));
                p += 0xC;
                __asm__ volatile("vmulax.xyzw  " VU0_ACC ", $vf24, $vf1x");
                __asm__ volatile("vmadday.xyzw " VU0_ACC ", $vf25, $vf1y");
                __asm__ volatile("vmaddaz.xyzw " VU0_ACC ", $vf26, $vf1z");
                __asm__ volatile("vmaddw.xyzw  $vf1, $vf27, $vf0w");
                __asm__ volatile("vmul.xyzw    $vf2, $vf1, $vf20");
                __asm__ volatile("vclipw.xyz   $vf1, $vf1w");
                __asm__ volatile("sqc2         $vf1, %0" : "=m"(clip));
                __asm__ volatile("vclipw.xyz   $vf2, $vf2w");
                __asm__ volatile("sqc2         $vf2, %0" : "=m"(guard));
                __asm__ volatile("vnop");
                __asm__ volatile("nop");
                __asm__ volatile("vnop");
                clipW = clip.i[3];
                __asm__ volatile("vnop");
                guardW = guard.i[3];
                __asm__ volatile("vnop");
                __asm__ volatile("cfc2.ni      %0, $vi18" : "=r"(flags));
#else
                U102Vec in;
                in.f[0] = u102_rd_f32(p + 0x0);
                in.f[1] = u102_rd_f32(p + 0x4);
                in.f[2] = u102_rd_f32(p + 0x8);
                in.i[3] = in.i[2] >> 31;    /* z's sign extension (pcpyld), never read: vmaddw takes vf0w */
                __asm__ volatile(
                    "lqc2         $vf1, 0(%1)\n\t"
                    "vmulax.xyzw  " VU0_ACC ", $vf24, $vf1x\n\t"
                    "vmadday.xyzw " VU0_ACC ", $vf25, $vf1y\n\t"
                    "vmaddaz.xyzw " VU0_ACC ", $vf26, $vf1z\n\t"
                    "vmaddw.xyzw  $vf1, $vf27, $vf0w\n\t"
                    "vmul.xyzw    $vf2, $vf1, $vf20\n\t"
                    "vclipw.xyz   $vf1, $vf1w\n\t"
                    "sqc2         $vf1, 0(%2)\n\t"
                    "vclipw.xyz   $vf2, $vf2w\n\t"
                    "sqc2         $vf2, 0(%3)\n\t"
                    "vnop\n\t"
                    "vnop\n\t"
                    "vnop\n\t"
                    "vnop\n\t"
                    "cfc2         %0, $vi18\n\t"
                    : "=r"(flags) : "r"(&in), "r"(&clip), "r"(&guard) : "memory");
                p += 0xC;
                clipW = clip.i[3];
                guardW = guard.i[3];
#endif
                /* behind the eye (w < 0, tested on the bits): swap and
                 * invert each +/- pair (the first in a statement block:
                 * retail aligns the label after it) */
                clipFlags = flags >> 6;
                do {
                    if (clipW < 0)
                        clipFlags = ((flags >> 7) | 0x2A) ^ ((clipFlags << 1) | 0x15);
                } while (0);
                allOut &= clipFlags;
                if (guardW < 0)
                    flags = ((flags >> 1) | 0x2A) ^ ((flags << 1) | 0x15);
                flags &= 0x3F;
                anyOut |= flags;
            }
            if (allOut != 0 || (g_00352AD8 != 0 && (anyOut & 0x10))) {
                U102_BRANCH(p, u102_rd_s16(p));
            } else {
                p += 2;
                if (anyOut == 0)
                    gfx_disable(PGL_CAP_VU1_PROG_33E);
                else
                    gfx_enable(PGL_CAP_VU1_PROG_33E);
            }
            break;
        }
        case 0x0B:
            gfx_enable(PGL_CAP_VU1_PROG_33E);
            break;
        case 0x2B:
            idx = p[0];
            p += 1;
            goto tex;
        case 0x2C: {
            Tex1Image *img;
            idx = U102_GET_U16(p);
        tex:
            img = NULL;
            if (m->texSets != NULL && texSet >= 0)
                img = m->texSets[texSet][idx];
            else if (m->tex != NULL)
                img = m->tex[idx];
            if (img != NULL) {
                g_002F4250.loadTex1(img);
                texsel_set_table(img->tex);
            }
            break;
        }
        case 0x21: {
            u32 rgb = U102_GET_U32(p);
            gfx_set_fog_color(rgb);
            break;
        }
        case 0x22:
            savedFog = gs_get_fog_color();
            break;
        case 0x23:
            gfx_set_fog_color(savedFog);
            break;
        default:
            p = modelset_exec(op, p);
            break;
        }
        op = *p++;
    }
}

/* The other commands; p is past the opcode, returns past the operands.
 * Branches: the offset is relative to the offset field itself.
 *   0x06 s16 id, u8 n, s16 off[n]   switch: k = callback->selectSwitch(id)
 *                         (0 without a callback); k < n (unsigned): take
 *                         off[k], else continue after the table
 *   0x07 sphere (4 f32), u8 n, s16 off[n]   LOD: k = callback->selectLOD(
 *                         projected size) (or modelset_default_lod), at
 *                         most n - 1; take off[k]
 *   0x08 s8 / 0x09 s16    jump
 *   0x0C push / 0x0D pop matrix, 0x0E u8 matrix mode, 0x0F / 0x10 4x4 f32
 *   load / multiply, 0x11 translate, 0x12 scale, 0x13 4 f32 rotate
 *   (degrees, axis), 0x14 / 0x15 / 0x16 f32 rotate about x / y / z
 *   render states (gfx_enable / gfx_disable): 0x17 / 0x18 depth test (0x17
 *   only if g_00352ADC), 0x1A / 0x1B alpha test, 0x1D / 0x1E destination
 *   alpha test, 0x2D / 0x2E PGL_CAP_CULL_FACE
 *   0x19 u8 depth func, 0x1C u8 atst, u8 ref alpha func, 0x1F u8 dest alpha
 *   mode, 0x20 u8 abcd, u8 fix blend, 0x24 u32 the colour bits NOT written,
 *   0x25 / 0x26 depth writes off / on, 0x27 f32 depth offset, 0x28 4 f32
 *   depth bias, 0x29 u8 texture base, 0x2A u8 material base, 0x2F u8 alpha
 *   fail, 0x31 3 f32 env map point at, 0x32 f32 / 0x33 4 f32 qB10, 0x36
 *   f32 / 0x37 4 f32 qB00 (lighting mode 3)
 * Any other opcode (0x01, 0x30, > 0x37, and those modelset_run_part
 * handles) has no operands here.  (0x01 is a case of its own: retail's
 * jump table starts at 1.  The operands are read with the asm readers of
 * model_set_internal.h, the branches with U102_BRANCH, whose loop notes align the case
 * labels after 0x08 and 0x07 as retail.) */
// FUNC 0x00225508 modelset_exec
u8 *modelset_exec(u8 op, u8 *p)
{
    switch (op) {
    case 0x08:
        U102_BRANCH(p, (s8)p[0]);
        break;
    case 0x09:
        U102_BRANCH(p, u102_rd_s16(p));
        break;
    case 0x06: {
        int id = u102_rd_s16(p);
        u32 n, k;
        p += 2;
        n = p[0];
        p += 1;
        if (g_00352AD4 != NULL)
            k = g_00352AD4->selectSwitch(id);
        else
            k = 0;
        if (k >= n) {                       /* unsigned */
            p += n * 2;
        } else {
            p += k * 2;
            U102_BRANCH(p, u102_rd_s16(p));
        }
        break;
    }
    case 0x07: {
        f32 size = modelset_sphere_size(p);
        int k, n;
        if (g_00352AD4 != NULL)
            k = g_00352AD4->selectLOD(size);
        else
            k = modelset_default_lod(size);
        p += 0x10;
        n = p[0];
        p += 1;
        if (!(k < n))
            k = n - 1;
        p += k * 2;
        U102_BRANCH(p, u102_rd_s16(p));
        break;
    }
    case 0x0C:
        pgl_push_matrix();
        break;
    case 0x0D:
        pgl_pop_matrix();
        break;
    case 0x0E: {
        int mode = p[0];
        p += 1;
        pgl_matrix_mode(mode);
        break;
    }
    case 0x0F:
        pgl_load_matrix((const float *)p);
        p += 0x40;
        break;
    case 0x10:
        pgl_mult_matrix((const float *)p);
        p += 0x40;
        break;
    case 0x11: {
        f32 x, y, z;
        x = U102_GET_F32(p);
        y = U102_GET_F32(p);
        z = U102_GET_F32(p);
        pgl_translate(x, y, z);
        break;
    }
    case 0x12: {
        f32 x, y, z;
        x = U102_GET_F32(p);
        y = U102_GET_F32(p);
        z = U102_GET_F32(p);
        vu0_scale(x, y, z);
        break;
    }
    case 0x13: {
        f32 deg, x, y, z;
        deg = U102_GET_F32(p);
        x = U102_GET_F32(p);
        y = U102_GET_F32(p);
        z = U102_GET_F32(p);
        pgl_rotate(deg, x, y, z);
        break;
    }
    case 0x14: {
        f32 deg = U102_GET_F32(p);
        vu0_rotate_x(deg);
        break;
    }
    case 0x15: {
        f32 deg = U102_GET_F32(p);
        vu0_rotate_y(deg);
        break;
    }
    case 0x16: {
        f32 deg = U102_GET_F32(p);
        vu0_rotate_z(deg);
        break;
    }
    case 0x17:
        if (g_00352ADC != 0)
            gfx_enable(PGL_CAP_DEPTH_TEST);
        break;
    case 0x18:
        gfx_disable(PGL_CAP_DEPTH_TEST);
        break;
    case 0x19: {
        int func = p[0];
        p += 1;
        gfx_depth_func(func);
        break;
    }
    case 0x1A:
        gfx_enable(PGL_CAP_ALPHA_TEST);
        break;
    case 0x1B:
        gfx_disable(PGL_CAP_ALPHA_TEST);
        break;
    case 0x1C: {
        int atst, ref;
        atst = p[0]; p += 1;
        ref = p[0]; p += 1;
        gfx_alpha_func_raw(atst, ref);
        break;
    }
    case 0x1D:
        gfx_enable(PGL_CAP_DEST_ALPHA);
        break;
    case 0x1E:
        gfx_disable(PGL_CAP_DEST_ALPHA);
        break;
    case 0x1F: {
        int mode = p[0];
        p += 1;
        gfx_dest_alpha_func(mode, 0.5f);
        break;
    }
    case 0x20: {
        int abcd, fix;
        abcd = p[0]; p += 1;
        fix = p[0]; p += 1;
        gfx_blend_func_raw(abcd, fix);
        break;
    }
    case 0x24: {
        u32 mask = ~U102_GET_U32(p);
        gfx_set_frame_write_mask(mask);
        break;
    }
    case 0x25:
        gfx_depth_mask(0);
        break;
    case 0x26:
        gfx_depth_mask(1);
        break;
    case 0x27: {
        f32 ofs = U102_GET_F32(p);
        gfx_set_depth_offset(ofs);
        break;
    }
    case 0x28: {
        f32 dz, x, y, z;
        dz = U102_GET_F32(p);
        x = U102_GET_F32(p);
        y = U102_GET_F32(p);
        z = U102_GET_F32(p);
        gs_set_depth_bias_at(dz, x, y, z);
        break;
    }
    case 0x29: {
        int base = p[0];
        p += 1;
        texsel_set_base(base);
        break;
    }
    case 0x2A: {
        int base = p[0];
        p += 1;
        matsel_set_base(base);
        break;
    }
    case 0x2D:
        gfx_enable(PGL_CAP_CULL_FACE);
        break;
    case 0x2E:
        gfx_disable(PGL_CAP_CULL_FACE);
        break;
    case 0x2F: {
        int afail = p[0];
        p += 1;
        gfx_alpha_fail(afail);
        break;
    }
    case 0x31: {
        f32 x, y, z;
        x = U102_GET_F32(p);
        y = U102_GET_F32(p);
        z = U102_GET_F32(p);
        pgl_envmap_point_at(x, y, z);
        break;
    }
    case 0x32: {
        f32 v = U102_GET_F32(p);
        pgl_set_scalar_param(PGL_PARAM_ENV_B10, v);
        break;
    }
    case 0x33: {
        f32 v[4];
        v[0] = U102_GET_F32(p);
        v[1] = U102_GET_F32(p);
        v[2] = U102_GET_F32(p);
        v[3] = U102_GET_F32(p);
        pgl_set_vec_param(PGL_PARAM_ENV_B10, v);
        break;
    }
    case 0x36: {
        f32 v = U102_GET_F32(p);
        pgl_set_scalar_param(PGL_PARAM_ENV_B00, v);
        break;
    }
    case 0x37: {
        f32 v[4];
        v[0] = U102_GET_F32(p);
        v[1] = U102_GET_F32(p);
        v[2] = U102_GET_F32(p);
        v[3] = U102_GET_F32(p);
        pgl_set_vec_param(PGL_PARAM_ENV_B00, v);
        break;
    }
    case 0x01:                              /* (no operands: the jump table starts at 1) */
    default:
        break;
    }
    return p;
}

/* ------------------------------------------------------------------------ */
/* Settings                                                                 */
/* ------------------------------------------------------------------------ */

/* Selects a variation (picture / CLUT) of each texture of the base table. */
// FUNC 0x00225AF8 modelset_select_tex_frame
void modelset_select_tex_frame(ModelSet *m, int index)
{
    int i;

    if (m == NULL || m->tex == NULL)
        return;
    for (i = 0; i < m->nTex; i++) {
        if (m->tex[i] != NULL)
            tex1_select_variation(m->tex[i], index);
    }
}

// FUNC 0x00225B80 modelset_select_material_set
void modelset_select_material_set(ModelSet *m, int i)
{
    if (m != NULL && i < m->nMaterialSets)
        m->materials = m->materialSets[i];
}

/* (no caller) */
// FUNC 0x00225BB8 F_00225BB8
void F_00225BB8(GsMaterial *t)
{
    matsel_set_table2(t);
}

// FUNC 0x00225BD8 modelset_set_env_tex
void modelset_set_env_tex(GsTexRegs *regs)
{
    texsel_set_table2(regs);
}

// FUNC 0x00225BF8 modelset_set_callback
void modelset_set_callback(ModelSet::Callback *cb)
{
    g_00352AD4 = cb;
}

/* The morph position of command 0x35, clamped to [0, 1]. */
// FUNC 0x00225C00 modelset_set_morph
void modelset_set_morph(f32 t)
{
    if (t < 0.0f)
        t = 0.0f;
    if (1.0f < t)
        t = 1.0f;
    g_0035325C = t;
}

// FUNC 0x00225C38 modelset_set_cull_far
void modelset_set_cull_far(int on)
{
    g_00352AD8 = on;
}

// FUNC 0x00225C40 modelset_set_lod_size
void modelset_set_lod_size(f32 size)
{
    g_00352AE0 = size;
}

// FUNC 0x00225C48 modelset_set_state5
void modelset_set_state5(int on)
{
    g_00352ADC = on;
}

// FUNC 0x00225C50 modelset_set_tex_lod
void modelset_set_tex_lod(int on)
{
    g_00352AE4 = on;
}

// FUNC 0x00225C58 modelset_set_tex_lod_size
void modelset_set_tex_lod_size(f32 size)
{
    g_00352AE8 = size;
}

// FUNC 0x00225C60 modelset_set_tex_set
void modelset_set_tex_set(int set)
{
    g_00352AEC = set;
}

/* g_00353258 = scale * |x(-1, 0, -1) - x(0, 0, -1)|: the two points through
 * the projection matrix (vu0_load_projection, vf20..vf23), then viewCenter +
 * v * viewScale (vu0_load_screen_regs_nosync: vf22, vf23), no divide by w:
 * the pixels per unit at w 1.  The x words come back with qmfc2.ni (the
 * compiler's mtc1 moves them to the FPU); the abs.s is boot_fabsf (a
 * volatile asm: retail keeps the code before it ahead of it). */
// FUNC 0x00225C68 modelset_set_projection_scale
void modelset_set_projection_scale(f32 scale)
{
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    f32 cx, sx;                             /* x of the two points (qmfc2.ni: the low word) */
#else
    U102Vec centre, side;
#endif

    __asm__ volatile(
        "vnop\n\t"
        "vmove.xyzw   $vf1, $vf0\n\t"
        "vmove.xyzw   $vf2, $vf0\n\t"
        "vsubw.z      $vf1, $vf1, $vf0w\n\t"    /* vf1 = (0, 0, -1, 1) */
        "vsubw.xz     $vf2, $vf2, $vf0w\n\t");  /* vf2 = (-1, 0, -1, 1) */
    vu0_load_projection();
    __asm__ volatile(
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf1x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf1y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf1z\n\t"
        "vmaddw.xyzw  $vf1, $vf23, $vf1w\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf2x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf2y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf2z\n\t"
        "vmaddw.xyzw  $vf2, $vf23, $vf2w\n\t");
    vu0_load_screen_regs_nosync();
    __asm__ volatile(
        "vmulaw.xyzw  " VU0_ACC ", $vf22, $vf0w\n\t"
        "vmadd.xyzw   $vf1, $vf1, $vf23\n\t"
        "vmulaw.xyzw  " VU0_ACC ", $vf22, $vf0w\n\t"
        "vmadd.xyzw   $vf2, $vf2, $vf23\n\t"
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
        "qmfc2.ni     %0, $vf1\n\t"
        "qmfc2.ni     %1, $vf2\n\t"
        : "=r"(cx), "=r"(sx));
    g_00353258 = boot_fabsf(sx - cx) * scale;
#else
        "sqc2         $vf1, 0(%0)\n\t"
        "sqc2         $vf2, 0(%1)\n\t"
        : : "r"(&centre), "r"(&side) : "memory");
    g_00353258 = __builtin_fabsf(side.f[0] - centre.f[0]) * scale;
#endif
}

/* ------------------------------------------------------------------------ */
/* ModelSet::Callback                                                       */
/* ------------------------------------------------------------------------ */

/* Stores the vtable; the deleting form (flags bit 0) then calls operator
 * delete. */
// FUNC 0x00225CF8 ModelSet::Callback::~Callback
ModelSet::Callback::~Callback()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* (egcs: with flags bit 0, operator delete(this) afterwards) */
}

// FUNC 0x00225D28 ModelSet::Callback::selectSwitch
int ModelSet::Callback::selectSwitch(int id)
{
    return 0;
}

// FUNC 0x00225D30 ModelSet::Callback::selectLOD
int ModelSet::Callback::selectLOD(f32 size)
{
    return modelset_default_lod(size);
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (a class without bases): builds the
 * __user_type_info __tiQ28ModelSet8Callback on the first call. */
// FUNC 0x00225D50 __tfQ28ModelSet8Callback
extern "C" void *__tfQ28ModelSet8Callback(void)
{
    if (__tiQ28ModelSet8Callback[0] == 0)
        __rtti_user(__tiQ28ModelSet8Callback, "Q28ModelSet8Callback");
    return __tiQ28ModelSet8Callback;
}
#endif
