/* T548_tinfo.h - <typeinfo> and gcc/cp/tinfo.h of the GCC 2.9x-ee C++ runtime:
 * type_info, bad_cast, bad_typeid and the class type_info nodes the
 * compiler-generated __tf<class> functions build (translation unit T548,
 * egcs tinfo.cc with #pragma implementation "typeinfo", unit u136),
 * 0x0027C350..0x0027CA1F.
 *
 * Every class here is in the GLOBAL namespace (mangled "9type_info":
 * -fhonor-std was off): no clash with GCC 14's std::type_info.
 *
 * Object layout (egcs): a type_info node is {+0x00 _name; +0x04 vptr; ...};
 * the vptr comes after type_info's data.  GCC 14 puts it at +0 instead.
 * The node of a class is placement-built on the first call of its
 * __tf<mangled> function, in .bss/.sbss storage __ti<mangled> whose word 0
 * (_name) == 0 means "not built yet":
 *     if (__tiX._name == 0) { __tf<Base>(); __rtti_si(&__tiX, "X", &__ti<Base>); }
 *     return __tiX;
 * (__rtti_user for a class without bases, __rtti_class with a base_info list
 * for multiple or virtual bases).  The __ti storage of this unit's own
 * classes (0x00353518 __ti9type_info, 0x004B2B50..0x004B2B9B) is used by
 * u136 only.
 *
 * egcs vtable entry 0 (vptr+4) holds the typeinfo function itself; for the
 * GCC 14 build only, the exception classes declare a __tf() stand-in first
 * (no address in the binary; see T550_new.h). */
#ifndef GAME_T548_TINFO_H
#define GAME_T548_TINFO_H

#include "game/game.h"
#include <stddef.h>
#include "runtime/cxx/T543_exception.h"            /* base exception (bad_cast, bad_typeid) */

class type_info;

/* Typeinfo functions (vtable entry 0) of this unit's classes: the GCC 14
 * build's (u136 defines them).  egcs generates them itself, as for every
 * class (rule 8): it declares a polymorphic class's __tf<mangled> when the
 * class is complete; a declaration before that would be the one it uses,
 * and it would then not emit the function. */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf17__class_type_info(void);         /* 0x0027C6E8 */
const type_info &__tf14__si_type_info(void);            /* 0x0027C778 */
const type_info &__tf16__user_type_info(void);          /* 0x0027C800 */
const type_info &__tf10bad_typeid(void);                /* 0x0027C8B8 */
const type_info &__tf8bad_cast(void);                   /* 0x0027C950 */
const type_info &__tf9type_info(void);                  /* 0x0027C9E0 */
}
#endif

#ifdef BOOT_OLD_GCC
/* type_info, bad_cast and bad_typeid are egcs's own <typeinfo> (#pragma
 * interface "typeinfo"): its inline members (the type_info ctor, name(),
 * operator!=, bad_cast's and bad_typeid's ctors and dtors) are inlined into
 * their users, as in the binary (T543's __throw_bad_cast /
 * __throw_bad_typeid / exception::what), and tinfo.cc's #pragma
 * implementation "typeinfo" emits their out-of-line copies (the addresses
 * below).  The same arrangement as T543 with <exception>.  The GCC 14 build
 * declares the classes itself, with the same members. */
#include <typeinfo>
#else
/* vtable _vt$9type_info 0x003484C8 (1 slot): entry 0 __tf9type_info 0x0027C9E0.
 * typeinfo __ti9type_info 0x00353518 (.sbss, a __user_type_info).  Size 8.
 * name(), operator!= and the ctor are inline in egcs <typeinfo>; the
 * addresses are tinfo.cc's out-of-line copies (no callers). */
class type_info {
private:
    type_info &operator=(const type_info &);    /* not implemented (egcs: no assignment) */
    type_info(const type_info &);               /* not implemented (egcs: no copy) */

protected:
    explicit type_info(const char *n);          /* 0x0027C9C8 */

public:
    const char *_name;                          /* 0x00 mangled name ("9type_info") */
    /* 0x04 vptr */

