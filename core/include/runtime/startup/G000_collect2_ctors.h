/* G000_collect2_ctors.h - the start-up code linked in front of main (the
 * no-FDE stretch G000, unit u000, C / assembly), 0x00100000..0x00100140:
 *
 *   crt0.S            0x00100000..0x001000C7  Sony SDK crt0.s: _start, _exit, _root
 *   collect2_ctors.c  0x001000C8..0x0010013F  the C file collect2 generated at
 *                                                  link time: reg_frame, dereg_frame
 *
 * Both are the bootstrap's code (decomp/bootstrap src/crt0.S, src/crtbegin.c).
 *
 * crt0: _start (the ELF entry point 0x00100008) clears .sbss / .bss from
 * _fbss (0x00353200) to past _end (0x004B2C0C), sets $gp = _gp, gets $sp
 * from InitMainThread (_stack 0x01F00000, _stack_size 0x00100000, _args,
 * _root), InitHeap(_end, -1), then _InitSys, FlushCache(0), ei and
 * main(_args.argc, _args.argv) (T000), and tail-jumps to Exit(main's
 * result) (0x00285880; main never returns).  _root (0x001000C0) is the
 * main thread's root function, a bare ExitThread syscall.  The two nops at
 * 0x00100000 are padding, 0x00100014 is the rest of _start: neither is a
 * function (two .data words happen to hold those values; ref/not_starts.txt).
 *
 * collect2's file: reg_frame (0x001000C8, the last .ctors entry, so the
 * first static constructor to run) registers the program's unwind info,
 * __register_frame_info_table(frame_table, &ob); dereg_frame (0x00100108,
 * the last .dtors entry) deregisters it, and never runs (nothing walks
 * .dtors).  Both are `static' in collect2's file and called only through
 * the lists, so they are not declared here.  The frame.c functions they
 * call are declared in G006_fp_bit.h.  The lists: __CTOR_LIST__ 0x0028F020
 * (count 110), __DTOR_LIST__ 0x0028F1E0 (count 49), __main_reference
 * 0x00350B00.
 */
#ifndef GAME_G000_COLLECT2_CTORS_H
#define GAME_G000_COLLECT2_CTORS_H

#include "game/game.h"
#include <stddef.h>                             /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

struct dwarf_fde;                               /* frame.c's FDE header (G006, u135: fde) */

/* frame.c's registration record (frame.h; collect2's file has a copy with
 * void pointers, "This must match what's in frame.h").  __register_frame
 * mallocs one, reg_frame uses the static `ob'. */
struct object {
    void              *pc_begin;                /* 0x00 lowest pc covered; 0: frame_init not run */
    void              *pc_end;                  /* 0x04 one past the highest pc covered */
    struct dwarf_fde  *fde_begin;               /* 0x08 the `begin' passed at registration */
    struct dwarf_fde **fde_array;               /* 0x0C table form: the NULL-terminated list of
                                                   FDE chains; after frame_init the sorted FDEs */
    size_t             count;                   /* 0x10 number of FDEs in fde_array */
    struct object     *next;                    /* 0x14 */
};
EGCS_SIZE(struct object, 0x18);

/* crt0's _exit: tail-jumps to Exit(0); the status is ignored.  Its caller
 * is newlib's abort (0x0027D7A0, u137). */
void _exit(int status);                         /* 0x001000B8 */

/* The linker's $gp value (0x003579F0): crt0 loads it, the render thread
 * (T002) and the other threads get &_gp as their gpReg. */
extern char _gp[];                              /* 0x003579F0 */

/* collect2's lists, walked by libgcc2's __do_global_ctors / __do_global_dtors
 * (G004, u131): word 0 = the count (-1 would mean "count the entries up to
 * the 0 terminator"), then the function pointers, then 0.  __CTOR_LIST__ =
 * {110, the 109 static constructors, reg_frame, 0}, __DTOR_LIST__ = {49, the
 * 48 static destructors, dereg_frame, 0}. */
extern void (*__CTOR_LIST__[])(void);           /* 0x0028F020 (.ctors) */
extern void (*__DTOR_LIST__[])(void);           /* 0x0028F1E0 (.dtors) */
/* collect2's `entry_pt *__main_reference = __main;' (keeps __main linked;
 * nothing reads it), .sdata. */
extern void (*__main_reference)(void);          /* 0x00350B00 */

/* collect2's `static void *frame_table[]': one pointer per C++ translation
 * unit's .eh_frame block (551; first 0x0028F2AC, last 0x00306850), then 0
 * at 0x0028F01C.  Static in the original. */
extern void *frame_table[];                     /* 0x0028E780 */

/* collect2's `static struct object ob;' (local to reg_frame), .bss right
 * after crt0's _args (0x00353580 + 0x144, 8-aligned). */
extern struct object reg_frame_ob;              /* 0x003536C8 */

#ifdef __cplusplus
}
#endif

#endif /* GAME_G000_COLLECT2_CTORS_H */
