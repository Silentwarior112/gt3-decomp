/* vu1_lighting.c - prebuilt VU1 models and the lighting state of
 * the graphics library (G001), 0x0024DF28..0x0024FA98.
 *
 * A prebuilt model (VifModel) is a list of VIF1 packets run by a VU1
 * program: each part goes out as a DMA "ref" tag followed by VIF ITOP
 * (the lighting variant, ctx->vu1LightProg + vu1LightOfs, which the
 * microcode reads back) and the MSCAL of the previous part, so the VU1
 * program of one part runs while the next one is transferred.  Before
 * the first MSCAL the combined matrix (pgl_send_matrix) goes to VU1 0xC1
 * and dirty GS registers to VU1 0xD9; each such insertion moves the MSCAL
 * entry back (VU1_ENTRY_MATRIX 1, VU1_ENTRY_REGS 5; a lit part enters
 * VU1_ENTRY_LIT 0x3C later) - the VU1 program's entry points are laid out
 * that way (? the microcode was not read).
 *
 * The VU1 data are double buffered: gfx_begin_packet (u116) starts the
 * VIF1 list with BASE 0 / OFFSET 0x1B3 and every UNPACK here has FLG set
 * (addresses relative to TOPS, which swaps between the two buffers at each
 * MSCAL).  So a block that must stay valid for every part, the lighting
 * block at 0xC9, is sent twice per model (the "resend" count of
 * vifmodel_send_lighting: once into each buffer), and again when a light,
 * the modelview or the material changed.
 *
 * Lighting follows OpenGL (8 lights, glMaterial on face 0, a global
 * ambient, the local-viewer switch).  Up to 4 enabled directional lights
 * with a colour get a VU1 slot: their model-space directions and half
 * vectors are kept transposed at lightDirX.. / halfX.. and their light x
 * material colours at lightDiffuse[] / lightSpecular[].  lightFlags bits
 * 0..2 (PGL_LF_DIRTY) mark what must be recomputed.  Lighting modes
 * (VifModel.mode bits 0..4, PGL_LMODE_*): 1 OpenGL lights, 2 an
 * environment matrix (pgl_build_env_matrix), 3 the env-map texture matrix
 * (pgl_build_envmap_matrix), 4 the lights with the eye position (the
 * local-viewer packet), 5 two constant vectors.
 */
#include "engine/pgl/G001_flare.h"
#include "vu0asm.h"
#include "engine/pgl/pgl_vu1_internal.h"

/* The offsets the VU0 asm below uses (lqc2 / sqc2 off(ctx), off(light)). */
_Static_assert(offsetof(GsContext, lightDirX) == 0x1E0, "lightDirX");
_Static_assert(offsetof(GsContext, lightDirY) == 0x1F0, "lightDirY");
_Static_assert(offsetof(GsContext, lightDirZ) == 0x200, "lightDirZ");
_Static_assert(offsetof(GsContext, ambientColor) == 0x210, "ambientColor");
_Static_assert(offsetof(GsContext, lightDiffuse) == 0x220, "lightDiffuse");
_Static_assert(offsetof(GsContext, halfX) == 0x260, "halfX");
_Static_assert(offsetof(GsContext, halfY) == 0x270, "halfY");
_Static_assert(offsetof(GsContext, halfZ) == 0x280, "halfZ");
_Static_assert(offsetof(GsContext, eyePos) == 0x290, "eyePos");
_Static_assert(offsetof(GsContext, lightSpecular) == 0x2A0, "lightSpecular");
_Static_assert(offsetof(GsContext, material) == 0x960, "material");
_Static_assert(offsetof(GsContext, eyeWorld) == 0xA00, "eyeWorld");
_Static_assert(offsetof(GsContext, eyeModel) == 0xA10, "eyeModel");
_Static_assert(offsetof(GsContext, viewDir) == 0xA20, "viewDir");
_Static_assert(offsetof(GsContext, globalAmbient) == 0xA30, "globalAmbient");
_Static_assert(offsetof(GsContext, envMtx) == 0xA70, "envMtx");
_Static_assert(offsetof(GsContext, normalMtx) == 0xAB0, "normalMtx");
_Static_assert(offsetof(GsContext, qAF0) == 0xAF0, "qAF0");
_Static_assert(offsetof(GsContext, qB00) == 0xB00, "qB00");
_Static_assert(offsetof(GsContext, qB10) == 0xB10, "qB10");
_Static_assert(offsetof(GsContext, scale) == 0xB20, "scale");
_Static_assert(offsetof(GsMaterial, ambient) == 0x00 && offsetof(GsMaterial, diffuse) == 0x10 &&
               offsetof(GsMaterial, specular) == 0x20 && offsetof(GsMaterial, emission) == 0x30,
               "GsMaterial colours");
_Static_assert(offsetof(GsLight, ambient) == 0x10, "GsLight.ambient");
_Static_assert(offsetof(GsLight, diffuse) == 0x20, "GsLight.diffuse");
_Static_assert(offsetof(GsLight, diffuseLit) == 0x30, "GsLight.diffuseLit");
_Static_assert(offsetof(GsLight, specular) == 0x40, "GsLight.specular");
_Static_assert(offsetof(GsLight, specularLit) == 0x50, "GsLight.specularLit");
_Static_assert(offsetof(GsLight, position) == 0x60, "GsLight.position");
_Static_assert(offsetof(GsLight, eyePosition) == 0x70, "GsLight.eyePosition");
_Static_assert(offsetof(GsLight, halfVector) == 0x80, "GsLight.halfVector");
_Static_assert(offsetof(GsLight, spotDir) == 0x90, "GsLight.spotDir");
_Static_assert(offsetof(GsLight, eyeSpotDir) == 0xA0, "GsLight.eyeSpotDir");

/* The colour scales as VU0 vectors, built in GPRs from the float bits
 * (lwu, dsll32, or) and moved in with pcpyld + qmtc2.i (MMI / COP2: an asm
 * statement; the pcpyld result replaces the hi operand's register, as in
 * the binary): (colorScale x3, alphaScale) and (255 x3, alphaScale).
 * U115_PAIR(hi, lo): the dword (hi << 32) | lo of two zero-extended
 * words. */
#define U115_PAIR(hi, lo)       (((hi) << 32) | (lo))
#define U115_F255_PAIR          0x437F0000437F0000ull       /* 255.0f, 255.0f */
#define U115_F255               0x437F0000u                 /* 255.0f */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
/* (egcs: the asm overwrites its hi input, which is dead after it; a "+r"
 * copy changes the schedule) */
#define U115_QMTC2_PAIR(vf, hi, lo)                                     \
        __asm__ __volatile__("pcpyld       %0, %0, %1\n\t"             \
                             "qmtc2.i      %0, $" vf                    \
                             : : "r"(hi), "r"(lo))
#else
#define U115_QMTC2_PAIR(vf, hi, lo) ({                                  \
        u64 hi_ = (hi);                                                 \
        __asm__ __volatile__("pcpyld       %0, %0, %1\n\t"             \
                             "qmtc2.i      %0, $" vf                    \
                             : "+r"(hi_) : "r"(lo));                    \
    })
#endif

/* The two scale vectors as statement macros (their loop notes order the
 * schedule as in the binary): cs / as get the colour and alpha scales,
 * vf = (cs, cs, cs, as); vf = (255, 255, 255, as). */
#define U115_LOAD_SCALE_VF(vf, ctx, cs, as) do {                        \
        (cs) = u115_f2u((ctx)->colorScale);                             \
        (as) = u115_f2u((ctx)->alphaScale);                             \
        U115_QMTC2_PAIR(vf, U115_PAIR(as, cs), U115_PAIR(cs, cs));      \
    } while (0)
#define U115_LOAD_255_VF(vf, as) do {                                   \
        U115_QMTC2_PAIR(vf, U115_PAIR(as, U115_F255), U115_F255_PAIR);  \
    } while (0)

/* The specular variant for the material shininess. */
#define U115_BY_SHININESS(ctx, lo, mid, hi) \
    ((ctx)->material[0].shininess < 6.0f ? (lo) : (ctx)->material[0].shininess < 12.0f ? (mid) : (hi))

// FUNC 0x0024DF28 vifmodel_setup_lighting
/* Per model: colour / alpha scales (bits 0 / 1 of scaleFlags: 255, else
 * 128), the matrices of lighting modes 2 and 3, the vertex-format offset of
 * the lighting entry (format 1..3: -2, -4, -6). */
void vifmodel_setup_lighting(GsContext *ctx, const VifModel *m)
{
    int format;
    u32 lmode;
    f32 scale;

    lmode = m->mode;
    scale = 128.0f;
    ctx->vu1LightProg = VU1L_DEFAULT;
    ctx->f014 = 0;
    format = lmode >> 5;                        /* VIFMODEL_FORMAT */
    lmode &= 0x1F;                              /* VIFMODEL_LMODE */
    if (m->scaleFlags & VIFMODEL_COLOR_255)
        scale = 255.0f;
    ctx->colorScale = scale;
    scale = 128.0f;
    if (m->scaleFlags & VIFMODEL_ALPHA_255)
        scale = 255.0f;
    ctx->alphaScale = scale;
    switch (lmode) {
    case PGL_LMODE_NONE:
    case PGL_LMODE_LIGHTS:
    case PGL_LMODE_LOCAL:
    case PGL_LMODE_CONST:
        break;
    case PGL_LMODE_ENVMTX:
        pgl_build_env_matrix(ctx);
        break;
    case PGL_LMODE_ENVMAP:
        pgl_build_envmap_matrix(ctx);
        break;
    }
    ctx->vu1LightOfs = 0;
    /* case 0 is listed: it makes 1 the root of retail's decision tree */
    switch (format) {
    case 0:
        break;
    case 1:
        ctx->vu1LightOfs = -2;
        break;
    case 2:
        ctx->vu1LightOfs = -4;
        break;
    case 3:
        ctx->vu1LightOfs = -6;
        break;
    }
}

// FUNC 0x0024E028 vifmodel_send_lighting
/* The lighting packet of the model's lighting mode while resend (one per
 * VU1 data buffer) is not exhausted; modes 1 and 4 start again at 2 when a
 * light, the modelview or the material changed.  Vertex formats 1 and 2
 * also unpack ctx->morphWeight into the w of VU1 0xF2 (STMASK 0x3F3F3F3F).
 * Returns the new count. */
