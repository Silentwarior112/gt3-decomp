/* render.c - unit u114, 0x0024BF10..0x0024D0C0 (G001, no FDE: C).
 *
 * Drawing helpers of the graphics library:
 *   - box culling with the VU0 clip test (vf24..vf27 = the combined matrix),
 *   - material colour scales (g_002F72C0.matScale) and the material loader,
 *   - a 2D polygon fan built as a GIF packet (DMA cnt + VIF DIRECT),
 *   - the VU1 strip batcher: vertex streams sent by DMA ref + VIF UNPACK
 *     (xyz V3-32 to 0, st V2-32 to 0x40, colours V4-8 to 0x80, a second xyz
 *     stream to g_stripBatchXyz2Addr, never set) and kicked with the strip
 *     lengths (UNPACK S-8 to the w of 0x40..), a GIF tag (UNPACK V3-32 to
 *     0xC0) and MSCAL of the context's VU1 program; at most 64 vertices per
 *     batch,
 *   - the texture register setter (gfx_set_texture_regs, the bootstrap's
 *     function with the override block in g_002F72C0) and the texture /
 *     material selection by index (two tables each).
 */
#include "engine/pgl/G001_flare.h"
#include "vu0asm.h"
#include "engine/pgl/pgl_font_internal.h"

// FUNC 0x0024BF10 vu0_clip_point
// Clip flags (6 bits) of the point (x, y, z) - passed as the float bits,
// zero-extended to 64 bits (box_visible loads them with lwu) - transformed
// by vf24..vf27 against its own w; for w < 0 the flags are remapped
// (VU0_CLIP_REMAP).  The clip-flag read (the fourth vnop and cfc2) is an asm
// of its own: retail schedules the w load in front of it.
u32 vu0_clip_point(u64 x, u64 y, u64 z)
{
    s32 clipPos[4] __attribute__((aligned(16)));   /* sp+0x00 */
    u64 xy = (y << 32) | x;
    u128 v;
    u32 clip;
    s32 w;

    __asm__("pcpyld   %0, %1, %2" : "=r"(v) : "r"(z), "r"(xy));
    /* %0 the point, %1 clipPos (positional: egcs has no named asm operands) */
    __asm__ volatile(
        "qmtc2.i  %0, $vf1\n\t"
        "vmulax.xyzw  " VU0_ACC ", $vf24, $vf1x\n\t"
        "vmadday.xyzw " VU0_ACC ", $vf25, $vf1y\n\t"
        "vmaddaz.xyzw " VU0_ACC ", $vf26, $vf1z\n\t"
        "vmaddw.xyzw  $vf1, $vf27, $vf0w\n\t"
        ".word    0x4BE109FF\n\t"           /* vclipw.xyz $vf1, $vf1w (dest field xyzw) */
        "sqc2     $vf1, 0(%1)\n\t"
        "vnop\n\t"
        "vnop\n\t"
        "vnop\n\t"
        :
        : "r"(v), "r"(clipPos)
        : "memory");
    __asm__("vnop\n\tcfc2.ni  %0, $vi18" : "=r"(clip));
    w = clipPos[3];
    VU0_CLIP_REMAP(clip, w);
    return clip;
}

// FUNC 0x0024BF80 box_visible
// 0 if the box lo..hi (x, y, z floats, read as words) lies wholly outside
// one clip plane (the AND of its 8 corners' clip flags is non-zero), else
// 1.  Callers: race_car_in_view (T131), F_001A1EF8.  BOX_CORNER is a
// statement macro (the loop notes align the return label: the nop in front
// of it).
#define BOX_CORNER(x, y, z) do {                    \
        outside &= vu0_clip_point(x, y, z);         \
        if (outside == 0)                           \
            return 1;                               \
    } while (0)
int box_visible(const f32 *loF, const f32 *hiF)
{
    /* the float corners go to vu0_clip_point as words (lwu, not lwc1) */
    typedef u32 __attribute__((may_alias)) u32_word;
    const u32_word *lo = (const u32_word *)loF;
    const u32_word *hi = (const u32_word *)hiF;
    u32 outside = 0x3F;

    vu0_load_screen_regs();
    BOX_CORNER(lo[0], lo[1], lo[2]);
    BOX_CORNER(hi[0], hi[1], hi[2]);
    BOX_CORNER(lo[0], lo[1], hi[2]);
    BOX_CORNER(hi[0], hi[1], lo[2]);
    BOX_CORNER(lo[0], hi[1], lo[2]);
    BOX_CORNER(hi[0], lo[1], hi[2]);
    BOX_CORNER(lo[0], hi[1], hi[2]);
    outside &= vu0_clip_point(hi[0], lo[1], lo[2]);
    return outside == 0;
}

