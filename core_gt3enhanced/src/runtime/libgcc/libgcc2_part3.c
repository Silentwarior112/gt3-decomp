/* libgcc2_part3.c - GCC 2.9x-ee libgcc2.c members at the end of .text,
 * 0x0028D958..0x0028E5C0, in libgcc.a archive member order:
 *   _fixdfdi.o    0x0028D958  __fixdfdi       libgcc2.c L_fixdfdi
 *   _fixunsdfdi.o 0x0028D9B8  __fixunsdfdi    libgcc2.c L_fixunsdfdi
 *   _udivdi3.o    0x0028DAA8  __udivdi3       libgcc2.c L_udivdi3
 *   _umoddi3.o    0x0028E078  __umoddi3       libgcc2.c L_umoddi3
 * (the rest of libgcc: __divdi3 in G004 (u131), __moddi3, __floatdidf and
 * dp-bit in G005 (u133), fp-bit in G006; this group sits after libkernl's
 * deci2.o, probably (a guess) pulled in by a second archive pass of the
 * link).  Types: sdk/libcdvd/G004_libcdvd.h's libgcc2.c types (SItype .. UDItype,
 * DWunion); dp-bit and __floatdidf: runtime/libgcc/G005_dp_bit.h.
 *
 * __fixunsdfdi, __udivdi3 and __umoddi3 are the same machine code as the
 * bootstrap's runtime/libgcc_math.c; __fixdfdi is core-only.
 *
 * The source is this egcs's libgcc2.c / longlong.h, as u131's _divdi3.o and
 * u133's _moddi3.o have them (SI_TYPE_SIZE words although the R5900 has
 * 64-bit GPRs; `long' is 64 bits).  Each division object inlines its own
 * copy of the static __udivmoddi4 and carries its own static __clz_tab
 * (_udivdi3.o: udivdi3_clz_tab 0x003493A0, _umoddi3.o: umoddi3_clz_tab
 * 0x003494A0, the last .rodata of the link: the VU0 microcode after them,
 * 0x00349600, is the .rdata output section, vu0_microcode.c):
 * __udivmoddi4 is defined once per object below, each copy reading its
 * object's table.  udiv_qrnnd is longlong.h's generic
 * __udiv_qrnnd_c (two 16-bit digit steps with divu, so
 * UDIV_NEEDS_NORMALIZATION = 1); umul_ppmm is the MIPS `multu'.  Every divu
 * is paired with gcc's -mcheck-zero-division guard (`beql divisor,$0 ->
 * break 7').
 *
 * Soft float: the EE FPU is single precision only, so every double operation
 * is a dp-bit (GOFAST) libcall the compiler emits itself from the plain C:
 * `a < 0' dpcmp, `a / 2^32' dpmul by 2^-32 (fold's exact reciprocal),
 * (USItype) dptoul, (DFtype) of an unsigned DI the halve-convert-double
 * expansion around __floatdidf, `-a' dpsub (0.0, a) (expand_unop: "The US
 * Software GOFAST library needs this").  Doubles travel as 64-bit values in
 * GPRs ($a0/$a1, result in $v0).
 */
#include "runtime/newlib/G007_stdio.h"
#include "sdk/libkernl/deci2_libgraph_internal.h"

/* ---- libgcc2.c / longlong.h of this egcs (SI_TYPE_SIZE era) --------------- */

#define BITS_PER_UNIT   8
#define SI_TYPE_SIZE    (sizeof (SItype) * BITS_PER_UNIT)   /* unsigned long */

/* longlong.h.  `long' is 64 bits in this compiler, so __ll_B (1L << 16)
 * makes `__r1 * __ll_B | ...' a 64-bit expression: the binary's
 * `dsll32; dsll 16; dsra32' where an SImode shift would be `sll 16'.
 * (Under GCC 14 long is 32 bits; the values are the same mod 2^32.) */
