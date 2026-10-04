/* cxxrt.h - GCC 2.9x-ee C/C++ runtime entry points of the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102) that are referenced across decompilation units.
 *
 * Kept minimal: only functions called explicitly from another unit (crt0 /
 * crtbegin glue, the EH runtime <-> new/tinfo, libkernl printf <-> libgcc
 * math) or that appear in vtables.  C linkage; all names are the GCC 2.9x
 * libgcc2.c / exception.cc / frame.c / tinfo2.cc spellings.
 *
 * Game C++ code does NOT call these by name: `new`/`delete`, `throw`, RAII
 * cleanups and catch clauses are written as normal C++ (see DECOMP_RULES.md).
 * In the binary, compiler-emitted EH landing pads end in `jal __throw`
 * (0x01017CA8 - GCC 2.9x's resume-unwinding call) and nothrow regions in
 * `jal std::terminate` (0x010180D8).
 *
 * Owners: libgcc_eh (runtime/libgcc2.c, runtime/eh.c, runtime/exception.cc),
 * frame (runtime/frame.c), cxxrt (runtime/new.cc, tinfo.cc, tinfo2.cc),
 * libgcc_math (runtime/libgcc_math.c).
 */
#ifndef BOOT_CXXRT_H
#define BOOT_CXXRT_H

#include "types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque runtime structures (layouts: ref/map_notes.md map:08/map:09). */
struct object;            /* frame.c registration record, 0x18 bytes            */
struct frame_state;       /* frame.c unwind state, ~0x2F0 bytes                 */
struct eh_context;        /* libgcc2 per-thread EH context, 0x10 (+8 top_elt)   */
struct cp_eh_info;        /* exception.cc thrown-object record, 0x30 bytes      */
struct __eh_info;         /* common head of cp_eh_info                          */

/* ---- libgcc2.c: global ctors/dtors -------------------------------------- */
/* The collect2-generated ctor/dtor lists walked by __do_global_ctors /
 * __do_global_dtors: word 0 = count (3 here; -1 would mean "count the
 * entries"), then the function pointers, then a 0 terminator.  Defined in
 * src/crtbegin.c (main unit):
 *   __CTOR_LIST__ @0x01026C98 = {3, 0x01003CC0, 0x01011480, 0x010000C8, 0}
 *   __DTOR_LIST__ @0x01026CB0 = {3, 0x01003C90, 0x01011458, 0x01000108, 0} */
extern void (*__CTOR_LIST__[])(void);                 /* @0x01026C98 (.data) */
extern void (*__DTOR_LIST__[])(void);                 /* @0x01026CB0 (.data) */

void  __main(void);                                   /* 0x010170E0 called first thing in main */
void  __do_global_ctors(void);                        /* 0x01017030 walks __CTOR_LIST__ @0x01026C98 backwards */
void  __do_global_dtors(void);                        /* 0x01016FD8 */

/* ---- libgcc2.c: exception handling (L_eh) ------------------------------- */
void  __default_terminate(void);                      /* 0x01017100 abort() */
void  __terminate(void);                              /* 0x01017110 (*__terminate_func)() */
void *__throw_type_match(void *catch_type, void *throw_type, void *obj); /* 0x01017138 */
void  __empty(void);                                  /* 0x01017168 */
void *__get_eh_context(void);                         /* 0x010171C8 */
void **__get_eh_info(void);                           /* 0x010171F0 */
void ***__get_dynamic_handler_chain(void);            /* 0x010172A8 */
void  __sjthrow(void);                                /* 0x010172D0 */
void  __sjpopnthrow(void);                            /* 0x01017448 */
int   __eh_rtime_match(void *rtime);                  /* 0x01017560 */
void  __unwinding_cleanup(void);                      /* 0x01017970 */
/* __throw: unwind to the handler for the exception in the current eh_context.
 * Also the resume call at the end of every cleanup landing pad in game code. */
void  __throw(void);                                  /* 0x01017CA8 */
void  __rethrow(void *index);                         /* 0x01017E88 (no direct callers) */

/* ---- libgcc2.c: 64-bit multiply (R5900 has no dmult) --------------------- */
long long __muldi3(long long a, long long b);         /* 0x01018070 (bignum, __muldf3) */

/* ---- exception.cc -------------------------------------------------------- */
/* GCC 2.9x __pure_virtual: `j __terminate`; fills abstract-class vtable slots
 * (e.g. ImageLoader::Source @0x0103E258) and the .rodata words 0x0103E26C/74. */
void  __pure_virtual(void);                           /* 0x010180D0 */
void *__cp_exception_info(void);                      /* 0x01018138 */
struct cp_eh_info *__cp_eh_info(void);                /* 0x01018160 */
struct cp_eh_info *__start_cp_handler(void);          /* 0x01018180 */
void *__eh_alloc(size_t size);                        /* 0x010181B8 malloc; terminate on NULL */
void  __eh_free(void *p);                             /* 0x010181E0 */
void *__cplus_type_matcher(struct __eh_info *info, void *match_info, void *exception_table); /* 0x01018200 */
void  __cp_push_exception(void *value, void *type, void (*cleanup)(void *, int)); /* 0x01018270 */
void  __cp_pop_exception(struct cp_eh_info *p);       /* 0x01018300 */
void  __uncatch_exception(void);                      /* 0x010183C8 */
void  __check_eh_spec(int n, const void **spec);      /* 0x010183F8 */
void  __throw_bad_cast(void);                         /* 0x010185B0 */
void  __throw_bad_typeid(void);                       /* 0x01018630 */

