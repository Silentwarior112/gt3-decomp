/* gs_driver_core.c - the GS driver core of the graphics library (G001),
 * 0x00250208..0x00250A58; the same machine code as the bootstrap's
 * src/gs_driver.cpp (0x0100BF98.., 0x0100E570) apart from the COP0
 * performance counter reads, which the bootstrap does not have.
 *
 * Core addresses of the bootstrap's data: g_gfx 0x004A69C0,
 * s_gfxInitialized = g_gfxInitialized 0x003530D4, the VU1 microcode
 * 0x002F7440 (scrambled), the VU0 microcode 0x00349600, g_010384D8 =
 * g_pglAttribStack 0x002FB1E8, g_gsVuWork = g_pglAttribSp 0x003530DC (the
 * attribute stack pointer of pgl_push_attrib / pgl_pop_attrib, u117).
 */
#include "engine/pgl/G001_flare.h"
#include "engine/pgl/pgl_vu1_internal.h"

/* ======================================================================== */
/* Data                                                                     */
/* ======================================================================== */

/* .data 0x002F7440..0x002FC1E8, both initialised, so in definition order.
 * First the VU1 microcode, 0x3DA8 bytes as the original stores them
 * (scrambled; gfx_init descrambles them in place before the upload, so
 * not const).  The bootstrap has the same blob between its gs_texture and
 * gs_driver objects; here it opens this file's .data range.  Quadword
 * aligned for the DMA (0x002F7440 is). */
u8 g_gsVu1Microcode[0x3DA8] __attribute__((aligned(16))) = {   /* 0x002F7440 */
#include "vu1_microcode.inc"
};
/* The byte stack of pgl_push_attrib / pgl_pop_attrib (u117) that
 * g_pglAttribSp walks.  Explicitly initialised (zeros): .data at its
 * definition, where a C global without an initialiser would be a common. */
u8 g_pglAttribStack[0x1000] = { 0 };            /* 0x002FB1E8 */

/* .sdata 0x003530D4..0x003530E0: g_gfxInitialized; FLT_MAX (0x7F7FFFFF,
 * the constant-pool literal of gfx_init_vu0_constants, written in front of
 * that function); g_pglAttribSp, defined after it. */
u8 g_gfxInitialized = 0;                        /* 0x003530D4 */

/* The driver state.  In the original a file static of the driver object
 * (it sits in the middle of the .bss, where commons are not); u116's and
 * u118's parts of the driver use it too, so here a C common (the full link
 * puts it at its address). */
GfxDriverState g_gfx;                           /* 0x004A69C0 */

/* Header qword of a list-0 block: DMA cnt tag (qwc) + VIF FLUSH + VIF
 * DIRECT (qwc) (the bootstrap's inline gs_put_cnt_direct). */
static inline void gs_put_cnt_direct(u32 *p, u32 qwc)
{
    p[0] = DMATAG_CNT(qwc);
    p[2] = VIF_FLUSH;
    p[3] = VIF_DIRECT(qwc);
    p[1] = 0;
}

// FUNC 0x00250208 gs_flush_dirty_regs
/* Without an active packet just forgets the dirty set.  Otherwise collects
 * the dirty & enabled shadows as A+D pairs at cur+0x20 and, if any, wraps
 * them: DMA cnt (n+1) + VIF FLUSH + DIRECT (n+1) + GIFtag (NLOOP n, EOP,
 * PACKED, NREG 1, A+D); cur += 0x20 + n*16.  (25 callers.)  The
 * bootstrap's source (gs_driver.cpp 0x0100E570). */
