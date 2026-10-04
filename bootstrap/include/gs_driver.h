/* gs_driver.h - Polyphony's own GS / VIF1 / GIF display driver as linked into
 * the GT3 US bootstrap (BOOTSTRAP_SCUS-97102), unit "gs",
 * .text 0x0100BE10..0x0100F130: three translation units of the original,
 * src/gs_texture.cpp (0x0100BE10..), src/gs_driver.cpp (0x0100BF98..) and
 * src/gs_state.cpp (0x0100EE48..); their shared helpers are in gs_internal.h.
 *
 * The driver is plain C (no EH / per-function metadata records), so every
 * function has C linkage.  It keeps
 *   - a 0xBC0-byte drawing context in scratchpad RAM at 0x70003000
 *     (GsContext: shadowed GS registers with a dirty mask, the VIF1/GIF DMA
 *     packet writer, OpenGL-style light/material state for the VU1 microcode),
 *   - a 0xC0-byte interrupt/display state block in .bss at 0x01048180
 *     (GfxDriverState: vblank counter, per-DMA-channel busy flags and waiter
 *     lists, display register shadows, INTC/DMAC handler ids).
 * Only one sce* function is used: sceGsResetGraph (libgraph.h).
 *
 * Packet model: the frame is two interleaved DMA source chains in one buffer
 * (VIF1 chain = list 0, GIF/PATH3 chain = list 1, header GsFrameHeader).
 * Register changes are written to shadows and flagged in GsContext.dirty; the
 * next primitive flushes them (gs_flush_dirty_regs) as
 * DMAtag(cnt) + VIF1{FLUSH, DIRECT} + GIFtag(A+D) blocks.
 */
#ifndef BOOT_GS_DRIVER_H
#define BOOT_GS_DRIVER_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================== */
/* Shadowed GS registers                                                    */
/* ======================================================================== */

/* GsContext.gen[] - general registers, emitted in this order (GS register
 * numbers from the byte table @0x01039510: 46 44 45 3D 49 1A 22 3B 1C 3F).
 * Dirty bit = 32 + index.                                                   */
enum {
    GS_GEN_COLCLAMP = 0, GS_GEN_DIMX, GS_GEN_DTHE, GS_GEN_FOGCOL, GS_GEN_PABE,
    GS_GEN_PRMODECONT, GS_GEN_SCANMSK, GS_GEN_TEXA, GS_GEN_TEXCLUT, GS_GEN_TEXFLUSH,
    GS_GEN_COUNT                                  /* 10 */
};

/* GsContext.reg[] - per-context registers.  ONE shadow array serves both GS
 * contexts: dirty bit i emits it to the context-1 register (table
 * @0x01039520: 4C 18 4A 4E 40 42 47 08 16 14 06 34 36), dirty bit 16+i to the
 * context-2 register (table @0x01039530: 4D 19 4B 4F 41 43 48 09 17 15 07 35 37). */
enum {
    GS_REG_FRAME = 0, GS_REG_XYOFFSET, GS_REG_FBA, GS_REG_ZBUF, GS_REG_SCISSOR,
    GS_REG_ALPHA, GS_REG_TEST, GS_REG_CLAMP, GS_REG_TEX2, GS_REG_TEX1, GS_REG_TEX0,
    GS_REG_MIPTBP1, GS_REG_MIPTBP2,
    GS_REG_COUNT                                  /* 13 */
};

/* GsContext.dirty / .written / .enable bit layout */
#define GS_DIRTY_CTX1(i)    (1ull << (i))                       /* bits 0..12  */
#define GS_DIRTY_CTX2(i)    (1ull << (16 + (i)))                /* bits 16..28 */
#define GS_DIRTY_BOTH(i)    (GS_DIRTY_CTX1(i) | GS_DIRTY_CTX2(i))
#define GS_DIRTY_GEN(i)     (1ull << (32 + (i)))                /* bits 32..41 */
#define GS_DIRTY_ALL        0x000003FF1FFF1FFFull
#define GS_ENABLE_GEN_CTX1  0x000003FF00001FFFull               /* no context 2 */

