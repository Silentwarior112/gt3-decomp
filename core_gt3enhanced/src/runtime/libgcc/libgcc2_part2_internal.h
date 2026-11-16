/* libgcc2_part2_internal.h - GCC 2.9x-ee libgcc arithmetic, the first part of the no-FDE stretch
 * G005 of the GT3 US main executable (0x00278630..0x00279CA8).
 *
 * Contents, in libgcc.a archive member order (the toolchain's own objects,
 * sdk/ee-gcc-2.9-991111b/.../libgcc.a: the same as the bootstrap's
 * runtime/libgcc_math.c and runtime/libgcc2.c, plus three libgcc2 members the
 * bootstrap does not link):
 *   _fixsfdi.o    0x00278630  __fixsfdi       libgcc2.c L_fixsfdi
 *   _fixunssfdi.o 0x00278670  __fixunssfdi    libgcc2.c L_fixunssfdi
 *   _floatdidf.o  0x00278768  __floatdidf     libgcc2.c L_floatdidf
 *   _floatdisf.o  0x00278800  __floatdisf     libgcc2.c L_floatdisf
 *   _moddi3.o     0x002788E0  __moddi3        libgcc2.c L_moddi3
 *   _muldi3.o     0x00278F48  __muldi3        libgcc2.c L_muldi3
 *   _pure.o       0x00278FA8  __pure_virtual  libgcc2.c L_pure
 *   dp-bit.o      0x00278FB0  config/fp-bit.c compiled for double
 * fp-bit.o (float, G006) follows in u135; __fixunsdfdi is in G007 (0x0028D9B8).
 *
 * Soft float: the EE FPU is single precision only; doubles travel as 64-bit
 * values in GPRs ($a0/$a1, result $v0).  The toolchain's libgcc uses the US
 * Software GOFAST libcall names (dpadd/dpsub/dpmul/dpdiv/dpcmp/litodp/dptoli/
 * dptoul/dptofp; see bootstrap runtime/libgcc_math.c for the evidence).
 * The compiler emits them itself (gofast.h's libcall names), so the libgcc2
 * conversions are plain C on doubles, as in libgcc2.c.
 *
 * The C is the original source: egcs-era libgcc2.c / longlong.h
 * (SI_TYPE_SIZE, `__ll_B (1L << 16)' with a 64-bit long) and gcc 2.9x
 * config/fp-bit.c (US_SOFTWARE_GOFAST, FLOAT_BIT_ORDER_MISMATCH,
 * NO_DENORMALS); all 22 functions are exact under the reference compiler
 * 991111-01 (ref/matching/u133/notes.md).
 *
 * The shared declarations are in the canonical headers:
 *   runtime/libgcc/G005_dp_bit.h   this translation unit's entry points used elsewhere
 *                        and fp_class_type; through it
 *   sdk/libcdvd/G004_libcdvd.h  the libgcc2.c types (SItype ... DFtype, Wtype / DWtype,
 *                        struct DWstruct, DWunion, W_TYPE_SIZE)
 *   runtime/libgcc/G006_fp_bit.h   __make_fp 0x0027AF70, fptodp 0x0027AFA0 (fp-bit.o)
 *   runtime/libgcc/T542_eh.h       __terminate 0x002776D0
 * This header keeps what only G005 uses: fp-bit's unpacked double, a note on
 * the unit's two data objects (file statics) and the prototypes of the two
 * entry points nothing outside the unit calls.
 */
#ifndef U133_H
#define U133_H

#include "types.h"
#include "runtime/libgcc/G005_dp_bit.h"           /* own TU; includes G004 (libgcc2.c types) */
#include "runtime/libgcc/G006_fp_bit.h"           /* __make_fp, fptodp */
#include "runtime/libgcc/T542_eh.h"               /* __terminate */

/* ======================================================================= */
/*  fp-bit.c common definitions                                            */
/* ======================================================================= */

/* The class predicates (fp-bit's isnan / isinf / iszero) are the inline
 * functions of dp_bit.c: as functions they return a store-flag, which
 * the binary tests with `xori; beqz` (a macro compare gives `li; beq`). */

