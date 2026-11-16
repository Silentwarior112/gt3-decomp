/* libgraph.c - the two Sony libgraph (PsIIlibgraph2221) functions
 * linked into the core, the last code of .text: 0x0028E5C0..0x0028E760.
 * The same machine code as the bootstrap's sdk/libgraph/libgraph.c, where
 * each sits in its own named section (.text.sceGsGetGParam /
 * .text.sceGsResetGraph).  (ps2sdk's libgraph is another API, graph_*:
 * nothing to take from it but the GS CSR / IMR bit layout of libgs.h.)
 *
 * Data: the object's .data (0x002FC200..0x002FC220) lies in link order
 * between u116's GS driver object (g_gsDepthFuncZtst 0x002FC1F8) and u118's
 * register tables (g_gsRegsGeneral 0x002FC220), as in the bootstrap: the
 * object was linked there, only its function-section code went to the end
 * of .text.  The version tag (unreferenced) and the sceGsGParam instance,
 * file statics as the bootstrap's.
 */
// LINK_AFTER src/engine/pgl/gs_display_driver.c
// DATA_START 0x002FC200
#include "runtime/newlib/G007_stdio.h"
#include "sdk/libkernl/deci2_libgraph_internal.h"

/* Library version tag: exactly 16 chars, no NUL (sceGsGParam follows). */
static BOOT_USED char s_libgraph_version[16] = "PsIIlibgraph2221";   /* 0x002FC200 */

/* Global GS parameters (sceGsGetGParam's). */
static sceGsGParam s_gs_gparam = {                                  /* 0x002FC210 */
    SCE_GS_INTERLACE,   /* +0x00 sceGsInterMode = 1 */
    SCE_GS_NTSC,        /* +0x02 sceGsOutMode   = 2 */
    SCE_GS_FRAME,       /* +0x04 sceGsFFMode    = 1 */
    3,                  /* +0x06 sceGsVersion   = 3 */
    NULL,               /* +0x08 sceGsVSCfunc (no v-sync callback installed) */
    0,                  /* +0x0C sceGsVSCid */
};

// FUNC 0x0028E5C0 sceGsGetGParam
/* Returns the address of the global sceGsGParam (0x002FC210). */
__attribute__((section(".text.sceGsGetGParam")))
sceGsGParam *sceGsGetGParam(void)
{
    return &s_gs_gparam;
}

// FUNC 0x0028E5D0 sceGsResetGraph
/* Sony libgraph sceGsResetGraph (arguments are shorts, sign-extended on entry).
 *   mode 0: GS CSR = RESET (0x200); record inter / omode, then the GS revision
 *           (CSR bits 23:16, read back after the reset); GsPutIMR(0xFF00); record
 *           ffmode as (ffmode != 0); if a v-sync callback is installed, disable
 *           INTC_VBLANK_S, remove its handler and clear the callback + id; then
 *           tail-call SetGsCrt(inter & 1, omode & 0xFF, ffmode & 1).
 *   mode 1: GS CSR = FLUSH (0x100) only.
 *   mode 5: record ffmode / inter / omode / revision (no reset, no IMR change, no
 *           handler removal) and tail-call SetGsCrt the same way.
 *   other : nothing.
 * The original returns no meaningful value (v0 is left as SetGsCrt's result, or
 * scratch) and no caller reads it.  Only caller: gfx_init (0x00250820). */
__attribute__((section(".text.sceGsResetGraph")))
void sceGsResetGraph(short mode, short inter, short omode, short ffmode)
{
    sceGsGParam *gparam;
    u64          csr;

    switch (mode) {
    case SCE_GS_INIT_RESET:                                 /* 0 */
        gparam = sceGsGetGParam();
        REG64(GS_CSR) = GS_CSR_RESET;                       /* 0x12001000 = 0x200 */
        gparam->sceGsInterMode = inter;
        csr = REG64(GS_CSR);
        gparam->sceGsOutMode = omode;
        gparam->sceGsVersion = (u16)((csr >> GS_CSR_REV_SHIFT) & GS_CSR_REV_MASK);
        GsPutIMR(GS_IMR_MASK_ALL);                          /* 0xFF00 */
        gparam->sceGsFFMode = (ffmode != 0);
        if (gparam->sceGsVSCfunc != NULL) {
            DisableIntc(INTC_VBLANK_S);
            RemoveIntcHandler(INTC_VBLANK_S, gparam->sceGsVSCid);
            gparam->sceGsVSCfunc = NULL;    /* egcs swaps these two stores: */
            gparam->sceGsVSCid = 0;         /* retail clears +0x0C, then +0x08 */
        }
        SetGsCrt(inter & 1, omode & 0xFF, ffmode & 1);      /* tail call (j 0x00285860) */
        return;

    case SCE_GS_RESET_FLUSH:                                /* 1 */
        REG64(GS_CSR) = GS_CSR_FLUSH;                       /* 0x12001000 = 0x100 */
        return;

    case SCE_GS_RESET_MODE:                                 /* 5 */
        gparam = sceGsGetGParam();
        csr = REG64(GS_CSR);
        gparam->sceGsFFMode = (ffmode != 0);
        gparam->sceGsInterMode = inter;
        gparam->sceGsOutMode = omode;
        gparam->sceGsVersion = (u16)((csr >> GS_CSR_REV_SHIFT) & GS_CSR_REV_MASK);
        SetGsCrt(inter & 1, omode & 0xFF, ffmode & 1);      /* tail call (j 0x00285860) */
        return;

    default:
        return;
    }
}
