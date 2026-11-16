/* T549_tinfo2.h - tinfo2.cc of the GCC 2.9x-ee C++ runtime: the type_info
 * nodes of non-class types, the EH catch matcher, __dynamic_cast and the
 * builtin typeinfo functions (translation unit T549, unit u136),
 * 0x0027CA20..0x0027D79F.  type_info::before (0x0027CA20) opens this unit; it
 * is declared with type_info in T548_tinfo.h.
 *
 * The seven classes are file-local to egcs tinfo2.cc; the binary emits their
 * typeinfo functions and implicit destructors (the destructors forward
 * (this, flags) to ~type_info 0x0027C350).  Their builders __rtti_ptr ..
 * __rtti_array have no callers, so no pointer, cv, function, member-pointer
 * or array node is ever built in the game; of this unit's kinds only the 17
 * builtin nodes exist (.data 0x00306DA0..0x00306E27, 8 bytes each, zero
 * until built).  Layout as in T548_tinfo.h: {+0x00 _name; +0x04 vptr; ...}
 * (egcs). */
#ifndef GAME_T549_TINFO2_H
#define GAME_T549_TINFO2_H

#include "game/game.h"
#include "runtime/cxx/T548_tinfo.h"                /* base type_info */

/* Typeinfo functions (vtable entry 0) of the local classes, emitted in
 * reverse declaration order.  GCC 14 build only: egcs generates them (see
 * T548_tinfo.h). */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf17__array_type_info(void);         /* 0x0027D490 */
const type_info &__tf16__ptmd_type_info(void);          /* 0x0027D500 */
const type_info &__tf16__ptmf_type_info(void);          /* 0x0027D570 */
const type_info &__tf16__func_type_info(void);          /* 0x0027D5E0 */
const type_info &__tf19__builtin_type_info(void);       /* 0x0027D650 */
const type_info &__tf16__attr_type_info(void);          /* 0x0027D6C0 */
const type_info &__tf19__pointer_type_info(void);       /* 0x0027D730 */
}
#endif

/* The seven classes.  Destructors: egcs tinfo2.cc declares none.  The
 * implicit ones have an empty body, so they set no vptr and only call
 * ~type_info (the binary's 7-instruction destructors); a declared
 * destructor, even empty, stores the class's vtable first.  So under egcs
 * T549_tinfo2.cpp, which defines T549_TINFO2_LOCAL_CLASSES, sees the classes
 * without destructor declarations.  Everywhere else they stay declared:
 * without one the classes have no key function, and egcs writes their type
 * names ("19__pointer_type_info" ...) into the .rodata of every object that
 * sees them (the other u136 objects, u134, the game units that include this
 * header for __dynamic_cast), while in the original they are local to
 * tinfo2.cc.  The layout and the vtable slots are the same either way.  The
 * GCC 14 build declares and defines them (T549_tinfo2.cpp, with the FUNC
 * markers). */

/* Type info for a pointer type.  vtable _vt$19__pointer_type_info 0x00348648
 * (1 slot): entry 0 __tf19__pointer_type_info 0x0027D730.  Size 0xC
 * (__rtti_ptr stores _name, vptr, type). */
struct __pointer_type_info : public type_info {
    const type_info &type;                      /* 0x08 the pointee's node */

    __pointer_type_info(const char *n, const type_info &ti)
        : type_info(n), type(ti) {}             /* inline (inlined into __rtti_ptr) */
#if !defined(BOOT_OLD_GCC) || !defined(T549_TINFO2_LOCAL_CLASSES)    /* (see above) */
    virtual ~__pointer_type_info();             /* [0] +0x0C 0x0027D780 */
#endif
};
EGCS_SIZE(__pointer_type_info, 0xC);

/* Type info for a cv-qualified type.  vtable _vt$16__attr_type_info
 * 0x00348638 (1 slot): entry 0 __tf16__attr_type_info 0x0027D6C0.  Size 0x10
 * (__rtti_attr stores _name, vptr, type, attr). */
