/* G004_libcdvd.h - the no-FDE stretch G004 (units u129, u130, u131, C): Sony EE libraries,
 * newlib libm and libgcc2, back to back.
 *
 * Modules (link order):
 *   libcdvd "PsIIlibcdvd 2240"  0x00270628..0x00271A60  (u129)
 *   libpad  "PsIIlibpad  2200"  0x00271A60..0x00272E08  (u129, tail in u130)
 *   libmc   2240                0x00272E08..0x002745C0  (u130)
 *   libscf  ("libscf.c")        0x002745C0..0x00274F70  (u130; tail in u131's range)
 *   libm    (newlib fdlibm 5.x) 0x00274F70..0x00276EA8  (u131)
 *   libgcc2 __main.o, _divdi3.o 0x00276EA8..0x002776C0  (u131)
 * libcdvd / libpad / libmc are the same machine code as the bootstrap's
 * sdk/ sources; their types and constants are in the shared include/libcdvd.h,
 * libpad.h, libmc.h (address comments there are the bootstrap's; the core
 * addresses are below).  The libgcc2.c integer types are defined here for the
 * whole runtime (G005, G006 include this header).
 */
#ifndef GAME_G004_LIBCDVD_H
#define GAME_G004_LIBCDVD_H

#include "game/game.h"
#include "sif.h"                                /* sceSifClientData */
#include "libcdvd.h"                            /* sceCdCLOCK */
#include "libmc.h"                              /* sceMcTblGetDir */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------ */
/* libgcc2.c types (LIBGCC2_UNITS_PER_WORD = 4: a word is 32 bits)          */
/* ------------------------------------------------------------------------ */

typedef unsigned char       UQItype;
typedef unsigned short      UHItype;
typedef int                 SItype;
typedef unsigned int        USItype;
typedef long long           DItype;
typedef unsigned long long  UDItype;
typedef float               SFtype;
typedef double              DFtype;

typedef SItype  Wtype;
typedef USItype UWtype;
typedef DItype  DWtype;
typedef UDItype UDWtype;

#define W_TYPE_SIZE 32

/* `int __attribute__ ((mode (__word__)))': word_mode is DImode on the EE
 * (64-bit GPRs; T542's frame_state word_type members are ld / sd), so a
 * 64-bit type (u131's __divdi3 flag, u132's register words, u133's __moddi3
 * flag). */
typedef long long           word_type;

/* Little-endian double word (LIBGCC2_WORDS_BIG_ENDIAN = 0): low word first. */
struct DWstruct {
    Wtype low;                                  /* 0x0 */
    Wtype high;                                 /* 0x4 */
};
EGCS_SIZE(struct DWstruct, 0x8);

typedef union {
    struct DWstruct s;
    DWtype ll;
} DWunion;

/* ------------------------------------------------------------------------ */
/* libpad data shared by its two halves                                     */
/* ------------------------------------------------------------------------ */

struct PadDmaHalf;      /* one 0x80-byte half of the padArea given to scePadPortOpen */
struct PadActDma;       /* EE actuator block, 0x40 bytes, one per port/slot (0x004AB980) */

/* Per port/slot control entry, [2 ports][4 slots]. */
typedef struct PadPortCtl {
    struct PadDmaHalf *padArea;                 /* 0x00 user DMA area (two halves) */
    struct PadActDma  *act;                     /* 0x04 &s_padActDma[port][slot] */
    u32                iopAct;                  /* 0x08 IOP actuator buffer (OPEN reply word 5) */
    u32                dmaId;                   /* 0x0C id of the last actuator sceSifSetDma */
    int                open;                    /* 0x10 1 = port/slot open */
    int                unk14;                   /* 0x14 only cleared (pad_init_rpc) */
    int                unk18;                   /* 0x18 only cleared (pad_init_rpc) */
} PadPortCtl;
EGCS_SIZE(PadPortCtl, 0x1C);

/* File-static in the original (libpad .bss 0x004AB840..). */
extern sceSifClientData s_padClient[2];         /* 0x004AB840 padman servers 0x80000100, 0x80000101 */
extern PadPortCtl       s_padCtl[2][4];         /* 0x004AB890 */
extern u32              s_padRpcBuf[32];        /* 0x004ABB80 shared 0x80-byte RPC in / out buffer */

/* The padman RPC protocol of both halves.  Every command is
 * sceSifCallRpc(&s_padClient[0], PAD_RPC_FNO, 0, s_padRpcBuf, 0x80,
 * s_padRpcBuf, 0x80, NULL, NULL); word 0 of s_padRpcBuf = the command,
 * words 1 / 2 = port / slot, words 3.. arguments and results.               */
#define PAD_RPC_SERVER0   0x80000100    /* padman: every command goes here  */
#define PAD_RPC_SERVER1   0x80000101    /* padman: bound by scePadInit, unused */
#define PAD_RPC_FNO       1             /* the only function number used    */

/* Word 0 of s_padRpcBuf: the padman command (ps2sdk spellings).            */
#define PAD_RPCCMD_OPEN         0x01
#define PAD_RPCCMD_SET_MMODE    0x06
#define PAD_RPCCMD_SET_ACTALIGN 0x08
#define PAD_RPCCMD_SET_BTNINFO  0x0A
#define PAD_RPCCMD_SET_VREF     0x0B    /* scePadSetVrefParam */
#define PAD_RPCCMD_GET_PORTMAX  0x0C
#define PAD_RPCCMD_GET_SLOTMAX  0x0D
#define PAD_RPCCMD_CLOSE        0x0E
#define PAD_RPCCMD_END          0x0F
#define PAD_RPCCMD_INIT         0x10
#define PAD_RPCCMD_GET_MODVER   0x12
#define PAD_RPCCMD_14           0x14    /* ? name: pad_rpc_cmd_0x14 (not in ps2sdk's list) */

