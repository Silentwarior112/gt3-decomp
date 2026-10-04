/* libc_syscall_stubs.S - EE kernel syscall stubs #0..#3 (Sony libkernl).
 *
 * These four 16-byte stubs are the head of libkernl's syscall stub table
 * (0x0101B7C0..; the table continues at 0x0101B800 with Exit = #4 in the kernl_sys
 * unit's sdk/libkernl/syscalls.S).  They are Sony libkernl code, not libc; they
 * are written here only because 0x0101B7C0..0x0101B7FF falls inside the libc
 * unit's address range.  An integrator may move them verbatim to the top of
 * sdk/libkernl/syscalls.S (marker lines included).
 *
 * The table starts on a 64-byte boundary: 0x30 bytes of nop padding precede it,
 * after the end of Sony libc's strncpy (0x0101B78C).
 *
 * Stub shape (as in the binary):  addiu $v1, $zero, N ; syscall ; jr $ra ; nop
 * Names follow the EE kernel syscall numbering used in include/eekernel.h.
 */
	.set	noreorder
	.set	noat
	.text
	.p2align 6		/* ? object alignment inferred from the padding before 0x0101B7C0 */

/* FUNC 0x0101B7C0 RFU000_FullReset */
/*   syscall #0: reserved kernel entry; nothing in the binary calls it (no prototype). */
	.globl	RFU000_FullReset
	.ent	RFU000_FullReset
RFU000_FullReset:
	addiu	$3, $0, 0
	syscall
	jr	$31
	nop
	.end	RFU000_FullReset

/* FUNC 0x0101B7D0 ResetEE */
/*   syscall #1: void ResetEE(int init_bitfield).  No callers in the binary. */
	.globl	ResetEE
	.ent	ResetEE
ResetEE:
	addiu	$3, $0, 1
	syscall
	jr	$31
	nop
	.end	ResetEE

/* FUNC 0x0101B7E0 SetGsCrt */
/*   syscall #2: void SetGsCrt(short interlace, short omode, short ffmode).
 *   Tail-called (j) by sceGsResetGraph modes 0 and 5 with
 *   (inter & 1, omode & 0xFF, ffmode & 1). */
	.globl	SetGsCrt
	.ent	SetGsCrt
SetGsCrt:
	addiu	$3, $0, 2
	syscall
	jr	$31
	nop
	.end	SetGsCrt

/* FUNC 0x0101B7F0 RFU003 */
/*   syscall #3: reserved kernel entry; nothing in the binary calls it (no prototype). */
	.globl	RFU003
	.ent	RFU003
RFU003:
	addiu	$3, $0, 3
	syscall
	jr	$31
	nop
	.end	RFU003
