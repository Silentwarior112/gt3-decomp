/* string.c - Sony EE libc / newlib string members of the no-FDE
 * stretch G007 in u137, in address order:
 *
 *   memcmp.o     0x0027D8FC  memcmp      } the same machine code as the
 *   memcpy.o     0x0027D990  memcpy      } bootstrap's runtime/libc.c
 *   memmove.o    0x0027DA3C  memmove     } (Sony's MMI versions: 16-byte
 *   memset.o     0x0027DB38  memset      } lq/sq blocks, 128-bit NUL tests);
 *   strcasecmp.o 0x0027E6E0  strcasecmp    the source below is copied from
 *   strcat.o     0x0027E794  strcat      } it, with the bootstrap's matching
 *   strchr.o     0x0027E8B0  strchr        build (the retail instructions as
 *   strcmp.o     0x0027EA40  strcmp      } asm, LIBC_RETAIL_ASM) and its
 *   strcpy.o     0x0027EB84  strcpy      } C reconstruction (#else).
 *   strncmp.o    0x0027EC98  strncmp
 *   strncpy.o    0x0027EE50  strncpy     }
 *   strstr.o     0x0027F008  strstr
 *   strupr.o     0x0027F2E8  strupr
 *
 * strchr and strncmp are Sony MMI routines of the same family that the
 * bootstrap does not have; strcasecmp, strstr and strupr are plain newlib C.
 * The core's index calls 0x0027D8FC "memcpy_small": it is memcmp (it returns
 * the first byte difference; callers _fio_version_check and
 * _sceSifCheckLoadFileVersion, as in the bootstrap).
 *
 * The C reconstructions (#else) keep the bootstrap's structure; their word
 * temporaries are named for the test they hold (nul, hit, diff, word).
 */
#include "runtime/newlib/G007_stdio.h"
#include "runtime/newlib/newlib_part1_internal.h"

#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
/* Matching build: the bootstrap-shared routines are the retail instructions
 * (see the bootstrap's runtime/libc.c matching note).  Registers are numeric;
 * a backward branch closing a loop of <= 6 instructions is a raw .word (the
 * assembler's R5900 short-loop workaround would pad it). */
#define LIBC_RETAIL_ASM 1
#define LIBC_ASM_BEGIN(name, p2align) \
    "\t.text\n\t.p2align\t" #p2align "\n\t.set\tnoreorder\n\t.globl\t" #name "\n" \
    "\t.ent\t" #name "\n\t.type\t" #name ", @function\n" #name ":\n"
#define LIBC_ASM_END(name) \
    "\t.set\treorder\n\t.end\t" #name "\n\t.size\t" #name ", .-" #name "\n\t.previous\n"
#endif

/* Word views of byte buffers (may_alias for GCC 14). */
typedef u64  u64_a  BOOT_MAY_ALIAS;
typedef u128 u128_a BOOT_MAY_ALIAS;

/* ------------------------------------------------------------------------ */
/* newlib's block-copy macros, EE flavour                                    */
/* ------------------------------------------------------------------------ */

#define LSB8  0x0101010101010101ULL   /* built with lui/ori/dsll/ori/dsll/ori */
#define MSB8  0x8080808080808080ULL

/* Non-zero iff the doubleword X has a 0x00 byte (dsubu / nor / and / and). */
#define DETECTNULL(X, lsb, msb)  (((X) - (lsb)) & ~(X) & (msb))

#define UNALIGNED8(X, Y)   (((u32)(X) | (u32)(Y)) & 7)
#define UNALIGNED16(X, Y)  (((u32)(X) | (u32)(Y)) & 15)
#define BIGBLOCKSIZE       (sizeof (u128) << 1)     /* two lq/sq per iteration */
#define LITTLEBLOCKSIZE    (sizeof (u64))           /* one ld/sd per iteration */
#define TOO_SMALL(LEN)     ((LEN) < BIGBLOCKSIZE)   /* unsigned compare (sltiu) */

/* ------------------------------------------------------------------------ */
/* MMI operations                                                            */
/*                                                                           */
/*   QW_SPLAT(q, x)             q = x in both doublewords        (pcpyld x, x)  */
/*   DW_PCPYH(r, x)             r = halfword 0 of x, 4 times      (pcpyh)       */
/*   QW_NUL(r, x, ones, highs)  r != 0 iff quadword x has a 0x00 byte:           */
/*                              psubb(x, ones) & ~x & highs, the upper           */
/*                              doubleword folded onto the lower (pcpyud + or)   */
/*   QW_NUL_STRNCPY(r, x, ...)  strncpy's broken variant, see strncpy            */
/*   QW_NE_SUBW(r, a, b)        r != 0 iff a != b (psubw, pcpyud + or)           */
/*   QW_NE_XOR(r, a, b)         r != 0 iff a != b (pxor, pcpyud + or)            */
/*   QW_XOR(q, a, b)            q = a ^ b (pxor)                  (strchr)       */
/* r is a u64, q/x/a/b/ones/highs are u128.                                  */
/* ------------------------------------------------------------------------ */

#ifdef BOOT_OLD_GCC

#define LIBC_FN

#define QW_SPLAT(q, x)  __asm__("pcpyld %0, %1, %1" : "=r"(q) : "r"(x))
#define DW_PCPYH(r, x)  __asm__("pcpyh %0, %1" : "=r"(r) : "r"(x))
#define QW_FOLD(r) \
    do { u64 _h; __asm__("pcpyud %0, %1, %1" : "=r"(_h) : "r"(r)); (r) |= _h; } while (0)
#define QW_NUL(r, x, ones, highs) \
    do { u128 _t, _n; \
        __asm__("psubb %0, %1, %2" : "=r"(_t) : "r"(x), "r"(ones)); \
        __asm__("pnor %0, $0, %1" : "=r"(_n) : "r"(x)); \
        __asm__("pand %0, %1, %2" : "=r"(_t) : "r"(_t), "r"(_n)); \
        __asm__("pand %0, %1, %2" : "=r"(r) : "r"(_t), "r"(highs)); \
        QW_FOLD(r); } while (0)
#define QW_NUL_STRNCPY(r, x, ones, highs) \
    do { u128 _t, _n; \
        __asm__("pnor %0, $0, %1" : "=r"(_n) : "r"(x)); \
        __asm__("psubb %0, %1, %2" : "=r"(_t) : "r"(_n), "r"(ones)); \
        __asm__("pand %0, %1, %2" : "=r"(_t) : "r"(_t), "r"(_n)); \
        __asm__("pand %0, %1, %2" : "=r"(r) : "r"(_t), "r"(highs)); \
        QW_FOLD(r); } while (0)
