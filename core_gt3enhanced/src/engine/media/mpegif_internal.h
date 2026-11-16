/* mpegif_internal.h - unit u120 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x0025E548..0x0025F390, second pass (on the canonical headers).
 *
 * Translation units:
 *   T529  mpegif - the MPEG (PSS) movie streamer: a feeder thread that reads
 *         the movie from the disc (pdimpeg.cxx IOP stream, T522) or from a
 *         preloaded copy in memory, and pushes it 4 KB at a time into the
 *         G002 MPEG player (u121, 0x0025F390..), whose own thread decodes
 *         frames.  Canonical header engine/media/T529_mpegif.h (mpegif, MpegifParam,
 *         the five .sdata words shared with the player).
 *
 * egcs ABI reminder (ref/CONTEXT.md): the vptr is a data member placed after
 * the data members of the first polymorphic class of a hierarchy.  Offsets in
 * the comments are the original (egcs) ones; a modern g++ build puts the vptr
 * at +0x00, which moves every member of mpegif ("vptr +0x150").
 *
 * This header keeps what only T529 uses: its private data.
 */
#ifndef U120_H
#define U120_H

#include "types.h"
#include "eekernel.h"                   /* ThreadParam, SemaParam */
#include "cxxrt.h"                      /* __rtti_user */
#include <stddef.h>

#include "engine/media/T529_mpegif.h"           /* own translation unit */
#include "game/app/T006_file_resolve_retry.h"
#include "engine/pdi/T521_pdibgm.h"           /* pdibgm_set_volume */
#include "engine/pdi/T522_pdimpeg.h"          /* pdimpeg_open / _feed / _close */
#include "engine/media/G002_mpeg_frameq.h"      /* mpeg_player_*, mpeg_vibuf_*, mpeg_pes_parse, mpeg_dma_alloc / _free */
/* _gp with a size, declared before G000's unsized declaration (as app_internal.h
 * does): egcs then counts it as small data (-G8) and loads &_gp with one
 * `la' kept in source order, as retail's startThread has it.  Not in G000
 * itself: a sized declaration costs u121 and u123 (ref/matching/u120/
 * pending.md). */
extern "C" char _gp[4];                 /* 0x003579F0 */
#include "runtime/startup/G000_collect2_ctors.h"   /* _gp */
#include "runtime/newlib/G007_stdio.h"            /* memset, memcpy, strcmp, strncmp, strncpy, threads, semaphores */
#include "engine/pdistd/T532_heap.h"             /* free, memalign */

/* ======================================================================== */
/* Private data of T529                                                     */
/* ======================================================================== */
extern "C" {
extern int g_frames_stopped;    /* 0x0035311C gp-0x48D4 1: nextFrame() hands out nothing (until feed()) */
extern int g_reading_stream;    /* 0x00353120 gp-0x48D0 1 while the feeder reads the IOP stream (only written) */

/* .rodata SemaParams (defined in T529_mpegif.cpp, as the .sdata words above
 * and T529_mpegif.h's g_mpeg_start_sema .. g_mpeg_wake_thread). */
extern const struct SemaParam g_mutex_sema_param;   /* 0x00345A20 {0, max 1, init 1} */
extern const struct SemaParam g_signal_sema_param;  /* 0x00345A38 {0, max 1, init 0} */
}

#endif /* U120_H */