/* ======================================================================== */
/* Structures                                                               */
/* ======================================================================== */

typedef struct GsVec4 {
    f32 x, y, z, w;
} __attribute__((aligned(16))) GsVec4;

/* A GsVec4 passed by value to gs_store_material_vec.  The EE compiler keeps
 * it in ONE 128-bit GPR (retail: $a3, built with qmfc2 and stored with sq),
 * which egcs does for a u128 (TImode) argument; GCC 14 passes the struct. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
typedef u128 GsVec4Arg;
#else
typedef GsVec4 GsVec4Arg;
#endif

/* Render-target descriptor (two in GsContext: tgtB @+0x170, tgtA @+0x184). */
typedef struct GsRenderTarget {
    u16 base;       /* +0x00 frame buffer base in 64-word blocks (GS word addr >> 6);
                     *       FRAME.FBP = base >> 5                                   */
    u8  fbw;        /* +0x02 FRAME.FBW (width / 64, rounded up)                      */
    u8  psm;        /* +0x03 FRAME.PSM                                               */
    s32 width;      /* +0x04 pixels                                                  */
    s32 height;     /* +0x08 pixels                                                  */
    u16 xoff;       /* +0x0C XYOFFSET.OFX (12.4) = (0x800 - width/2) << 4            */
    u16 yoff;       /* +0x0E XYOFFSET.OFY                                            */
    u16 xend;       /* +0x10 xoff + width*16                                         */
    u16 yend;       /* +0x12 yoff + height*16                                        */
} GsRenderTarget;   /* 0x14 */

/* One light (8 of them at GsContext+0x2E0).  gfx_init_lights initialises only
 * light 0, with the OpenGL defaults (diffuse/specular 1, position (0,0,1,0),
 * spot direction (0,0,-1), cutoff 180) - field names follow that reading.   */
typedef struct GsLight {
    u8     pad00[8];
    u8     enable;          /* +0x08 cleared by gfx_disable(0x65 + i)          */
    u8     pad09[3];
    u32    spotExpEnc;      /* +0x0C float_to_gs_param_encode(spotExponent) ?  */
    GsVec4 ambient;         /* +0x10 {0,0,0,1}                                 */
    GsVec4 diffuse;         /* +0x20 {1,1,1,1}                                 */
    GsVec4 q30;             /* +0x30                                           */
    GsVec4 specular;        /* +0x40 {1,1,1,1} ?                               */
    GsVec4 q50;             /* +0x50                                           */
    GsVec4 position;        /* +0x60 {0,0,1,0}                                 */
    GsVec4 q70;             /* +0x70                                           */
    GsVec4 q80;             /* +0x80                                           */
    GsVec4 spotDir;         /* +0x90 {0,0,-1,0}                                */
    GsVec4 qA0;             /* +0xA0                                           */
    GsVec4 qB0;             /* +0xB0 .w = 180.0 (spot cutoff ?)                */
    GsVec4 atten;           /* +0xC0 {0, 1.0, 0.0 (spot exponent ?), 1.0}      */
} GsLight;                  /* 0xD0 */

/* Material parameters, one block per face/context (GsContext+0x960, stride
 * 0x50).  gs_set_material_param kinds: 0 ambient, 1 diffuse, 2 both,
 * 3 +0x30, 4 +0x20, 11 shininess (clamped 0..128) - an OpenGL glMaterial
 * mirror; defaults ambient 0.2 / diffuse 0.8 / others (0,0,0,1).           */
typedef struct GsMaterial {
    GsVec4 ambient;         /* +0x00 kind 0 (and 2)                            */
    GsVec4 diffuse;         /* +0x10 kind 1 (and 2)                            */
    GsVec4 specular;        /* +0x20 kind 4 (? name)                           */
    GsVec4 emission;        /* +0x30 kind 3 (? name)                           */
    f32    shininess;       /* +0x40 kind 11                                   */
    u32    zero44;          /* +0x44 written 0 with shininess                  */
    u32    pad48;
    u32    shininessEnc;    /* +0x4C float_to_gs_param_encode(shininess)       */
} GsMaterial;               /* 0x50 */

