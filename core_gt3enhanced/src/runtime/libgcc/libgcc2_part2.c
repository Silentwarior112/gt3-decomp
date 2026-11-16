/* libgcc2_part2.c - libgcc2.c members of the no-FDE stretch G005,
 * 0x00278630..0x00278FB0 of the GT3 US main executable:
 *
 *   _fixsfdi.o    0x00278630  __fixsfdi       float -> signed 64-bit
 *   _fixunssfdi.o 0x00278670  __fixunssfdi    float -> unsigned 64-bit
 *   _floatdidf.o  0x00278768  __floatdidf     signed 64-bit -> double
 *   _floatdisf.o  0x00278800  __floatdisf     signed 64-bit -> float
 *   _moddi3.o     0x002788E0  __moddi3        signed 64-bit remainder
 *   _muldi3.o     0x00278F48  __muldi3        64x64 -> 64 multiply
 *   _pure.o       0x00278FA8  __pure_virtual  abstract vtable slot filler
 *
 * __floatdidf, __moddi3, __muldi3 and __pure_virtual are the same machine
 * code as the bootstrap's (runtime/libgcc_math.c, runtime/libgcc2.c); the
 * three SF <-> DI conversions are not in the bootstrap.  The source is this
 * egcs's libgcc2.c / longlong.h (SI_TYPE_SIZE words although the R5900 has
 * 64-bit GPRs; `long' is 64 bits).  Every double operation is a dp-bit call
 * the compiler emits itself (GOFAST names, see libgcc2_part2_internal.h); float comparisons
 * and negation are EE FPU instructions (c.lt.s, neg.s).
 */
#include "runtime/libgcc/G005_dp_bit.h"           /* own TU; G004 libgcc2.c types */
#include "runtime/libgcc/libgcc2_part2_internal.h"

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

#define __umulsidi3(u, v)                                                   \
    ({DWunion __w;                                                          \
      umul_ppmm (__w.s.high, __w.s.low, u, v);                              \
      __w.ll; })

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

/* Generic count_leading_zeros over the bit-length table __clz_tab
 * (0x00348198, defined below).  The dead `for' branch (SI_TYPE_SIZE > 32) still leaves its
 * loop notes: the alignment nop after each copy's `b'. */
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

/* libgcc2.c: static inline in the division objects (L_divdi3, L_moddi3),
 * i.e. inlined into __moddi3 as the 32-bit-half negate
 * low = -low; high = -high - (low != 0). */
static inline DItype
__negdi2 (DItype u)
{
  DWunion w;
  DWunion uu;

  uu.ll = u;

  w.s.low = -uu.s.low;
  w.s.high = -uu.s.high - ((USItype) w.s.low > 0);

  return w.ll;
}

/* libgcc2.c L_udivmoddi4: the bit length of 0..255, read by
 * count_leading_zeros.  A file static of every division object (the SDK's
 * _moddi3.o: `r __clz_tab', its whole .rodata, 0x100 bytes, 8-aligned);
 * this is _moddi3.o's copy.  _divdi3.o's (u131, divdi3_clz_tab 0x00348098)
 * ends right where it starts. */
static const UQItype __clz_tab[] =
{                                                   /* 0x00348198 (.rodata) */
  0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
  6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
  8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
};

/* libgcc2.c __udivmoddi4 (UDIV_NEEDS_NORMALIZATION variant), static inline
 * in the division objects: inlined into __moddi3, which passes rp = &w (its
 * stack slot, sp+0). */
