/* libgcc2_part1_internal.h - unit u131 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x00274AD0..0x002776C0, second pass (on the canonical headers).
 *
 * One more piece of the no-FDE stretch G004 (split: the rest of it is in the
 * neighbouring units).  The unit's range opens with the tail of Sony libscf
 * (0x00274AD0..0x00274F70, the RTC clock arithmetic): libscf is one object,
 * merged back into one file in u130's folder (src/sdk/libscf/libscf.c).
 * Then two libraries back to back:
 *   libm.c     newlib libm (fdlibm 5.x): cos, sin, sqrt (w_sqrt
 *                        wrapper), __ieee754_rem_pio2, __ieee754_sqrt,
 *                        __kernel_cos, __kernel_rem_pio2, __kernel_sin, fabs,
 *                        floor, isnan, matherr, scalbn, copysign
 *   libgcc2_part1.c  libgcc2.c: __do_global_dtors, __do_global_ctors,
 *                        __main (BOOT), __divdi3 (BOOT)
 *
 * The public functions of these files (cos, sin, sqrt, __ieee754_sqrt,
 * __do_global_*, __main, __divdi3) and the libgcc2.c types
 * (SItype .. DFtype, Wtype / DWtype, DWstruct, DWunion) are declared in the
 * canonical sdk/libcdvd/G004_libcdvd.h; this header keeps only what nothing outside
 * the unit uses.
 *
 * Soft double: the EE FPU is single precision only, so every double operation
 * in libm is a call into dp-bit (G005_dp_bit.h, GOFAST names: dpadd, dpsub,
 * dpmul, dpdiv, dpcmp, litodp, dptoli).  The libm source is written as
 * ordinary double arithmetic; the compiler emits those calls.  Two things of
 * the old compiler to keep in mind when reading the binary:
 *   - negation `-x` is a dpsub(0.0, x) call (GOFAST has no negate libcall,
 *     so egcs subtracts from zero; this gives +0.0 for x = +0.0);
 *   - every relational test is dpcmp(a, b) compared with 0 (dpcmp returns 1
 *     when either operand is a NaN).
 * Doubles travel as one 64-bit value in a GPR ($a0/$a1, result $v0).
 */
#ifndef U131_H
#define U131_H

#include "types.h"
#include "sdk/libcdvd/G004_libcdvd.h"  /* own TU: libm / libgcc2 API, libgcc2.c types */
#include "runtime/newlib/G007_stdio.h"    /* __errno */
#include "runtime/startup/G000_collect2_ctors.h"  /* __CTOR_LIST__ 0x0028F020, __DTOR_LIST__ 0x0028F1E0 */
#include <stddef.h>

/* ======================================================================== */
/* libm (fdlibm)                                                            */
/* ======================================================================== */

/* fdlibm's `-x'.  egcs expands it as dpsub(0.0, x) (+0.0 for x = +0.0) and
 * needs the NEGATE_EXPR for retail's code (__kernel_rem_pio2's `ih ? -fw'
 * arms); GCC 14 flips the sign bit, so its build subtracts as retail does. */
#ifdef BOOT_OLD_GCC
#define FD_NEG(x)   (-(x))
#else
#define FD_NEG(x)   (0.0 - (x))
#endif

/* fdlibm.h word access, little-endian: the low word first. */
typedef union {
    double value;
    struct {
        u32 lsw;   /* +0 */
        u32 msw;   /* +4 sign, exponent, top 20 mantissa bits */
    } parts;
} ieee_double_shape_type;

#define EXTRACT_WORDS(ix0, ix1, d)                \
    do {                                          \
        ieee_double_shape_type ew_u;              \
        ew_u.value = (d);                         \
        (ix0) = ew_u.parts.msw;                   \
        (ix1) = ew_u.parts.lsw;                   \
    } while (0)

#define GET_HIGH_WORD(i, d)                       \
    do {                                          \
        ieee_double_shape_type gh_u;              \
        gh_u.value = (d);                         \
        (i) = gh_u.parts.msw;                     \
    } while (0)

#define GET_LOW_WORD(i, d)                        \
    do {                                          \
        ieee_double_shape_type gl_u;              \
        gl_u.value = (d);                         \
        (i) = gl_u.parts.lsw;                     \
    } while (0)