/* The two chain entry points reserved at the start of a frame packet
 * (gfx_begin_packet returns it; gfx_kick_frame_lists consumes it).         */
typedef struct GsFrameHeader {
    u32 *vif1Chain;         /* +0x00 list 0 start (VIF1 source chain)          */
    u32 *gifChain;          /* +0x04 list 1 start (GIF source chain)           */
    u32 *save0;             /* +0x08 read by gfx_close_subpacket, never set    */
    u32 *save1;             /* +0x0C                                           */
} GsFrameHeader;

/* Drawing context, 0xBC0 bytes, at scratchpad 0x70003000 (GS_CTX). */
typedef struct GsContext {
    u8     flag00;          /* +0x000 set 1 by vu0_load_identity_regs, gfx_begin_packet,
                             *        gfx_disable(0x12)                              */
    u8     flag01;          /* +0x001                                                */
    u8     flag02;          /* +0x002 cleared by scissor/viewport update, begin_packet */
    u8     curList;         /* +0x003 packet list being written: 0 VIF1, 1 GIF       */
    u16    primAttr;        /* +0x004 PRIM attribute bits (IIP 8, TME 0x10, FGE 0x20,
                             *        ABE 0x40, AA1 0x80); init 8                    */
    u8     flag06;          /* +0x006 */
    u8     flag07;          /* +0x007 */
    u32    displayPeriod;   /* +0x008 0x320 / 0x33E (+0xA/+0x14) - gs_compute_display_period */
    u8     pad00C[0x0C];
    u32    stmask;          /* +0x018 = 0xC0C0C0C0 (the STMASK sent by begin_packet) */
    s32    lodBias;         /* +0x01C added to TEX1.K (<< TEX1.L) by gfx_set_texture_regs */
    u8     lightFlags;      /* +0x020 init 7; |1 light change, |4 material change, &0x7F */
    u8     lightMode;       /* +0x021 init 2 */
    u8     bgImmediate;     /* +0x022 gs_set_bg_color also pushes the colour as a material */
    u8     halfHeight;      /* +0x023 field rendering: y coordinates use 12.3 (<< 3) */
    u8     flag24;          /* +0x024 */
    u8     pal;             /* +0x025 init 1 */
    u8     periodAdjust;    /* +0x026 */
    u8     flag27;          /* +0x027 */
    u8     flag28;          /* +0x028 */
    u8     flag29;          /* +0x029 */
    u8     flag2A;          /* +0x02A */
    u8     pad02B[5];
    u64    gen[GS_GEN_COUNT];   /* +0x030 general register shadows (GS_GEN_*)        */
    u64    reg[GS_REG_COUNT];   /* +0x080 per-context register shadows (GS_REG_*)    */
    u64    dirty;           /* +0x0E8 registers to emit on the next flush            */
    u64    written;         /* +0x0F0 every register ever emitted                    */
    u64    enable;          /* +0x0F8 emit mask of the current list                  */
    u64    enableList0;     /* +0x100 saved emit mask of list 0 (VIF1)               */
    u64    enableList1;     /* +0x108 saved emit mask of list 1 (GIF)                */
    u8    *bufBase;         /* +0x110 packet buffer                                  */
    u32    bufSize;         /* +0x114                                                */
    u32   *cur;             /* +0x118 write pointer (qword aligned, | addrBits)      */
    u32   *end;             /* +0x11C                                                */
    u32    uncachedBuf;     /* +0x120 buffer accessed through an uncached segment    */
    u32    addrBits;        /* +0x124 segment bits ORed into buffer addresses        */
    u32   *tail0;           /* +0x128 reserved next-tag slot of list 0               */
    u32   *tail1;           /* +0x12C reserved next-tag slot of list 1               */
    GsFrameHeader *frameHdr;/* +0x130                                                */
    u64   *subHdr;          /* +0x134 read by gfx_close_subpacket, never set         */
    u8     pad138[0x14];
    u32    clearColor;      /* +0x14C RGBA for clears of tgtA (gs_set_clear_color)   */
    u32    clearColorB;     /* +0x150 RGBA for clears of tgtB                        */
    f32    clearDepth;      /* +0x154 depth clear value = clearDepth * depthScale    */
    u32    f158;            /* +0x158 */
    f32    f15C;            /* +0x15C init 1.0 */
    f32    depthScale;      /* +0x160 */
    u8     pad164[8];
    u32    spriteZF;        /* +0x16C XYZF high word (Z|F) of gs_draw_textured_sprite */
    GsRenderTarget tgtB;    /* +0x170 secondary target (cleared by gs_clear_buffers bit 2) */
    GsRenderTarget tgtA;    /* +0x184 draw buffer (gfx_set_draw_buffer)              */
    u8     pad198[0x14];
    s32    winX;            /* +0x1AC draw window (gfx_set_draw_area)                */
    s32    winY;            /* +0x1B0 */
    s32    winW;            /* +0x1B4 */
    s32    winH;            /* +0x1B8 */
    s8     field;           /* +0x1BC current field (from the vblank handler)        */
    u8     f1BD;            /* +0x1BD */
    u8     packetOpen;      /* +0x1BE \ tested together as one u16 ("packet active") */
    u8     subPacketOpen;   /* +0x1BF / (subPacketOpen is never set in this binary)  */
    u8     path3Masked;     /* +0x1C0 */
    u8     pad1C1[3];
    u32    videoMode;       /* +0x1C4 init 2 */
    u32    frameMode;       /* +0x1C8 */
    u8     pad1CC[4];
    u8     clearZMask;      /* +0x1D0 goes to ZBUF.ZMSK of clear packets             */
    u8     f1D1;            /* +0x1D1 */
    u8     pad1D2[2];
    u32    f1D4;            /* +0x1D4 */
    f32    f1D8;            /* +0x1D8 init 1.0 */
    u8     pad1DC[0x104];
    GsLight    lights[8];   /* +0x2E0 */
    GsMaterial material[2]; /* +0x960 */
    GsVec4 qA00;            /* +0xA00 {0,0,0,1} */
    u8     padA10[0x20];
    GsVec4 globalAmbient;   /* +0xA30 {0.2,0.2,0.2,1} (OpenGL light-model ambient) */
    u8     padA40[0xB0];
    GsVec4 qAF0;            /* +0xAF0 {1,1,1,1} */
    GsVec4 qB00;            /* +0xB00 {1,1,1,1} */
    GsVec4 qB10;            /* +0xB10 {0,0,0,0} */
    u8     padB20[0x20];
    u32    fB40;            /* +0xB40 */
    u32    fB44;            /* +0xB44 */
    f32    fB48;            /* +0xB48 init 1.0 */
    u32    fB4C;            /* +0xB4C */
    u8     padB50[0x10];
    u32    bgColor;         /* +0xB60 gs_set_bg_color value (init 0xFFFFFFFF)        */
    u32    bgBlend;         /* +0xB64 bgColor rgb * 129/256, alpha kept (init 0x80808080);
                             *        RGBAQ of gs_draw_textured_sprite               */
    s32    bgParamKind;     /* +0xB68 material kind for bgImmediate (init 2)         */
    s32    bgParamMask;     /* +0xB6C face mask for bgImmediate (init 1)             */
    u8     padB70[4];
    u32    fB74;            /* +0xB74 */
    u32    fB78;            /* +0xB78 */
    u32    fB7C;            /* +0xB7C */
    f32    viewScaleX;      /* +0xB80 winW * 0.5  */
    f32    viewScaleY;      /* +0xB84 -winH * 0.5 */
    u8     padB88[8];
    f32    viewCenterX;     /* +0xB90 (xoff + winX*16 + winW*8) / 16 */
    f32    viewCenterY;     /* +0xB94 (yoff + winY*16 + winH*8) / 16 */
    u8     padB98[4];
    f32    colorMax;        /* +0xB9C 255.0 */
    f32    limits[4];       /* +0xBA0 FLT_MAX x 4 */
    f32    fBB0;            /* +0xBB0 0.5 */
    f32    fBB4;            /* +0xBB4 0.5 */
    f32    fBB8;            /* +0xBB8 1.0 */
    f32    fBBC;            /* +0xBBC 1.0 */
} GsContext;                /* 0xBC0 */