static inline UDItype
__udivmoddi4 (UDItype n, UDItype d, UDItype *rp)
{
  DWunion ww;
  DWunion nn, dd;
  DWunion rr;
  USItype d0, d1, n0, n1, n2;
  USItype q0, q1;
  USItype b, bm;

  nn.ll = n;
  dd.ll = d;

  d0 = dd.s.low;
  d1 = dd.s.high;
  n0 = nn.s.low;
  n1 = nn.s.high;

  if (d1 == 0)
    {
      if (d0 > n1)
        {
          /* 0q = nn / 0D */

          count_leading_zeros (bm, d0);

          if (bm != 0)
            {
              /* Normalize, i.e. make the most significant bit of the
                 denominator set.  */

              d0 = d0 << bm;
              n1 = (n1 << bm) | (n0 >> (SI_TYPE_SIZE - bm));
              n0 = n0 << bm;
            }

          udiv_qrnnd (q0, n0, n1, n0, d0);
          q1 = 0;

          /* Remainder in n0 >> bm.  */
        }
      else
        {
          /* qq = NN / 0d */

          if (d0 == 0)
            {
#ifdef BOOT_OLD_GCC
              d0 = 1 / d0;      /* Divide intentionally by zero.  */
#else
              /* GCC 14 may fold a known division by zero away: divide
                 through a volatile so the `divu' (and its trap) stays. */
              volatile USItype zero = d0;
              d0 = 1 / zero;
#endif
            }

          count_leading_zeros (bm, d0);

          if (bm == 0)
            {
              /* From (n1 >= d0) /\ (the most significant bit of d0 is set),
                 conclude (the most significant bit of n1 is set) /\ (the
                 leading quotient digit q1 = 1).

                 This special case is necessary, not an optimization.
                 (Shifts counts of SI_TYPE_SIZE are undefined.)  */

              n1 -= d0;
              q1 = 1;
            }
          else
            {
              /* Normalize.  */

              b = SI_TYPE_SIZE - bm;

              d0 = d0 << bm;
              n2 = n1 >> b;
              n1 = (n1 << bm) | (n0 >> b);
              n0 = n0 << bm;

              udiv_qrnnd (q1, n1, n2, n1, d0);
            }

          /* n1 != d0...  */

          udiv_qrnnd (q0, n0, n1, n0, d0);

          /* Remainder in n0 >> bm.  */
        }

      if (rp != 0)
        {
          rr.s.low = n0 >> bm;
          rr.s.high = 0;
          *rp = rr.ll;
        }
    }
  else
    {
      if (d1 > n1)
        {
          /* 00 = nn / DD */

          q0 = 0;
          q1 = 0;

          /* Remainder in n1n0.  */
          if (rp != 0)
            {
              rr.s.low = n0;
              rr.s.high = n1;
              *rp = rr.ll;
            }
        }
      else
        {
          /* 0q = NN / dd */

          count_leading_zeros (bm, d1);
          if (bm == 0)
            {
              /* From (n1 >= d1) /\ (the most significant bit of d1 is set),
                 conclude (the most significant bit of n1 is set) /\ (the
                 quotient digit q0 = 0 or 1).

                 This special case is necessary, not an optimization.  */

              /* The condition on the next line takes advantage of that
                 n1 >= d1 (true due to program flow).  */
              if (n1 > d1 || n0 >= d0)
                {
                  q0 = 1;
                  sub_ddmmss (n1, n0, n1, n0, d1, d0);
                }
              else
                q0 = 0;

              q1 = 0;

              if (rp != 0)
                {
                  rr.s.low = n0;
                  rr.s.high = n1;
                  *rp = rr.ll;
                }
            }
          else
            {
              USItype m1, m0;
              /* Normalize.  */

              b = SI_TYPE_SIZE - bm;

              d1 = (d1 << bm) | (d0 >> b);
              d0 = d0 << bm;
              n2 = n1 >> b;
              n1 = (n1 << bm) | (n0 >> b);
              n0 = n0 << bm;

              udiv_qrnnd (q0, n1, n2, n1, d1);
              umul_ppmm (m1, m0, q0, d0);

              if (m1 > n1 || (m1 == n1 && m0 > n0))
                {
                  q0--;
                  sub_ddmmss (m1, m0, m1, m0, d1, d0);
                }

              q1 = 0;

              /* Remainder in (n1n0 - m1m0) >> bm.  */
              if (rp != 0)
                {
                  sub_ddmmss (n1, n0, n1, n0, m1, m0);
                  rr.s.low = (n1 << b) | (n0 >> bm);
                  rr.s.high = n1 >> bm;
                  *rp = rr.ll;
                }
            }
        }
    }

  ww.s.low = q0;
  ww.s.high = q1;
  return ww.ll;
}

/* ------------------------------------------------------------------------ */
/* L_fixsfdi                                                                */
/* ------------------------------------------------------------------------ */

