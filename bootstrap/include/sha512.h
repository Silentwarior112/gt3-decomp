/* sha512.h - SHA-512 module of the GT3 US bootstrap (BOOTSTRAP_SCUS-97102).
 *
 * Code range 0x010010F0..0x01003570 (one translation unit: its own .eh_frame
 * CIE at 0x01027170, then one FDE per function from 0x01027184; those FDE
 * pc_begin words are the "data xrefs" the listing shows for every function).
 * Round constants K[0..79] at .rodata 0x0103DF18..0x0103E198 (identical to
 * FIPS 180-2).  Verified by emulating the original machine code against a
 * reference SHA-512 (all lengths 0..259 plus 300..3000 bytes, random update
 * splits): the unit computes standard SHA-512 with no deviations.
 *
 * Only consumer: sha512_digest_to_bignum (0x010043A8, imageloader unit), which
 * does, on a stack SHA512Ctx:
 *     sha512_init_state(ctx.state);   // (looks like an inlined ctor of the
 *     sha512_reset(&ctx);             //  state sub-object + ctx ctor)
 *     sha512_update(&ctx, data, len);
 *     sha512_final(&ctx, digest);     // digest = u64[8] in native word order
 *     sha512_store_digest_be(bytes, digest);   // -> 64 big-endian bytes
 * and then imports the 64 bytes as a bignum for the RSA compare.
 *
 * Plain C-style functions with C++ linkage (the TU was compiled as C++; no
 * mangled names are known, the names are descriptive).
 */
#ifndef BOOT_SHA512_H
#define BOOT_SHA512_H

#include "types.h"

#define SHA512_BLOCK_SIZE   0x80    /* 128-byte message block */
#define SHA512_DIGEST_WORDS 8       /* 8 x u64 = 64-byte digest */

/* Streaming context.  Used extent 0xCC bytes; sizeof is 0xD0 because of the
 * u64 members (sha512_digest_to_bignum reserves 0xD0 bytes of stack for it:
 * ctx at sp+0x40, next object at sp+0x110). */
typedef struct SHA512Ctx {
    u64 total_bits;           /* +0x00 message length in BITS (64-bit counter only) */
    u64 state[8];             /* +0x08 chaining value H0..H7 */
    u8 *buf_pos;              /* +0x48 write cursor into block[] (== block when empty) */
    u8  block[0x80];          /* +0x4C pending partial block / padding buffer */
} SHA512Ctx;                  /* +0xCC (+4 bytes tail padding) */

/* 0x0103DF18 (.rodata): SHA-512 round constants K[0..79]. */
extern const u64 sha512_K[80];

/* 0x010010F0: compare two 8-word digests word by word (word 0 most
 * significant, unsigned): -1 if a < b, 1 if a > b, 0 if equal.
 * DEAD CODE - no caller anywhere in the binary (only its FDE references it). */
int  sha512_digest_compare(const u64 *a, const u64 *b);

/* 0x01001140: serialise 8 u64 words as 64 big-endian bytes (out[0] = MSB of
 * state[0]). */
void sha512_store_digest_be(u8 *out, const u64 *state);

/* 0x010011D0: state[0..7] = SHA-512 initial hash value H(0). */
void sha512_init_state(u64 *state);

/* 0x010012B8: load 8 bytes (any alignment) as a big-endian u64. */
u64  sha512_load_be64(const u8 *p);

/* 0x01001320: SHA-512 compression function, one 128-byte block (any
 * alignment) into state[0..7]. */
void sha512_transform(u64 *state, const u8 *block);

/* 0x01003280: (re)initialise a context: bit count 0, H(0), empty buffer. */
void sha512_reset(SHA512Ctx *ctx);

/* 0x010032B8: absorb len bytes. */
void sha512_update(SHA512Ctx *ctx, const u8 *data, u32 len);

/* 0x010033C8: append 0x80 and zero padding (compressing one extra block if
 * fewer than 16 bytes remain), then the big-endian bit length.  Does NOT
 * compress the final block - sha512_final does. */
void sha512_pad(SHA512Ctx *ctx);

/* 0x01003468: block[0x78..0x7F] = total_bits, big-endian. */
void sha512_store_bitlen(SHA512Ctx *ctx);

/* 0x010034E0: pad, compress the last block, copy the 8 state words (native
 * u64 order, NOT bytes) to out[0..7], then sha512_reset(ctx). */
void sha512_final(SHA512Ctx *ctx, u64 *out);

#endif /* BOOT_SHA512_H */