// FUNC 0x0024C098 mat_scale_set_grey
// mat_scale_set(kind, v, v, v, 1.0) (kind in $a0 passes through).  Caller:
// CarModel::draw (T446).
void mat_scale_set_grey(u32 kind, f32 v)
{
    mat_scale_set(kind, v, v, v, 1.0f);
}

// FUNC 0x0024C0B0 mat_scale_set_rgb
// mat_scale_set(kind, r, g, b, 1.0).  No callers.
void mat_scale_set_rgb(u32 kind, f32 r, f32 g, f32 b)
{
    mat_scale_set(kind, r, g, b, 1.0f);
}

// FUNC 0x0024C0C0 mat_scale_set
// The colour scale of a material component (the kinds of
// gs_set_material_param): 0 ambient, 1 diffuse, 2 both, 3 emission,
// 4 specular.  material_load applies them when matScaleOn is set.
void mat_scale_set(u32 kind, f32 r, f32 g, f32 b, f32 a)
{
    GsLibState *st = &g_002F72C0;

    switch (kind) {
    case PGL_MAT_AMBIENT:
    case PGL_MAT_AMBIENT_AND_DIFFUSE:
        st->matScale[MATSCALE_AMBIENT][0] = r;
        st->matScale[MATSCALE_AMBIENT][1] = g;
        st->matScale[MATSCALE_AMBIENT][2] = b;
        st->matScale[MATSCALE_AMBIENT][3] = a;
        if (kind == PGL_MAT_AMBIENT)
            break;
        /* fall through: ambient and diffuse */
    case PGL_MAT_DIFFUSE:
        st->matScale[MATSCALE_DIFFUSE][0] = r;
        st->matScale[MATSCALE_DIFFUSE][1] = g;
        st->matScale[MATSCALE_DIFFUSE][2] = b;
        st->matScale[MATSCALE_DIFFUSE][3] = a;
        break;
    case PGL_MAT_EMISSION:
        st->matScale[MATSCALE_EMISSION][0] = r;
        st->matScale[MATSCALE_EMISSION][1] = g;
        st->matScale[MATSCALE_EMISSION][2] = b;
        st->matScale[MATSCALE_EMISSION][3] = a;
        break;
    case PGL_MAT_SPECULAR:
        st->matScale[MATSCALE_SPECULAR][0] = r;
        st->matScale[MATSCALE_SPECULAR][1] = g;
        st->matScale[MATSCALE_SPECULAR][2] = b;
        st->matScale[MATSCALE_SPECULAR][3] = a;
        break;
    }
}

