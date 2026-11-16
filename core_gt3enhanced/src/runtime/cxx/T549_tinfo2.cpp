/* T549_tinfo2.cpp - tinfo2.cc of the GCC 2.9x-ee C++ runtime,
 * 0x0027CA20..0x0027D79F.  Same machine code as bootstrap runtime/tinfo2.cc.
 * Checked: .text, .rodata and the .data layout of this object are those of
 * tinfo2.o in the toolchain's own libgcc.a (sdk/ee-gcc-2.9-991111b).
 *
 * Source order (egcs gcc/cp/tinfo2.cc): type_info::before, the EH catch
 * matcher __throw_type_match_rtti_2, __is_pointer, the __rtti_ptr/attr/func/
 * ptmf/ptmd/array builders, __dynamic_cast, the 17 builtin typeinfo
 * functions (BUILTIN macro), then the compiler-generated typeinfo function +
 * implicit destructor pairs of the file-local classes, in REVERSE
 * declaration order (__array ... __pointer).
 *
 * Callers from outside: __throw_type_match_rtti_2 <- __cplus_type_matcher
 * (0x00279DD0) and __check_eh_spec (0x00279FC8); __is_pointer <-
 * __cp_pop_exception (0x00279ED0); __dynamic_cast <- 52 game functions
 * (dynamic_cast in game code).  __rtti_ptr / attr / func / ptmf / ptmd /
 * array have no callers, so no pointer, cv, function, member-pointer or array
 * typeinfo object ever exists and the matcher's pointer branch cannot be
 * reached.
 *
 * Data: builtin objects 0x00306DA0..0x00306E27 (.data), <new>'s "bad_alloc"
 * 0x00348550 (unreferenced: the .rodata's DATA_START), builtin names
 * 0x00348560..0x003485E7, vtables 0x003485E8..0x00348657, class names
 * "17__array_type_info" 0x00348658 .. "19__pointer_type_info" 0x003486E8,
 * __ti storage of the local classes 0x004B2BA0..0x004B2C0B (.bss).
 *
 * The egcs source's dynamic_cast<const T *>(p) expressions are written as
 * TINFO2_DYNAMIC_CAST(T, p, __tf<T>) (cxx_runtime_internal.h): the operator itself under
 * egcs, its GCC 2.9x lowering for GCC 14.  The typeinfo functions and
 * destructors of the local classes are compiler-generated (#ifndef
 * BOOT_OLD_GCC); the builtin ones are written out in tinfo2.cc itself
 * (BUILTIN macro).
 */
// DATA_START 0x00348550
/* egcs: the local classes as tinfo2.cc has them, without destructor
 * declarations (T549_tinfo2.h). */
#define T549_TINFO2_LOCAL_CLASSES
#include "runtime/cxx/cxx_runtime_internal.h"
#include "runtime/cxx/T549_tinfo2.h"

// FUNC 0x0027CA20 type_info::before(type_info const &) const
//   Collation order: strcmp(_name, arg._name) < 0 (sign bit, srl 31).  No
//   callers.
bool type_info::before(const type_info &arg) const
{
    return strcmp(name(), arg.name()) < 0;
}

