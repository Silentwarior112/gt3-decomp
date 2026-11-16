/* libcdvd_internal.h - unit u129 (0x00270628..0x00272B40), part of the no-FDE stretch G004:
 *   libcdvd.c  Sony EE libcdvd "PsIIlibcdvd 2240" (0x00270628..0x00271A60)
 *   libpad_part1.c   Sony EE libpad "PsIIlibpad  2200", first part
 *                        (0x00271A60..0x00272B40; the rest is in u130)
 * Both are the same machine code as the bootstrap's sdk/libcdvd/libcdvd.c and
 * sdk/libpad/libpad.c; this source is theirs with the core addresses.
 *
 * Canonical headers:
 *   sdk/libcdvd/G004_libcdvd.h  this translation unit: the functions with callers
 *                        outside the unit, PadPortCtl and the libpad data
 *                        shared with u130 (s_padClient, s_padCtl, s_padRpcBuf)
 *   runtime/newlib/G007_stdio.h    the libkernl / SIF / string API with the core
 *                        addresses, and sce_printf_deci2 (0x00287B80)
 *   engine/pdistd/T533_printf.h   printf (libpad's debug output)
 *   runtime/startup/G000_collect2_ctors.h  _gp (the callback thread's gp)
 * The SDK headers still give the types and constants (struct ThreadParam /
 * SemaParam and ExitHandler() from eekernel.h, sceSif* types and SIF_* from
 * sif.h, sceCd* / scePad* constants and the prototypes of the functions with
 * no caller outside the unit from libcdvd.h / libpad.h).
 *
 * Everything below is local to the unit.  The library state is file-static
 * data in the original, and so it is in the C (the data pass): the two files
 * define it with the core addresses, as the bootstrap's sources do.
 * Bootstrap -> core: libcdvd .data 0x0103A3C0 -> 0x003043C0, libcdvd .bss
 * 0x01048280 -> 0x004AB540, libpad .data 0x0103BE30 -> 0x00305E30, libpad
 * .bss 0x01048580 -> 0x004AB840 (same layouts).
 */
#ifndef U129_H
#define U129_H

#include "types.h"
#include "eekernel.h"
#include "sif.h"
#include "libcdvd.h"
#include "libpad.h"
#include "runtime/startup/G000_collect2_ctors.h"
#include "sdk/libcdvd/G004_libcdvd.h"
#include "runtime/newlib/G007_stdio.h"
#include "engine/pdistd/T533_printf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================== */
/* libcdvd constants                                                        */
/* ======================================================================== */

/* cdvdfsv RPC servers on the IOP (ps2sdk spellings).                        */
#define CD_SERVER_INIT        0x80000592    /* sceCdInit: init_mode in, 16-byte reply  */
#define CD_SERVER_SCMD        0x80000593    /* S-commands (fno = command)              */
#define CD_SERVER_NCMD        0x80000595    /* N-commands (fno = command)              */
#define CD_SERVER_POFF        0x80000596    /* power-off notification enable           */
#define CD_SERVER_SEARCHFILE  0x80000597    /* sceCdSearchFile                         */
#define CD_SERVER_DISKREADY   0x8000059A    /* sceCdDiskReady                          */

/* The command numbers (fno) used here (ps2sdk's numbering).                 */
#define CD_NCMD_DISKREADY     0x0E          /* sceCdNCmdDiskReady                      */
#define CD_SCMD_MMODE         0x22          /* sceCdMmode                              */

/* keep_cmd tags of the N-commands (s_ncmd_keep, only shown by the "Ncmd fail
 * sema" debug print): the library's own numbering, not the RPC numbers.     */
#define CD_KEEP_SEARCHFILE      1   /* sceCdSearchFile (its own gate)  */
#define CD_KEEP_NCMD_DISKREADY  2   /* sceCdNCmdDiskReady              */

/* _sceCdDelay period of the sceCdSync / _sceCdSyncS wait loops (H-SYNCs).   */
#define CD_SYNC_DELAY_HSYNC   0x3C

/* sceCdDiskReady mode that turns the busy / RPC-error result into -1 (the
 * other modes get SCECdNotReady).  Its name and the IOP side's use of it are
 * unknown.                                                                  */
#define CD_DISKREADY_MODE_8   8

/* sceCdInit reply word 3 markers.                                           */
#define CD_INIT_REPLY_QUIET   0xFF          /* no version check, debug output stays off */
#define CD_INIT_REPLY_DEBUG   0xFE          /* turn libcdvd's debug output on           */

/* Completion reason that _sceCdCbDispatch does not forward to the user
 * callback; _sceCdPOffCbBind stores the same value in s_poff_sbuf.         */
