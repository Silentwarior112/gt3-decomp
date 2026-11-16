/* vu1_prim.c - immediate-mode primitives of the graphics library
 * (G001), 0x0024FA98..0x00250208: a vertex array drawn as one GS strip
 * (PRIM 4 | ctx->primAttr) by the VU1 primitive program (ctx->vu1Prog).
 *
 * Per strip (at most VU1_MAX_VERTS = 64 vertices) the VIF1 chain gets: the
 * GIFtag (NLOOP n, EOP, PRE, PRIM, REGS ST / RGBAQ / XYZF2) unpacked to VU1
 * 0xC0; a DMA ref
 * of the xyz array (12 bytes a vertex, UNPACK V3-32 -> 0); two bytes
 * {1, 3 (n - 2)} to 0x40; the texture coordinates (8 bytes, UNPACK V2-32
 * -> 0x40), if any; the colours (4 bytes, UNPACK V4-8 -> 0x80) or one
 * constant colour filled in through STROW / STMASK; the matrix and dirty
 * GS registers when due; the MSCAL.  Generated arrays (texture coordinate
 * and colour callbacks, the texture matrix, CPU lighting) are taken from
 * the end of the packet buffer (ctx->end, downwards).
 */
#include "engine/pgl/G001_flare.h"
#include "vu0asm.h"
#include "engine/pgl/pgl_vu1_internal.h"

// FUNC 0x0024FA98 pgl_gen_colors
/* A colour buffer of n words (rounded to qwords) from the packet end,
 * filled by ctx->colorGenFunc; the binary only sets $a0 for the call,
 * $a1..$a4 still hold xyz, normals, colors, n. */
void *pgl_gen_colors(GsContext *ctx, const void *xyz, const void *normals,
                     const void *colors, int n)
{
    u32 *buf;

    buf = (u32 *)((u8 *)ctx->end - ((n + 3) >> 2) * 16);
    ctx->end = buf;
    ctx->colorGenFunc(buf, xyz, normals, colors, n);
    return buf;
}

// FUNC 0x0024FAE0 pgl_gen_texcoords
/* A texture-coordinate buffer (8 bytes a vertex) from the packet end;
 * ctx->texGenFunc is called once with all zeros, then to fill it. */
void *pgl_gen_texcoords(GsContext *ctx, const void *xyz, const void *normals,
                        const void *uvs, int n)
{
    u32 *buf;

    buf = (u32 *)((u8 *)ctx->end - ((n + 1) >> 1) * 16);
    ctx->end = buf;
    ctx->texGenFunc(NULL, NULL, NULL, NULL, 0);
    ctx->texGenFunc(buf, xyz, normals, uvs, n);
    return buf;
}

// FUNC 0x0024FB90 pgl_xform_texcoords
/* (s, t) -> texture matrix x (s, t, 0, 1), keeping x and y, into a buffer
 * from the packet end.  The texture matrix is vf28..vf31 when it is the
 * current one (matrixMode 2), else VU0 data memory qwords 12..15. */
void *pgl_xform_texcoords(GsContext *ctx, const void *uvs, int n)
{
    u64 *buf;
    u64 *dst;
    const u64 *src;

    buf = (u64 *)((u8 *)ctx->end - ((n + 1) >> 1) * 16);
    ctx->end = (u32 *)buf;
    dst = buf;
    src = (const u64 *)uvs;
    do {                        /* (a statement macro: the matrix mode is read after the store) */
        if (ctx->matrixMode == PGL_TEXTURE)
            U115_VU_MOVE_CUR_TO_VF20();
        else
            U115_VU_LOAD_MEM_TO_VF20(VU0_TEXTURE_QW);
    } while (0);
    /* (u32) n-- > 0: the same as n-- != 0, which egcs folds to
     * --n != -1; retail tests a copy of the old count */
    while ((u32)n-- > 0) {
        u64 v = *src;
        __asm__ __volatile__(
            "qmtc2.ni     %0, $vf1\n\t"
            "vmove.zw     $vf1, $vf0\n\t"
            "vmulax.xyzw  " VU0_ACC ", $vf20, $vf1x\n\t"
            "vmadday.xyzw " VU0_ACC ", $vf21, $vf1y\n\t"
            "vmaddaz.xyzw " VU0_ACC ", $vf22, $vf1z\n\t"
            "vmaddw.xyzw  $vf1, $vf23, $vf1w\n\t"
            "qmfc2.ni     %0, $vf1\n\t"
            : "+r"(v));
        *dst = v;
        src++;
        dst++;
    }
    return buf;
}