// FUNC 0x0027CA48 __throw_type_match_rtti_2
//   Can an exception whose type is *throw_type_r be caught by a handler for
//   *catch_type_r?  Returns the (adjusted) object pointer, or 0.
//   1. identical types (operator==)            -> objptr
//   2. thrown class type (dynamic_cast to __user_type_info succeeds)
//                                              -> its dcast(catch_type, 1, objptr)
//      (require_public 1: no conversion to private bases)
//   3. thrown pointer type: catch type must be a pointer too; strip one
//      __attr_type_info level on each pointee (cv: 1 const, 2 volatile); the
//      handler may not be LESS qualified (signed slt compares); then
//        same pointee                          -> objptr
//        catch void* and pointee not a function type -> objptr
//        pointee is a class                    -> dcast(*subto, 1, objptr)
//        pointee is a pointer                  -> multi-level loop: every level
//          needs the target at least as qualified, and while all previous
//          target levels were const (constp) the qualifications may differ,
//          otherwise they must be equal; stop at equal types.
//   Name: this is egcs 1.1's 3-argument __throw_type_match_rtti (returns the
//   adjusted pointer or 0, uses dcast); gcc 2.95's _2 takes a 4th argument.
//   The _2 spelling follows include/cxxrt.h.  The "p->dcast(x, 1, objptr,
//   0, 0)" blocks of cases 2 and 3 are shared in the binary.
extern "C" void *
__throw_type_match_rtti_2(const void *catch_type_r, const void *throw_type_r,
                          void *objptr)
{
    const type_info &catch_type = *(const type_info *)catch_type_r;
    const type_info &throw_type = *(const type_info *)throw_type_r;

    if (catch_type == throw_type)  /* implicit: type_info::operator== */
        return objptr;

    void *new_objptr = 0;       /* kept at sp+0 in the binary */

    if (const __user_type_info *p                 /* implicit: __dynamic_cast (egcs_dynamic_cast, inline) */
        = TINFO2_DYNAMIC_CAST(__user_type_info, &throw_type, __tf16__user_type_info)) {
        /* The 1 skips conversions to private bases. */
        new_objptr = p->dcast(catch_type, 1, objptr);
    } else if (const __pointer_type_info *fr
               = TINFO2_DYNAMIC_CAST(__pointer_type_info, &throw_type,
                                     __tf19__pointer_type_info)) {
        const __pointer_type_info *to
            = TINFO2_DYNAMIC_CAST(__pointer_type_info, &catch_type,
                                  __tf19__pointer_type_info);

        if (!to)
            goto fail;

        const type_info *subfr = &fr->type, *subto = &to->type;
        __attr_type_info::cv cvfrom, cvto;

        if (const __attr_type_info *at
            = TINFO2_DYNAMIC_CAST(__attr_type_info, subfr, __tf16__attr_type_info)) {
            cvfrom = at->attr;
            subfr = &at->type;
        } else
            cvfrom = __attr_type_info::NONE;

        if (const __attr_type_info *at
            = TINFO2_DYNAMIC_CAST(__attr_type_info, subto, __tf16__attr_type_info)) {
            cvto = at->attr;
            subto = &at->type;
        } else
            cvto = __attr_type_info::NONE;

        if (((cvfrom & __attr_type_info::CONST)
             > (cvto & __attr_type_info::CONST))
            || ((cvfrom & __attr_type_info::VOLATILE)
                > (cvto & __attr_type_info::VOLATILE)))
            goto fail;

        if (*subto == *subfr)
            new_objptr = objptr;
        else if (*subto == __tfv()      /* typeid (void) */
                 && TINFO2_DYNAMIC_CAST(__func_type_info, subfr,
                                        __tf16__func_type_info) == 0)
            new_objptr = objptr;
        else if (const __user_type_info *p
                 = TINFO2_DYNAMIC_CAST(__user_type_info, subfr, __tf16__user_type_info))
            new_objptr = p->dcast(*subto, 1, objptr);
        else if (const __pointer_type_info *pfr
                 = TINFO2_DYNAMIC_CAST(__pointer_type_info, subfr,
                                       __tf19__pointer_type_info)) {
            /* Multi-level pointer conversion. */

            const __pointer_type_info *pto
                = TINFO2_DYNAMIC_CAST(__pointer_type_info, subto,
                                      __tf19__pointer_type_info);

            if (!pto)
                goto fail;

            /* $s4 in the binary: computed together with the first-level
             * const check above. */
            bool constp = (cvto & __attr_type_info::CONST);
            for (subto = &pto->type, subfr = &pfr->type; ;
                 subto = &pto->type, subfr = &pfr->type) {
                if (const __attr_type_info *at
                    = TINFO2_DYNAMIC_CAST(__attr_type_info, subfr,
                                          __tf16__attr_type_info)) {
                    cvfrom = at->attr;
                    subfr = &at->type;
                } else
                    cvfrom = __attr_type_info::NONE;

                if (const __attr_type_info *at
                    = TINFO2_DYNAMIC_CAST(__attr_type_info, subto,
                                          __tf16__attr_type_info)) {
                    cvto = at->attr;
                    subto = &at->type;
                } else
                    cvto = __attr_type_info::NONE;

                if (((cvfrom & __attr_type_info::CONST)
                     > (cvto & __attr_type_info::CONST))
                    || ((cvfrom & __attr_type_info::VOLATILE)
                        > (cvto & __attr_type_info::VOLATILE)))
                    goto fail;

                if (!constp
                    && (((cvfrom & __attr_type_info::CONST)
                         < (cvto & __attr_type_info::CONST))
                        || ((cvfrom & __attr_type_info::VOLATILE)
                            < (cvto & __attr_type_info::VOLATILE))))
                    goto fail;

                if (*subto == *subfr) {
                    new_objptr = objptr;
                    break;
                }

                /* both casts are evaluated before either is tested */
                pto = TINFO2_DYNAMIC_CAST(__pointer_type_info, subto,
                                          __tf19__pointer_type_info);
                pfr = TINFO2_DYNAMIC_CAST(__pointer_type_info, subfr,
                                          __tf19__pointer_type_info);
                if (!pto || !pfr)
                    goto fail;

                if (!(cvto & __attr_type_info::CONST))
                    constp = false;         /* movn $s4, $zero */
            }
        }
    }
fail:
    return new_objptr;
}

