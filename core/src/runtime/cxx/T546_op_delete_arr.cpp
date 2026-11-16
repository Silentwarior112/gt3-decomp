/* T546_op_delete_arr.cpp - operator delete[](void *) (egcs new2.cc,
 * L_op_vdel), 0x0027C260..0x0027C2D7.  Same machine code as bootstrap
 * runtime/new.cc.  throw() spec, same landing-pad shape as operator delete
 * (T544: egcs's <new> first).  .text and .rodata ("bad_alloc", 0x00348420)
 * are those of opvdel.o in the toolchain's own libgcc.a.
 * The .rodata is only <new>'s "bad_alloc", which no code references: anchored.
 */
// DATA_START 0x00348420
#include "runtime/cxx/cxx_runtime_internal.h"
#include "runtime/cxx/T546_op_delete_arr.h"

// FUNC 0x0027C260 operator delete[](void *)
//   Same as operator delete: `if (ptr) free(ptr);` - calls free (0x0026C380)
//   directly, not ::operator delete.  3 callers (e.g. ~ParticleManager).
void operator delete[](void *ptr) throw()
{
    if (ptr)
        free(ptr);
}