#define __BITS4           (SI_TYPE_SIZE / 4)
#define __ll_B            (1L << (SI_TYPE_SIZE / 2))
#define __ll_lowpart(t)   ((USItype) (t) % __ll_B)
#define __ll_highpart(t)  ((USItype) (t) / __ll_B)

#define sub_ddmmss(sh, sl, ah, al, bh, bl)                                  \
    do {                                                                    \
        USItype __x;                                                        \
        __x = (al) - (bl);                                                  \
        (sh) = (ah) - (bh) - (__x > (al));                                  \
        (sl) = __x;                                                         \
    } while (0)

/* MIPS umul_ppmm (longlong.h, __GNUC_MINOR__ >= 7): `multu' with the
 * results taken from LO / HI ("=l" / "=h"), so the compiler emits the
 * mflo / mfhi itself.  This is the original's asm.  GCC 14 has no "h"
 * constraint: there the asm reads HI / LO itself. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
#define umul_ppmm(w1, w0, u, v)                                             \
    __asm__ ("multu %2,%3"                                                  \
             : "=l" (w0),                                                   \
               "=h" (w1)                                                    \
             : "d" ((USItype)(u)),                                          \
               "d" ((USItype)(v)))
#else
#define umul_ppmm(w1, w0, u, v)                                             \
    __asm__ ("multu %2,%3\n\tmflo %0\n\tmfhi %1"                            \
             : "=r" (w0),                                                   \
               "=r" (w1)                                                    \
             : "r" ((USItype)(u)),                                          \
               "r" ((USItype)(v))                                           \
             : "hi", "lo")
#endif

/* __udiv_qrnnd_c (MIPS has no udiv_qrnnd: UDIV_NEEDS_NORMALIZATION). */
#define udiv_qrnnd(q, r, n1, n0, d)                                         \
    do {                                                                    \
        USItype __d1, __d0, __q1, __q0;                                     \
        USItype __r1, __r0, __m;                                            \
        __d1 = __ll_highpart (d);                                           \
        __d0 = __ll_lowpart (d);                                            \
                                                                            \
        __r1 = (n1) % __d1;                                                 \
        __q1 = (n1) / __d1;                                                 \
        __m = (USItype) __q1 * __d0;                                        \
        __r1 = __r1 * __ll_B | __ll_highpart (n0);                          \
        if (__r1 < __m)                                                     \
          {                                                                 \
            __q1--, __r1 += (d);                                            \
            if (__r1 >= (d)) /* i.e. we didn't get carry when adding to __r1 */ \
              if (__r1 < __m)                                               \
                __q1--, __r1 += (d);                                        \
          }                                                                 \
        __r1 -= __m;                                                        \
                                                                            \
        __r0 = __r1 % __d1;                                                 \
        __q0 = __r1 / __d1;                                                 \
        __m = (USItype) __q0 * __d0;                                        \
        __r0 = __r0 * __ll_B | __ll_lowpart (n0);                           \
        if (__r0 < __m)                                                     \
          {                                                                 \
            __q0--, __r0 += (d);                                            \
            if (__r0 >= (d))                                                \
              if (__r0 < __m)                                               \
                __q0--, __r0 += (d);                                        \
          }                                                                 \
        __r0 -= __m;                                                        \
                                                                            \
        (q) = (USItype) __q1 * __ll_B | __q0;                               \
        (r) = __r0;                                                         \
    } while (0)

/* Generic count_leading_zeros over the bit-length table __clz_tab (each
 * object's own copy, see __clz_tab below).  The dead `for' branch
 * (SI_TYPE_SIZE > 32) still leaves its loop notes: the alignment nop after
 * each copy's `b'. */