/* Preserve the sticky-bit when shifting fractions to the right. */
#define LSHIFT(a) { a = (a & 1) | (a >> 1); }

/* F_D_BITOFF: bit offset between the MSB of a float and a double mantissa
   (double::FRAC_BITS + double::NGARDS - (float::FRAC_BITS - float::NGARDS)). */
#define F_D_BITOFF (52+8-(23+7))

/* ---- double parameters (dp-bit.o), fp-bit.c's own spellings ---- */
#define NGARDS_D        8L
#define GARDROUND_D     0x7f
#define GARDMASK_D      0xff
#define GARDMSB_D       0x80
#define EXPBITS_D       11
#define EXPBIAS_D       1023
#define FRACBITS_D      52
#define EXPMAX_D        (0x7ff)
#define QUIET_NAN_D     0x8000000000000LL                            /* 1 << 51 */
#define FRAC_NBITS_D    64
#define FRACHIGH_D      0x8000000000000000LL
#define NORMAL_EXPMIN_D (-(EXPBIAS_D)+1)                             /* -1022 */
#define IMPLICIT_1_D    (1LL<<(FRACBITS_D+NGARDS_D))                 /* 1 << 60 */
#define IMPLICIT_2_D    (1LL<<(FRACBITS_D+1+NGARDS_D))               /* 1 << 61 */

#define MAX_SI_INT      ((SItype) ((unsigned) (~0)>>1))
#define MAX_USI_INT     ((USItype) ~0)

/* Unpacked double (fp-bit fp_number_type, dp-bit flavour).  The member keeps
 * fp-bit's name `class' (this is C). */
typedef struct {
    fp_class_type class;        /* +0x00 */
    unsigned int  sign;         /* +0x04 */
    int           normal_exp;   /* +0x08 unbiased */
                                /* +0x0C padding */
    union {
        UDItype ll;             /* +0x10 1.52 mantissa << 8 (implicit bit 60) */
        USItype l[2];
    } fraction;
} fp_number_type_d;             /* 0x18 */
_Static_assert(sizeof(fp_number_type_d) == 0x18, "dp-bit fp_number_type size");

/* FLO_union_type.  dp-bit.c is built with FLOAT_BIT_ORDER_MISMATCH (MIPS
 * little-endian), so pack_d / unpack_d use the IEEE bitfield struct: pack_d's
 * three read-modify-write masks (fraction, exp, sign) are those bitfield
 * stores. */
typedef union {
    DFtype  value;
    UDItype value_raw;
    USItype words[2];
    struct {
        UDItype      fraction:FRACBITS_D __attribute__ ((packed));
        unsigned int exp:EXPBITS_D __attribute__ ((packed));
        unsigned int sign:1 __attribute__ ((packed));
    } bits;
} FLO_union_type_d;
_Static_assert(sizeof(FLO_union_type_d) == 8, "dp-bit FLO_union_type size");

/* ======================================================================= */
/*  Data of the unit (file statics in the original objects)                */
/* ======================================================================= */

/* Both are statics of their files now, as in the original objects (no
 * extern here: a C `static' after an `extern' is an error for GCC 14):
 *   libgcc2_part2.c  `static const UQItype __clz_tab[]' (_moddi3.o's copy,
 *                   0x00348198 .rodata; _divdi3.o's is 0x00348098, u131)
 *   dp_bit.c   nan_d()'s `static fp_number_type_d thenan;' (dp-bit.o's
 *                   `thenan.3', 0x004AD228 .bss, 0x18 bytes; fp-bit.o's
 *                   float twin is 0x004AD240, u135) */

/* ======================================================================= */
/*  Entry points nothing outside the unit calls                            */
/* ======================================================================= */

/* The used ones are in runtime/libgcc/G005_dp_bit.h.  dp-bit.o's statics: pack_d
 * 0x00278FB0, unpack_d 0x002790E0, _fpadd_parts_d 0x00279180,
 * __fpcmp_parts_d 0x00279890 (see dp_bit.c). */
DItype  __fixunssfdi(SFtype original_a);            /* 0x00278670 (called by __fixsfdi) */
DFtype  __negdf2(DFtype arg_a);                     /* 0x00279BE8 (no callers) */

#endif /* U133_H */
