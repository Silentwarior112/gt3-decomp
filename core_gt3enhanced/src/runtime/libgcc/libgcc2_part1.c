/* libgcc2_part1.c - GCC 2.9x-ee libgcc2.c pieces, 0x00276EA8..0x002776C0.
 *
 *   __main.o   0x00276EA8  __do_global_dtors, __do_global_ctors, __main   (L__main)
 *   _divdi3.o  0x00276FD0  __divdi3                                       (L_divdi3)
 *
 * The same objects as the bootstrap's runtime/libgcc2.c (L__main) and
 * runtime/libgcc_math.c (L_divdi3) (the bootstrap links the toolchain's own
 * libgcc.a and never matched them).  __do_global_dtors' .data cursor is
 * addressed with lui/addiu here; there are no callers.  L_divdi3 is
 * libgcc2.c / longlong.h of this egcs as u133's libgcc2_part2.c has them
 * (its __moddi3 is _moddi3.o, the same code with a remainder).
 *
 * The core's collect2 lists: __CTOR_LIST__ @0x0028F020 = {0x6E (110), the
 * 110 static constructors, 0}, __DTOR_LIST__ @0x0028F1E0.
 *
 * Data, as libgcc2.c has it: __main.o's two function-local statics,
 * __do_global_dtors' cursor `p' (.data 0x00305EB8, the object's only .data)
 * and __main's guard `initialized' (.bss 0x004AD200), and _divdi3.o's
 * `static const UQItype __clz_tab[]' (.rodata 0x00348098..0x00348198, the
 * object's only .rodata), written divdi3_clz_tab: u133's _moddi3.o copy is
 * the declared __clz_tab, and one name at two addresses would drop both.
 */
#include "runtime/libgcc/libgcc2_part1_internal.h"

typedef void (*func_ptr)(void);

/* ------------------------------------------------------------------------ */
/* L__main                                                                  */
/* ------------------------------------------------------------------------ */

// FUNC 0x00276EA8 __do_global_dtors
/* Run the global destructors in list order: while (*p) { p++; (*(p-1))(); }.
 * The cursor p (the .data word 0x00305EB8 = 0x0028F1E4) is stored back
 * before each call, so a second call resumes after the destructors that
 * already ran.  The count word is ignored; the walk stops at the 0
 * terminator.  No callers. */
void __do_global_dtors(void)
{
    static func_ptr *p = __DTOR_LIST__ + 1;     /* 0x00305EB8 */
    while (*p) {
        p++;
        (*(p - 1))();
    }
}

/* libgcc2.c's default DO_GLOBAL_CTORS_BODY, a do { } while (0) statement
 * macro: its loop notes give retail's registers ($s1 = i, $s0 = the list
 * pointer; written as a plain block the two are swapped). */
#define DO_GLOBAL_CTORS_BODY                                                \
do {                                                                        \
    unsigned long nptrs = (unsigned long)__CTOR_LIST__[0];                  \
    unsigned i;                                                             \
    if (nptrs == (unsigned long)-1)                                         \
        for (nptrs = 0; __CTOR_LIST__[nptrs + 1] != 0; nptrs++);            \
    for (i = nptrs; i >= 1; i--)        /* unsigned: while i != 0 */        \
        __CTOR_LIST__[i]();                                                 \
} while (0)

// FUNC 0x00276F00 __do_global_ctors
/* Run the global constructors in REVERSE list order (GCC's DO_GLOBAL_CTORS_BODY):
 * nptrs = __CTOR_LIST__[0]; if that is -1 the entries are counted up to the
 * 0 terminator; then __CTOR_LIST__[nptrs] .. __CTOR_LIST__[1] are called.
 * In the core nptrs = 0x6E: the 110 _GLOBAL_$I$ functions (the last entry,
 * called first, is the crtbegin EH frame registration).  Reached only by the
 * tail jump in __main.  No atexit(__do_global_dtors) in this build. */
void __do_global_ctors(void)
{
    DO_GLOBAL_CTORS_BODY;
}

// FUNC 0x00276FB0 __main
/* Called by GCC at the top of main() (0x00100140); also referenced from the
 * data word 0x00350B00.  Runs the global constructors exactly once
 * (supports recursive main).  The call to __do_global_ctors is a tail jump
 * (`j 0x00276F00` with the guard store in the delay slot). */
void __main(void)
{
    /* Support recursive calls to `main': run initializers just once. */
    static int initialized;                     /* 0x004AD200 (.bss) */
    if (!initialized) {
        initialized = 1;
        __do_global_ctors();
    }
}

/* ------------------------------------------------------------------------ */
/* L_divdi3                                                                 */
/* ------------------------------------------------------------------------ */

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
 * (_divdi3.o's copy divdi3_clz_tab, 0x00348098).  The dead `for' branch
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
        (count) = SI_TYPE_SIZE - (divdi3_clz_tab[__xr >> __a] + __a);       \
    } while (0)

/* libgcc2.c: static inline in the division objects (L_divdi3, L_moddi3),
 * i.e. inlined into __divdi3 as the 32-bit-half negate
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

/* libgcc2.c's `static const UQItype __clz_tab[]' (L_udivmoddi4): the bit
 * length of 0..255, _divdi3.o's own copy. */
static const UQItype divdi3_clz_tab[256] = {        /* 0x00348098 */
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
 * in the division objects: inlined into __divdi3, which passes rp = 0 (the
 * remainder code folds away). */
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


// FUNC 0x00276FD0 __divdi3
/* Signed 64-bit divide: negate negative operands (flag c toggled for each,
 * kept in $t3), unsigned divide with no remainder (_divdi3.o's own
 * __clz_tab divdi3_clz_tab at 0x00348098), negate the quotient if exactly
 * one operand was negative.  Callers: gt2k_menu::PrizeCounter::digit,
 * MenuBgMovie::update, sce_doprnt (0x00287580). */
DWtype
__divdi3(DWtype u, DWtype v)
{
    word_type c = 0;
    DWunion uu, vv;
    DWtype w;

    uu.ll = u;
    vv.ll = v;

    if (uu.s.high < 0)
        c = ~c,
        uu.ll = __negdi2(uu.ll);
    if (vv.s.high < 0)
        c = ~c,
        vv.ll = __negdi2(vv.ll);

    w = __udivmoddi4(uu.ll, vv.ll, (UDWtype *)0);
    if (c)
        w = __negdi2(w);

    return w;
}
