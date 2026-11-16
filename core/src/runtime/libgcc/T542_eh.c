/* T542_eh.c - libgcc2.c L_eh block (GCC 2.9x-ee exception-handling runtime),
 * 0x002776C0..0x00278630 of the GT3 US main executable.
 *
 * C source (libgcc2.c), although the translation unit has an FDE block like
 * the C++ units (libgcc2 is built with -fexceptions; ref/compile_flags.json
 * gives this file -O2 -G0 -fexceptions: in C, -fexceptions keeps the final
 * calls as jal with a frame, as __default_terminate's `jal abort` and
 * __sjpopnthrow's `jal __sjthrow` show).  All 24 functions are the
 * same machine code as the bootstrap's runtime/eh.c (0x01017100..0x01018070):
 * the source below is that file with the core addresses.
 *
 * Functions are in libgcc2.c source order: terminate hooks, the old string
 * type matcher, the per-thread eh_context (single-threaded build: one static
 * context), the setjmp/longjmp ("sjlj") throw pair, the exception-table
 * searchers, and the DWARF2 frame-unwinding throw (__throw / __rethrow +
 * helpers) that the game's C++ code actually uses: __throw has 581 call
 * sites in the core, nearly all of them the "resume unwinding" call at the
 * end of a cleanup landing pad.
 *
 * Build configuration evident from the code:
 *   - DWARF2_UNWIND_INFO: __throw/__rethrow walk frames with frame.c's
 *     __frame_state_for (0x0027BCC0) and return into the handler through
 *     __builtin_eh_return.
 *   - __builtin_setjmp buffers are {fp, receiver label, sp}.
 *   - no gthreads: get_eh_context -> eh_context_initialize -> eh_context_static.
 *   - inhibit_libc: no diagnostics are printed.
 *   - word_type is 64-bit on the EE (frame_state fields).
 *   - 79 DWARF frame registers; saved-register copy widths: 16 bytes for regs
 *     0..30 (128-bit GPRs), 4 for 31..63, 8 for >= 64 (see copy_reg).
 *   - in_reg_window() is 0 on MIPS, so the SPARC register-window code of the
 *     generic source folds away (a couple of dead loads remain, noted).
 *
 * Shared structures and the entry points other units call: runtime/libgcc/T542_eh.h;
 * eh_context, the old-model table and __terminate_func's extern: eh_internal.h.
 * Local names are libgcc2.c's own.
 *
 * Data, defined here as libgcc2.c defines it (the object is the toolchain's
 * libgcc.a _eh.o: .text, .data with its relocations, .bss and the symbol
 * table are the same; ref/data/u132/notes.md):
 *   .data 0x00305EBC __terminate_func = __default_terminate (global)
 *         0x00305EC0 get_eh_context   = eh_context_initialize (static)
 *         0x00305EC4 the frame info (_GLOBAL_$F$__default_terminate)
 *   .bss  0x004AD208 eh_context_static's local statics eh, initialized, top_elt
 */
#include "runtime/libgcc/eh_internal.h"
#include "runtime/libgcc/T542_eh.h"

/* ------------------------------------------------------------------------ */
/* terminate                                                                */
/* ------------------------------------------------------------------------ */

// FUNC 0x002776C0 __default_terminate
/* Default terminate handler: abort() (0x0027D7A0), no return, no epilogue.
 * Its address is the initial value of __terminate_func (0x00305EBC). */
void __default_terminate(void)
{
    abort();
}

/* The terminate handler; std::set_terminate (exception.cc, T543) swaps it.
 * The file's first .data word (libgcc2.c defines it here, after the first
 * function, so the frame info keeps the name _GLOBAL_$F$__default_terminate). */
void (*__terminate_func)(void) = __default_terminate;  /* 0x00305EBC */

// FUNC 0x002776D0 __terminate
/* Call the current terminate handler.  Unlike std::terminate this one is
 * compiled as returning (full epilogue), and several callers (new_eh_context,
 * __sjthrow, throw_helper, __throw, __rethrow) have code after the call. */
void __terminate(void)
{
    (*__terminate_func)();
}

