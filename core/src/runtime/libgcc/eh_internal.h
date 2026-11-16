/* eh_internal.h - GCC 2.9x-ee exception-handling runtime (libgcc2.c, L_eh block),
 * translation unit T542, 0x002776C0..0x00278630 of the GT3 US main executable.
 *
 * The same machine code as the bootstrap's runtime/eh.c (0x01017100..).  The
 * shared structures (__eh_info, exception_lang_info, exception_table,
 * exception_descriptor, frame_state) are in the canonical runtime/libgcc/T542_eh.h;
 * this header keeps what only T542 uses.
 */
#ifndef U132_H
#define U132_H

#include "types.h"
#include "libc.h"                   /* abort 0x0027D7A0, strcmp 0x0027EA40, memset 0x0027DB38,
                                     * memcpy 0x0027D990, malloc 0x0026C3A8 (pdistd allocator) */
#include "cxxrt.h"                  /* prototypes of the public L_eh entry points (bootstrap addresses) */
#include "runtime/libgcc/T542_eh.h"           /* __eh_info, exception_descriptor, frame_state, ... */
#include "runtime/libgcc/G006_fp_bit.h"       /* __frame_state_for 0x0027BCC0 (frame.c) */

/* ------------------------------------------------------------------------ */
/* Types local to T542                                                      */
/* ------------------------------------------------------------------------ */

/* Per-thread EH context (gcc/libgcc2.c).  Other units reach it only through
 * __get_eh_context / __get_eh_info (cxxrt.h forward-declares the struct). */
struct eh_context {
    void *handler_label;                /* +0x0 handler address found by throw_helper */
    void **dynamic_handler_chain;       /* +0x4 sjlj: top of the dynamic handler stack */
    void *info;                         /* +0x8 language info (cp_eh_info *) of the exception in flight */
    void *table_index;                  /* +0xC rethrow: exception_table entry to rethrow from */
};
_Static_assert(sizeof(struct eh_context) == 0x10, "eh_context size");

/* Old-model exception table: {start, end, handler} triples (no match_info,
 * no exception_descriptor head); a start_region of -1 ends it. */
typedef struct old_exception_table {
    void *start_region;                 /* +0x0 */
    void *end_region;                   /* +0x4 */
    void *exception_handler;            /* +0x8 */
} old_exception_table;
_Static_assert(sizeof(old_exception_table) == 0xC, "old table entry");

/* start_region of the terminator entry of both table models. */
#define EH_TABLE_END ((void *)-1)

/* sjlj cleanups are egcs destructors, called with in-charge flags 2
 * (destroy, do not delete). */
#define CLEANUP_IN_CHARGE 2

/* No register windows on MIPS. */
#define in_reg_window(REG, UDATA) 0

/* The static helpers exist as real functions in the binary. */
#define NOINLINE __attribute__((noinline))

/* ------------------------------------------------------------------------ */
/* Data of this translation unit (defined in T542_eh.c)                     */
/* ------------------------------------------------------------------------ */

/* The terminate handler; std::set_terminate (exception.cc, T543) swaps it.
 * Initial value: __default_terminate (0x002776C0).  Declared without
 * noreturn here: __terminate returns through a full epilogue, while T543
 * (exception_internal.h) declares it noreturn, as egcs's exception.cc does. */
extern void (*__terminate_func)(void);              /* 0x00305EBC (.data) */

/* T542_eh.c's file statics (no extern here: GCC 14 rejects a static
 * definition after an extern declaration):
 *   get_eh_context                 0x00305EC0 (.data, = eh_context_initialize)
 *   eh_context_static's eh         0x004AD208 (.bss)
 *                       initialized 0x004AD218 (.bss)
 *                       top_elt     0x004AD220 (.bss) */

#endif /* U132_H */
