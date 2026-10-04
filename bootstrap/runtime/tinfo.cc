/* tinfo.cc - tinfo.o of the GCC 2.9x-ee (egcs 1.1 era) C++ runtime linked into
 * the GT3 US bootstrap (BOOTSTRAP_SCUS-97102), 0x01019BA0..0x0101A26F.
 *
 * Source order (egcs gcc/cp/tinfo.cc): ~type_info, type_info::operator==,
 * __rtti_class, __rtti_si, __rtti_user, the three dcast methods; then the
 * out-of-line copies of the header's inline members and the compiler-generated
 * typeinfo functions / implicit destructors (#pragma implementation "typeinfo"):
 * __class_type_info, __si_type_info, __user_type_info, bad_typeid, bad_cast,
 * type_info.  type_info::before (0x0101A270) is the first function of
 * tinfo2.o (its FDE follows a fresh CIE at 0x0103CE00, right after tinfo2.o's
 * .data), so it lives in tinfo2.cc.
 *
 * Class layouts, vtables and the __tf/__ti/dynamic_cast conventions are
 * documented in runtime/tinfo.h.
 *
 * .rodata of tinfo.o: vtables __class 0x01040260, __si 0x01040278,
 * __user 0x01040290, bad_typeid 0x010402A8, bad_cast 0x010402C0,
 * type_info 0x010402D8; names "17__class_type_info" 0x010402E8,
 * "14__si_type_info" 0x01040300, "16__user_type_info" 0x01040318,
 * "10bad_typeid" 0x01040330, "8bad_cast" 0x01040340, "9type_info" 0x01040350,
 * plus an unreferenced "bad_alloc" 0x01040360 (inline what() literal from <new>).
 */
#include "types.h"
#include "libc.h"
#include "new.h"
#include "tinfo.h"

/* Typeinfo objects built by the __tf functions of this file (raw storage,
 * placement-constructed on first use; word 0 = _name). */
void *__ti9type_info[2];            /* @0x01041EC0 (.sbss)  __user_type_info  */
void *__ti16__user_type_info[3];    /* @0x0104DDF8 (.bss)   __si_type_info    */
void *__ti14__si_type_info[3];      /* @0x0104DE08 (.bss)   __si_type_info    */
void *__ti17__class_type_info[3];   /* @0x0104DE18 (.bss)   __si_type_info    */
void *__ti10bad_typeid[3];          /* @0x0104DDD8 (.bss)   __si_type_info    */
void *__ti8bad_cast[3];             /* @0x0104DDE8 (.bss)   __si_type_info    */

// FUNC 0x01019BA0 type_info::~type_info()
//   Virtual dtor, _vt$9type_info entry 1 (@0x010402E4).  (this, flags): stores
//   type_info's vtable 0x010402D8 into the vptr at this+4, then
//   operator delete(this) (0x01019910) if (flags & 1).  Every *_type_info
//   destructor calls this with (this, flags) unchanged and returns.
type_info::~type_info()
{
}

// FUNC 0x01019BD0 type_info::operator==(type_info const &) const
//   Same object, or same mangled name (strcmp 0x0101B380) - the name compare is
//   what makes typeinfo objects built in different places (every __tf function
//   has its own copy for template/local classes) compare equal.
//   Used by all dcast methods, operator!= and the EH matcher.
bool type_info::operator==(const type_info &arg) const
{
    return (&arg == this) || (strcmp(name(), arg.name()) == 0);
}

// FUNC 0x01019C10 __rtti_class
//   extern "C" builder for compiler-generated __tf functions of classes with
//   multiple/virtual bases: placement-new a __class_type_info at addr (ctor
//   inlined: _name@0 = name, vptr@4 = 0x01040260, base_list@8 = bl,
//   n_bases@0xC = bn), skipped when addr == 0.  Returns nothing.  No callers.
//   (Mapping pass had this as the __class_type_info ctor; that is 0x01019F18.)
extern "C" void __rtti_class(void *addr, const char *name,
                             const __class_type_info::base_info *bl, size_t bn)
{
    /* egcs: new (addr) __class_type_info (name, bl, bn);  placement new is
     * throw(), so the compiler tests addr before running the ctor. */
    if (addr)
        new (addr) __class_type_info(name, bl, bn);
}

