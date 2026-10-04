/* tinfo.h - RTTI classes of the GCC 2.9x-ee (egcs 1.1 era) C++ runtime in the
 * GT3 US bootstrap (BOOTSTRAP_SCUS-97102): egcs <typeinfo> (type_info,
 * bad_cast, bad_typeid) plus gcc/cp/tinfo.h (__user_type_info,
 * __si_type_info, __class_type_info).  Shared by runtime/tinfo.cc (tinfo.o,
 * 0x01019BA0..0x0101A26F) and runtime/tinfo2.cc (tinfo2.o,
 * 0x0101A270..0x0101AFEF); the tinfo2-only classes (__pointer_type_info,
 * __attr_type_info, ...) are declared in tinfo2.cc, as in egcs.
 *
 * type_info objects in the binary: {+0x00 const char *_name; +0x04 vptr; ...}
 * - the vptr comes AFTER the data members (type_info has no polymorphic base).
 * Every "first word == 0" test on a typeinfo object therefore tests _name.
 * (Modern g++ places the vptr at +0; the sizes still match on the EE.)
 *
 * How the compiler uses this runtime (GCC 2.9x rtti.c):
 *   - For each polymorphic class X it emits storage `__ti<X>` (.bss/.sbss) and
 *     a typeinfo function `const type_info &__tf<X>()` (vtable entry 0):
 *       if (__ti<X>.first_word == 0) { [__tf<Base>();] __rtti_si/__rtti_user/
 *                                      __rtti_class(&__ti<X>, "<X>", ...); }
 *       return __ti<X>;
 *     __rtti_si (0x01019C38) and __rtti_user (0x01019C58) are the builders
 *     called by every __tf function in the game code (the mapping pass called
 *     them "__si_type_info ctor" / "__user_type_info ctor").
 *   - dynamic_cast<const T *>(p) for a polymorphic p is lowered to
 *       p == 0 ? 0
 *              : __dynamic_cast(vptr(p)[0].pfn,            // p's own __tf
 *                               &__tf<T>, 1,                // require_public
 *                               (char *)p + vptr(p)[0].delta,  // "headof"
 *                               &__tf<static type of *p>, p)
 *     __dynamic_cast (0x0101A878) dispatches to the virtual dcast() (vtable
 *     entry 2) of the dynamic type's typeinfo object.
 *   - typeid(void) is lowered to __tfv() (0x0101A928).
 */
#ifndef BOOT_RUNTIME_TINFO_H
#define BOOT_RUNTIME_TINFO_H

#include "types.h"
#include "new.h"
#include <stddef.h>

/* ------------------------------------------------------------------------ */
/* <typeinfo>                                                               */
/* ------------------------------------------------------------------------ */

/* class type_info.  sizeof == 8: +0x00 _name, +0x04 vptr.
 * vtable _vt$9type_info @0x010402D8: [0] __tf9type_info 0x0101A230,
 *   [1] ~type_info 0x01019BA0.
 * Its own typeinfo: __ti9type_info @0x01041EC0 (.sbss), a __user_type_info. */
class type_info {
private:
    type_info &operator=(const type_info &);    /* not implemented (egcs) */
    type_info(const type_info &);               /* not implemented (egcs) */

protected:
    explicit type_info(const char *n);          /* 0x0101A218 (inline in egcs) */

    const char *_name;                          /* +0x00 mangled name */
    /* +0x04 vptr (original layout) */

public:
    virtual ~type_info();                       /* 0x01019BA0 */

    bool before(const type_info &arg) const;    /* 0x0101A270 (tinfo2.cc) */
    const char *name() const;                   /* 0x0101A210 (inline in egcs) */
    bool operator==(const type_info &arg) const;    /* 0x01019BD0 */
    bool operator!=(const type_info &arg) const;    /* 0x0101A1F0 (inline in egcs) */
};

/* class bad_cast (egcs <typeinfo>).  sizeof == 4 (vptr).
 * vtable _vt$8bad_cast @0x010402C0: [0] __tf8bad_cast 0x0101A1A0,
 *   [1] ~bad_cast 0x0101A158, [2] exception::what 0x010186E8 (inherited). */
class bad_cast : public exception {
public:
    bad_cast();                                 /* 0x0101A188 (inline in egcs) */
    virtual const type_info &__tf() const;      /* vtable entry 0 = __tf8bad_cast (see new.h) */
    virtual ~bad_cast();                        /* 0x0101A158 (implicit in egcs) */
};