// FUNC 0x0024C148 draw_poly2d
// Draws a triangle fan of n vertices in window pixel coordinates xy (pairs
// of floats; y * 8 instead of 16 for field rendering), with optional
// texture coordinates uv (texels, pairs) and colours rgba (one word each;
// without them the context colour colorTex - textured - or color is set
// once).  One GIF packet: A+D (RGBAQ), PRIM (fan | IIP | FST, | TME with
// uv), then REGLIST tags: 2 vertices with XYZF3, the rest (n - 2, EOP)
// with XYZF2.  Caller: F_002231D0 (T449).
// The packet part takes the scratchpad context once more (retail loads
// 0x70003000 a second time there: spriteZF, cur and the colours go through
// it, the draw buffer offsets through the first); each 2-dword item is
// written through a copy of the walking pointer (q = p; p += 2).
// Not exact (79%): the allocation of ctx / uv / rgba / nreg ($s0..$s3 in
// another order) and of the temporaries after it; tried int / u32 regs,
// the offsets' load order, the kick tag through a copy.
void draw_poly2d(const f32 *xy, const f32 *uv, const u32 *rgba, u32 n)
{
    GsContext *ctx = GS_SPR_CTX;
    f32 yScale = 16.0f;
    s32 regs = 0;
    s32 nreg = 0;
    u64 prim;
    s32 xyzShift;
    u32 qwc, zf, i;
    s32 regsKick;
    f32 winX16, winY16;
    u32 *cur;
    u64 *p;
    s32 xoff, yoff;

    if (ctx->halfHeight)
        yScale = 8.0f;
    gs_flush_dirty_regs();
    prim = (ctx->primAttr | (GS_PRIM_FST | GS_PRIM_IIP | GS_PRIM_TRIANGLE_FAN)) & 0xFFFF;
    if (uv != NULL) {
        regs = GIF_REG_UV;
        prim |= GS_PRIM_TME;
        nreg = 1;
    }
    if (rgba != NULL) {
        regs |= GIF_REG_RGBAQ << (nreg * 4);
        nreg++;
    }
    xyzShift = nreg * 4;
    nreg++;                                     /* + XYZF */
    qwc = ((nreg * (n - 1) + 1) >> 1) + (nreg + 1) / 2 + 4 + (rgba == NULL);
    winX16 = (f32)ctx->winX * 16.0f;
    winY16 = (f32)ctx->winY * 16.0f;
    regsKick = (GIF_REG_XYZF2 << xyzShift) | regs;
    regs = (GIF_REG_XYZF3 << xyzShift) | regs;  /* the first two: no drawing kick */
    {
        GsContext *c2 = GS_SPR_CTX;
        u64 *q;

        zf = c2->spriteZF;
        cur = c2->cur;
        xoff = GS_TGT_A(ctx).xoff;
        yoff = GS_TGT_A(ctx).yoff;
        if (qwc != 0) {
            cur[0] = DMATAG_CNT(qwc);
            cur[2] = VIF_FLUSH;
            cur[3] = VIF_DIRECT(qwc);
            c2->cur = cur + qwc * 4 + 4;
            cur[1] = 0;
        }
        p = (u64 *)(cur + 4);
        q = p;
        p += 2;
        q[0] = (rgba != NULL) ? GIF_NREG(1) | GIF_TAG_LO(1, 0) : GIF_NREG(1) | GIF_TAG_LO(2, 0);  /* A+D */
        q[1] = GIF_REG_AD;
        if (rgba == NULL) {
            u32 color;

            q = p;
            p += 2;
            if (uv != NULL)
                color = c2->colorTex;
            else
                color = c2->color;
            q[1] = 1;                               /* RGBAQ */
            ((u32 *)q)[0] = color;
            ((u32 *)q)[1] = GS_Q_ONE;
        }
        q = p;
        p += 2;
        q[0] = prim;
        q[1] = 0;                                   /* PRIM */
        q = p;
        p += 2;
        q[1] = regs;
        q[0] = GIF_NREG(nreg) | GIF_FLG_REGLIST | GIF_TAG_LO(2, 0);
    }
    if (n != 0) {
        u64 tag = GIF_TAG_LO(n - 2, 1) | (GIF_NREG(nreg) | GIF_FLG_REGLIST);

        i = 0;
        do {
            if (i == 2) {
                p = (u64 *)(((u32)p + 0xF) & 0xFFFFFFF0);
                p[1] = regsKick;
                p[0] = tag;
                p += 2;
            }
            if (uv != NULL) {
                f32 v = uv[1] * 16.0f + 8.0f;
                f32 u = uv[0] * 16.0f + 8.0f;

                uv += 2;
                *p = (s64)(s32)((s32)u | ((s32)v << 16));
                p++;
            }
            if (rgba != NULL) {
                u32 color = *rgba;

                ((u32 *)p)[1] = GS_Q_ONE;
                rgba++;
                ((u32 *)p)[0] = color;
                p++;
            }
            i++;
            {
                f32 x = winX16 + xy[0] * 16.0f;
                f32 y = winY16 + xy[1] * yScale;

                ((u32 *)p)[1] = zf;
                xy += 2;
                ((u32 *)p)[0] = ((xoff + (s32)x) & 0xFFFF) | ((yoff + (s32)y) << 16);
                p++;
            }
        } while (i < n);
    }
}

/* The strip batcher's state.  The open flag is initialised (.sdata), the
 * rest are file statics: the original's .sbss has them between u104's and
 * u119's statics, where commons cannot be (they come last, as the __ti
 * ones at 0x003532E8). */
u8 g_stripBatchOpen = 0;                    /* 0x003530B4 gp-0x493C, a batch is open */
static s32 g_stripBatchVerts;               /* 0x00353274 gp-0x477C, vertices in the batch (<= 64) */
static s32 g_stripBatchCount;               /* 0x00353278 gp-0x4778, strips in the batch */
static s32 g_0035327C;                      /* 0x0035327C gp-0x4774, only cleared (no reader in the binary) */
static s32 g_stripBatchXyz2Addr;            /* 0x00353280 gp-0x4770, VU1 address of a 2nd xyz stream, 0 = none
                                             * (only cleared: nothing in the binary sets it) */
static u8  g_stripBatchLengths[VU1_MAX_VERTS]; /* 0x004A6880 .bss, the strip lengths of the batch */

// FUNC 0x0024C488 strip_batch_begin
// Opens a strip batch (does nothing if one is open).  Callers:
// FlareClass::beginReflections (T159), ParticleManager::draw (T165),
// CarRenderCtx::drawGlows (T181), spriteset_draw_part (T454).
void strip_batch_begin(void)
{
    if (g_stripBatchOpen == 0) {
        g_stripBatchVerts = 0;
        g_stripBatchOpen = 1;
        g_stripBatchCount = 0;
        g_0035327C = 0;
        g_stripBatchXyz2Addr = 0;
    }
}