int vifmodel_send_lighting(GsContext *ctx, u32 mode, int resend)
{
    int format;
    u32 *p;

    format = (int)(mode & 0xE0) >> 5;           /* sra: VIFMODEL_FORMAT */
    switch (VIFMODEL_LMODE(mode)) {     /* (case order as in the binary) */
    case PGL_LMODE_LOCAL:
        if (ctx->lightFlags & PGL_LF_DIRTY)
            resend = 2;
        if (resend != 0) {
            pgl_send_lights_mode4(ctx);
            resend--;
        }
        break;
    case PGL_LMODE_LIGHTS:
        if (ctx->lightFlags & PGL_LF_DIRTY)
            resend = 2;
        if (resend != 0) {
            resend--;
            pgl_send_lights_mode1(ctx);
        }
        break;
    case PGL_LMODE_ENVMTX:
        if (resend != 0) {
            resend--;
            pgl_send_mode2(ctx);
        }
        break;
    case PGL_LMODE_ENVMAP:
        if (resend != 0) {
            resend--;
            pgl_send_mode3(ctx);
        }
        break;
    case PGL_LMODE_CONST:
        if (resend != 0) {
            pgl_send_mode5(ctx);
            resend--;
        }
        break;
    }
    if (format < 3 && format != 0) {
        do {                    /* a statement macro (its loop notes align the return label) */
            p = ctx->cur;
            *p++ = DMATAG_CNT(1);
            *p++ = 0;
            *p++ = VIF_STMASK;
            *p++ = STMASK_W_ONLY;
            *p++ = VIF_UNPACK_M(VIF_S_32, 1, VU1_FB70_ADDR);
            *p++ = u115_f2u(ctx->morphWeight);
            *p++ = VIF_STMASK;
            *p++ = STMASK_DEFAULT;
            ctx->cur = p;
            U115_PREF(p);
        } while (0);
    }
    return resend;
}

// FUNC 0x0024E1B8 vifmodel_put_part
/* DMA ref of one part + ITOP + MSCAL(mscal) (a VIF NOP when 0).  With the
 * texture / PRIM override of the library state (g_002F72C0.enable) on,
 * the part's first qword is replaced: the dword at data + 4 (the GIFtag's
 * low half, read unaligned) gets the ctx->primAttr bits of primMask in
 * its PRIM field and is unpacked twice to VU1 0xC0; the ref then starts
 * one qword later. */
void vifmodel_put_part(GsContext *ctx, const VifPart *part, u32 mscal)
{
    GsLibState *lib = &g_002F72C0;
    u32 *p;
    u32 itop;
    u32 call;
    u32 addr;
    u32 qwc;
    u64 tag;

    itop = ctx->vu1LightProg + ctx->vu1LightOfs;
    p = ctx->cur;
    if (mscal != 0)
        call = VIF_MSCAL(mscal);
    else
        call = VIF_NOP;
    addr = U115_DMA_ADDR(part->data);
    qwc = part->qwc;
    if (lib->enable != 0) {
        tag = ((const u115_u64u *)(addr + 4))->v;
        tag &= ~((u64)lib->primMask << 47);
        tag |= (u64)(lib->primMask & ctx->primAttr) << 47;
        p[0] = DMATAG_REF(qwc - 1);
        p[1] = addr + 0x10;
        p[2] = VIF_ITOP(itop);
        p[3] = call;
        p[4] = DMATAG_CNT(1);
        p[5] = 0;
        p[6] = VIF_NOP;
        p[7] = VIF_UNPACK(VIF_V4_32, 1, VU1_STRIP_GIFTAG);
        U115_PKT64(p, 8) = tag;
        U115_PKT64(p, 10) = tag;
        p += 12;
    } else {
        p[0] = DMATAG_REF(qwc);
        p[1] = addr;
        p[2] = VIF_ITOP(itop);
        p[3] = call;
        p += 4;
    }
    U115_PREF(p);
    ctx->cur = p;
}

// FUNC 0x0024E2B8 vifmodel_draw_lit
/* The general path of model_draw: a prebuilt model with lighting (mode
 * != 0), with the texture override on, or without a part-texture
 * function.  prog is the VU1 address of the model program. */
void vifmodel_draw_lit(const VifModel *m, u32 prog)
{
    GsContext *ctx = GS_SPR_CTX;
    const VifPart *part;
    u32 entry;                  /* MSCAL address for the part just queued */
    int left;
    int resend;
    u32 *p;

    if (ctx->stmask != STMASK_DEFAULT)
        gs_emit_stmask(ctx, STMASK_DEFAULT);
    if (ctx->matrixDirty != 0)
        pgl_sync_matrices(ctx);
    if (m->mode != 0)
        vifmodel_setup_lighting(ctx, m);
    part = m->part;
    ctx->nVerts += m->nVerts;
    ctx->nPrims += m->nPrims;
    ctx->nParts += m->nParts;
    entry = prog;
    vifmodel_put_part(ctx, part, 0);
    if (ctx->vu1MatrixSent == 0) {
        pgl_send_matrix(ctx, VU1_MATRIX_ADDR);
        entry -= VU1_ENTRY_MATRIX;
        ctx->vu1MatrixSent = 1;
    }
    left = m->nParts;
    resend = 2;
    /* A label / goto loop (the binary has no loop alignment here; a for
     * loop gets loop notes and an alignment nop at its top) */
    goto first;
next:
    vifmodel_put_part(ctx, part, entry);
    entry = prog;
first:
    if (part->tex != 0 && ctx->partTexFunc != NULL)
        ctx->partTexFunc(part->tex);
    if (m->mode != 0) {
        entry += VU1_ENTRY_LIT;
        resend = vifmodel_send_lighting(ctx, m->mode, resend);
    }
    if (ctx->dirty & ctx->enable) {
        pgl_send_dirty_regs(ctx, VU1_REGS_ADDR);
        entry -= VU1_ENTRY_REGS;
    }
    left--;
    part++;
    if (left != 0)
        goto next;
    p = ctx->cur;
    *p++ = DMATAG_CNT(0);
    *p++ = 0;
    *p++ = VIF_ITOP(ctx->vu1LightProg + ctx->vu1LightOfs);
    *p++ = VIF_MSCAL(entry);
    ctx->cur = p;
}

// FUNC 0x0024E4A0 vifmodel_draw
/* The fast path of model_draw: an unlit model (mode 0), override off, a
 * part-texture function set - so, unlike vifmodel_draw_lit, the callback
 * is called without a NULL test, and before ctx->cur is updated: it only
 * changes register shadows (texmat_select: gfx_set_texture_regs /
 * material_load), which the dirty test below then sends.  The first ref
 * carries VIF STMASK 0xC0C0C0C0. */
void vifmodel_draw(const VifModel *m, u32 prog)
{
    GsContext *ctx = GS_SPR_CTX;
    const VifPart *part;
    u32 *p;
    u32 entry;                  /* MSCAL address for the part just queued */
    int left;

    if (ctx->matrixDirty != 0)
        pgl_sync_matrices(ctx);
    part = m->part;
    ctx->nVerts += m->nVerts;
    ctx->nPrims += m->nPrims;
    ctx->nParts += m->nParts;
    p = ctx->cur;
    __asm__ __volatile__("pref 0, 0(%0)" : : "r"(p));
    ctx->stmask = STMASK_DEFAULT;
    p[0] = DMATAG_REF(part->qwc);
    p[1] = U115_DMA_ADDR(part->data);
    p[2] = VIF_STMASK;
    p[3] = STMASK_DEFAULT;
    entry = prog;
    p += 4;
    if (ctx->vu1MatrixSent == 0) {
        ctx->cur = p;
        entry -= VU1_ENTRY_MATRIX;
        pgl_send_matrix(ctx, VU1_MATRIX_ADDR);
        p = ctx->cur;
        ctx->vu1MatrixSent = 1;
    }
    left = m->nParts - 1;
    /* A label / goto loop, as vifmodel_draw_lit's: the binary hoists no
     * constants out of it (a for loop's loop notes let loop.c keep the
     * tag constants in three more saved registers); the next part's ref
     * is a statement macro (its loop notes align the label after it) */
    goto first;
next:
    do {
        p[0] = DMATAG_REF(part->qwc);
        p[1] = U115_DMA_ADDR(part->data);
        p[2] = VIF_MSCAL(entry);
        p[3] = VIF_NOP;
        entry = prog;
        left--;
        p += 4;
        U115_PREF(p);
    } while (0);
first:
    if (part->tex != 0)
        ctx->partTexFunc(part->tex);
    if (ctx->dirty & ctx->enable) {
        ctx->cur = p;
        entry -= VU1_ENTRY_REGS;
        pgl_send_dirty_regs(ctx, VU1_REGS_ADDR);
        p = ctx->cur;
    }
    part++;
    if (left != 0)
        goto next;
    p[0] = DMATAG_CNT(0);
    p[1] = 0;
    p[2] = VIF_MSCAL(entry);
    p[3] = VIF_NOP;
    p += 4;
    ctx->cur = p;
}

/* vf24..vf27 = (vf24..vf27) x (VU0 data memory qwords 4..7, the stored
 * modelview): each column vfN of the loaded matrix becomes
 * vf24 * N.x + vf25 * N.y + vf26 * N.z + vf27 * N.w. */
