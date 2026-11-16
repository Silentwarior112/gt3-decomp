#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/runtime/cxx/T548_tinfo.cpp"
#pragma implementation "src/runtime/cxx/T548_tinfo.cpp"
#pragma implementation "typeinfo"
#endif
/* T548_tinfo.cpp - tinfo.cc of the GCC 2.9x-ee C++ runtime,
 * 0x0027C350..0x0027CA1F.  Same machine code as bootstrap runtime/tinfo.cc.
 *
 * Source order (egcs gcc/cp/tinfo.cc): ~type_info, type_info::operator==,
 * __rtti_class, __rtti_si, __rtti_user, the three dcast methods; then the
 * out-of-line copies of the inline members of <typeinfo> and tinfo.h
 * (#pragma implementation "typeinfo").  egcs emits those in REVERSE
 * definition order (__class_type_info's ctor first, type_info's ctor last),
 * each class's typeinfo function and implicit destructor right after the
 * first inline member that needs its vtable: __class_type_info,
 * __si_type_info, __user_type_info, bad_typeid, bad_cast, type_info.
 * type_info::before opens tinfo2.cc (T549).
 *
 * egcs build (BOOT_OLD_GCC): type_info, bad_cast and bad_typeid are egcs's
 * <typeinfo> (T548_tinfo.h), whose inline members this file's #pragma
 * implementation "typeinfo" emits, as the original's does.  The ctors of
 * __user_type_info, __si_type_info and __class_type_info are inline in
 * T548_tinfo.h and their destructors implicit, as in egcs's tinfo.h: egcs
 * inlines the ctors into the __rtti_* builders and, since this file defines
 * the classes' first non-inline virtual (dcast), emits their out-of-line
 * copies too.  All these definitions below are the GCC 14 build's only (the
 * FUNC markers stay; those of the copies egcs emits from the header name
 * the mangled symbol).  cxx_runtime_internal.h includes egcs's <new> first, as tinfo.cc
 * does ("new", for the placement form): its "bad_alloc" literal opens this
 * object's .rodata (0x00348440).
 * The #line + first #pragma implementation (egcs only, first lines) name
 * this file: the SCE driver compiles a temporary .ii and egcs rejects the
 * pragma ("can only appear at top-level") when the names differ (u125's
 * trick).
 *
 * Checked: .text and .rodata of this object are byte-identical to tinfo.o of
 * the toolchain's own libgcc.a (sdk/ee-gcc-2.9-991111b), the retail code.
 *
 * .rodata: vtables __class 0x00348450, __si 0x00348468, __user 0x00348480,
 * bad_typeid 0x00348498, bad_cast 0x003484B0, type_info 0x003484C8; names
 * "17__class_type_info" 0x003484D8, "14__si_type_info" 0x003484F0,
 * "16__user_type_info" 0x00348508, "10bad_typeid" 0x00348520, "8bad_cast"
 * 0x00348530, "9type_info" 0x00348540.  The .rodata opens with <new>'s
 * "bad_alloc" (0x00348440), which no code references: anchored.
 */
// DATA_START 0x00348440
#include "runtime/cxx/cxx_runtime_internal.h"
#include "runtime/cxx/T548_tinfo.h"

// FUNC 0x0027C350 type_info::~type_info()
//   Virtual dtor, slot 0 (+0xC) of _vt$9type_info.  (this, flags): stores
//   type_info's vtable 0x003484C8 into the vptr at this+4, then
//   operator delete(this) (0x0027C0C0) if (flags & 1).  Every *_type_info
//   destructor calls this with (this, flags) unchanged and returns.
type_info::~type_info()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x0027C380 type_info::operator==(type_info const &) const
//   Same object, or same mangled name (strcmp 0x0027EA40): the name compare is
//   what makes typeinfo objects built in different places compare equal.
//   Used by all dcast methods, operator!= and the EH matcher.
bool type_info::operator==(const type_info &arg) const
{
    return (&arg == this) || (strcmp(name(), arg.name()) == 0);
}

// FUNC 0x0027C3C0 __rtti_class
//   extern "C" builder for the __tf functions of classes with multiple or
//   virtual bases (43 callers in the game: __tfQ29gt2k_menu13TopButtonList,
//   __tf9GarageCar, ...).  Placement-new a __class_type_info at addr (ctor
//   inlined: _name@0 = name, vptr@4 = 0x00348450, base_list@8 = bl,
//   n_bases@0xC = bn), skipped when addr == 0.  Returns nothing.
extern "C" void __rtti_class(void *addr, const char *name,
                             const __class_type_info::base_info *bl, size_t bn)
{
    /* egcs: new (addr) __class_type_info (name, bl, bn);  placement new is
     * throw(), so the compiler tests addr before running the ctor.  With the
     * test written out (here and in the other builders) egcs gives the same
     * code (checked); it keeps the GCC 14 build's behaviour explicit. */
    if (addr)
        new (addr) __class_type_info(name, bl, bn);
}

