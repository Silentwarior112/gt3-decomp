/* libmc_internal.h - unit u130 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x00272B40..0x00274AD0, part of the no-FDE stretch G004 (split: the rest
 * of it is in u129 and u131).  Three Sony EE libraries back to back:
 *   libpad_part2.c  the tail of libpad 2200 (scePadExitPressMode ..
 *                       pad_rpc_cmd_0x14)                              BOOT
 *   libmc.c   all of libmc 2240 (sceMcInit .. sceMcGetEntSpace)
 *                       BOOT (sceMcWrite and sceMcSync too: the same code as
 *                       the bootstrap's, matched with its statement order)
 *   libscf.c  all of libscf ("libscf.c", 0x002745C0..0x00274F70:
 *                       OSD system configuration getters, T10K overrides, BCD
 *                       helpers, RTC clock arithmetic); its tail from
 *                       0x00274AD0 lies in u131's range (one object, merged
 *                       back into one file)
 * libpad / libmc are the same machine code as the bootstrap's
 * sdk/libpad/libpad.c and sdk/libmc/libmc.c.
 *
 * Canonical headers:
 *   sdk/libcdvd/G004_libcdvd.h      this translation unit: the functions with callers
 *                            outside the unit, PadPortCtl, the libpad data and
 *                            padman commands shared with u129
 *   runtime/newlib/G007_stdio.h        the libkernl syscalls / SIF RPC / fileio API,
 *                            strncpy, memcpy and __assert, core addresses
 *   engine/pdistd/T533_printf.h       printf (the libraries' messages)
 *   engine/media/G002_mpeg_frameq.h  strlen (the game's MMI strlen, 0x00269A68)
 * The SDK headers give the types and constants: struct SemaParam,
 * ExitHandler() and WRITEBACK_DCACHE (eekernel.h), SIF_RPCM_NOWAIT (sif.h),
 * SCE_RDONLY (sifdev.h), scePadReqStateBusy and the prototypes of the pad
 * functions nobody calls (libpad.h), the sceMcFunc* / sceMcExec* codes,
 * sceMcTblGetDir and the libmc prototypes (libmc.h), sceCdCLOCK (libcdvd.h).
 *
 * Everything below is local to the unit.  The library state is file-static
 * data in the original, and so it is in the C (the data pass): libmc's and
 * libscf's own files define it; libpad's (one object in the original, split
 * between u129 and u130) is u129's, declared in sdk/libcdvd/G004_libcdvd.h.
 */
#ifndef U130_H
#define U130_H

#include "types.h"
#include "eekernel.h"
#include "sif.h"
#include "sifdev.h"
#include "libcdvd.h"
#include "libpad.h"
#include "libmc.h"
#include <stddef.h>
#include "sdk/libcdvd/G004_libcdvd.h"
#include "runtime/newlib/G007_stdio.h"
#include "engine/pdistd/T533_printf.h"
#include "engine/media/G002_mpeg_frameq.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================== */
/* libpad (tail)                                                            */
/* ======================================================================== */

/* scePadSetVrefParam: bytes of parameter data at s_padRpcBuf words 3..5.    */
#define PAD_VREF_PARAM_SIZE  12

/* ======================================================================== */
/* libmc                                                                    */
/* ======================================================================== */

#define MCSERV_RPC_ID      0x80000400   /* mcserv.irx SIF RPC server id        */
#define MC_RPC_INIT        0xFE         /* sceMcInit handshake function number */

#define MC_ERR_SEMA        (-200)       /* PollSema failed: request pending    */
#define MC_ERR_NOT_BOUND   (-100)       /* client not bound (no sceMcInit)     */
#define MC_ERR_NAME        (-210)       /* NULL or empty path argument         */
#define MC_ERR_OLD_MCSERV  (-120)       /* mcserv.irx version < 0x20A          */
#define MC_ERR_OLD_MCMAN   (-121)       /* mcman.irx version < 0x20E           */
/* sceMcInit: a failed handshake RPC returns its error minus this.           */
#define MC_ERR_RPC_BIAS    100

#define MC_MIN_MCSERV_VER  0x20A
#define MC_MIN_MCMAN_VER   0x20E

/* sceMcInit: iterations of the busy wait between two bind polls.           */
#define MC_BIND_SPIN       0x100000
/* sceMcSync: SetAlarm delay between two RPC status polls (H-SYNC units).    */
#define MC_SYNC_DELAY_HSYNC 0x3C

/* Receive sizes: the asynchronous calls take back one result word, the
 * sceMcInit handshake three (result, mcserv version, mcman version).        */
#define MC_RDATA_SIZE      4
#define MC_INIT_RDATA_SIZE 12

