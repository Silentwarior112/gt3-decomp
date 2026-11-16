/* cxx_runtime_internal.h - the C++ dynamic-memory and RTTI runtime of the GT3 US main
 * executable (GCC 2.9x-ee libgcc.a, egcs 1.1 era), 0x0027C010..0x0027D7A0.
 * Same machine code as the bootstrap's runtime/new.cc, tinfo.cc, tinfo2.cc,
 * i.e. the members new.o, opdel.o, opnew.o, opvdel.o, opvnew.o, tinfo.o and
 * tinfo2.o of the toolchain's own libgcc.a (sdk/ee-gcc-2.9-991111b): the
 * egcs build of this unit reproduces their .text and .rodata byte for byte
 * (ref/matching/u136/notes.md).
 *
 * Translation units (one libgcc.a member each, each with its own CIE):
 *   T550 0x0027C010  new.cc: set_new_handler, placement new[]/new, bad_alloc
 *                    members (#pragma implementation "new")
 *   T544 0x0027C0C0  operator delete(void *)      (egcs new2.cc L_op_delete)
 *   T545 0x0027C138  operator new(size_t)         (egcs new1.cc L_op_new)
 *   T546 0x0027C260  operator delete[](void *)    (egcs new2.cc L_op_vdel)
 *   T547 0x0027C2D8  operator new[](size_t)       (egcs new1.cc L_op_vnew)
 *   T548 0x0027C350  tinfo.cc
 *   T549 0x0027CA20  tinfo2.cc
 *
 * The classes (exception T543; bad_alloc T550; type_info, bad_cast,
 * bad_typeid and the class type_info nodes T548; the non-class nodes T549),
 * the builders, __dynamic_cast and the typeinfo functions are declared by the
 * canonical headers.  This header keeps what only this unit uses: the __ti
 * storage of the runtime's own classes and of the builtin types, and the
 * GCC 2.9x lowering of tinfo2.cc's dynamic_casts.
 *
 * Object model (GCC 2.9x): a type_info node is {+0x00 _name; +0x04 vptr; ...}
 * (the vptr after type_info's data); the exception classes are {+0x00 vptr}.
 * vtables are 8-byte entries {s16 delta; s16 index; pfn}; entry 0's pfn is
 * the class's __tf<mangled>, slot k is entry k+1 (call offset 8k+0xC).
 * Destructors take a hidden `int flags` (bit 0 = operator delete this).
 *
 * Core addresses of the EH runtime this unit calls: __throw 0x00278268,
 * std::terminate 0x00279CA8, __start_cp_handler 0x00279D50, __eh_alloc
 * 0x00279D88, __cp_push_exception 0x00279E40, __cp_pop_exception 0x00279ED0,
 * __check_eh_spec 0x00279FC8; libc: malloc 0x0026C3A8, free 0x0026C380,
 * strcmp 0x0027EA40.  The exception-specification landing pads (throw() /
 * throw(bad_alloc)) of T544..T547 call __start_cp_handler,
 * __check_eh_spec(n, spec), __cp_pop_exception, __throw and std::terminate:
 * that is what the exception specifications on the declarations mean; they
 * are not written out separately.
 */
#ifndef U136_H
#define U136_H

#include "types.h"
#ifdef BOOT_OLD_GCC
/* Every member of the original includes egcs's own <new> ("new"): its
 * bad_alloc (inline what(), implicit destructor), the inline placement forms
 * (operator new's null test in the builders), and its "bad_alloc" literal at
 * the start of each object's .rodata (retail 0x00348400..0x00348440,
 * 0x00348550).  First: the throw() specifications of its inline functions
 * make egcs declare __start_cp_handler / __check_eh_spec itself, noreturn,
 * as in the landing pads of T544..T547 (T543_exception.h then leaves them
 * out); T550_new.h takes the classes from it. */
#include <new>
#endif
#include "libc.h"                       /* strcmp, malloc, free */
#include <stddef.h>
#include "runtime/cxx/T543_exception.h"        /* exception, __eh_alloc, __cp_push_exception, __tf9exception */
#include "runtime/cxx/T550_new.h"              /* bad_alloc, __new_handler, the placement forms */
#include "runtime/cxx/T548_tinfo.h"            /* type_info, __user/__si/__class_type_info, __rtti_si */
#include "runtime/cxx/T549_tinfo2.h"           /* __pointer/__attr/...type_info, __dynamic_cast */

