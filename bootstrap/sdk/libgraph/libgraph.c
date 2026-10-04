/* libgraph.c - the two Sony EE libgraph (PsIIlibgraph2221) functions linked into
 * the GT3 US bootstrap (BOOTSTRAP_SCUS-97102).
 *
 * Both were built with -ffunction-sections and land in their own named sections
 * at the very end of .text:
 *   .text.sceGsGetGParam   0x010269E8
 *   .text.sceGsResetGraph  0x010269F8
 * Their data sit in the middle of the gs driver's .data tables:
 *   'PsIIlibgraph2221' version tag @0x010394F0 (16 chars, no NUL - GParam follows)
 *   sceGsGParam instance           @0x01039500, initial {1, 2, 1, 3, 0, 0}
 * Only caller of sceGsResetGraph: the Polyphony GS driver (gfx_init 0x0100C490 with
 * mode 0, and the mode-change tail 0x0100C890 with mode 5).
 */
#include "types.h"
#include "eekernel.h"
#include "libgraph.h"

/* Library version tag. @0x010394F0 (.data) - exactly 16 bytes, no terminator. */
static char s_libgraph_version[16] BOOT_USED = "PsIIlibgraph2221";

/* Global GS parameters. @0x01039500 (.data) */
static sceGsGParam s_gs_gparam = {
    SCE_GS_INTERLACE,   /* +0x00 sceGsInterMode = 1       */
    SCE_GS_NTSC,        /* +0x02 sceGsOutMode   = 2       */
    SCE_GS_FRAME,       /* +0x04 sceGsFFMode    = 1       */
    3,                  /* +0x06 sceGsVersion   = 3       */
    NULL,               /* +0x08 sceGsVSCfunc  (no v-sync callback installed) */
    0,                  /* +0x0C sceGsVSCid               */
};

// FUNC 0x010269E8 sceGsGetGParam
/* Returns the address of the global sceGsGParam (0x01039500). */
__attribute__((section(".text.sceGsGetGParam")))
sceGsGParam *sceGsGetGParam(void)
{
    return &s_gs_gparam;
}

// FUNC 0x010269F8 sceGsResetGraph
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
 * scratch) and no caller reads it. */
__attribute__((section(".text.sceGsResetGraph")))
void sceGsResetGraph(short mode, short inter, short omode, short ffmode)
{
    sceGsGParam *gp;
    u64          csr;

    switch (mode) {
    case SCE_GS_INIT_RESET:                                 /* 0 */
        gp = sceGsGetGParam();
        REG64(GS_CSR) = GS_CSR_RESET;                       /* 0x12001000 = 0x200 */
        gp->sceGsInterMode = inter;
        csr = REG64(GS_CSR);
        gp->sceGsOutMode = omode;
        gp->sceGsVersion = (u16)((csr >> 16) & 0xFF);
        GsPutIMR(0xFF00);
        gp->sceGsFFMode = (ffmode != 0);
        if (gp->sceGsVSCfunc != NULL) {
            DisableIntc(INTC_VBLANK_S);
            RemoveIntcHandler(INTC_VBLANK_S, gp->sceGsVSCid);
            gp->sceGsVSCfunc = NULL;    /* egcs swaps these two stores: */
            gp->sceGsVSCid = 0;         /* retail clears +0x0C, then +0x08 */
        }
        SetGsCrt(inter & 1, omode & 0xFF, ffmode & 1);      /* tail call (j 0x0101B7E0) */
        return;

    case SCE_GS_RESET_FLUSH:                                /* 1 */
        REG64(GS_CSR) = GS_CSR_FLUSH;                       /* 0x12001000 = 0x100 */
        return;

    case SCE_GS_RESET_MODE:                                 /* 5 */
        gp = sceGsGetGParam();
        csr = REG64(GS_CSR);
        gp->sceGsFFMode = (ffmode != 0);
        gp->sceGsInterMode = inter;
        gp->sceGsOutMode = omode;
        gp->sceGsVersion = (u16)((csr >> 16) & 0xFF);
        SetGsCrt(inter & 1, omode & 0xFF, ffmode & 1);      /* tail call (j 0x0101B7E0) */
        return;

    default:
        return;
    }
}
