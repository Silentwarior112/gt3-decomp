/* core decomp: copied from bootstrap/include/libgraph.h. Address comments in it are the
 * bootstrap's (0x010xxxxx); the core addresses of shared code are in ref/libmatch.json. */
/* libgraph.h - the parts of Sony EE libgraph (PsIIlibgraph2221) linked into
 * the GT3 US bootstrap (BOOTSTRAP_SCUS-97102), plus the GS register numbers,
 * pixel formats and privileged-register addresses used by Polyphony's own GS
 * driver (gs unit).  Sony SDK 2.2-era names.
 *
 * Only two libgraph functions are linked, each in its own named section at the
 * very end of .text (owned by the libc unit, sdk/libgraph/libgraph.c):
 *   .text.sceGsGetGParam  0x010269E8
 *   .text.sceGsResetGraph 0x010269F8
 * Their data - the version string 'PsIIlibgraph2221' @0x010394F0 and the
 * sceGsGParam instance @0x01039500 (initial {1, 2, 1, 3, 0, 0}) - sit in the
 * middle of the gs driver's .data tables.
 */
#ifndef BOOT_LIBGRAPH_H
#define BOOT_LIBGRAPH_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Global GS parameters (single instance @0x01039500, .data). */
typedef struct {
    short  sceGsInterMode;           /* +0x00 SCE_GS_NOINTERLACE / SCE_GS_INTERLACE      */
    short  sceGsOutMode;             /* +0x02 SCE_GS_NTSC / SCE_GS_PAL ...                */
    short  sceGsFFMode;              /* +0x04 SCE_GS_FIELD / SCE_GS_FRAME (stored as !=0) */
    short  sceGsVersion;             /* +0x06 GS CSR bits 23:16 (revision)                */
    int  (*sceGsVSCfunc)(int);       /* +0x08 v-sync callback; non-NULL = INTC handler installed */
    int    sceGsVSCid;               /* +0x0C its INTC_VBLANK_S handler id                */
} sceGsGParam;                       /* 0x10 */

#ifdef __cplusplus
static_assert(sizeof(sceGsGParam) == 0x10, "sceGsGParam");
#else
_Static_assert(sizeof(sceGsGParam) == 0x10, "sceGsGParam");
#endif

/* sceGsResetGraph arguments */
#define SCE_GS_NOINTERLACE   0
#define SCE_GS_INTERLACE     1
#define SCE_GS_FIELD         0
#define SCE_GS_FRAME         1
#define SCE_GS_NTSC          0x02
#define SCE_GS_PAL           0x03
#define SCE_GS_DTV480P       0x50
/* mode argument: 0 = full reset (CSR.RESET, record params + GS revision,
 * GsPutIMR(0xFF00), drop the v-sync callback, SetGsCrt);
 * 1 = CSR.FLUSH only (0x100 -> GS CSR);
 * 5 = record params + revision and SetGsCrt without resetting the GS
 *     (used by the gs driver's mode-change path 0x0100C890);
 * any other value: no effect.                                              */
#define SCE_GS_INIT_RESET    0
#define SCE_GS_RESET_FLUSH   1   /* ? name (not Sony-confirmed) */
#define SCE_GS_RESET_MODE    5   /* ? name (not Sony-confirmed) */

sceGsGParam *sceGsGetGParam(void);                                            /* 0x010269E8 */
/* No meaningful return value: every path ends in the SetGsCrt tail-call or
 * falls out without setting v0, and no caller reads it.                     */
void         sceGsResetGraph(short mode, short inter, short omode, short ffmode); /* 0x010269F8 */