#define GS_CTX  ((GsContext *)0x70003000)

/* Sleeping-thread record linked into the waiter lists (lives on the stack). */
typedef struct GfxWaitNode {
    struct GfxWaitNode *next;   /* +0x00 */
    int                 tid;    /* +0x04 */
} GfxWaitNode;

/* Per DMA channel (0 VIF0, 1 VIF1, 2 GIF) record inside GfxDriverState. */
typedef struct GfxDmaChan {
    volatile u8  tagIrq;        /* +0x00 channel stopped on a tag without IRQ bit */
    volatile u8  busy;          /* +0x01 transfer started, completion pending     */
    u8           pad[2];
    GfxWaitNode * volatile waiters; /* +0x04 threads sleeping in gfx_dma_sync         */
    int          sema;          /* +0x08 channel mutex (gfx_dma_lock/unlock)      */
} GfxDmaChan;                   /* 0x0C */

/* GS->EE download record embedded in the VIF1 chain (read uncached through
 * D1_TADR by gs_readback_begin); the chain resumes at rec + 0x10.  Its
 * first qword is the DMA tag the VIF1 chain stopped on (QWC, ADDR = madr).
 * The QWC halfword is a member of a union with the tag's first word ? -
 * egcs gives a direct union-member access alias set 0, so the qwc load
 * conflicts with every store: in gs_readback_begin it stays ahead of the
 * g->readback store, as in the binary (a plain u16 member would not).    */