#define count_leading_zeros(count, x)                                       \
    do {                                                                    \
        USItype __xr = (x);                                                 \
        USItype __a;                                                        \
                                                                            \
        if (SI_TYPE_SIZE <= 32)                                             \
          {                                                                 \
            __a = __xr < ((USItype)1<<2*__BITS4)                            \
              ? (__xr < ((USItype)1<<__BITS4) ? 0 : __BITS4)                \
              : (__xr < ((USItype)1<<3*__BITS4) ?  2*__BITS4 : 3*__BITS4);  \
          }                                                                 \
        else                                                                \
          {                                                                 \
            for (__a = SI_TYPE_SIZE - 8; __a > 0; __a -= 8)                 \
              if (((__xr >> __a) & 0xff) != 0)                              \
                break;                                                      \
          }                                                                 \
                                                                            \
        (count) = SI_TYPE_SIZE - (__clz_tab[__xr >> __a] + __a);            \
    } while (0)

/* libgcc2.c __udivmoddi4 (UDIV_NEEDS_NORMALIZATION variant), static inline
 * in each division object.  UDIVMODDI4 is its definition under a name, so
 * that each object's copy reads that object's own __clz_tab (libgcc2.c
 * compiles it once per L_* object).  The quotient-only __udivdi3 passes
 * rp = 0 (the remainder code folds away); __umoddi3 passes &w (its stack
 * slot), whose `rp != 0' tests fold to true. */
