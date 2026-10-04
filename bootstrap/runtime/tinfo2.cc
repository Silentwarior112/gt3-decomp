/* tinfo2.cc - tinfo2.o of the GCC 2.9x-ee (egcs 1.1 era) C++ runtime linked
 * into the GT3 US bootstrap (BOOTSTRAP_SCUS-97102), 0x0101A270..0x0101AFEF.
 *
 * Source order (egcs gcc/cp/tinfo2.cc): type_info::before, the EH catch
 * matcher __throw_type_match_rtti_2, __is_pointer, the __rtti_ptr/attr/func/
 * ptmf/ptmd/array builders, __dynamic_cast, the 17 builtin typeinfo
 * functions (BUILTIN macro), then the compiler-generated typeinfo function +
 * implicit destructor pairs of the file-local classes, emitted in REVERSE
 * declaration order (__array ... __pointer).
 *
 * Callers from outside: __throw_type_match_rtti_2 <- __cplus_type_matcher
 * (0x01018200) and __check_eh_spec (0x010183F8); __is_pointer <-
 * __cp_pop_exception (0x01018300).  __rtti_ptr / __rtti_attr / __rtti_func /
 * __rtti_ptmf / __rtti_ptmd / __rtti_array have no callers, so no pointer,
 * cv, function, member-pointer or array typeinfo object ever exists and the
 * matcher's pointer branch cannot be reached in this binary.
 *
 * Data of tinfo2.o:
 *   .data  0x0103CD78..0x0103CDFF  17 builtin typeinfo objects, 8 bytes each
 *                                  (zero until the matching __tf runs)
 *   .rodata 0x01040370..0x010403F7 builtin names "v" "x" "l" "i" "s" "b" "c"
 *                                  "w" "r" "d" "f" "Ui" "Ul" "Ux" "Us" "Uc" "Sc",
 *                                  8-byte aligned
 *   .rodata 0x010403F8..0x01040467 vtables (see the classes below)
 *   .rodata 0x01040468..0x0104050F class names "17__array_type_info" ...
 *   .bss   0x0104DE28..0x0104DE93  __ti storage of the local classes
 *
 * The egcs source's dynamic_cast<const T *>(p) expressions are written out as
 * egcs_dynamic_cast<T>(p, __tf<T>) (below), the GCC 2.9x lowering the binary
 * contains: after a p == 0 test, a call of __dynamic_cast with p's own __tf
 * taken from vtable entry 0, &__tf<T>, require_public 1, p + entry-0 delta,
 * &__tf9type_info, p (see runtime/tinfo.h).  The C++ dynamic_cast operator is
 * NOT used: g++ 3+ lowers it to the Itanium-ABI __dynamic_cast(src, src_type,
 * dst_type, hint), which binds to the egcs extern "C" __dynamic_cast defined
 * in this file and would call it with the wrong arguments.
 */
#include "types.h"
#include "libc.h"
#include "cxxrt.h"
#include "new.h"
#include "tinfo.h"

/* ------------------------------------------------------------------------ */
/* File-local typeinfo classes (egcs declares them in tinfo2.cc)            */
/* ------------------------------------------------------------------------ */

/* type info for pointer type.  sizeof == 0xC: +0x08 const type_info &type
 * (pointee).  vtable @0x01040458: [0] __tf19__pointer_type_info 0x0101AF80,
 * [1] ~__pointer_type_info 0x0101AFD0. */
struct __pointer_type_info : public type_info {
    const type_info &type;                          /* +0x08 */

    __pointer_type_info(const char *n, const type_info &ti)
        : type_info(n), type(ti) {}
    virtual ~__pointer_type_info();
};

/* type info for cv-qualifiers.  sizeof == 0x10: +0x08 const type_info &type
 * (unqualified type), +0x0C cv attr.  vtable @0x01040448: [0]
 * __tf16__attr_type_info 0x0101AF10, [1] ~__attr_type_info 0x0101AF60. */
struct __attr_type_info : public type_info {
    enum cv { NONE = 0, CONST = 1, VOLATILE = 2, CONSTVOL = 1 | 2 };

    const type_info &type;                          /* +0x08 */
    cv attr;                                        /* +0x0C */

    __attr_type_info(const char *n, cv a, const type_info &t)
        : type_info(n), type(t), attr(a) {}
    virtual ~__attr_type_info();
};

/* type_info for builtin types.  sizeof == 8.  vtable @0x01040438 (used by all
 * 17 builtin objects): [0] __tf19__builtin_type_info 0x0101AEA0,
 * [1] ~__builtin_type_info 0x0101AEF0. */