    virtual ~type_info();                       /* [0] +0x0C 0x0027C350 */

    bool before(const type_info &arg) const;    /* 0x0027CA20 (defined in T549): strcmp < 0 */
    const char *name() const;                   /* 0x0027C9C0 returns _name */
    bool operator==(const type_info &arg) const;    /* 0x0027C380 same node or strcmp == 0 */
    bool operator!=(const type_info &arg) const;    /* 0x0027C9A0 */
};

/* Thrown by a failed dynamic_cast to a reference (__throw_bad_cast, T543).
 * vtable _vt$8bad_cast 0x003484B0 (2 slots): entry 0 __tf8bad_cast 0x0027C950.
 * typeinfo __ti8bad_cast 0x004B2B60 (a __si_type_info, base exception).
 * Size 4, vptr at +0x00. */
class bad_cast : public exception {
public:
    bad_cast();                                 /* 0x0027C938 (inline in egcs; out-of-line copy): vptr = 0x003484B0 */
    virtual const type_info &__tf() const;      /* GCC 14 stand-in for entry 0 = __tf8bad_cast */
    virtual ~bad_cast();                        /* [0] +0x0C 0x0027C908 */
    /* [1] +0x14 exception::what 0x0027A2B8 (inherited) */
};

/* Thrown by typeid(*p) with p == 0 (__throw_bad_typeid, T543).  vtable
 * _vt$10bad_typeid 0x00348498 (2 slots): entry 0 __tf10bad_typeid 0x0027C8B8.
 * typeinfo __ti10bad_typeid 0x004B2B50 (a __si_type_info, base exception).
 * Size 4, vptr at +0x00. */
class bad_typeid : public exception {
public:
    bad_typeid();                               /* 0x0027C8A0 (inline in egcs; out-of-line copy): vptr = 0x00348498 */
    virtual const type_info &__tf() const;      /* GCC 14 stand-in for entry 0 = __tf10bad_typeid */
    virtual ~bad_typeid();                      /* [0] +0x0C 0x0027C870 */
    /* [1] +0x14 exception::what 0x0027A2B8 (inherited) */
};
#endif /* BOOT_OLD_GCC */
EGCS_SIZE(type_info, 8);
EGCS_SIZE(bad_cast, 4);
EGCS_SIZE(bad_typeid, 4);

/* The type_info of a class without bases (also the base of the other class
 * nodes).  vtable _vt$16__user_type_info 0x00348480 (2 slots): entry 0
 * __tf16__user_type_info 0x0027C800.  typeinfo __ti16__user_type_info
 * 0x004B2B70.  Size 8 (__rtti_user stores _name and the vptr only). */
struct __user_type_info : public type_info {
#ifdef BOOT_OLD_GCC
    /* egcs tinfo.h: inline ctor (inlined into __rtti_user and the derived
     * ctors; tinfo.cc, which defines dcast, also emits it out of line), and
     * no destructor declaration: the implicit one (0x0027C850) is empty and
     * sets no vptr, it only calls ~type_info.  The same holds for the two
     * classes below. */
    __user_type_info(const char *n) : type_info(n) {}  /* 0x0027C7E8 */
#else
    __user_type_info(const char *n);            /* 0x0027C7E8 (inline in egcs; out-of-line copy) */
    virtual ~__user_type_info();                /* [0] +0x0C 0x0027C850 */
#endif
    /* If our type can be converted to the desired type `to`, return objptr
     * adjusted accordingly, else 0. */
    virtual void *dcast(const type_info &to, int require_public, void *objptr,
                        const type_info *sub = 0, void *subptr = 0) const;  /* [1] +0x14 0x0027C428 */
};
EGCS_SIZE(__user_type_info, 8);

