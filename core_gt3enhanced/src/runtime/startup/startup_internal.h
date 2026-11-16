/* startup_internal.h - unit u000 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x00100000..0x00100140: G000, the start-up code linked in front of main
 * (0x00100140).
 *
 * Files (link order):
 *   crt0.S            0x00100000..0x001000C7  Sony SDK crt0.s: _start, _exit, _root
 *   collect2_ctors.c  0x001000C8..0x0010013F  the C file collect2 generates at link
 *                                                  time: reg_frame, dereg_frame
 *
 * Both are the same machine code as the bootstrap's 0x01000000..0x0100013F
 * (decomp/bootstrap/src/crt0.S and src/crtbegin.c), word for word apart from
 * relocations.
 *
 * ref/funcs.json functions here (the same as the real symbols):
 *   0x00100008  _start (0xAC bytes, up to the .align 3 nop at 0x001000B4)
 *   0x001000B8  _exit,  0x001000C0  _root (8 bytes each)
 *   0x001000C8  reg_frame,  0x00100108  dereg_frame
 * The 2 padding nops at 0x00100000 belong to no function.  (Before
 * ref/not_starts.txt, .data words equal to 0x00100000 (0x00339688) and
 * 0x00100014 (0x00340CEC) made false starts there.)
 *
 * The canonical header of G000 is runtime/startup/G000_collect2_ctors.h (struct object,
 * frame_table, reg_frame_ob, _gp, _exit).  This header keeps only what is
 * local to the unit: crt0's syscall numbers and the linker-script values.
 *
 * crt0.S includes this header too: everything C is behind
 * __ASSEMBLER__.
 */
#ifndef U000_H
#define U000_H

/* ======================================================================== */
/* crt0 (crt0.S)                                                        */
/* ======================================================================== */
/* Linker-script symbols crt0 uses, with their values in this binary:
 *   _fbss        0x00353200  start of .sbss: the .bss clear starts here
 *   _end         0x004B2C0C  end of .bss
 *   _gp          0x003579F0  (declared in runtime/startup/G000_collect2_ctors.h)
 *   _stack       0x01F00000  main-thread stack: the top 1 MB of RAM
 *   _stack_size  0x00100000
 *   _heap_size   -1          InitHeap(_end, -1): the heap runs up to the stack
 * crt0.o's own .bss (defined at the end of crt0.S, declared below):
 *   _args        0x00353580  64-byte aligned, 256 + 16*4 + 4 = 0x144 bytes,
 *                            {int argc; char *argv[16]; char strings[256]},
 *                            filled in by the kernel's InitMainThread.
 *
 * Functions crt0 calls, and where they are declared:
 *   _InitSys     0x0028D750  runtime/newlib/G007_stdio.h (libkernl initsys.c)
 *   FlushCache   0x00285EE0  runtime/newlib/G007_stdio.h (syscall 0x64; crt0: FlushCache(0),
 *                            WRITEBACK_DCACHE in eekernel.h)
 *   main         0x00100140  game/app/T000_main.h (u001, T000; a C++ header)
 *   Exit         0x00285880  eekernel.h only, with the bootstrap's address: the
 *                            canonical G007_stdio.h lacks it (see notes.md)
 * Nothing in C calls them here, so this header declares none of them.     */

/* Kernel calls crt0 makes inline (addiu $v1, $0, N; syscall) instead of
 * through the libkernl stubs (InitMainThread 0x00285C20, InitHeap 0x00285C30,
 * ExitThread 0x00285A90; u139).                                              */
#define U000_SYSCALL_EXIT_THREAD        0x23    /* ExitThread()                  */
#define U000_SYSCALL_INIT_MAIN_THREAD   0x3C    /* InitMainThread(gp, stack, stack_size, args, root) -> sp */
#define U000_SYSCALL_INIT_HEAP          0x3D    /* InitHeap(heap, heap_size)     */

#ifndef __ASSEMBLER__

#include "types.h"                      /* BOOT_USED */
#include "runtime/startup/G000_collect2_ctors.h"   /* own TU: struct object, frame_table, reg_frame_ob */
#include "runtime/libgcc/G006_fp_bit.h"           /* frame.c: __register_frame_info_table,
                                           __deregister_frame_info */

/* crt0's {int argc; char *argv[16]; char strings[256]} (crt0.S's .bss) */
extern char _args[];                    /* 0x00353580 */

#endif /* !__ASSEMBLER__ */

#endif /* U000_H */