void gs_flush_dirty_regs(void)
{
    GsContext *ctx = GS_SPR_CTX;
    u32 *p;
    int n;

    if (!(ctx->packetOpen || ctx->subPacketOpen)) {     /* ld +0x1B8 & 0xFFFF000000000000 */
        ctx->dirty = 0;
        return;
    }
    p = ctx->cur;
    n = gs_collect_dirty_regs(ctx, (u64 *)(p + 8));
    if (n == 0)
        return;
    gs_put_cnt_direct(p, n + 1);
    do {                                                /* GIFtag NLOOP n, EOP, A+D */
        U115_PKT64(p, 4) = (u64)(u32)((u32)n | GIF_EOP) | GIF_NREG(1);
        U115_PKT64(p, 6) = GIF_REG_AD;
    } while (0);
    ctx->cur = (u32 *)((u128 *)p + (n + 2));
}

// FUNC 0x002502C8 gfx_perf_counter0
/* The CPU-cycle count: the per-vblank accumulator g_gfx.perfCount0 plus
 * COP0 PCR0 (mfpc, sign-extended); read twice, and if the accumulator
 * changed in between (a vblank) the second pair is used.  Callers (8, all
 * seeding a random generator from the clock): the cameras' slot-23
 * methods (AutomaticCamera, AutomountCamera, MatrixCamera, ZoomupCamera),
 * CameraRotDamper::init, CameraManager::init, CameraGrooveMeter::reset,
 * F_0020E020 (TargetChooser). */
u64 gfx_perf_counter0(void)
{
    u64 base;
    u64 base2;
    s64 count;                  /* mfpc: the counter sign-extended */
    s64 count2;

    base = g_gfx.perfCount0;
    __asm__ __volatile__("mfpc         %0, 0" : "=r"(count));
    base2 = g_gfx.perfCount0;
    __asm__ __volatile__("mfpc         %0, 0" : "=r"(count2));
    if (base != base2) {
        base = base2;
        count = count2;
    }
    return base + count;
}

// FUNC 0x002502F8 gfx_perf_counter1
/* The same for perfCount1 and PCR1 (counter 1, event 6 - see gfx_init).
 * No callers (jal targets). */
u64 gfx_perf_counter1(void)
{
    u64 base;
    u64 base2;
    s64 count;                  /* mfpc: the counter sign-extended */
    s64 count2;

    base = g_gfx.perfCount1;
    __asm__ __volatile__("mfpc         %0, 1" : "=r"(count));
    base2 = g_gfx.perfCount1;
    __asm__ __volatile__("mfpc         %0, 1" : "=r"(count2));
    if (base != base2) {
        base = base2;
        count = count2;
    }
    return base + count;
}

// FUNC 0x00250328 gfx_wait_vsync_poll
/* Polling vblank wait: writes CSR.VSINT (8) to clear the event, spins until
 * CSR bit 3 is set again or 0x3A980 polls elapsed, then clears it again. */
void gfx_wait_vsync_poll(void)
{
    u32 polls = 0x3A980;

    GS_CSR_REG = GS_CSR_VSINT;
    for (;;) {
        u64 csr = GS_CSR_REG;
        polls--;                                /* decremented in the branch delay slot */
        if (csr & GS_CSR_VSINT)
            break;
        if (polls == 0)
            break;
    }
    GS_CSR_REG = GS_CSR_VSINT;
}

// FUNC 0x00250378 gfx_reset_paths
/* Resets VIF0, VIF1 (FBRST.RST), VU0 and VU1 (COP2 control reg vi28 = FBRST
 * |= RS0|RS1 = 0x202), the GIF (GIF_CTRL.RST, GIF_MODE = IMT) and sets
 * VIF0_ERR/VIF1_ERR = 6 (ME0|ME1). */
void gfx_reset_paths(void)
{
    u32 fbrst;

    VIF0_FBRST_REG = VIF_FBRST_RST;
    VIF1_FBRST_REG = VIF_FBRST_RST;
    __asm__ __volatile__(
        "cfc2.ni      %0, $vi28\n\t"
        "ori          %0, %0, 0x202\n\t"        /* VU_FBRST_RS0_RS1 */
        "ctc2.ni      %0, $vi28\n\t"
        : "=&r"(fbrst) : : "memory");
    GIF_CTRL_REG = GIF_CTRL_RST;
    GIF_MODE_REG = GIF_MODE_IMT;
    VIF0_ERR_REG = VIF_ERR_ME0_ME1;
    VIF1_ERR_REG = VIF_ERR_ME0_ME1;
}