/* sceMcWrite: writes of up to this many bytes go inline in McDescParam.data. */
#define MC_WRITE_INLINE_MAX 16

/* sceMcRename's "valid" word: the EntryName field of the info block.        */
#define MC_FILEINFO_NAME   0x10

/* fd / port-slot request, 0x30 bytes (s_mcDesc). */
typedef struct {
    int   fd;          /* +0x00 Close/Seek/Read/Write/Flush */
    int   port;        /* +0x04 GetInfo/Format/Unformat/GetSlotMax */
    int   slot;        /* +0x08 */
    int   size;        /* +0x0C Read: size; Write: aligned-part size; GetInfo: format wanted */
    int   offset;      /* +0x10 Seek: offset; GetInfo: free wanted */
    int   origin;      /* +0x14 Seek: whence; Write: inline byte count; GetInfo: type wanted;
                                ChangeThreadPriority: level */
    void *buffer;      /* +0x18 Read: user buffer; Write: 16-byte aligned remainder */
    void *param;       /* +0x1C Read/GetInfo: &s_mcEndPara */
    u8    data[MC_WRITE_INLINE_MAX]; /* +0x20 Write: unaligned head / small write, inline */
} McDescParam;         /* 0x30 */
_Static_assert(sizeof(McDescParam) == 0x30, "McDescParam");

/* path request, 0x414 bytes (s_mcName). */
typedef struct {
    int   port;        /* +0x000 */
    int   slot;        /* +0x004 */
    int   flags;       /* +0x008 Open: mode; GetDir: mode; SetFileInfo: valid; Delete: 0 */
    int   maxent;      /* +0x00C GetDir */
    void *table;       /* +0x010 GetDir: user table; Chdir: s_mcPwdBuf; SetFileInfo/Rename: &s_mcFileInfo */
    char  name[0x400]; /* +0x014 strncpy'd, 0x3FF chars max, [0x3FF] forced to 0 */
} McNameParam;         /* 0x414 */
_Static_assert(sizeof(McNameParam) == 0x414, "McNameParam");

/* Longest path copied into McNameParam.name (the last byte is forced to 0). */
#define MC_NAME_MAX        (sizeof(s_mcName.name) - 1)

/* End-parameter area that mcserv writes back (s_mcEndPara, 0xC0 bytes, read
 * through the uncached mirror by the end callbacks).
 * Read: unaligned head/tail fragments of the user buffer.
 * GetInfo: type in the size1 word, free in size2, format at +0x90. */
typedef struct {
    int   size1;                      /* +0x00 Read: head bytes;  GetInfo: type */
    int   size2;                      /* +0x04 Read: tail bytes;  GetInfo: free */
    u8   *dest1;                      /* +0x08 */
    u8   *dest2;                      /* +0x0C */
    u8    src1[64];                   /* +0x10 */
    u8    src2[64];                   /* +0x50 */
    int   formatted;                  /* +0x90 GetInfo: format */
    u8    pad94[0xC0 - 0x94];         /* +0x94 */
} McEndParam;                         /* 0xC0 */
_Static_assert(sizeof(McEndParam) == 0xC0, "McEndParam");
_Static_assert(offsetof(McEndParam, src2) == 0x50, "src2");
_Static_assert(offsetof(McEndParam, formatted) == 0x90, "formatted");

/* The libmc state (Bootstrap -> core: .data 0x0103BE78 -> 0x00305E78, .bss
 * 0x01048940 -> 0x004ABC00, same layouts) is file-static data of
 * libmc.c, defined there: s_mcVersion, s_mcFuncNo, s_mcSema
 * (.data), s_mcClient .. s_mcRdata (.bss, the SIF RPC buffers 64-aligned). */

sceSifClientData *mc_get_rpc_state(int **rdata, int **func_no);   /* 0x00272FC0 */

/* ======================================================================== */
/* libscf                                                                   */
/* ======================================================================== */

/* The assert() of libscf.c (newlib assert.h, expanded at the given libscf.c
 * source line): __assert(__FILE__, __LINE__, #e) when e is false. */
#define SCF_ASSERT(e, line) ((e) ? (void)0 : __assert("libscf.c", (line), #e))

/* RTC (Japan time) minus GMT: 9 h. */
#define SCF_JST_OFFSET_MIN  (9 * 60)

/* sceScfSetT10kConfig's block: the values the getters return on a T10K
 * (DTL-T10000 tool), whose OSD config is not used.  8 bytes, 2-aligned
 * (copied with an unaligned ldl/ldr + sdl/sdr doubleword). */