// FUNC 0x01019C38 __rtti_si
//   extern "C" builder called by every compiler-generated __tf function of a
//   single-inheritance class (23 callers: game __tf functions, __tf9bad_alloc,
//   __tf13bad_exception, the __tf*_type_info of this unit ...).  Placement-new
//   a __si_type_info at addr (ctor inlined: _name@0 = n, vptr@4 = 0x01040278,
//   base@8 = ti), skipped when addr == 0.  Returns nothing.
//   (Mapping pass / other units call this "__si_type_info ctor"; that is 0x01019FA8.)
extern "C" void __rtti_si(void *addr, const char *n, const type_info *ti)
{
    if (addr)
        new (addr) __si_type_info(n, static_cast<const __user_type_info &>(*ti));
}

// FUNC 0x01019C58 __rtti_user
//   extern "C" builder for __tf functions of classes without bases (7 callers:
//   __tf13FileName_Base, pdistd __tf functions, __tf9exception, __tf9type_info
//   ...).  Placement-new a __user_type_info at addr (_name@0 = name,
//   vptr@4 = 0x01040290), skipped when addr == 0.  Returns nothing.
//   (Mapping pass: "__user_type_info ctor"; that is 0x0101A038.)
extern "C" void __rtti_user(void *addr, const char *name)
{
    if (addr)
        new (addr) __user_type_info(name);
}

// FUNC 0x01019C78 __user_type_info::dcast(type_info const &, int, void *, type_info const *, void *) const
//   _vt$16__user_type_info entry 2 (@0x010402A4).  A class without bases can
//   only be converted to itself: (*this == to) ? objptr : 0 (movz).
void *__user_type_info::dcast(const type_info &to, int, void *addr,
                              const type_info *, void *) const
{
    return (*this == to) ? addr : 0;
}

// FUNC 0x01019CA8 __si_type_info::dcast(type_info const &, int, void *, type_info const *, void *) const
//   _vt$14__si_type_info entry 2 (@0x0104028C).  Match self, else recurse into
//   the single base (this+8) through ITS virtual dcast (entry 2: lh delta
//   @vtbl+0x10, lw pfn @vtbl+0x14) with all five arguments unchanged.
void *__si_type_info::dcast(const type_info &to, int require_public, void *addr,
                            const type_info *sub, void *subptr) const
{
    if (*this == to)
        return addr;
    return base.dcast(to, require_public, addr, sub, subptr);
}