// FUNC 0x0024C4B0 strip_batch_end
// Closes the open batch and, if it has vertices, kicks it: resyncs VU0 /
// sends the matrices (MSCAL address - 1) / the dirty GS registers (- 5) as
// needed, then one DMA cnt packet: STMASK 0x3F3F3F3F (once), the strip
// lengths ((len - 2) * 3, UNPACK S-8 to the w of 0x40.., count first), the
// GIF tag (NLOOP vertices, EOP, PRE, PRIM = triangle strip | primAttr,
// NREG 3: ST, RGBAQ, XYZF2; UNPACK V3-32 to 0xC0) and MSCAL.  The header
// goes through the $a2 register variable (U114_IN_A2), the strip count byte
// through it too, the lengths through a byte pointer, the tail through a
// pointer of its own.
void strip_batch_end(void)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 entry;
    u32 attr;
    u32 *tag;
    register u32 *w U114_IN_A2;
    u32 *p;
    u8 *b;
    int i;

    if (g_stripBatchOpen == 0)
        return;
    g_stripBatchOpen = 0;
    if (g_stripBatchVerts == 0)
        return;
    ctx->nParts++;
    if (ctx->matrixDirty)
        pgl_sync_matrices(ctx);
    entry = ctx->vu1Prog;
    if (!ctx->vu1MatrixSent) {
        pgl_send_matrix(ctx, VU1_MATRIX_ADDR);
        entry -= 1;
        ctx->vu1MatrixSent = 1;
    }
    if (ctx->dirty & ctx->enable) {
        pgl_send_dirty_regs(ctx, VU1_REGS_ADDR);
        entry -= 5;
    }
    w = ctx->cur;
    attr = ctx->primAttr | GS_PRIM_TRIANGLE_STRIP;
    tag = (u32 *)w;
    w++;
    *w++ = 0;
    if (ctx->stmask != STRIP_STMASK) {
        *w++ = VIF_STMASK;
        *w++ = STRIP_STMASK;
        ctx->stmask = STRIP_STMASK;
    }
    *w++ = VIF_UNPACK_M(VIF_S_8, 0, VU1_STRIP_ST) | ((g_stripBatchCount + 1) << 16);
    *(u8 *)w = g_stripBatchCount;
    b = (u8 *)w + 1;
    for (i = 0; i < g_stripBatchCount; i++)
        *b++ = (g_stripBatchLengths[i] - 2) * 3;
    p = (u32 *)(((u32)b + 3) & 0xFFFFFFFC);
    *p++ = VIF_UNPACK(VIF_V3_32, 1, VU1_STRIP_GIFTAG);
    *p++ = g_stripBatchVerts | GIF_EOP;
    *p++ = (attr << 15) | 0x30004000;
    *p++ = GIF_REG_ST | (GIF_REG_RGBAQ << 4) | (GIF_REG_XYZF2 << 8);
    *p++ = VIF_MSCAL(entry);
    while ((u32)p & 0xF)
        *p++ = VIF_NOP;
    ctx->cur = p;
    tag[0] = DMATAG_CNT(((u32)((u8 *)p - (u8 *)tag) >> 4) - 1);
}

/* One DMA ref packet of the VU1 vertex streams: qwc quadwords at data
 * (physical address), VIF NOP + UNPACK code with NUM = num.  An inline
 * function: its arguments are evaluated before the stores (the g_stripBatchVerts
 * loads stay in front of them) and the VIF code (address | the constant)
 * is computed apart from (num << 16), as retail does. */
static inline u32 *u114_ref_unpack(u32 *w, u32 qwc, const void *data, u32 num, u32 code)
{
    w[0] = DMATAG_REF(qwc);
    w[1] = (u32)data & 0x0FFFFFFF;
    w[2] = VIF_NOP;
    w[3] = (num << 16) | code;
    return w + 4;
}
// FUNC 0x0024C6B0 strip_batch_add
// Adds n (3..64) vertices to the open batch (flushed first if they would
// not fit; the counts are padded to 4): DMA ref packets of xyz (V3-32),
// st (V2-32), rgba (V4-8) and, if the batch has a second xyz stream
// (g_stripBatchXyz2Addr), xyz2.  Addresses are physical (& 0x0FFFFFFF).
// strips = {count, lengths...}, NULL for one strip of n.  Nothing without a VU1
// program.  Callers: FlareClass::drawReflection (T159), strip_draw.
// Not exact (85%): retail copies qwcXyz into a register of its own and
// recomputes the xyz2 packet's tag from it (here the first tag is reused),
// and the temporaries' registers follow; tried the VIF code / count forms
// (inline helper, in-macro count, association orders), the counter update
// forms (this one gained 5%).
void strip_batch_add(const void *xyz, const void *xyz2, const void *rgba, const void *st,
                     s32 n, const u8 *strips)
{
    GsContext *ctx = GS_SPR_CTX;
    s32 n4, base, qwcXyz;
    u32 *w;

    if (ctx->vu1Prog == 0)
        return;
    if (n < 3)
        return;
    if (n > VU1_MAX_VERTS)
        return;
    n4 = (n + 3) & ~3;
    base = g_stripBatchVerts;
    if (base + n4 > VU1_MAX_VERTS) {
        strip_batch_end();
        g_stripBatchVerts = 0;
        g_stripBatchOpen = 1;
        base = 0;
        g_stripBatchCount = 0;
        g_0035327C = 0;
        g_stripBatchXyz2Addr = 0;
    }
    qwcXyz = (n4 * 2 + n4) >> 2;
    w = ctx->cur;
    w = u114_ref_unpack(w, qwcXyz, xyz, n4, VIF_UNPACK(VIF_V3_32, 0, base));
    if (st != NULL) {
        s32 n2 = (n + 1) & ~1;

        w = u114_ref_unpack(w, n2 >> 1, st, n2, VIF_UNPACK(VIF_V2_32, 0, g_stripBatchVerts + VU1_STRIP_ST));
    }
    if (rgba != NULL)
        w = u114_ref_unpack(w, n4 >> 2, rgba, n4, VIF_UNPACK(VIF_V4_8, 0, g_stripBatchVerts + VU1_STRIP_RGBA));
    if (xyz2 != NULL && g_stripBatchXyz2Addr != 0)
        w = u114_ref_unpack(w, qwcXyz, xyz2, n4, VIF_UNPACK(VIF_V3_32, 0, g_stripBatchXyz2Addr + g_stripBatchVerts));
    ctx->nVerts += n;
    ctx->cur = w;
    g_stripBatchVerts += n;
    if (strips != NULL) {
        int count = strips[0];
        int i;

        strips++;
        for (i = 0; i < count; i++) {
            g_stripBatchLengths[g_stripBatchCount] = strips[i];
            ctx->nPrims += strips[i] - 2;
            g_stripBatchCount++;
        }
    } else {
        ctx->nPrims += n - 2;
        g_stripBatchLengths[g_stripBatchCount] = n;
        g_stripBatchCount++;
    }
}

