/* T545_op_new.cpp - operator new(size_t) (egcs new1.cc, L_op_new),
 * 0x0027C138..0x0027C25F.  Same machine code as bootstrap runtime/new.cc.
 * egcs build: bad_alloc is egcs's <new> (cxx_runtime_internal.h includes it first, as
 * new1.cc): its implicit destructor is inlined into the temporary's
 * cleanup, and the pads see egcs's own noreturn __check_eh_spec (T544).
 * .text and .rodata ("bad_alloc", 0x00348410) are those of opnew.o in the
 * toolchain's own libgcc.a.
 * The .rodata is only <new>'s "bad_alloc", which no code references: anchored.
 */
// DATA_START 0x00348410
#include "runtime/cxx/cxx_runtime_internal.h"
#include "runtime/cxx/T545_op_new.h"

#ifndef BOOT_OLD_GCC
/* throw(bad_alloc) is the original specification (it generated the landing
 * pads); GCC 14 only deprecates it. */
#pragma GCC diagnostic ignored "-Wdeprecated"

/* The cleanup operator new hands to __cp_push_exception: in the binary it is
 * bad_alloc::~bad_alloc (0x0027C090) itself, called as (obj, 2) = destroy
 * without delete.  C++ cannot take a destructor's address, so this adaptor
 * runs it; flag 2 never deletes, which a plain destructor call matches.  Not
 * a function of the binary. */
static void bad_alloc_cleanup(void *obj, int /* flags: always 2 */)
{
    static_cast<bad_alloc *>(obj)->~bad_alloc();
}
#endif

// FUNC 0x0027C138 operator new(unsigned int)
//   malloc (0x0026C3A8) with size 0 bumped to 1 (movn); on failure call
//   __new_handler (0x0030684C, re-read every iteration) and retry, or throw
//   bad_alloc if there is no handler.  Nothing in the game installs a
//   handler, so an allocation failure throws immediately.
//   `throw bad_alloc()` is lowered as:
//       tmp.vptr = 0x003483D8;             // temporary bad_alloc at sp+0
//       obj = __eh_alloc(4);               // 0x00279D88 (terminates on NULL)
//       obj->vptr = 0x003483D8;            // copy of tmp
//       tmp.vptr = 0x003482B0;             // tmp destroyed (inlined ~exception)
//       __cp_push_exception(obj, obj->vptr[0].pfn() /* __tf9bad_alloc */,
//                           bad_alloc::~bad_alloc /* 0x0027C090 */);  // 0x00279E40
//       __throw();                         // 0x00278268
//   (landing pad 0x0027C1F8 destroys tmp and resumes unwinding if
//   __eh_alloc / the copy throws).  throw(bad_alloc) spec: the pads compute
//   spec[0] = &__tf9bad_alloc() and call __check_eh_spec(1, spec) (0x00279FC8).
void *operator new(size_t sz) throw(bad_alloc)
{
    /* implicit: __tf9bad_alloc (the throw(bad_alloc) specification and the throw) */
    void *p;

    /* malloc (0) is unpredictable; avoid it. */
    if (sz == 0)
        sz = 1;
    p = malloc(sz);
    while (p == 0) {
        new_handler handler = __new_handler;
        if (!handler) {
#ifdef BOOT_OLD_GCC
            throw bad_alloc();
#else
            /* throw bad_alloc(); written as the egcs lowering above (the
             * old-model EH of libgcc2 / exception.cc, not the modern C++ ABI). */
            bad_alloc *obj = new (__eh_alloc(sizeof(bad_alloc))) bad_alloc;
            __cp_push_exception(obj, (void *)&obj->__tf(), bad_alloc_cleanup);
            __throw();
#endif
        }
        handler();
        p = malloc(sz);
    }

    return p;
}