// FUNC 0x002776F8 __throw_type_match
/* Old (pre-RTTI) type matcher: types are compared as mangled-name strings
 * (strcmp 0x0027EA40).  Returns obj on match, else 0 (movn).  No callers. */
void *__throw_type_match(void *catch_type, void *throw_type, void *obj)
{
    if (strcmp((const char *)catch_type, (const char *)throw_type) == 0)
        return obj;
    return 0;
}

// FUNC 0x00277728 __empty
/* Empty function (jr ra).  Unreferenced; has its own FDE. */
void __empty(void)
{
}

/* ------------------------------------------------------------------------ */
/* eh_context                                                               */
/* ------------------------------------------------------------------------ */

// FUNC 0x00277730 new_eh_context
/* Allocate a zeroed eh_context followed by its 2-word top_elt, with
 * dynamic_handler_chain pointing at top_elt.  Only used by the gthreads
 * variant of eh_context_initialize, which is not built: dead code (no
 * callers).  If malloc (0x0026C3A8) fails __terminate() is called and, should
 * it return, memset runs on the NULL pointer.  The source's
 * "&ehfc->c != ehfc" sanity check folded away. */
static __attribute__((used)) struct eh_context *new_eh_context(void)
{
    struct eh_full_context {
        struct eh_context c;            /* +0x00 */
        void *top_elt[2];               /* +0x10 */
    } *ehfc = (struct eh_full_context *)malloc(sizeof *ehfc);   /* 0x18 */

    if (!ehfc)
        __terminate();

    memset(ehfc, 0, sizeof *ehfc);

    ehfc->c.dynamic_handler_chain = (void **)ehfc->top_elt;

    return &ehfc->c;
}

/* Pointer to the function returning the EH context (libgcc2.c's order: the
 * declarations and the pointer follow new_eh_context). */
static __attribute__((used)) struct eh_context *eh_context_initialize(void);
static struct eh_context *eh_context_static(void);

/* Returns the eh_context of the current thread; eh_context_initialize
 * replaces itself with eh_context_static.  The file's second .data word. */
static struct eh_context *(*get_eh_context)(void) = &eh_context_initialize;  /* 0x00305EC0 */

// FUNC 0x00277788 __get_eh_context
/* Public accessor: return (*get_eh_context)(). */
void *__get_eh_context(void)
{
    return (void *)(*get_eh_context)();
}

// FUNC 0x002777B0 __get_eh_info
/* Return &eh->info (eh_context +8): the slot holding the current exception's
 * language record (cp_eh_info *).  Used by all of exception.cc (T543). */
void **__get_eh_info(void)
{
    struct eh_context *eh = (*get_eh_context)();
    return &eh->info;
}

// FUNC 0x002777D8 eh_context_initialize
/* First-call hook (non-threaded build): point get_eh_context at
 * eh_context_static and call it (the call goes through the just-computed
 * register value; the store is in the jalr delay slot).  Referenced only
 * as the initial value of get_eh_context (0x00305EC0). */
static __attribute__((used)) struct eh_context *eh_context_initialize(void)
{
    get_eh_context = &eh_context_static;
    return (*get_eh_context)();
}

// FUNC 0x00277800 eh_context_static
/* The single static eh_context, zeroed on first use, with
 * dynamic_handler_chain = top_elt.  The file's .bss, 0x004AD208..0x004AD228
 * (eh +0x0, initialized +0x10, top_elt +0x18: 8-aligned array). */
static struct eh_context *eh_context_static(void)
{
    static struct eh_context eh;        /* .bss 0x004AD208 */
    static int initialized;             /* .bss 0x004AD218 */
    static void *top_elt[2];            /* .bss 0x004AD220 */

    if (!initialized) {
        initialized = 1;
        memset(&eh, 0, sizeof eh);
        eh.dynamic_handler_chain = top_elt;
    }
    return &eh;
}

// FUNC 0x00277868 __get_dynamic_handler_chain
/* Return &eh->dynamic_handler_chain (eh_context +4).  Unused in this binary
 * (the game is compiled for the frame-unwinding model, not sjlj). */
void ***__get_dynamic_handler_chain(void)
{
    struct eh_context *eh = (*get_eh_context)();
    return &eh->dynamic_handler_chain;
}