extern "C" {

#ifndef BOOT_OLD_GCC
/* Typeinfo nodes of the runtime's own classes (egcs generates them itself),
 * placement-built on first use by their __tf functions; word 0 (_name) == 0
 * means "not built yet".  __user_type_info nodes are 2 words, __si 3. */
extern void *__ti9type_info[2];                     /* 0x00353518 (.sbss) __user_type_info */
extern void *__ti9bad_alloc[3];                     /* 0x004B2B40 (.bss)  __si_type_info */
extern void *__ti10bad_typeid[3];                   /* 0x004B2B50 */
extern void *__ti8bad_cast[3];                      /* 0x004B2B60 */
extern void *__ti16__user_type_info[3];             /* 0x004B2B70 */
extern void *__ti14__si_type_info[3];               /* 0x004B2B80 */
extern void *__ti17__class_type_info[3];            /* 0x004B2B90 */
extern void *__ti16__func_type_info[3];             /* 0x004B2BA0 */
extern void *__ti16__attr_type_info[3];             /* 0x004B2BB0 */
extern void *__ti16__ptmf_type_info[3];             /* 0x004B2BC0 */
extern void *__ti17__array_type_info[3];            /* 0x004B2BD0 */
extern void *__ti19__pointer_type_info[3];          /* 0x004B2BE0 */
extern void *__ti16__ptmd_type_info[3];             /* 0x004B2BF0 */
extern void *__ti19__builtin_type_info[3];          /* 0x004B2C00 */
#endif

/* The 17 builtin typeinfo nodes (egcs tinfo2.cc's BUILTIN macro defines them
 * by hand: 8 bytes each, .data, zero until built), 0x00306DA0..0x00306E27,
 * in __tf order: v x l i s b c w r d f Ui Ul Ux Us Uc Sc.  Names "v".."Sc"
 * @0x00348560..0x003485E0 (.rodata, 8-aligned). */
extern void *__tiv[2];      /* 0x00306DA0 */
extern void *__tix[2];      /* 0x00306DA8 */
extern void *__til[2];      /* 0x00306DB0 */
extern void *__tii[2];      /* 0x00306DB8 */
extern void *__tis[2];      /* 0x00306DC0 */
extern void *__tib[2];      /* 0x00306DC8 */
extern void *__tic[2];      /* 0x00306DD0 */
extern void *__tiw[2];      /* 0x00306DD8 */
extern void *__tir[2];      /* 0x00306DE0 */
extern void *__tid[2];      /* 0x00306DE8 */
extern void *__tif[2];      /* 0x00306DF0 */
extern void *__tiUi[2];     /* 0x00306DF8 */
extern void *__tiUl[2];     /* 0x00306E00 */
extern void *__tiUx[2];     /* 0x00306E08 */
extern void *__tiUs[2];     /* 0x00306E10 */
extern void *__tiUc[2];     /* 0x00306E18 */
extern void *__tiSc[2];     /* 0x00306E20 */

} /* extern "C" */

/* ------------------------------------------------------------------------ */
/* The dynamic_cast lowering of tinfo2.cc                                   */
/* ------------------------------------------------------------------------ */

/* GCC 2.9x vtable entry: {s16 delta; s16 index; pfn}.  Entry 0's pfn is the
 * class's __tf typeinfo function. */
struct egcs_vtable_entry {
    s16 delta;                                      /* +0x00 (lh) */
    s16 index;                                      /* +0x02 */
    const type_info &(*pfn)(void);                  /* +0x04 (lw) */
};

/* dynamic_cast<const T *>(node) for a node of static type `const type_info *`,
 * as GCC 2.9x emits it inline at every use in tinfo2.cc:
 *     node == 0 ? 0
 *               : __dynamic_cast(vptr(node)[0].pfn, target_tf, 1,
 *                                (char *)node + vptr(node)[0].delta,
 *                                __tf9type_info, node)
 * vptr(node) is the word at node+4 (type_info layout {_name, vptr}).  In the
 * GCC 14 build the C++ dynamic_cast operator is NOT used: modern g++ lowers
 * it to the Itanium-ABI __dynamic_cast, which would bind to the egcs
 * extern "C" one.  The egcs build uses the operator itself, as tinfo2.cc
 * does (TINFO2_DYNAMIC_CAST below): egcs's own lowering loads the target's
 * __tf after the vptr (the hand-written form loaded it first, in the
 * prologue) and calls the extern "C" __dynamic_cast T549_tinfo2.h declares.
 * The __tf names of the local classes are egcs's own implicit declarations
 * there: T548 / T549 declare them for GCC 14 only. */
template <class T>
static inline const T *
egcs_dynamic_cast(const type_info *node, const type_info &(*target_tf)(void))
{
    if (node == 0)
        return 0;
    const egcs_vtable_entry *vtbl
        = *(const egcs_vtable_entry *const *)((const char *)node + 4);
    return (const T *)__dynamic_cast(vtbl[0].pfn, target_tf, 1,
                                     (char *)node + vtbl[0].delta,
                                     __tf9type_info, (void *)node);
}

/* dynamic_cast<const T *>(node), node a `const type_info *`; target_tf is
 * T's typeinfo function (used by the GCC 14 form only). */
#ifdef BOOT_OLD_GCC
#define TINFO2_DYNAMIC_CAST(T, node, target_tf) dynamic_cast<const T *>(node)
#else
#define TINFO2_DYNAMIC_CAST(T, node, target_tf) egcs_dynamic_cast<T>((node), (target_tf))
#endif

#endif /* U136_H */