// FUNC 0x00278630 __fixsfdi
/* float -> signed 64-bit, truncating: a < 0 (c.lt.s against 0.0f) negates
 * with neg.s, converts the magnitude and negates the result (dsubu).
 * Only caller: gt2k_menu::MemorycardGT3File::makeUniqueName (0x00144B90). */
DItype
__fixsfdi (SFtype a)
{
  if (a < 0)
    return - __fixunssfdi (-a);
  return __fixunssfdi (a);
}

/* ------------------------------------------------------------------------ */
/* L_fixunssfdi                                                             */
/* ------------------------------------------------------------------------ */

#define WORD_SIZE (sizeof (SItype) * BITS_PER_UNIT)
#define HIGH_HALFWORD_COEFF (((UDItype) 1) << (WORD_SIZE / 2))
#define HIGH_WORD_COEFF (((UDItype) 1) << WORD_SIZE)

// FUNC 0x00278670 __fixunssfdi
/* float -> unsigned 64-bit, through double.  Every double operation is a
 * GOFAST libcall the compiler emits itself: the widening is fptodp
 * (0x0027AFA0), `a < 0' dpcmp, `a / HIGH_WORD_COEFF' a dpmul by 2^-32
 * (fold's exact reciprocal), (USItype) dptoul, (DFtype) v of the unsigned
 * DI the halve-convert-double expansion around __floatdidf
 * ((v & 1) | (v >> 1), then dpadd of the result to itself), and `-a'
 * dpsub (0.0, a) (expand_unop: "The US Software GOFAST library needs
 * this").  Only caller: __fixsfdi. */
DItype
__fixunssfdi (SFtype original_a)
{
  /* Convert the SFtype to a DFtype, because that is surely not going
     to lose any bits.  Some day someone else can write a faster version
     that avoids converting to DFtype, and verify it really works right.  */
  DFtype a = original_a;
  DFtype b;
  UDItype v;
  /* implicit: fptodp, dpcmp, dpmul, dptoul, __floatdidf, dpadd, dpsub */

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
}

/* ------------------------------------------------------------------------ */
/* L_floatdidf                                                              */
/* ------------------------------------------------------------------------ */

// FUNC 0x00278768 __floatdidf
/* libgcc2.c signed 64-bit -> double: (double)(int)hi * 2^16 * 2^16
 * + (double)(unsigned)lo; the compiler expands the unsigned SI -> DF
 * conversion as litodp plus 2^32 when the low word's top bit is set.
 * Callers: __fixunssfdi, _strtod_r (0x00282BF0), __fixunsdfdi
 * (0x0028D9B8). */
DFtype
__floatdidf (DItype u)
{
  DFtype d;
  /* implicit: litodp, dpmul, dpadd */

  d = (SItype) (u >> WORD_SIZE);
  d *= HIGH_HALFWORD_COEFF;
  d *= HIGH_HALFWORD_COEFF;
  d += (USItype) (u & (HIGH_WORD_COEFF - 1));

  return d;
}

/* ------------------------------------------------------------------------ */
/* L_floatdisf                                                              */
/* ------------------------------------------------------------------------ */

#define DI_SIZE (sizeof (DItype) * BITS_PER_UNIT)
/* IEEE_FLOAT_FORMAT */
#define DF_SIZE 53
#define SF_SIZE 24

// FUNC 0x00278800 __floatdisf
/* libgcc2.c signed 64-bit -> float, computed in double: protect against
 * double rounding by folding the 11 low bits that DFmode would drop into a
 * sticky bit 11 when |u| >= 2^53 (the range test is the unsigned compare
 * u + (2^53 - 1) > 2^54 - 2; the bit test reads only the low word), then
 * the __floatdidf body inline and a final dptofp.  Callers:
 * gt2k_menu::PrizeCounter::digit (0x00129208), RaceTimeDisplay::draw
 * (0x00197468), RaceRoundMeterBase::render_2nd (0x0019C540),
 * pdibgm_get_status (0x0025CA98). */