/* ------------------------------------------------------------------------ */
/* setjmp/longjmp ("sjlj") throw                                            */
/*                                                                          */
/* Dynamic handler chain element: [0] = next element, [1] = cleanup chain,  */
/* [2..] = __builtin_setjmp buffer {fp, resume label, sp}.                  */
/* Cleanup chain element: [0] = next, [1] = func(arg, int), [2] = arg.      */
/* Only reachable via __sjpopnthrow, which nothing calls.                   */
/* ------------------------------------------------------------------------ */

/* libgcc2.c's own declaration (cxxrt.h has the plain prototype): __sjpopnthrow
 * ends in `jal __sjthrow` with no epilogue after it. */
extern void __sjthrow(void) __attribute__((__noreturn__));

// FUNC 0x00277890 __sjthrow
/* Throw via the dynamic handler chain: run the pending cleanups of the top
 * handler (each called as func(arg, 2)) under a temporary handler whose
 * receiver (0x00277948) calls __terminate if a cleanup throws; then, if there
 * is no exception info or no next handler, __terminate(); then pop the top
 * handler and __builtin_longjmp to its buffer (restores fp and sp, jumps to
 * the saved label).  Never returns.  Frame 0x750 with all callee-saved GPRs
 * and $f20-$f31 saved (setjmp receiver).  Only caller: __sjpopnthrow. */
void __sjthrow(void)
{
    struct eh_context *eh = (*get_eh_context)();
    void ***dhc = &eh->dynamic_handler_chain;
    void *jmpbuf;
    void (*func)(void *, int);
    void *arg;
    /* The cleanup chain is one word into the buffer. */
    void ***cleanup = (void ***)&(*dhc)[1];

    /* If there are any cleanups in the chain, run them now. */
    if (cleanup[0]) {
        double store[200];              /* fp+0 .. fp+0x640 */
        void **buf = (void **)store;
        buf[1] = 0;
        buf[0] = (*dhc);

        /* try { */
        if (!__builtin_setjmp(&buf[2])) {
            *dhc = buf;
            while (cleanup[0]) {
                func = (void (*)(void *, int))cleanup[0][1];
                arg = (void *)cleanup[0][2];

                /* Update this before running the cleanup. */
                cleanup[0] = (void **)cleanup[0][0];

                (*func)(arg, CLEANUP_IN_CHARGE);
            }
            *dhc = buf[0];
        }
        /* catch (...) */
        else {
            __terminate();
        }
    }

    /* terminate on a rethrow with no active exception, or with no handler left */
    if (!eh->info || (*dhc)[0] == 0)
        __terminate();

    /* The jmpbuf starts two words into the top element. */
    jmpbuf = &(*dhc)[2];

    /* Pop the top element off the dynamic handler chain. */
    *dhc = (void **)(*dhc)[0];

    /* And jump to the handler. */
    __builtin_longjmp(jmpbuf, 1);
}

// FUNC 0x00277A08 __sjpopnthrow
/* Run the top handler's cleanups (as __sjthrow does), pop the top handler
 * and throw to the next one via __sjthrow (jal, noreturn).  No callers.
 *
 * QUIRK (compiler): the source is `if (!__builtin_setjmp(&buf[2])) {...}
 * else __terminate();` (libgcc2.c's, as in __sjthrow; egcs with -fexceptions
 * gives the listing's words from it), but in the binary the setjmp receiver
 * label is 0x00277AB0 - the first instruction of the "returned 0" path - and
 * there is no __terminate call at all.  So a cleanup that throws resumes the
 * cleanup loop (re-installing buf) instead of terminating.  The GCC 14 branch
 * reproduces that by ignoring the setjmp result.  sp-based frame (0x750),
 * buf[2] = sp. */
