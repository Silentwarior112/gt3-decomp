/* deci2_libgraph_internal.h - the last stretch of the no-FDE region G007 of the GT3 US main
 * executable (0x0028D7C0..0x0028E760, the very end of .text).
 *
 * Contents (Sony / GCC runtime objects, all the same machine code as the
 * bootstrap's except __fixdfdi, which the bootstrap does not link):
 *   libkernl deci2.o (rest)  0x0028D7C0..0x0028D958   deci2_part2.c
 *                            (sceDeci2Open is in u141; bootstrap
 *                            sdk/libkernl/deci2_tty.c)
 *   libgcc2.c _fixdfdi.o     0x0028D958  __fixdfdi     libgcc2_part3.c
 *   libgcc2.c _fixunsdfdi.o  0x0028D9B8  __fixunsdfdi
 *   libgcc2.c _udivdi3.o     0x0028DAA8  __udivdi3
 *   libgcc2.c _umoddi3.o     0x0028E078  __umoddi3
 *   libgraph                 0x0028E5C0  sceGsGetGParam, libgraph.c
 *                            0x0028E5D0  sceGsResetGraph
 *                            (.data 0x002FC200, linked after u116's driver)
 *   VU0 microcode (data only, the original's .rdata output section)
 *                            0x00349600  g_gsVu0Microcode
 *                                        vu0_microcode.c
 *
 * runtime/newlib/G007_stdio.h (the canonical header of G007, core addresses) declares
 * the functions of this unit that other units call (sceDeci2ReqSend, Poll,
 * ExRecv, ExSend, deci2_write, __fixdfdi .. __umoddi3, sceGsResetGraph) and
 * the libkernl stubs it calls (Deci2Call, RemoveIntcHandler, GsPutIMR,
 * DisableIntc).  runtime/libgcc/G005_dp_bit.h declares the dp-bit routines and
 * __floatdidf; it brings sdk/libcdvd/G004_libcdvd.h, which defines the libgcc2.c
 * types (SItype .. UDWtype, struct DWstruct, DWunion) for the whole runtime.
 * The Sony SDK headers declare the rest with the BOOTSTRAP's addresses; the
 * core addresses of what is declared only there:
 *   eekernel.h  sceDeci2Close 0x0028D7C0, sceDeci2ExReqSend 0x0028D8B0,
 *               sceDeci2ExLock 0x0028D8E0, sceDeci2ExUnLock 0x0028D908 (no
 *               callers), SetGsCrt 0x00285860 (syscall 0x02, tail-called by
 *               sceGsResetGraph)
 *   libgraph.h  sceGsGetGParam 0x0028E5C0 (only caller: sceGsResetGraph)
 */
#ifndef U142_H
#define U142_H

#include "types.h"
#include "eekernel.h"           /* SetGsCrt, INTC_VBLANK_S, the uncalled deci2
                                   stubs' prototypes */
#include "libgraph.h"           /* sceGsGParam, sceGsGetGParam, GS_CSR,
                                   GS_CSR_RESET / FLUSH, SCE_GS_* modes */
#include "runtime/newlib/G007_stdio.h"    /* own TU */
#include "runtime/libgcc/G005_dp_bit.h"   /* dpadd .. dptoul, __floatdidf; libgcc2.c
                                   types (sdk/libcdvd/G004_libcdvd.h) */

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================= */
/*  deci2.o                                                                */
/* ======================================================================= */

/* Deci2Call function numbers (libkernl_part2_internal.h has DECI2_CALL_OPEN = 1).  The "Ex"
 * calls, made from inside a DECI2 event handler, pass the number negated.
 * Not in ps2sdk (kernel.h declares only `s32 Deci2Call(s32, u32 *)`): the
 * names are descriptive, after the sceDeci2* function each number serves. */
#define DECI2_CALL_CLOSE        2
#define DECI2_CALL_REQSEND      3
#define DECI2_CALL_POLL         4
#define DECI2_CALL_EXRECV       5       /* passed as -5 */
#define DECI2_CALL_EXSEND       6       /* passed as -6 */
#define DECI2_CALL_EXREQSEND    7       /* passed as -7 */
#define DECI2_CALL_EXLOCK       8       /* passed as -8 */
#define DECI2_CALL_EXUNLOCK     9       /* passed as -9 */
#define DECI2_CALL_KPUTS        0x10    /* deci2_write (the bootstrap's kPuts) */

/* ======================================================================= */
/*  libgcc2.c division objects                                             */
/* ======================================================================= */

/* The private `static const UQItype __clz_tab[256]' (bit length of 0..255)
 * of _udivdi3.o and of _umoddi3.o: each libgcc2 division object carries its
 * own copy (u131's _divdi3.o: divdi3_clz_tab 0x00348098, u133's _moddi3.o:
 * __clz_tab 0x00348198).  Here file statics of libgcc2_part3.c (a C
 * `static' after an `extern' of the same name does not compile):
 * udivdi3_clz_tab 0x003493A0 and umoddi3_clz_tab 0x003494A0 (.rodata). */

/* ======================================================================= */
/*  libgraph                                                               */
/* ======================================================================= */

/* libgraph's .data, file statics of libgraph.c (as the
 * bootstrap's; a C `static' after an `extern' of the same name does not
 * compile): s_libgraph_version 0x002FC200 ("PsIIlibgraph2221", no NUL) and
 * the single sceGsGParam instance s_gs_gparam 0x002FC210 (bootstrap
 * 0x01039500, initial {1, 2, 1, 3, 0, 0}). */

/* GS CSR / IMR fields (ps2sdk libgs.h GS_CSR.gs_rev_number = bits 23:16;
 * GS_IMR signal .. exvs_mask = bits 8..14). */
#define GS_CSR_REV_SHIFT        16
#define GS_CSR_REV_MASK         0xFF
#define GS_IMR_MASK_ALL         0xFF00  /* every GS interrupt masked (bits 8..15) */

#ifdef __cplusplus
}
#endif

#endif /* U142_H */