// FUNC 0x002503E0 gfx_init_vu0_constants
/* vu0_load_identity_regs() puts an identity matrix in vf28..vf31; this sets
 * the VU0 matrix stack pointers vi11 = 0x10, vi12 = 0x50, vi13 = 0x60
 * (modelview, projection, texture) and parks the identity in VU0 data
 * memory qwords 4-7, 8-11 and 12-15 (the modelview, projection and
 * texture matrices).  Then the context's constant floats and the
 * attribute stack pointer g_pglAttribSp = g_pglAttribStack (the
 * bootstrap's g_gsVuWork = g_010384D8). */
void gfx_init_vu0_constants(GsContext *ctx)
{
    vu0_load_identity_regs();

    __asm__ __volatile__("ctc2.ni      %0, $vi11" : : "r"(0x10));
    __asm__ __volatile__("ctc2.ni      %0, $vi12" : : "r"(0x50));
    __asm__ __volatile__("ctc2.ni      %0, $vi13" : : "r"(0x60));
    __asm__ __volatile__(
        "ctc2.i       %0, $vi10\n\t"
        "vsqi.xyzw    $vf28, ($vi10++)\n\t"
        "vsqi.xyzw    $vf29, ($vi10++)\n\t"
        "vsqi.xyzw    $vf30, ($vi10++)\n\t"
        "vsqi.xyzw    $vf31, ($vi10++)\n\t"
        "vnop\n\t"
        "vnop" : : "r"(VU0_MODELVIEW_QW) : "memory");
    __asm__ __volatile__(
        "ctc2.i       %0, $vi10\n\t"
        "vsqi.xyzw    $vf28, ($vi10++)\n\t"
        "vsqi.xyzw    $vf29, ($vi10++)\n\t"
        "vsqi.xyzw    $vf30, ($vi10++)\n\t"
        "vsqi.xyzw    $vf31, ($vi10++)\n\t"
        "vnop\n\t"
        "vnop" : : "r"(VU0_PROJECTION_QW) : "memory");
    __asm__ __volatile__(
        "ctc2.i       %0, $vi10\n\t"
        "vsqi.xyzw    $vf28, ($vi10++)\n\t"
        "vsqi.xyzw    $vf29, ($vi10++)\n\t"
        "vsqi.xyzw    $vf30, ($vi10++)\n\t"
        "vsqi.xyzw    $vf31, ($vi10++)\n\t"
        "vnop\n\t"
        "vnop" : : "r"(VU0_TEXTURE_QW) : "memory");

    ctx->matrixDirty = 1;
    ctx->matrixMode = PGL_MODELVIEW;
    ctx->vu1MatrixSent = 0;
    ctx->limits.x = GS_F_FLT_MAX;
    ctx->limits.y = GS_F_FLT_MAX;
    ctx->limits.z = GS_F_FLT_MAX;
    ctx->limits.w = GS_F_FLT_MAX;
    ctx->viewCenter.w = 255.0f;
    ctx->vBB0.x = 0.5f;
    ctx->vBB0.y = 0.5f;
    ctx->vBB0.z = 1.0f;
    ctx->vBB0.w = 1.0f;
    ctx->fogStart = 0.0f;                       /* sw $zero */
    ctx->fogAmount = 1.0f;
    g_pglAttribSp = g_pglAttribStack;
}

/* .sdata, after gfx_init_vu0_constants's FLT_MAX: the attribute stack
 * pointer (G001_flare.h's extern; u117's pgl_push_attrib / pgl_pop_attrib
 * move it), zero until the line above sets it. */
u8 *g_pglAttribSp = NULL;                       /* 0x003530DC */