void __sjpopnthrow(void)
{
    struct eh_context *eh = (*get_eh_context)();
    void ***dhc = &eh->dynamic_handler_chain;
    void (*func)(void *, int);
    void *arg;
    void ***cleanup;

    /* The cleanup chain is one word into the buffer. */
    cleanup = (void ***)&(*dhc)[1];

    /* If there are any cleanups in the chain, run them now. */
    if (cleanup[0]) {
        double store[200];              /* sp+0 .. sp+0x640 */
        void **buf = (void **)store;
        buf[1] = 0;
        buf[0] = (*dhc);

        /* try { */
#ifdef BOOT_OLD_GCC
        if (!__builtin_setjmp(&buf[2]))
#else
        (void)__builtin_setjmp(&buf[2]);    /* receiver == fall-through, see QUIRK */
#endif
        {
            *dhc = buf;
            while (cleanup[0]) {
                func = (void (*)(void *, int))cleanup[0][1];
                arg = (void *)cleanup[0][2];

                /* Update this before running the cleanup. */
                cleanup[0] = (void **)cleanup[0][0];

                (*func)(arg, CLEANUP_IN_CHARGE);
            }
            *dhc = buf[0];
        }
#ifdef BOOT_OLD_GCC
        /* catch (...): egcs drops this arm (QUIRK) */
        else {
            __terminate();
        }
#endif
    }

    /* Pop the top element off the dynamic handler chain. */
    *dhc = (void **)(*dhc)[0];

    __sjthrow();
}

/* ------------------------------------------------------------------------ */
/* Region-table searching                                                   */
/* ------------------------------------------------------------------------ */

// FUNC 0x00277B20 __eh_rtime_match
/* Ask the current exception's matcher whether it matches runtime type rtime:
 * info = *__get_eh_info(); return info->match_function
 * ? (*match_function)(info, rtime, 0) != 0 : 0.  Unreferenced here. */
int __eh_rtime_match(void *rtime)
{
    void *info;
    __eh_matcher matcher;
    void *ret;

    info = *(__get_eh_info());
    matcher = ((__eh_info *)info)->match_function;
    if (!matcher)
        return 0;               /* inhibit_libc: no "No runtime type matcher" message */
    ret = (*matcher)(info, rtime, (void *)0);
    return (ret != NULL);
}

// FUNC 0x00277B68 __get_eh_table_version
/* return table->lang.version (lh +6). */
short __get_eh_table_version(exception_descriptor *table)
{
    return table->lang.version;
}

// FUNC 0x00277B70 __get_eh_table_language
/* return table->lang.language (lh +4). */
short __get_eh_table_language(exception_descriptor *table)
{
    return table->lang.language;
}

// FUNC 0x00277B78 old_find_exception_handler
/* Old-model table search (12-byte {start, end, handler} entries, start == -1
 * terminates): pick the innermost region with start <= pc < end (unsigned
 * compares).  The table is not sorted across functions, so the scan is
 * linear; once a match exists, an entry starting above pc ends the search.
 * Returns the handler or 0. */
static NOINLINE void *old_find_exception_handler(void *pc, old_exception_table *table)
{
    if (table) {
        int pos;
        int best = -1;

        for (pos = 0; table[pos].start_region != EH_TABLE_END; ++pos) {
            if (table[pos].start_region <= pc && table[pos].end_region > pc) {
                /* This can apply.  Make sure it is at least as small as
                   the previous best. */
                if (best == -1 || (table[pos].end_region <= table[best].end_region
                                   && table[pos].start_region >= table[best].start_region))
                    best = pos;
            }
            /* But it is sorted by starting PC within a function. */
            else if (best >= 0 && table[pos].start_region > pc)
                break;
        }
        if (best != -1)
            return table[best].exception_handler;
    }

    return (void *)0;
}

// FUNC 0x00277C50 find_exception_handler
/* New-model table search: entries are sorted innermost-first, so the first
 * region containing pc wins.  *cleanup is set to 1 on entry.  Entries with
 * match_info ask eh_info->match_function(eh_info, match_info, table); a match
 * sets *cleanup = 0 and returns the first handler seen (an earlier cleanup's,
 * if any).  Entries without match_info are cleanups: remember the first.
 * For a rethrow, pc is the table entry to rethrow out of: the search starts
 * after it (pos = entry - tab, +1 unless that entry is the terminator) with
 * pc = entry->end_region - 1; otherwise pc-- to stay out of the next region.
 * Returns the handler or NULL. */