typedef struct GsReadbackRec {
    union {
        u16  qwc;                       /* +0x00 tag QWC */
        u32  tag0;                      /*       tag word 0 (QWC | ID ...) */
    } h;
    u32  madr;                          /* +0x04 destination */
    void (*callback)(u32 madr, u32 arg);/* +0x08 optional */
    u32  arg;                           /* +0x0C */
} GsReadbackRec;

/* Interrupt / display state, 0xC0 bytes @0x01048180 (.bss). */
typedef struct GfxDriverState {
    volatile u64 vblankCount;   /* +0x00 incremented by intc_vblank_handler        */
    volatile u64 lastVsync;     /* +0x08 count at the last gfx_vsync               */
    void (*vsyncCallback)(int count); /* +0x10 (only ever set to NULL here)         */
    volatile u8 field;          /* +0x14 GS CSR.FIELD sampled at vblank            */
    volatile s8 finishCount;    /* +0x15 FINISH / VIF1 marker events of a readback */
    volatile s8 pathSync16;     /* +0x16 \ VIF1 <-> GIF chain rendezvous counters,  */
    volatile s8 pathSync17;     /* +0x17 | stepped by the VIF1 i-bit handler and   */
    volatile s8 pathSync18;     /* +0x18 / the GIF DMA tag-IRQ handler             */
    u8   pad19[3];
    GfxDmaChan chan[3];         /* +0x1C VIF0, VIF1, GIF                           */
    GfxWaitNode * volatile vsyncWaiters; /* +0x40 threads in gfx_wait_vblank_event          */
    GsReadbackRec * volatile readback; /* +0x44 download in progress                      */
    volatile u64 perfCount0;    /* +0x48 accumulated COP0 PCR0 (per vblank)        */
    volatile u64 perfCount1;    /* +0x50 accumulated COP0 PCR1                     */
    u64  pmode;                 /* +0x58 PMODE shadow (0x66)                       */
    u64  smode2;                /* +0x60 SMODE2 shadow                             */
    u64  dispfb2;               /* +0x68 DISPFB2 shadow                            */
    u64  display2;              /* +0x70 DISPLAY2 shadow                           */
    u64  bgcolor;               /* +0x78 BGCOLOR shadow                            */
    u64  imr;                   /* +0x80 GS IMR shadow                             */
    u16  dby;                   /* +0x88 DISPFB2.DBY                               */
    u16  dbx;                   /* +0x8A DISPFB2.DBX                               */
    s16  dispDX;                /* +0x8C DISPLAY2 x offset (never set: 0)          */
    s16  dispDY;                /* +0x8E DISPLAY2 y offset (never set: 0)          */
    u16  dispW;                 /* +0x90 display width                             */
    u16  dispH;                 /* +0x92 display height                            */
    u16  screenW;               /* +0x94 screen width (MAGH divisor)               */
    u16  screenH;               /* +0x96 screen height (MAGV divisor)              */
    u8   gsRevision;            /* +0x98 GS CSR bits 23:16                         */
    u8   inter;                 /* +0x99 */
    u8   omode;                 /* +0x9A */
    u8   ffmd;                  /* +0x9B */
    u8   pad9C[4];
    int  intcIdVblank;          /* +0xA0 AddIntcHandler(INTC_VBLANK_S) id          */
    int  dmacId[3];             /* +0xA4 AddDmacHandler ids, channels 0..2         */
    u8   padB0[4];
    int  intcIdVif1;            /* +0xB4 AddIntcHandler(INTC_VIF1) id              */
    int  intcIdGs;              /* +0xB8 AddIntcHandler(INTC_GS) id                */
    u8   padBC[4];
} GfxDriverState;               /* 0xC0 */