// FUNC 0x002504E8 gfx_init_lights
/* Clears the 8 lights (memset ctx->lights, 0x680) and gives light 0 the
 * OpenGL GL_LIGHT0 defaults (white diffuse / specular, position (0, 0, 1,
 * 0), spot direction (0, 0, -1), cutoff 180 = no spot, exponent 0; but
 * quadAtten 1.0 where OpenGL has 0); eyeWorld {0,0,0,1}, the global
 * ambient {0.2,0.2,0.2,1}, all lighting flags dirty, lightMode = 2.
 * Quirk kept: the 0.0f used for several fields is obtained by storing 0
 * to the linearAtten of light 0 and reading it back (sw / lwc1). */
void gfx_init_lights(GsContext *ctx)
{
    f32 zero;

    memset(ctx->lights, 0, sizeof(ctx->lights));
    ctx->lights[0].linearAtten = 0.0f;          /* stored (sw $0) and read back */
    zero = ctx->lights[0].linearAtten;
    ctx->lights[0].ambient.w  = 1.0f;
    ctx->lights[0].diffuse.x  = 1.0f;
    ctx->lights[0].diffuse.y  = 1.0f;
    ctx->lights[0].diffuse.z  = 1.0f;
    ctx->lights[0].diffuse.w  = 1.0f;
    ctx->lights[0].specular.x = 1.0f;
    ctx->lights[0].specular.y = 1.0f;
    ctx->lights[0].specular.z = 1.0f;
    ctx->lights[0].specular.w = 1.0f;
    ctx->lights[0].position.z = 1.0f;
    ctx->lights[0].constAtten = 1.0f;
    ctx->lights[0].quadAtten  = 1.0f;
    ctx->lights[0].spotDir.z  = -1.0f;
    ctx->lights[0].spotCutoff = 180.0f;
    ctx->lights[0].spotExpEnc = float_to_gs_param_encode(zero);
    ctx->globalAmbient.x = GS_F_0_2;            /* .lit4 0x00350A78 */
    ctx->globalAmbient.y = GS_F_0_2;
    ctx->globalAmbient.z = GS_F_0_2;
    ctx->globalAmbient.w = 1.0f;
    ctx->eyeWorld.x = zero;
    ctx->eyeWorld.y = zero;
    ctx->eyeWorld.z = zero;
    ctx->eyeWorld.w = 1.0f;
    ctx->lightFlags = PGL_LF_DIRTY;
    ctx->lightMode = 2;
}

// FUNC 0x002505C0 gfx_init_material
/* Clears both material blocks (memset ctx->material, 0xA0) and sets the
 * OpenGL material defaults on face 0: ambient {0.2,0.2,0.2,1}, diffuse
 * {0.8,0.8,0.8,1}, specular / emission w = 1, shininessEnc = encode(0.0).
 * Also qAF0 = qB00 = {1,1,1,1}, qB10 = 0, and colorMaterial off with
 * kind 2 (ambient + diffuse, as glColorMaterial's default) on face mask
 * 1. */
void gfx_init_material(GsContext *ctx)
{
    memset(ctx->material, 0, sizeof(ctx->material));
    ctx->material[0].ambient.x  = GS_F_0_2;     /* .lit4 0x00350A7C */
    ctx->material[0].ambient.y  = GS_F_0_2;
    ctx->material[0].ambient.z  = GS_F_0_2;
    ctx->material[0].ambient.w  = 1.0f;
    ctx->material[0].diffuse.x  = GS_F_0_8;     /* .lit4 0x00350A80 */
    ctx->material[0].diffuse.y  = GS_F_0_8;
    ctx->material[0].diffuse.z  = GS_F_0_8;
    ctx->material[0].diffuse.w  = 1.0f;
    ctx->material[0].specular.w = 1.0f;
    ctx->material[0].emission.w = 1.0f;
    ctx->material[0].shininessEnc = float_to_gs_param_encode(0.0f);
    ctx->qAF0.x = 1.0f;
    ctx->qAF0.y = 1.0f;
    ctx->qAF0.z = 1.0f;
    ctx->qAF0.w = 1.0f;
    ctx->qB00.x = 1.0f;
    ctx->qB00.y = 1.0f;
    ctx->qB00.z = 1.0f;
    ctx->qB00.w = 1.0f;
    ctx->colorMaterialKind = 2;                       /* gs_set_material_param kind 2: ambient + diffuse */
    ctx->colorMaterialFaces = 1;                       /* face 0 */
    ctx->colorMaterial = 0;
    ctx->qB10.x = 0.0f;
    ctx->qB10.y = 0.0f;
    ctx->qB10.z = 0.0f;
    ctx->qB10.w = 0.0f;
}

