/* strtoul.c - newlib strtoul.o, the non-reentrant entry point
 * (0x00283D50).  _strtoul_r itself (0x00283B30) is in unit u138.
 *
 * `unsigned long` is 64 bits on the EE (the _strtoul_r core divides with
 * __udivdi3(-1, base) and returns the accumulator with daddu), so the
 * result is u64 here.
 */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part3_internal.h"

// FUNC 0x00283D50 strtoul
//   strtoul(nptr, endptr, base) = _strtoul_r(_impure_ptr, nptr, endptr, base).
//   No callers in the binary.
u64 strtoul(const char *nptr, char **endptr, int base)
{
    return _strtoul_r(_impure_ptr, nptr, endptr, base);
}