#define U115_VU_MUL_VF24_BY_MEM4_ASM                                     \
        "ctc2.i       %0, $vi10\n\t"                                     \
        "vlqi.xyzw    $vf1, ($vi10++)\n\t"                               \
        "vlqi.xyzw    $vf2, ($vi10++)\n\t"                               \
        "vlqi.xyzw    $vf3, ($vi10++)\n\t"                               \
        "vlqi.xyzw    $vf4, ($vi10++)\n\t"                               \
        "vmulax.xyzw  " VU0_ACC ", $vf24, $vf1x\n\t"                            \
        "vmadday.xyzw " VU0_ACC ", $vf25, $vf1y\n\t"                            \
        "vmaddaz.xyzw " VU0_ACC ", $vf26, $vf1z\n\t"                            \
        "vmaddw.xyzw  $vf1, $vf27, $vf1w\n\t"                            \
        "vmulax.xyzw  " VU0_ACC ", $vf24, $vf2x\n\t"                            \
        "vmadday.xyzw " VU0_ACC ", $vf25, $vf2y\n\t"                            \
        "vmaddaz.xyzw " VU0_ACC ", $vf26, $vf2z\n\t"                            \
        "vmaddw.xyzw  $vf2, $vf27, $vf2w\n\t"                            \
        "vmulax.xyzw  " VU0_ACC ", $vf24, $vf3x\n\t"                            \
        "vmadday.xyzw " VU0_ACC ", $vf25, $vf3y\n\t"                            \
        "vmaddaz.xyzw " VU0_ACC ", $vf26, $vf3z\n\t"                            \
        "vmaddw.xyzw  $vf3, $vf27, $vf3w\n\t"                            \
        "vmulax.xyzw  " VU0_ACC ", $vf24, $vf4x\n\t"                            \
        "vmadday.xyzw " VU0_ACC ", $vf25, $vf4y\n\t"                            \
        "vmaddaz.xyzw " VU0_ACC ", $vf26, $vf4z\n\t"                            \
        "vmaddw.xyzw  $vf27, $vf27, $vf4w\n\t"                           \
        "vmulw.xyzw   $vf24, $vf1, $vf0w\n\t"                            \
        "vmulw.xyzw   $vf25, $vf2, $vf0w\n\t"                            \
        "vmulw.xyzw   $vf26, $vf3, $vf0w\n\t"

// FUNC 0x0024E670 pgl_sync_matrices
/* vf24..vf27 = projection x modelview for VU1: matrix mode 0 (modelview
 * current) runs the VU0 micro subroutine at 0x120; mode 1 (projection
 * current) multiplies vf28..vf31 by the parked modelview; mode 2 (texture
 * current) the parked projection (qwords 8..11) by it.  scaleEnable scales
 * the first three columns by ctx->scale.  Then the matrix and the
 * eye-space lights are due again. */
void pgl_sync_matrices(GsContext *ctx)
{
    __asm__ __volatile__("vnop" : : : "memory");
    switch (ctx->matrixMode) {
    case PGL_MODELVIEW:
        __asm__ __volatile__("vcallms      " VU0_MS_COMBINE : : : "memory");
        break;
    case PGL_PROJECTION:
        do {                    /* a statement macro: its loop notes align the next case label */
            __asm__ __volatile__(
                "vmove.xyzw   $vf24, $vf28\n\t"
                "vmove.xyzw   $vf25, $vf29\n\t"
                "vmove.xyzw   $vf26, $vf30\n\t"
                "vmove.xyzw   $vf27, $vf31\n\t"
                : : : "memory");
        } while (0);
        __asm__ __volatile__(
            U115_VU_MUL_VF24_BY_MEM4_ASM
            : : "r"(VU0_MODELVIEW_QW) : "memory");
        break;
    case PGL_TEXTURE:
        __asm__ __volatile__(
            "ctc2.i       %0, $vi10\n\t"
            "vlqi.xyzw    $vf24, ($vi10++)\n\t"
            "vlqi.xyzw    $vf25, ($vi10++)\n\t"
            "vlqi.xyzw    $vf26, ($vi10++)\n\t"
            "vlqi.xyzw    $vf27, ($vi10++)\n\t"
            "vnop\n\t"
            "vnop\n\t"
            : : "r"(VU0_PROJECTION_QW) : "memory");
        __asm__ __volatile__(
            U115_VU_MUL_VF24_BY_MEM4_ASM
            : : "r"(VU0_MODELVIEW_QW) : "memory");
        break;
    }
    if (ctx->scaleEnable != 0) {
        __asm__ __volatile__(
            "vnop\n\t"
            "lqc2         $vf1, 0xB20(%0)\n\t"      /* scale */
            "vmulx.xyzw   $vf24, $vf24, $vf1x\n\t"
            "vmuly.xyzw   $vf25, $vf25, $vf1y\n\t"
            "vmulz.xyzw   $vf26, $vf26, $vf1z\n\t"
            : : "r"(ctx) : "memory");
    }
    ctx->matrixDirty = 0;
    ctx->vu1MatrixSent = 0;
    ctx->lightFlags |= PGL_LF_MODELVIEW;
}

// FUNC 0x0024E7F0 pgl_send_matrix
/* DMA cnt 8 + UNPACK V4-32 8 -> vuAddr: vf24..vf27 and the viewport
 * vectors viewScale, viewCenter, limits, vBB0. */
void pgl_send_matrix(GsContext *ctx, u32 vuAddr)
{
    u32 *p = ctx->cur;

    p[3] = VIF_UNPACK(VIF_V4_32, 8, vuAddr);
    p[0] = DMATAG_CNT(8);
    p[1] = 0;
    p[2] = VIF_NOP;
    __asm__ __volatile__(
        "vnop\n\t"
        "sqc2         $vf24, 0x10(%0)\n\t"
        "sqc2         $vf25, 0x20(%0)\n\t"
        "sqc2         $vf26, 0x30(%0)\n\t"
        "sqc2         $vf27, 0x40(%0)\n\t"
        : : "r"(p) : "memory");
    U115_QW_COPY(p + 0x14, &ctx->viewScale);
    ctx->cur = p + 0x24;
    U115_QW_COPY(p + 0x18, &ctx->viewCenter);
    U115_QW_COPY(p + 0x1C, &ctx->limits);
    U115_QW_COPY(p + 0x20, &ctx->vBB0);
}

// FUNC 0x0024E858 pgl_send_dirty_regs
/* The dirty GS registers as A+D data of a GIFtag unpacked to vuAddr
 * (DMA cnt n + 1, UNPACK V4-32 n + 1).  Unlike gs_flush_dirty_regs there
 * is no packet-open test and the block is written even for n == 0. */
void pgl_send_dirty_regs(GsContext *ctx, u32 vuAddr)
{
    u32 *p = ctx->cur;
    int nRegs;
    u32 qwc;

    nRegs = gs_collect_dirty_regs(ctx, (u64 *)(p + 8));
    qwc = nRegs + 1;
    p[1] = 0;
    p[3] = VIF_UNPACK(VIF_V4_32, qwc, vuAddr);
    p[0] = DMATAG_CNT(qwc);
    p[2] = VIF_NOP;
    do {                                                /* GIFtag NLOOP n, EOP, A+D */
        U115_PKT64(p, 4) = (u64)(int)(nRegs | GIF_EOP) | GIF_NREG(1);
        U115_PKT64(p, 6) = GIF_REG_AD;
    } while (0);
    ctx->cur = (u32 *)((u128 *)p + (nRegs + 2));
}

// FUNC 0x0024E8F8 pgl_send_lights_diffuse
/* Diffuse lighting, 4 lights: the transposed directions, ambient and
 * diffuse colours (x colour scales), (255, 255, 255, alpha scale); VU1
 * 0xC9, lighting variant VU1L_DIFFUSE.  (Reached by a tail jump.) */
void pgl_send_lights_diffuse(GsContext *ctx)
{
    u64 cs;
    u64 as;
    u32 *p;

    U115_LOAD_SCALE_VF("vf10", ctx, cs, as);
    __asm__ __volatile__(
        "lqc2         $vf1, 0x1E0(%0)\n\t"      /* lightDirX */
        "lqc2         $vf2, 0x1F0(%0)\n\t"      /* lightDirY */
        "lqc2         $vf3, 0x200(%0)\n\t"      /* lightDirZ */
        "lqc2         $vf4, 0x210(%0)\n\t"      /* ambientColor */
        "lqc2         $vf5, 0x220(%0)\n\t"      /* lightDiffuse[0..3] */
        "lqc2         $vf6, 0x230(%0)\n\t"
        "lqc2         $vf7, 0x240(%0)\n\t"
        "lqc2         $vf8, 0x250(%0)\n\t"
        : : "r"(ctx) : "memory");
    U115_LOAD_255_VF("vf9", as);
    __asm__ __volatile__(
        "vmul.xyzw    $vf4, $vf4, $vf10\n\t"
        "vmul.xyzw    $vf5, $vf5, $vf10\n\t"
        "vmul.xyzw    $vf6, $vf6, $vf10\n\t"
        "vmul.xyzw    $vf7, $vf7, $vf10\n\t"
        "vmul.xyzw    $vf8, $vf8, $vf10\n\t"
        : : : "memory");
    p = ctx->cur;
    p[0] = DMATAG_CNT(9);
    p[3] = VIF_UNPACK(VIF_V4_32, 9, VU1_LIGHT_ADDR);
    p[1] = 0;
    p[2] = VIF_NOP;
    __asm__ __volatile__(
        "sqc2         $vf1, 0x10(%0)\n\t"
        "sqc2         $vf2, 0x20(%0)\n\t"
        "sqc2         $vf3, 0x30(%0)\n\t"
        "sqc2         $vf4, 0x40(%0)\n\t"
        "sqc2         $vf5, 0x50(%0)\n\t"
        "sqc2         $vf6, 0x60(%0)\n\t"
        "sqc2         $vf7, 0x70(%0)\n\t"
        "sqc2         $vf8, 0x80(%0)\n\t"
        "sqc2         $vf9, 0x90(%0)\n\t"
        : : "r"(p) : "memory");
    ctx->cur = p + 0x28;
    ctx->vu1LightProg = VU1L_DIFFUSE;
}

// FUNC 0x0024E9C8 pgl_send_lights_specular4
/* Diffuse + specular, 4 lights: directions, ambient, diffuse, half
 * vectors, specular colours, (255, 255, 255, alpha scale) - 16 qwords to
 * VU1 0xC9; the variant by shininess: VU1L_SPEC4_S6 / _S12 / VU1L_SPEC4.
 * (Tail jump.) */