// FUNC 0x0024C940 strip_draw
// One strip of n vertices: into the open batch, or as a batch of its own
// (kicked at once).  Nothing while the GIF list is selected.  Callers:
// particle_draw_streak (T165), F_001AC420 (T181), spriteset_draw_part
// (T454).
void strip_draw(const void *xyz, const void *xyz2, const void *rgba, const void *st, s32 n)
{
    GsContext *ctx = GS_SPR_CTX;

    if (ctx->curList != 0)
        return;
    if (g_stripBatchOpen != 0) {
        strip_batch_add(xyz, xyz2, rgba, st, n, NULL);
        return;
    }
    g_stripBatchVerts = 0;
    g_stripBatchOpen = 1;
    g_stripBatchCount = 0;
    g_0035327C = 0;
    g_stripBatchXyz2Addr = 0;
    strip_batch_add(xyz, xyz2, rgba, st, n, NULL);
    strip_batch_end();
}

// FUNC 0x0024C9A8 tri_draw16
// n triangles (3n <= 64 vertices) from 16-bit data, kicked at once: xyzw
// V4-16 to 0, st V2-16 to 0x40 (both signed), rgba V4-8 to 0x80 (DMA ref),
// then strip lengths 3, the GIF tag (PRIM triangle | primAttr), ITOP 0x48
// and MSCAL of the VU1 program + 0x3C.  The second argument is not used.
// Caller: icon_model_draw (T077).  The kick is written as strip_batch_end's
// (the header through the $a2 register variable).
// Not exact (87%): the registers of the DMA ref part (retail: the pointer
// in $t0, the data pointers in $t1 / $t3) and the copy of nVerts that
// retail makes for the GIF tag's NLOOP; tried the ref pointer bound to $t0
// (then its stores are not folded either: worse), a volatile tail pointer.
void tri_draw16(const void *xyzw16, const void *unused, const void *rgba,
                const void *st16, s32 n)
{
    GsContext *ctx = GS_SPR_CTX;
    s32 nVerts, n2, n4;
    u32 entry;
    u32 attr;
    volatile u32 *w;
    u32 *p;
    u32 *tag;
    u8 *b;
    register u32 *h U114_IN_A2;
    int i;

    if (ctx->vu1Prog == 0)
        return;
    if (n == 0)
        return;
    nVerts = n * 3;
    if (nVerts > VU1_MAX_VERTS)
        return;
    w = ctx->cur;
    u114_pref40((void *)w);
    n2 = (nVerts + 1) & ~1;
    ctx->nParts++;
    n4 = (nVerts + 3) & ~3;
    w = u114_ref_unpack((u32 *)w, (n2 * 8) >> 4, xyzw16, n2, VIF_UNPACK_S(VIF_V4_16, 0, 0));
    if (st16 != NULL)
        w = u114_ref_unpack((u32 *)w, (n4 * 4) >> 4, st16, n4, VIF_UNPACK_S(VIF_V2_16, 0, VU1_STRIP_ST));
    if (rgba != NULL)
        w = u114_ref_unpack((u32 *)w, (n4 * 4) >> 4, rgba, n4, VIF_UNPACK(VIF_V4_8, 0, VU1_STRIP_RGBA));
    ctx->cur = (u32 *)w;
    u114_pref40((void *)w);
    if (ctx->matrixDirty)
        pgl_sync_matrices(ctx);
    entry = ctx->vu1Prog;
    if (!ctx->vu1MatrixSent) {
        pgl_send_matrix(ctx, VU1_MATRIX_ADDR);
        entry -= 1;
        ctx->vu1MatrixSent = 1;
    }
    if (ctx->dirty & ctx->enable) {
        pgl_send_dirty_regs(ctx, VU1_REGS_ADDR);
        entry -= 5;
    }
    h = ctx->cur;
    attr = ctx->primAttr | GS_PRIM_TRIANGLE;
    tag = h;
    h++;
    *h++ = 0;
    if (ctx->stmask != STRIP_STMASK) {
        *h++ = VIF_STMASK;
        *h++ = STRIP_STMASK;
        ctx->stmask = STRIP_STMASK;
    }
    *h++ = VIF_UNPACK_M(VIF_S_8, 0, VU1_STRIP_ST) | ((n + 1) << 16);
    *(u8 *)h = n;
    b = (u8 *)h + 1;
    for (i = 0; i < n; i++)
        *b++ = 3;
    p = (u32 *)(((u32)b + 3) & 0xFFFFFFFC);
    *p++ = VIF_UNPACK(VIF_V3_32, 1, VU1_STRIP_GIFTAG);
    *p++ = nVerts | GIF_EOP;
    *p++ = (attr << 15) | 0x30004000;           /* PRE, PRIM, NREG 3 */
    *p++ = GIF_REG_ST | (GIF_REG_RGBAQ << 4) | (GIF_REG_XYZF2 << 8);
    *p++ = VIF_ITOP(0x48);
    *p++ = VIF_MSCAL(entry + 0x3C);
    while ((u32)p & 0xF)
        *p++ = VIF_NOP;
    u114_pref40((void *)p);
    ctx->cur = p;
    tag[0] = DMATAG_CNT(((u32)((u8 *)p - (u8 *)tag) >> 4) - 1);
}