/* ------------------------------------------------------------------------ */
/* libcdvd                                                                  */
/* ------------------------------------------------------------------------ */

int sceCdInit(int init_mode);                                       /* 0x002714C0 */
int sceCdMmode(int media);                                          /* 0x00271998 */

/* ------------------------------------------------------------------------ */
/* libpad                                                                   */
/* ------------------------------------------------------------------------ */

int scePadInit(int mode);                                           /* 0x00271B90 */
int scePadEnd(void);                                                /* 0x00271D70 */
int scePadPortOpen(int port, int slot, u128 *addr);                 /* 0x00271DF0 */
int scePadRead(int port, int slot, unsigned char *rdata);           /* 0x00272140 */
int scePadGetState(int port, int slot);                             /* 0x002721C0 */
int scePadSetReqState(int port, int slot, int state);               /* 0x00272270 */
int scePadGetReqState(int port, int slot);                          /* 0x002722D8 */
int scePadInfoAct(int port, int slot, int actno, int term);         /* 0x00272360 */
int scePadInfoMode(int port, int slot, int term, int offs);         /* 0x002725A0 */
int scePadSetMainMode(int port, int slot, int offs, int lock);      /* 0x002726D8 */
int scePadSetActDirect(int port, int slot, const unsigned char *data);  /* 0x00272790 */
int scePadSetActAlign(int port, int slot, const unsigned char *data);   /* 0x00272848 */
int scePadSetButtonInfo(int port, int slot, int info);              /* 0x002729D8 */
int scePadInfoPressMode(int port, int slot);                        /* 0x00272A88 */
int scePadEnterPressMode(int port, int slot);                       /* 0x00272AE8 */
int scePadGetModVersion(void);                                      /* 0x00272D38 */
/* padman RPC command 0x14 with one argument word (? name). */
int pad_rpc_cmd_0x14(int arg);                                      /* 0x00272DA0 */

/* ------------------------------------------------------------------------ */
/* libmc                                                                    */
/* ------------------------------------------------------------------------ */

int sceMcInit(void);                                                /* 0x00272E08 */
int sceMcGetSlotMax(int port);                                      /* 0x002730A8 */
int sceMcOpen(int port, int slot, const char *name, int mode);      /* 0x00273168 */
int sceMcMkdir(int port, int slot, const char *name);               /* 0x002732A0 */
int sceMcClose(int fd);                                             /* 0x002732D8 */
int sceMcSeek(int fd, int offset, int origin);                      /* 0x00273390 */
int sceMcRead(int fd, void *buf, int size);                         /* 0x002734F8 */
int sceMcWrite(int fd, const void *buf, int size);                  /* 0x00273618 */
int sceMcSync(int mode, int *cmd, int *result);                     /* 0x00273808 */
int sceMcGetInfo(int port, int slot, int *type, int *free, int *format);    /* 0x00273940 */
int sceMcGetDir(int port, int slot, const char *name, unsigned int mode,
                int maxent, sceMcTblGetDir *table);                 /* 0x00273AC8 */
int sceMcFormat(int port, int slot);                                /* 0x00273DF8 */
int sceMcDelete(int port, int slot, const char *name);              /* 0x00273EC0 */
int sceMcSetFileInfo(int port, int slot, const char *name, const char *info,
                     unsigned int valid);                           /* 0x002740A0 */
int sceMcRename(int port, int slot, const char *org, const char *new_name);  /* 0x00274278 */
int sceMcUnformat(int port, int slot);                              /* 0x002743E0 */
int sceMcGetEntSpace(int port, int slot, const char *path);         /* 0x002744A8 */

/* ------------------------------------------------------------------------ */
/* libscf                                                                   */
/* ------------------------------------------------------------------------ */

void sceScfGetGMTfromRTC(sceCdCLOCK *prtc);     /* 0x00274EC0 RTC (JST) -> GMT */
void sceScfGetLocalTimefromRTC(sceCdCLOCK *prtc);   /* 0x00274F08 RTC (JST) -> local time */

/* ------------------------------------------------------------------------ */
/* libm (fdlibm; every double operation is a dp-bit call, G005)             */
/* ------------------------------------------------------------------------ */

double cos(double x);                           /* 0x00274F70 */
double sin(double x);                           /* 0x00275078 */
double sqrt(double x);                          /* 0x00275170 w_sqrt wrapper (matherr / errno) */
double __ieee754_sqrt(double x);                /* 0x002757A0 */

/* ------------------------------------------------------------------------ */
/* libgcc2                                                                  */
/* ------------------------------------------------------------------------ */

/* collect2's lists: __CTOR_LIST__ 0x0028F020 (.ctors, count word 110),
 * __DTOR_LIST__ 0x0028F1E0 (.dtors). */
void   __do_global_dtors(void);                 /* 0x00276EA8 */
void   __do_global_ctors(void);                 /* 0x00276F00 __CTOR_LIST__ backwards (reached by __main's tail jump) */
void   __main(void);                            /* 0x00276FB0 runs __do_global_ctors once */
DWtype __divdi3(DWtype u, DWtype v);            /* 0x00276FD0 */

#ifdef __cplusplus
}
#endif

#endif /* GAME_G004_LIBCDVD_H */