void pgl_send_lights_specular4(GsContext *ctx)
{
    u64 cs;
    u64 as;
    u32 *p;

    U115_LOAD_SCALE_VF("vf17", ctx, cs, as);
    __asm__ __volatile__(
        "lqc2         $vf1, 0x1E0(%0)\n\t"      /* lightDirX, Y, Z */
        "lqc2         $vf2, 0x1F0(%0)\n\t"
        "lqc2         $vf3, 0x200(%0)\n\t"
        "lqc2         $vf4, 0x210(%0)\n\t"      /* ambientColor */
        "lqc2         $vf5, 0x220(%0)\n\t"      /* lightDiffuse[0..3] */
        "lqc2         $vf6, 0x230(%0)\n\t"
        "lqc2         $vf7, 0x240(%0)\n\t"
        "lqc2         $vf8, 0x250(%0)\n\t"
        "lqc2         $vf9, 0x260(%0)\n\t"      /* halfX, Y, Z */
        "lqc2         $vf10, 0x270(%0)\n\t"
        "lqc2         $vf11, 0x280(%0)\n\t"
        "lqc2         $vf12, 0x2A0(%0)\n\t"     /* lightSpecular[0..3] */
        "lqc2         $vf13, 0x2B0(%0)\n\t"
        "lqc2         $vf14, 0x2C0(%0)\n\t"
        "lqc2         $vf15, 0x2D0(%0)\n\t"
        "vmul.xyzw    $vf4, $vf4, $vf17\n\t"
        "vmul.xyzw    $vf5, $vf5, $vf17\n\t"
        "vmul.xyzw    $vf6, $vf6, $vf17\n\t"
        "vmul.xyzw    $vf7, $vf7, $vf17\n\t"
        "vmul.xyzw    $vf8, $vf8, $vf17\n\t"
        "vmul.xyzw    $vf12, $vf12, $vf17\n\t"
        "vmul.xyzw    $vf13, $vf13, $vf17\n\t"
        "vmul.xyzw    $vf14, $vf14, $vf17\n\t"
        "vmul.xyzw    $vf15, $vf15, $vf17\n\t"
        : : "r"(ctx) : "memory");
    U115_LOAD_255_VF("vf16", as);
    p = ctx->cur;
    p[0] = DMATAG_CNT(16);
    p[3] = VIF_UNPACK(VIF_V4_32, 16, VU1_LIGHT_ADDR);
    p[1] = 0;
    p[2] = VIF_NOP;
    __asm__ __volatile__(
        "sqc2         $vf1, 0x10(%0)\n\t"
        "sqc2         $vf2, 0x20(%0)\n\t"
        "sqc2         $vf3, 0x30(%0)\n\t"
        "sqc2         $vf4, 0x40(%0)\n\t"
        "sqc2         $vf5, 0x50(%0)\n\t"
        "sqc2         $vf6, 0x60(%0)\n\t"
        "sqc2         $vf7, 0x70(%0)\n\t"
        "sqc2         $vf8, 0x80(%0)\n\t"
        "sqc2         $vf9, 0x90(%0)\n\t"
        "sqc2         $vf10, 0xA0(%0)\n\t"
        "sqc2         $vf11, 0xB0(%0)\n\t"
        "sqc2         $vf12, 0xC0(%0)\n\t"
        "sqc2         $vf13, 0xD0(%0)\n\t"
        "sqc2         $vf14, 0xE0(%0)\n\t"
        "sqc2         $vf15, 0xF0(%0)\n\t"
        "sqc2         $vf16, 0x100(%0)\n\t"
        : : "r"(p) : "memory");
    ctx->cur = p + 0x44;
    if (ctx->material[0].shininess < 6.0f)
        ctx->vu1LightProg = VU1L_SPEC4_S6;
    else if (ctx->material[0].shininess < 12.0f)
        ctx->vu1LightProg = VU1L_SPEC4_S12;
    else
        ctx->vu1LightProg = VU1L_SPEC4;
}

// FUNC 0x0024EB30 pgl_send_lights_specular2
/* Diffuse + specular, 2 lights: (x, y) of the transposed directions and
 * of the half vectors packed in pairs ({dirX.xy, halfX.xy}, ...), ambient,
 * diffuse 0..1, specular 0..1, (255, 255, 255, alpha scale) - 9 qwords to
 * VU1 0xC9; variant by shininess VU1L_SPEC2_S6 / _S12 / VU1L_SPEC2.
 * (Tail jump.) */
void pgl_send_lights_specular2(GsContext *ctx)
{
    u64 cs;
    u64 as;
    u64 dirX, dirY, dirZ, halfX, halfY, halfZ;  /* the (x, y) lanes: lights 0 and 1 */
    u32 *p;

    U115_LOAD_SCALE_VF("vf9", ctx, cs, as);
    dirX = *(const u115_u64a *)&ctx->lightDirX;
    dirY = *(const u115_u64a *)&ctx->lightDirY;
    dirZ = *(const u115_u64a *)&ctx->lightDirZ;
    __asm__ __volatile__(
        "lqc2         $vf1, 0x210(%0)\n\t"      /* ambientColor */
        "lqc2         $vf2, 0x220(%0)\n\t"      /* lightDiffuse[0..1] */
        "lqc2         $vf3, 0x230(%0)\n\t"
        : : "r"(ctx) : "memory");
    halfX = *(const u115_u64a *)&ctx->halfX;
    halfY = *(const u115_u64a *)&ctx->halfY;
    halfZ = *(const u115_u64a *)&ctx->halfZ;
    __asm__ __volatile__(
        "lqc2         $vf4, 0x2A0(%0)\n\t"      /* lightSpecular[0..1] */
        "lqc2         $vf5, 0x2B0(%0)\n\t"
        "vmul.xyzw    $vf1, $vf1, $vf9\n\t"
        "vmul.xyzw    $vf2, $vf2, $vf9\n\t"
        "vmul.xyzw    $vf3, $vf3, $vf9\n\t"
        "vmul.xyzw    $vf4, $vf4, $vf9\n\t"
        "vmul.xyzw    $vf5, $vf5, $vf9\n\t"
        : : "r"(ctx) : "memory");
    U115_LOAD_255_VF("vf6", as);
    p = ctx->cur;
    p[0] = DMATAG_CNT(9);
    p[3] = VIF_UNPACK(VIF_V4_32, 9, VU1_LIGHT_ADDR);
    p[1] = 0;
    p[2] = VIF_NOP;
    do {                        /* (a statement macro: the pairs stay after the header) */
        U115_PKT64(p, 4) = dirX;
        U115_PKT64(p, 6) = halfX;
        U115_PKT64(p, 8) = dirY;
        U115_PKT64(p, 10) = halfY;
        U115_PKT64(p, 12) = dirZ;
        U115_PKT64(p, 14) = halfZ;
    } while (0);
    __asm__ __volatile__(
        "sqc2         $vf1, 0x40(%0)\n\t"
        "sqc2         $vf2, 0x50(%0)\n\t"
        "sqc2         $vf3, 0x60(%0)\n\t"
        "sqc2         $vf4, 0x70(%0)\n\t"
        "sqc2         $vf5, 0x80(%0)\n\t"
        "sqc2         $vf6, 0x90(%0)\n\t"
        : : "r"(p) : "memory");
    ctx->cur = p + 0x28;
    if (ctx->material[0].shininess < 6.0f)
        ctx->vu1LightProg = VU1L_SPEC2_S6;
    else if (ctx->material[0].shininess < 12.0f)
        ctx->vu1LightProg = VU1L_SPEC2_S12;
    else
        ctx->vu1LightProg = VU1L_SPEC2;
}

// FUNC 0x0024EC68 pgl_send_lights_mode1
/* Lighting mode 1: recompute what the flags ask for, then the diffuse
 * program unless both a light and the material have a specular colour;
 * with specular and a local viewer: the eye-position packet of mode 4
 * (fewer than 2 VU lights) or just VU1L_DEFAULT; else the 2- or 4-light
 * specular packet. */
void pgl_send_lights_mode1(GsContext *ctx)
{
    if (ctx->lightFlags & PGL_LF_LIGHTS)
        pgl_build_light_list(ctx);
    if (ctx->lightFlags & PGL_LF_MODELVIEW)
        pgl_update_eye_lights(ctx);
    if (ctx->lightFlags & PGL_LF_MATERIAL)
        pgl_update_light_colors(ctx);
    if ((ctx->lightFlags & (PGL_LF_MAT_SPECULAR | PGL_LF_SPECULAR))
        != (PGL_LF_MAT_SPECULAR | PGL_LF_SPECULAR)) {
        pgl_send_lights_diffuse(ctx);
        return;
    }
    if (ctx->lightFlags & PGL_LF_LOCAL_VIEWER) {
        if (ctx->nVuLights < 2) {
            pgl_send_lights_mode4(ctx);
            return;
        }
        ctx->vu1LightProg = VU1L_DEFAULT;
        return;
    }
    if (ctx->nVuLights < 3) {
        pgl_send_lights_specular2(ctx);
        return;
    }
    pgl_send_lights_specular4(ctx);
}

// FUNC 0x0024ED50 pgl_send_lights_mode4
/* Lighting mode 4 (and mode 1 with a local viewer): transposed directions,
 * the eye in model space (eyeModel: what a local viewer needs for the
 * specular term) and the specular colours of slots 0..1 (x colour
 * scales) - 6 qwords to VU1 0xC9; variant by shininess VU1L_LOCAL_S6 /
 * _S12 / VU1L_LOCAL.  (? No ambient / diffuse colours in this packet.) */
void pgl_send_lights_mode4(GsContext *ctx)
{
    u64 cs;
    u64 as;
    u32 *p;

    if (ctx->lightFlags & PGL_LF_LIGHTS)
        pgl_build_light_list(ctx);
    if (ctx->lightFlags & PGL_LF_MODELVIEW)
        pgl_update_eye_lights(ctx);
    if (ctx->lightFlags & PGL_LF_MATERIAL)
        pgl_update_light_colors(ctx);
    U115_LOAD_SCALE_VF("vf10", ctx, cs, as);
    __asm__ __volatile__(
        "lqc2         $vf5, 0x2A0(%0)\n\t"      /* lightSpecular[0..1] */
        "lqc2         $vf6, 0x2B0(%0)\n\t"
        "lqc2         $vf1, 0x1E0(%0)\n\t"      /* lightDirX, Y, Z */
        "lqc2         $vf2, 0x1F0(%0)\n\t"
        "lqc2         $vf3, 0x200(%0)\n\t"
        "lqc2         $vf4, 0xA10(%0)\n\t"      /* eyeModel */
        "vmul.xyzw    $vf5, $vf5, $vf10\n\t"
        "vmul.xyzw    $vf6, $vf6, $vf10\n\t"
        : : "r"(ctx) : "memory");
    p = ctx->cur;
    p[0] = DMATAG_CNT(6);
    p[3] = VIF_UNPACK(VIF_V4_32, 6, VU1_LIGHT_ADDR);
    p[1] = 0;
    p[2] = VIF_NOP;
    __asm__ __volatile__(
        "sqc2         $vf1, 0x10(%0)\n\t"
        "sqc2         $vf2, 0x20(%0)\n\t"
        "sqc2         $vf3, 0x30(%0)\n\t"
        "sqc2         $vf4, 0x40(%0)\n\t"
        "sqc2         $vf5, 0x50(%0)\n\t"
        "sqc2         $vf6, 0x60(%0)\n\t"
        : : "r"(p) : "memory");
    ctx->cur = p + 0x1C;
    /* !(s < 12) ? LOCAL : S12 - an exact form, not the original (the
     * same test as s < 12 ? S12 : LOCAL; retail's branch sense) */
    ctx->vu1LightProg = ctx->material[0].shininess < 6.0f ? VU1L_LOCAL_S6
                      : !(ctx->material[0].shininess < 12.0f) ? VU1L_LOCAL : VU1L_LOCAL_S12;
}