SFtype
__floatdisf (DItype u)
{
  /* Do the calculation in DFmode
     so that we don't lose any of the precision of the high word
     while multiplying it.  */
  DFtype f;
  /* implicit: litodp, dpmul, dpadd, dptofp */

  /* Protect against double-rounding error.
     Represent any low-order bits, that might be truncated in DFmode,
     by a bit that won't be lost.  The bit can go in anywhere below the
     rounding position of the SFmode.  A fixed mask and bit position
     handles all usual configurations.  It doesn't handle the case
     of 128-bit DImode, however.  */
  if (DF_SIZE < DI_SIZE
      && DF_SIZE > (DI_SIZE - DF_SIZE + SF_SIZE))
    {
#define REP_BIT ((USItype) 1 << (DI_SIZE - DF_SIZE))
      if (! (- ((DItype) 1 << DF_SIZE) < u
             && u < ((DItype) 1 << DF_SIZE)))
        {
          if ((USItype) u & (REP_BIT - 1))
            u |= REP_BIT;
        }
    }
  f = (SItype) (u >> WORD_SIZE);
  f *= HIGH_HALFWORD_COEFF;
  f *= HIGH_HALFWORD_COEFF;
  f += (USItype) (u & (HIGH_WORD_COEFF - 1));

  return (SFtype) f;
}

/* ------------------------------------------------------------------------ */
/* L_moddi3                                                                 */
/* ------------------------------------------------------------------------ */

// FUNC 0x002788E0 __moddi3
/* Signed 64-bit remainder: only the dividend's sign (kept in $s0) decides
 * the sign of the result; the divisor is just made positive.  w lives at
 * sp+0 and is passed as __udivmoddi4's rp.  Callers:
 * gt2k_menu::PrizeCounter::digit (0x00129208), sce_doprnt (0x00287580).
 * Exact under the reference compiler 991111-01.  The default 991111b
 * differs in two things the source does not reach: the frame (0x20 instead
 * of 0x40: 991111-01 reserves 32 more bytes when it inlines __udivmoddi4,
 * vars 48 vs 16; with __udivmoddi4 out of line both say 16) and one delay
 * slot 991111b's reorg leaves empty (`bne; nop' at the m1 == n1 test,
 * where retail and 991111-01 put the `sltu'). */
DItype
__moddi3 (DItype u, DItype v)
{
  word_type c = 0;
  DWunion uu, vv;
  DItype w;

  uu.ll = u;
  vv.ll = v;

  if (uu.s.high < 0)
    c = ~c,
    uu.ll = __negdi2 (uu.ll);
  if (vv.s.high < 0)
    vv.ll = __negdi2 (vv.ll);

  (void) __udivmoddi4 (uu.ll, vv.ll, (UDItype *) &w);
  if (c)
    w = __negdi2 (w);

  return w;
}

/* ------------------------------------------------------------------------ */
/* L_muldi3                                                                 */
/* ------------------------------------------------------------------------ */

// FUNC 0x00278F48 __muldi3
/* 64x64->64 multiply (the R5900 has no dmult).  low*low through
 * __umulsidi3 (longlong.h's `multu' asm), the two cross products
 * low(u)*high(v) and high(u)*low(v) via the EE's 3-operand `mult' on
 * pipeline 0 and `mult1' on pipeline 1, added into the high word.
 * Callers: gt2k_menu::mcDecodeNameCode (0x00144A80), dpmul, _strtol_r
 * (0x0027F078), _malloc_trim_r (0x00281568), _strtoul_r (0x00283B30). */
DItype
__muldi3 (DItype u, DItype v)
{
  DWunion w;
  DWunion uu, vv;

  uu.ll = u,
  vv.ll = v;

  w.ll = __umulsidi3 (uu.s.low, vv.s.low);
  w.s.high += ((USItype) uu.s.low * (USItype) vv.s.high
               + (USItype) uu.s.high * (USItype) vv.s.low);

  return w.ll;
}

/* ------------------------------------------------------------------------ */
/* L_pure                                                                   */
/* ------------------------------------------------------------------------ */

// FUNC 0x00278FA8 __pure_virtual
/* Fills the pure-virtual slots of abstract-class vtables (MemorycardFile,
 * MemorycardFileManagerBase, RaceDisplayEventBase, VariableCameraBase,
 * PortBase, Serialize::Base, IOPLoadModuleBase, ...).  inhibit_libc build,
 * so no "pure virtual method called" message: the whole body is a tail jump
 * `j __terminate` (0x002776D0). */
void
__pure_virtual (void)
{
  __terminate ();
}