#define INSERT_WORDS(d, ix0, ix1)                 \
    do {                                          \
        ieee_double_shape_type iw_u;              \
        iw_u.parts.msw = (ix0);                   \
        iw_u.parts.lsw = (ix1);                   \
        (d) = iw_u.value;                         \
    } while (0)

#define SET_HIGH_WORD(d, v)                       \
    do {                                          \
        ieee_double_shape_type sh_u;              \
        sh_u.value = (d);                         \
        sh_u.parts.msw = (v);                     \
        (d) = sh_u.value;                         \
    } while (0)

#define SET_LOW_WORD(d, v)                        \
    do {                                          \
        ieee_double_shape_type sl_u;              \
        sl_u.value = (d);                         \
        sl_u.parts.lsw = (v);                     \
        (d) = sl_u.value;                         \
    } while (0)

/* SVID error record (newlib math.h struct exception), 0x28 bytes: the
 * argument of matherr.  Not the C++ class `exception' of T543 (whose dossier
 * merged the two); local to this unit. */
struct exception {
    int    type;     /* +0x00 DOMAIN = 1 */
    char  *name;     /* +0x04 */
    double arg1;     /* +0x08 */
    double arg2;     /* +0x10 */
    double retval;   /* +0x18 */
    int    err;      /* +0x20 errno to set when != 0 */
};                   /* 0x28 (padded) */
_Static_assert(sizeof(struct exception) == 0x28, "struct exception");

#define DOMAIN 1     /* struct exception.type: argument domain error */
#define EDOM   33    /* newlib errno.h */

/* _LIB_VERSION values (newlib enum __fdlibm_version). */
#define _IEEE_  (-1)
#define _SVID_  0
#define _XOPEN_ 1
#define _POSIX_ 2

/* s_lib_ver.o's mode word, const (.rodata, = 1 _XOPEN_); defined in
 * libm.c after sqrt, its reader. */
extern const int _LIB_VERSION;            /* 0x00348060 */

/* The `static const' tables of e_rem_pio2.c and k_rem_pio2.c (.rodata),
 * defined in libm.c (with fdlibm's constants, in retail's order). */
extern const int    two_over_pi[66];      /* 0x00347CA8 e_rem_pio2: 2/pi in 24-bit chunks
                                             (0xA2F983, 0x6E4E44, ...) */
extern const int    npio2_hw[32];         /* 0x00347DB0 e_rem_pio2: high words of n*pi/2 */
extern const int    init_jk[4];           /* 0x00347F70 k_rem_pio2: {2, 3, 4, 6} */
extern const double PIo2[8];              /* 0x00347F80 k_rem_pio2: pi/2 in 24-bit pieces
                                             (0x3FF921FB40000000, 0x3E74442D00000000, ...) */

/* The libm functions only libm itself calls (cos, sin, sqrt and
 * __ieee754_sqrt are in G004_libcdvd.h). */
int    __ieee754_rem_pio2(double x, double *y);              /* 0x00275280 */
double __kernel_cos(double x, double y);                     /* 0x00275AA8 */
int    __kernel_rem_pio2(double *x, double *y, int e0, int nx, int prec,
                         const int *ipio2);                  /* 0x00275CF8 */
double __kernel_sin(double x, double y, int iy);             /* 0x00276860 */
double fabs(double x);                                       /* 0x00276A38 */
double floor(double x);                                      /* 0x00276A70 */
/* isnan is const for its callers: sqrt keeps _LIB_VERSION in $s2 across the
 * isnan call (no reload), which only a const declaration allows. */
int    isnan(double x) __attribute__((const));               /* 0x00276C48 */
int    matherr(struct exception *x);                         /* 0x00276C80 */
double scalbn(double x, int n);                              /* 0x00276CA8 */
double copysign(double x, double y);                         /* 0x00276E60 */

/* ======================================================================== */
/* libgcc2                                                                  */
/* ======================================================================== */

/* libgcc2's data are statics of libgcc2_part1.c, as in libgcc2.c:
 * __do_global_dtors' `static func_ptr *p = __DTOR_LIST__ + 1;' (.data
 * 0x00305EB8), __main's `static int initialized;' (.bss 0x004AD200) and
 * _divdi3.o's private copy of `static const UQItype __clz_tab[]'
 * (divdi3_clz_tab, .rodata 0x00348098; every division object has its own). */

#endif /* U131_H */