#define QW_NE_SUBW(r, a, b) \
    do { __asm__("psubw %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); QW_FOLD(r); } while (0)
#define QW_NE_XOR(r, a, b) \
    do { __asm__("pxor %0, %1, %2" : "=r"(r) : "r"(a), "r"(b)); QW_FOLD(r); } while (0)
#define QW_XOR(q, a, b)  __asm__("pxor %0, %1, %2" : "=r"(q) : "r"(a), "r"(b))

#else /* GCC 14 */

/* Keep modern GCC from turning these loops back into calls to themselves
 * (memcpy/memset idiom recognition).  Not part of the original. */
#define LIBC_FN __attribute__((optimize("no-tree-loop-distribute-patterns")))

/* MMI psubb restricted to one doubleword: per-byte a - b, no borrow between bytes. */
static inline u64 psubb_dw(u64 a, u64 b)
{
    return ((a | MSB8) - (b & ~MSB8)) ^ ((a ^ ~b) & MSB8);
}

/* psubb(x, 0x01..) & ~x & 0x80.. on both doublewords, or'ed. */
static inline u64 qw_nul_bits(u128 q)
{
    u64 lo = (u64)q, hi = (u64)(q >> 64);
    return (psubb_dw(lo, LSB8) & ~lo & MSB8) | (psubb_dw(hi, LSB8) & ~hi & MSB8);
}

/* psubb(~x, 0x01..) & ~x & 0x80..: non-zero iff some byte is in 0x00..0x7E. */
static inline u64 qw_strncpy_first_test(u128 q)
{
    u64 lo = ~(u64)q, hi = ~(u64)(q >> 64);
    return (psubb_dw(lo, LSB8) & lo & MSB8) | (psubb_dw(hi, LSB8) & hi & MSB8);
}

/* Non-zero iff a != b (the psubw and pxor forms are both zero exactly then). */
static inline u64 qw_diff(u128 a, u128 b)
{
    u128 d = a ^ b;
    return (u64)d | (u64)(d >> 64);
}

/* pcpyh: halfword 0 of x copied into all four halfwords of the doubleword. */
static inline u64 pcpyh_dw(u64 x)
{
    x &= 0xFFFF;
    x |= x << 16;
    return x | (x << 32);
}

/* pcpyld x, x.  Built through memory: GCC 14 turns (x << 64) | x into a call of
 * the 128-bit multiply __multi3. */
static inline u128 qw_splat(u64 x)
{
    union { u64 dw[2]; u128 qw; } u;

    u.dw[0] = x;
    u.dw[1] = x;
    return u.qw;
}

#define QW_SPLAT(q, x)                     ((q) = qw_splat(x))
#define DW_PCPYH(r, x)                     ((r) = pcpyh_dw(x))
#define QW_NUL(r, x, ones, highs)          ((r) = qw_nul_bits(x))
#define QW_NUL_STRNCPY(r, x, ones, highs)  ((r) = qw_strncpy_first_test(x))
#define QW_NE_SUBW(r, a, b)                ((r) = qw_diff(a, b))
#define QW_NE_XOR(r, a, b)                 ((r) = qw_diff(a, b))
#define QW_XOR(q, a, b)                    ((q) = (a) ^ (b))

#endif /* BOOT_OLD_GCC */

// FUNC 0x0027D8FC memcmp
/* Sony EE libc memcmp.  If n >= 16 and both pointers are 16-aligned, equal
 * quadwords are skipped (lq / pxor / pcpyud / or) while at least 16 bytes remain;
 * at the first differing quadword (pointers not advanced) or for the tail, a byte
 * loop returns the first difference as unsigned-byte subtraction, else 0.
 * Callers: libkernl _fio_version_check and _sceSifCheckLoadFileVersion,
 * bytes_equal (0x0021C1E8), padThreadMain (0x0022F328). */
#ifdef LIBC_RETAIL_ASM
__asm__(LIBC_ASM_BEGIN(memcmp, 2)
"	sltiu	$2, $6, 0x10\n"		/* n < 16: bytewise */
"	bnez	$2, $Lmemcmp_tail\n"
"	or	$2, $4, $5\n"
"	andi	$2, $2, 0xF\n"		/* (s1 | s2) & 15: bytewise */
"	bnez	$2, $Lmemcmp_tail\n"
"	nop\n"
"$Lmemcmp_qw:\n"			/* while (n >= 16) */
"	lq	$3, 0($4)\n"
"	sltiu	$7, $6, 0x20\n"
"	lq	$2, 0($5)\n"
"	addiu	$4, $4, 0x10\n"
"	pxor	$8, $2, $3\n"
"	addiu	$2, $5, 0x10\n"
"	pcpyud	$10, $8, $7\n"
"	or	$9, $10, $8\n"
"	movz	$5, $2, $9\n"		/* equal: s2 += 16 */
"	bnel	$9, $0, $Lmemcmp_tail\n"	/* differ: undo s1 += 16, bytewise */
"	addiu	$4, $4, -0x10\n"
"	beqz	$7, $Lmemcmp_qw\n"
"	addiu	$6, $6, -0x10\n"
"$Lmemcmp_tail:\n"			/* while (n--) */
"	lui	$2, 0xFFFF\n"
"	addiu	$6, $6, -0x1\n"
"	ori	$2, $2, 0xFFFF\n"
"	beq	$6, $2, $Lmemcmp_ret0\n"
"	nop\n"
"	lui	$7, 0xFFFF\n"
"	ori	$7, $7, 0xFFFF\n"
"$Lmemcmp_bytes:\n"
"	lbu	$3, 0($4)\n"
"	lbu	$2, 0($5)\n"
"	beq	$3, $2, $Lmemcmp_next\n"
"	addiu	$4, $4, 0x1\n"
"	jr	$31\n"			/* return *s1 - *s2 */
"	subu	$2, $3, $2\n"
"$Lmemcmp_next:\n"
"	addiu	$6, $6, -0x1\n"
"	bne	$6, $7, $Lmemcmp_bytes\n"
"	addiu	$5, $5, 0x1\n"
"$Lmemcmp_ret0:\n"
"	jr	$31\n"
"	daddu	$2, $0, $0\n"
LIBC_ASM_END(memcmp));
#else
LIBC_FN int memcmp(const void *m1, const void *m2, size_t n)
{
    const unsigned char *s1 = (const unsigned char *)m1;
    const unsigned char *s2 = (const unsigned char *)m2;
    const u128_a          *a1;
    const u128_a          *a2;
    u64                  diff;

    if (!(n < sizeof (u128)) && !UNALIGNED16(s1, s2)) {
        a1 = (const u128_a *)s1;
        a2 = (const u128_a *)s2;
        while (n >= sizeof (u128)) {
            QW_NE_XOR(diff, *a2, *a1);
            if (diff)
                break;
            a1++;
            a2++;
            n -= sizeof (u128);
        }
        s1 = (const unsigned char *)a1;
        s2 = (const unsigned char *)a2;
    }
    while (n--) {
        if (*s1 != *s2)
            return *s1 - *s2;
        s1++;
        s2++;
    }
    return 0;
}
#endif

// FUNC 0x0027D990 memcpy
/* Sony EE libc memcpy.  If len >= 32 and both pointers are 16-aligned: 32 bytes per
 * iteration with two lq/sq pairs, then 8 bytes per iteration with ld/sd; the rest
 * (or everything, when unaligned or len < 32) is copied bytewise.  Returns dst0.
 * len is an int, as in newlib: the size tests compare unsigned (sltiu against a
 * sizeof), the byte loop counts down to -1.  66 callers. */
#ifdef LIBC_RETAIL_ASM
__asm__(LIBC_ASM_BEGIN(memcpy, 2)
"	daddu	$8, $4, $0\n"		/* dst0 */
"	sltiu	$2, $6, 0x20\n"		/* len < 32: bytewise */
"	bnez	$2, $Lmemcpy_tail\n"
"	daddu	$3, $8, $0\n"		/* dst */
"	or	$2, $5, $8\n"
"	andi	$2, $2, 0xF\n"		/* (src | dst) & 15: bytewise */
"	bnel	$2, $0, $Lmemcpy_tail1\n"
"	addiu	$6, $6, -0x1\n"
"	daddu	$7, $8, $0\n"		/* aligned_dst */
"$Lmemcpy_qw:\n"			/* 32 bytes per iteration */
"	lq	$3, 0($5)\n"
"	addiu	$6, $6, -0x20\n"
"	addiu	$5, $5, 0x10\n"
"	sltiu	$4, $6, 0x20\n"
"	sq	$3, 0($7)\n"
"	addiu	$7, $7, 0x10\n"
"	lq	$2, 0($5)\n"
"	addiu	$5, $5, 0x10\n"
"	sq	$2, 0($7)\n"
"	beqz	$4, $Lmemcpy_qw\n"
"	addiu	$7, $7, 0x10\n"
"	sltiu	$2, $6, 0x8\n"
"	bnez	$2, $Lmemcpy_tail\n"
"	daddu	$3, $7, $0\n"
"$Lmemcpy_dw:\n"			/* 8 bytes per iteration */
"	ld	$3, 0($5)\n"
"	addiu	$6, $6, -0x8\n"
"	addiu	$5, $5, 0x8\n"
"	sltiu	$2, $6, 0x8\n"
"	sd	$3, 0($7)\n"
"	beqz	$2, $Lmemcpy_dw\n"
"	addiu	$7, $7, 0x8\n"
"	daddu	$3, $7, $0\n"
"$Lmemcpy_tail:\n"			/* while (len--) *dst++ = *src++; */
"	addiu	$6, $6, -0x1\n"
"$Lmemcpy_tail1:\n"
"	addiu	$2, $0, -0x1\n"
"	beq	$6, $2, $Lmemcpy_ret\n"
"	daddu	$4, $2, $0\n"
"$Lmemcpy_bytes:\n"
"	lbu	$2, 0($5)\n"
"	addiu	$6, $6, -0x1\n"
"	addiu	$5, $5, 0x1\n"
"	sb	$2, 0($3)\n"
"	.word	0x14C4FFFB\n"		/* bne $6, $4, $Lmemcpy_bytes */
"	addiu	$3, $3, 0x1\n"
"$Lmemcpy_ret:\n"
"	jr	$31\n"
"	daddu	$2, $8, $0\n"
LIBC_ASM_END(memcpy));
#else
LIBC_FN void *memcpy(void *dst0, const void *src0, size_t len0)
{
    char       *dst = dst0;
    const char *src = src0;
    u128_a       *aligned_dst;
    const u128_a *aligned_src;
    int         len = len0;

    if (!TOO_SMALL(len) && !UNALIGNED16(src, dst)) {
        aligned_dst = (u128_a *)dst;
        aligned_src = (const u128_a *)src;
        while (len >= BIGBLOCKSIZE) {
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            len -= BIGBLOCKSIZE;
        }
        while (len >= LITTLEBLOCKSIZE) {
            *(u64_a *)aligned_dst = *(const u64_a *)aligned_src;
            aligned_dst = (u128_a *)((u64_a *)aligned_dst + 1);
            aligned_src = (const u128_a *)((const u64_a *)aligned_src + 1);
            len -= LITTLEBLOCKSIZE;
        }
        dst = (char *)aligned_dst;
        src = (const char *)aligned_src;
    }
    while (len--)
        *dst++ = *src++;
    return dst0;
}
#endif

// FUNC 0x0027DA3C memmove
/* Sony EE libc memmove.  Only when src < dst < src + len (unsigned compares) does it
 * copy backwards, one byte at a time; otherwise it runs the same forward path as
 * memcpy.  Returns dst_void. */
#ifdef LIBC_RETAIL_ASM
__asm__(LIBC_ASM_BEGIN(memmove, 2)
"	daddu	$8, $4, $0\n"		/* dst_void */
"	sltu	$2, $5, $8\n"		/* src < dst ... */
"	beqz	$2, $Lmemmove_fwd\n"
"	daddu	$3, $8, $0\n"		/* dst */
"	addu	$7, $5, $6\n"
"	sltu	$2, $8, $7\n"		/* ... && dst < src + len: backwards */
"	beqz	$2, $Lmemmove_fwd\n"
"	addiu	$2, $0, -0x1\n"
"	addu	$3, $8, $6\n"		/* dst += len */
"	addiu	$6, $6, -0x1\n"
"	beq	$6, $2, $Lmemmove_ret\n"
"	daddu	$5, $7, $0\n"		/* src += len */
"	daddu	$4, $2, $0\n"
"$Lmemmove_back:\n"			/* while (len--) *--dst = *--src; */
"	addiu	$5, $5, -0x1\n"
"	addiu	$3, $3, -0x1\n"
"	lbu	$2, 0($5)\n"
"	addiu	$6, $6, -0x1\n"
"	.word	0x14C4FFFB\n"		/* bne $6, $4, $Lmemmove_back */
"	sb	$2, 0($3)\n"
"	jr	$31\n"
"	daddu	$2, $8, $0\n"
"$Lmemmove_fwd:\n"			/* forward: memcpy's algorithm */
"	sltiu	$2, $6, 0x20\n"
"	bnel	$2, $0, $Lmemmove_tail1\n"
"	addiu	$6, $6, -0x1\n"
"	or	$2, $5, $3\n"
"	andi	$2, $2, 0xF\n"
"	bnel	$2, $0, $Lmemmove_tail1\n"
"	addiu	$6, $6, -0x1\n"
"	daddu	$7, $3, $0\n"		/* aligned_dst */
"$Lmemmove_qw:\n"			/* 32 bytes per iteration */
"	lq	$3, 0($5)\n"
"	addiu	$6, $6, -0x20\n"
"	addiu	$5, $5, 0x10\n"
"	sltiu	$4, $6, 0x20\n"
"	sq	$3, 0($7)\n"
"	addiu	$7, $7, 0x10\n"
"	lq	$2, 0($5)\n"
"	addiu	$5, $5, 0x10\n"
"	sq	$2, 0($7)\n"
"	beqz	$4, $Lmemmove_qw\n"
"	addiu	$7, $7, 0x10\n"
"	sltiu	$2, $6, 0x8\n"
"	bnez	$2, $Lmemmove_tail\n"
"	daddu	$3, $7, $0\n"
"$Lmemmove_dw:\n"			/* 8 bytes per iteration */
"	ld	$3, 0($5)\n"
"	addiu	$6, $6, -0x8\n"
"	addiu	$5, $5, 0x8\n"
"	sltiu	$2, $6, 0x8\n"
"	sd	$3, 0($7)\n"
"	beqz	$2, $Lmemmove_dw\n"
"	addiu	$7, $7, 0x8\n"
"	daddu	$3, $7, $0\n"
"$Lmemmove_tail:\n"			/* while (len--) *dst++ = *src++; */
"	addiu	$6, $6, -0x1\n"
"$Lmemmove_tail1:\n"
"	addiu	$2, $0, -0x1\n"
"	beq	$6, $2, $Lmemmove_ret\n"
"	daddu	$4, $2, $0\n"
"$Lmemmove_bytes:\n"
"	lbu	$2, 0($5)\n"
"	addiu	$6, $6, -0x1\n"
"	addiu	$5, $5, 0x1\n"
"	sb	$2, 0($3)\n"
"	.word	0x14C4FFFB\n"		/* bne $6, $4, $Lmemmove_bytes */
"	addiu	$3, $3, 0x1\n"
"$Lmemmove_ret:\n"
"	jr	$31\n"
"	daddu	$2, $8, $0\n"
LIBC_ASM_END(memmove));
#else
LIBC_FN void *memmove(void *dst_void, const void *src_void, size_t length)
{
    char       *dst = dst_void;
    const char *src = src_void;
    u128_a       *aligned_dst;
    const u128_a *aligned_src;
    int         len = length;

    if (src < dst && dst < src + len) {
        /* Destructive overlap: copy backwards. */
        src += len;
        dst += len;
        while (len--)
            *--dst = *--src;
    } else {
        if (!TOO_SMALL(len) && !UNALIGNED16(src, dst)) {
            aligned_dst = (u128_a *)dst;
            aligned_src = (const u128_a *)src;
            while (len >= BIGBLOCKSIZE) {
                *aligned_dst++ = *aligned_src++;
                *aligned_dst++ = *aligned_src++;
                len -= BIGBLOCKSIZE;
            }
            while (len >= LITTLEBLOCKSIZE) {
                *(u64_a *)aligned_dst = *(const u64_a *)aligned_src;
                aligned_dst = (u128_a *)((u64_a *)aligned_dst + 1);
                aligned_src = (const u128_a *)((const u64_a *)aligned_src + 1);
                len -= LITTLEBLOCKSIZE;
            }
            dst = (char *)aligned_dst;
            src = (const char *)aligned_src;
        }
        while (len--)
            *dst++ = *src++;
    }
    return dst_void;
}
#endif

// FUNC 0x0027DB38 memset
/* Sony EE libc memset.  If n >= 8 and m is 16-aligned the byte is broadcast
 * ((c & 0xff) << 8 | (c & 0xff), pcpyh to a doubleword, pcpyld to a quadword): 32
 * bytes per iteration with two sq while n >= 32, then 8 bytes per sd while n >= 8.
 * The remainder (or everything, when unaligned or n < 8) is stored bytewise with
 * the low byte of c.  Returns m.  82 callers. */
#ifdef LIBC_RETAIL_ASM
__asm__(LIBC_ASM_BEGIN(memset, 2)
"	sltiu	$2, $6, 0x8\n"		/* n < 8: bytewise */
"	bnez	$2, $Lmemset_tail\n"
"	daddu	$3, $4, $0\n"		/* s */
"	andi	$2, $4, 0xF\n"		/* m & 15: bytewise */
"	bnez	$2, $Lmemset_tail\n"
"	daddu	$7, $4, $0\n"		/* aligned_addr */
"	andi	$9, $5, 0xFF\n"
"	sltiu	$10, $6, 0x20\n"
"	daddu	$8, $9, $0\n"
"	dsll	$3, $8, 8\n"
"	or	$8, $3, $9\n"		/* (c & 0xff) << 8 | (c & 0xff) */
"	pcpyh	$3, $8\n"		/* buffer: the byte 8 times */
"	bnez	$10, $Lmemset_dwtest\n"
"	sltiu	$2, $6, 0x8\n"
"	pcpyld	$8, $3, $3\n"		/* qbuffer: the byte 16 times */
"$Lmemset_qw:\n"			/* 32 bytes per iteration */
"	sq	$8, 0($7)\n"
"	addiu	$6, $6, -0x20\n"
"	addiu	$7, $7, 0x10\n"
"	sltiu	$2, $6, 0x20\n"
"	sq	$8, 0($7)\n"
"	beqz	$2, $Lmemset_qw\n"
"	addiu	$7, $7, 0x10\n"
"	b	$Lmemset_dwtest\n"
"	sltiu	$2, $6, 0x8\n"
"$Lmemset_dw:\n"			/* 8 bytes per iteration (sd in the slot below) */
"	addiu	$6, $6, -0x8\n"
"	addiu	$7, $7, 0x8\n"
"	sltiu	$2, $6, 0x8\n"
"$Lmemset_dwtest:\n"
"	.word	0x5040FFFC\n"		/* beql $2, $0, $Lmemset_dw */
"	sd	$3, 0($7)\n"
"	daddu	$3, $7, $0\n"
"$Lmemset_tail:\n"			/* while (n--) *s++ = c; (n unsigned) */
"	lui	$2, 0xFFFF\n"
"	addiu	$6, $6, -0x1\n"
"	ori	$2, $2, 0xFFFF\n"
"	beq	$6, $2, $Lmemset_ret\n"
"	nop\n"
"	lui	$2, 0xFFFF\n"
"	ori	$2, $2, 0xFFFF\n"
"$Lmemset_bytes:\n"
"	sb	$5, 0($3)\n"
"	addiu	$6, $6, -0x1\n"
"	.word	0x14C2FFFD\n"		/* bne $6, $2, $Lmemset_bytes */
"	addiu	$3, $3, 0x1\n"
"$Lmemset_ret:\n"
"	jr	$31\n"
"	daddu	$2, $4, $0\n"
LIBC_ASM_END(memset));
#else
LIBC_FN void *memset(void *m, int c, size_t n)
{
    char *s = (char *)m;
    u64   buffer;
    u64_a  *aligned_addr;

    if (!(n < LITTLEBLOCKSIZE) && !((u32)m & 15)) {
        aligned_addr = (u64_a *)m;
        buffer = c & 0xff;
        buffer = (buffer << 8) | (c & 0xff);
        DW_PCPYH(buffer, buffer);
        if (n >= BIGBLOCKSIZE) {
            u128 qbuffer;

            QW_SPLAT(qbuffer, buffer);
            do {
                *(u128_a *)aligned_addr = qbuffer;
                aligned_addr += 2;
                *(u128_a *)aligned_addr = qbuffer;
                aligned_addr += 2;
                n -= BIGBLOCKSIZE;
            } while (n >= BIGBLOCKSIZE);
        }
        while (n >= LITTLEBLOCKSIZE) {
            *aligned_addr++ = buffer;
            n -= LITTLEBLOCKSIZE;
        }
        s = (char *)aligned_addr;
    }
    while (n--)
        *s++ = (char)c;
    return m;
}
#endif

/* newlib tolower() (ctype.h macro): isupper(c) ? c - 'A' + 'a' : c. */
static inline int ctype_tolower(int c)
{
    return ISUPPER(c) ? c + ('a' - 'A') : c;
}

// FUNC 0x0027E6E0 strcasecmp
/* newlib strcasecmp.  The loop compares tolower() of the signed chars (lb),
 * the final difference uses the unsigned chars (lbu).  One caller,
 * iso9660_find_in_dir. */
int strcasecmp(const char *s1, const char *s2)
{
    while (*s1 != '\0' && ctype_tolower(*s1) == ctype_tolower(*s2)) {
        s1++;
        s2++;
    }
    return ctype_tolower(*(const unsigned char *)s1) - ctype_tolower(*(const unsigned char *)s2);
}

// FUNC 0x0027E794 strcat
/* Sony EE libc strcat.  If s1 is 8-aligned, the end of s1 is located a quadword
 * (16-aligned s1, MMI test) or doubleword (8-aligned s1, 64-bit test) at a time; the
 * NUL is then found bytewise from the start of the word that holds it (from s1
 * itself when s1 is not 8-aligned).  The copy is a call to strcpy (0x0027EB84),
 * whose result is ignored; returns s1.  48 callers. */
#ifdef LIBC_RETAIL_ASM
__asm__(LIBC_ASM_BEGIN(strcat, 2)
"	addiu	$29, $29, -0x20\n"
"	sq	$16, 0($29)\n"
"	daddu	$16, $4, $0\n"		/* s = s1 */
"	andi	$2, $16, 0x7\n"		/* s1 & 7: byte scan from s1 */
"	bnez	$2, $Lstrcat_bytes\n"
"	sq	$31, 0x10($29)\n"
"	andi	$2, $16, 0xF\n"
"	lui	$3, 0x101\n"		/* lsb = 0x0101010101010101 */
"	ori	$3, $3, 0x101\n"
"	dsll	$3, $3, 16\n"
"	ori	$3, $3, 0x101\n"
"	dsll	$3, $3, 16\n"
"	ori	$3, $3, 0x101\n"
"	lui	$4, 0x8080\n"		/* msb = 0x8080808080808080 */
"	ori	$4, $4, 0x8080\n"
"	dsll	$4, $4, 16\n"
"	ori	$4, $4, 0x8080\n"
"	dsll	$4, $4, 16\n"
"	ori	$4, $4, 0x8080\n"
"	bnez	$2, $Lstrcat_dw\n"	/* 8- but not 16-aligned */
"	ld	$6, 0($16)\n"
"	lq	$2, 0($16)\n"		/* 16-aligned: quadword NUL test */
"	pcpyld	$7, $3, $3\n"		/* ones */
"	pcpyld	$8, $4, $4\n"		/* highs */
"	psubb	$3, $2, $7\n"
"	pnor	$2, $0, $2\n"
"	pand	$3, $3, $2\n"
"	pand	$3, $3, $8\n"
"	pcpyud	$2, $3, $3\n"
"	or	$3, $2, $3\n"
"	bnez	$3, $Lstrcat_bytes\n"
"	daddu	$4, $16, $0\n"
"	addiu	$6, $4, 0x10\n"
"$Lstrcat_qwloop:\n"
"	lq	$2, 0($6)\n"
"	pnor	$3, $0, $2\n"
"	psubb	$2, $2, $7\n"
"	pand	$2, $2, $3\n"
"	pand	$2, $2, $8\n"
"	pcpyud	$3, $2, $2\n"
"	or	$2, $2, $3\n"
"	beql	$2, $0, $Lstrcat_qwloop\n"
"	addiu	$6, $6, 0x10\n"
"	b	$Lstrcat_bytes\n"
"	daddu	$4, $6, $0\n"
"$Lstrcat_dw:\n"			/* doubleword NUL test (DETECTNULL) */
"	daddu	$7, $3, $0\n"
"	daddu	$8, $4, $0\n"
"	dsubu	$3, $6, $3\n"
"	nor	$2, $0, $6\n"
"	and	$3, $3, $2\n"
"	and	$3, $3, $4\n"
"	bnez	$3, $Lstrcat_bytes\n"
"	daddu	$4, $16, $0\n"
"	addiu	$6, $16, 0x8\n"
"$Lstrcat_dwloop:\n"
"	ld	$2, 0($6)\n"
"	nor	$3, $0, $2\n"
"	dsubu	$2, $2, $7\n"
"	and	$2, $2, $3\n"
"	and	$2, $2, $8\n"
"	beql	$2, $0, $Lstrcat_dwloop\n"
"	addiu	$6, $6, 0x8\n"
"	daddu	$4, $6, $0\n"
"$Lstrcat_bytes:\n"			/* while (*s1) s1++; */
"	lb	$2, 0($4)\n"
"	.word	0x5440FFFE\n"		/* bnel $2, $0, $Lstrcat_bytes */
"	addiu	$4, $4, 0x1\n"
"	jal	strcpy\n"		/* strcpy(s1, s2) */
"	nop\n"
"	daddu	$2, $16, $0\n"		/* return s */
"	lq	$31, 0x10($29)\n"
"	lq	$16, 0($29)\n"
"	jr	$31\n"
"	addiu	$29, $29, 0x20\n"
LIBC_ASM_END(strcat));
#else
LIBC_FN char *strcat(char *s1, const char *s2)
{
    char *s = s1;

    if (!((u32)s1 & 7)) {
        u64 lsb = LSB8, msb = MSB8;

        if (!((u32)s1 & 15)) {
            u128_a *aligned_s1 = (u128_a *)s1;
            u128  ones, highs;
            u64   nul;

            QW_SPLAT(ones, lsb);
            QW_SPLAT(highs, msb);
            QW_NUL(nul, *aligned_s1, ones, highs);
            while (!nul) {
                aligned_s1++;
                QW_NUL(nul, *aligned_s1, ones, highs);
            }
            s1 = (char *)aligned_s1;
        } else {
            u64_a *aligned_s1 = (u64_a *)s1;

            while (!DETECTNULL(*aligned_s1, lsb, msb))
                aligned_s1++;
            s1 = (char *)aligned_s1;
        }
    }
    while (*s1)
        s1++;
    strcpy(s1, s2);
    return s;
}
#endif

// FUNC 0x0027E8B0 strchr
/* Sony EE libc strchr (MMI family; not in the bootstrap).  c is reduced to an
 * unsigned char and broadcast to a doubleword ((c << 8) + c, then << 16, << 32,
 * with daddu).  When s is 8-aligned it is scanned a quadword (16-aligned s:
 * QW_NUL of x and of x ^ mask, the constants rebuilt each round) or doubleword
 * (DETECTNULL of x and of x ^ mask) at a time while the word holds neither a
 * NUL nor c; the rest is bytewise.  Returns s if *s == c (also for c == 0),
 * else NULL.  Callers: gt2k_menu::mcDecodeChar, gt2k_menu::layoutBaseName,
 * gt2k_menu::RawWindow::getShortName.
 * Matching build: the retail instructions, as for its bootstrap siblings
 * (pcpyud's third register holds no operand: $7 / $5 here, $4 in strcmp and
 * strncmp, $7 in memcmp; no C + MMI asm form reproduces that, see
 * ref/matching/u137/notes.md).  The C below is the GCC 14 / NON_MATCHING model. */
#ifdef LIBC_RETAIL_ASM
__asm__(LIBC_ASM_BEGIN(strchr, 3)
"	andi	$2, $4, 0x7\n"		/* s & 7: bytewise */
"	bnez	$2, $Lstrchr_test\n"
"	andi	$5, $5, 0xFF\n"		/* c = (unsigned char)i */
"	dsll	$3, $5, 8\n"
"	lui	$6, 0x101\n"		/* lsb = 0x0101010101010101 */
"	ori	$6, $6, 0x101\n"
"	dsll	$6, $6, 16\n"
"	ori	$6, $6, 0x101\n"
"	dsll	$6, $6, 16\n"
"	ori	$6, $6, 0x101\n"
"	daddu	$10, $3, $5\n"		/* mask = (c << 8) + c */
"	andi	$3, $4, 0xF\n"
"	dsll	$2, $10, 16\n"
"	lui	$8, 0x8080\n"		/* msb = 0x8080808080808080 */
"	ori	$8, $8, 0x8080\n"
"	dsll	$8, $8, 16\n"
"	ori	$8, $8, 0x8080\n"
"	dsll	$8, $8, 16\n"
"	ori	$8, $8, 0x8080\n"
"	daddu	$2, $2, $10\n"		/* mask = (mask << 16) + mask */
"	dsll32	$10, $2, 0\n"
"	bnez	$3, $Lstrchr_dw\n"	/* 8- but not 16-aligned */
"	daddu	$7, $2, $10\n"		/* mask = (mask << 32) + mask */
"$Lstrchr_qwloop:\n"			/* quadwords: QW_NUL(x), QW_NUL(x ^ cmask) */
"	lq	$9, 0($4)\n"
"	pcpyld	$10, $6, $6\n"		/* ones */
"	pnor	$3, $0, $9\n"
"	psubb	$2, $9, $10\n"
"	pcpyld	$6, $8, $8\n"		/* highs */
"	pand	$2, $2, $3\n"
"	pcpyld	$8, $7, $7\n"		/* cmask */
"	pand	$2, $2, $6\n"
"	pcpyud	$3, $2, $7\n"
"	or	$3, $2, $3\n"
"	bnel	$3, $0, $Lstrchr_bytes\n"
"	lbu	$2, 0($4)\n"
"	pxor	$2, $9, $8\n"
"	psubb	$3, $2, $10\n"
"	pnor	$2, $0, $2\n"
"	lui	$8, 0x8080\n"		/* msb again */
"	ori	$8, $8, 0x8080\n"
"	dsll	$8, $8, 16\n"
"	ori	$8, $8, 0x8080\n"
"	dsll	$8, $8, 16\n"
"	ori	$8, $8, 0x8080\n"
"	pand	$3, $3, $2\n"
"	pand	$3, $3, $6\n"
"	lui	$6, 0x101\n"		/* lsb again */
"	ori	$6, $6, 0x101\n"
"	dsll	$6, $6, 16\n"
"	ori	$6, $6, 0x101\n"
"	dsll	$6, $6, 16\n"
"	ori	$6, $6, 0x101\n"
"	pcpyud	$2, $3, $5\n"
"	or	$3, $2, $3\n"
"	beql	$3, $0, $Lstrchr_qwloop\n"
"	addiu	$4, $4, 0x10\n"
"	b	$Lstrchr_bytes\n"
"	lbu	$2, 0($4)\n"
"$Lstrchr_dw:\n"			/* doublewords: DETECTNULL(x), DETECTNULL(x ^ mask) */
"	ld	$9, 0($4)\n"
"	nor	$3, $0, $9\n"
"	dsubu	$2, $9, $6\n"
"	and	$2, $2, $3\n"
"	and	$2, $2, $8\n"
"	bnel	$2, $0, $Lstrchr_bytes\n"
"	lbu	$2, 0($4)\n"
"	xor	$2, $9, $7\n"
"	dsubu	$3, $2, $6\n"
"	nor	$2, $0, $2\n"
"	and	$3, $3, $2\n"
"	and	$3, $3, $8\n"
"	bnel	$3, $0, $Lstrchr_bytes\n"
"	lbu	$2, 0($4)\n"
"	daddu	$9, $6, $0\n"
"	addiu	$4, $4, 0x8\n"
"$Lstrchr_dwloop:\n"
"	ld	$6, 0($4)\n"
"	dsubu	$2, $6, $9\n"
"	nor	$3, $0, $6\n"
"	and	$2, $2, $3\n"
"	and	$2, $2, $8\n"
"	bnez	$2, $Lstrchr_test\n"
"	xor	$2, $6, $7\n"
"	nor	$3, $0, $2\n"
"	dsubu	$2, $2, $9\n"
"	and	$2, $2, $3\n"
"	and	$2, $2, $8\n"
"	beql	$2, $0, $Lstrchr_dwloop\n"
"	addiu	$4, $4, 0x8\n"
"	b	$Lstrchr_bytes\n"
"	lbu	$2, 0($4)\n"
"$Lstrchr_next:\n"			/* while (*s && *s != c) s++; */
"	beql	$2, $5, $Lstrchr_end\n"
"	lbu	$3, 0($4)\n"
"	addiu	$4, $4, 0x1\n"
"$Lstrchr_test:\n"
"	lbu	$2, 0($4)\n"
"$Lstrchr_bytes:\n"
"	.word	0x1440FFFB\n"		/* bnez $2, $Lstrchr_next */
"	nop\n"
"	lbu	$3, 0($4)\n"
"$Lstrchr_end:\n"			/* return *s == c ? s : NULL */
"	daddu	$2, $0, $0\n"
"	xor	$3, $3, $5\n"
"	jr	$31\n"
"	movz	$2, $4, $3\n"
LIBC_ASM_END(strchr));
#else
LIBC_FN char *strchr(const char *s1, int i)
{
    const unsigned char *s = (const unsigned char *)s1;
    unsigned char        c = (unsigned char)i;

    if (!((u32)s & 7)) {
        u64 lsb = LSB8, msb = MSB8, mask;

        mask = c;
        mask = (mask << 8) + mask;
        mask = (mask << 16) + mask;
        mask = (mask << 32) + mask;
        if (!((u32)s & 15)) {
            u128 ones, highs, cmask, word, cword;
            u64  hit;

            for (;;) {
                word = *(const u128_a *)s;
                QW_SPLAT(ones, lsb);
                QW_SPLAT(highs, msb);
                QW_SPLAT(cmask, mask);
                QW_NUL(hit, word, ones, highs);
                if (hit)
                    break;
                QW_XOR(cword, word, cmask);
                QW_NUL(hit, cword, ones, highs);
                if (hit)
                    break;
                s += sizeof (u128);
            }
        } else {
            u64 word = *(const u64_a *)s;

            while (!DETECTNULL(word, lsb, msb) && !DETECTNULL(word ^ mask, lsb, msb)) {
                s += sizeof (u64);
                word = *(const u64_a *)s;
            }
        }
    }
    while (*s && *s != c)
        s++;
    if (*s == c)
        return (char *)s;
    return NULL;
}
#endif

// FUNC 0x0027EA40 strcmp
/* Sony EE libc strcmp.  When s1 and s2 are both 8-aligned, equal quadwords (both
 * 16-aligned) or doublewords are skipped; if a skipped word of s1 contains a NUL
 * the strings are equal and 0 is returned directly.  At the first differing word
 * (or immediately, when unaligned) a byte loop runs while *s1 != 0 && *s1 == *s2
 * and returns (unsigned char)*s1 - (unsigned char)*s2.  48 callers. */
#ifdef LIBC_RETAIL_ASM
__asm__(LIBC_ASM_BEGIN(strcmp, 2)
"	or	$8, $4, $5\n"
"	andi	$2, $8, 0x7\n"		/* (s1 | s2) & 7: bytewise */
"	bnel	$2, $0, $Lstrcmp_test\n"
"	lb	$2, 0($4)\n"
"	andi	$9, $8, 0xF\n"
"	lui	$7, 0x101\n"		/* lsb = 0x0101010101010101 */
"	ori	$7, $7, 0x101\n"
"	dsll	$7, $7, 16\n"
"	ori	$7, $7, 0x101\n"
"	dsll	$7, $7, 16\n"
"	ori	$7, $7, 0x101\n"
"	lui	$6, 0x8080\n"		/* msb = 0x8080808080808080 */
"	ori	$6, $6, 0x8080\n"
"	dsll	$6, $6, 16\n"
"	ori	$6, $6, 0x8080\n"
"	dsll	$6, $6, 16\n"
"	ori	$6, $6, 0x8080\n"
"	bnez	$9, $Lstrcmp_dw\n"	/* 8- but not 16-aligned */
"	ld	$2, 0($5)\n"
"	lq	$3, 0($4)\n"		/* both 16-aligned */
"	pcpyld	$8, $7, $7\n"		/* ones */
"	lq	$2, 0($5)\n"
"	pcpyld	$10, $6, $6\n"		/* highs */
"	psubw	$7, $2, $3\n"		/* *a2 - *a1 */
"	pcpyud	$6, $7, $4\n"
"	or	$3, $6, $7\n"
"	bnel	$3, $0, $Lstrcmp_test\n"	/* quadwords differ: bytewise */
"	lb	$2, 0($4)\n"
"	lq	$2, 0($4)\n"
"	pnor	$3, $0, $2\n"
"$Lstrcmp_qwnul:\n"			/* equal quadwords: NUL in *a1? */
"	psubb	$2, $2, $8\n"
"	pand	$2, $2, $3\n"
"	pand	$2, $2, $10\n"
"	pcpyud	$3, $2, $4\n"
"	or	$6, $3, $2\n"
"	beqz	$6, $Lstrcmp_qwnext\n"
"	addiu	$4, $4, 0x10\n"
"	jr	$31\n"			/* equal strings: 0 */
"	daddu	$2, $0, $0\n"
"$Lstrcmp_qwnext:\n"
"	addiu	$5, $5, 0x10\n"
"	lq	$2, 0($4)\n"
"	lq	$3, 0($5)\n"
"	psubw	$7, $2, $3\n"
"	pcpyud	$6, $7, $4\n"
"	or	$9, $6, $7\n"
"	beql	$9, $0, $Lstrcmp_qwnul\n"
"	pnor	$3, $0, $2\n"
"	b	$Lstrcmp_test\n"
"	lb	$2, 0($4)\n"
"$Lstrcmp_dw:\n"			/* doublewords */
"	ld	$3, 0($4)\n"
"	bnel	$3, $2, $Lstrcmp_test\n"
"	lb	$2, 0($4)\n"
"	ld	$2, 0($4)\n"
"	nor	$8, $0, $2\n"
"$Lstrcmp_dwnul:\n"			/* equal doublewords: DETECTNULL(*a1)? */
"	dsubu	$2, $2, $7\n"
"	and	$2, $2, $8\n"
"	and	$2, $2, $6\n"
"	beqz	$2, $Lstrcmp_dwnext\n"
"	addiu	$4, $4, 0x8\n"
"	jr	$31\n"			/* equal strings: 0 */
"	daddu	$2, $0, $0\n"
"$Lstrcmp_dwnext:\n"
"	addiu	$5, $5, 0x8\n"
"	ld	$2, 0($4)\n"
"	ld	$3, 0($5)\n"
"	beql	$3, $2, $Lstrcmp_dwnul\n"
"	nor	$8, $0, $2\n"
"	b	$Lstrcmp_test\n"
"	lb	$2, 0($4)\n"
"$Lstrcmp_bytes:\n"			/* while (*s1 && *s1 == *s2) s1++, s2++; */
"	sll	$2, $3, 24\n"
"	lb	$3, 0($5)\n"
"	sra	$2, $2, 24\n"
"	bnel	$2, $3, $Lstrcmp_diff\n"
"	lbu	$3, 0($4)\n"
"	addiu	$4, $4, 0x1\n"
"	addiu	$5, $5, 0x1\n"
"	lb	$2, 0($4)\n"
"$Lstrcmp_test:\n"
"	bnez	$2, $Lstrcmp_bytes\n"
"	lbu	$3, 0($4)\n"
"$Lstrcmp_diff:\n"			/* (unsigned char)*s1 - (unsigned char)*s2 */
"	lbu	$2, 0($5)\n"
"	jr	$31\n"
"	subu	$2, $3, $2\n"
LIBC_ASM_END(strcmp));
#else
LIBC_FN int strcmp(const char *s1, const char *s2)
{
    if (!UNALIGNED8(s1, s2)) {
        u64 lsb = LSB8, msb = MSB8;

        if (!UNALIGNED16(s1, s2)) {
            const u128_a *a1 = (const u128_a *)s1;
            const u128_a *a2 = (const u128_a *)s2;
            u128        ones, highs;
            u64         hit;

            QW_SPLAT(ones, lsb);
            QW_SPLAT(highs, msb);
            for (;;) {
                QW_NE_SUBW(hit, *a2, *a1);
                if (hit)
                    break;
                /* Equal quadwords: a NUL in them means equal strings. */
                QW_NUL(hit, *a1, ones, highs);
                if (hit)
                    return 0;
                a1++;
                a2++;
            }
            s1 = (const char *)a1;
            s2 = (const char *)a2;
        } else {
            const u64_a *a1 = (const u64_a *)s1;
            const u64_a *a2 = (const u64_a *)s2;

            while (*a1 == *a2) {
                if (DETECTNULL(*a1, lsb, msb))
                    return 0;
                a1++;
                a2++;
            }
            s1 = (const char *)a1;
            s2 = (const char *)a2;
        }
    }
    while (*s1 != '\0' && *s1 == *s2) {      /* lb / lb: signed bytes, equality only */
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}
#endif

// FUNC 0x0027EB84 strcpy
/* Sony EE libc strcpy.  When src and dst are both 8-aligned, whole quadwords
 * (both 16-aligned; MMI test) or doublewords (64-bit test) without a NUL are
 * copied; the word holding the NUL (or everything, when unaligned) is copied
 * bytewise up to and including the terminator.  Returns dst0.  216 callers. */
#ifdef LIBC_RETAIL_ASM
__asm__(LIBC_ASM_BEGIN(strcpy, 2)
"	daddu	$7, $4, $0\n"		/* dst0 */
"	or	$8, $5, $7\n"
"	andi	$2, $8, 0x7\n"		/* (src | dst) & 7: bytewise */
"	bnez	$2, $Lstrcpy_bytes\n"
"	daddu	$3, $7, $0\n"		/* dst */
"	andi	$2, $8, 0xF\n"
"	lui	$9, 0x101\n"		/* lsb = 0x0101010101010101 */
"	ori	$9, $9, 0x101\n"
"	dsll	$9, $9, 16\n"
"	ori	$9, $9, 0x101\n"
"	dsll	$9, $9, 16\n"
"	ori	$9, $9, 0x101\n"
"	lui	$4, 0x8080\n"		/* msb = 0x8080808080808080 */
"	ori	$4, $4, 0x8080\n"
"	dsll	$4, $4, 16\n"
"	ori	$4, $4, 0x8080\n"
"	dsll	$4, $4, 16\n"
"	ori	$4, $4, 0x8080\n"
"	bnel	$2, $0, $Lstrcpy_dw\n"	/* 8- but not 16-aligned */
"	ld	$10, 0($5)\n"
"	pcpyld	$10, $9, $9\n"		/* ones */
"	lq	$9, 0($5)\n"		/* x */
"	pcpyld	$8, $4, $4\n"		/* highs */
"	psubb	$2, $9, $10\n"
"	pnor	$3, $0, $9\n"
"	pand	$2, $2, $3\n"
"	pand	$2, $2, $8\n"
"	pcpyud	$4, $2, $9\n"
"	or	$3, $2, $4\n"
"	bnez	$3, $Lstrcpy_aligned_end\n"
"	daddu	$6, $7, $0\n"
"$Lstrcpy_qwloop:\n"			/* copy quadwords without a NUL */
"	sq	$9, 0($6)\n"
"	addiu	$5, $5, 0x10\n"
"	lq	$9, 0($5)\n"
"	psubb	$2, $9, $10\n"
"	pnor	$3, $0, $9\n"
"	pand	$2, $2, $3\n"
"	pand	$2, $2, $8\n"
"	pcpyud	$4, $2, $9\n"
"	or	$3, $2, $4\n"
"	beqz	$3, $Lstrcpy_qwloop\n"
"	addiu	$6, $6, 0x10\n"
"	b	$Lstrcpy_bytes\n"
"	daddu	$3, $6, $0\n"
"$Lstrcpy_dw:\n"			/* DETECTNULL(*aligned_src) */
"	dsubu	$2, $10, $9\n"
"	nor	$3, $0, $10\n"
"	and	$2, $2, $3\n"
"	and	$2, $2, $4\n"
"	bnez	$2, $Lstrcpy_aligned_end\n"
"	daddu	$6, $7, $0\n"
"$Lstrcpy_dwloop:\n"			/* copy doublewords without a NUL */
"	sd	$10, 0($6)\n"
"	addiu	$5, $5, 0x8\n"
"	ld	$10, 0($5)\n"
"	nor	$2, $0, $10\n"
"	dsubu	$3, $10, $9\n"
"	and	$3, $3, $2\n"
"	and	$3, $3, $4\n"
"	beqz	$3, $Lstrcpy_dwloop\n"
"	addiu	$6, $6, 0x8\n"
"$Lstrcpy_aligned_end:\n"
"	daddu	$3, $6, $0\n"
"$Lstrcpy_bytes:\n"			/* while ((*dst++ = *src++) != '\0'); */
"	lbu	$2, 0($5)\n"
"	addiu	$5, $5, 0x1\n"
"	sb	$2, 0($3)\n"
"	sll	$2, $2, 24\n"
"	.word	0x1440FFFB\n"		/* bnez $2, $Lstrcpy_bytes */
"	addiu	$3, $3, 0x1\n"
"	jr	$31\n"
"	daddu	$2, $7, $0\n"
LIBC_ASM_END(strcpy));
#else
LIBC_FN char *strcpy(char *dst0, const char *src0)
{
    char       *dst = dst0;
    const char *src = src0;

    if (!UNALIGNED8(src, dst)) {
        u64 lsb = LSB8, msb = MSB8;

        if (!UNALIGNED16(src, dst)) {
            u128_a       *aligned_dst = (u128_a *)dst;
            const u128_a *aligned_src = (const u128_a *)src;
            u128        ones, highs, word;
            u64         nul;

            QW_SPLAT(ones, lsb);
            QW_SPLAT(highs, msb);
            word = *aligned_src;
            QW_NUL(nul, word, ones, highs);
            while (!nul) {
                *aligned_dst++ = word;
                aligned_src++;
                word = *aligned_src;
                QW_NUL(nul, word, ones, highs);
            }
            dst = (char *)aligned_dst;
            src = (const char *)aligned_src;
        } else {
            u64_a       *aligned_dst = (u64_a *)dst;
            const u64_a *aligned_src = (const u64_a *)src;

            while (!DETECTNULL(*aligned_src, lsb, msb))
                *aligned_dst++ = *aligned_src++;
            dst = (char *)aligned_dst;
            src = (const char *)aligned_src;
        }
    }
    while ((*dst++ = *src++) != '\0')
        ;
    return dst0;
}
#endif

// FUNC 0x0027EC98 strncmp
/* Sony EE libc strncmp (MMI family; not in the bootstrap).  n == 0: 0.  When
 * both pointers are 8-aligned, equal words are skipped - quadwords (QW_NE_SUBW
 * *a1 - *a2) when both are 16-aligned and n >= 16, else doublewords if n >= 8 -
 * returning 0 when n runs out or the skipped word of s1 holds a NUL; the rest
 * is bytewise (lb compares, lbu difference).  Callers: iso9660_read_pvd,
 * mpegif::control.
 * Matching build: the retail instructions (see strchr).  The C below is the
 * GCC 14 / NON_MATCHING model. */
#ifdef LIBC_RETAIL_ASM
__asm__(LIBC_ASM_BEGIN(strncmp, 3)	/* retail: one nop after strcpy */
"	bnez	$6, $Lstrncmp_go\n"
"	or	$3, $4, $5\n"
"$Lstrncmp_ret0:\n"
"	jr	$31\n"			/* n == 0, or equal: 0 */
"	daddu	$2, $0, $0\n"
"$Lstrncmp_go:\n"
"	andi	$2, $3, 0x7\n"		/* (s1 | s2) & 7: bytewise */
"	bnez	$2, $Lstrncmp_bytes\n"
"	daddu	$2, $6, $0\n"
"	andi	$2, $3, 0xF\n"
"	sltiu	$7, $6, 0x10\n"
"	lui	$9, 0x101\n"		/* lsb = 0x0101010101010101 */
"	ori	$9, $9, 0x101\n"
"	dsll	$9, $9, 16\n"
"	ori	$9, $9, 0x101\n"
"	dsll	$9, $9, 16\n"
"	ori	$9, $9, 0x101\n"
"	or	$2, $2, $7\n"		/* ((s1 | s2) & 15) | (n < 16): doublewords */
"	bnez	$2, $Lstrncmp_dw\n"
"	daddu	$7, $4, $0\n"		/* a1 = s1 */
"	lq	$3, 0($4)\n"		/* quadwords: *a1 != *a2 (psubw)? */
"	pcpyld	$10, $9, $9\n"		/* ones */
"	lq	$2, 0($5)\n"
"	lui	$8, 0x8080\n"		/* msb = 0x8080808080808080 */
"	ori	$8, $8, 0x8080\n"
"	dsll	$8, $8, 16\n"
"	ori	$8, $8, 0x8080\n"
"	dsll	$8, $8, 16\n"
"	ori	$8, $8, 0x8080\n"
"	psubw	$3, $3, $2\n"
"	pcpyld	$9, $8, $8\n"		/* highs */
"	pcpyud	$2, $3, $4\n"
"	daddu	$8, $5, $0\n"		/* a2 = s2 */
"	or	$3, $2, $3\n"
"	bnez	$3, $Lstrncmp_bytes\n"
"	daddu	$2, $6, $0\n"
"	addiu	$6, $6, -0x10\n"
"$Lstrncmp_qwnext:\n"			/* n -= 16 done */
"	beqz	$6, $Lstrncmp_ret0\n"
"	nop\n"
"	lq	$2, 0($7)\n"		/* NUL in *a1: equal */
"	pnor	$3, $0, $2\n"
"	psubb	$2, $2, $10\n"
"	pand	$2, $2, $3\n"
"	pand	$3, $2, $9\n"
"	pcpyud	$2, $3, $4\n"
"	or	$2, $2, $3\n"
"	bnez	$2, $Lstrncmp_ret0\n"
"	addiu	$7, $7, 0x10\n"
"	sltiu	$2, $6, 0x10\n"
"	lq	$3, 0($7)\n"
"	bnez	$2, $Lstrncmp_tail\n"
"	addiu	$8, $8, 0x10\n"
"	lq	$2, 0($8)\n"
"	psubw	$3, $3, $2\n"
"	pcpyud	$2, $3, $4\n"
"	or	$2, $2, $3\n"
"	beql	$2, $0, $Lstrncmp_qwnext\n"
"	addiu	$6, $6, -0x10\n"
"	b	$Lstrncmp_tail2\n"
"	daddu	$4, $7, $0\n"
"$Lstrncmp_dw:\n"			/* doublewords: while (n >= 8 && *a1 == *a2) */
"	sltiu	$2, $6, 0x8\n"
"	bnez	$2, $Lstrncmp_tail\n"
"	daddu	$8, $5, $0\n"		/* a2 = s2 */
"	ld	$3, 0($4)\n"
"	ld	$2, 0($5)\n"
"	bne	$3, $2, $Lstrncmp_bytes\n"
"	daddu	$2, $6, $0\n"
"	addiu	$6, $6, -0x8\n"
"	lui	$10, 0x8080\n"		/* msb */
"	ori	$10, $10, 0x8080\n"
"	dsll	$10, $10, 16\n"
"	ori	$10, $10, 0x8080\n"
"	dsll	$10, $10, 16\n"
"	ori	$10, $10, 0x8080\n"
"$Lstrncmp_dwnext:\n"
"	beqz	$6, $Lstrncmp_ret0\n"
"	nop\n"
"	ld	$2, 0($7)\n"		/* DETECTNULL(*a1): equal */
"	nor	$3, $0, $2\n"
"	dsubu	$2, $2, $9\n"
"	and	$2, $2, $3\n"
"	and	$2, $2, $10\n"
"	bnez	$2, $Lstrncmp_ret0\n"
"	addiu	$7, $7, 0x8\n"
"	sltiu	$2, $6, 0x8\n"
"	bnez	$2, $Lstrncmp_tail\n"
"	addiu	$8, $8, 0x8\n"
"	ld	$3, 0($7)\n"
"	ld	$2, 0($8)\n"
"	beql	$3, $2, $Lstrncmp_dwnext\n"
"	addiu	$6, $6, -0x8\n"
"$Lstrncmp_tail:\n"			/* s1 = a1, s2 = a2 */
"	daddu	$4, $7, $0\n"
"$Lstrncmp_tail2:\n"
"	daddu	$5, $8, $0\n"
"	daddu	$2, $6, $0\n"
"$Lstrncmp_bytes:\n"			/* while (n-- > 0 && *s1 == *s2) */
"	beqz	$2, $Lstrncmp_diff\n"
"	addiu	$6, $6, -0x1\n"
"	b	$Lstrncmp_cmp\n"
"	lb	$3, 0($4)\n"
"	nop\n"
"$Lstrncmp_loop:\n"			/* if (n == 0 || *s1 == 0) return 0; */
"	beqz	$6, $Lstrncmp_ret0\n"
"	nop\n"
"	beqz	$7, $Lstrncmp_ret0\n"
"	addiu	$4, $4, 0x1\n"
"	addiu	$5, $5, 0x1\n"
"	lb	$3, 0($4)\n"
"	addiu	$6, $6, -0x1\n"
"$Lstrncmp_cmp:\n"
"	lb	$2, 0($5)\n"
"	beq	$3, $2, $Lstrncmp_loop\n"
"	lbu	$7, 0($4)\n"
"$Lstrncmp_diff:\n"			/* (unsigned char)*s1 - (unsigned char)*s2 */
"	lbu	$2, 0($5)\n"
"	lbu	$3, 0($4)\n"
"	jr	$31\n"
"	subu	$2, $3, $2\n"
LIBC_ASM_END(strncmp));
#else
LIBC_FN int strncmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0)
        return 0;
    if (!UNALIGNED8(s1, s2)) {
        u64 lsb = LSB8, msb = MSB8;

        if (!UNALIGNED16(s1, s2) && !(n < sizeof (u128))) {
            const u128_a *a1 = (const u128_a *)s1;
            const u128_a *a2 = (const u128_a *)s2;
            u128        ones, highs;
            u64         hit;

            QW_SPLAT(ones, lsb);
            QW_SPLAT(highs, msb);
            while (n >= sizeof (u128)) {
                QW_NE_SUBW(hit, *a1, *a2);
                if (hit)
                    break;
                n -= sizeof (u128);
                /* Out of bytes, or a NUL in the (equal) words: equal. */
                if (n == 0)
                    return 0;
                QW_NUL(hit, *a1, ones, highs);
                if (hit)
                    return 0;
                a1++;
                a2++;
            }
            s1 = (const char *)a1;
            s2 = (const char *)a2;
        } else {
            const u64_a *a1 = (const u64_a *)s1;
            const u64_a *a2 = (const u64_a *)s2;

            while (n >= LITTLEBLOCKSIZE && *a1 == *a2) {
                n -= LITTLEBLOCKSIZE;
                if (n == 0 || DETECTNULL(*a1, lsb, msb))
                    return 0;
                a1++;
                a2++;
            }
            s1 = (const char *)a1;
            s2 = (const char *)a2;
        }
    }
    while (n-- != 0 && *s1 == *s2) {
        if (n == 0 || *s1 == '\0')
            break;
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}
#endif

// FUNC 0x0027EE50 strncpy
/* Sony EE libc strncpy: copies at most count bytes, then zero-pads so exactly count
 * bytes are written.  Returns dst0.  21 callers.
 *
 * Word copies need both pointers 8-aligned and count >= 16 (both 16-aligned) or
 * count >= 8 (movz picks the minimum).
 * QUIRK (reproduced): the 16-aligned path's first test is broken - its pnor
 * writes ~x over the loaded quadword and psubb then uses it, so the "has NUL"
 * test fires whenever any of the first 16 source bytes is in 0x00..0x7E, i.e.
 * for practically every real string, and the whole copy then goes bytewise.
 * Only when all 16 bytes are 0x7F..0xFF is one quadword copied with lq/sq; if the
 * next quadword passes the (correct) test the copy continues in the 8-byte ld/sd
 * loop, never with a second quadword.  Results are always correct; only the
 * speed is affected. */
#ifdef LIBC_RETAIL_ASM
__asm__(LIBC_ASM_BEGIN(strncpy, 3)	/* retail: one nop after strcpy */
"	daddu	$8, $4, $0\n"		/* dst0 */
"	or	$7, $5, $4\n"
"	addiu	$10, $0, 0x10\n"
"	andi	$2, $7, 0x7\n"		/* (src | dst) & 7: bytewise */
"	addiu	$9, $0, 0x8\n"
"	bnez	$2, $Lstrncpy_bytes\n"
"	andi	$2, $7, 0xF\n"
"	movz	$9, $10, $2\n"		/* minimum count: 16-aligned ? 16 : 8 */
"	bnez	$2, $Lstrncpy_dw\n"
"	sltu	$2, $6, $9\n"
"	bnez	$2, $Lstrncpy_bytes\n"
"	nop\n"
"	lui	$7, 0x101\n"		/* lsb = 0x0101010101010101 */
"	ori	$7, $7, 0x101\n"
"	dsll	$7, $7, 16\n"
"	ori	$7, $7, 0x101\n"
"	dsll	$7, $7, 16\n"
"	ori	$7, $7, 0x101\n"
"	lq	$3, 0($5)\n"
"	pcpyld	$9, $7, $7\n"		/* ones */
"	pnor	$3, $0, $3\n"		/* QUIRK: ~x replaces x for the psubb below */
"	lui	$7, 0x8080\n"		/* msb = 0x8080808080808080 */
"	ori	$7, $7, 0x8080\n"
"	dsll	$7, $7, 16\n"
"	ori	$7, $7, 0x8080\n"
"	dsll	$7, $7, 16\n"
"	ori	$7, $7, 0x8080\n"
"	psubb	$2, $3, $9\n"
"	pcpyld	$10, $7, $7\n"		/* highs */
"	pand	$2, $2, $3\n"
"	pand	$2, $2, $10\n"
"	pcpyud	$3, $2, $4\n"
"	or	$3, $2, $3\n"
"	bnez	$3, $Lstrncpy_aligned_end\n"
"	daddu	$7, $8, $0\n"
"	lq	$3, 0($5)\n"		/* one quadword */
"	addiu	$6, $6, -0x10\n"
"	addiu	$5, $5, 0x10\n"
"	sltiu	$2, $6, 0x10\n"
"	sq	$3, 0($7)\n"
"	bnez	$2, $Lstrncpy_aligned_end\n"
"	addiu	$7, $7, 0x10\n"
"	lq	$2, 0($5)\n"		/* next quadword: correct NUL test */
"	pnor	$3, $0, $2\n"
"	psubb	$2, $2, $9\n"
"	pand	$2, $2, $3\n"
"	pand	$2, $2, $10\n"
"	pcpyud	$3, $2, $4\n"
"	or	$2, $2, $3\n"
"	beql	$2, $0, $Lstrncpy_dwloop\n"
"	lq	$3, 0($5)\n"
"	b	$Lstrncpy_bytes\n"
"	daddu	$4, $7, $0\n"
"$Lstrncpy_dw:\n"			/* 8- but not 16-aligned */
"	bnez	$2, $Lstrncpy_bytes\n"	/* count < 8 */
"	nop\n"
"	ld	$3, 0($5)\n"
"	lui	$9, 0x101\n"		/* lsb */
"	ori	$9, $9, 0x101\n"
"	dsll	$9, $9, 16\n"
"	ori	$9, $9, 0x101\n"
"	dsll	$9, $9, 16\n"
"	ori	$9, $9, 0x101\n"
"	lui	$10, 0x8080\n"		/* msb */
"	ori	$10, $10, 0x8080\n"
"	dsll	$10, $10, 16\n"
"	ori	$10, $10, 0x8080\n"
"	dsll	$10, $10, 16\n"
"	ori	$10, $10, 0x8080\n"
"	dsubu	$2, $3, $9\n"
"	nor	$3, $0, $3\n"
"	and	$2, $2, $3\n"
"	and	$2, $2, $10\n"
"	bnez	$2, $Lstrncpy_aligned_end\n"
"	daddu	$7, $8, $0\n"
"	ld	$3, 0($5)\n"
"	nop\n"
"$Lstrncpy_dwloop:\n"			/* do { count -= 8; *dst++ = *src++; } while ... */
"	addiu	$6, $6, -0x8\n"
"	addiu	$5, $5, 0x8\n"
"	sltiu	$2, $6, 0x8\n"
"	sd	$3, 0($7)\n"
"	bnez	$2, $Lstrncpy_aligned_end\n"
"	addiu	$7, $7, 0x8\n"
"	ld	$2, 0($5)\n"
"	nor	$3, $0, $2\n"
"	dsubu	$2, $2, $9\n"
"	and	$2, $2, $3\n"
"	and	$2, $2, $10\n"
"	beql	$2, $0, $Lstrncpy_dwloop\n"
"	ld	$3, 0($5)\n"
"$Lstrncpy_aligned_end:\n"
"	daddu	$4, $7, $0\n"
"$Lstrncpy_bytes:\n"			/* while (count > 0) { --count; copy; stop at NUL } */
"	beqz	$6, $Lstrncpy_ret\n"
"	daddu	$2, $6, $0\n"
"	lbu	$2, 0($5)\n"
"	addiu	$6, $6, -0x1\n"
"	addiu	$5, $5, 0x1\n"
"	sb	$2, 0($4)\n"
"	sll	$2, $2, 24\n"
"	bnez	$2, $Lstrncpy_bytes\n"
"	addiu	$4, $4, 0x1\n"
"	daddu	$2, $6, $0\n"		/* while (count-- > 0) *dst++ = '\0'; */
"	beqz	$2, $Lstrncpy_ret\n"
"	addiu	$6, $6, -0x1\n"
"$Lstrncpy_zero:\n"
"	sb	$0, 0($4)\n"
"	daddu	$2, $6, $0\n"
"	addiu	$4, $4, 0x1\n"
"	.word	0x1440FFFC\n"		/* bnez $2, $Lstrncpy_zero */
"	addiu	$6, $6, -0x1\n"
"$Lstrncpy_ret:\n"
"	jr	$31\n"
"	daddu	$2, $8, $0\n"
LIBC_ASM_END(strncpy));
#else
LIBC_FN char *strncpy(char *dst0, const char *src0, size_t count)
{
    char       *dst = dst0;
    const char *src = src0;

    if (!UNALIGNED8(src, dst) && !(count < (UNALIGNED16(src, dst) ? LITTLEBLOCKSIZE : sizeof (u128)))) {
        u64_a       *aligned_dst = (u64_a *)dst;
        const u64_a *aligned_src = (const u64_a *)src;
        u64        lsb = LSB8, msb = MSB8;

        if (!UNALIGNED16(src, dst)) {
            u128 ones, highs;
            u64  nul;

            QW_SPLAT(ones, lsb);
            QW_SPLAT(highs, msb);
            QW_NUL_STRNCPY(nul, *(const u128_a *)aligned_src, ones, highs);
            if (nul)
                goto bytes;
            *(u128_a *)aligned_dst = *(const u128_a *)aligned_src;
            count -= sizeof (u128);
            aligned_src += 2;
            aligned_dst += 2;
            if (count < sizeof (u128))
                goto bytes;
            QW_NUL(nul, *(const u128_a *)aligned_src, ones, highs);
            if (nul)
                goto bytes;
        } else if (DETECTNULL(*aligned_src, lsb, msb)) {
            goto bytes;
        }
        do {
            count -= LITTLEBLOCKSIZE;
            *aligned_dst++ = *aligned_src++;
        } while (count >= LITTLEBLOCKSIZE && !DETECTNULL(*aligned_src, lsb, msb));
    bytes:
        dst = (char *)aligned_dst;
        src = (const char *)aligned_src;
    }
    while (count > 0) {
        --count;
        if ((*dst++ = *src++) == '\0')
            break;
    }
    while (count-- > 0)
        *dst++ = '\0';
    return dst0;
}
#endif

// FUNC 0x0027F008 strstr
/* newlib strstr (naive).  "" in "" is found; one caller, race_league. */
char *strstr(const char *searchee, const char *lookfor)
{
    if (*searchee == 0) {
        if (*lookfor)
            return (char *)NULL;
        return (char *)searchee;
    }

    while (*searchee) {
        size_t i;

        i = 0;
        while (1) {
            if (lookfor[i] == 0)
                return (char *)searchee;
            if (lookfor[i] != searchee[i])
                break;
            i++;
        }
        searchee++;
    }
    return (char *)NULL;
}

// FUNC 0x0027F2E8 strupr
/* newlib strupr: islower(c) ? c - 'a' + 'A' in place; returns a.  One caller,
 * IOPLoadModuleBase::makePath.  toupper is newlib's GNU statement expression
 * (TOUPPER, newlib_part1_internal.h): its own islower test folds into the first one, and the
 * char is read once per iteration (lbu, then sign-extended). */
char *strupr(char *a)
{
    char *ret = a;

    while (*a != '\0') {
        if (ISLOWER(*a))
            *a = TOUPPER(*a);
        ++a;
    }
    return ret;
}