/* class bad_typeid (egcs <typeinfo>).  sizeof == 4 (vptr).
 * vtable _vt$10bad_typeid @0x010402A8: [0] __tf10bad_typeid 0x0101A108,
 *   [1] ~bad_typeid 0x0101A0C0, [2] exception::what 0x010186E8 (inherited). */
class bad_typeid : public exception {
public:
    bad_typeid();                               /* 0x0101A0F0 (inline in egcs) */
    virtual const type_info &__tf() const;      /* vtable entry 0 = __tf10bad_typeid (see new.h) */
    virtual ~bad_typeid();                      /* 0x0101A0C0 (implicit in egcs) */
};

/* ------------------------------------------------------------------------ */
/* gcc/cp/tinfo.h                                                           */
/* ------------------------------------------------------------------------ */

/* type_info for a class with no base classes (or an enum).  sizeof == 8.
 * vtable _vt$16__user_type_info @0x01040290: [0] __tf16__user_type_info
 *   0x0101A050, [1] ~__user_type_info 0x0101A0A0, [2] dcast 0x01019C78. */
struct __user_type_info : public type_info {
    __user_type_info(const char *n);            /* 0x0101A038 (inline in egcs) */
    virtual ~__user_type_info();                /* 0x0101A0A0 (implicit in egcs) */

    /* If our type can be converted to the desired type, return the pointer,
     * adjusted accordingly; else return 0.  Vtable entry 2. */
    virtual void *dcast(const type_info &to, int require_public, void *objptr,
                        const type_info *sub = 0, void *subptr = 0) const;
};

/* type_info for a class with one public, non-virtual base.  sizeof == 0xC:
 *   +0x08 const __user_type_info &base.
 * vtable _vt$14__si_type_info @0x01040278: [0] __tf14__si_type_info
 *   0x01019FC8, [1] ~__si_type_info 0x0101A018, [2] dcast 0x01019CA8. */
class __si_type_info : public __user_type_info {
    const __user_type_info &base;               /* +0x08 */

public:
    __si_type_info(const char *n, const __user_type_info &b);  /* 0x01019FA8 */
    virtual ~__si_type_info();                  /* 0x0101A018 (implicit in egcs) */

    virtual void *dcast(const type_info &to, int require_public, void *objptr,
                        const type_info *sub = 0, void *subptr = 0) const;
};

/* type_info for a general class (multiple and/or virtual bases).
 * sizeof == 0x10: +0x08 const base_info *base_list, +0x0C size_t n_bases.
 * vtable _vt$17__class_type_info @0x01040260: [0] __tf17__class_type_info
 *   0x01019F38, [1] ~__class_type_info 0x01019F88, [2] dcast 0x01019D40.
 * Nothing in this binary builds one (__rtti_class has no callers). */
struct __class_type_info : public __user_type_info {
    enum access { PUBLIC = 1, PROTECTED = 2, PRIVATE = 3 };

    /* 8 bytes.  Word +4 is {offset:29 | is_virtual:1 | access:2}, LSB first.
     * The offset is UNSIGNED in this build: dcast extracts it with
     * `and 0x1FFFFFFF` (no sign extension). */
    struct base_info {
        const __user_type_info *base;           /* +0x00 */
        u32 offset : 29;                        /* +0x04 bits 0..28 */
        bool is_virtual : 1;                    /* +0x04 bit 29 */
        enum access access : 2;                 /* +0x04 bits 30..31 */
    };

    const base_info *base_list;                 /* +0x08 */
    size_t n_bases;                             /* +0x0C */

    __class_type_info(const char *name, const base_info *bl, size_t bn);  /* 0x01019F18 */
    virtual ~__class_type_info();               /* 0x01019F88 (implicit in egcs) */

    virtual void *dcast(const type_info &desired, int is_public, void *objptr,
                        const type_info *sub = 0, void *subptr = 0) const;
};

#if defined(__cplusplus) && __cplusplus >= 201103L
static_assert(sizeof(type_info) == 8, "type_info: {name, vptr}");
static_assert(sizeof(__user_type_info) == 8, "__user_type_info adds nothing");
static_assert(sizeof(__si_type_info) == 0xC, "__si_type_info: + base&");
static_assert(sizeof(__class_type_info) == 0x10, "__class_type_info: + base_list, n_bases");
static_assert(sizeof(__class_type_info::base_info) == 8, "base_info entries are 8 bytes");
#endif