typedef struct {
    short TimeZone;       /* +0 minutes from GMT */
    u8    Aspect;         /* +2 */
    u8    DateNotation;   /* +3 */
    u8    Language;       /* +4 */
    u8    Spdif;          /* +5 */
    u8    SummerTime;     /* +6 */
    u8    TimeNotation;   /* +7 */
} sceScfT10kConfig;       /* 8 */
_Static_assert(sizeof(sceScfT10kConfig) == 8, "sceScfT10kConfig");

/* The libscf state is file-static data of libscf.c, defined there:
 * s_scfVersion, s_scfT10kConfig, s_scfRomVer (.data 0x00305E90..0x00305EB8). */

/* rom0:ROMVER: bytes read into s_scfRomVer, and the character that tells a
 * T10K (position 4, after the 4-digit ROM version).                        */
#define SCF_ROMVER_READ_SIZE  14
#define SCF_ROMVER_REGION     4
#define SCF_ROMVER_T10K       'T'

/* GetOsdConfigParam word (ps2sdk's ConfigParam bit fields): bit 0 spdif,
 * bits 1-2 screen type (aspect), bit 4 language when the version is 0
 * (0 Japanese, 1 English), bits 13-15 version, bits 16-20 language, bits
 * 21-31 time zone offset in minutes (signed).                              */
#define SCF_OSD_SPDIF(p)        ((p) & 1)
#define SCF_OSD_ASPECT(p)       (((p) >> 1) & 3)
#define SCF_OSD_JAP_LANGUAGE(p) (((p) >> 4) & 1)
#define SCF_OSD_VERSION(p)      (((p) >> 13) & 7)
#define SCF_OSD_LANGUAGE(p)     (((p) >> 16) & 0x1F)
#define SCF_OSD_TIMEZONE(p)     ((s32)(p) >> 21)

/* Time zone of an OSD config of version 0 (no time zone field): +9 h, JST. */
#define SCF_DEFAULT_TIMEZONE    0x21C

/* GetOsdConfigParam2(&b, 1, SCF_OSD2_TIME_BYTE): the one byte the getters
 * read (ps2sdk's Config2Param byte 1).  A one-byte struct of bit fields, as
 * the retail frame shows: egcs keeps a struct (no mode promotion) out of
 * memory until after the u32 config word, so the word lands at sp+0 and this
 * byte at sp+4 (a plain u8 local gets its slot first, at sp+0).             */
#define SCF_OSD2_TIME_BYTE      1
typedef struct {
    u8 reserved     : 4;
    u8 summerTime   : 1;  /* bit 4: 1 = daylight saving time */
    u8 timeNotation : 1;  /* bit 5: 0 = 24 h, 1 = 12 h */
    u8 dateNotation : 2;  /* bits 6-7: 0 YYYYMMDD, 1 MMDDYYYY, 2 DDMMYYYY */
} ScfOsdConfig2Time;
_Static_assert(sizeof(ScfOsdConfig2Time) == 1, "ScfOsdConfig2Time");

char *scf_get_romver(void);                     /* 0x002745C0 */
int   scf_is_t10k(void);                        /* 0x00274660 */
int   sceScfGetLanguage(void);                  /* 0x002746A0 */
void  sceScfSetT10kConfig(sceScfT10kConfig *config); /* 0x00274700 */
int   sceScfGetAspect(void);                    /* 0x00274720 */
int   sceScfGetSpdif(void);                     /* 0x00274760 */
int   sceScfGetTimeZone(void);                  /* 0x002747A0 minutes from GMT */
int   sceScfGetDateNotation(void);              /* 0x00274808 */
int   sceScfGetSummerTime(void);                /* 0x00274888 0 / 1 */
int   sceScfGetTimeNotation(void);              /* 0x00274908 */
u8    scf_bin_to_bcd(u8 c);                     /* 0x00274988 (asserts c <= 99) */
u8    scf_bcd_to_bin(u8 c);                     /* 0x002749F0 */
void  scf_clock_bcd_to_bin(sceCdCLOCK *prtc);   /* 0x00274A48 the whole clock, BCD -> binary */
/* The clock helpers: only the two sceScfGet*fromRTC reach them (probably
 * `static' in the original). */
void  scf_clock_bin_to_bcd(sceCdCLOCK *prtc);        /* 0x00274AD0 */
void  scf_clock_next_day(sceCdCLOCK *prtc);          /* 0x00274B58 */
void  scf_clock_prev_day(sceCdCLOCK *prtc);          /* 0x00274C40 */
void  scf_clock_next_hour(sceCdCLOCK *prtc);         /* 0x00274D18 */
void  scf_clock_prev_hour(sceCdCLOCK *prtc);         /* 0x00274D88 */
void  scf_clock_adjust(sceCdCLOCK *prtc, int diff);  /* 0x00274DF0 */

#ifdef __cplusplus
}
#endif

#endif /* U130_H */