// FUNC 0x0027C3E8 __rtti_si
//   extern "C" builder called by every __tf function of a single-inheritance
//   class (405 callers).  Placement-new a __si_type_info at addr (ctor
//   inlined: _name@0 = n, vptr@4 = 0x00348468, base@8 = ti), skipped when
//   addr == 0.  Returns nothing.
extern "C" void __rtti_si(void *addr, const char *n, const type_info *ti)
{
    if (addr)
        new (addr) __si_type_info(n, static_cast<const __user_type_info &>(*ti));
}

// FUNC 0x0027C408 __rtti_user
//   extern "C" builder for the __tf functions of classes without bases (86
//   callers: __tf10GameObject, __tf9exception, __tf9type_info ...).
//   Placement-new a __user_type_info at addr (_name@0 = name,
//   vptr@4 = 0x00348480), skipped when addr == 0.  Returns nothing.
extern "C" void __rtti_user(void *addr, const char *name)
{
    if (addr)
        new (addr) __user_type_info(name);
}

// FUNC 0x0027C428 __user_type_info::dcast(type_info const &, int, void *, type_info const *, void *) const
//   Slot 1 (+0x14) of _vt$16__user_type_info.  A class without bases can only
//   be converted to itself: (*this == to) ? objptr : 0 (movz).
void *__user_type_info::dcast(const type_info &to, int, void *objptr,
                              const type_info *, void *) const
{
    return (*this == to) ? objptr : 0;  /* implicit: type_info::operator== */
}

// FUNC 0x0027C458 __si_type_info::dcast(type_info const &, int, void *, type_info const *, void *) const
//   Slot 1 of _vt$14__si_type_info.  Match self, else recurse into the single
//   base (this+8) through ITS virtual dcast (lh delta @vtbl+0x10, lw pfn
//   @vtbl+0x14) with all five arguments unchanged.
void *__si_type_info::dcast(const type_info &to, int require_public, void *objptr,
                            const type_info *sub, void *subptr) const
{
    if (*this == to)  /* implicit: type_info::operator== */
        return objptr;
    return base.dcast(to, require_public, objptr, sub, subptr);
}

// FUNC 0x0027C4F0 __class_type_info::dcast(type_info const &, int, void *, type_info const *, void *) const
//   Slot 1 of _vt$17__class_type_info.  Walks base_list[0..n_bases) (i
//   unsigned, sltu): skips non-public bases when is_public; base address =
//   objptr + (word4 & 0x1FFFFFFF), loaded through if is_virtual (bit 29) - NO
//   test of objptr == 0 (gcc 2.95's tinfo.cc guards this); recurses via the
//   base's virtual dcast.  Two different hits are resolved with the
//   sub/subptr protocol (two more virtual dcast calls on `desired` with
//   require_public 1 and sub/subptr 0), otherwise the cast is ambiguous -> 0.
//   Reached in the game through __dynamic_cast on the multiple-inheritance
//   classes whose __tf calls __rtti_class.
void *__class_type_info::dcast(const type_info &desired, int is_public,
                               void *objptr, const type_info *sub,
                               void *subptr) const
{
    if (*this == desired)  /* implicit: type_info::operator== */
        return objptr;

    void *match_found = 0;
    for (size_t i = 0; i < n_bases; i++) {
        if (is_public && base_list[i].access != PUBLIC)
            continue;

        void *p = (char *)objptr + base_list[i].offset;
        if (base_list[i].is_virtual)
            p = *(void **)p;
        p = base_list[i].base->dcast(desired, is_public, p, sub, subptr);

        if (p) {
            if (match_found == 0)
                match_found = p;
            else if (match_found != p) {
                if (sub) {
                    /* Perhaps we're downcasting from *sub to desired; see if
                     * subptr is a subobject of exactly one of {match_found,p}. */
                    const __user_type_info &d =
                        static_cast<const __user_type_info &>(desired);

                    /* egcs names: the *sub subobject of the old (os) and of
                     * the new (ns) match */
                    void *os = d.dcast(*sub, 1, match_found);
                    void *ns = d.dcast(*sub, 1, p);

                    if (os == ns)
                        /* ambiguous -- subptr is a virtual base */;
                    else if (os == subptr)
                        continue;
                    else if (ns == subptr) {
                        match_found = p;
                        continue;
                    }
                }

                /* base found at two different pointers, conversion is not
                 * unique */
                return 0;
            }
        }
    }

    return match_found;
}