static NOINLINE void *find_exception_handler(void *pc, exception_descriptor *table,
                                             __eh_info *eh_info, int rethrow, int *cleanup)
{
    void *retval = NULL;
    *cleanup = 1;
    if (table) {
        int pos = 0;
        exception_table *tab = &(table->table[0]);

        if (rethrow) {
            /* pc is actually the region table entry to rethrow out of */
            pos = ((exception_table *)pc) - tab;           /* (pc - tab) >> 4, signed */
            pc = (char *)((exception_table *)pc)->end_region - 1;

            /* The label is always on the LAST handler entry for a region,
               so the next entry is a different region.  Not past the end. */
            if (tab[pos].start_region != EH_TABLE_END)
                pos++;
        } else
            pc = (char *)pc - 1;

        for (; tab[pos].start_region != EH_TABLE_END; pos++) {
            if (tab[pos].start_region <= pc && tab[pos].end_region > pc) {
                if (tab[pos].match_info) {
                    __eh_matcher matcher = eh_info->match_function;
                    /* match info but no matcher is NOT a match */
                    if (matcher) {
                        void *ret = (*matcher)((void *)eh_info, tab[pos].match_info, table);
                        if (ret) {
                            if (retval == NULL)
                                retval = tab[pos].exception_handler;
                            *cleanup = 0;
                            return retval;
                        }
                    }
                } else {
                    if (retval == NULL)
                        retval = tab[pos].exception_handler;
                }
            }
        }
    }
    return retval;
}

/* ------------------------------------------------------------------------ */
/* Frame unwinding                                                          */
/* ------------------------------------------------------------------------ */

// FUNC 0x00277D98 get_reg_addr
/* Address where register `reg` of the frame described by udata is saved:
 * follow REG_SAVED_REG links (reg = low word of reg_or_offset[reg]; udata
 * does not change - in_reg_window is 0), then for REG_SAVED_OFFSET return
 * cfa + reg_or_offset[reg] (low word); anything else -> abort() (0x0027D7A0).
 * saved[] is read as signed char (lb). */
static NOINLINE void *get_reg_addr(unsigned reg, frame_state *udata, frame_state *sub_udata)
{
    while (udata->saved[reg] == REG_SAVED_REG) {
        reg = (unsigned)udata->reg_or_offset[reg];
        if (in_reg_window(reg, udata)) {
            udata = sub_udata;
            sub_udata = NULL;
        }
    }
    if (udata->saved[reg] == REG_SAVED_OFFSET)
        return (char *)udata->cfa + (int)udata->reg_or_offset[reg];
    else
        abort();
}

/* Inlined in the binary: the saved value of a register (64-bit ld, then
 * truncated/sign-extended to a 32-bit pointer by the callers). */
static inline word_type get_reg(unsigned reg, frame_state *udata, frame_state *sub_udata)
{
    return *(word_type *)get_reg_addr(reg, udata, sub_udata);
}

/* Inlined in the binary: the return address of the frame described by udata. */
static inline void *get_return_addr(frame_state *udata, frame_state *sub_udata)
{
    return (void *)(int)get_reg(udata->retaddr_column, udata, sub_udata);
}

/* The size of a saved register: regs <= 30 -> 16 (128-bit GPRs), 31..63 -> 4
 * (so $31/ra and the FPRs copy 4 bytes), >= 64 -> 8; SIGNED compares
 * (slti/slt: the builtin takes an int).
 * An exact form, not the original: libgcc2.c has __builtin_dwarf_reg_size(reg),
 * which cc1 expands to this chain of `reg <= end ? size : ...` per range of
 * hard registers with one size.  The retail compiler (the reference 991111-01
 * gives copy_reg exact with the builtin) has three ranges; the Windows 991111b
 * has the VU0 registers $vf0-$vf31 as hard registers too, so its expansion gets
 * a fourth range (`reg <= 78 ? 8 : 16`, 88.2%).  Both give the same size for
 * every column copy_reg is called with (0..78).  GCC 14 has no such builtin. */
static inline int dwarf_reg_size(int reg)
{
    return reg <= 30 ? 16 : (reg <= 63 ? 4 : 8);
}

// FUNC 0x00277E20 copy_reg
/* Copy the saved value of `reg` from the frame udata into target_udata's
 * save slot (memcpy 0x0027D990, width per dwarf_reg_size). */