#define UDIVMODDI4(name)                                                    \
static inline UDItype                                                       \
name (UDItype n, UDItype d, UDItype *rp)                                    \
{                                                                           \
  DWunion ww;                                                               \
  DWunion nn, dd;                                                           \
  DWunion rr;                                                               \
  USItype d0, d1, n0, n1, n2;                                               \
  USItype q0, q1;                                                           \
  USItype b, bm;                                                            \
                                                                            \
  nn.ll = n;                                                                \
  dd.ll = d;                                                                \
                                                                            \
  d0 = dd.s.low;                                                            \
  d1 = dd.s.high;                                                           \
  n0 = nn.s.low;                                                            \
  n1 = nn.s.high;                                                           \
                                                                            \
  if (d1 == 0)                                                              \
    {                                                                       \
      if (d0 > n1)                                                          \
        {                                                                   \
          /* 0q = nn / 0D */                                                \
                                                                            \
          count_leading_zeros (bm, d0);                                     \
                                                                            \
          if (bm != 0)                                                      \
            {                                                               \
              /* Normalize, i.e. make the most significant bit of the       \
                 denominator set.  */                                       \
                                                                            \
              d0 = d0 << bm;                                                \
              n1 = (n1 << bm) | (n0 >> (SI_TYPE_SIZE - bm));                \
              n0 = n0 << bm;                                                \
            }                                                               \
                                                                            \
          udiv_qrnnd (q0, n0, n1, n0, d0);                                  \
          q1 = 0;                                                           \
                                                                            \
          /* Remainder in n0 >> bm.  */                                     \
        }                                                                   \
      else                                                                  \
        {                                                                   \
          /* qq = NN / 0d */                                                \
                                                                            \
          if (d0 == 0)                                                      \
            d0 = DIVIDE_BY_ZERO (d0); /* Divide intentionally by zero.  */  \
                                                                            \
          count_leading_zeros (bm, d0);                                     \
                                                                            \
          if (bm == 0)                                                      \
            {                                                               \
              /* From (n1 >= d0) /\ (the most significant bit of d0 is     \
                 set), conclude (the most significant bit of n1 is set)     \
                 /\ (the leading quotient digit q1 = 1).                    \
                                                                            \
                 This special case is necessary, not an optimization.       \
                 (Shifts counts of SI_TYPE_SIZE are undefined.)  */         \
                                                                            \
              n1 -= d0;                                                     \
              q1 = 1;                                                       \
            }                                                               \
          else                                                              \
            {                                                               \
              /* Normalize.  */                                             \
                                                                            \
              b = SI_TYPE_SIZE - bm;                                        \
                                                                            \
              d0 = d0 << bm;                                                \
              n2 = n1 >> b;                                                 \
              n1 = (n1 << bm) | (n0 >> b);                                  \
              n0 = n0 << bm;                                                \
                                                                            \
              udiv_qrnnd (q1, n1, n2, n1, d0);                              \
            }                                                               \
                                                                            \
          /* n1 != d0...  */                                                \
                                                                            \
          udiv_qrnnd (q0, n0, n1, n0, d0);                                  \
                                                                            \
          /* Remainder in n0 >> bm.  */                                     \
        }                                                                   \
                                                                            \
      if (rp != 0)                                                          \
        {                                                                   \
          rr.s.low = n0 >> bm;                                              \
          rr.s.high = 0;                                                    \
          *rp = rr.ll;                                                      \
        }                                                                   \
    }                                                                       \
  else                                                                      \
    {                                                                       \
      if (d1 > n1)                                                          \
        {                                                                   \
          /* 00 = nn / DD */                                                \
                                                                            \
          q0 = 0;                                                           \
          q1 = 0;                                                           \
                                                                            \
          /* Remainder in n1n0.  */                                         \
          if (rp != 0)                                                      \
            {                                                               \
              rr.s.low = n0;                                                \
              rr.s.high = n1;                                               \
              *rp = rr.ll;                                                  \
            }                                                               \
        }                                                                   \
      else                                                                  \
        {                                                                   \
          /* 0q = NN / dd */                                                \
                                                                            \
          count_leading_zeros (bm, d1);                                     \
          if (bm == 0)                                                      \
            {                                                               \
              /* From (n1 >= d1) /\ (the most significant bit of d1 is      \
                 set), conclude (the most significant bit of n1 is set)     \
                 /\ (the quotient digit q0 = 0 or 1).                       \
                                                                            \
                 This special case is necessary, not an optimization.  */   \
                                                                            \
              /* The condition on the next line takes advantage of that     \
                 n1 >= d1 (true due to program flow).  */                   \
              if (n1 > d1 || n0 >= d0)                                      \
                {                                                           \
                  q0 = 1;                                                   \
                  sub_ddmmss (n1, n0, n1, n0, d1, d0);                      \
                }                                                           \
              else                                                          \
                q0 = 0;                                                     \
                                                                            \
              q1 = 0;                                                       \
                                                                            \
              if (rp != 0)                                                  \
                {                                                           \
                  rr.s.low = n0;                                            \
                  rr.s.high = n1;                                           \
                  *rp = rr.ll;                                              \
                }                                                           \
            }                                                               \
          else                                                              \
            {                                                               \
              USItype m1, m0;                                               \
              /* Normalize.  */                                             \
                                                                            \
              b = SI_TYPE_SIZE - bm;                                        \
                                                                            \
              d1 = (d1 << bm) | (d0 >> b);                                  \
              d0 = d0 << bm;                                                \
              n2 = n1 >> b;                                                 \
              n1 = (n1 << bm) | (n0 >> b);                                  \
              n0 = n0 << bm;                                                \
                                                                            \
              udiv_qrnnd (q0, n1, n2, n1, d1);                              \
              umul_ppmm (m1, m0, q0, d0);                                   \
                                                                            \
              if (m1 > n1 || (m1 == n1 && m0 > n0))                         \
                {                                                           \
                  q0--;                                                     \
                  sub_ddmmss (m1, m0, m1, m0, d1, d0);                      \
                }                                                           \
                                                                            \
              q1 = 0;                                                       \
                                                                            \
              /* Remainder in (n1n0 - m1m0) >> bm.  */                      \
              if (rp != 0)                                                  \
                {                                                           \
                  sub_ddmmss (n1, n0, n1, n0, m1, m0);                      \
                  rr.s.low = (n1 << b) | (n0 >> bm);                        \
                  rr.s.high = n1 >> bm;                                     \
                  *rp = rr.ll;                                              \
                }                                                           \
            }                                                               \
        }                                                                   \
    }                                                                       \
                                                                            \
  ww.s.low = q0;                                                            \
  ww.s.high = q1;                                                           \
  return ww.ll;                                                             \
}

