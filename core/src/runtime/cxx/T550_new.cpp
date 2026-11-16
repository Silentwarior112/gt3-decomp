#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/runtime/cxx/T550_new.cpp"
#pragma implementation "src/runtime/cxx/T550_new.cpp"
#pragma implementation "new"
#endif
/* T550_new.cpp - new.cc of the GCC 2.9x-ee C++ runtime, 0x0027C010..0x0027C0BF:
 * set_new_handler, the placement forms and the out-of-line copies of
 * bad_alloc's inline members (#pragma implementation "new").  Same machine
 * code as bootstrap runtime/new.cc.
 *
 * Emission order: set_new_handler (the only non-inline function), then
 * <new>'s inline functions in REVERSE definition order - placement new[],
 * placement new, bad_alloc::what - then bad_alloc's vtable companions
 * (__tf9bad_alloc, the implicit ~bad_alloc).  Checked with ee-gcc
 * 2.9-ee-991111b on `#pragma implementation "new"` + egcs's own <new>: it
 * emits __vn__FUiPv, __nw__FUiPv, what__C9bad_alloc, __tf9bad_alloc,
 * _$_9bad_alloc in that order.  .text, .rodata and .data are those of new.o
 * in the toolchain's own libgcc.a (sdk/ee-gcc-2.9-991111b).
 *
 * egcs build (BOOT_OLD_GCC): as the original, egcs's own <new> (cxx_runtime_internal.h)
 * with #pragma implementation "new" (first lines; the #line + first pragma
 * name this file, as T548): egcs emits the inline members of <new> itself,
 * so the definitions below marked "GCC 14 build only" are the modern
 * build's (the FUNC markers stay).
 *
 * Data: __new_handler @0x0030684C (.data), "bad_alloc" @0x003483C0, nothrow
 * @0x003483D0, _vt$9bad_alloc @0x003483D8, "9bad_alloc" @0x003483F0,
 * __ti9bad_alloc @0x004B2B40 (.bss, 0xC).
 */
#include "runtime/cxx/cxx_runtime_internal.h"
#include "runtime/cxx/T550_new.h"

#ifdef BOOT_OLD_GCC
/* egcs new.cc's `const std::nothrow_t std::nothrow = { };` (std:: is
 * ignored): the 1-byte tag object of the nothrow forms, 8-aligned between the
 * "bad_alloc" literal and the vtable.  Unreferenced (the nothrow forms were
 * not linked). */
const nothrow_t nothrow = { };                          /* 0x003483D0 (.rodata) */

/* new.cc defines the handler without an initializer; egcs emits it as a zero
 * word in new.o's .data (the target has no .bss output for it). */
new_handler __new_handler;                              /* 0x0030684C (.data) */
#endif

// FUNC 0x0027C010 set_new_handler
//   Swap __new_handler (0x0030684C) and return the previous handler.  No
//   callers.
new_handler set_new_handler(new_handler handler)
{
    new_handler prev_handler = __new_handler;
    __new_handler = handler;
    return prev_handler;
}

#ifndef BOOT_OLD_GCC                    /* egcs: inline in <new> */
// FUNC 0x0027C020 operator new[](unsigned int, void *)
//   Default placement array new: returns place.  Emitted first: <new>
//   defines it after the scalar form, and egcs emits inline functions in
//   reverse order (see the file comment).  No callers.
void *operator new[](size_t, void *place) throw()
{
    return place;
}

// FUNC 0x0027C028 operator new(unsigned int, void *)
//   Default placement new: returns place (the same two instructions as the
//   array form).  No callers.
void *operator new(size_t, void *place) throw()
{
    return place;
}

// FUNC 0x0027C030 bad_alloc::what() const
//   Slot 1 (+0x14) of _vt$9bad_alloc.  Returns "bad_alloc" (.rodata 0x003483C0).
const char *bad_alloc::what() const throw()
{
    return "bad_alloc";
}
#endif

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0027C040 __tf9bad_alloc
//   Vtable entry 0 of bad_alloc.  If __ti9bad_alloc (0x004B2B40) is unbuilt:
//   __tf9exception() (0x0027A3C0, result unused), then
//   __rtti_si(&__ti9bad_alloc, "9bad_alloc" (0x003483F0), &__ti9exception
//   (0x00353510)).  Returns __ti9bad_alloc.  Called by the exception-spec
//   landing pads of operator new / new[] and, through the vtable, by the
//   throw in operator new.
extern "C" const type_info &__tf9bad_alloc(void)
{
    if (__ti9bad_alloc[0] == 0) {
        __tf9exception();                   /* the base's node first */
        __rtti_si(__ti9bad_alloc, "9bad_alloc", (const type_info *)__ti9exception);
    }
    return *(const type_info *)__ti9bad_alloc;
}

/* GCC 14 stand-in for vtable entry 0 (egcs stores __tf9bad_alloc itself
 * there).  No address of its own in the binary. */
const type_info &bad_alloc::__tf() const
{
    return __tf9bad_alloc();
}
#endif

#ifndef BOOT_OLD_GCC                    /* egcs: implicit (<new> declares none) */
// FUNC 0x0027C090 bad_alloc::~bad_alloc() (_$_9bad_alloc)
//   Implicit virtual dtor, slot 0 (+0xC).  (this, flags): the inlined
//   ~exception stores _vt$9exception 0x003482B0 into the vptr (+0) (an
//   implicit destructor sets no vptr of its own), then operator delete(this)
//   (0x0027C0C0) if (flags & 1).  Also the cleanup function of the object
//   thrown by operator new.
bad_alloc::~bad_alloc()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
}
#endif