// FUNC 0x0027CF10 __is_pointer
//   Called from __cp_pop_exception (0x00279ED0): is this type_info node a
//   pointer type?  p == 0 -> 0, else __dynamic_cast(<p's __tf>,
//   __tf19__pointer_type_info, 1, p + delta, __tf9type_info, p) != 0.
//   (bool in egcs; include/cxxrt.h declares it int - same 0/1.)
extern "C" int __is_pointer(void *p)
{
    const type_info *node = reinterpret_cast<const type_info *>(p);
    const __pointer_type_info *pointer_node
        = TINFO2_DYNAMIC_CAST(__pointer_type_info, node, __tf19__pointer_type_info);
    /* implicit: __dynamic_cast (egcs_dynamic_cast, inline) */
    return pointer_node != 0;
}

// FUNC 0x0027CF60 __rtti_ptr
//   Builder for compiler-generated pointer typeinfo: placement-new a
//   __pointer_type_info (_name@0 = n, vptr@4 = 0x00348648, type@8 = ti) at
//   addr, skipped when addr == 0.  No callers.
extern "C" void __rtti_ptr(void *addr, const char *n, const type_info *ti)
{
    if (addr)
        new (addr) __pointer_type_info(n, *ti);
}

// FUNC 0x0027CF80 __rtti_attr
//   Builder for cv-qualified typeinfo: placement-new a __attr_type_info
//   (_name@0 = n, vptr@4 = 0x00348638, attr@0xC = attrval, type@8 = ti) at
//   addr, skipped when addr == 0.  No callers.
extern "C" void __rtti_attr(void *addr, const char *n, int attrval,
                            const type_info *ti)
{
    if (addr)
        new (addr) __attr_type_info(n, static_cast<__attr_type_info::cv>(attrval), *ti);
}

// FUNC 0x0027CFA8 __rtti_func
//   Placement-new a __func_type_info (_name@0, vptr@4 = 0x00348618) at addr
//   unless addr == 0.  No callers.
extern "C" void __rtti_func(void *addr, const char *name)
{
    if (addr)
        new (addr) __func_type_info(name);
}