#ifndef BOOT_OLD_GCC                    /* egcs: inline in tinfo.h (T548_tinfo.h) */
// FUNC 0x0027C6C8 __class_type_info::__class_type_info(char const *, base_info const *, unsigned int) (__17__class_type_infoPCcPCQ217__class_type_info9base_infoUi)
//   Out-of-line copy of the inline ctor: _name@0 = name, base_list@8 = bl,
//   vptr@4 = 0x00348450, n_bases@0xC = bn; returns this.  No null test, no
//   callers.
__class_type_info::__class_type_info(const char *name, const base_info *bl, size_t bn)
    : __user_type_info(name), base_list(bl), n_bases(bn)
{
}
#endif

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0027C6E8 __tf17__class_type_info
//   Entry 0 of _vt$17__class_type_info.  Builds __ti17__class_type_info
//   (0x004B2B90) once: __tf16__user_type_info() (0x0027C800), then
//   __rtti_si(ti, "17__class_type_info" (0x003484D8), &__ti16__user_type_info
//   (0x004B2B70)).  Returns &__ti17__class_type_info.
extern "C" const type_info &__tf17__class_type_info(void)
{
    if (__ti17__class_type_info[0] == 0) {
        __tf16__user_type_info();  /* the base's node first (result unused) */
        __rtti_si(__ti17__class_type_info, "17__class_type_info",
                  (const type_info *)__ti16__user_type_info);
    }
    return *(const type_info *)__ti17__class_type_info;
}
#endif

#ifndef BOOT_OLD_GCC                    /* egcs: implicit */
// FUNC 0x0027C738 __class_type_info::~__class_type_info() (_$_17__class_type_info)
//   Implicit virtual dtor, slot 0: calls type_info::~type_info (0x0027C350)
//   with (this, flags) unchanged; that one does the delete.  (An implicit
//   destructor with an empty body sets no vptr.)
__class_type_info::~__class_type_info()
{
    /* implicit: ~type_info (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x0027C758 __si_type_info::__si_type_info(char const *, __user_type_info const &) (__14__si_type_infoPCcRC16__user_type_info)
//   Out-of-line copy of the inline ctor: _name@0 = n, base@8 = b,
//   vptr@4 = 0x00348468; returns this.  No callers.
__si_type_info::__si_type_info(const char *n, const __user_type_info &b)
    : __user_type_info(n), base(b)
{
}
#endif

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0027C778 __tf14__si_type_info
//   Entry 0 of _vt$14__si_type_info.  Builds __ti14__si_type_info (0x004B2B80)
//   once: __tf16__user_type_info(), then __rtti_si(ti, "14__si_type_info"
//   (0x003484F0), &__ti16__user_type_info).
extern "C" const type_info &__tf14__si_type_info(void)
{
    if (__ti14__si_type_info[0] == 0) {
        __tf16__user_type_info();  /* the base's node first (result unused) */
        __rtti_si(__ti14__si_type_info, "14__si_type_info",
                  (const type_info *)__ti16__user_type_info);
    }
    return *(const type_info *)__ti14__si_type_info;
}
#endif

#ifndef BOOT_OLD_GCC                    /* egcs: implicit / inline in tinfo.h */
// FUNC 0x0027C7C8 __si_type_info::~__si_type_info() (_$_14__si_type_info)
//   Implicit virtual dtor, slot 0: forwards (this, flags) to ~type_info.
__si_type_info::~__si_type_info()
{
    /* implicit: ~type_info (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x0027C7E8 __user_type_info::__user_type_info(char const *) (__16__user_type_infoPCc)
//   Out-of-line copy of the inline ctor: _name@0 = n, vptr@4 = 0x00348480;
//   returns this.  No callers.
__user_type_info::__user_type_info(const char *n)
    : type_info(n)
{
}
#endif

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0027C800 __tf16__user_type_info
//   Entry 0 of _vt$16__user_type_info.  Builds __ti16__user_type_info
//   (0x004B2B70) once: __tf9type_info() (0x0027C9E0), then __rtti_si(ti,
//   "16__user_type_info" (0x00348508), &__ti9type_info (0x00353518)).  Also
//   called by the __class/__si typeinfo functions and, as the dynamic_cast
//   target, by the EH matcher.
extern "C" const type_info &__tf16__user_type_info(void)
{
    if (__ti16__user_type_info[0] == 0) {
        __tf9type_info();  /* the base's node first (result unused) */
        __rtti_si(__ti16__user_type_info, "16__user_type_info",
                  (const type_info *)__ti9type_info);
    }
    return *(const type_info *)__ti16__user_type_info;
}
#endif

#ifndef BOOT_OLD_GCC                    /* egcs: implicit */
// FUNC 0x0027C850 __user_type_info::~__user_type_info() (_$_16__user_type_info)
//   Implicit virtual dtor, slot 0: forwards (this, flags) to ~type_info.
__user_type_info::~__user_type_info()
{
    /* implicit: ~type_info (compiler-generated: bases, members, deleting form) */
}
#endif