// FUNC 0x0024CCB0 pgl_set_part_tex_func
// GsContext.partTexFunc = fn: called by the model parts with a texture
// index (texmat_select, the tire packet's callback); NULL makes model_draw
// always take the general path.  Callers: TirePacket::bindPassCallback
// (T462), modelset_begin_draw (T451), spriteset_begin_draw (T454).
void pgl_set_part_tex_func(PglPartTexFunc fn)
{
    GS_SPR_CTX->partTexFunc = fn;
}

// FUNC 0x0024CCC0 material_load
// Loads a material record into the context's material 0 (scaled
// component-wise by g_002F72C0.matScale when enabled and the record's flag
// bit 0 is set) and flags the lighting for re-upload.  Callers:
// TirePacket::virtual_2 (T462), RaceTirePacket::virtual_2 (T449),
// texmat_select.
void material_load(const GsMaterial *m)
{
    GsLibState *st = &g_002F72C0;
    GsContext *ctx = GS_SPR_CTX;

    /* memory operands: retail addresses the quadwords from the base
     * registers (0x20($a1), 0x960($v1)) */
    __asm__ volatile(
        "vnop\n\t"
        "lqc2     $vf1, %0\n\t"
        "lqc2     $vf2, %1\n\t"
        "lqc2     $vf3, %2\n\t"
        "lqc2     $vf4, %3\n\t"
        : : "m"(m->ambient), "m"(m->diffuse), "m"(m->specular), "m"(m->emission));
    if (st->matScaleOn && (m->x44.flags & 1))
        __asm__ volatile(
            "lqc2     $vf5, %0\n\t"
            "lqc2     $vf6, %1\n\t"
            "lqc2     $vf7, %2\n\t"
            "lqc2     $vf8, %3\n\t"
            "vmul.xyzw $vf1, $vf1, $vf5\n\t"
            "vmul.xyzw $vf2, $vf2, $vf6\n\t"
            "vmul.xyzw $vf3, $vf3, $vf7\n\t"
            "vmul.xyzw $vf4, $vf4, $vf8\n\t"
            : : "m"(st->matScale[MATSCALE_AMBIENT][0]), "m"(st->matScale[MATSCALE_DIFFUSE][0]),
                "m"(st->matScale[MATSCALE_SPECULAR][0]), "m"(st->matScale[MATSCALE_EMISSION][0]));
    ctx->material[0].shininess = m->shininess;
    ctx->lightFlags |= PGL_LF_MATERIAL;
    __asm__ volatile(
        "sqc2     $vf1, %0\n\t"
        "sqc2     $vf2, %1\n\t"
        "sqc2     $vf3, %2\n\t"
        "sqc2     $vf4, %3\n\t"
        : "=m"(ctx->material[0].ambient), "=m"(ctx->material[0].diffuse),
          "=m"(ctx->material[0].specular), "=m"(ctx->material[0].emission));
}