// FUNC 0x0027CFC8 __rtti_ptmf
//   Placement-new a __ptmf_type_info (_name@0, vptr@4 = 0x00348608) at addr
//   unless addr == 0.  No callers.
extern "C" void __rtti_ptmf(void *addr, const char *name)
{
    if (addr)
        new (addr) __ptmf_type_info(name);
}

// FUNC 0x0027CFE8 __rtti_ptmd
//   Placement-new a __ptmd_type_info (_name@0, vptr@4 = 0x003485F8) at addr
//   unless addr == 0.  Only name + vptr: this egcs version's
//   __ptmd_type_info has no other members.  No callers.
extern "C" void __rtti_ptmd(void *addr, const char *name)
{
    if (addr)
        new (addr) __ptmd_type_info(name);
}

// FUNC 0x0027D008 __rtti_array
//   Placement-new a __array_type_info (_name@0, vptr@4 = 0x003485E8) at addr
//   unless addr == 0.  No callers.
extern "C" void __rtti_array(void *addr, const char *name)
{
    if (addr)
        new (addr) __array_type_info(name);
}

// FUNC 0x0027D028 __dynamic_cast
//   Runtime half of dynamic_cast: from() gives the typeinfo of the object's
//   dynamic type (taken by the caller from the object's vtable entry 0); it
//   is treated as a __user_type_info and its virtual dcast (slot 1: lh delta
//   @vtbl+0x10, lw pfn @vtbl+0x14) is called with (to(), require_public,
//   address, &sub(), subptr).  Call order: from(), to(), sub(), dcast.  No
//   `require_public == 0` check (gcc 2.95's version aborts there).
//   54 callers: the matcher, __is_pointer and 52 game dynamic_casts.
extern "C" void *
__dynamic_cast(const type_info &(*from)(void), const type_info &(*to)(void),
               int require_public, void *address,
               const type_info &(*sub)(void), void *subptr)
{
#ifdef BOOT_OLD_GCC
    /* tinfo2.cc's one expression: egcs evaluates the object (from()) and
     * looks up dcast in its vtable before the arguments (to(), then sub()). */
    return static_cast<const __user_type_info &>(from()).dcast
        (to(), require_public, address, &(sub()), subptr);
#else
    /* the same order, spelled out (GCC 14 may order call arguments
     * differently) */
    const __user_type_info &from_ti = static_cast<const __user_type_info &>(from());
    const type_info &to_ti = to();
    const type_info *sub_ti = &sub();
    return from_ti.dcast(to_ti, require_public, address, sub_ti, subptr);
#endif
}

/* ------------------------------------------------------------------------ */
/* Builtin types                                                            */
/* ------------------------------------------------------------------------ */

/* type_info nodes and functions for the builtin types (egcs BUILTIN macro).
 * __ti<m>: 8 bytes in .data, zero until built (declared in cxx_runtime_internal.h; egcs
 * defines them in the macro as unsigned char[sizeof (__builtin_type_info)]).
 * __tf<m>: if word 0 (_name) is 0, placement-construct a
 * __builtin_type_info("<m>") there (inlined ctor: vptr@4 = 0x00348628,
 * _name@0 = "<m>"); return it.  The binary also tests the (constant, never
 * NULL) storage address before the ctor - the placement-new null check, dead
 * here.  Only __tfv is referenced (typeid(void) in the matcher).
 * The egcs build defines the node storage in the macro too, as tinfo2.cc
 * does: zero words in this object's .data, 8-aligned (0x00306DA0..; the
 * addresses are on the declarations in cxx_runtime_internal.h).  The GCC 14 build keeps
 * them declarations. */