/* ---- GS general-purpose register numbers (A+D / GIF PACKED addresses) ----- */
#define SCE_GS_PRIM         0x00
#define SCE_GS_RGBAQ        0x01
#define SCE_GS_ST           0x02
#define SCE_GS_UV           0x03
#define SCE_GS_XYZF2        0x04
#define SCE_GS_XYZ2         0x05
#define SCE_GS_TEX0_1       0x06
#define SCE_GS_TEX0_2       0x07
#define SCE_GS_CLAMP_1      0x08
#define SCE_GS_CLAMP_2      0x09
#define SCE_GS_FOG          0x0A
#define SCE_GS_XYZF3        0x0C
#define SCE_GS_XYZ3         0x0D
#define SCE_GS_NOP          0x0F   /* A+D no-op address */
#define SCE_GS_TEX1_1       0x14
#define SCE_GS_TEX1_2       0x15
#define SCE_GS_TEX2_1       0x16
#define SCE_GS_TEX2_2       0x17
#define SCE_GS_XYOFFSET_1   0x18
#define SCE_GS_XYOFFSET_2   0x19
#define SCE_GS_PRMODECONT   0x1A
#define SCE_GS_PRMODE       0x1B
#define SCE_GS_TEXCLUT      0x1C
#define SCE_GS_SCANMSK      0x22
#define SCE_GS_MIPTBP1_1    0x34
#define SCE_GS_MIPTBP1_2    0x35
#define SCE_GS_MIPTBP2_1    0x36
#define SCE_GS_MIPTBP2_2    0x37
#define SCE_GS_TEXA         0x3B
#define SCE_GS_FOGCOL       0x3D
#define SCE_GS_TEXFLUSH     0x3F
#define SCE_GS_SCISSOR_1    0x40
#define SCE_GS_SCISSOR_2    0x41
#define SCE_GS_ALPHA_1      0x42
#define SCE_GS_ALPHA_2      0x43
#define SCE_GS_DIMX         0x44
#define SCE_GS_DTHE         0x45
#define SCE_GS_COLCLAMP     0x46
#define SCE_GS_TEST_1       0x47
#define SCE_GS_TEST_2       0x48
#define SCE_GS_PABE         0x49
#define SCE_GS_FBA_1        0x4A
#define SCE_GS_FBA_2        0x4B
#define SCE_GS_FRAME_1      0x4C
#define SCE_GS_FRAME_2      0x4D
#define SCE_GS_ZBUF_1       0x4E
#define SCE_GS_ZBUF_2       0x4F
#define SCE_GS_BITBLTBUF    0x50
#define SCE_GS_TRXPOS       0x51
#define SCE_GS_TRXREG       0x52
#define SCE_GS_TRXDIR       0x53
#define SCE_GS_HWREG        0x54
#define SCE_GS_SIGNAL       0x60
#define SCE_GS_FINISH       0x61
#define SCE_GS_LABEL        0x62
/* The gs driver's shadow-register order tables (.data) use these numbers:
 *   0x01039510 general: 46 44 45 3D 49 1A 22 3B 1C 3F
 *   0x01039520 ctx 1  : 4C 18 4A 4E 40 42 47 08 16 14 06 34 36
 *   0x01039530 ctx 2  : 4D 19 4B 4F 41 43 48 09 17 15 07 35 37           */

/* ---- pixel storage formats (TEX0.PSM / FRAME.PSM / ZBUF.PSM) -------------- */
#define SCE_GS_PSMCT32      0x00
#define SCE_GS_PSMCT24      0x01
#define SCE_GS_PSMCT16      0x02
#define SCE_GS_PSMCT16S     0x0A
#define SCE_GS_PSMT8        0x13
#define SCE_GS_PSMT4        0x14
#define SCE_GS_PSMT8H       0x1B
#define SCE_GS_PSMT4HL      0x24    /* (integration: the two 4HL/4HH values were */
#define SCE_GS_PSMT4HH      0x2C    /*  swapped; its only user, gs_driver.cpp, puts
                                     *  both in one case group - no behaviour change) */
#define SCE_GS_PSMZ32       0x30
#define SCE_GS_PSMZ24       0x31
#define SCE_GS_PSMZ16       0x32
#define SCE_GS_PSMZ16S      0x3A

/* ---- GS privileged registers (64-bit; Sony eeregs.h names) --------------- */
#define GS_PMODE     0x12000000
#define GS_SMODE1    0x12000010
#define GS_SMODE2    0x12000020
#define GS_SRFSH     0x12000030
#define GS_SYNCH1    0x12000040
#define GS_SYNCH2    0x12000050
#define GS_SYNCV     0x12000060
#define GS_DISPFB1   0x12000070
#define GS_DISPLAY1  0x12000080
#define GS_DISPFB2   0x12000090
#define GS_DISPLAY2  0x120000A0
#define GS_EXTBUF    0x120000B0
#define GS_EXTDATA   0x120000C0
#define GS_EXTWRITE  0x120000D0
#define GS_BGCOLOR   0x120000E0
#define GS_CSR       0x12001000   /* bit 8 FLUSH, bit 9 RESET, bits 23:16 revision */
#define GS_IMR       0x12001010
#define GS_BUSDIR    0x12001040
#define GS_SIGLBLID  0x12001080

#define GS_CSR_FLUSH  0x0100
#define GS_CSR_RESET  0x0200

#ifdef __cplusplus
}
#endif

#endif /* BOOT_LIBGRAPH_H */