/* ---- frame.c (DWARF2 unwind-info registry) -------------------------------- */
/* crtbegin (main unit): reg_frame registers .eh_frame @.data via
 * __register_frame_info_table; dereg_frame deregisters it.                 */
void  __register_frame_info(void *begin, struct object *ob);        /* 0x010193C8 */
void  __register_frame(void *begin);                                /* 0x010193F0 */
void  __register_frame_info_table(void *begin, struct object *ob);  /* 0x01019420 */
void  __register_frame_table(void *begin);                          /* 0x01019448 */
struct object *__deregister_frame_info(void *begin);                /* 0x01019478 */
void  __deregister_frame(void *begin);                              /* 0x010194F0 */
struct frame_state *__frame_state_for(void *pc, struct frame_state *state); /* 0x01019510 */

/* ---- tinfo2.cc helpers used by the EH runtime ------------------------------ */
void *__throw_type_match_rtti_2(const void *catch_type, const void *throw_type, void *objptr); /* 0x0101A298 */
int   __is_pointer(void *type_info_ptr);                            /* 0x0101A760 */

/* ---- libgcc arithmetic used by libkernl's printf (compiler-emitted) ------- */
long long          __divdi3(long long a, long long b);              /* 0x01023678 */
long long          __moddi3(long long a, long long b);              /* 0x01023EF0 */
unsigned long long __udivdi3(unsigned long long a, unsigned long long b); /* 0x01024558 */
unsigned long long __umoddi3(unsigned long long a, unsigned long long b); /* 0x01024B28 */

/* libgcc2 DF<->DI conversions and the dp-bit soft-double routines (the EE
 * FPU is single precision only).  This toolchain's libgcc uses the US
 * Software GOFAST libcall names (evidence in runtime/libgcc_math.c); the
 * standard libgcc name is in brackets.  Doubles are passed/returned as
 * 64-bit values in GPRs.  Callers: libkernl _print_double (0x0101CB40),
 * _kernel_vprintf (0x0101CCA8).  Note: negation compiles to dpsub(0.0, x). */
unsigned long long __fixunsdfdi(double a);                          /* 0x01023D68 */
double       __floatdidf(long long u);                              /* 0x01023E58 */
double       dpadd(double a, double b);  /* 0x01025478 [__adddf3] */
double       dpsub(double a, double b);  /* 0x010254D0 [__subdf3] */
double       dpmul(double a, double b);  /* 0x01025538 [__muldf3] */
double       dpdiv(double a, double b);  /* 0x010257E0 [__divdf3] */
int          dpcmp(double a, double b);  /* 0x01025A60 [__cmpdf2] <0/0/>0, NaN -> 1 */
double       litodp(int i);              /* 0x01025AB0 [__floatsidf] */
int          dptoli(double a);           /* 0x01025B68 [__fixdfsi] (no callers) */
unsigned int dptoul(double a);           /* 0x01025C00 [__fixunsdfsi] */
double       __negdf2(double a);         /* 0x01025CA0 (no callers) */
float        dptofp(double a);           /* 0x01025D08 [__truncdfsf2] (no callers) */
double       fptodp(float a);            /* 0x01026900 [__extendsfdf2] */

#ifdef __cplusplus
}
#endif

/* C++-only runtime entry points (operator new/delete, std::terminate,
 * set_new_handler, type_info and friends) are declared by the language /
 * the cxxrt unit's own sources, not here:
 *   operator new(size_t)      0x01019988   operator delete(void*)    0x01019910
 *   operator new[](size_t)    0x01019B28   operator delete[](void*)  0x01019AB0
 *   std::terminate()          0x010180D8   std::unexpected()         0x01018120
 *   __si_type_info ctor       0x01019C38   __user_type_info ctor     0x01019C58 */

/* ---- tinfo.cc builders (appended by the cxxrt unit) ----------------------
 * Correction to the list above: 0x01019C38 / 0x01019C58 are the extern "C"
 * placement builders __rtti_si / __rtti_user that every compiler-generated
 * __tf<class> function calls (null-checked `new (addr) X(...)`, no return
 * value).  The real out-of-line ctors are __si_type_info 0x01019FA8 and
 * __user_type_info 0x0101A038 (both unused).  A game __tf function is:
 *   if (first word of __ti<X> == 0) { __tf<Base>(); __rtti_si(&__ti<X>, "<X>", &__ti<Base>); }
 *   (or __rtti_user(&__ti<X>, "<X>") for a class without bases); return __ti<X>;
 * Class declarations: runtime/tinfo.h, runtime/new.h. */
#ifdef __cplusplus
class type_info;
extern "C" {
void __rtti_si(void *addr, const char *n, const type_info *ti);     /* 0x01019C38 */
void __rtti_user(void *addr, const char *name);                     /* 0x01019C58 */
}
#endif

#endif /* BOOT_CXXRT_H */