// FUNC 0x01019D40 __class_type_info::dcast(type_info const &, int, void *, type_info const *, void *) const
//   _vt$17__class_type_info entry 2 (@0x01040274).  Walks base_list[0..n_bases)
//   (i unsigned, sltu): skips non-public bases when is_public; base address =
//   objptr + (word4 & 0x1FFFFFFF), loaded through if is_virtual (bit 29) - NO
//   test of objptr == 0 (gcc 2.95's tinfo.cc guards this); recurses via
//   the base's virtual dcast.  Two different hits are resolved with the
//   sub/subptr protocol (two more virtual dcast calls on `desired` with
//   require_public 1 and sub/subptr 0), otherwise the cast is ambiguous -> 0.
//   Unused in this binary (no __class_type_info objects are ever built).
void *__class_type_info::dcast(const type_info &desired, int is_public,
                               void *objptr, const type_info *sub,
                               void *subptr) const
{
    if (*this == desired)
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

// FUNC 0x01019F18 __class_type_info::__class_type_info(char const *, base_info const *, unsigned int)
//   Out-of-line copy of the inline ctor: _name@0 = name, base_list@8 = bl,
//   vptr@4 = 0x01040260, n_bases@0xC = bn; returns this.  No null test, no
//   callers.  (Mapping pass had this as __rtti_class; that is 0x01019C10.)
__class_type_info::__class_type_info(const char *name, const base_info *bl, size_t bn)
    : __user_type_info(name), base_list(bl), n_bases(bn)
{
}

// FUNC 0x01019F38 __tf17__class_type_info (compiler-generated)
//   _vt$17__class_type_info entry 0 (@0x01040264).  Builds __ti17__class_type_info
//   (0x0104DE18) once: __tf16__user_type_info() (0x0101A050), then
//   __rtti_si(ti, "17__class_type_info" (0x010402E8), &__ti16__user_type_info
//   (0x0104DDF8)).  Returns &__ti17__class_type_info.
extern "C" const type_info &__tf17__class_type_info(void)
{
    return egcs_tf_si(__ti17__class_type_info, "17__class_type_info",
                      __tf16__user_type_info, __ti16__user_type_info);
}

// FUNC 0x01019F88 __class_type_info::~__class_type_info()
//   Implicit virtual dtor, entry 1 (@0x0104026C): calls type_info::~type_info
//   (0x01019BA0) with (this, flags) unchanged; that one does the delete.
__class_type_info::~__class_type_info()
{
}

// FUNC 0x01019FA8 __si_type_info::__si_type_info(char const *, __user_type_info const &)
//   Out-of-line copy of the inline ctor: _name@0 = n, base@8 = b,
//   vptr@4 = 0x01040278; returns this.  No callers.
__si_type_info::__si_type_info(const char *n, const __user_type_info &b)
    : __user_type_info(n), base(b)
{
}

// FUNC 0x01019FC8 __tf14__si_type_info (compiler-generated)
//   Entry 0 of _vt$14__si_type_info (@0x0104027C).  Builds __ti14__si_type_info
//   (0x0104DE08) once: __tf16__user_type_info(), then __rtti_si(ti,
//   "14__si_type_info" (0x01040300), &__ti16__user_type_info).
extern "C" const type_info &__tf14__si_type_info(void)
{
    return egcs_tf_si(__ti14__si_type_info, "14__si_type_info",
                      __tf16__user_type_info, __ti16__user_type_info);
}

// FUNC 0x0101A018 __si_type_info::~__si_type_info()
//   Implicit virtual dtor, entry 1 (@0x01040284): forwards (this, flags) to
//   type_info::~type_info.
__si_type_info::~__si_type_info()
{
}

// FUNC 0x0101A038 __user_type_info::__user_type_info(char const *)
//   Out-of-line copy of the inline ctor: _name@0 = n, vptr@4 = 0x01040290;
//   returns this.  No callers.
__user_type_info::__user_type_info(const char *n)
    : type_info(n)
{
}

// FUNC 0x0101A050 __tf16__user_type_info (compiler-generated)
//   Entry 0 of _vt$16__user_type_info (@0x01040294).  Builds
//   __ti16__user_type_info (0x0104DDF8) once: __tf9type_info() (0x0101A230),
//   then __rtti_si(ti, "16__user_type_info" (0x01040318), &__ti9type_info
//   (0x01041EC0)).  Also called by the __class/__si typeinfo functions and, as
//   the dynamic_cast target, by the EH matcher.
extern "C" const type_info &__tf16__user_type_info(void)
{
    return egcs_tf_si(__ti16__user_type_info, "16__user_type_info",
                      __tf9type_info, __ti9type_info);
}

// FUNC 0x0101A0A0 __user_type_info::~__user_type_info()
//   Implicit virtual dtor, entry 1 (@0x0104029C): forwards (this, flags) to
//   type_info::~type_info.
__user_type_info::~__user_type_info()
{
}

// FUNC 0x0101A0C0 bad_typeid::~bad_typeid()
//   Implicit virtual dtor, _vt$10bad_typeid entry 1 (@0x010402B4).  The
//   inlined ~exception stores 0x010400C8 into the vptr (+0), then
//   operator delete(this) if (flags & 1).  Also the cleanup function that
//   __throw_bad_typeid (0x01018630) passes to __cp_push_exception.
bad_typeid::~bad_typeid()
{
}

// FUNC 0x0101A0F0 bad_typeid::bad_typeid()
//   Out-of-line copy of the inline ctor: vptr (+0) = 0x010402A8 (the exception
//   vptr store is dead and dropped); returns this.  No callers
//   (__throw_bad_typeid 0x01018630 inlines the vptr store instead).
bad_typeid::bad_typeid()
{
}

// FUNC 0x0101A108 __tf10bad_typeid (compiler-generated)
//   Entry 0 of _vt$10bad_typeid (@0x010402AC).  Builds __ti10bad_typeid
//   (0x0104DDD8) once: __tf9exception() (0x010187F0), then __rtti_si(ti,
//   "10bad_typeid" (0x01040330), &__ti9exception (0x01041EB8)).
extern "C" const type_info &__tf10bad_typeid(void)
{
    return egcs_tf_si(__ti10bad_typeid, "10bad_typeid",
                      __tf9exception, __ti9exception);
}

/* Modern-vtable stand-in for entry 0 (egcs stores __tf10bad_typeid itself
 * there; see runtime/new.h).  No address of its own in the binary. */
const type_info &bad_typeid::__tf() const
{
    return __tf10bad_typeid();
}

// FUNC 0x0101A158 bad_cast::~bad_cast()
//   Implicit virtual dtor, _vt$8bad_cast entry 1 (@0x010402CC): vptr (+0) =
//   0x010400C8 (inlined ~exception), operator delete(this) if (flags & 1).
//   Cleanup function passed by __throw_bad_cast (0x010185B0).
bad_cast::~bad_cast()
{
}

// FUNC 0x0101A188 bad_cast::bad_cast()
//   Out-of-line copy of the inline ctor: vptr (+0) = 0x010402C0; returns this.
//   No callers (__throw_bad_cast 0x010185B0 inlines the vptr store).
bad_cast::bad_cast()
{
}

// FUNC 0x0101A1A0 __tf8bad_cast (compiler-generated)
//   Entry 0 of _vt$8bad_cast (@0x010402C4).  Builds __ti8bad_cast (0x0104DDE8)
//   once: __tf9exception(), then __rtti_si(ti, "8bad_cast" (0x01040340),
//   &__ti9exception).
extern "C" const type_info &__tf8bad_cast(void)
{
    return egcs_tf_si(__ti8bad_cast, "8bad_cast", __tf9exception, __ti9exception);
}

/* Modern-vtable stand-in for entry 0 (egcs: __tf8bad_cast itself). */
const type_info &bad_cast::__tf() const
{
    return __tf8bad_cast();
}

// FUNC 0x0101A1F0 type_info::operator!=(type_info const &) const
//   Out-of-line copy of the inline `!operator==(arg)` (calls 0x01019BD0, xori 1).
//   No callers.
bool type_info::operator!=(const type_info &arg) const
{
    return !operator==(arg);
}

// FUNC 0x0101A210 type_info::name() const
//   Out-of-line copy of the inline accessor: returns _name (+0).  No callers.
const char *type_info::name() const
{
    return _name;
}

// FUNC 0x0101A218 type_info::type_info(char const *)
//   Out-of-line copy of the inline ctor: _name@0 = n, vptr@4 = 0x010402D8;
//   returns this.  No callers.
type_info::type_info(const char *n)
    : _name(n)
{
}

// FUNC 0x0101A230 __tf9type_info (compiler-generated)
//   Entry 0 of _vt$9type_info (@0x010402DC).  type_info has no bases, so if
//   __ti9type_info (0x01041EC0, .sbss) is unbuilt (word 0 == 0) it is made a
//   __user_type_info: __rtti_user(ti, "9type_info" (0x01040350)).  Returns
//   &__ti9type_info.  Base typeinfo of every *_type_info class, and the static
//   type ("sub") argument of every dynamic_cast in tinfo2.cc.
extern "C" const type_info &__tf9type_info(void)
{
    if (__ti9type_info[0] == 0)
        __rtti_user(__ti9type_info, "9type_info");
    return *(const type_info *)__ti9type_info;
}
