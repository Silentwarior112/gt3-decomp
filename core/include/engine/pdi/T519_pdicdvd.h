/* T519_pdicdvd.h - pdicdvd.cxx: disc reads and the clock over the IOP
 * (translation unit T519, unit u119). */
#ifndef GAME_T519_PDICDVD_H
#define GAME_T519_PDICDVD_H

#include "game/game.h"
#include "libcdvd.h"                            /* sceCdCLOCK (CdClock's layout) */

/* The file's two RPC objects are of classes of its anonymous namespace
 * (_GLOBAL_$N$.._.._pdicdvd.cxxs1Kop7), both derived from PDIClientRpc (T527):
 *   RPC   vtable 0x00345818, typeinfo 0x0025BEE8, 0xC0 bytes; object 0x004A6B00,
 *         server 'PCDV' (0x50434456): disc reads (fno 3), the disc identity
 *         (fno 2), fno 1.  RPC() 0x0025B490, ~RPC() [0] +0x0C 0x0025B510,
 *         onCallEnd() [1] +0x14 0x0025B538; inline: isPending 0x0025BCF0,
 *         waitRead 0x0025BD38, read 0x0025BD68, setDiscId 0x0025BE10,
 *         F_0025BE70, F_0025BEC8.
 *   RPC2  vtable 0x00345800, typeinfo 0x0025BCA0, 0x80 bytes; object 0x004A6A80,
 *         server 'Pcdv' (0x50636476): the clock (fno 2), fno 1.  RPC2()
 *         0x0025B568, ~RPC2() [0] +0x0C 0x0025B5B0, [1] PDIClientRpc::onCallEnd;
 *         inline: getTime 0x0025BC08, F_0025BC80.
 * They stay defined in src/engine/pdi/T519_pdicdvd.cpp: every pdi*.cxx has its own
 * class RPC, so one anonymous namespace in the headers would define RPC twice
 * in a file that includes two of them (u001 calls all five pdi*_init). */

/* A date as the CD clock gives it (sceCdCLOCK's layout, libcdvd.h):
 * pdicdvd_time_to_clock* fill one from a packed time; T465 keeps three, the
 * records screen (T092) and the memory-card files read them.  The fields are
 * BCD.  Not polymorphic, 8 bytes, byte aligned.  (A struct of its own, not a
 * typedef of sceCdCLOCK: T465 forward-declares struct CdClock, and
 * sceCdCLOCK is a typedef of an unnamed struct.) */
struct CdClock {
    u8 stat;                    /* 0x0 */
    u8 second;                  /* 0x1 */
    u8 minute;                  /* 0x2 */
    u8 hour;                    /* 0x3 */
    u8 pad;                     /* 0x4 */
    u8 day;                     /* 0x5 */
    u8 month;                   /* 0x6 */
    u8 year;                    /* 0x7 2 digits (years after 2000) */
};
EGCS_SIZE(struct CdClock, 0x8);

/* The disc identity sent to the IOP once the volume descriptor is read
 * (fno 2 of 'PCDV'). */
typedef struct PdiDiscId {
    u32 pvdLsn;                 /* 0x0 lsn of the primary volume descriptor */
    int checksum;               /* 0x4 iso9660_checksum of that sector */
} PdiDiscId;
EGCS_SIZE(PdiDiscId, 8);

void pdicdvd_init(void);                                    /* 0x0025B5D8 binds 'PCDV' and 'Pcdv' */
void F_0025B620(void);                                      /* 0x0025B620 'PCDV' fno 1 (NOWAIT), sent when an iso9660 lookup fails */
void pdicdvd_set_disc_id(const PdiDiscId *id);              /* 0x0025B648 */
/* Blocking read of size bytes at lsn into buf; 1 = read OK. */
int  pdicdvd_read(void *buf, u32 lsn, u32 size, int mode);  /* 0x0025B6A8 */
void pdicdvd_read_start(void *buf, u32 lsn, u32 size, int mode); /* 0x0025B6D0 NOWAIT */
int  pdicdvd_read_wait(void);                               /* 0x0025B780 1 = read OK */
int  pdicdvd_read_pending(void);                            /* 0x0025B7B8 */
void F_0025B800(void);                                      /* 0x0025B800 'Pcdv' fno 1 (NOWAIT) */
/* The IOP clock: low word the seconds since 2000-01-01 00:00, high word
 * the stat byte. */
u64  pdicdvd_get_time(void);                                /* 0x0025B828 */
/* t (pdicdvd_get_time) as BCD fields; returns out->stat. */
u8   pdicdvd_time_to_clock(struct CdClock *out, u64 t);     /* 0x0025B848 */
/* pdicdvd_time_to_clock, then sceScfGetGMTfromRTC(out) under the libscf
 * lock (G002 scf_clock_to_gmt). */
u8   pdicdvd_time_to_clock_gmt(struct CdClock *out, u64 t); /* 0x0025BB20 */
/* pdicdvd_time_to_clock, then sceScfGetLocalTimefromRTC(out) under the
 * libscf lock (G002 scf_clock_to_local). */
u8   pdicdvd_time_to_clock_local(struct CdClock *out, u64 t); /* 0x0025BB60 */
/* T519_static_fini 0x0025BBA0 (.dtors) and T519_static_init 0x0025BBD8 (.ctors) are
 * egcs's own static destructor / initialiser of the two objects (no source
 * functions: the objects are defined in the .cpp file). */

#if defined(__cplusplus) && !defined(BOOT_OLD_GCC)  /* egcs generates them itself */
extern "C" {
void *tf_pdicdvd_RPC2(void);    /* 0x0025BCA0 __tfQ234_GLOBAL_$N$.._.._pdicdvd.cxxs1Kop74RPC2 */
void *tf_pdicdvd_RPC(void);     /* 0x0025BEE8 __tfQ234_GLOBAL_$N$.._.._pdicdvd.cxxs1Kop73RPC */
}
#endif

#endif /* GAME_T519_PDICDVD_H */