/* `d0 = 1 / d0' with d0 == 0: plain under egcs (CSE divides by another
 * register known to be 0, as retail); GCC 14 may fold a known division by
 * zero away, so its build divides through a volatile and keeps the `divu'
 * (and its trap). */
#ifdef BOOT_OLD_GCC
#define DIVIDE_BY_ZERO(z)   (1 / (z))
#else
#define DIVIDE_BY_ZERO(z)   ({ volatile USItype __zero = (z); 1 / __zero; })
#endif

/* ======================================================================= */
/*  Functions                                                              */
/* ======================================================================= */

/* ------------------------------------------------------------------------ */
/* L_fixdfdi                                                                */
/* ------------------------------------------------------------------------ */

// FUNC 0x0028D958 __fixdfdi
//   libgcc2.c L_fixdfdi: double -> signed 64-bit.  `a < 0' is a dpcmp
//   against 0.0 and `-a' is dpsub (0.0, a) (GOFAST has no negate libcall).
//   Only caller: _strtod_r (0x00282BF0).
DItype
__fixdfdi (DFtype a)
{
  /* implicit: dpcmp, dpsub */
  if (a < 0)
    return - __fixunsdfdi (-a);
  return __fixunsdfdi (a);
}

/* ------------------------------------------------------------------------ */
/* L_fixunsdfdi                                                             */
/* ------------------------------------------------------------------------ */

#define WORD_SIZE (sizeof (SItype) * BITS_PER_UNIT)
#define HIGH_WORD_COEFF (((UDItype) 1) << WORD_SIZE)

// FUNC 0x0028D9B8 __fixunsdfdi
//   libgcc2.c double -> unsigned 64-bit.  Every double operation is a GOFAST
//   libcall the compiler emits itself: `a < 0' dpcmp, `a / HIGH_WORD_COEFF'
//   a dpmul by 2^-32 (0x3DF0000000000000, fold's exact reciprocal),
//   (USItype) dptoul, (DFtype) v of the unsigned DI the halve-convert-double
//   expansion around __floatdidf ((v & 1) | (v >> 1), then dpadd of the
//   result to itself; its sticky `v & 1' is always 0 here, v's low word
//   being 0), `-a' dpsub (0.0, a).
//   The GCC 14 build makes those calls explicitly: its own soft-float
//   (__fixunsdfsi ..) differs from dp-bit's dptoul for NaN (dp-bit: 0), so
//   plain C there would not be retail's behaviour for every input.
//   Callers: _print_double (0x00287418), __fixdfdi.
UDItype
__fixunsdfdi (DFtype a)
{
  DFtype b;
  UDItype v;
#ifdef BOOT_OLD_GCC
  /* implicit: dpcmp, dpmul, dptoul, __floatdidf, dpadd, dpsub */

  if (a < 0)
    return 0;

  /* Compute high word of result, as a flonum.  */
  b = (a / HIGH_WORD_COEFF);
  /* Convert that to fixed (but not to DItype!),
     and shift it into the high word.  */
  v = (USItype) b;
  v <<= WORD_SIZE;
  /* Remove high part from the DFtype, leaving the low part as flonum.  */
  a -= (DFtype)v;
  /* Convert that to fixed (but not to DItype!) and add it in.
     Sometimes A comes out negative.  This is significant, since
     A has more bits than a long int does.  */
  if (a < 0)
    v -= (USItype) (- a);
  else
    v += (USItype) a;
  return v;
#else
  /* The calls egcs emits for the code above, in its order. */
  DFtype v_flonum;                              /* (DFtype) v */

  if (dpcmp (a, 0.0) < 0)                       /* a < 0 */
    return 0;

  b = dpmul (a, 0x1p-32);                       /* a / HIGH_WORD_COEFF */
  v = (USItype) dptoul (b);
  v <<= WORD_SIZE;
  /* a -= (DFtype) v, the unsigned DI -> DF conversion expanded. */
  if ((DItype) v >= 0)
    v_flonum = __floatdidf (v);
  else
    {
      v_flonum = __floatdidf ((v & 1) | (v >> 1));
      v_flonum = dpadd (v_flonum, v_flonum);
    }
  a = dpsub (a, v_flonum);
  if (dpcmp (a, 0.0) < 0)                       /* a < 0 */
    v -= (USItype) dptoul (dpsub (0.0, a));     /* (USItype) (- a) */
  else
    v += (USItype) dptoul (a);
  return v;
#endif
}