#define CDVD_CBNUM_POFF   0x0B      /* ? name: meaning inferred from those two uses */

/* ======================================================================== */
/* libcdvd types                                                            */
/* ======================================================================== */

/* sceCdSearchFile send buffer (0x128 bytes, 64-byte aligned).  The IOP
 * writes the sceCdlFILE result back over the head of the buffer, using .addr
 * (the EE address of the buffer itself) as the destination.                */
typedef struct {
    sceCdlFILE  file;           /* +0x000 result (written by the IOP)   */
    char        name[256];      /* +0x024 path to search                */
    void       *addr;           /* +0x124 EE address of this buffer     */
} _sceCdSearchSbuf;             /* 0x128 */

/* The 0x24-byte sceCdlFILE as raw bytes (the copy-out is a byte-aligned
 * block copy: ldl/ldr + sdl/sdr).                                          */
typedef struct {
    u8 b[sizeof(sceCdlFILE)];
} _sceCdFileImage;

/* Unaligned-read head/tail data sent by the IOP (cdvdfsv) for a read buffer
 * that is not 64-byte aligned.  Only read by _sceCdAlignReadCb.            */
typedef struct {
    int   size1;                /* +0x00 bytes of buf1 to copy          */
    int   size2;                /* +0x04 bytes of buf2 to copy          */
    u8   *dest1;                /* +0x08 head destination               */
    u8   *dest2;                /* +0x0C tail destination               */
    u8    buf1[0x40];           /* +0x10 head bytes                     */
    u8    buf2[0x40];           /* +0x50 tail bytes ? size: only the offset is used */
} _sceCdAlignData;

#ifdef __cplusplus
static_assert(sizeof(_sceCdSearchSbuf) == 0x128, "_sceCdSearchSbuf");
#else
_Static_assert(sizeof(_sceCdSearchSbuf) == 0x128, "_sceCdSearchSbuf");
#endif

/* ======================================================================== */
/* libcdvd data (file-static in the original)                               */
/* ======================================================================== */

/* Flags shared with interrupt-context code (SIF command handler, RPC end
 * functions).  Sony's objects declared them plain: the retail code puts their
 * stores in delay slots, where egcs never puts a volatile access, and reuses
 * loaded values (the bootstrap's libcdvd.c, matched).  The GCC 14 build keeps
 * them volatile so that its optimiser cannot drop or move an access the
 * retail code performs; CD_VI below marks the accesses the retail code shows
 * as volatile.                                                              */
#ifdef BOOT_OLD_GCC
#define CD_ISR_VOLATILE
#else
#define CD_ISR_VOLATILE volatile
#endif

/* The library state (.data 0x003043C0..0x00305E30, .bss 0x004AB540..
 * 0x004AB814) is defined in libcdvd.c as file statics, with the
 * core addresses (the data pass, ref/data/u129/notes.md): no other file uses
 * it, and a C static cannot follow an extern of the same name.             */

/* A volatile access to an int where the code shows one (load repeated /
 * kept out of delay slots).                                                 */
#define CD_VI(x)  (*(volatile int *)&(x))
/* The gates see the two mutex ids as volatile, the commands do not.         */
#define NCMD_SEMA_V  CD_VI(s_ncmd_sema)
#define SCMD_SEMA_V  CD_VI(s_scmd_sema)
/* Posting a completion reason (a statement macro in the original).          */
#define CD_SET_CBNUM(n)  do { s_cd_cb_num = (n); } while (0)

/* ======================================================================== */
/* libpad constants                                                         */
/* ======================================================================== */

/* The padman RPC protocol (PAD_RPC_SERVER0 / 1, PAD_RPC_FNO, PAD_RPCCMD_*)
 * is in sdk/libcdvd/G004_libcdvd.h, shared with u130's half of libpad.             */

/* Size of the padArea given to scePadPortOpen: two PadDmaHalf.              */
#define PAD_DMA_AREA_SIZE   (scePadDmaBufferMax * 16)  /* 0x100 */
/* Bytes of PadActDma DMAed to the IOP per transfer, and the stride of the
 * IOP's double buffer.                                                      */
#define PAD_ACT_DMA_SIZE    0x20
/* Actuator bytes in PadActDma.act / in the SETACTALIGN request.             */
#define PAD_ACT_BYTES       6
/* PadDmaHalf.modeCurId of a pad in config (escape) mode.                    */
#define PAD_MODE_ID_CONFIG  0xF3
/* scePadGetButtonMask of a pad in pressure mode (DualShock 2: 18 buttons).  */
#define PAD_BTNMASK_PRESS   0x3FFFF
/* scePadSetButtonInfo argument that turns the pressure-sensitive buttons on. */
#define PAD_BTNINFO_PRESS   0xFFF
/* scePadGetState result for a port/slot that is not open.                  */
#define PAD_STATE_NOT_OPEN  99