// FUNC 0x0024EE80 pgl_build_light_list
/* Links the enabled lights (lights[7] ends up first), counts them, and
 * gives the coloured directional ones VU1 slots 0..3.  Flags: MODELVIEW
 * and MATERIAL set (recompute), LIGHTS cleared; SPECULAR if a light has a
 * specular colour, POINT if a positional light has a colour.  "Has a
 * colour" tests the bits of r, g, b (lw / or), not their values. */
void pgl_build_light_list(GsContext *ctx)
{
    GsLight *l;
    int i;
    int nSlots;
    s8 flags;                   /* (a signed char: lb, and the mask in a register) */
    int directional;
    u8 hasDiffuse;
    u8 hasSpecular;

    flags = (s8)ctx->lightFlags;
    flags |= PGL_LF_DIRTY;
    flags &= ~(PGL_LF_LIGHTS | PGL_LF_SPECULAR | PGL_LF_POINT);
    nSlots = 0;
    ctx->lightList = NULL;
    l = ctx->lights;
    ctx->nLights = 0;
    for (i = 0; i < 8; i++, l++) {      /* (loop.c turns it into a count-down) */
        l->vuIndex = -1;
        if (l->enable == 0)
            continue;
        directional = 1;
        hasDiffuse = (*(u32 *)&l->diffuse.x | *(u32 *)&l->diffuse.y | *(u32 *)&l->diffuse.z) != 0;
        hasSpecular = (*(u32 *)&l->specular.x | *(u32 *)&l->specular.y | *(u32 *)&l->specular.z) != 0;
        l->next = ctx->lightList;
        ctx->nLights++;
        ctx->lightList = l;
        if (!(l->position.w == 0.0f))
            directional = 0;
        if (directional == 0 && (hasDiffuse || hasSpecular))
            flags |= PGL_LF_POINT;
        if (hasSpecular)
            flags |= PGL_LF_SPECULAR;
        if (nSlots < 4 && (hasDiffuse || hasSpecular) && directional != 0) {
            l->vuIndex = nSlots;
            nSlots++;
        }
    }
    ctx->nVuLights = nSlots;
    ctx->lightFlags = flags;
}

/* v[i] = x for one lane of a transposed vector (an inline: its pointer
 * parameter is a pseudo of its own, so loop.c hoists the six row
 * addresses out of pgl_update_eye_lights' light loop, as retail has
 * them). */
static inline void u115_set_lane(f32 *v, int i, f32 x)
{
    v += i;
    *v = x;
}

// FUNC 0x0024EF80 pgl_update_eye_lights
/* Model space for the lights: vf20..vf23 = inverse(modelview) (rigid
 * inverse, columns divided by ctx->scale when scaleEnable).  eyeModel /
 * eyePos = M * eyeWorld; viewDir = normalize(M * (0, 0, -1)); per light:
 * directional - eyePosition = normalize(M * position), halfVector =
 * normalize(eyePosition - viewDir); positional - eyePosition = M *
 * position, eyeSpotDir = normalize(M * spotDir).  Lights with a VU slot
 * copy eyePosition / halfVector into the transposed block (slots without
 * a light keep 0). */
void pgl_update_eye_lights(GsContext *ctx)
{
    u64 t;                      /* the first row of the inverse (an asm temporary) */
    GsLight *l;
    int slot;

    ctx->lightFlags &= ~PGL_LF_MODELVIEW;
    do {                        /* (a statement macro: the flags byte is
                                 * written before the matrix mode is read) */
        if (ctx->matrixMode == PGL_MODELVIEW)
            U115_VU_MOVE_CUR_TO_VF20();
        else
            U115_VU_LOAD_MEM_TO_VF20(VU0_MODELVIEW_QW);
    } while (0);
    *(u128 *)&ctx->lightDirX = 0;               /* por + sq */
    *(u128 *)&ctx->lightDirY = 0;
    *(u128 *)&ctx->lightDirZ = 0;
    *(u128 *)&ctx->halfX = 0;
    *(u128 *)&ctx->halfY = 0;
    *(u128 *)&ctx->halfZ = 0;
    U115_VU_INVERT_VF20(t);
    /* (the VU0 blocks below are statement macros: their loop notes give
     * the labels after them retail's alignment nops) */
    if (ctx->scaleEnable != 0) {
        do { __asm__ __volatile__(
            "vnop\n\t"
            "lqc2         $vf1, 0xB20(%0)\n\t"      /* scale */
            "vdiv         " VU0_Q ", $vf0w, $vf1x\n\t"
            "vmove.xyzw   $vf2, $vf0\n\t"
            "vwaitq\n\t"
            "vaddq.x      $vf2, $vf0, " VU0_Q "\n\t"
            "vdiv         " VU0_Q ", $vf0w, $vf1y\n\t"
            "vwaitq\n\t"
            "vaddq.y      $vf2, $vf0, " VU0_Q "\n\t"
            "vdiv         " VU0_Q ", $vf0w, $vf1z\n\t"
            "vwaitq\n\t"
            "vaddq.z      $vf2, $vf0, " VU0_Q "\n\t"
            "vmul.xyzw    $vf20, $vf20, $vf2\n\t"
            "vmul.xyzw    $vf21, $vf21, $vf2\n\t"
            "vmul.xyzw    $vf22, $vf22, $vf2\n\t"
            : : "r"(ctx) : "memory"); } while (0);
    }
    __asm__ __volatile__(
        "lqc2         $vf2, 0xA00(%0)\n\t"      /* eyeWorld */
        "vsub.xyw     $vf4, $vf0, $vf0\n\t"
        "vsubw.z      $vf4, $vf0, $vf0w\n\t"    /* (0, 0, -1, 0) */
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf2x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf2y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf2z\n\t"
        "vmaddw.xyzw  $vf2, $vf23, $vf2w\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf4x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf4y\n\t"
        "vmaddz.xyzw  $vf4, $vf22, $vf4z\n\t"
        "sqc2         $vf2, 0xA10(%0)\n\t"      /* eyeModel */
        "sqc2         $vf2, 0x290(%0)\n\t"      /* eyePos */
        "vmul.xyzw    $vf1, $vf4, $vf4\n\t"
        "vaddy.x      $vf1, $vf1, $vf1y\n\t"
        "vaddz.x      $vf1, $vf1, $vf1z\n\t"
        "vrsqrt       " VU0_Q ", $vf0w, $vf1x\n\t"
        "vwaitq\n\t"
        "vmulq.xyzw   $vf4, $vf4, " VU0_Q "\n\t"
        "sqc2         $vf4, 0xA20(%0)\n\t"      /* viewDir */
        : : "r"(ctx) : "memory");
    for (l = ctx->lightList; l != NULL; l = l->next) {
        if (l->position.w == 0.0f) {            /* directional */
            do { __asm__ __volatile__(
                "lqc2         $vf2, 0x60(%0)\n\t"       /* position */
                "vmulax.xyzw  " VU0_ACC ", $vf20, $vf2x\n\t"
                "vmadday.xyzw " VU0_ACC ", $vf21, $vf2y\n\t"
                "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf2z\n\t"
                "vmaddw.xyzw  $vf2, $vf23, $vf2w\n\t"
                U115_VU_NORMALIZE_VF2_ASM
                "sqc2         $vf2, 0x70(%0)\n\t"       /* eyePosition */
                "vsub.xyzw    $vf2, $vf2, $vf4\n\t"
                U115_VU_NORMALIZE_VF2_ASM
                "sqc2         $vf2, 0x80(%0)\n\t"       /* halfVector */
                : : "r"(l) : "memory"); } while (0);
        } else {
            do { __asm__ __volatile__(
                "lqc2         $vf2, 0x60(%0)\n\t"       /* position */
                "vmulax.xyzw  " VU0_ACC ", $vf20, $vf2x\n\t"
                "vmadday.xyzw " VU0_ACC ", $vf21, $vf2y\n\t"
                "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf2z\n\t"
                "vmaddw.xyzw  $vf2, $vf23, $vf2w\n\t"
                "sqc2         $vf2, 0x70(%0)\n\t"       /* eyePosition */
                "lqc2         $vf2, 0x90(%0)\n\t"       /* spotDir */
                "vmulax.xyzw  " VU0_ACC ", $vf20, $vf2x\n\t"
                "vmadday.xyzw " VU0_ACC ", $vf21, $vf2y\n\t"
                "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf2z\n\t"
                "vmaddw.xyzw  $vf2, $vf23, $vf2w\n\t"
                U115_VU_NORMALIZE_VF2_ASM
                "sqc2         $vf2, 0xA0(%0)\n\t"       /* eyeSpotDir */
                : : "r"(l) : "memory"); } while (0);
        }
        if (l->vuIndex >= 0) {
            slot = l->vuIndex;
            u115_set_lane(&ctx->lightDirX.x, slot, l->eyePosition.x);
            u115_set_lane(&ctx->lightDirY.x, slot, l->eyePosition.y);
            u115_set_lane(&ctx->lightDirZ.x, slot, l->eyePosition.z);
            u115_set_lane(&ctx->halfX.x, slot, l->halfVector.x);
            u115_set_lane(&ctx->halfY.x, slot, l->halfVector.y);
            u115_set_lane(&ctx->halfZ.x, slot, l->halfVector.z);
        }
    }
}