// FUNC 0x00250690 gfx_reset_texture_state
/* Default texture state: TEXA {TA0 0, AEM 0, TA1 0x80}; TEX0 = TCC;
 * TEX1 = MMAG / MMIN linear; TEX2 = PSMT8 with CLD 1; MIPTBP1/2 and
 * CLAMP = 0.  Dirty |= TEXA + CLAMP / TEX1 / TEX0 / MIPTBP1 / MIPTBP2 in
 * both contexts.  Quirk: TEX2 is reset but not marked dirty. */
void gfx_reset_texture_state(GsContext *ctx)
{
    u8 *texa = (u8 *)&ctx->gen[GS_GEN_TEXA];

    texa[0] = 0;                                    /* TA0 */
    texa[4] = GS_TEXA_TA1_DEFAULT;                  /* TA1 */
    ctx->gen[GS_GEN_TEXA] &= ~GS_TEXA_AEM;
    ctx->dirty |= GS_DIRTY_TEX_STATE;
    ctx->reg[GS_REG_TEX0] = GS_TEX0_TCC;
    ctx->reg[GS_REG_TEX1] = GS_TEX1_DEFAULT;
    ctx->reg[GS_REG_TEX2] = GS_TEX2_DEFAULT;
    ctx->reg[GS_REG_MIPTBP2] = 0;
    ctx->reg[GS_REG_CLAMP] = 0;
    ctx->reg[GS_REG_MIPTBP1] = 0;
}

// FUNC 0x00250708 gfx_context_init
/* Master context init: memset(ctx, 0, 0xBC0); COLCLAMP.CLAMP = 1,
 * PRMODECONT.AC = 1, ZBUF.ZMSK = 0, everything dirty, PRIM attr = IIP,
 * depth range 0..1, color 0xFFFFFFFF, colorTex 0x80808080, default
 * normal (0, 0, 1), cullFace 2, vu1ProgAlt 1, primitive program 0x33E; then
 * texture state, VU0 constants, lights and (tail call) material. */
void gfx_context_init(GsContext *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->gen[GS_GEN_COLCLAMP] |= GS_COLCLAMP_CLAMP;
    ctx->gen[GS_GEN_PRMODECONT] |= GS_PRMODECONT_AC;
    ctx->reg[GS_REG_ZBUF] &= ~GS_ZBUF_ZMSK;
    ctx->uncachedBuf = 0;
    ctx->depthFar = 1.0f;
    ctx->dirty = GS_DIRTY_ALL;
    ctx->primAttr = GS_PRIM_IIP;
    ctx->depthNear = 0.0f;                      /* sw $zero */
    ctx->normal[0] = 0.0f;                      /* sw $zero */
    ctx->color = 0xFFFFFFFF;
    ctx->normal[1] = 0.0f;                      /* sw $zero */
    ctx->fB4C = 0;
    ctx->frontFace = 0;
    ctx->cullEnable = 0;
    ctx->colorTex = 0x80808080;
    ctx->normal[2] = 1.0f;
    ctx->cullFace = 2;
    ctx->vu1ProgAlt = 1;
    ctx->vu1Prog = VU1_PRIM_PROG_DEFAULT;
    gfx_reset_texture_state(ctx);
    gfx_init_vu0_constants(ctx);
    gfx_init_lights(ctx);
    gfx_init_material(ctx);                     /* tail call */
}