/* gfx_set_texture_regs argument: a texture's register block. */
typedef struct GsTexRegs {
    u64 tex0;       /* +0x00 */
    u64 tex1;       /* +0x08 */
    u64 miptbp1;    /* +0x10 (used when TEX1.MXL != 0) */
    u64 miptbp2;    /* +0x18 (used when TEX1.MXL >= 4) */
    u64 clamp;      /* +0x20 */
} GsTexRegs;

/* Texture register override @0x01034640 (.data; s_gsTexOverride, static in
 * src/gs_texture.cpp): bits set in a mask keep the context's current value.
 * Disabled (enable = 0) in this binary.                                    */
typedef struct GsTexOverride {
    u64 tex0Mask;   /* +0x00 0xE000001800000000 */
    u64 tex1Mask;   /* +0x08 0x00000000000001E0 */
    u64 clampMask;  /* +0x10 0 */
    u32 unk18;      /* +0x18 0x00010060 (unused by code) */
    u8  enable;     /* +0x1C 0 */
    u8  pad1D[3];
} GsTexOverride;    /* 0x20 */

#ifdef __cplusplus
static_assert(sizeof(GsRenderTarget) == 0x14, "GsRenderTarget");
static_assert(sizeof(GsLight) == 0xD0, "GsLight");
static_assert(sizeof(GsMaterial) == 0x50, "GsMaterial");
static_assert(sizeof(GsContext) == 0xBC0, "GsContext");
static_assert(sizeof(GfxDmaChan) == 0x0C, "GfxDmaChan");
static_assert(sizeof(GfxDriverState) == 0xC0, "GfxDriverState");
static_assert(sizeof(GsTexOverride) == 0x20, "GsTexOverride");
#else
_Static_assert(sizeof(GsRenderTarget) == 0x14, "GsRenderTarget");
_Static_assert(sizeof(GsLight) == 0xD0, "GsLight");
_Static_assert(sizeof(GsMaterial) == 0x50, "GsMaterial");
_Static_assert(sizeof(GsContext) == 0xBC0, "GsContext");
_Static_assert(sizeof(GfxDmaChan) == 0x0C, "GfxDmaChan");
_Static_assert(sizeof(GfxDriverState) == 0xC0, "GfxDriverState");
_Static_assert(sizeof(GsTexOverride) == 0x20, "GsTexOverride");
#endif

/* ======================================================================== */
/* Globals                                                                  */
/* ======================================================================== */

/* g_gfx (GfxDriverState @0x01048180, .bss) is static in src/gs_driver.cpp. */
extern u8  g_gsVu1Microcode[0x3DA8];          /* @0x01034730 .data, LCG-XOR scrambled in the ELF
                                               *   (data/gs_vu1_microcode_scrambled.bin) */
