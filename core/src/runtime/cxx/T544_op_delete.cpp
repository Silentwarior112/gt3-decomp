/* T544_op_delete.cpp - operator delete(void *) (egcs new2.cc, L_op_delete),
 * 0x0027C0C0..0x0027C137.  Same machine code as bootstrap runtime/new.cc.
 * throw() spec: landing pads 0x0027C0D8.. (__start_cp_handler,
 * __check_eh_spec(0, ...), __cp_pop_exception, __throw, std::terminate).
 * egcs build: cxx_runtime_internal.h includes egcs's <new> first, as new2.cc does; egcs
 * then declares __check_eh_spec itself (noreturn: the pads go on to
 * __throw, not back to the body).  .text and .rodata ("bad_alloc",
 * 0x00348400) are those of opdel.o in the toolchain's own libgcc.a.
 * The .rodata is only <new>'s "bad_alloc", which no code references: anchored.
 */
// DATA_START 0x00348400
#include "runtime/cxx/cxx_runtime_internal.h"
#include "runtime/cxx/T544_op_delete.h"

// FUNC 0x0027C0C0 operator delete(void *)
//   `if (ptr) free(ptr);` (free = 0x0026C380).  126 callers (every deleting
//   destructor of the game).
void operator delete(void *ptr) throw()
{
    if (ptr)
        free(ptr);
}
