/* new.h - egcs-1.1-era <new> / <exception> declarations for the cxxrt unit
 * (runtime/new.cc, runtime/tinfo.cc, runtime/tinfo2.cc) of the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102).
 *
 * The C++ runtime in this binary is the libgcc.a C++ part of GCC 2.9x-ee
 * (egcs 1.1 era).  Its classes live in the GLOBAL namespace (mangled names
 * "9exception", "9bad_alloc", no "St" prefix: -fhonor-std was off).
 *
 * Object model notes (apply to every polymorphic class here):
 *   - vtables are arrays of 8-byte entries {s16 delta; s16 index; void *pfn};
 *     the vptr points at entry 0.  Entry 0's pfn is the class's typeinfo
 *     function (__tf<mangled>), entry 1 the virtual destructor, entry 2+ the
 *     other virtuals.  A virtual call to slot n is
 *       lh delta, 8n(vptr); lw pfn, 8n+4(vptr); pfn(this + delta, ...).
 *   - GCC 2.9x destructors take a hidden `int flags` (bit 0 = operator delete
 *     this after destruction).  Written here as normal destructors.
 *   - Constructors return `this` in $v0.
 *
 * Modern g++ lays these classes out with the vptr at +0; offsets quoted in
 * comments are those of the ORIGINAL binary.
 */
#ifndef BOOT_RUNTIME_NEW_H
#define BOOT_RUNTIME_NEW_H

#include "types.h"
#include <stddef.h>

/* The egcs dynamic exception specifications (throw(bad_alloc)) are kept on
 * purpose: they are what generated the landing pads in the binary. */
#pragma GCC diagnostic ignored "-Wdeprecated"

/* ------------------------------------------------------------------------ */
/* class exception (egcs <exception>)                                       */
/* ------------------------------------------------------------------------ */

/* Defined in runtime/exception.cc (libgcc_eh unit), which also includes this
 * header: exception() (0x010187D8), ~exception (0x010187A8),
 * exception::what (0x010186E8) and __tf9exception (0x010187F0).
 *
 * sizeof == 4: only the vptr (+0x00).
 * vtable _vt$9exception @0x010400C8 (.rodata):
 *   [0] __tf9exception 0x010187F0   [1] ~exception 0x010187A8   [2] what 0x010186E8
 * egcs declares `exception () { }` and `virtual ~exception () { }` inline in
 * <exception>; exception.o (#pragma implementation) also emits out-of-line
 * copies, and those are what the libgcc_eh unit defines, so both are declared
 * out of line here (integration: the defining unit wins).  In the binary every
 * derived ctor/dtor of this unit (bad_alloc, bad_typeid, bad_cast) inlines
 * them, which is why the dtors only store 0x010400C8 into the vptr before the
 * conditional operator delete.
 *
 * __tf(): egcs puts the class's typeinfo function itself in vtable entry 0,
 * and typeid(*obj) is a call through that entry.  Modern g++ has no such
 * entry, so each class here declares __tf() as its FIRST virtual (modern
 * vtable slot 0, as in egcs); every override just returns the class's
 * __tf<mangled>() and has no address of its own in the binary. */
class type_info;                                        /* runtime/tinfo.h */

class exception {
public:
    exception();                                        /* 0x010187D8 */
    virtual const type_info &__tf() const;              /* vtable entry 0 = __tf9exception */
    virtual ~exception();                               /* 0x010187A8 */
    virtual const char *what() const;                   /* 0x010186E8 */
};

extern "C" {
/* Defined in runtime/exception.cc (libgcc_eh unit).
 * __ti9exception: the __user_type_info object describing class exception,
 * 8 bytes @0x01041EB8 (.sbss), built by __tf9exception (0x010187F0). */
const type_info &__tf9exception(void);                  /* 0x010187F0 */
extern void *__ti9exception[2];                         /* @0x01041EB8 (.sbss) */
}

/* ------------------------------------------------------------------------ */
/* <new>                                                                    */
/* ------------------------------------------------------------------------ */

/* class bad_alloc (egcs <new>).  sizeof == 4 (vptr only); thrown objects are
 * allocated with __eh_alloc(4).
 * vtable _vt$9bad_alloc @0x010401E8 (.rodata):
 *   [0] __tf9bad_alloc 0x01019890   [1] ~bad_alloc 0x010198E0   [2] what 0x01019880
 * Typeinfo: __ti9bad_alloc @0x0104DDC8 (.bss), __si_type_info with base exception. */
class bad_alloc : public exception {
public:
    bad_alloc() {}                                      /* inline: vptr = 0x010401E8 */
    virtual const type_info &__tf() const;              /* vtable entry 0 = __tf9bad_alloc */
    virtual ~bad_alloc();                               /* 0x010198E0 (implicit in egcs) */
    virtual const char *what() const throw();           /* 0x01019880 "bad_alloc" */
};

/* egcs <new>: tag type of the nothrow allocation forms.  The object is defined
 * by new.cc (see there); the nothrow operator new/delete themselves were not
 * linked into this binary. */
struct nothrow_t {};
extern const nothrow_t nothrow;

typedef void (*new_handler)(void);

/* The handler operator new polls when malloc fails.  @0x0103C824 (.data,
 * new.o's only data word; initially 0). */
extern new_handler __new_handler;

new_handler set_new_handler(new_handler handler);       /* 0x01019860 */

/* Placement forms (inline in egcs <new>; out-of-line copies in new.o). */
void *operator new(size_t, void *place) throw();        /* 0x01019870 */
void *operator new[](size_t, void *place) throw();      /* 0x01019878 */

/* Global allocation functions (one libgcc.a member each; egcs new1.cc / new2.cc). */
void *operator new(size_t sz) throw(bad_alloc);         /* 0x01019988 */
void  operator delete(void *ptr) throw();               /* 0x01019910 */
void *operator new[](size_t sz) throw(bad_alloc);       /* 0x01019B28 */
void  operator delete[](void *ptr) throw();             /* 0x01019AB0 */

#if defined(__cplusplus) && __cplusplus >= 201103L
static_assert(sizeof(bad_alloc) == 4, "bad_alloc is vptr-only (__eh_alloc(4))");
#endif

#endif /* BOOT_RUNTIME_NEW_H */