// FUNC 0x0024CD48 gfx_set_texture_regs
// The bootstrap's gfx_set_texture_regs (0x0100BE10), with the override
// block at the head of g_002F72C0 (disabled: enable = 0): loads CLAMP,
// TEX0 (CLD stripped), TEX1 into the context shadows (dirty in both
// contexts); TEX0 with CLD != 0 also goes to TEX2 (PSM forced to PSMT8,
// CSA cleared when CSA != 0) to load the CLUT; with mipmapping TEX1.K gets
// the context's LOD bias added and MIPTBP1 (MIPTBP2 from MXL >= 4) are
// loaded.  19 callers.  The bootstrap's source shape (o inside the test, the
// mask complements as statements, the `not` as an asm) is exact here too.
void gfx_set_texture_regs(const GsTexRegs *t)
{
    GsContext *ctx = GS_SPR_CTX;
    u64 tex0 = t->tex0;
    u64 tex1 = t->tex1;
    u64 clamp = t->clamp;
    u64 dirty = ctx->dirty;
    s32 mxl4;

    if (g_002F72C0.enable) {
        GsLibState *o = &g_002F72C0;

        tex0 &= ~o->tex0Mask;
        tex0 |= ctx->reg[GS_REG_TEX0] & o->tex0Mask;
        tex1 &= ~o->tex1Mask;
        tex1 |= ctx->reg[GS_REG_TEX1] & o->tex1Mask;
        clamp &= ~o->clampMask;
        clamp |= ctx->reg[GS_REG_CLAMP] & o->clampMask;
    }
    ctx->reg[GS_REG_CLAMP] = clamp;
    ctx->reg[GS_REG_TEX0] = tex0 & (~0ull >> 3);        /* without CLD */
    dirty |= GS_DIRTY_REG_BOTH(GS_REG_CLAMP) | GS_DIRTY_REG_BOTH(GS_REG_TEX1) |
             GS_DIRTY_REG_BOTH(GS_REG_TEX0);
    ctx->reg[GS_REG_TEX1] = tex1;
    if ((tex0 >> 61) != 0) {                            /* TEX0.CLD */
        if (((tex0 >> 56) & 0x1F) != 0) {               /* TEX0.CSA */
            tex0 &= u114_not64(0x1F00000003F00000ull);  /* CSA, PSM */
            tex0 |= (u64)GS_PSMT8 << 20;
        }
        ctx->reg[GS_REG_TEX2] = tex0;
        dirty |= GS_DIRTY_REG_BOTH(GS_REG_TEX2);
    }
    mxl4 = (s32)(tex1 & 0x1C);                          /* TEX1.MXL << 2 */
    if (mxl4 != 0) {
        s32 k = (s32)((tex1 >> 32) & 0xFFF);            /* TEX1.K, signed 7.4 */
        s32 l = (s32)((tex1 >> 19) & 3);                /* TEX1.L */

        k = (s32)((u32)k << 20) >> 16;
        k += ctx->lodBias << l;
        k >>= 4;
        ctx->reg[GS_REG_TEX1] = (ctx->reg[GS_REG_TEX1] & 0xFFFFF000FFFFFFFFull) |
                                ((u64)(k & 0xFFF) << 32);
        dirty |= GS_DIRTY_REG_BOTH(GS_REG_MIPTBP1);
        ctx->reg[GS_REG_MIPTBP1] = t->miptbp1;
        if ((u32)mxl4 >= 0xD) {                         /* MXL >= 4 */
            dirty |= GS_DIRTY_REG_BOTH(GS_REG_MIPTBP2);
            ctx->reg[GS_REG_MIPTBP2] = t->miptbp2;
        }
    }
    ctx->dirty = dirty;
}

/* The texture / material selection: the indices initialised (.sdata), the
 * tables file statics (.sbss, after the strip batcher's). */
s32 g_texselTex = 0;                        /* 0x003530B8 gp-0x4938, current texture index */
s32 g_texselMat = 0;                        /* 0x003530BC gp-0x4934, current material index */
s32 g_texselTexBase2 = 0;                   /* 0x003530C0 gp-0x4930, texture index into table 2 */
s32 g_texselMatBase2 = 0;                   /* 0x003530C4 gp-0x492C, material index into table 2 */
static GsTexRegs  *g_texselTexTable;        /* 0x00353284 gp-0x476C, texture table (indices 1..0x1FE) */
static GsMaterial *g_texselMatTable;        /* 0x00353288 gp-0x4768, material table (indices 1..0x7E) */
static GsTexRegs  *g_texselTexTable2;       /* 0x0035328C gp-0x4764, texture table 2 */
static GsMaterial *g_texselMatTable2;       /* 0x00353290 gp-0x4760, material table 2 */