/* The type_info of a class with one public, non-virtual base (built by
 * __rtti_si, 405 callers: every single-inheritance __tf function).  vtable _vt$14__si_type_info
 * 0x00348468 (2 slots): entry 0 __tf14__si_type_info 0x0027C778.  typeinfo
 * __ti14__si_type_info 0x004B2B80.  Size 0xC. */
class __si_type_info : public __user_type_info {
public:
    const __user_type_info &base;               /* 0x08 the base's node */

#ifdef BOOT_OLD_GCC
    __si_type_info(const char *n, const __user_type_info &b)
        : __user_type_info(n), base(b) {}       /* 0x0027C758; implicit dtor 0x0027C7C8 */
#else
    __si_type_info(const char *n, const __user_type_info &b);  /* 0x0027C758 (inline in egcs; out-of-line copy) */
    virtual ~__si_type_info();                  /* [0] +0x0C 0x0027C7C8 */
#endif
    virtual void *dcast(const type_info &to, int require_public, void *objptr,
                        const type_info *sub = 0, void *subptr = 0) const;  /* [1] +0x14 0x0027C458 */
};
EGCS_SIZE(__si_type_info, 0xC);

/* The type_info of a class with multiple and/or virtual bases (43 game __tf
 * functions build one through __rtti_class: gt2k_menu::TopButtonList,
 * GarageCar, ...).  vtable _vt$17__class_type_info 0x00348450 (2 slots):
 * entry 0 __tf17__class_type_info 0x0027C6E8.  typeinfo
 * __ti17__class_type_info 0x004B2B90.  Size 0x10. */
struct __class_type_info : public __user_type_info {
    enum access { PUBLIC = 1, PROTECTED = 2, PRIVATE = 3 };

    /* One base: 8 bytes (the game's base lists are u32[2] per entry).  Word
     * +4 is {offset:29 | is_virtual:1 | access:2}, LSB first.  egcs declares
     * `USItype offset : 29; bool is_virtual : 1; access access : 2;` - the
     * same word; u32 bit-fields here, because a member `access` of type
     * `access` is an error in GCC 14 (-Wchanges-meaning).  The offset is UNSIGNED: dcast
     * extracts it with `and 0x1FFFFFFF`. */
    struct base_info {
        const __user_type_info *base;           /* 0x00 the base's node */
        u32 offset : 29;                        /* 0x04 bits 0..28: base offset in the object */
        u32 is_virtual : 1;                     /* bit 29: offset locates a pointer to the base */
        u32 access : 2;                         /* bits 30..31: enum access */
    };

    const base_info *base_list;                 /* 0x08 */
    size_t n_bases;                             /* 0x0C */

#ifdef BOOT_OLD_GCC
    __class_type_info(const char *name, const base_info *bl, size_t bn)
        : __user_type_info(name), base_list(bl), n_bases(bn) {}  /* 0x0027C6C8; implicit dtor 0x0027C738 */
#else
    __class_type_info(const char *name, const base_info *bl, size_t bn);  /* 0x0027C6C8 (inline in egcs; out-of-line copy) */
    virtual ~__class_type_info();               /* [0] +0x0C 0x0027C738 */
#endif
    virtual void *dcast(const type_info &desired, int is_public, void *objptr,
                        const type_info *sub = 0, void *subptr = 0) const;  /* [1] +0x14 0x0027C4F0 */
};
EGCS_SIZE(__class_type_info, 0x10);
EGCS_SIZE(__class_type_info::base_info, 8);

extern "C" {

/* Placement builders called by the compiler-generated __tf<class> functions:
 * `new (addr) X(...)` with the ctor inlined, skipped when addr == 0.  They
 * return nothing. */
void __rtti_class(void *addr, const char *name,
                  const __class_type_info::base_info *bl, size_t bn);  /* 0x0027C3C0 (43 callers) */
void __rtti_si(void *addr, const char *n, const type_info *ti);        /* 0x0027C3E8 (405 callers) */
void __rtti_user(void *addr, const char *name);                        /* 0x0027C408 (86 callers) */

} /* extern "C" */

#endif /* GAME_T548_TINFO_H */