// FUNC 0x00250808 gfx_init_default
/* gfx_init(fullReset 1, interlace 1, omode, frame mode 1) - tail jump.
 * Called from game_init. */
void gfx_init_default(int omode)
{
    gfx_init(1, 1, omode, 1);
}

// FUNC 0x00250820 gfx_init
/* One-shot driver init (guard g_gfxInitialized).  fullReset (low byte)
 * selects gfx_wait_vsync_poll + sceGsResetGraph(0, ...) versus a plain
 * CSR.FLUSH.  GS IMR = ~0x200 (only FINISH unmasked), display 640x448,
 * VIF/GIF/VU reset, DMA channels, the VU1 microcode descrambled, INTC
 * handlers for VBLANK_S / VIF1 / GS, the scratchpad context, VU0
 * microcode (0x398 bytes) and VU1 microcode (0x3DA8 bytes) uploaded to
 * VU address 0, COP0 PCCR = PCCR_GFX. */
void gfx_init(int fullReset, int inter, int omode, int ffmd)
{
    GsContext *ctx = GS_SPR_CTX;
    GfxDriverState *g = &g_gfx;
    u8 doReset = (u8)fullReset;

    if (g_gfxInitialized == 1)
        return;

    memset((void *)g, 0, sizeof(*g));
    gfx_store_display_mode(inter, omode, ffmd);
    if (doReset) {
        gfx_wait_vsync_poll();
        sceGsResetGraph(0, (short)inter, (short)omode, (short)ffmd);
    } else {
        GS_CSR_REG = GS_CSR_FLUSH;
    }
    g->gsRevision = (u8)((GS_CSR_REG >> 16) & 0xFF);
    GFX_SET_IMR(GS_IMR_FINISH_ONLY);
    GS_CSR_REG = GS_CSR_FINISH;                 /* clear FINISH */
    gfx_set_screen_size(640, 448);
    gfx_set_display_area(0, 0, 640, 448);
    gfx_reset_paths();
    gfx_dma_init();
    xor_unscramble_with_lcg(g_gsVu1Microcode, sizeof(g_gsVu1Microcode));
    g->vsyncCallback = NULL;
    g->intcIdVblank = AddIntcHandler(INTC_VBLANK_S, intc_vblank_handler, 0);
    g->intcIdVif1   = AddIntcHandler(INTC_VIF1, intc_vif1_handler, 0);
    g->intcIdGs     = AddIntcHandler(INTC_GS, intc_gs_finish_handler, 0);
    EnableIntc(INTC_VBLANK_S);
    EnableIntc(INTC_VIF1);
    EnableIntc(INTC_GS);
    gfx_context_init(ctx);
    vu_upload_microcode(0, 0, sizeof(g_gsVu0Microcode), g_gsVu0Microcode);
    vu_upload_microcode(1, 0, sizeof(g_gsVu1Microcode), g_gsVu1Microcode);

    /* COP0 performance counters: PCCR = 0; PCR0 = PCR1 = 0; PCCR = PCCR_GFX. */
    __asm__ __volatile__(
        "mtps         $0, 0\n\t"
        "sync.p\n\t"
        "mtpc         $0, 0\n\t"
        "mtpc         $0, 1\n\t"
        "mtps         %0, 0\n\t"
        "sync.p\n\t"
        : : "r"(PCCR_GFX) : "memory");

    g_gfxInitialized = 1;
}

// FUNC 0x00250A30 gfx_context_init_spr
/* gfx_context_init on the scratchpad context.  No callers in the call
 * graph. */
void gfx_context_init_spr(void)
{
    gfx_context_init(GS_SPR_CTX);
}
