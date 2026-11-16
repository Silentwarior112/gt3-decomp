/* crt0.S - EE start-up code of the GT3 A-Spec US main executable
 * (SCUS-97102), .text 0x00100000..0x001000C7 (unit u000, G000).
 *
 * This is the standard Sony SDK 2.x crt0.s (sce/ee/lib/crt0.o), unchanged,
 * and the same instruction stream as the bootstrap crt0
 * (decomp/bootstrap/src/crt0.S, 0x01000000..0x010000C7), word for word apart
 * from relocations: the nop padding of the .bss clear loop (R5900 short-loop
 * erratum) and the 8-byte alignment of _exit / _root included.  It is the
 * first object on the link line; collect2's generated constructor-table
 * object (collect2_ctors.c, 0x001000C8) follows it directly.
 *
 * Linker-script symbols it uses, with their values in this binary:
 *   _fbss        0x00353200  start of .sbss (the .bss clear starts here)
 *   _end         0x004B2C0C  end of .bss (and __bss_end, the clear loop's end)
 *   _gp          0x003579F0
 *   _stack       0x01F00000  main-thread stack base (the top 1 MB of RAM)
 *   _stack_size  0x00100000
 *   _heap_size   -1          (InitHeap(_end, -1): heap up to the stack)
 *
 * .bss of this object (at the end, as the original object ends):
 *   _args        0x00353580  64-byte aligned, 256 + 16*4 + 4 = 0x144 bytes:
 *                            {int argc; char *argv[16]; char strings[256]},
 *                            filled in by the kernel (InitMainThread).
 *
 * ref/funcs.json functions here are the real symbols: _start, _exit, _root.
 *
 * All code here is hand-written assembly: nothing else can clear .bss and
 * set up $gp / $sp before any C runs.  Register moves are written as the
 * `daddu rd, rs, $0` the original encodes.
 */
/* startup_internal.h: the syscall numbers.  Its C part, with the canonical header of
 * this translation unit (runtime/startup/G000_collect2_ctors.h, a C header: _gp,
 * _exit), is behind __ASSEMBLER__. */
#include "runtime/startup/startup_internal.h"

	.set	noat
	.set	noreorder

	.text

/* The two nops in front of the ELF entry point (e_entry = 0x00100008).
 * Never executed; part of the crt0.s layout, no function (ref/not_starts.txt:
 * the .data word at 0x00339688 that holds 0x00100000 is a constant, not a
 * code pointer).  The local symbol _crt0_nop_padding (as in the bootstrap
 * tree) changes no bytes. */
	.type	_crt0_nop_padding, @function
_crt0_nop_padding:
	nop
	nop
	.size	_crt0_nop_padding, . - _crt0_nop_padding

/* _start, the ELF entry point.  In order:
 *   1. Clear .sbss + .bss with `sq $0` from _fbss upwards.  The bound test
 *      uses the address BEFORE the increment (sltu, then addiu in the delay
 *      slot), so the loop also stores the quadword that holds _end and the
 *      one after it: it clears up to 0x004B2C20, 0x14 bytes past _end
 *      (harmless: that is the start of the heap, set up next).  The three
 *      nops pad the loop against the R5900 short-loop erratum.
 *   2. InitMainThread (syscall 0x3C) with ($gp = _gp, _stack, _stack_size,
 *      _args, _root); its return value becomes $sp.
 *   3. InitHeap (syscall 0x3D) with (_end, _heap_size = -1).
 *   4. _InitSys() (0x0028D750), then FlushCache(0) (0x00285EE0, write back
 *      the D-cache).
 *   5. ei, then main(_args.argc, _args.argv) (0x00100140), then tail-jump to
 *      Exit(main's return value) (0x00285880).  main never returns (it loops
 *      forever).
 * _InitSys takes no arguments: what $a0..$a4 hold at that call (_end, -1,
 * 0x00100000, _args, _root) is left over from the two syscalls, and the
 * 0x00100000 in $a2 is _stack_size, not the address 0x00100000. */
// FUNC 0x00100008 _start
	.globl	_start
	.type	_start, @function
	.ent	_start
_start:
	/* The loop end is __bss_end: the original's linker gave it _end's address
	 * (the same code), but the hybrid link (tools/hybrid.py) puts the compiled
	 * C between the .bss and _end, the heap start, and must not clear it. */
	lui	$2, %hi(_fbss)
	lui	$3, %hi(__bss_end)
	addiu	$2, $2, %lo(_fbss)
	addiu	$3, $3, %lo(__bss_end)
1:
	nop
	nop
	nop
	sq	$0, 0($2)
	sltu	$1, $2, $3
	bne	$1, $0, 1b
	addiu	$2, $2, 16

	/* $sp = InitMainThread(_gp, _stack, _stack_size, _args, _root) */
	lui	$4, %hi(_gp)
	lui	$5, %hi(_stack)
	lui	$6, %hi(_stack_size)
	lui	$7, %hi(_args)
	lui	$8, %hi(_root)
	addiu	$4, $4, %lo(_gp)
	addiu	$5, $5, %lo(_stack)
	addiu	$6, $6, %lo(_stack_size)
	addiu	$7, $7, %lo(_args)
	addiu	$8, $8, %lo(_root)
	daddu	$28, $4, $0		/* move $gp, $a0 */
	addiu	$3, $0, U000_SYSCALL_INIT_MAIN_THREAD	/* 0x3C */
	syscall
	daddu	$29, $2, $0		/* move $sp, $v0 */

	/* InitHeap(_end, _heap_size) */
	lui	$4, %hi(_end)
	lui	$5, %hi(_heap_size)
	addiu	$4, $4, %lo(_end)
	addiu	$5, $5, %lo(_heap_size)
	addiu	$3, $0, U000_SYSCALL_INIT_HEAP		/* 0x3D */
	syscall

	jal	_InitSys
	nop

	jal	FlushCache		/* FlushCache(WRITEBACK_DCACHE) */
	daddu	$4, $0, $0

	ei

	/* Exit(main(_args.argc, _args.argv)) */
	lui	$2, %hi(_args)
	addiu	$2, $2, %lo(_args)
	lw	$4, 0($2)
	jal	main
	addiu	$5, $2, 4

	j	Exit
	daddu	$4, $2, $0
	.end	_start
	.size	_start, . - _start

/* _exit: j Exit with $a0 = 0.  The status argument is IGNORED (always exits
 * with 0).  Only caller: abort() (0x0027D7A0), which calls _exit(1), so
 * abort ends in Exit(0).  The nop at 0x001000B4 in front of it is the
 * .align 3 padding. */
// FUNC 0x001000B8 _exit
	.align	3
	.globl	_exit
	.type	_exit, @function
	.ent	_exit
_exit:
	j	Exit
	daddu	$4, $0, $0
	.end	_exit
	.size	_exit, . - _exit

/* _root (0x001000C0): root function of the main thread, passed to
 * InitMainThread: the kernel "returns" into it if the thread entry function
 * returns.  A private ExitThread stub (syscall 0x23) with no jr $ra, since
 * ExitThread does not return.  (The libkernl ExitThread stub is
 * 0x00285A90.) */
// FUNC 0x001000C0 _root
	.align	3
	.type	_root, @function
	.ent	_root
_root:
	addiu	$3, $0, U000_SYSCALL_EXIT_THREAD	/* 0x23 */
	syscall
	.end	_root
	.size	_root, . - _root

	.bss
	.align	6
_args:	.space	256 + 16 * 4 + 4	/* 0x00353580 */
