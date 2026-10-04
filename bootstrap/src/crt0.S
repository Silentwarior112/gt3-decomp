/* crt0.S - EE start-up code of the GT3 US bootstrap (BOOTSTRAP_SCUS-97102),
 * .text 0x01000000..0x010000C7 (unit "main").
 *
 * This is Sony's standard SDK 2.x crt0.s (sce/ee/lib/crt0.o), unchanged:
 * the instruction stream matches it one for one, including the nop
 * padding of the .bss clear loop (R5900 short-loop erratum) and the
 * 8-byte alignment of _exit / _root.  It is the first object on the link
 * line; collect2's generated constructor-table object (src/crtbegin.c,
 * 0x010000C8) follows it directly.
 *
 * Linker-script symbols it uses, with their values in this binary:
 *   _fbss        0x01041E80  start of .sbss (the .bss clear starts here)
 *   _end         0x0104DE94  end of .bss
 *   _gp          0x01049CF0
 *   _stack       0x01F00000  main-thread stack base (explicit, not -1:
 *                            the stack is the top 1 MB of RAM)
 *   _stack_size  0x00100000
 *   _heap_size   -1          (InitHeap(_end, -1): heap up to the stack)
 *
 * .bss of this object:
 *   _args        0x01041F00  64-byte aligned, 256+16*4+4 = 0x144 bytes:
 *                            {int argc; char *argv[16]; char strings[256]}
 *                            filled in by the kernel (InitMainThread).
 *
 * All code here is hand-written assembly: nothing else can clear .bss and
 * set up $gp / $sp before any C runs.  Register moves are written as the
 * `daddu rd, rs, $0` the original encodes (the n32 toolchain would assemble
 * `move` as `or`), so the object assembles to the same words as the binary
 * apart from relocations.
 */

	.set	noat
	.set	noreorder

	.text

// FUNC 0x01000000 nop_padding (not a function)
//   The two nops in front of the ELF entry point (e_entry = 0x01000008).
//   Never executed; part of Sony's crt0.s layout.  The local symbol
//   _crt0_nop_padding exists only so that tools/score.py, which pairs the
//   function symbols of an assembly file with its FUNC markers in order,
//   lines the markers up; it changes no bytes.
	.type	_crt0_nop_padding, @function
_crt0_nop_padding:
	nop
	nop
	.size	_crt0_nop_padding, . - _crt0_nop_padding

// FUNC 0x01000008 _start
//   ELF entry point.  In order:
//   1. Clear .sbss + .bss with `sq $0` from _fbss upwards.  The bound test
//      uses the address BEFORE the increment (sltu, then addiu in the delay
//      slot), so the loop also stores the quadword that contains _end and
//      the one after it: it clears up to 0x0104DEB0, 0x1C bytes past _end
//      (harmless, that is the start of the heap).  The three nops pad
//      the loop against the R5900 short-loop erratum.
//   2. InitMainThread (syscall 0x3C) with ($gp = _gp, _stack, _stack_size,
//      _args, _root); its return value becomes $sp.
//   3. InitHeap (syscall 0x3D) with (_end, _heap_size = -1).
//   4. _InitSys() (0x01022C18: clear user syscalls, InitAlarm, InitThread),
//      then FlushCache(0) (0x0101BE60, write back the D-cache).
//   5. ei, then main(_args.argc, _args.argv) (0x01000538), then tail-jump
//      to Exit(main's return value) (0x0101B800).  In practice main never
//      returns: it ends in ExecPS2 or in an endless loop.
	.globl	_start
	.type	_start, @function
	.ent	_start
_start:
	lui	$2, %hi(_fbss)
	lui	$3, %hi(_end)
	addiu	$2, $2, %lo(_fbss)
	addiu	$3, $3, %lo(_end)
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
	addiu	$3, $0, 0x3C		/* InitMainThread */
	syscall
	daddu	$29, $2, $0		/* move $sp, $v0 */

	/* InitHeap(_end, _heap_size) */
	lui	$4, %hi(_end)
	lui	$5, %hi(_heap_size)
	addiu	$4, $4, %lo(_end)
	addiu	$5, $5, %lo(_heap_size)
	addiu	$3, $0, 0x3D		/* InitHeap */
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

// FUNC 0x010000B8 _exit
//   j Exit with $a0 = 0.  The status argument is IGNORED (always exits
//   with 0).  Only caller: abort() (0x0101AFF0).  The nop at 0x010000B4 in
//   front of it is the .align 3 padding.
	.align	3
	.globl	_exit
	.type	_exit, @function
	.ent	_exit
_exit:
	j	Exit
	daddu	$4, $0, $0
	.end	_exit
	.size	_exit, . - _exit

// FUNC 0x010000C0 _root
//   Root function of the main thread, passed to InitMainThread: the kernel
//   "returns" into it if the thread's entry function returns.  It is a
//   private ExitThread stub (syscall 0x23) with no `jr $ra` - ExitThread does
//   not return.  (libkernl's public ExitThread stub is 0x0101BA10.)
	.align	3
	.type	_root, @function
	.ent	_root
_root:
	addiu	$3, $0, 0x23		/* ExitThread */
	syscall
	.end	_root
	.size	_root, . - _root

	.bss
	.align	6
_args:	.space	256 + 16 * 4 + 4	/* @0x01041F00 */