#ifdef BOOT_OLD_GCC
#define BUILTIN_NODE(mangled) void *__ti##mangled[2];
#else
#define BUILTIN_NODE(mangled)
#endif
#define BUILTIN(mangled)                                                \
    BUILTIN_NODE(mangled)                                               \
    extern "C" const type_info &__tf##mangled(void)                     \
    {                                                                   \
        if (__ti##mangled[0] == 0)                                      \
            new (__ti##mangled) __builtin_type_info(#mangled);          \
        return *(const type_info *)__ti##mangled;                       \
    }

// FUNC 0x0027D0D8 __tfv
//   void: __tiv @0x00306DA0, name "v" @0x00348560.  Used by the matcher's
//   catch(void *) rule.
BUILTIN(v)

// FUNC 0x0027D110 __tfx
//   long long: __tix @0x00306DA8, name "x" @0x00348568.
BUILTIN(x)

// FUNC 0x0027D148 __tfl
//   long: __til @0x00306DB0, name "l" @0x00348570.
BUILTIN(l)

// FUNC 0x0027D180 __tfi
//   int: __tii @0x00306DB8, name "i" @0x00348578.
BUILTIN(i)

// FUNC 0x0027D1B8 __tfs
//   short: __tis @0x00306DC0, name "s" @0x00348580.
BUILTIN(s)

// FUNC 0x0027D1F0 __tfb
//   bool: __tib @0x00306DC8, name "b" @0x00348588.
BUILTIN(b)

// FUNC 0x0027D228 __tfc
//   char: __tic @0x00306DD0, name "c" @0x00348590.
BUILTIN(c)

// FUNC 0x0027D260 __tfw
//   wchar_t: __tiw @0x00306DD8, name "w" @0x00348598.
BUILTIN(w)

// FUNC 0x0027D298 __tfr
//   long double: __tir @0x00306DE0, name "r" @0x003485A0.
BUILTIN(r)

// FUNC 0x0027D2D0 __tfd
//   double: __tid @0x00306DE8, name "d" @0x003485A8.
BUILTIN(d)

// FUNC 0x0027D308 __tff
//   float: __tif @0x00306DF0, name "f" @0x003485B0.
BUILTIN(f)

// FUNC 0x0027D340 __tfUi
//   unsigned int: __tiUi @0x00306DF8, name "Ui" @0x003485B8.
BUILTIN(Ui)

// FUNC 0x0027D378 __tfUl
//   unsigned long: __tiUl @0x00306E00, name "Ul" @0x003485C0.
BUILTIN(Ul)

// FUNC 0x0027D3B0 __tfUx
//   unsigned long long: __tiUx @0x00306E08, name "Ux" @0x003485C8.
BUILTIN(Ux)

// FUNC 0x0027D3E8 __tfUs
//   unsigned short: __tiUs @0x00306E10, name "Us" @0x003485D0.
BUILTIN(Us)

// FUNC 0x0027D420 __tfUc
//   unsigned char: __tiUc @0x00306E18, name "Uc" @0x003485D8.
BUILTIN(Uc)

// FUNC 0x0027D458 __tfSc
//   signed char: __tiSc @0x00306E20, name "Sc" @0x003485E0.  Last builtin.
BUILTIN(Sc)

#undef BUILTIN
#undef BUILTIN_NODE

/* ------------------------------------------------------------------------ */
/* Compiler-generated typeinfo functions / implicit destructors of the      */
/* local classes (emitted in reverse declaration order).  Every __tf here:  */
/* __tf9type_info() (0x0027C9E0), then __rtti_si(&__ti<X>, "<X>",           */
/* &__ti9type_info (0x00353518)) once; every dtor calls                     */
/* type_info::~type_info (0x0027C350) with (this, flags) unchanged.         */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
// FUNC 0x0027D490 __tf17__array_type_info
//   Entry 0 of _vt$17__array_type_info 0x003485E8.  __ti17__array_type_info
//   @0x004B2BD0, name "17__array_type_info" @0x00348658.
extern "C" const type_info &__tf17__array_type_info(void)
{
    if (__ti17__array_type_info[0] == 0) {
        __tf9type_info();  /* the base's node first (result unused) */
        __rtti_si(__ti17__array_type_info, "17__array_type_info",
                  (const type_info *)__ti9type_info);
    }
    return *(const type_info *)__ti17__array_type_info;
}
#endif

#ifndef BOOT_OLD_GCC                    /* egcs: implicit (T549_tinfo2.h) */
// FUNC 0x0027D4E0 __array_type_info::~__array_type_info() (_$_17__array_type_info)
//   Slot 0 of 0x003485E8: forwards to ~type_info.
__array_type_info::~__array_type_info()
{
    /* implicit: ~type_info (compiler-generated: bases, members, deleting form) */
}
#endif

#ifndef BOOT_OLD_GCC
// FUNC 0x0027D500 __tf16__ptmd_type_info
//   Entry 0 of _vt$16__ptmd_type_info 0x003485F8.  __ti16__ptmd_type_info
//   @0x004B2BF0, name "16__ptmd_type_info" @0x00348670.
extern "C" const type_info &__tf16__ptmd_type_info(void)
{
    if (__ti16__ptmd_type_info[0] == 0) {
        __tf9type_info();  /* the base's node first (result unused) */
        __rtti_si(__ti16__ptmd_type_info, "16__ptmd_type_info",
                  (const type_info *)__ti9type_info);
    }
    return *(const type_info *)__ti16__ptmd_type_info;
}
#endif

#ifndef BOOT_OLD_GCC                    /* egcs: implicit (T549_tinfo2.h) */
// FUNC 0x0027D550 __ptmd_type_info::~__ptmd_type_info() (_$_16__ptmd_type_info)
//   Slot 0 of 0x003485F8: forwards to ~type_info.
__ptmd_type_info::~__ptmd_type_info()
{
    /* implicit: ~type_info (compiler-generated: bases, members, deleting form) */
}
#endif

#ifndef BOOT_OLD_GCC
// FUNC 0x0027D570 __tf16__ptmf_type_info
//   Entry 0 of _vt$16__ptmf_type_info 0x00348608.  __ti16__ptmf_type_info
//   @0x004B2BC0, name "16__ptmf_type_info" @0x00348688.
extern "C" const type_info &__tf16__ptmf_type_info(void)
{
    if (__ti16__ptmf_type_info[0] == 0) {
        __tf9type_info();  /* the base's node first (result unused) */
        __rtti_si(__ti16__ptmf_type_info, "16__ptmf_type_info",
                  (const type_info *)__ti9type_info);
    }
    return *(const type_info *)__ti16__ptmf_type_info;
}
#endif

#ifndef BOOT_OLD_GCC                    /* egcs: implicit (T549_tinfo2.h) */
// FUNC 0x0027D5C0 __ptmf_type_info::~__ptmf_type_info() (_$_16__ptmf_type_info)
//   Slot 0 of 0x00348608: forwards to ~type_info.
__ptmf_type_info::~__ptmf_type_info()
{
    /* implicit: ~type_info (compiler-generated: bases, members, deleting form) */
}
#endif

#ifndef BOOT_OLD_GCC
// FUNC 0x0027D5E0 __tf16__func_type_info
//   Entry 0 of _vt$16__func_type_info 0x00348618.  __ti16__func_type_info
//   @0x004B2BA0, name "16__func_type_info" @0x003486A0.  dynamic_cast target
//   of the matcher's catch(void *) rule.
extern "C" const type_info &__tf16__func_type_info(void)
{
    if (__ti16__func_type_info[0] == 0) {
        __tf9type_info();  /* the base's node first (result unused) */
        __rtti_si(__ti16__func_type_info, "16__func_type_info",
                  (const type_info *)__ti9type_info);
    }
    return *(const type_info *)__ti16__func_type_info;
}
#endif

#ifndef BOOT_OLD_GCC                    /* egcs: implicit (T549_tinfo2.h) */
// FUNC 0x0027D630 __func_type_info::~__func_type_info() (_$_16__func_type_info)
//   Slot 0 of 0x00348618: forwards to ~type_info.
__func_type_info::~__func_type_info()
{
    /* implicit: ~type_info (compiler-generated: bases, members, deleting form) */
}
#endif

#ifndef BOOT_OLD_GCC
// FUNC 0x0027D650 __tf19__builtin_type_info
//   Entry 0 of _vt$19__builtin_type_info 0x00348628 (the vtable of all
//   builtin objects).  __ti19__builtin_type_info @0x004B2C00, name
//   "19__builtin_type_info" @0x003486B8.
extern "C" const type_info &__tf19__builtin_type_info(void)
{
    if (__ti19__builtin_type_info[0] == 0) {
        __tf9type_info();  /* the base's node first (result unused) */
        __rtti_si(__ti19__builtin_type_info, "19__builtin_type_info",
                  (const type_info *)__ti9type_info);
    }
    return *(const type_info *)__ti19__builtin_type_info;
}
#endif

#ifndef BOOT_OLD_GCC                    /* egcs: implicit (T549_tinfo2.h) */
// FUNC 0x0027D6A0 __builtin_type_info::~__builtin_type_info() (_$_19__builtin_type_info)
//   Slot 0 of 0x00348628: forwards to ~type_info.
__builtin_type_info::~__builtin_type_info()
{
    /* implicit: ~type_info (compiler-generated: bases, members, deleting form) */
}
#endif

#ifndef BOOT_OLD_GCC
// FUNC 0x0027D6C0 __tf16__attr_type_info
//   Entry 0 of _vt$16__attr_type_info 0x00348638.  __ti16__attr_type_info
//   @0x004B2BB0, name "16__attr_type_info" @0x003486D0.  dynamic_cast target
//   of the matcher's cv-stripping steps.
extern "C" const type_info &__tf16__attr_type_info(void)
{
    if (__ti16__attr_type_info[0] == 0) {
        __tf9type_info();  /* the base's node first (result unused) */
        __rtti_si(__ti16__attr_type_info, "16__attr_type_info",
                  (const type_info *)__ti9type_info);
    }
    return *(const type_info *)__ti16__attr_type_info;
}
#endif

#ifndef BOOT_OLD_GCC                    /* egcs: implicit (T549_tinfo2.h) */
// FUNC 0x0027D710 __attr_type_info::~__attr_type_info() (_$_16__attr_type_info)
//   Slot 0 of 0x00348638: forwards to ~type_info.
__attr_type_info::~__attr_type_info()
{
    /* implicit: ~type_info (compiler-generated: bases, members, deleting form) */
}
#endif

#ifndef BOOT_OLD_GCC
// FUNC 0x0027D730 __tf19__pointer_type_info
//   Entry 0 of _vt$19__pointer_type_info 0x00348648.
//   __ti19__pointer_type_info @0x004B2BE0, name "19__pointer_type_info"
//   @0x003486E8.  dynamic_cast target in the matcher and __is_pointer.
extern "C" const type_info &__tf19__pointer_type_info(void)
{
    if (__ti19__pointer_type_info[0] == 0) {
        __tf9type_info();  /* the base's node first (result unused) */
        __rtti_si(__ti19__pointer_type_info, "19__pointer_type_info",
                  (const type_info *)__ti9type_info);
    }
    return *(const type_info *)__ti19__pointer_type_info;
}
#endif

#ifndef BOOT_OLD_GCC                    /* egcs: implicit (T549_tinfo2.h) */
// FUNC 0x0027D780 __pointer_type_info::~__pointer_type_info() (_$_19__pointer_type_info)
//   Slot 0 of 0x00348648: forwards to ~type_info.
__pointer_type_info::~__pointer_type_info()
{
    /* implicit: ~type_info (compiler-generated: bases, members, deleting form) */
}
#endif
