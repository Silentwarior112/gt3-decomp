/* pdi_internal.h - unit u119 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x0025B0C8..0x0025E548, second pass (on the canonical headers).
 *
 * Translation units (link order) and their canonical headers:
 *   T528  sif_call_rpc: sceSifCallRpc with the D-cache written back by hand
 *         engine/pdi/T528_sif_call_rpc.h (C linkage)
 *   T527  PDIClientRpc: the EE side of one Polyphony "pdi" IOP RPC server
 *         engine/pdi/T527_pdi_client_rpc.h
 *   T519  pdicdvd.cxx   disc reads ('PCDV') and the clock ('Pcdv')
 *         engine/pdi/T519_pdicdvd.h
 *   T520  pdispu2.cxx   SPU2 state / transfers ('SPUP', 'SPUV')
 *         engine/pdi/T520_pdispu2.h (C linkage)
 *   T521  pdibgm.cxx    streamed BGM ('PBGM')         engine/pdi/T521_pdibgm.h
 *   T522  pdimpeg.cxx   MPEG movie streams ('MPG1', 'MPG2')
 *         engine/pdi/T522_pdimpeg.h
 *   T523  pdipipe.cxx   streamed file reads ('PDIP')  engine/pdi/T523_pdipipe.h
 *   T524  IOPLoadModuleBase: the IOP reboot / module load script runner
 *         engine/pdi/T524_iop_load_module_base.h
 *   T525  a sector cache over the pdicdvd reads       engine/io/T525_sector_cache.h
 *   T526  an ISO 9660 path lookup over the sector cache engine/io/T526_iso9660.h
 *
 * egcs ABI reminders (ref/CONTEXT.md): the vptr is a data member placed after
 * the data members of the first polymorphic class of a hierarchy.  Offsets in
 * the comments are the original (egcs) ones.  PDIClientRpc's vptr is its last
 * word (+0x38), so the members of the derived RPC classes keep their offsets
 * in a GCC 14 build as well.
 *
 * The classes of the pdi*.cxx files live in their files' anonymous
 * namespaces (_GLOBAL_$N$.._.._pdicdvd.cxxs1Kop7::RPC, ...); every file has
 * its own `RPC', so they are defined in the .cpp files, not here.  Their
 * pad_ fields look like alignment (? an aligned attribute in the original):
 * every send area starts on 16 bytes (+0x40 / +0x50), pdicdvd's receive area
 * on 64 bytes (+0x80; that object is at 0x004A6B00), as the D-cache
 * write-backs of SIF transfers want.
 *
 * This header keeps what only u119 uses: the translation units' private
 * data (and the types it needs) and two static helpers.
 */
#ifndef U119_H
#define U119_H

#include "types.h"
#include "eekernel.h"                   /* WaitSema, DeleteSema, SyncDCache */
#include "sif.h"                        /* sceSifClientData, sceSifCallRpc, sceSifInitRpc, the loadfile API */
#include "sifdev.h"                     /* sceFsReset */
#include "libcdvd.h"                    /* sceCdCLOCK, SCECdINIT */
#include "cxxrt.h"                      /* __rtti_user, __rtti_si, type_info */
#include <stddef.h>

#include "engine/media/G002_mpeg_frameq.h"      /* intr_suspend, create_mutex_sema, yield_low_priority,
                                           signal_sema_helper, strlen, scf_clock_to_gmt / _local */
#include "sdk/libcdvd/G004_libcdvd.h"          /* sceCdInit, sceCdMmode */
#include "runtime/newlib/G007_stdio.h"            /* strcasecmp, strupr, strncmp, str* / mem*,
                                           sceSifBindRpc, sceSifInitIopHeap */

/* own translation units; T528 and T520 are C headers: included before the
 * definitions, they give those definitions C linkage */
#include "engine/pdi/T528_sif_call_rpc.h"
#include "engine/pdi/T527_pdi_client_rpc.h"
#include "engine/pdi/T519_pdicdvd.h"
#include "engine/pdi/T520_pdispu2.h"
#include "engine/pdi/T521_pdibgm.h"
#include "engine/pdi/T522_pdimpeg.h"
#include "engine/pdi/T523_pdipipe.h"
#include "engine/pdi/T524_iop_load_module_base.h"
#include "engine/io/T525_sector_cache.h"
#include "engine/io/T526_iso9660.h"

