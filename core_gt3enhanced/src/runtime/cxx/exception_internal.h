/* exception_internal.h - egcs (GCC 2.9x-ee) cp/exception.cc, translation unit T543,
 * 0x00279CA8..0x0027A400 of the GT3 US main executable.
 *
 * The same machine code as the bootstrap's runtime/exception.cc
 * (0x010180D8..0x01018830).  The shared declarations are in the canonical
 * headers:
 *   runtime/cxx/T543_exception.h  cp_eh_info, cleanup_fn, exception, bad_exception,
 *                          __default_unexpected, this unit's EH entry points,
 *                          __tf9exception / __tf13bad_exception, __ti9exception
 *   runtime/libgcc/T542_eh.h         __eh_info, __eh_matcher, exception_descriptor,
 *                          __get_eh_info 0x002777B0, __throw 0x00278268
 *   runtime/cxx/T548_tinfo.h      type_info, bad_cast, bad_typeid, __rtti_si
 *                          0x0027C3E8, __rtti_user 0x0027C408
 *   runtime/cxx/T549_tinfo2.h     __throw_type_match_rtti_2 0x0027CA48, __is_pointer
 *                          0x0027CF10
 *   runtime/cxx/T550_new.h        placement operator new(size_t, void *) 0x0027C028
 *                          (modern build: `throw X()` is written out as its egcs
 *                          lowering; egcs inlines <new>'s copy)
 * This header keeps only what T543 alone uses.
 *
 * egcs ignores `namespace std` (-fno-honor-std): the classes are mangled
 * "9exception" / "13bad_exception" and live at global scope; the functions
 * keep their std:: names from the symbol map.
 */
#ifndef U134_H
#define U134_H

#include "types.h"
#include "libc.h"                   /* malloc 0x0026C3A8, free 0x0026C380 (pdistd allocator, T532) */
#include "runtime/libgcc/T542_eh.h"
#include "runtime/cxx/T543_exception.h"    /* under egcs also <exception> */
#include "runtime/cxx/T548_tinfo.h"
#include "runtime/cxx/T549_tinfo2.h"
#include "runtime/cxx/T550_new.h"
#include <stddef.h>

/* ------------------------------------------------------------------------ */
/* std:: entry points                                                       */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* egcs declares these in its <exception> (included by T543_exception.h).
 * `noexcept` only to agree with GCC 14's built-in idea of std::terminate;
 * egcs had no exception specifications on them. */
namespace std {
typedef void (*terminate_handler)();
typedef void (*unexpected_handler)();

void terminate() noexcept __attribute__((__noreturn__));                /* 0x00279CA8 */
void unexpected() __attribute__((__noreturn__));                        /* 0x00279CF0 */
terminate_handler set_terminate(terminate_handler func) noexcept;       /* 0x00279CD0 */
unexpected_handler set_unexpected(unexpected_handler func) noexcept;    /* 0x00279CE0 */
bool uncaught_exception() noexcept;                                     /* 0x0027A280 */
}
#endif

/* ------------------------------------------------------------------------ */
/* Constants (gcc/eh-common.h, cp/exception.cc)                             */
/* ------------------------------------------------------------------------ */

/* match_info of a catch (...) clause. */
#define CATCH_ALL_TYPE ((void *)-1)
/* __eh_info.language / exception_lang_info.language of C++ (DW_LANG_C_plus_plus). */
#define EH_LANG_C_plus_plus 4
/* __eh_info.version of a cp_eh_info ("there is only one version"). */
#define CP_EH_VERSION 1
/* egcs destructor flags for a thrown object's cleanup: in charge (destroy
 * the whole object), bit 0 clear = do not delete (build_delete()). */
#define CLEANUP_IN_CHARGE 2

/* The current exception record: head of eh_context.info, the list of C++
 * exceptions in flight (__get_eh_info returns its address). */
#define CP_EH_INFO ((cp_eh_info *)*__get_eh_info())

/* ------------------------------------------------------------------------ */
/* Data (not defined here)                                                  */
/* ------------------------------------------------------------------------ */

/* libgcc2 (T542) .data: the terminate handler, initially __default_terminate
 * (0x002776C0).  Declared noreturn as exception.cc does, so std::terminate
 * has no epilogue (T542's own __terminate, which sees it without the
 * attribute, has one).  Not in T542_eh.h on purpose (u132 notes). */
extern "C" void (*__terminate_func)(void) __attribute__((__noreturn__));    /* 0x00305EBC (.data) */

/* (__unexpected_func, exception.cc's `static` unexpected handler at
 * 0x003064B4, is defined in T543_exception.cpp.) */

#ifndef BOOT_OLD_GCC
/* bad_exception's __si_type_info node, placement-built by
 * __tf13bad_exception on first use; word 0 (_name) == 0 means "not built
 * yet".  (__ti9exception, 0x00353510, is in T543_exception.h.)  egcs emits
 * the node itself and rejects a hand declaration. */
extern "C" void *__ti13bad_exception[3];    /* 0x004B2B30 (.bss) */
#endif

#endif /* U134_H */
