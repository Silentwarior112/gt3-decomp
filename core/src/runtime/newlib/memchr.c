/* u138 / G007: memchr, the Sony EE libc (MMI) version: when the length is at
 * least 16 and the source is quadword aligned, it tests 16 bytes at a time
 * with lq and the classic "has a zero byte" trick on (block ^ c):
 * psubb(x, 0x01..01) & ~x & 0x80..80 (pnor / psubb / pand), the upper
 * doubleword folded onto the lower (pcpyud + or); on a hit, or for the tail,
 * it goes on one byte at a time from the start of that block.
 *
 * Matching build: the retail instructions, as for its siblings strchr /
 * strncmp (u137) and the bootstrap's memcmp, memcpy, strcmp, ... (Sony's
 * hand-written MMI family).  Evidence that this one is hand-written too:
 *   - the c mask is `dsll; daddu` ((c << 8) + c); compiled from C, egcs turns
 *     that add into `or` (c is known to be 0..255), as strchr's mask;
 *   - the 0x01..01 constant is built in $v0 and copied to $a0 (`move`) for
 *     the loop, and the result / temporaries sit in $v0, $v1, $t2 ($10)
 *     while $t0 / $t1 hold the 0x80 and c quadwords: no C + MMI asm form
 *     gives that allocation (build/scratch/u138/gen_memchr.py, gen2.py: 66%);
 *   - the object is only 4-aligned (0x00282794 follows mbtowc_r.o at once),
 *     as an assembled .S with `.align 2`, not cc1's `.p2align 3`.
 * The C below (GCC 14 / NON_MATCHING) computes the same: the doubleword
 * "has a zero byte" test is exact for "some byte is zero" just as psubb's. */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part2_internal.h"

// FUNC 0x00282794 memchr
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)

/* Registers are numeric; the backward branch closing the 4-instruction byte
 * loop is a raw .word (the assembler's R5900 short-loop workaround would pad
 * it with nops). */
__asm__(
"\t.text\n\t.p2align\t2\n\t.set\tnoreorder\n\t.globl\tmemchr\n"
"\t.ent\tmemchr\n\t.type\tmemchr, @function\nmemchr:\n"
"	sltiu	$2, $6, 0x10\n"		/* length < 16: bytewise */
"	bnez	$2, $Lmemchr_bytes\n"
"	andi	$5, $5, 0xFF\n"		/* c &= 0xff */
"	andi	$2, $4, 0xF\n"		/* not 16-aligned: bytewise */
"	bnez	$2, $Lmemchr_bytes\n"
"	daddu	$7, $4, $0\n"		/* q = src */
"	dsll	$3, $5, 8\n"
"	lui	$2, 0x101\n"		/* 0x0101010101010101 */
"	ori	$2, $2, 0x101\n"
"	dsll	$2, $2, 16\n"
"	ori	$2, $2, 0x101\n"
"	dsll	$2, $2, 16\n"
"	ori	$2, $2, 0x101\n"
"	daddu	$10, $3, $5\n"		/* (c << 8) + c */
"	lui	$3, 0x8080\n"		/* 0x8080808080808080 */
"	ori	$3, $3, 0x8080\n"
"	dsll	$3, $3, 16\n"
"	ori	$3, $3, 0x8080\n"
"	dsll	$3, $3, 16\n"
"	ori	$3, $3, 0x8080\n"
"	pcpyh	$8, $10\n"
"	pcpyld	$9, $8, $8\n"		/* c in all 16 bytes */
"	daddu	$4, $2, $0\n"
"	pcpyld	$8, $3, $3\n"		/* 0x80 in all 16 bytes */
"$Lmemchr_qwloop:\n"
"	lq	$2, 0($7)\n"
"	pxor	$2, $2, $9\n"		/* bytes equal to c become 0 */
"	pcpyld	$10, $4, $4\n"		/* 0x01 in all 16 bytes */
"	pnor	$3, $0, $2\n"
"	psubb	$2, $2, $10\n"
"	pand	$2, $2, $3\n"
"	pand	$2, $2, $8\n"
"	pcpyud	$3, $2, $9\n"
"	or	$2, $2, $3\n"
"	bnel	$2, $0, $Lmemchr_bytes\n"	/* a hit: bytewise from this block */
"	daddu	$4, $7, $0\n"
"	addiu	$6, $6, -0x10\n"
"	sltiu	$2, $6, 0x10\n"
"	beqz	$2, $Lmemchr_qwloop\n"
"	addiu	$7, $7, 0x10\n"
"	daddu	$4, $7, $0\n"
"$Lmemchr_bytes:\n"			/* while (length--) */
"	lui	$2, 0xFFFF\n"
"	addiu	$6, $6, -0x1\n"
"	ori	$2, $2, 0xFFFF\n"
"	beq	$6, $2, $Lmemchr_null\n"
"	nop\n"
"	lui	$3, 0xFFFF\n"
"	ori	$3, $3, 0xFFFF\n"
"$Lmemchr_next:\n"
"	lbu	$2, 0($4)\n"
"	beq	$2, $5, $Lmemchr_found\n"
"	addiu	$6, $6, -0x1\n"
"	.word	0x14C3FFFC\n"		/* bne $6, $3, $Lmemchr_next */
"	addiu	$4, $4, 0x1\n"
"$Lmemchr_null:\n"
"	jr	$31\n"
"	daddu	$2, $0, $0\n"
"$Lmemchr_found:\n"
"	jr	$31\n"
"	daddu	$2, $4, $0\n"
"\t.set\treorder\n\t.end\tmemchr\n\t.size\tmemchr, .-memchr\n\t.previous\n");

#else

typedef u64 u64_a BOOT_MAY_ALIAS;

#define LSB8  0x0101010101010101ULL
#define MSB8  0x8080808080808080ULL

/* Non-zero iff the doubleword x has a 0x00 byte. */
#define DETECTNULL(x)  (((x) - LSB8) & ~(x) & MSB8)

void *memchr(const void *src, int c, size_t length)
{
    const unsigned char *p = (const unsigned char *)src;

    c &= 0xFF;
    if (length >= 16 && ((u32)p & 0xF) == 0) {
        const u64_a *q = (const u64_a *)p;
        u64 mask = (u64)c * LSB8;   /* c in every byte */

        do {
            u64 lo = q[0] ^ mask, hi = q[1] ^ mask;

            if (DETECTNULL(lo) | DETECTNULL(hi))
                break;  /* found in this block: scan it bytewise */
            length -= 16;
            q += 2;
        } while (length >= 16);
        p = (const unsigned char *)q;
    }

    while (length-- != 0) {
        if (*p == c)
            return (void *)p;
        p++;
    }
    return NULL;
}

#endif