/* ======================================================================== */
/* Static helpers                                                           */
/* ======================================================================== */
/* intr_resume (the second half of intr_suspend): engine/media/G002_mpeg_frameq.h. */

/* A 64-byte aligned RPC answer buffer inside a 0x80-byte stack area (an
 * alloca or a local array, as the listing shows).  In int arithmetic: the
 * listing's mask is `addiu -0x40' (an unsigned ~0x3Fu gives lui / ori). */
#define RPC_ALIGN64(p) ((void *)(((int)(p) + 0x3F) & ~0x3F))

/* ======================================================================== */
/* T519 - pdicdvd.cxx                                                       */
/* ======================================================================== */
extern int g_003530E8;          /* 0x003530E8 gp-0x4908 (=-1) lock of the disc reads */
extern const u16 g_003530F0[4]; /* 0x003530F0 gp-0x4900 {0, 366, 731, 1096}: first day of each year of 4 */
extern u16 g_002FC4B0[2][12];   /* 0x002FC4B0 first day of each month: [0] normal, [1] leap year (.data) */

/* ======================================================================== */
/* T520 - pdispu2.cxx                                                       */
/* ======================================================================== */
/* Receive buffer of the 'SPUP' calls (0x40 bytes); doubleword 0 is the
 * voice-active mask that pdispu2_status returns. */
extern u64 g_002FCA00[8];       /* 0x002FCA00 */

/* ======================================================================== */
/* T521 - pdibgm.cxx                                                        */
/* ======================================================================== */
/* Stream status, copied from the 'PBGM' server (fno 6) by pdibgm_update;
 * pdibgm_get_status reads it. */
struct BgmStatus {
    s64 time;                   /* 0x00 stream position in microseconds, -1 at start */
    int pos;                    /* 0x08 */
    int unk_0C;                 /* 0x0C */
    int id;                     /* 0x10 changes with the song */
    int ended;                  /* 0x14 the song finished; cleared at start */
    u8  unk_18[0x28];           /* 0x18 */
};
static_assert(sizeof(BgmStatus) == 0x40, "BgmStatus");

extern BgmStatus g_002FCE00;    /* 0x002FCE00 */

/* ======================================================================== */
/* T525 - sector cache                                                      */
/* ======================================================================== */
struct SectorCacheEntry {
    u32 stamp;                  /* 0x0 clock of the last use */
    u32 lsn;                    /* 0x4 0xFFFFFFFF after a failed read */
    u8 *data;                   /* 0x8 0x800 bytes */
};
static_assert(sizeof(SectorCacheEntry) == 0xC, "SectorCacheEntry");

struct SectorCache {
    int               capacity; /* 0x00 */
    int               used;     /* 0x04 */
    u32               clock;    /* 0x08 */
    SectorCacheEntry *entries;  /* 0x0C sorted by lsn */
    u8               *data;     /* 0x10 64-byte aligned, capacity x 0x800 */
};
static_assert(sizeof(SectorCache) == 0x14, "SectorCache");

#define SECTOR_SIZE 0x800

/* T525's variables (.data, zeros).  egcs emits a C++ file's uninitialised
 * globals in the order of their first declaration: these externs (and
 * g_002FE280's below) give T525 the original's order. */
extern SectorCache g_002FDA40;  /* 0x002FDA40 */
extern u8 g_002FDA80[SECTOR_SIZE]; /* 0x002FDA80 unreferenced sector buffer */

/* ======================================================================== */
/* T526 - ISO 9660                                                          */
/* ======================================================================== */
/* (g_004A75E0 and g_00353298 are T526's file statics: .bss / .sbss) */
extern const char g_003459E0[4][16]; /* 0x003459E0 the PVD date labels (unreferenced) */
extern u8           g_002FE280[SECTOR_SIZE]; /* 0x002FE280 directory sector being searched (T525's .data) */

#endif /* U119_H */