// FUNC 0x0024FC48 pgl_light_vertices
/* CPU lighting of the vertices (PGL_LF_CPU_LIGHTING): colour scale 128 for
 * textured primitives, else 255; the light state brought up to date;
 * returns __pglShading's colour buffer.  (u115's pass-1 name
 * pgl_shade_vertices belongs to u117's lighting loop 0x002570C8.) */
void *pgl_light_vertices(GsContext *ctx, const void *xyz, const void *normals,
                         const void *colors, const void *uvs, int n)
{
    if (uvs != NULL)
        ctx->colorScale = 128.0f;
    else
        ctx->colorScale = 255.0f;
    if (ctx->lightFlags & PGL_LF_LIGHTS)
        pgl_build_light_list(ctx);
    if (ctx->lightFlags & PGL_LF_MODELVIEW)
        pgl_update_eye_lights(ctx);
    if (ctx->lightFlags & PGL_LF_MATERIAL)
        pgl_update_light_colors(ctx);
    return __pglShading(ctx, (const float *)xyz, (const float *)normals,
                        (const u32 *)colors, n);
}

/* The 4 bytes of rgba as 4 words (r, g, b, a): pextlb / pextlh with $0,
 * MMI that egcs does not emit.  Volatile, as the binary shows: nothing is
 * scheduled across the pair. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
static inline u128 u115_spread_bytes(u32 rgba)
{
    u128 r;

    __asm__ __volatile__("pextlb       %0, $0, %1" : "=r"(r) : "r"(rgba));
    __asm__ __volatile__("pextlh       %0, $0, %1" : "=r"(r) : "r"(r));
    return r;
}
#else
static inline u128 u115_spread_bytes(u32 rgba)
{
    return (u128)(rgba & 0xFF) | ((u128)((rgba >> 8) & 0xFF) << 32) |
           ((u128)((rgba >> 16) & 0xFF) << 64) | ((u128)(rgba >> 24) << 96);
}
#endif

// FUNC 0x0024FD18 pgl_put_const_color
/* One colour for all n vertices (colorTex when textured, else color):
 * STCYCL CL 0 / WL 4, STROW (r, g, b, a as words), STMASK 0x55555555
 * (every field from the row), UNPACK V4-32 (masked) n -> 0x80,
 * STCYCL 4 / 4. */
void pgl_put_const_color(GsContext *ctx, int n, const void *uvs)
{
    u32 *p;
    u32 rgba;
    u128 row;

    p = ctx->cur;
    if (uvs != NULL)
        rgba = ctx->colorTex;
    else
        rgba = ctx->color;
    p[0] = DMATAG_CNT(2);
    p[2] = VIF_STCYCL(0, 4);
    p[3] = VIF_STROW;
    p[1] = 0;
    row = u115_spread_bytes(rgba);
    p[8] = VIF_STMASK;
    p[9] = STMASK_ROW;
    p[10] = VIF_UNPACK_M(VIF_V4_32, n, VU1_STRIP_RGBA);
    p[11] = VIF_STCYCL(4, 4);
    ctx->cur = p + 12;
    ctx->stmask = STMASK_ROW;
    *(u128 *)(p + 4) = row;                     /* the row: sq */
}

// FUNC 0x0024FDA8 pgl_put_colors
/* DMA ref of the colour array, UNPACK V4-8 -> 0x80. */
void pgl_put_colors(GsContext *ctx, const void *colors, int n)
{
    u32 *p;
    int n4;                     /* n rounded up to whole qwords of colours */

    n4 = (n + 3) & ~3;
    p = ctx->cur;
    p[0] = DMATAG_REF(n4 >> 2);
    ctx->cur = p + 4;
    p[1] = U115_DMA_ADDR(colors);
    p[3] = VIF_UNPACK(VIF_V4_8, n4, VU1_STRIP_RGBA);
    p[2] = VIF_NOP;
}

// FUNC 0x0024FE00 pgl_put_texcoords
/* DMA ref of the texture coordinates, UNPACK V2-32 (masked, STMASK
 * 0xC0C0C0C0 first if another mask is set) -> 0x40. */
void pgl_put_texcoords(GsContext *ctx, const void *uvs, int n)
{
    u32 *p;

    if (ctx->stmask != STMASK_DEFAULT)
        gs_emit_stmask(ctx, STMASK_DEFAULT);
    n = (n + 1) & ~1;           /* whole qwords of (s, t) pairs */
    p = ctx->cur;
    ctx->cur = p + 4;
    p[0] = DMATAG_REF(n >> 1);
    p[1] = U115_DMA_ADDR(uvs);
    p[3] = VIF_UNPACK_M(VIF_V2_32, n, VU1_STRIP_ST);
    p[2] = VIF_NOP;
}

// FUNC 0x0024FEA8 pgl_draw_strip_vu1
/* The strip itself (see the file header).  Nothing for n > 64 or without
 * a primitive program.  Statistics: n vertices, n - 2 primitives, 1 part. */