struct __builtin_type_info : public type_info {
    __builtin_type_info(const char *n) : type_info(n) {}
    virtual ~__builtin_type_info();
};

/* type info for function.  vtable @0x01040428: [0] __tf16__func_type_info
 * 0x0101AE30, [1] ~__func_type_info 0x0101AE80. */
struct __func_type_info : public type_info {
    __func_type_info(const char *n) : type_info(n) {}
    virtual ~__func_type_info();
};

/* type info for pointer to member function.  vtable @0x01040418:
 * [0] __tf16__ptmf_type_info 0x0101ADC0, [1] ~__ptmf_type_info 0x0101AE10. */
struct __ptmf_type_info : public type_info {
    __ptmf_type_info(const char *n) : type_info(n) {}
    virtual ~__ptmf_type_info();
};

/* type info for pointer to data member.  Name only (no class/member type
 * fields in this egcs version).  vtable @0x01040408: [0]
 * __tf16__ptmd_type_info 0x0101AD50, [1] ~__ptmd_type_info 0x0101ADA0. */
struct __ptmd_type_info : public type_info {
    __ptmd_type_info(const char *n) : type_info(n) {}
    virtual ~__ptmd_type_info();
};

/* type info for array.  vtable @0x010403F8: [0] __tf17__array_type_info
 * 0x0101ACE0, [1] ~__array_type_info 0x0101AD30. */
struct __array_type_info : public type_info {
    __array_type_info(const char *n) : type_info(n) {}
    virtual ~__array_type_info();
};

#if defined(__cplusplus) && __cplusplus >= 201103L
static_assert(sizeof(__pointer_type_info) == 0xC, "__pointer_type_info: name, vptr, type&");
static_assert(sizeof(__attr_type_info) == 0x10, "__attr_type_info: name, vptr, type&, attr");
static_assert(sizeof(__builtin_type_info) == 8, "builtin typeinfo objects are 8 bytes");
#endif

extern "C" {
const type_info &__tf19__pointer_type_info(void);   /* 0x0101AF80 */
const type_info &__tf16__attr_type_info(void);      /* 0x0101AF10 */
const type_info &__tf19__builtin_type_info(void);   /* 0x0101AEA0 */
const type_info &__tf16__func_type_info(void);      /* 0x0101AE30 */
const type_info &__tf16__ptmf_type_info(void);      /* 0x0101ADC0 */
const type_info &__tf16__ptmd_type_info(void);      /* 0x0101AD50 */
const type_info &__tf17__array_type_info(void);     /* 0x0101ACE0 */
}

/* GCC 2.9x vtable entry: {s16 delta; s16 index; pfn}.  Entry 0's pfn is the
 * class's __tf typeinfo function. */
struct egcs_vtable_entry {
    s16 delta;                                      /* +0x00 (lh) */
    s16 index;                                      /* +0x02 */
    const type_info &(*pfn)(void);                  /* +0x04 (lw) */
};

/* dynamic_cast<const T *>(p) for a p of static type `const type_info *`, as
 * GCC 2.9x emits it inline at every use in this file:
 *     p == 0 ? 0
 *            : __dynamic_cast(vptr(p)[0].pfn, to_tf, 1,
 *                             (char *)p + vptr(p)[0].delta, __tf9type_info, p)
 * vptr(p) is the word at p+4 (original type_info layout {_name, vptr}). */
template <class T>
static inline const T *
egcs_dynamic_cast(const type_info *p, const type_info &(*to_tf)(void))
{
    if (p == 0)
        return 0;
    const egcs_vtable_entry *vtbl
        = *(const egcs_vtable_entry *const *)((const char *)p + 4);
    return (const T *)__dynamic_cast(vtbl[0].pfn, to_tf, 1,
                                     (char *)p + vtbl[0].delta,
                                     __tf9type_info, (void *)p);
}

/* __ti storage of the local classes (__si_type_info, base type_info). */
void *__ti16__func_type_info[3];      /* @0x0104DE28 (.bss) */
void *__ti16__attr_type_info[3];      /* @0x0104DE38 (.bss) */
void *__ti16__ptmf_type_info[3];      /* @0x0104DE48 (.bss) */
void *__ti17__array_type_info[3];     /* @0x0104DE58 (.bss) */
void *__ti19__pointer_type_info[3];   /* @0x0104DE68 (.bss) */
void *__ti16__ptmd_type_info[3];      /* @0x0104DE78 (.bss) */
void *__ti19__builtin_type_info[3];   /* @0x0104DE88 (.bss) */