// FUNC 0x0024F218 pgl_update_light_colors
/* Light x material (face 0): ambientColor = emission + globalAmbient *
 * ambient + sum of light ambient * ambient (in ACC); per light
 * diffuseLit = diffuse * material diffuse, specularLit = specular *
 * material specular, also into the VU slot.  MAT_SPECULAR follows the
 * bits of the material's specular r, g, b. */
void pgl_update_light_colors(GsContext *ctx)
{
    GsLight *l;
    int slot;
    u32 spec;

    /* the bits are read before the flags byte is written (a char store:
     * the loads could not pass it) */
    spec = u115_f2u(ctx->material[0].specular.x) | u115_f2u(ctx->material[0].specular.y)
         | u115_f2u(ctx->material[0].specular.z);
    ctx->lightFlags &= ~PGL_LF_MATERIAL;
    if (spec != 0)
        ctx->lightFlags |= PGL_LF_MAT_SPECULAR;
    else
        ctx->lightFlags &= ~(PGL_LF_MATERIAL | PGL_LF_MAT_SPECULAR);
    __asm__ __volatile__(
        "vnop\n\t"
        "lqc2         $vf1, 0xA30(%0)\n\t"      /* globalAmbient */
        "lqc2         $vf2, 0x990(%0)\n\t"      /* material[0].emission */
        "lqc2         $vf3, 0x960(%0)\n\t"      /*             .ambient */
        "lqc2         $vf4, 0x970(%0)\n\t"      /*             .diffuse */
        "lqc2         $vf5, 0x980(%0)\n\t"      /*             .specular */
        "vmulaw.xyzw  " VU0_ACC ", $vf2, $vf0w\n\t"
        "vmadda.xyzw  " VU0_ACC ", $vf1, $vf3\n\t"
        : : "r"(ctx) : "memory");
    for (l = ctx->lightList; l != NULL; l = l->next) {
        __asm__ __volatile__(
            "lqc2         $vf6, 0x20(%0)\n\t"   /* diffuse */
            "lqc2         $vf7, 0x40(%0)\n\t"   /* specular */
            "lqc2         $vf8, 0x10(%0)\n\t"   /* ambient */
            "vmul.xyzw    $vf6, $vf6, $vf4\n\t"
            "vmul.xyzw    $vf7, $vf7, $vf5\n\t"
            "vmadda.xyzw  " VU0_ACC ", $vf8, $vf3\n\t"
            : : "r"(l) : "memory");
        if (l->vuIndex >= 0) {
            slot = l->vuIndex;
            __asm__ __volatile__(
                "sqc2         $vf6, %0\n\t"
                "sqc2         $vf7, %1\n\t"
                : "=m"(*(ctx->lightDiffuse + slot)), "=m"(*(ctx->lightSpecular + slot)) : : "memory");
        }
        __asm__ __volatile__(
            "sqc2         $vf6, 0x30(%0)\n\t"   /* diffuseLit */
            "sqc2         $vf7, 0x50(%0)\n\t"   /* specularLit */
            : : "r"(l) : "memory");
    }
    __asm__ __volatile__(
        "vmaddx.xyzw  $vf1, $vf0, $vf0x\n\t"
        "sqc2         $vf1, 0x210(%0)\n\t"      /* ambientColor */
        : : "r"(ctx) : "memory");
}

// FUNC 0x0024F2D0 pgl_send_mode5
/* Lighting mode 5: constVector, (255, 255, 255, alpha scale), constColor (set by
 * pgl_set_const_vector / pgl_set_const_color) - 3 qwords to VU1 0xC9, variant VU1L_CONST. */
void pgl_send_mode5(GsContext *ctx)
{
    u64 as;
    u32 *p;

    __asm__ __volatile__("lqc2         $vf1, 0xA40(%0)" : : "r"(ctx) : "memory");     /* constVector */
    do {                        /* (a statement macro; the 255.0f bits in a
                                 * local of their own, as the binary
                                 * allocates them: loaded first, in $a1) */
        u64 f255 = U115_F255;
        as = u115_f2u(ctx->alphaScale);
        U115_QMTC2_PAIR("vf2", U115_PAIR(as, f255), U115_F255_PAIR);
    } while (0);
    __asm__ __volatile__("lqc2         $vf3, 0xA50(%0)" : : "r"(ctx) : "memory");     /* constColor */
    p = ctx->cur;
    p[0] = DMATAG_CNT(3);
    p[3] = VIF_UNPACK(VIF_V4_32, 3, VU1_LIGHT_ADDR);
    p[1] = 0;
    p[2] = VIF_NOP;
    __asm__ __volatile__(
        "sqc2         $vf1, 0x10(%0)\n\t"
        "sqc2         $vf2, 0x20(%0)\n\t"
        "sqc2         $vf3, 0x30(%0)\n\t"
        : : "r"(p) : "memory");
    ctx->cur = p + 16;
    ctx->vu1LightProg = VU1L_CONST;
}

// FUNC 0x0024F348 pgl_send_mode2
/* Lighting mode 2: envMtx[0..3], qB00 * qAF0, qB10 * qAF0 - 6 qwords to
 * VU1 0xC9, variant VU1L_ENVMTX. */
void pgl_send_mode2(GsContext *ctx)
{
    u32 *p = ctx->cur;

    p[0] = DMATAG_CNT(6);
    p[3] = VIF_UNPACK(VIF_V4_32, 6, VU1_LIGHT_ADDR);
    p[1] = 0;
    p[2] = VIF_NOP;
    /* one asm statement per instruction here: the packet pointer then
     * lives across them all and is allocated after the header constants,
     * as in the binary ($a1) */
    __asm__ __volatile__("vnop" : : : "memory");
    __asm__ __volatile__("lqc2         $vf5, 0xB00(%0)" : : "r"(ctx) : "memory");  /* qB00 */
    __asm__ __volatile__("lqc2         $vf6, 0xB10(%0)" : : "r"(ctx) : "memory");  /* qB10 */
    __asm__ __volatile__("lqc2         $vf7, 0xAF0(%0)" : : "r"(ctx) : "memory");  /* qAF0 */
    __asm__ __volatile__("lqc2         $vf1, 0xA70(%0)" : : "r"(ctx) : "memory");  /* envMtx[0..3] */
    __asm__ __volatile__("lqc2         $vf2, 0xA80(%0)" : : "r"(ctx) : "memory");
    __asm__ __volatile__("lqc2         $vf3, 0xA90(%0)" : : "r"(ctx) : "memory");
    __asm__ __volatile__("lqc2         $vf4, 0xAA0(%0)" : : "r"(ctx) : "memory");
    __asm__ __volatile__("vmul.xyzw    $vf5, $vf5, $vf7" : : : "memory");
    __asm__ __volatile__("vmul.xyzw    $vf6, $vf6, $vf7" : : : "memory");
    __asm__ __volatile__("sqc2         $vf1, 0x10(%0)" : : "r"(p) : "memory");
    __asm__ __volatile__("sqc2         $vf2, 0x20(%0)" : : "r"(p) : "memory");
    __asm__ __volatile__("sqc2         $vf3, 0x30(%0)" : : "r"(p) : "memory");
    __asm__ __volatile__("sqc2         $vf4, 0x40(%0)" : : "r"(p) : "memory");
    __asm__ __volatile__("sqc2         $vf5, 0x50(%0)" : : "r"(p) : "memory");
    __asm__ __volatile__("sqc2         $vf6, 0x60(%0)" : : "r"(p) : "memory");
    ctx->cur = p + 0x1C;
    ctx->vu1LightProg = VU1L_ENVMTX;
}

// FUNC 0x0024F3C0 pgl_build_env_matrix
/* Lighting mode 2's matrix (envMtx): with M = modelview (vf20..vf23):
 * rows R = M's 3x3 (premultiplied by normalMtx rows 0..2 when
 * normalMtxEnable), translation R^T * -t (divided by ctx->scale when
 * scaleEnable); its w is whatever vf4.w held (the asm never sets it). */