void pgl_draw_strip_vu1(const void *xyz, const void *normals, const void *colors,
                        const void *uvs, int n)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 entry;                  /* MSCAL address of the primitive program */
    u32 prim;
    int n4;                     /* n rounded up to 4 */
    u32 *p;
    u32 *q;
    u32 *r;
    u32 *t;                     /* the closing tag (a pointer of its own: p
                                 * stays block-local, retail's $v0) */

    if (n >= VU1_MAX_VERTS + 1)
        return;
    entry = ctx->vu1Prog;
    if (entry == 0)
        return;
    ctx->nVerts += n;
    ctx->nPrims += n - 2;
    ctx->nParts++;
    if (ctx->matrixDirty != 0)
        pgl_sync_matrices(ctx);
    if (ctx->texGenEnable != 0 && ctx->texGenFunc != NULL)
        uvs = pgl_gen_texcoords(ctx, xyz, normals, uvs, n);
    if (ctx->texMatrixEnable != 0)
        uvs = pgl_xform_texcoords(ctx, uvs, n);
    if ((s8)ctx->lightFlags & PGL_LF_CPU_LIGHTING)  /* lb; andi 0x80 */
        colors = pgl_light_vertices(ctx, xyz, normals, colors, uvs, n);
    if (ctx->colorGenEnable != 0 && ctx->colorGenFunc != NULL)
        colors = pgl_gen_colors(ctx, xyz, normals, colors, n);
    prim = ctx->primAttr | GS_PRIM_TRIANGLE_STRIP;
    if (uvs != NULL)
        prim |= GS_PRIM_TME;
    else
        prim &= ~GS_PRIM_TME;
    n4 = (n + 3) & ~3;
    p = ctx->cur;
    q = p + 8;
    r = p + 12;
    p[0] = DMATAG_CNT(1);
    p[1] = 0;
    p[2] = VIF_NOP;
    p[3] = VIF_UNPACK(VIF_V4_32, 1, VU1_STRIP_GIFTAG);
    p[4] = n | GIF_EOP;
    p[5] = GIF_HI_PRIM(prim) | GIF_HI_NREG(3) | GIF_HI_PRE;
    p[6] = GIF_REGS_ST_RGBAQ_XYZF2;
    p[7] = 0;
    q[0] = DMATAG_REF((n4 * 3) >> 2);
    q[1] = U115_DMA_ADDR(xyz);
    q[2] = VIF_NOP;
    q[3] = VIF_UNPACK(VIF_V3_32, n4, VU1_STRIP_XYZ);
    r[0] = DMATAG_CNT(0);
    r[1] = VIF_NOP;
    r[2] = VIF_UNPACK(VIF_S_8, 2, VU1_STRIP_ST);
    r[3] = (((n - 2) * 3) << 8) | 1;
    ctx->cur = p + 16;
    if (uvs != NULL)
        pgl_put_texcoords(ctx, uvs, n);
    if (colors != NULL)
        pgl_put_colors(ctx, colors, n);
    else
        pgl_put_const_color(ctx, n, uvs);
    if (ctx->vu1MatrixSent == 0) {
        pgl_send_matrix(ctx, VU1_MATRIX_ADDR);
        entry -= VU1_ENTRY_MATRIX;
        ctx->vu1MatrixSent = 1;
    }
    if (ctx->dirty & ctx->enable) {
        pgl_send_dirty_regs(ctx, VU1_REGS_ADDR);
        entry -= VU1_ENTRY_REGS;
    }
    t = ctx->cur;
    t[0] = DMATAG_CNT(0);
    t[1] = 0;
    t[2] = VIF_MSCAL(entry);
    t[3] = VIF_NOP;
    ctx->cur = t + 4;
}

// FUNC 0x002501A8 pgl_draw_strip
/* 40 callers (menus, HUD, race effects).  Only strips of 3 vertices or
 * more, and only while list 0 (VIF1) is being written. */
void pgl_draw_strip(const void *xyz, const void *normals, const void *colors,
                    const void *uvs, int n)
{
    GsContext *ctx = GS_SPR_CTX;

    /* (no tail jump in retail: the early return leaves the call followed
     * by the jump to the return label) */
    if (n < 3)
        return;
    if (ctx->curList == 0)
        pgl_draw_strip_vu1(xyz, normals, colors, uvs, n);
}

// FUNC 0x002501E0 gs_flush_all_regs
/* Every register dirty, then a flush (tail jump).  No callers; the
 * bootstrap's dead gs_flush_all_regs (0x0100E548). */
void gs_flush_all_regs(void)
{
    GS_SPR_CTX->dirty = GS_DIRTY_ALL;
    gs_flush_dirty_regs();
}