/* ------------------------------------------------------------------------ */
/* Compiler entry points (C linkage)                                        */
/* ------------------------------------------------------------------------ */

extern "C" {

/* tinfo.cc: placement builders used by compiler-generated __tf functions.
 * Each is `new (addr) X(...)` with the ctor inlined; the placement new is
 * throw(), so egcs null-checks addr first.  They return nothing. */
void __rtti_class(void *addr, const char *name,
                  const __class_type_info::base_info *bl, size_t bn);   /* 0x01019C10 */
void __rtti_si(void *addr, const char *n, const type_info *ti);         /* 0x01019C38 */
void __rtti_user(void *addr, const char *name);                         /* 0x01019C58 */

/* tinfo2.cc builders (no callers in this binary). */
void __rtti_ptr(void *addr, const char *n, const type_info *ti);        /* 0x0101A7B0 */
void __rtti_attr(void *addr, const char *n, int attrval, const type_info *ti); /* 0x0101A7D0 */
void __rtti_func(void *addr, const char *name);                         /* 0x0101A7F8 */
void __rtti_ptmf(void *addr, const char *name);                         /* 0x0101A818 */
void __rtti_ptmd(void *addr, const char *name);                         /* 0x0101A838 */
void __rtti_array(void *addr, const char *name);                        /* 0x0101A858 */

/* tinfo2.cc: runtime of dynamic_cast (see the lowering above). */
void *__dynamic_cast(const type_info &(*from)(void),
                     const type_info &(*to)(void),
                     int require_public, void *address,
                     const type_info &(*sub)(void), void *subptr);      /* 0x0101A878 */

/* Typeinfo functions (vtable entry 0 of the respective classes) and the
 * storage they build.  Storage is declared as raw words: the object is
 * placement-constructed on first use; word 0 (_name) != 0 means "built". */
const type_info &__tf9type_info(void);              /* 0x0101A230 */
const type_info &__tf16__user_type_info(void);      /* 0x0101A050 */
const type_info &__tf14__si_type_info(void);        /* 0x01019FC8 */
const type_info &__tf17__class_type_info(void);     /* 0x01019F38 */
const type_info &__tf10bad_typeid(void);            /* 0x0101A108 */
const type_info &__tf8bad_cast(void);               /* 0x0101A1A0 */
const type_info &__tf9bad_alloc(void);              /* 0x01019890 (new.cc) */
const type_info &__tfv(void);                       /* 0x0101A928 typeid(void) */

extern void *__ti9type_info[2];                     /* @0x01041EC0 (.sbss) __user_type_info */
extern void *__ti16__user_type_info[3];             /* @0x0104DDF8 (.bss)  __si_type_info */
extern void *__ti14__si_type_info[3];               /* @0x0104DE08 (.bss)  __si_type_info */
extern void *__ti17__class_type_info[3];            /* @0x0104DE18 (.bss)  __si_type_info */
extern void *__ti10bad_typeid[3];                   /* @0x0104DDD8 (.bss)  __si_type_info */
extern void *__ti8bad_cast[3];                      /* @0x0104DDE8 (.bss)  __si_type_info */
extern void *__ti9bad_alloc[3];                     /* @0x0104DDC8 (.bss)  __si_type_info */

} /* extern "C" */

/* The body every compiler-generated __tf function of a single-inheritance
 * class has in this binary (13 of them in this unit, more in game code):
 *     lw   v0, 0(ti)           ; built yet?  (word 0 = _name)
 *     bnez v0, done
 *     jal  base_tf             ; make sure the base's typeinfo exists (result unused)
 *     jal  __rtti_si(ti, name, base_ti)   ; base_ti is the base's __ti symbol
 *   done: return ti
 * Written once here; each __tf function in tinfo.cc / tinfo2.cc / new.cc is a
 * call of it with its own constants. */
static inline const type_info &
egcs_tf_si(void **ti, const char *name,
           const type_info &(*base_tf)(void), void **base_ti)
{
    if (ti[0] == 0) {
        base_tf();
        __rtti_si(ti, name, (const type_info *)base_ti);
    }
    return *(const type_info *)ti;
}

#endif /* BOOT_RUNTIME_TINFO_H */
