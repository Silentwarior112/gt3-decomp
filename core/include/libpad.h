/* core decomp: copied from bootstrap/include/libpad.h. Address comments in it are the
 * bootstrap's (0x010xxxxx); the core addresses of shared code are in ref/libmatch.json. */
/* libpad.h - Sony EE libpad (PsIIlibpad 2200) as linked into the GT3 US
 * bootstrap (BOOTSTRAP_SCUS-97102).  Sony SDK 2.2-era names.
 *
 * Owner: libpad unit (sdk/libpad/libpad.c, 0x01014410..0x010157B8).
 * padman RPC servers 0x80000100 / 0x80000101; one 0x80-byte command buffer
 * (@0x010488C0), fno always 1, word 0 = command:
 *   1 OPEN, 6 SETMAINMODE, 8 SETACTALIGN, 0xA SETBUTTONINFO, 0xB SETVREFPARAM,
 *   0xC GETPORTMAX, 0xD GETSLOTMAX, 0xE CLOSE, 0xF END, 0x10 INIT,
 *   0x12 GETMODVERSION, 0x14 (unknown, 0x01015750).
 * Debug output goes through the libc printf (Polyphony's, libc.h).
 *
 * main's use: scePadInit(0); scePadPortOpen(0, 0, pad_dma_buf @0x01042080);
 * poll scePadGetState(0,0) until 0 / 2 / 6 (up to 16 frames); then
 * scePadRead(0, 0, buf) and test (0xFFFF ^ (buf[2]<<8 | buf[3])) & SCE_PADstart.
 */
#ifndef BOOT_LIBPAD_H
#define BOOT_LIBPAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DMA area handed to scePadPortOpen: 16 quadwords (256 bytes, 64-byte
 * aligned) = two ping-pong 0x80 halves written by padman.                  */
#define scePadDmaBufferMax  16

/* scePadGetState */
#define scePadStateDiscon    0
#define scePadStateFindPad   1
#define scePadStateFindCTP1  2
#define scePadStateExecCmd   5
#define scePadStateStable    6
#define scePadStateError     7

/* scePadGetReqState */
#define scePadReqStateComplete  0
#define scePadReqStateFaild     1   /* sic (Sony spelling) */
#define scePadReqStateBusy      2

/* scePadInfoMode term */
#define InfoModeCurID       1
#define InfoModeCurExID     2
#define InfoModeCurExOffs   3
#define InfoModeIdTable     4

/* scePadInfoAct term */
#define InfoActFunc         1
#define InfoActSub          2
#define InfoActSize         3
#define InfoActCurr         4

/* Button bits of (0xFFFF ^ ((rdata[2] << 8) | rdata[3])). */
#define SCE_PADLup      (1 << 12)
#define SCE_PADLdown    (1 << 14)
#define SCE_PADLleft    (1 << 15)
#define SCE_PADLright   (1 << 13)
#define SCE_PADRup      (1 << 4)
#define SCE_PADRdown    (1 << 6)
#define SCE_PADRleft    (1 << 7)
#define SCE_PADRright   (1 << 5)
#define SCE_PADi        (1 << 9)    /* L3 */
#define SCE_PADj        (1 << 10)   /* R3 */
#define SCE_PADk        (1 << 8)    /* SELECT */
#define SCE_PADl        (1 << 3)    /* R1 */
#define SCE_PADm        (1 << 1)    /* R2 */
#define SCE_PADn        (1 << 2)    /* L1 */
#define SCE_PADo        (1 << 0)    /* L2 */
#define SCE_PADh        (1 << 11)   /* START */
#define SCE_PADL1       SCE_PADn
#define SCE_PADL2       SCE_PADo
#define SCE_PADR1       SCE_PADl
#define SCE_PADR2       SCE_PADm
#define SCE_PADstart    SCE_PADh    /* 0x800: main's "hold START" check */
#define SCE_PADselect   SCE_PADk

int   scePadInit(int mode);                                                /* 0x01014540 */
int   scePadEnd(void);                                                     /* 0x01014720 */
/* addr: 64-byte aligned u128[scePadDmaBufferMax]. */
int   scePadPortOpen(int port, int slot, u128 *addr);                      /* 0x010147A0 */
int   scePadPortClose(int port, int slot);                                 /* 0x01014988 */
/* Copies the newest half's data (length word at +0x60) into rdata and
 * returns that length; returns 0 if the port/slot is not open.            */
int   scePadRead(int port, int slot, unsigned char *rdata);                /* 0x01014AF0 */
int   scePadGetState(int port, int slot);                                  /* 0x01014B70 */
int   scePadGetReqState(int port, int slot);                               /* 0x01014C88 */
int   scePadSetReqState(int port, int slot, int state);                    /* 0x01014C20 */
void  scePadStateIntToStr(int state, unsigned char *str);                  /* 0x01014BE8 */
void  scePadReqIntToStr(int state, unsigned char *str);                    /* 0x01014CD8 */
int   scePadInfoAct(int port, int slot, int actno, int term);              /* 0x01014D10 (+0x6A count, +0x30 table; verified) */
int   scePadInfoComb(int port, int slot, int listno, int offs);            /* 0x01014E30 */
int   scePadInfoMode(int port, int slot, int term, int offs);              /* 0x01014F50 */
int   scePadSetMainMode(int port, int slot, int offs, int lock);           /* 0x01015088 */
int   scePadSetActDirect(int port, int slot, const unsigned char *data);   /* 0x01015140 */
int   scePadSetActAlign(int port, int slot, const unsigned char *data);    /* 0x010151F8 */
int   scePadGetButtonMask(int port, int slot);                             /* 0x010152D0 */
int   scePadSetButtonInfo(int port, int slot, int info);                   /* 0x01015388 */
int   scePadInfoPressMode(int port, int slot);                             /* 0x01015438 */
int   scePadEnterPressMode(int port, int slot);                            /* 0x01015498 */
int   scePadExitPressMode(int port, int slot);                             /* 0x010154F0 */
int   scePadSetVrefParam(int port, int slot, const unsigned char *data);   /* 0x01015548 */
int   scePadGetPortMax(void);                                              /* 0x01015618 */
int   scePadGetSlotMax(int port);                                          /* 0x01015680 */
int   scePadGetModVersion(void);                                           /* 0x010156E8 */
/* Newest half's frame counter; official 2.2 name unconfirmed.             */
int   scePadGetFrameCount(int port, int slot);                             /* 0x01014AA0 ? name */

/* libpad internals (libpad unit only):
 *   0x01014410 pad_act_dma_transfer(port, slot)   0x01014680 pad_init_rpc(mode)
 *   0x01014A40 pad_get_recv_buf(port, slot)       0x01014984 4-byte stub
 *   0x01015750 padman RPC command 0x14 (arg @+4, result @+8, no callers; name unknown) */

#ifdef __cplusplus
}
#endif

#endif /* BOOT_LIBPAD_H */