/* ------------------------------------------------------------------------ */
/* L_udivdi3                                                                */
/* ------------------------------------------------------------------------ */

/* libgcc2.c L_udivmoddi4's `static const UQItype __clz_tab[]' (the bit
 * length of 0..255), _udivdi3.o's copy: the SDK's _udivdi3.o has it as a
 * local OBJECT `__clz_tab', its whole .rodata (0x100 bytes, 8-aligned), right
 * before the inlined __udivmoddi4 as libgcc2.c has it.  Written
 * udivdi3_clz_tab: u133's _moddi3.o copy is the declared __clz_tab, and one
 * name at two addresses would drop both from the link's names. */
static const UQItype udivdi3_clz_tab[] =
{                                                   /* 0x003493A0 (.rodata) */
  0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
};

/* _udivdi3.o's `static const UQItype __clz_tab[256]' (udivdi3_clz_tab). */
#define __clz_tab udivdi3_clz_tab
UDIVMODDI4 (__udivmoddi4_udivdi3)
#undef __clz_tab

// FUNC 0x0028DAA8 __udivdi3
//   Unsigned 64-bit divide (quotient only).  Allocates an unused 16-byte
//   frame.  Callers: _strtol_r (0x0027F078), _vfprintf_r (0x0027F4B8),
//   _malloc_trim_r (0x00281568), _strtoul_r (0x00283B30), sce_doprnt
//   (0x00287580).
//   Exact under the reference compiler 991111-01.  The default 991111b
//   gives no frame (`.frame' vars 0, 991111-01: vars 16, as u133's
//   __moddi3), so retail's `addiu sp,-0x10' in the first branch's delay
//   slot is missing and reorg fills two other slots differently (97.4%).
//   Passing the table to one shared inline instead of one __udivmoddi4
//   copy per object: 93.2%.
UDItype
__udivdi3 (UDItype n, UDItype d)
{
  return __udivmoddi4_udivdi3 (n, d, (UDItype *) 0);
}

/* ------------------------------------------------------------------------ */
/* L_umoddi3                                                                */
/* ------------------------------------------------------------------------ */

/* _umoddi3.o's copy of libgcc2.c's __clz_tab (the SDK's _umoddi3.o: local
 * OBJECT `__clz_tab', 0x100 bytes, 8-aligned, its whole .rodata), right
 * after _udivdi3.o's. */
static const UQItype umoddi3_clz_tab[] =
{                                                   /* 0x003494A0 (.rodata) */
  0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
};

/* _umoddi3.o's `static const UQItype __clz_tab[256]' (umoddi3_clz_tab). */
#define __clz_tab umoddi3_clz_tab
UDIVMODDI4 (__udivmoddi4_umoddi3)
#undef __clz_tab

// FUNC 0x0028E078 __umoddi3
//   Unsigned 64-bit remainder via __udivmoddi4 with rp = &w (sp+0).
//   Callers: _strtol_r (0x0027F078), _vfprintf_r (0x0027F4B8), _strtoul_r
//   (0x00283B30), sce_doprnt (0x00287580).
//   Exact under the reference compiler 991111-01.  The default 991111b
//   differs only in the frame: 0x10 instead of retail's 0x30 (vars 16 vs
//   48, as u133's __moddi3).
UDItype
__umoddi3 (UDItype u, UDItype v)
{
  UDItype w;

  (void) __udivmoddi4_umoddi3 (u, v, &w);

  return w;
}