extern const u8 g_gsVu0Microcode[0x398];      /* @0x01040C00 .rdata (data/gs_vu0_microcode.bin) */
extern u8  g_010384D8[0x1000];                /* @0x010384D8 .data, zero-filled; purpose unknown */
extern void *g_gsVuWork;                      /* @0x01041E2C .sdata = g_010384D8 (never read)   */

/* ======================================================================== */
/* Functions (address order)                                                */
/* ======================================================================== */

void gfx_set_texture_regs(const GsTexRegs *t);                         /* 0x0100BE10 */
void gfx_wait_vsync_poll(void);                                        /* 0x0100BF98 */
void gfx_reset_paths(void);                                            /* 0x0100BFE8 */
void gfx_init_vu0_constants(GsContext *ctx);                           /* 0x0100C050 */
void gfx_init_lights(GsContext *ctx);                                  /* 0x0100C158 */
void gfx_init_material(GsContext *ctx);                                /* 0x0100C230 */
void gfx_reset_texture_state(GsContext *ctx);                          /* 0x0100C300 (was gfx_reset_blend_state) */
void gfx_context_init(GsContext *ctx);                                 /* 0x0100C378 */
void gfx_init_default(int omode);                                      /* 0x0100C478 = gfx_init(1, 1, omode, 1) */
void gfx_init(int fullReset, int inter, int omode, int ffmd);          /* 0x0100C490 */
void gfx_context_init_spr(void);                                       /* 0x0100C6A0 */
void gfx_init_vu0_constants_spr(void);                                 /* 0x0100C6C8 */
void gfx_init_lights_material_spr(void);                               /* 0x0100C6F0 */
void gfx_reset_texture_state_spr(void);                                /* 0x0100C728 */
void gfx_shutdown(void);                                               /* 0x0100C750 */
void gfx_change_display_mode(int inter, int omode, int ffmd);          /* 0x0100C7E0 */
void gfx_store_display_mode(int inter, int omode, int ffmd);           /* 0x0100C848 */
void gfx_reset_graph_stored_mode(void);                                /* 0x0100C890 */
void gfx_update_display_regs(GfxDriverState *g);                       /* 0x0100C8B0 */
void gfx_set_display_area(int dbx, int dby, int w, int h);             /* 0x0100C998 */
void gfx_set_screen_size(int w, int h);                                /* 0x0100C9B8 */
void gfx_set_display_fb(u32 fbWordAddr, int psm, int fbw, int fbh);    /* 0x0100C9D0 (fbh unused: $a3 is reused at entry) */
void gfx_dma_channel_init(int ch, u32 chcr);                           /* 0x0100CA78 */
void gfx_dma_init(void);                                               /* 0x0100CB08 */
void gfx_dma_shutdown(void);                                           /* 0x0100CC00 */
void gfx_dma_lock(int ch);                                             /* 0x0100CC78 WaitSema (tail jump) */
void gfx_dma_unlock(int ch);                                           /* 0x0100CC98 SignalSema (tail jump) */
int  gfx_dma_sync(int ch);                                             /* 0x0100CCB8 0 ok, -1 timeout */
void gfx_dma_send(int ch, const void *madr, u32 qwc);                  /* 0x0100CE68 */
void gfx_kick_frame_lists(GsFrameHeader *hdr);                         /* 0x0100CF28 NULL = ctx->frameHdr */
void vu_upload_microcode(int ch, u32 vuAddr, int byteSize, const void *code); /* 0x0100CFE8 */
void gfx_disable(u32 what);                                            /* 0x0100D0F8 (was gfx_state_command) */
void gfx_mskpath3(int mask);                                           /* 0x0100D330 */
void gfx_set_packet_buffer(void *buf, u32 size);                       /* 0x0100D3A0 */
u32 *gfx_get_packet_ptr(void);                                         /* 0x0100D410 */
int  gfx_get_packet_space(void);                                       /* 0x0100D420 */
GsFrameHeader *gfx_begin_packet(void);                                 /* 0x0100D438 NULL if already open */
void gfx_end_packet(void);                                             /* 0x0100D530 */
void gfx_close_subpacket(void);                                        /* 0x0100D5E0 */
int  gfx_select_list(int which);                                       /* 0x0100D6E0 */
void gfx_sync_pipeline(void);                                          /* 0x0100D7B0 */
void gfx_wait_vblank_event(void);                                      /* 0x0100D8D0 */
int  gfx_vsync(int n);                                                 /* 0x0100D9A8 */
void gfx_set_draw_buffer(u32 fbWordAddr, int psm, int w, int h);       /* 0x0100DAD0 */
void gfx_set_draw_area(int x, int y, int w, int h);                    /* 0x0100DB88 */
void gs_update_scissor_and_viewport(GsContext *ctx);                   /* 0x0100DBB8 */
void gs_set_clear_color(u32 rgba);                                     /* 0x0100DD00 */
void gs_build_clear_packet(GsContext *ctx, u64 test, u32 rgba, u32 z,
                           const GsRenderTarget *tgt);                 /* 0x0100DD10 */