static NOINLINE void copy_reg(unsigned reg, frame_state *udata, frame_state *target_udata)
{
    word_type *preg = get_reg_addr(reg, udata, NULL);
    word_type *ptreg = get_reg_addr(reg, target_udata, NULL);

    memcpy(ptreg, preg, dwarf_reg_size(reg));
}

// FUNC 0x00277EA0 next_stack_level
/* Compute the caller's frame_state (into caller_udata) for return address pc
 * via __frame_state_for (frame.c, 0x0027BCC0); NULL if pc has no FDE.
 * The caller's CFA is its CFA register as saved in udata's frame (if saved:
 * get_reg(...) with sub_udata = 0) or else udata->cfa, plus cfa_offset
 * (low word). */
static NOINLINE frame_state *next_stack_level(void *pc, frame_state *udata,
                                              frame_state *caller_udata)
{
    caller_udata = __frame_state_for(pc, caller_udata);
    if (!caller_udata)
        return 0;

    /* If our caller's CFA register was saved in our stack frame, restore it;
       otherwise, assume the CFA register is SP and restore it to our CFA. */
    if (udata->saved[caller_udata->cfa_reg])
        caller_udata->cfa = (void *)(int)get_reg(caller_udata->cfa_reg, udata, 0);  /* implicit: get_reg_addr (inline get_reg) */
    else
        caller_udata->cfa = udata->cfa;
    caller_udata->cfa = (char *)caller_udata->cfa + (int)caller_udata->cfa_offset;

    return caller_udata;
}

// FUNC 0x00277F30 __unwinding_cleanup
/* Debugger hook: called by throw_helper when only cleanups (no real handler)
 * were found.  Empty; has its own FDE. */
void __unwinding_cleanup(void)
{
}

// FUNC 0x00277F38 throw_helper
/* Find the handler for the exception in eh and prepare the unwind.
 *   *udata_p : in  = the throwing frame's state (__throw's own, cfa set);
 *              out = the frame called by the handler frame (for the SP adjust).
 *   my_udata : a copy of __throw's state whose register save slots are
 *              overwritten with the handler frame's register values.
 * Phase 1 walks up with next_stack_level; per frame it searches the frame's
 * exception table (udata->eh_ptr): the rethrow entry eh->table_index first
 * (once; then cleared), else find_exception_handler for new-model tables
 * (*eh_ptr == -2), else old_find_exception_handler.  A cleanup-only hit is
 * remembered (first one, with a 0x2F0 copy of its frame state) and the walk
 * continues; a real handler stops it.  If only cleanups were found, the
 * first cleanup is used and __unwinding_cleanup() is called.  No handler at
 * all -> __terminate().  eh->handler_label = handler.
 * Phase 2 (handler not in the throwing frame): restart from the throw point
 * and, for each frame up to the handler frame, copy_reg every saved register
 * except the return-address column into my_udata.
 * Frame 0x6A0: ustruct @sp+0, saved_ustruct @sp+0x2F0, cleanup @0x5E0,
 * udata_p @0x5E4, saved_pc @0x5E8, handler_p @0x5EC, pc_p @0x5F0,
 * only_cleanup @0x5F4, saved_state @0x5F8. */