// FUNC 0x0024CED0 texsel_reset
// Resets the texture / material selection.  No callers (dead code).
void texsel_reset(void)
{
    g_texselMat = 0;
    g_texselTex = 0;
    g_texselTexBase2 = 0;
    g_texselMatBase2 = 0;
}

// FUNC 0x0024CEE8 texsel_get_table2
GsTexRegs *texsel_get_table2(void)
{
    return g_texselTexTable2;
}

// FUNC 0x0024CEF0 matsel_set_base
void matsel_set_base(s32 base)
{
    g_texselMatBase2 = base;
}

// FUNC 0x0024CEF8 matsel_set_table2
void matsel_set_table2(GsMaterial *t)
{
    g_texselMatTable2 = t;
    g_texselMatBase2 = 0;
    g_texselMat = 0;
}

// FUNC 0x0024CF08 matsel_set_table
void matsel_set_table(GsMaterial *t)
{
    g_texselMatTable = t;
    g_texselMat = 0;
}

// FUNC 0x0024CF18 texsel_set_base
void texsel_set_base(s32 base)
{
    g_texselTexBase2 = base;
}

// FUNC 0x0024CF20 texsel_set_table2
void texsel_set_table2(GsTexRegs *t)
{
    g_texselTexTable2 = t;
    g_texselTexBase2 = 0;
    g_texselTex = 0;
}

// FUNC 0x0024CF30 texsel_set_table
void texsel_set_table(GsTexRegs *t)
{
    g_texselTexTable = t;
    g_texselTex = 0;
}

// FUNC 0x0024CF40 texmat_select
// id bits 0-8 = texture index (1-based in table 1; 0x1FF = entry
// g_texselTexBase2 of table 2), bits 9-15 = material index (1-based in
// table 1; 0x7F = entry g_texselMatBase2 of table 2).  0 keeps the current
// one; a repeated index is not reloaded.  The material's "0x1FF" test
// compares a 7-bit value with 0x1FF (never true: a source bug, the
// texture's test copied), as in the code; so a material 0x7F is remembered
// as 0x7F, not base + 0x1FF.  A PglPartTexFunc; caller: spriteset_draw_part
// (T454).
void texmat_select(s32 id)
{
    s32 mat = (id >> 9) & 0x7F;
    s32 tex = id & 0x1FF;

    if (tex == 0x1FF)
        tex = g_texselTexBase2 + 0x1FF;
    if (tex != 0 && tex != g_texselTex) {
        if (tex >= 0x1FF) {
            if (g_texselTexTable2 != NULL)
                gfx_set_texture_regs(&g_texselTexTable2[g_texselTexBase2]);
        } else if (g_texselTexTable != NULL) {
            gfx_set_texture_regs(g_texselTexTable + tex - 1);
        }
        g_texselTex = tex;
    }
    if (mat == 0x1FF)                           /* dead: mat <= 0x7F */
        mat = g_texselMatBase2 + 0x1FF;
    if (mat != 0 && mat != g_texselMat) {
        if (mat >= 0x7F) {
            if (g_texselMatTable2 != NULL)
                material_load(&g_texselMatTable2[g_texselMatBase2]);
        } else if (g_texselMatTable != NULL) {
            material_load(g_texselMatTable + mat - 1);
        }
        g_texselMat = mat;
    }
}

// FUNC 0x0024D048 model_draw
// Draws a prebuilt VU1 model (if a VU1 program is selected and it has
// parts): the general path vifmodel_draw_lit when the model is lit (mode
// != 0), the texture override is on or no part-texture function is set,
// else the fast path vifmodel_draw.  7 callers.
void model_draw(VifModel *m)
{
    GsContext *ctx = GS_SPR_CTX;
    GsLibState *st = &g_002F72C0;
    u32 prog = ctx->vu1Prog;

    if (prog == 0)
        return;
    if (m->nParts == 0)
        return;
    if (m->mode != 0 || st->enable != 0 || ctx->partTexFunc == NULL)
        vifmodel_draw_lit(m, prog);
    else
        vifmodel_draw(m, prog);
}

// FUNC 0x0024D0C0 vu1_unpack_xyz
// Reserves a DMA cnt packet UNPACK V3-32 of n (rounded up to 4) vertices to
// VU1 address 0 and returns its data area.  Callers:
// RaceTirePacket::setDiscVertices / setCaliperVertices (T449).
u32 *vu1_unpack_xyz(s32 n)
{
    GsContext *ctx = GS_SPR_CTX;
    s32 n4 = (n + 3) & ~3;
    s32 words = n4 * 2 + n4;
    u32 *cur = ctx->cur;

    cur[0] = DMATAG_CNT(words >> 2);
    ctx->cur = cur + 4 + words;
    cur[3] = (n4 << 16) | VIF_UNPACK(VIF_V3_32, 0, 0);
    cur[1] = 0;
    cur[2] = VIF_NOP;
    return cur + 4;
}