void pgl_build_env_matrix(GsContext *ctx)
{
    __asm__ __volatile__("vnop" : : : "memory");
    if (ctx->matrixMode == PGL_MODELVIEW)
        U115_VU_MOVE_CUR_TO_VF20();
    else
        U115_VU_LOAD_MEM_TO_VF20(VU0_MODELVIEW_QW);
    /* vf1..vf3 = transpose(vf20..vf22) (w = 0), vf4.xyz = R^T * -t,
     * vf23 = 0 * vf1..vf3 + vf4 * 1 = vf4 */
    __asm__ __volatile__(
        "vsub.xyz     $vf4, $vf0, $vf23\n\t"
        "qmfc2.ni     $7, $vf20\n\t"
        "qmfc2.ni     $6, $vf21\n\t"
        "qmfc2.ni     $5, $vf22\n\t"
        "pextlw       $4, $6, $7\n\t"
        "pextuw       $3, $6, $7\n\t"
        "pextlw       $2, $0, $5\n\t"
        "pextuw       $5, $0, $5\n\t"
        "pcpyld       $7, $2, $4\n\t"
        "pcpyud       $6, $4, $2\n\t"
        "pcpyld       $5, $5, $3\n\t"
        "qmtc2.ni     $7, $vf1\n\t"
        "qmtc2.ni     $6, $vf2\n\t"
        "qmtc2.ni     $5, $vf3\n\t"
        "vmulax.xyz   " VU0_ACC ", $vf1, $vf4x\n\t"
        "vmadday.xyz  " VU0_ACC ", $vf2, $vf4y\n\t"
        "vmaddz.xyz   $vf4, $vf3, $vf4z\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf1, $vf0x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf2, $vf0y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf3, $vf0z\n\t"
        "vmaddw.xyzw  $vf23, $vf4, $vf0w\n\t"
        : : : "memory", "$3", "$4", "$5", "$6", "$7" U115_CLOBBER_V0);
    if (ctx->scaleEnable != 0) {
        __asm__ __volatile__(
            "lqc2         $vf1, 0xB20(%0)\n\t"      /* scale */
            "vdiv         " VU0_Q ", $vf0w, $vf1x\n\t"
            "vwaitq\n\t"
            "vmulq.x      $vf23, $vf23, " VU0_Q "\n\t"
            "vdiv         " VU0_Q ", $vf0w, $vf1y\n\t"
            "vwaitq\n\t"
            "vmulq.y      $vf23, $vf23, " VU0_Q "\n\t"
            "vdiv         " VU0_Q ", $vf0w, $vf1z\n\t"
            "vwaitq\n\t"
            "vmulq.z      $vf23, $vf23, " VU0_Q "\n\t"
            : : "r"(ctx) : "memory");
    }
    if (ctx->normalMtxEnable != 0) {
        __asm__ __volatile__(
            "lqc2         $vf1, 0xAB0(%0)\n\t"      /* normalMtx[0..2] */
            "lqc2         $vf2, 0xAC0(%0)\n\t"
            "lqc2         $vf3, 0xAD0(%0)\n\t"
            "vmulax.xyzw  " VU0_ACC ", $vf1, $vf20x\n\t"
            "vmadday.xyzw " VU0_ACC ", $vf2, $vf20y\n\t"
            "vmaddz.xyzw  $vf20, $vf3, $vf20z\n\t"
            "vmulax.xyzw  " VU0_ACC ", $vf1, $vf21x\n\t"
            "vmadday.xyzw " VU0_ACC ", $vf2, $vf21y\n\t"
            "vmaddz.xyzw  $vf21, $vf3, $vf21z\n\t"
            "vmulax.xyzw  " VU0_ACC ", $vf1, $vf22x\n\t"
            "vmadday.xyzw " VU0_ACC ", $vf2, $vf22y\n\t"
            "vmaddz.xyzw  $vf22, $vf3, $vf22z\n\t"
            : : "r"(ctx) : "memory");
    }
    __asm__ __volatile__(
        "sqc2         $vf20, 0xA70(%0)\n\t"     /* envMtx[0..3] */
        "sqc2         $vf21, 0xA80(%0)\n\t"
        "sqc2         $vf22, 0xA90(%0)\n\t"
        "sqc2         $vf23, 0xAA0(%0)\n\t"
        : : "r"(ctx) : "memory");
}

// FUNC 0x0024F4E0 pgl_set_vec_param
/* kind 0: qAF0, 2: qB00, 3: qB10 = v[0..3] (called by modelset_exec,
 * ops 0x33 / 0x37). */
void pgl_set_vec_param(u32 kind, const f32 *v)
{
    GsContext *ctx = GS_SPR_CTX;

    switch (kind) {
    case PGL_PARAM_AF0:
        ctx->qAF0.x = v[0];
        ctx->qAF0.y = v[1];
        ctx->qAF0.z = v[2];
        ctx->qAF0.w = v[3];
        return;
    case PGL_PARAM_B00:
        ctx->qB00.x = v[0];
        ctx->qB00.y = v[1];
        ctx->qB00.z = v[2];
        ctx->qB00.w = v[3];
        return;
    case PGL_PARAM_B10:
        ctx->qB10.x = v[0];
        ctx->qB10.y = v[1];
        ctx->qB10.z = v[2];
        ctx->qB10.w = v[3];
        return;
    }
}

// FUNC 0x0024F580 pgl_set_scalar_param
/* kind 0: qAF0 = (v, v, v, 1), 2: qB00 = (v, v, v, 1), 3: qB10 =
 * (v, v, v, 0).  Kind 0 fades the car models (T107, T446 pass a fade
 * factor, then 1.0 again); modelset_exec ops 0x32 / 0x36 set kinds 3 / 2. */
void pgl_set_scalar_param(u32 kind, f32 v)
{
    GsContext *ctx = GS_SPR_CTX;

    switch (kind) {
    case PGL_PARAM_AF0:
        ctx->qAF0.x = v;
        ctx->qAF0.y = v;
        ctx->qAF0.z = v;
        ctx->qAF0.w = 1.0f;
        return;
    case PGL_PARAM_B00:
        ctx->qB00.x = v;
        ctx->qB00.y = v;
        ctx->qB00.z = v;
        ctx->qB00.w = 1.0f;
        return;
    case PGL_PARAM_B10:
        ctx->qB10.x = v;
        ctx->qB10.y = v;
        ctx->qB10.z = v;
        ctx->qB10.w = 0.0f;
        return;
    }
}

/* ---- The environment map (lighting mode 3) ---------------------------- */

/* .sdata 0x003530C8..0x003530D4, initialised (written at the definition, in
 * definition order); the file's only small data. */
f32 g_envmapPhase  = 0.0f;                  /* 0x003530C8 the view azimuth in turns, -1..0 */
f32 g_envmapParamA = 4.0f;                  /* 0x003530CC pgl_set_envmap_params a */
f32 g_envmapParamB = 0.25f;                 /* 0x003530D0 pgl_set_envmap_params b */

/* .bss 0x004A68C0..0x004A69A0: file statics (egcs writes a C file's statics
 * at the end, in definition order; a global would be a common, allocated
 * after every object's .bss). */
static GsVec4 g_envmapTexMtx[4];            /* 0x004A68C0 the matrix of lighting mode 3 (pgl_send_mode3) */
static GsVec4 g_envmapRot[3];               /* 0x004A6900 pgl_envmap_rotate's result, pgl_build_envmap_matrix's input */
static GsVec4 g_envmapBasis[3];             /* 0x004A6930 the basis built by pgl_envmap_set_basis */
static GsVec4 g_envmapViewRot[3];           /* 0x004A6960 the transposed modelview rotation (pgl_envmap_set_basis) */
static GsVec4 g_envmapEye;                  /* 0x004A6990 the eye (pgl_envmap_set_basis) */

// FUNC 0x0024F600 pgl_send_mode3
/* Lighting mode 3: the env-map matrix g_envmapTexMtx[0..3], qB00, qB10 -
 * 6 qwords to VU1 0xC9, variant VU1L_ENVMAP. */
void pgl_send_mode3(GsContext *ctx)
{
    u32 *p = ctx->cur;

    p[0] = DMATAG_CNT(6);
    p[3] = VIF_UNPACK(VIF_V4_32, 6, VU1_LIGHT_ADDR);
    p[1] = 0;
    p[2] = VIF_NOP;
    *(u128 *)(p + 4) = *(u128 *)&g_envmapTexMtx[0];
    *(u128 *)(p + 8) = *(u128 *)&g_envmapTexMtx[1];
    *(u128 *)(p + 12) = *(u128 *)&g_envmapTexMtx[2];
    *(u128 *)(p + 16) = *(u128 *)&g_envmapTexMtx[3];
    *(u128 *)(p + 20) = *(u128 *)&ctx->qB00;
    *(u128 *)(p + 24) = *(u128 *)&ctx->qB10;
    ctx->cur = p + 0x1C;
    ctx->vu1LightProg = VU1L_ENVMAP;
}

// FUNC 0x0024F670 pgl_build_envmap_matrix
/* Lighting mode 3's texture matrix g_envmapTexMtx: rows 0..2 = the
 * rotated env-map basis g_envmapRot times the modelview's 3x3, row 3 =
 * R^T * -t of the modelview; then the w column: -0.125, g_envmapPhase +
 * 0.875, g_envmapParamA * -1/6, g_envmapParamB + 0.5. */
void pgl_build_envmap_matrix(GsContext *ctx)
{
    u64 t;                      /* the first row of the inverse (an asm temporary) */
    GsVec4 *m;
    GsVec4 *q;

    __asm__ __volatile__("vnop" : : : "memory");
    if (ctx->matrixMode == PGL_MODELVIEW)
        U115_VU_MOVE_CUR_TO_VF20();
    else
        U115_VU_LOAD_MEM_TO_VF20(VU0_MODELVIEW_QW);
    /* (the loads, the products and the stores as asm statements of their
     * own: retail's registers for the row addresses) */
    __asm__ __volatile__(
        "lqc2         $vf4, 0x00(%0)\n\t"
        "lqc2         $vf5, 0x10(%0)\n\t"
        "lqc2         $vf6, 0x20(%0)\n\t"
        : : "r"(g_envmapRot) : "memory");
    __asm__ __volatile__(
        "vmulax.xyzw  " VU0_ACC ", $vf4, $vf20x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf5, $vf20y\n\t"
        "vmaddz.xyzw  $vf1, $vf6, $vf20z\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf4, $vf21x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf5, $vf21y\n\t"
        "vmaddz.xyzw  $vf2, $vf6, $vf21z\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf4, $vf22x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf5, $vf22y\n\t"
        "vmaddz.xyzw  $vf3, $vf6, $vf22z\n\t"
        : : : "memory");
    m = g_envmapTexMtx;
    do {                        /* (a statement macro: its loop notes end
                                 * CSE's block, so row 3 is addressed anew) */
        __asm__ __volatile__("sqc2         $vf1, %0" : "=m"(m[0]) : : "memory");
        __asm__ __volatile__("sqc2         $vf2, %0" : "=m"(m[1]) : : "memory");
        __asm__ __volatile__("sqc2         $vf3, %0" : "=m"(m[2]) : : "memory");
    } while (0);
    U115_VU_INVERT_VF20(t);
    __asm__ __volatile__(
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf0x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf0y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf0z\n\t"
        "vmaddw.xyzw  $vf1, $vf23, $vf0w\n\t"
        : : : "memory");
    q = &g_envmapTexMtx[3];
    __asm__ __volatile__("sqc2         $vf1, %0" : "=m"(*q) : : "memory");
    m[1].w = g_envmapPhase + 0.875f;
    m[0].w = -0.125f;
    m[2].w = g_envmapParamA * GS_F_M1_6;
    m[3].w = g_envmapParamB + 0.5f;
}

// FUNC 0x0024F7A8 pgl_set_envmap_params
/* (called by course_env_set_envmap with CourseEnv +0xEC0 / +0xEC4) */
void pgl_set_envmap_params(f32 a, f32 b)
{
    g_envmapParamA = a;
    g_envmapParamB = b;
}