/* ------------------------------------------------------------------------ */

// FUNC 0x0101A270 type_info::before(type_info const &) const
//   Collation order: strcmp(_name, arg._name) < 0 (sign bit, srl 31).  No callers.
bool type_info::before(const type_info &arg) const
{
    return strcmp(name(), arg.name()) < 0;
}

// FUNC 0x0101A298 __throw_type_match_rtti_2
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
//   Callers: __cplus_type_matcher (0x01018200), __check_eh_spec (0x010183F8).
//   Name: this is egcs 1.1's 3-argument __throw_type_match_rtti (returns the
//   adjusted pointer or 0, uses dcast).  gcc 2.95's __throw_type_match_rtti_2
//   takes a 4th `void **valp` and returns int.  The _2 spelling is kept only
//   because include/cxxrt.h and runtime/exception.cc link against it.
//   The code blocks for "p->dcast(x, 1, objptr, 0, 0)" of cases 2 and 3 are
//   shared in the binary (0x0101A50C, with x = catch_type or subto in $s1).
extern "C" void *
__throw_type_match_rtti_2(const void *catch_type_r, const void *throw_type_r,
                          void *objptr)
{
    const type_info &catch_type = *(const type_info *)catch_type_r;
    const type_info &throw_type = *(const type_info *)throw_type_r;

    if (catch_type == throw_type)
        return objptr;

    void *new_objptr = 0;       /* kept at sp+0 in the binary */

    if (const __user_type_info *p
        = egcs_dynamic_cast<__user_type_info>(&throw_type, __tf16__user_type_info)) {
        /* The 1 skips conversions to private bases. */
        new_objptr = p->dcast(catch_type, 1, objptr);
    } else if (const __pointer_type_info *fr
               = egcs_dynamic_cast<__pointer_type_info>(&throw_type,
                                                        __tf19__pointer_type_info)) {
        const __pointer_type_info *to
            = egcs_dynamic_cast<__pointer_type_info>(&catch_type,
                                                     __tf19__pointer_type_info);

        if (!to)
            goto fail;

        const type_info *subfr = &fr->type, *subto = &to->type;
        __attr_type_info::cv cvfrom, cvto;

        if (const __attr_type_info *at
            = egcs_dynamic_cast<__attr_type_info>(subfr, __tf16__attr_type_info)) {
            cvfrom = at->attr;
            subfr = &at->type;
        } else
            cvfrom = __attr_type_info::NONE;

        if (const __attr_type_info *at
            = egcs_dynamic_cast<__attr_type_info>(subto, __tf16__attr_type_info)) {
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
                 && egcs_dynamic_cast<__func_type_info>(subfr,
                                                        __tf16__func_type_info) == 0)
            new_objptr = objptr;
        else if (const __user_type_info *p
                 = egcs_dynamic_cast<__user_type_info>(subfr, __tf16__user_type_info))
            new_objptr = p->dcast(*subto, 1, objptr);
        else if (const __pointer_type_info *pfr
                 = egcs_dynamic_cast<__pointer_type_info>(subfr,
                                                          __tf19__pointer_type_info)) {
            /* Multi-level pointer conversion. */

            const __pointer_type_info *pto
                = egcs_dynamic_cast<__pointer_type_info>(subto,
                                                         __tf19__pointer_type_info);

            if (!pto)
                goto fail;

            /* $s4 in the binary: computed together with the first-level
             * const check above. */
            bool constp = (cvto & __attr_type_info::CONST);
            for (subto = &pto->type, subfr = &pfr->type; ;
                 subto = &pto->type, subfr = &pfr->type) {
                if (const __attr_type_info *at
                    = egcs_dynamic_cast<__attr_type_info>(subfr,
                                                          __tf16__attr_type_info)) {
                    cvfrom = at->attr;
                    subfr = &at->type;
                } else
                    cvfrom = __attr_type_info::NONE;

                if (const __attr_type_info *at
                    = egcs_dynamic_cast<__attr_type_info>(subto,
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
                pto = egcs_dynamic_cast<__pointer_type_info>(subto,
                                                             __tf19__pointer_type_info);
                pfr = egcs_dynamic_cast<__pointer_type_info>(subfr,
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

// FUNC 0x0101A760 __is_pointer
//   Called from __cp_pop_exception (0x01018300): is this type_info node a
//   pointer type?  p == 0 -> 0, else __dynamic_cast(<p's __tf>,
//   __tf19__pointer_type_info, 1, p + delta, __tf9type_info, p) != 0.
//   (bool in egcs; the header include/cxxrt.h declares it int - same 0/1.)
extern "C" int __is_pointer(void *p)
{
    const type_info *t = reinterpret_cast<const type_info *>(p);
    const __pointer_type_info *pt
        = egcs_dynamic_cast<__pointer_type_info>(t, __tf19__pointer_type_info);
    return pt != 0;
}

// FUNC 0x0101A7B0 __rtti_ptr
//   Builder for compiler-generated pointer typeinfo: placement-new a
//   __pointer_type_info (_name@0 = n, vptr@4 = 0x01040458, type@8 = ti) at
//   addr, skipped when addr == 0.  No callers.  (Mapping pass: the ctor.)
extern "C" void __rtti_ptr(void *addr, const char *n, const type_info *ti)
{
    if (addr)
        new (addr) __pointer_type_info(n, *ti);
}

// FUNC 0x0101A7D0 __rtti_attr
//   Builder for cv-qualified typeinfo: placement-new a __attr_type_info
//   (_name@0 = n, vptr@4 = 0x01040448, attr@0xC = attrval, type@8 = ti) at
//   addr, skipped when addr == 0.  No callers.  (Mapping pass: the ctor.)
extern "C" void __rtti_attr(void *addr, const char *n, int attrval,
                            const type_info *ti)
{
    if (addr)
        new (addr) __attr_type_info(n, static_cast<__attr_type_info::cv>(attrval), *ti);
}

// FUNC 0x0101A7F8 __rtti_func
//   Placement-new a __func_type_info (_name@0, vptr@4 = 0x01040428) at addr
//   unless addr == 0.  No callers.
extern "C" void __rtti_func(void *addr, const char *name)
{
    if (addr)
        new (addr) __func_type_info(name);
}

// FUNC 0x0101A818 __rtti_ptmf
//   Placement-new a __ptmf_type_info (_name@0, vptr@4 = 0x01040418) at addr
//   unless addr == 0.  No callers.
extern "C" void __rtti_ptmf(void *addr, const char *name)
{
    if (addr)
        new (addr) __ptmf_type_info(name);
}

// FUNC 0x0101A838 __rtti_ptmd
//   Placement-new a __ptmd_type_info (_name@0, vptr@4 = 0x01040408) at addr
//   unless addr == 0.  Stores only name + vptr because this egcs version's
//   __ptmd_type_info has no other members.  No callers.
extern "C" void __rtti_ptmd(void *addr, const char *name)
{
    if (addr)
        new (addr) __ptmd_type_info(name);
}

// FUNC 0x0101A858 __rtti_array
//   Placement-new a __array_type_info (_name@0, vptr@4 = 0x010403F8) at addr
//   unless addr == 0.  No callers.
extern "C" void __rtti_array(void *addr, const char *name)
{
    if (addr)
        new (addr) __array_type_info(name);
}

// FUNC 0x0101A878 __dynamic_cast
//   Runtime half of dynamic_cast: from() gives the typeinfo of the object's
//   dynamic type (taken by the caller from the object's vtable entry 0); it is
//   treated as a __user_type_info and its virtual dcast (entry 2: lh delta
//   @vtbl+0x10, lw pfn @vtbl+0x14) is called with (to(), require_public,
//   address, &sub(), subptr).  Call order in the binary: from(), to(), sub(),
//   dcast.  No `require_public == 0` check (gcc 2.95's version aborts there).
//   Callers: __throw_type_match_rtti_2, __is_pointer.
extern "C" void *
__dynamic_cast(const type_info &(*from)(void), const type_info &(*to)(void),
               int require_public, void *address,
               const type_info &(*sub)(void), void *subptr)
{
    const __user_type_info &ti = static_cast<const __user_type_info &>(from());
    const type_info &to_ti = to();
    const type_info *sub_ti = &sub();
    return ti.dcast(to_ti, require_public, address, sub_ti, subptr);
}

/* ------------------------------------------------------------------------ */
/* Builtin types                                                            */
/* ------------------------------------------------------------------------ */

/* type_info nodes and functions for the builtin types (egcs BUILTIN macro).
 * __ti<m>: 8 bytes in .data, zero until built.  __tf<m>: if word 0 (_name) is
 * 0, placement-construct a __builtin_type_info("<m>") there (inlined ctor:
 * vptr@4 = 0x01040438, _name@0 = "<m>"); return it.  The binary also tests the
 * (constant, never NULL) storage address before the ctor - the placement-new
 * null check, dead here.  Only __tfv is referenced (typeid(void) in the
 * matcher); the other 16 have no callers in this binary. */
#define BUILTIN(mangled)                                                \
    void *__ti##mangled[2];                                             \
    extern "C" const type_info &__tf##mangled(void)                     \
    {                                                                   \
        if (__ti##mangled[0] == 0)                                      \
            new (__ti##mangled) __builtin_type_info(#mangled);          \
        return *(const type_info *)__ti##mangled;                       \
    }

// FUNC 0x0101A928 __tfv
//   void: __tiv @0x0103CD78, name "v" @0x01040370.  Used by the matcher's
//   catch(void *) rule.
BUILTIN(v)

// FUNC 0x0101A960 __tfx
//   long long: __tix @0x0103CD80, name "x" @0x01040378.
BUILTIN(x)

// FUNC 0x0101A998 __tfl
//   long: __til @0x0103CD88, name "l" @0x01040380.
BUILTIN(l)

// FUNC 0x0101A9D0 __tfi
//   int: __tii @0x0103CD90, name "i" @0x01040388.
BUILTIN(i)

// FUNC 0x0101AA08 __tfs
//   short: __tis @0x0103CD98, name "s" @0x01040390.
BUILTIN(s)

// FUNC 0x0101AA40 __tfb
//   bool: __tib @0x0103CDA0, name "b" @0x01040398.
BUILTIN(b)

// FUNC 0x0101AA78 __tfc
//   char: __tic @0x0103CDA8, name "c" @0x010403A0.
BUILTIN(c)

// FUNC 0x0101AAB0 __tfw
//   wchar_t: __tiw @0x0103CDB0, name "w" @0x010403A8.
BUILTIN(w)

// FUNC 0x0101AAE8 __tfr
//   long double: __tir @0x0103CDB8, name "r" @0x010403B0.
BUILTIN(r)

// FUNC 0x0101AB20 __tfd
//   double: __tid @0x0103CDC0, name "d" @0x010403B8.
BUILTIN(d)

// FUNC 0x0101AB58 __tff
//   float: __tif @0x0103CDC8, name "f" @0x010403C0.
BUILTIN(f)

// FUNC 0x0101AB90 __tfUi
//   unsigned int: __tiUi @0x0103CDD0, name "Ui" @0x010403C8.
BUILTIN(Ui)

// FUNC 0x0101ABC8 __tfUl
//   unsigned long: __tiUl @0x0103CDD8, name "Ul" @0x010403D0.
BUILTIN(Ul)

// FUNC 0x0101AC00 __tfUx
//   unsigned long long: __tiUx @0x0103CDE0, name "Ux" @0x010403D8.
BUILTIN(Ux)

// FUNC 0x0101AC38 __tfUs
//   unsigned short: __tiUs @0x0103CDE8, name "Us" @0x010403E0.
BUILTIN(Us)

// FUNC 0x0101AC70 __tfUc
//   unsigned char: __tiUc @0x0103CDF0, name "Uc" @0x010403E8.
BUILTIN(Uc)

// FUNC 0x0101ACA8 __tfSc
//   signed char: __tiSc @0x0103CDF8, name "Sc" @0x010403F0.  Last builtin.
BUILTIN(Sc)

#undef BUILTIN

/* ------------------------------------------------------------------------ */
/* Compiler-generated typeinfo functions / implicit destructors of the       */
/* local classes (emitted in reverse declaration order).  Every __tf here:   */
/* __tf9type_info() (0x0101A230), then __rtti_si(&__ti<X>, "<X>",            */
/* &__ti9type_info (0x01041EC0)) once; every dtor calls                      */
/* type_info::~type_info (0x01019BA0) with (this, flags) unchanged.          */
/* ------------------------------------------------------------------------ */

// FUNC 0x0101ACE0 __tf17__array_type_info (compiler-generated)
//   Entry 0 of vtable 0x010403F8.  __ti17__array_type_info @0x0104DE58, name
//   "17__array_type_info" @0x01040468.
extern "C" const type_info &__tf17__array_type_info(void)
{
    return egcs_tf_si(__ti17__array_type_info, "17__array_type_info",
                      __tf9type_info, __ti9type_info);
}

// FUNC 0x0101AD30 __array_type_info::~__array_type_info()
//   Entry 1 of vtable 0x010403F8 (@0x01040404): forwards to ~type_info.
__array_type_info::~__array_type_info()
{
}

// FUNC 0x0101AD50 __tf16__ptmd_type_info (compiler-generated)
//   Entry 0 of vtable 0x01040408.  __ti16__ptmd_type_info @0x0104DE78, name
//   "16__ptmd_type_info" @0x01040480.
extern "C" const type_info &__tf16__ptmd_type_info(void)
{
    return egcs_tf_si(__ti16__ptmd_type_info, "16__ptmd_type_info",
                      __tf9type_info, __ti9type_info);
}

// FUNC 0x0101ADA0 __ptmd_type_info::~__ptmd_type_info()
//   Entry 1 of vtable 0x01040408 (@0x01040414): forwards to ~type_info.
__ptmd_type_info::~__ptmd_type_info()
{
}

// FUNC 0x0101ADC0 __tf16__ptmf_type_info (compiler-generated)
//   Entry 0 of vtable 0x01040418.  __ti16__ptmf_type_info @0x0104DE48, name
//   "16__ptmf_type_info" @0x01040498.
extern "C" const type_info &__tf16__ptmf_type_info(void)
{
    return egcs_tf_si(__ti16__ptmf_type_info, "16__ptmf_type_info",
                      __tf9type_info, __ti9type_info);
}

// FUNC 0x0101AE10 __ptmf_type_info::~__ptmf_type_info()
//   Entry 1 of vtable 0x01040418 (@0x01040424): forwards to ~type_info.
__ptmf_type_info::~__ptmf_type_info()
{
}

// FUNC 0x0101AE30 __tf16__func_type_info (compiler-generated)
//   Entry 0 of vtable 0x01040428.  __ti16__func_type_info @0x0104DE28, name
//   "16__func_type_info" @0x010404B0.  dynamic_cast target of the matcher's
//   catch(void *) rule.
extern "C" const type_info &__tf16__func_type_info(void)
{
    return egcs_tf_si(__ti16__func_type_info, "16__func_type_info",
                      __tf9type_info, __ti9type_info);
}

// FUNC 0x0101AE80 __func_type_info::~__func_type_info()
//   Entry 1 of vtable 0x01040428 (@0x01040434): forwards to ~type_info.
__func_type_info::~__func_type_info()
{
}

// FUNC 0x0101AEA0 __tf19__builtin_type_info (compiler-generated)
//   Entry 0 of vtable 0x01040438 (the vtable of all builtin objects).
//   __ti19__builtin_type_info @0x0104DE88, name "19__builtin_type_info"
//   @0x010404C8.
extern "C" const type_info &__tf19__builtin_type_info(void)
{
    return egcs_tf_si(__ti19__builtin_type_info, "19__builtin_type_info",
                      __tf9type_info, __ti9type_info);
}

// FUNC 0x0101AEF0 __builtin_type_info::~__builtin_type_info()
//   Entry 1 of vtable 0x01040438 (@0x01040444): forwards to ~type_info.
__builtin_type_info::~__builtin_type_info()
{
}

// FUNC 0x0101AF10 __tf16__attr_type_info (compiler-generated)
//   Entry 0 of vtable 0x01040448.  __ti16__attr_type_info @0x0104DE38, name
//   "16__attr_type_info" @0x010404E0.  dynamic_cast target of the matcher's
//   cv-stripping steps.
extern "C" const type_info &__tf16__attr_type_info(void)
{
    return egcs_tf_si(__ti16__attr_type_info, "16__attr_type_info",
                      __tf9type_info, __ti9type_info);
}

// FUNC 0x0101AF60 __attr_type_info::~__attr_type_info()
//   Entry 1 of vtable 0x01040448 (@0x01040454): forwards to ~type_info.
__attr_type_info::~__attr_type_info()
{
}

// FUNC 0x0101AF80 __tf19__pointer_type_info (compiler-generated)
//   Entry 0 of vtable 0x01040458.  __ti19__pointer_type_info @0x0104DE68, name
//   "19__pointer_type_info" @0x010404F8.  dynamic_cast target in the matcher
//   and __is_pointer.
extern "C" const type_info &__tf19__pointer_type_info(void)
{
    return egcs_tf_si(__ti19__pointer_type_info, "19__pointer_type_info",
                      __tf9type_info, __ti9type_info);
}

// FUNC 0x0101AFD0 __pointer_type_info::~__pointer_type_info()
//   Entry 1 of vtable 0x01040458 (@0x01040464): forwards to ~type_info.
__pointer_type_info::~__pointer_type_info()
{
}