#ifndef BOOT_OLD_GCC                    /* egcs: inline in <typeinfo> */
// FUNC 0x0027C870 bad_typeid::~bad_typeid()
//   Implicit virtual dtor, slot 0 of _vt$10bad_typeid.  The inlined
//   ~exception stores 0x003482B0 into the vptr (+0), then
//   operator delete(this) if (flags & 1).  Also the cleanup function that
//   __throw_bad_typeid passes to __cp_push_exception.
bad_typeid::~bad_typeid()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x0027C8A0 bad_typeid::bad_typeid()
//   Out-of-line copy of the inline ctor: vptr (+0) = 0x00348498 (the
//   exception vptr store is dead and dropped); returns this.  No callers.
bad_typeid::bad_typeid()
{
}
#endif

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0027C8B8 __tf10bad_typeid
//   Entry 0 of _vt$10bad_typeid.  Builds __ti10bad_typeid (0x004B2B50) once:
//   __tf9exception() (0x0027A3C0), then __rtti_si(ti, "10bad_typeid"
//   (0x00348520), &__ti9exception (0x00353510)).
extern "C" const type_info &__tf10bad_typeid(void)
{
    if (__ti10bad_typeid[0] == 0) {
        __tf9exception();  /* the base's node first (result unused) */
        __rtti_si(__ti10bad_typeid, "10bad_typeid",
                  (const type_info *)__ti9exception);
    }
    return *(const type_info *)__ti10bad_typeid;
}

/* GCC 14 stand-in for vtable entry 0 (egcs stores __tf10bad_typeid itself
 * there).  No address of its own in the binary. */
const type_info &bad_typeid::__tf() const
{
    return __tf10bad_typeid();
}
#endif

#ifndef BOOT_OLD_GCC                    /* egcs: inline in <typeinfo> */
// FUNC 0x0027C908 bad_cast::~bad_cast()
//   Implicit virtual dtor, slot 0 of _vt$8bad_cast: vptr (+0) = 0x003482B0
//   (inlined ~exception), operator delete(this) if (flags & 1).  Cleanup
//   function passed by __throw_bad_cast.
bad_cast::~bad_cast()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x0027C938 bad_cast::bad_cast()
//   Out-of-line copy of the inline ctor: vptr (+0) = 0x003484B0; returns
//   this.  No callers.
bad_cast::bad_cast()
{
}
#endif

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0027C950 __tf8bad_cast
//   Entry 0 of _vt$8bad_cast.  Builds __ti8bad_cast (0x004B2B60) once:
//   __tf9exception(), then __rtti_si(ti, "8bad_cast" (0x00348530),
//   &__ti9exception).
extern "C" const type_info &__tf8bad_cast(void)
{
    if (__ti8bad_cast[0] == 0) {
        __tf9exception();  /* the base's node first (result unused) */
        __rtti_si(__ti8bad_cast, "8bad_cast",
                  (const type_info *)__ti9exception);
    }
    return *(const type_info *)__ti8bad_cast;
}

/* GCC 14 stand-in for vtable entry 0 (egcs: __tf8bad_cast itself). */
const type_info &bad_cast::__tf() const
{
    return __tf8bad_cast();
}
#endif

#ifndef BOOT_OLD_GCC                    /* egcs: inline in <typeinfo> */
// FUNC 0x0027C9A0 type_info::operator!=(type_info const &) const
//   Out-of-line copy of the inline `!operator==(arg)` (calls 0x0027C380,
//   xori 1).  No callers.
bool type_info::operator!=(const type_info &arg) const
{
    return !operator==(arg);
}

// FUNC 0x0027C9C0 type_info::name() const
//   Out-of-line copy of the inline accessor: returns _name (+0).  No callers.
const char *type_info::name() const
{
    return _name;
}

// FUNC 0x0027C9C8 type_info::type_info(char const *)
//   Out-of-line copy of the inline ctor: _name@0 = n, vptr@4 = 0x003484C8;
//   returns this.  No callers.
type_info::type_info(const char *n)
    : _name(n)
{
}
#endif

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0027C9E0 __tf9type_info
//   Entry 0 of _vt$9type_info.  type_info has no bases, so if __ti9type_info
//   (0x00353518, .sbss) is unbuilt (word 0 == 0) it is made a
//   __user_type_info: __rtti_user(ti, "9type_info" (0x00348540)).  Returns
//   &__ti9type_info.  Base typeinfo of every *_type_info class, and the
//   static type ("sub") argument of every dynamic_cast in tinfo2.cc.
extern "C" const type_info &__tf9type_info(void)
{
    if (__ti9type_info[0] == 0)
        __rtti_user(__ti9type_info, "9type_info");
    return *(const type_info *)__ti9type_info;
}
#endif
