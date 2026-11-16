/* infconst.c - newlib s_infconst.o (libm/common/s_infconst.c):
 * __infinity, the +inf behind math.h's HUGE_VAL (`__infinity[0].d').  A
 * data-only object: no code, .rodata 0x003490E8..0x003490F0.
 *
 * Evidence:
 *   - strtod (u138's _strtod_r, `ld $s1' at 0x002831EC) loads its overflow
 *     result from 0x003490E8: a memory object, as newlib's HUGE_VAL is (an
 *     extern const; the compiler builds double constants it knows in
 *     registers instead: _mprec_log10's 1.0 and 10.0 are ori / dsll32).
 *   - The 8 bytes follow mprec.o's tables (0x00348FC0..0x003490E8) and
 *     precede libkernl's first .rodata (u140's thread.o string, 0x003490F0):
 *     an object linked after the last newlib objects with code (readr.o,
 *     reallocr.o, callocr.o: no .rodata) and before libkernl's syscall table
 *     (syscalls.S).  It has no code, so nothing in .text orders it:
 *     the LINK_AFTER line below puts it after callocr.o's file for the range
 *     tools (tools/datalayout.py tu_order), the DATA_START line opens its
 *     range there (strtod's code is the only reference).
 *   - Only the double is there (no float __infinityf word beside it): this
 *     newlib's s_infconst.o defined __infinity alone.
 * The value is the original's: words 0x00000000, 0x7FF00000 (little-endian
 * IEEE +inf).
 */
// LINK_AFTER src/runtime/newlib/mallocr_part2.c
// DATA_START 0x003490E8
#include "types.h"

/* newlib math.h's union (two 32-bit words, IEEE_8087 order); math.h declares
 * __infinity as an array "to avoid a problem with gcc's handling of
 * initialized union constants". */
union __dmath {
    u32    i[2];
    double d;
};

const union __dmath __infinity[1] = { { { 0x00000000, 0x7FF00000 } } };      /* 0x003490E8 */