// FUNC 0x0024F7B8 pgl_envmap_rotate
/* The env map follows the azimuth of a direction (x, y, z), in the basis
 * g_envmapBasis.  As written the basis' FIRST row is used for all three
 * terms (vmulax / vmadday / vmaddz all on vf1; vf2 / vf3 are loaded but
 * not used), so v = row0 * (x + y + z) (sic).  The phase a =
 * -(pseudo_angle(v.z, v.x) + 1) / 8 (pseudo_angle: -1 .. 7 in units of 45
 * degrees, so a is -1 .. 0 turns) goes to g_envmapPhase;
 * vu0_sincos_deg(a * -360) leaves (sin, cos) in vf1.xy.  Then
 * g_envmapRot = g_envmapViewRot x (Ry x g_envmapBasis) with Ry = rows
 * (c, 0, -s), (0, 1, 0), (s, 0, c) (their w lanes are stale VU
 * registers). */
void pgl_envmap_rotate(f32 x, f32 y, f32 z)
{
    volatile GsVec4 v;          /* volatile: the VU0 asm writes it (retail reloads it, outside the call's delay slot) */
    f32 phase;

    __asm__ __volatile__("vnop" : : : "memory");
    v.x = x;
    v.y = y;
    v.z = z;                                    /* v.w: not written (stack) */
    __asm__ __volatile__("lqc2         $vf9, %0" : : "m"(v) : "memory");
    __asm__ __volatile__("lqc2         $vf1, 0(%0)" : : "r"(&g_envmapBasis[0]) : "memory");
    __asm__ __volatile__("lqc2         $vf2, 0(%0)" : : "r"(&g_envmapBasis[1]) : "memory");
    __asm__ __volatile__("lqc2         $vf3, 0(%0)" : : "r"(&g_envmapBasis[2]) : "memory");
    __asm__ __volatile__(
        "vmulax.xyzw  " VU0_ACC ", $vf1, $vf9x\n\t"    /* (sic) vf1 three times */
        "vmadday.xyzw " VU0_ACC ", $vf1, $vf9y\n\t"
        "vmaddz.xyzw  $vf9, $vf1, $vf9z\n\t"
        : : : "memory");
    __asm__ __volatile__("sqc2         $vf9, %0" : "=m"(v) : : "memory");
    phase = -((pseudo_angle(v.z, v.x) + 1.0f) * 0.125f);
    g_envmapPhase = phase;
    vu0_sincos_deg(phase * -360.0f);            /* vf1 = (sin, cos) */
    __asm__ __volatile__(
        "vaddy.x      $vf4, $vf0, $vf1y\n\t"
        "vadd.y       $vf4, $vf0, $vf0\n\t"
        "vsubx.z      $vf4, $vf0, $vf1x\n\t"
        "vadd.xz      $vf5, $vf0, $vf0\n\t"
        "vaddw.y      $vf5, $vf0, $vf0w\n\t"
        "vaddx.x      $vf6, $vf0, $vf1x\n\t"
        "vadd.y       $vf6, $vf0, $vf0\n\t"
        "vaddy.z      $vf6, $vf0, $vf1y\n\t"
        : : : "memory");
    __asm__ __volatile__("lqc2         $vf1, 0(%0)" : : "r"(&g_envmapBasis[0]) : "memory");
    __asm__ __volatile__("lqc2         $vf2, 0(%0)" : : "r"(&g_envmapBasis[1]) : "memory");
    __asm__ __volatile__("lqc2         $vf3, 0(%0)" : : "r"(&g_envmapBasis[2]) : "memory");
    __asm__ __volatile__(
        "vmulax.xyzw  " VU0_ACC ", $vf4, $vf1x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf5, $vf1y\n\t"
        "vmaddz.xyzw  $vf1, $vf6, $vf1z\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf4, $vf2x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf5, $vf2y\n\t"
        "vmaddz.xyzw  $vf2, $vf6, $vf2z\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf4, $vf3x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf5, $vf3y\n\t"
        "vmaddz.xyzw  $vf3, $vf6, $vf3z\n\t"
        : : : "memory");
    /* the loads, the product and each store as asm statements of their
     * own: the store addresses then get retail's registers (the %hi in
     * $v0, the row pointer in $v1) */
    __asm__ __volatile__(
        "lqc2         $vf4, 0x00(%0)\n\t"
        "lqc2         $vf5, 0x10(%0)\n\t"
        "lqc2         $vf6, 0x20(%0)\n\t"
        : : "r"(g_envmapViewRot) : "memory");
    __asm__ __volatile__(
        "vmulax.xyzw  " VU0_ACC ", $vf1, $vf4x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf2, $vf4y\n\t"
        "vmaddz.xyzw  $vf4, $vf3, $vf4z\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf1, $vf5x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf2, $vf5y\n\t"
        "vmaddz.xyzw  $vf5, $vf3, $vf5z\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf1, $vf6x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf2, $vf6y\n\t"
        "vmaddz.xyzw  $vf6, $vf3, $vf6z\n\t"
        : : : "memory");
    __asm__ __volatile__("sqc2         $vf4, %0" : "=m"(g_envmapRot[0]) : : "memory");
    __asm__ __volatile__("sqc2         $vf5, %0" : "=m"(g_envmapRot[1]) : : "memory");
    __asm__ __volatile__("sqc2         $vf6, %0" : "=m"(g_envmapRot[2]) : : "memory");
}

// FUNC 0x0024F900 pgl_envmap_point_at
/* pgl_envmap_rotate(p - the eye g_envmapEye) (tail jump; called by
 * modelset_exec, op 0x31). */
void pgl_envmap_point_at(f32 x, f32 y, f32 z)
{
    pgl_envmap_rotate(x - g_envmapEye.x, y - g_envmapEye.y, z - g_envmapEye.z);
}

// FUNC 0x0024F928 pgl_envmap_set_basis
/* (called by course_env_set_envmap with a = (0, 1, 0), b = (1, 0, 0): the identity
 * basis)  With a = (ax, ay, az) and b = (bx, by, bz) (vopmula / vopmsub
 * cross products): the basis g_envmapBasis = { a' x c, a', c } with
 * a' = normalize(a) (the up axis), c = normalize(b x a) (w lanes: stale
 * stack / registers).  Then the modelview's inverse: g_envmapViewRot =
 * the R^T rows, g_envmapEye = R^T * -t (the eye), and
 * pgl_envmap_rotate(R^T * (0, 0, -1)) (the view direction). */
void pgl_envmap_set_basis(f32 ax, f32 ay, f32 az, f32 bx, f32 by, f32 bz)
{
    u64 t;                      /* the first row of the inverse (an asm temporary) */
    GsContext *ctx = GS_SPR_CTX;
    volatile GsVec4 b;          /* (volatile: written by the VU0 asm) */
    GsVec4 a;
    GsVec4 *w;                  /* the third axis' row (see below) */

    __asm__ __volatile__("vnop" : : : "memory");
    b.x = bx;
    b.y = by;
    b.z = bz;                                   /* .w: not written */
    a.x = ax;
    a.y = ay;
    a.z = az;
    __asm__ __volatile__("lqc2         $vf2, %0" : : "m"(b) : "memory");
    __asm__ __volatile__("lqc2         $vf3, %0" : : "m"(a) : "memory");
    __asm__ __volatile__(
        "vopmula.xyz  " VU0_ACC ", $vf2, $vf3\n\t"
        "vopmsub.xyz  $vf4, $vf3, $vf2\n\t"
        "vmul.xyzw    $vf1, $vf3, $vf3\n\t"
        "vaddy.x      $vf1, $vf1, $vf1y\n\t"
        "vaddz.x      $vf1, $vf1, $vf1z\n\t"
        "vrsqrt       " VU0_Q ", $vf0w, $vf1x\n\t"
        "vwaitq\n\t"
        "vmulq.xyzw   $vf3, $vf3, " VU0_Q "\n\t"
        "vmul.xyzw    $vf1, $vf4, $vf4\n\t"
        "vaddy.x      $vf1, $vf1, $vf1y\n\t"
        "vaddz.x      $vf1, $vf1, $vf1z\n\t"
        "vrsqrt       " VU0_Q ", $vf0w, $vf1x\n\t"
        "vwaitq\n\t"
        "vmulq.xyzw   $vf4, $vf4, " VU0_Q "\n\t"
        "vopmula.xyz  " VU0_ACC ", $vf3, $vf4\n\t"
        "vopmsub.xyz  $vf2, $vf4, $vf3\n\t"
        : : : "memory");
    /* the third axis first, through a pointer; the other rows as
     * globals - an exact form (retail addresses rows 1 and 0 relative to
     * row 2, and folds row 2's address into the store) */
    w = &g_envmapBasis[2];
    __asm__ __volatile__("sqc2         $vf4, %0" : "=m"(w[0]) : : "memory");
    __asm__ __volatile__("sqc2         $vf3, %0" : "=m"(g_envmapBasis[1]) : : "memory");
    __asm__ __volatile__("sqc2         $vf2, %0" : "=m"(g_envmapBasis[0]) : : "memory");
    if (ctx->matrixMode == PGL_MODELVIEW)
        U115_VU_MOVE_CUR_TO_VF20();
    else
        U115_VU_LOAD_MEM_TO_VF20(VU0_MODELVIEW_QW);
    U115_VU_INVERT_VF20(t);
    __asm__ __volatile__("sqc2         $vf20, %0" : "=m"(g_envmapViewRot[0]) : : "memory");
    __asm__ __volatile__("sqc2         $vf21, %0" : "=m"(g_envmapViewRot[1]) : : "memory");
    __asm__ __volatile__("sqc2         $vf22, %0" : "=m"(g_envmapViewRot[2]) : : "memory");
    __asm__ __volatile__(
        "vsub.xy      $vf1, $vf0, $vf0\n\t"
        "vsubw.z      $vf1, $vf0, $vf0w\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf0x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf0y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf0z\n\t"
        "vmaddw.xyzw  $vf2, $vf23, $vf0w\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf20, $vf1x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf21, $vf1y\n\t"
        "vmaddz.xyzw  $vf1, $vf22, $vf1z\n\t"
        : : : "memory");
    __asm__ __volatile__("sqc2         $vf2, %0" : "=m"(g_envmapEye) : : "memory");
    __asm__ __volatile__("sqc2         $vf1, %0" : "=m"(b) : : "memory");
    pgl_envmap_rotate(b.x, b.y, b.z);           /* the view direction (in b's slot) */
}
