/* libc_strlen.S - the game-provided MMI strlen of the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102), 0x0100F538..0x0100F5CC.
 *
 * Hand-written assembly (it uses the trapping sub/addi forms and branch-likely
 * delay slots no C compiler of the era emits), so it is kept as assembly: this
 * file assembles to the retail bytes with both the matching toolchain (SCE
 * ee-gcc 2.9) and the GCC 14 behaviour toolchain.  Part of the libc unit
 * (see runtime/libc.c); it sits between Polyphony's I/O functions in the
 * retail link and satisfies every strlen reference in the binary (game code,
 * libmc, libgcc frame.c) - no libc-archive strlen was linked.
 *
 * Algorithm: read whole aligned quadwords.  The bytes in front of s in the
 * first quadword are forced to 0xFF (dsllv -1 by off*8, not; the low
 * doubleword is filled completely when off >= 8), then pceqb finds NUL bytes
 * 16 at a time.  The NUL's position inside its doubleword is found with
 * ppac5 / ppach / (x ^ -x) / dsll 31 / plzcw / dsrl32 2, which yields
 * 7 - byte index.  May read up to 15 bytes past the terminator (same
 * quadword, never another page).  Registers: $8-$12 are caller-saved
 * temporaries in both the EABI of the retail build and the n32 ABI of the
 * GCC 14 build; only $a0 is an argument, the result is in $v0.
 */
	.set	noreorder
	.text
	.p2align 3

/* FUNC 0x0100F538 strlen */
/*   size_t strlen(const char *s).  Callers include load_irx_module
 *   (0x01000790), strdup_alloc and the other FileName helpers
 *   (0x010035E0..0x01003BF0), libmc's mc_chdir_end_callback (0x010165D0) and
 *   frame.c's extract_cie_info. */
	.globl	strlen
	.ent	strlen
	.type	strlen, @function
strlen:
	andi	$8, $4, 0xF		/* off = s & 15                         */
	addiu	$9, $0, -1
	sll	$10, $8, 3
	sub	$5, $4, $8		/* q = s - off (16-aligned)             */
	dsllv	$11, $9, $10
	lq	$3, 0($5)
	andi	$12, $8, 8
	nor	$11, $11, $0		/* head mask: bytes 0..(off&7)-1 = 0xFF */
	beql	$12, $0, 1f		/* off < 8: mask the low doubleword     */
	or	$3, $3, $11
	pcpyld	$12, $11, $9		/* off >= 8: low dword all 0xFF, high   */
	por	$3, $3, $12		/*   dword masked                       */
1:	pceqb	$10, $3, $0		/* 0xFF in every NUL byte               */
	pcpyud	$9, $10, $0
	or	$8, $10, $9
	bne	$8, $0, 3f
	sub	$2, $5, $4		/* len = q - s                          */
2:	lq	$3, 16($5)
	addi	$5, $5, 16
	pceqb	$10, $3, $0
	pcpyud	$9, $10, $0
	or	$8, $10, $9
	beq	$8, $0, 2b
	sub	$2, $5, $4
3:	beql	$10, $0, 4f		/* no NUL in the low dword: use the high */
	addi	$2, $2, 8		/*   one, 8 bytes further               */
	daddu	$9, $10, $0
4:	ppac5	$8, $9
	ppach	$8, $0, $8
	dsub	$9, $0, $8
	xor	$8, $8, $9		/* bits above the lowest set bit        */
	dsll	$8, $8, 31
	plzcw	$8, $8
	addi	$2, $2, 7
	dsrl32	$8, $8, 2		/* 7 - byte index                       */
	jr	$31
	sub	$2, $2, $8
	.end	strlen
	.size	strlen, .-strlen

	.set	reorder