/* ======================================================================== */
/* libpad types                                                             */
/* ======================================================================== */

/* One 0x80-byte half of the padArea handed to scePadPortOpen, written by
 * padman over SIF DMA; readers take the half with the larger frame counter.
 * The tag is the one PadPortCtl.padArea (G004_libcdvd.h) points to.         */
typedef struct PadDmaHalf {
    u8  data[0x20];       /* +0x00 received pad data; scePadRead copies `length` bytes */
    u8  unk20[0x10];      /* +0x20 not touched here (ps2sdk: actDirData/actAlignData) */
    u8  actInfo[4][4];    /* +0x30 scePadInfoAct: 4 bytes per actuator */
    u8  actComb[4][4];    /* +0x40 scePadInfoComb: {count, act0, act1, act2} per list */
    u16 modeTable[4];     /* +0x50 extended mode-ID table (scePadInfoMode) */
    int frame;            /* +0x58 frame counter; compared signed between halves */
    int unk5C;            /* +0x5C */
    int length;           /* +0x60 valid bytes at +0x00 */
    u8  modeConfig;       /* +0x64 info stage: >=2 act/comb info valid, 1 = no mode table */
    u8  modeCurId;        /* +0x65 current mode id (high nibble), 0xF3 = config */
    u8  model;            /* +0x66 >=2 required by scePadGetButtonMask */
    u8  unk67;            /* +0x67 cleared by scePadPortOpen */
    u8  nrOfModes;        /* +0x68 entries in modeTable */
    u8  modeCurOffs;      /* +0x69 current index into modeTable */
    u8  nrOfActuators;    /* +0x6A */
    u8  numActComb;       /* +0x6B */
    u8  unk6C[4];         /* +0x6C */
    u8  state;            /* +0x70 scePadState* */
    u8  reqState;         /* +0x71 scePadReqState* */
    u8  ok;               /* +0x72 ==1: pad information valid */
    u8  unk73[6];         /* +0x73 */
    u8  buttonMask[4];    /* +0x79 32-bit button mask, little-endian bytes */
    u8  unk7D[3];         /* +0x7D */
} PadDmaHalf;             /* 0x80 */

/* EE-side actuator block, one per port/slot (0x40 stride, 64-byte aligned);
 * the first 0x20 bytes are DMAed to the IOP by pad_act_dma_transfer.  The
 * tag is the one PadPortCtl.act (G004_libcdvd.h) points to.                 */
typedef struct PadActDma {
    int frame;            /* +0x00 incremented per transfer; bit 0 picks the IOP half */
    int valid;            /* +0x04 set to 1 by scePadSetActDirect */
    int length;           /* +0x08 set to 6 by scePadSetActDirect */
    u8  act[6];           /* +0x0C actuator values */
    u8  pad12[0x40 - 0x12];
} PadActDma;              /* 0x40 */

#ifdef __cplusplus
static_assert(sizeof(PadDmaHalf) == 0x80, "PadDmaHalf");
static_assert(sizeof(PadActDma) == 0x40, "PadActDma");
#else
_Static_assert(sizeof(PadDmaHalf) == 0x80, "PadDmaHalf");
_Static_assert(offsetof(PadDmaHalf, frame) == 0x58, "frame");
_Static_assert(offsetof(PadDmaHalf, length) == 0x60, "length");
_Static_assert(offsetof(PadDmaHalf, state) == 0x70, "state");
_Static_assert(offsetof(PadDmaHalf, buttonMask) == 0x79, "buttonMask");
_Static_assert(sizeof(PadActDma) == 0x40, "PadActDma");
_Static_assert(offsetof(PadActDma, act) == 0x0C, "act");
#endif

/* ======================================================================== */
/* libpad data (file-static in the original)                                */
/* ======================================================================== */

/* s_padClient, s_padCtl and s_padRpcBuf (used by u130 too) are declared in
 * sdk/libcdvd/G004_libcdvd.h and defined (C commons) in libpad_part1.c.  The
 * rest of libpad's state (.data 0x00305E30..0x00305E78: s_padVersion,
 * s_padInitialized, s_padWarning, s_padStateStr, s_padReqStr; .bss
 * s_padActDma 0x004AB980) is defined there as file statics (the data pass,
 * ref/data/u129/notes.md): only that file uses it.                         */

#ifdef __cplusplus
}
#endif

#endif /* U129_H */