static NOINLINE void *throw_helper(struct eh_context *eh, void *pc, frame_state *my_udata,
                                   frame_state **udata_p)
{
    frame_state *udata = *udata_p;
    frame_state ustruct;
    frame_state *sub_udata = &ustruct;
    void *saved_pc = pc;
    void *handler;
    void *handler_p;                    /* only read when saved_state */
    void *pc_p;                         /* only read when saved_state */
    frame_state saved_ustruct;
    int new_eh_model;
    int cleanup = 0;
    int only_cleanup = 0;
    int rethrow = 0;
    int saved_state = 0;
    __eh_info *eh_info = (__eh_info *)eh->info;

    /* Do we find a handler based on a re-throw PC? */
    if (eh->table_index != (void *)0)
        rethrow = 1;

    handler = (void *)0;
    for (;;) {
        frame_state *callee = udata;    /* libgcc2.c: p */
        udata = next_stack_level(pc, udata, sub_udata);
        sub_udata = callee;             /* the frame udata's frame called */

        /* If we couldn't find the next frame, we lose. */
        if (!udata)
            break;

        if (udata->eh_ptr == NULL)
            new_eh_model = 0;
        else
            new_eh_model = (((exception_descriptor *)(udata->eh_ptr))->runtime_id_field
                            == NEW_EH_RUNTIME);

        if (rethrow) {
            rethrow = 0;
            handler = find_exception_handler(eh->table_index, udata->eh_ptr,
                                             eh_info, 1, &cleanup);
            eh->table_index = (void *)0;
        } else if (new_eh_model)
            handler = find_exception_handler(pc, udata->eh_ptr, eh_info, 0, &cleanup);
        else
            handler = old_find_exception_handler(pc, udata->eh_ptr);

        /* A real handler ends the search; for cleanups, save the state and
           keep looking (to call the debug hook if there are only cleanups). */
        if (handler) {
            if (cleanup) {
                if (!saved_state) {
                    saved_ustruct = *udata;         /* 0x2F0-byte inline copy */
                    handler_p = handler;
                    pc_p = pc;
                    saved_state = 1;
                    only_cleanup = 1;
                }
            } else {
                only_cleanup = 0;
                break;
            }
        }

        /* Continue with the caller; -1 avoids hitting the next region. */
        pc = (char *)get_return_addr(udata, sub_udata) - 1;
    }

    if (saved_state) {
        udata = &saved_ustruct;
        handler = handler_p;
        pc = pc_p;
        if (only_cleanup)
            __unwinding_cleanup();
    }

    /* If we haven't found a handler by now, this is an unhandled exception. */
    if (!handler)
        __terminate();

    eh->handler_label = handler;

    if (pc == saved_pc)
        /* We found a handler in the throw context, no need to unwind. */
        udata = my_udata;
    else {
        int i;

        /* Remember the PC where we found the handler. */
        void *handler_pc = pc;

        /* Start from the throw context again. */
        pc = saved_pc;
        memcpy(udata, my_udata, sizeof(*udata));    /* inline 0x2F0 copy (into *udata!) */

        while (pc != handler_pc) {
            frame_state *callee = udata;
            udata = next_stack_level(pc, udata, sub_udata);   /* not NULL-checked */
            sub_udata = callee;

            for (i = 0; i < DWARF_FRAME_REGISTERS; ++i)
                if (i != udata->retaddr_column && udata->saved[i]) {
                    /* (SPARC register-window exclusion folds away on MIPS) */
                    if (in_reg_window(i, udata)
                        && udata->saved[udata->retaddr_column] == REG_SAVED_REG
                        && udata->reg_or_offset[udata->retaddr_column] == i)
                        continue;
                    copy_reg(i, udata, my_udata);
                }

            pc = (char *)get_return_addr(udata, sub_udata) - 1;  /* implicit: get_reg_addr (inline get_return_addr) */
        }

        /* The source also updates the saved return-address register of the
           last unwound frame if it lives in a register window; on MIPS only
           a dead load of udata->saved[udata->retaddr_column] survives. */
        if (udata->saved[udata->retaddr_column] == REG_SAVED_REG) {
            i = (int)udata->reg_or_offset[udata->retaddr_column];
            if (in_reg_window(i, udata))
                copy_reg(i, udata, my_udata);
        }
    }
    /* udata now refers to the frame called by the handler frame. */

    *udata_p = udata;
    return handler;
}

/* egcs-ee's __builtin_eh_return(context, offset, handler) puts the context in
 * $v0, the SP adjustment in $v1 and the handler in $a0, patches the saved $ra
 * slot of the frame to point at an in-function stub (`jr $a0; addu $sp,$sp,$v1`)
 * and returns through the normal epilogue, which reloads every callee-saved
 * register from the (patched, by copy_reg) save area.  Modern GCC's builtin
 * takes (offset, handler) only; the context argument is noted here.  (egcs's
 * cc1 crashes, without a message, on the 2-argument call.) */
#ifdef BOOT_OLD_GCC
#define EH_RETURN(context, offset, handler) \
    __builtin_eh_return((context), (offset), (handler))