struct __attr_type_info : public type_info {
    enum cv { NONE = 0, CONST = 1, VOLATILE = 2, CONSTVOL = 1 | 2 };

    const type_info &type;                      /* 0x08 the unqualified type's node */
    cv attr;                                    /* 0x0C qualifiers */

    __attr_type_info(const char *n, cv a, const type_info &t)
        : type_info(n), type(t), attr(a) {}     /* inline (inlined into __rtti_attr) */
#if !defined(BOOT_OLD_GCC) || !defined(T549_TINFO2_LOCAL_CLASSES)    /* (see above) */
    virtual ~__attr_type_info();                /* [0] +0x0C 0x0027D710 */
#endif
};
EGCS_SIZE(__attr_type_info, 0x10);

/* Type info for a builtin type.  vtable _vt$19__builtin_type_info 0x00348628
 * (1 slot, the vtable of all 17 builtin nodes): entry 0
 * __tf19__builtin_type_info 0x0027D650.  Size 8 (the builtin nodes are 8
 * bytes apart). */
struct __builtin_type_info : public type_info {
    __builtin_type_info(const char *n) : type_info(n) {}    /* inline (inlined into the builtin __tf<m>) */
#if !defined(BOOT_OLD_GCC) || !defined(T549_TINFO2_LOCAL_CLASSES)    /* (see above) */
    virtual ~__builtin_type_info();             /* [0] +0x0C 0x0027D6A0 */
#endif
};
EGCS_SIZE(__builtin_type_info, 8);

/* Type info for a function type.  vtable _vt$16__func_type_info 0x00348618
 * (1 slot): entry 0 __tf16__func_type_info 0x0027D5E0.  Size 8. */
struct __func_type_info : public type_info {
    __func_type_info(const char *n) : type_info(n) {}       /* inline (inlined into __rtti_func) */
#if !defined(BOOT_OLD_GCC) || !defined(T549_TINFO2_LOCAL_CLASSES)    /* (see above) */
    virtual ~__func_type_info();                /* [0] +0x0C 0x0027D630 */
#endif
};
EGCS_SIZE(__func_type_info, 8);

/* Type info for a pointer to member function.  vtable _vt$16__ptmf_type_info
 * 0x00348608 (1 slot): entry 0 __tf16__ptmf_type_info 0x0027D570.  Size 8. */
struct __ptmf_type_info : public type_info {
    __ptmf_type_info(const char *n) : type_info(n) {}       /* inline (inlined into __rtti_ptmf) */
#if !defined(BOOT_OLD_GCC) || !defined(T549_TINFO2_LOCAL_CLASSES)    /* (see above) */
    virtual ~__ptmf_type_info();                /* [0] +0x0C 0x0027D5C0 */
#endif
};
EGCS_SIZE(__ptmf_type_info, 8);

/* Type info for a pointer to data member: name only in this egcs version
 * (__rtti_ptmd stores _name and vptr).  vtable _vt$16__ptmd_type_info
 * 0x003485F8 (1 slot): entry 0 __tf16__ptmd_type_info 0x0027D500.  Size 8. */
struct __ptmd_type_info : public type_info {
    __ptmd_type_info(const char *n) : type_info(n) {}       /* inline (inlined into __rtti_ptmd) */
#if !defined(BOOT_OLD_GCC) || !defined(T549_TINFO2_LOCAL_CLASSES)    /* (see above) */
    virtual ~__ptmd_type_info();                /* [0] +0x0C 0x0027D550 */
#endif
};
EGCS_SIZE(__ptmd_type_info, 8);

/* Type info for an array type.  vtable _vt$17__array_type_info 0x003485E8
 * (1 slot): entry 0 __tf17__array_type_info 0x0027D490.  Size 8. */
