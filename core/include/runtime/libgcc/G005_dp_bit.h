/* G005_dp_bit.h - the no-FDE stretch G005 (unit u133, C): GCC 2.9x-ee libgcc arithmetic
 * and the dp-bit soft-double routines.
 *
 * libgcc.a members, in link order (same objects as the bootstrap's
 * runtime/libgcc_math.c and runtime/libgcc2.c, a few more of them):
 *   _fixsfdi.o 0x00278630, _fixunssfdi.o 0x00278670, _floatdidf.o 0x00278768,
 *   _floatdisf.o 0x00278800, _moddi3.o 0x002788E0, _muldi3.o 0x00278F48,
 *   _pure.o 0x00278FA8, dp-bit.o 0x00278FB0 (config/fp-bit.c for double).
 * fp-bit.o (float) follows in G006.
 *
 * The EE FPU is single precision only: the compiler turns every double
 * operation into one of these calls, under the US Software GOFAST libcall
 * names of this toolchain (dpadd ...; the standard libgcc name in brackets).
 * Doubles travel as one 64-bit value in a GPR ($a0 / $a1, result $v0).
 * Negation compiles to dpsub(0.0, x).  Also declared (bootstrap addresses) in
 * include/cxxrt.h.
 */
#ifndef GAME_G005_DP_BIT_H
#define GAME_G005_DP_BIT_H

#include "game/game.h"
#include "sdk/libcdvd/G004_libcdvd.h"                  /* libgcc2.c types: SItype, DItype, DWtype, SFtype, DFtype */

#ifdef __cplusplus
extern "C" {
#endif

/* fp-bit.c's classification of an unpacked number (dp-bit and fp-bit). */
typedef enum {
    CLASS_SNAN,                                 /* 0 */
    CLASS_QNAN,                                 /* 1 */
    CLASS_ZERO,                                 /* 2 */
    CLASS_NUMBER,                               /* 3 */
    CLASS_INFINITY                              /* 4 */
} fp_class_type;

/* ---- libgcc2.c ---------------------------------------------------------- */
DWtype __fixsfdi(SFtype a);                     /* 0x00278630 */
DFtype __floatdidf(DWtype u);                   /* 0x00278768 */
SFtype __floatdisf(DWtype u);                   /* 0x00278800 */
DWtype __moddi3(DWtype u, DWtype v);            /* 0x002788E0 */
DItype __muldi3(DItype u, DItype v);            /* 0x00278F48 (no dmult on the R5900) */
/* `j __terminate`: fills the vtable slots of pure virtual methods. */
void   __pure_virtual(void);                    /* 0x00278FA8 */

/* ---- dp-bit.o ------------------------------------------------------------ */
DFtype dpadd(DFtype arg_a, DFtype arg_b);       /* 0x002793C0 [__adddf3] */
DFtype dpsub(DFtype arg_a, DFtype arg_b);       /* 0x00279418 [__subdf3] */
DFtype dpmul(DFtype arg_a, DFtype arg_b);       /* 0x00279480 [__muldf3] */
DFtype dpdiv(DFtype arg_a, DFtype arg_b);       /* 0x00279728 [__divdf3] */
int    dpcmp(DFtype arg_a, DFtype arg_b);       /* 0x002799A8 [__cmpdf2] <0 / 0 / >0, NaN -> 1 */
DFtype litodp(SItype arg_a);                    /* 0x002799F8 [__floatsidf] */
SItype dptoli(DFtype arg_a);                    /* 0x00279AB0 [__fixdfsi] */
USItype dptoul(DFtype arg_a);                   /* 0x00279B48 [__fixunsdfsi] */
/* Packs an unpacked double; called by fptodp (G006). */
DFtype __make_dp(fp_class_type fp_class, unsigned int sign, int exp,
                 UDItype frac);                 /* 0x00279C20 */
SFtype dptofp(DFtype arg_a);                    /* 0x00279C50 [__truncdfsf2] */

#ifdef __cplusplus
}
#endif

#endif /* GAME_G005_DP_BIT_H */
