/* Common types for the GT3 US bootstrap (BOOTSTRAP_SCUS-97102) decompilation.
 * Target: PS2 EE (MIPS R5900), 32-bit pointers, 64-bit GPRs, 128-bit lq/sq.
 *
 * The tree builds with two compilers (see MATCHING.md):
 *   - SCE ee-gcc 2.9-ee-991111b (egcs era), the compiler the retail file was
 *     built with: the matching build.  C is GNU C89, C++ is the egcs dialect.
 *   - GCC 14 (ps2sdk): the behaviour build.
 * Everything below is written so both accept it. */
#ifndef BOOT_TYPES_H
#define BOOT_TYPES_H

#if defined(__GNUC__) && __GNUC__ < 3
#define BOOT_OLD_GCC 1
#endif

#define BOOT_CAT_(a, b) a##b
#define BOOT_CAT(a, b)  BOOT_CAT_(a, b)

#ifdef BOOT_OLD_GCC
/* egcs has no static_assert: an extern array of negative size fails the
 * compile instead.  (extern, not typedef: repeated declarations on the same
 * line number from different headers are then compatible, not an error.)
 * Only usable at file or namespace scope. */
#define static_assert(c, m)  extern char BOOT_CAT(boot_static_assert_, __LINE__)[(c) ? 1 : -1]
#define _Static_assert(c, m) static_assert(c, m)
#ifdef __cplusplus
#define noexcept
#define override
#define nullptr 0
#endif
/* Attributes the old compiler does not know (it would only warn). */
#define BOOT_USED
#define BOOT_MAY_ALIAS
/* EE sqrt.s.  egcs turns __builtin_sqrtf into a call of sqrtf, which this
 * binary does not have; the retail code has the instruction inline. */
static inline float boot_sqrtf(float x)
{
    float r;
    __asm__("sqrt.s %0, %1" : "=f"(r) : "f"(x));
    return r;
}
#else
#define BOOT_USED      __attribute__((used))
#define BOOT_MAY_ALIAS __attribute__((may_alias))
#define boot_sqrtf(x)  __builtin_sqrtf(x)
#endif

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned int        u32;
typedef unsigned long long  u64;
typedef signed char         s8;
typedef signed short        s16;
typedef signed int          s32;
typedef signed long long    s64;
typedef float               f32;
typedef double              f64;

typedef unsigned int u128 __attribute__((mode(TI)));
typedef int          s128 __attribute__((mode(TI)));

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C extern
#endif

/* Uncached / uncached-accelerated views of a KSEG0 address. */
#define UNCACHED(p)     ((void *)((u32)(p) | 0x20000000))
#define UNCACHED_ACC(p) ((void *)((u32)(p) | 0x30000000))

/* Raw memory-mapped register access. */
#define REG32(a)  (*(volatile u32 *)(a))
#define REG64(a)  (*(volatile u64 *)(a))
#define REG128(a) (*(volatile u128 *)(a))

#endif /* BOOT_TYPES_H */