struct __array_type_info : public type_info {
    __array_type_info(const char *n) : type_info(n) {}      /* inline (inlined into __rtti_array) */
#if !defined(BOOT_OLD_GCC) || !defined(T549_TINFO2_LOCAL_CLASSES)    /* (see above) */
    virtual ~__array_type_info();               /* [0] +0x0C 0x0027D4E0 */
#endif
};
EGCS_SIZE(__array_type_info, 8);

extern "C" {

/* EH catch matcher (egcs 1.1's 3-argument __throw_type_match_rtti): can an
 * exception of type *throw_type_r be caught by a handler for *catch_type_r?
 * Returns the adjusted object pointer, or 0.  Called by __cplus_type_matcher
 * and __check_eh_spec (T543).  The same declaration is in include/cxxrt.h. */
void *__throw_type_match_rtti_2(const void *catch_type_r, const void *throw_type_r,
                                void *objptr);                  /* 0x0027CA48 */

/* Is the type_info node p a pointer type?  (bool in egcs: 0/1.)  Called by
 * __cp_pop_exception (T543). */
int __is_pointer(void *p);                                      /* 0x0027CF10 */

/* Placement builders for compiler-generated non-class typeinfo (null-checked
 * `new (addr) X(...)`, no return value).  No callers in the game. */
void __rtti_ptr(void *addr, const char *n, const type_info *ti);            /* 0x0027CF60 */
void __rtti_attr(void *addr, const char *n, int attrval, const type_info *ti); /* 0x0027CF80 */
void __rtti_func(void *addr, const char *name);                             /* 0x0027CFA8 */
void __rtti_ptmf(void *addr, const char *name);                             /* 0x0027CFC8 */
void __rtti_ptmd(void *addr, const char *name);                             /* 0x0027CFE8 */
void __rtti_array(void *addr, const char *name);                            /* 0x0027D008 */

/* Runtime half of the egcs dynamic_cast: from() is the typeinfo function of
 * the object's dynamic type (vtable entry 0 of the object), to() the
 * target's, sub()/subptr the static type and the original pointer; calls
 * from()'s node's dcast(to(), require_public, address, &sub(), subptr).  52
 * game dynamic_casts, the matcher and __is_pointer call it.  (GCC 14 lowers
 * dynamic_cast to the Itanium __cxxabiv1::__dynamic_cast, which has the same
 * symbol name and other arguments.) */
void *__dynamic_cast(const type_info &(*from)(void), const type_info &(*to)(void),
                     int require_public, void *address,
                     const type_info &(*sub)(void), void *subptr);  /* 0x0027D028 */

/* Builtin typeinfo functions (egcs BUILTIN macro): build the 8-byte
 * __builtin_type_info node __ti<m> (.data) on first call and return it.
 * Only __tfv is called (typeid(void), the matcher's catch(void *) rule). */
const type_info &__tfv(void);           /* 0x0027D0D8 void */
const type_info &__tfx(void);           /* 0x0027D110 long long */
const type_info &__tfl(void);           /* 0x0027D148 long */
const type_info &__tfi(void);           /* 0x0027D180 int */
const type_info &__tfs(void);           /* 0x0027D1B8 short */
const type_info &__tfb(void);           /* 0x0027D1F0 bool */
const type_info &__tfc(void);           /* 0x0027D228 char */
const type_info &__tfw(void);           /* 0x0027D260 wchar_t */
const type_info &__tfr(void);           /* 0x0027D298 long double */
const type_info &__tfd(void);           /* 0x0027D2D0 double */
const type_info &__tff(void);           /* 0x0027D308 float */
const type_info &__tfUi(void);          /* 0x0027D340 unsigned int */
const type_info &__tfUl(void);          /* 0x0027D378 unsigned long */
const type_info &__tfUx(void);          /* 0x0027D3B0 unsigned long long */
const type_info &__tfUs(void);          /* 0x0027D3E8 unsigned short */
const type_info &__tfUc(void);          /* 0x0027D420 unsigned char */
const type_info &__tfSc(void);          /* 0x0027D458 signed char */

} /* extern "C" */

#endif /* GAME_T549_TINFO2_H */
