/* T547_op_new_arr.cpp - operator new[](size_t) (egcs new1.cc, L_op_vnew),
 * 0x0027C2D8..0x0027C34F.  Same machine code as bootstrap runtime/new.cc.
 * (egcs's <new> first, see T544.)  .text and .rodata ("bad_alloc",
 * 0x00348430) are those of opvnew.o in the toolchain's own libgcc.a.
 * The .rodata is only <new>'s "bad_alloc", which no code references: anchored.
 */
// DATA_START 0x00348430
#include "runtime/cxx/cxx_runtime_internal.h"
#include "runtime/cxx/T547_op_new_arr.h"

#ifndef BOOT_OLD_GCC
/* throw(bad_alloc) is the original specification; GCC 14 only deprecates it. */
#pragma GCC diagnostic ignored "-Wdeprecated"
#endif

// FUNC 0x0027C2D8 operator new[](unsigned int)
//   Forwards to ::operator new (0x0027C138).  throw(bad_alloc) spec: landing
//   pads call __check_eh_spec(1, {&__tf9bad_alloc()}).  9 callers.
void *operator new[](size_t sz) throw(bad_alloc)
{
    /* implicit: __tf9bad_alloc (the throw(bad_alloc) specification) */
    return ::operator new(sz);
}