void gs_clear_buffers(int mask);                                       /* 0x0100DE80 bit0 colour, bit1 depth, bit2 tgtB */
void gs_emit_render_target_regs(GsContext *ctx);                       /* 0x0100DF40 */
int  gs_build_image_transfer_packet(u32 *pkt, const u32 *dstBitblt,
                                    int dx, int dy, int w, int h);     /* 0x0100E0A0 returns image qwc */
u32  float_to_gs_param_encode(float v);                                /* 0x0100E198 */
float float_pow_encoded(float x, int enc);                             /* 0x0100E228 (was float_pow_fixed44; dead) */
void gs_draw_textured_sprite(int x0, int y0, int x1, int y1,
                             int u0, int v0, int u1, int v1);          /* 0x0100E2A0 */
void vu0_load_identity_regs(void);                                     /* 0x0100E404 (was gs_init_vu0_constants) */
int  gs_get_display_state(void);                                       /* 0x0100E430 */
void gs_set_bg_color_rgb(float r, float g, float b);                   /* 0x0100E478 (was gs_set_bg_color_f) */
int  lcg_rand(void);                                                   /* 0x0100E4A8 */
void xor_unscramble_with_lcg(u8 *buf, int len);                        /* 0x0100E4E8 */
void gs_flush_all_regs(void);                                          /* 0x0100E548 (dead) */
void gs_flush_dirty_regs(void);                                        /* 0x0100E570 */
int  intc_vblank_handler(int cause);                                   /* 0x0100E630 */
int  intc_vif1_handler(int cause);                                     /* 0x0100E770 */
int  intc_gs_finish_handler(int cause);                                /* 0x0100E888 */
int  dmac_channel_handler(int ch);                                     /* 0x0100E8E8 */
void gs_readback_begin(GfxDriverState *g);                             /* 0x0100EB00 */
void gs_readback_end(GfxDriverState *g);                               /* 0x0100EB78 */
void dma_start_source_chain(int ch, const void *tadr);                 /* 0x0100EC20 */
void gs_compute_display_period(GsContext *ctx);                        /* 0x0100ECD0 */
void gs_set_bg_color(u32 rgba);                                        /* 0x0100ED58 */
u32  pack_rgba_floats_to_u32(float r, float g, float b, float a, float scale); /* 0x0100EDB0 */
u64 *gs_emit_masked_regs(u64 *out, u64 mask, const u8 *regNums,
                         const u64 *shadow);                           /* 0x0100EE48 */
int  gs_collect_dirty_regs(GsContext *ctx, u64 *out);                  /* 0x0100EE80 */
void gs_store_material_vec(GsContext *ctx, int face, u32 kind, GsVec4Arg v); /* 0x0100EF50 (was gs_store_param_quad) */
void gs_set_material_param(int faceMask, int kind, u32 value);         /* 0x0100EFF0 (was gs_set_color_param) */

#ifdef __cplusplus
}
#endif

#endif /* BOOT_GS_DRIVER_H */