#else
#define EH_RETURN(context, offset, handler) \
    ((void)(context), __builtin_eh_return((offset), (handler)))
#endif

// FUNC 0x00278268 __throw
/* Throw the exception already recorded in the eh_context (callers first run
 * __cp_push_exception).  Also the "resume unwinding" call at the end of every
 * cleanup landing pad in game code (581 callers in the core).
 *   - __terminate() if eh->info is NULL (rethrow with nothing active);
 *   - udata = __frame_state_for(&&label [0x0027830C], udata), __terminate
 *     if NULL; udata->cfa = __builtin_dwarf_cfa() (= fp + 0x6F0);
 *   - my_udata = copy of udata; __builtin_unwind_init() (all callee-saved
 *     GPRs + $f20-$f31 spilled into the frame so copy_reg can patch them);
 *   - pc = return address - 1; handler = throw_helper(eh, pc, my_udata, &udata);
 *   - eh_return: SP += udata->cfa - my_udata->cfa, jump to handler with
 *     $v0 = eh (stub at 0x002783D8).
 * Frame 0x6F0, fp-based: ustruct (udata) @fp+0, ustruct2 (my_udata)
 * @fp+0x2F0, udata @0x5E0, eh @0x5E4, my_udata @0x5E8, &udata @0x5EC. */
void __throw(void)
{
    struct eh_context *eh = (*get_eh_context)();
    void *pc, *handler;
    frame_state ustruct, ustruct2;
    frame_state *udata = &ustruct;
    frame_state *my_udata = &ustruct2;

    /* C++ semantics: terminate on a rethrow with no active exception. */
    if (!eh->info)
        __terminate();

    /* Start at our stack frame. */
label:
    udata = __frame_state_for(&&label, udata);
    if (!udata)
        __terminate();

    /* We need to get the value from the CFA register. */
    udata->cfa = __builtin_dwarf_cfa();

    memcpy(my_udata, udata, sizeof(*udata));        /* inline 0x2F0 copy */

    /* Spill all callee-saved registers into this frame. */
    __builtin_unwind_init();

    /* Now reset pc to the right throw point. */
    pc = (char *)__builtin_extract_return_addr(__builtin_return_address(0)) - 1;

    handler = throw_helper(eh, pc, my_udata, &udata);

    /* Now go! */
    EH_RETURN((void *)eh, (char *)udata->cfa - (char *)my_udata->cfa, handler);

    /* Epilogue: restore the handler frame's register values and return
       to the stub. */
}

// FUNC 0x00278448 __rethrow
/* Like __throw, but first records `index` (the exception-table entry to
 * rethrow from) in eh->table_index (+0xC), after the eh->info check, so that
 * throw_helper resumes the search after that entry.  Label 0x002784F4,
 * eh-return stub 0x002785C0.  No callers in this binary.
 * Frame 0x700: index @0x5E4, eh @0x5E8, my_udata @0x5EC, &udata @0x5F0. */
void __rethrow(void *index)
{
    struct eh_context *eh = (*get_eh_context)();
    void *pc, *handler;
    frame_state ustruct, ustruct2;
    frame_state *udata = &ustruct;
    frame_state *my_udata = &ustruct2;

    /* C++ semantics: terminate on a rethrow with no active exception. */
    if (!eh->info)
        __terminate();

    /* This is the table index we want to rethrow from.  The value of the
       END_REGION label is used for the PC of the throw, and the search
       begins with the next table entry.  (Label 0x002784F4 follows the
       store.  egcs's cc1 aborts on this order without -fexceptions, which
       the file's flag rule has.) */
    eh->table_index = index;

    /* Start at our stack frame. */
label:
    udata = __frame_state_for(&&label, udata);
    if (!udata)
        __terminate();

    udata->cfa = __builtin_dwarf_cfa();

    memcpy(my_udata, udata, sizeof(*udata));

    __builtin_unwind_init();

    pc = (char *)__builtin_extract_return_addr(__builtin_return_address(0)) - 1;

    handler = throw_helper(eh, pc, my_udata, &udata);

    EH_RETURN((void *)eh, (char *)udata->cfa - (char *)my_udata->cfa, handler);
}
