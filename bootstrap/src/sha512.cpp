/* sha512.cpp - SHA-512 (FIPS 180-2) as linked into the GT3 US bootstrap.
 *
 * Range 0x010010F0..0x01003570, one C++ translation unit (own .eh_frame CIE at
 * 0x01027170).  Used by sha512_digest_to_bignum (0x010043A8) to hash the
 * inflated core.gt3 payload for the RSA signature check in
 * ImageLoader::verify (0x010044A8).
 *
 * Behaviour check: the ORIGINAL machine code of every function in this file
 * was run in an R5900 subset interpreter and compared with a reference
 * SHA-512 (lengths 0..259, 300, 511..513, 1000, 1024, 3000 bytes, random
 * update() splits, plus the compare / BE-store / BE-load / IV helpers): it
 * is bit-exact standard SHA-512.  tools/difftest.py (group sha512) compares
 * this file's code with the original call by call (load_be64 / transform /
 * memcpy / memset, with their arguments) on the SCE and GCC 14 builds.  The
 * GCC 14 behaviour build (tools/build.py: -ffreestanding -fno-builtin) makes
 * every memcpy/memset call of the original with the same arguments; only a
 * compile with builtins (tools/cc.sh) drops sha512_transform's wipe and trims
 * sha512_pad's second memset as dead stores.  The only non-standard aspects are API
 * details: final() hands back the eight state words in native u64 order (the
 * caller serialises them with sha512_store_digest_be), the length counter is
 * 64-bit (bits), and the context is re-initialised by final().
 *
 * Implementation style (reconstructed so that SCE ee-gcc 2.9 reproduces the
 * binary): working variables a..h live in a local array S[8] that is never
 * rotated - the round macro renames them instead (Crypto++-style
 * a(i) = S[(0 - i) & 7] ... h(i) = S[(7 - i) & 7]) and updates in place:
 *     h += Sigma1(e) + Ch(e,f,g) + K[i] + W[i];  d += h;
 *     h += Sigma0(a) + Maj(a,b,c);
 * Rounds 0..15 are straight-line, each loading its message word with
 * sha512_load_be64; rounds 16..79 are a do-while over j = 16, 32, 48, 64 with
 * 16 rounds per pass on a 16-word circular schedule W[16].  Ch/Maj are the
 * reduced forms z ^ (x & (y ^ z)) and (x & y) | (z & (x | y)).  After the
 * feedback add both locals are wiped with memset.
 */
#include "types.h"
#include "libc.h"
#include "sha512.h"

static_assert(offsetof(SHA512Ctx, total_bits) == 0x00, "SHA512Ctx.total_bits");
static_assert(offsetof(SHA512Ctx, state) == 0x08, "SHA512Ctx.state");
static_assert(offsetof(SHA512Ctx, buf_pos) == 0x48, "SHA512Ctx.buf_pos");
static_assert(offsetof(SHA512Ctx, block) == 0x4C, "SHA512Ctx.block");
static_assert(sizeof(SHA512Ctx) == 0xD0, "SHA512Ctx size (0xCC used + pad)");

/* @0x0103DF18 (.rodata, 80 x u64 = 0x280 bytes, ends 0x0103E198 where the
 * filename unit's "cdrom0:\" string starts).  Extracted from the ELF with
 * tools/sha512_ktable.py; identical to the FIPS 180-2 SHA-512 constants. */
const u64 sha512_K[80] = {
    0x428A2F98D728AE22ULL, 0x7137449123EF65CDULL,
    0xB5C0FBCFEC4D3B2FULL, 0xE9B5DBA58189DBBCULL,
    0x3956C25BF348B538ULL, 0x59F111F1B605D019ULL,
    0x923F82A4AF194F9BULL, 0xAB1C5ED5DA6D8118ULL,
    0xD807AA98A3030242ULL, 0x12835B0145706FBEULL,
    0x243185BE4EE4B28CULL, 0x550C7DC3D5FFB4E2ULL,
    0x72BE5D74F27B896FULL, 0x80DEB1FE3B1696B1ULL,
    0x9BDC06A725C71235ULL, 0xC19BF174CF692694ULL,
    0xE49B69C19EF14AD2ULL, 0xEFBE4786384F25E3ULL,
    0x0FC19DC68B8CD5B5ULL, 0x240CA1CC77AC9C65ULL,
    0x2DE92C6F592B0275ULL, 0x4A7484AA6EA6E483ULL,
    0x5CB0A9DCBD41FBD4ULL, 0x76F988DA831153B5ULL,
    0x983E5152EE66DFABULL, 0xA831C66D2DB43210ULL,
    0xB00327C898FB213FULL, 0xBF597FC7BEEF0EE4ULL,
    0xC6E00BF33DA88FC2ULL, 0xD5A79147930AA725ULL,
    0x06CA6351E003826FULL, 0x142929670A0E6E70ULL,
    0x27B70A8546D22FFCULL, 0x2E1B21385C26C926ULL,
    0x4D2C6DFC5AC42AEDULL, 0x53380D139D95B3DFULL,
    0x650A73548BAF63DEULL, 0x766A0ABB3C77B2A8ULL,
    0x81C2C92E47EDAEE6ULL, 0x92722C851482353BULL,
    0xA2BFE8A14CF10364ULL, 0xA81A664BBC423001ULL,
    0xC24B8B70D0F89791ULL, 0xC76C51A30654BE30ULL,
    0xD192E819D6EF5218ULL, 0xD69906245565A910ULL,
    0xF40E35855771202AULL, 0x106AA07032BBD1B8ULL,
    0x19A4C116B8D2D0C8ULL, 0x1E376C085141AB53ULL,
    0x2748774CDF8EEB99ULL, 0x34B0BCB5E19B48A8ULL,
    0x391C0CB3C5C95A63ULL, 0x4ED8AA4AE3418ACBULL,
    0x5B9CCA4F7763E373ULL, 0x682E6FF3D6B2B8A3ULL,
    0x748F82EE5DEFB2FCULL, 0x78A5636F43172F60ULL,
    0x84C87814A1F0AB72ULL, 0x8CC702081A6439ECULL,
    0x90BEFFFA23631E28ULL, 0xA4506CEBDE82BDE9ULL,
    0xBEF9A3F7B2C67915ULL, 0xC67178F2E372532BULL,
    0xCA273ECEEA26619CULL, 0xD186B8C721C0C207ULL,
    0xEADA7DD6CDE0EB1EULL, 0xF57D4F7FEE6ED178ULL,
    0x06F067AA72176FBAULL, 0x0A637DC5A2C898A6ULL,
    0x113F9804BEF90DAEULL, 0x1B710B35131C471BULL,
    0x28DB77F523047D84ULL, 0x32CAAB7B40C72493ULL,
    0x3C9EBE0A15C9BEBCULL, 0x431D67C49C100D4CULL,
    0x4CC5D4BECB3E42B6ULL, 0x597F299CFC657E2AULL,
    0x5FCB6FAB3AD6FAECULL, 0x6C44198C4A475817ULL,
};

/* ------------------------------------------------------------------------ */
/* Round primitives.                                                         */
/*                                                                          */
/* Codegen notes (SCE ee-gcc 2.9, see the report / MATCHING.md):            */
/*  - The rotate is an inline FUNCTION.  As a macro, fold() turns           */
/*    (x >> n) | (x << (64 - n)) into an LROTATE_EXPR that expands as       */
/*    (x << (64 - n)) | (x >> n); the binary has the right shift first.     */
/*    Being calls, the inline rotates of Sigma1(e) are also pre-expanded    */
/*    ahead of the load_be64 call in rounds 0..15 (their XORs are not).     */
/*  - Each round is wrapped in do { } while (0): its loop notes end CSE's   */
/*    extended basic blocks, so S[]/W[] values stored by one round are      */
/*    re-loaded by the next.  GCSE turns those loads into the loop-carried  */
/*    registers and the spilled W[]/state[] copies of the binary; without   */
/*    the wrappers the code and frame (0x240) come out entirely different. */
/* ------------------------------------------------------------------------ */
static inline u64 rotr64(u64 x, unsigned int y)
{
    return (x >> y) | (x << (64 - y));      /* y is always 1..61 here */
}

#define SHA512_CH(x, y, z)   ((z) ^ ((x) & ((y) ^ (z))))
#define SHA512_MAJ(x, y, z)  (((x) & (y)) | ((z) & ((x) | (y))))
#define SHA512_S0(x)  (rotr64(x, 28) ^ rotr64(x, 34) ^ rotr64(x, 39))   /* Sigma0 */
#define SHA512_S1(x)  (rotr64(x, 14) ^ rotr64(x, 18) ^ rotr64(x, 41))   /* Sigma1 */
#define SHA512_s0(x)  (rotr64(x, 1) ^ rotr64(x, 8) ^ ((x) >> 7))        /* sigma0 */
#define SHA512_s1(x)  (rotr64(x, 19) ^ rotr64(x, 61) ^ ((x) >> 6))      /* sigma1 */

/* Message schedule: rounds 0..15 load W[i] big-endian from the block,
 * rounds 16..79 update the 16-word circular buffer in place. */
#define SHA512_BLK0(i) (W[i] = sha512_load_be64(block + 8 * (i)))
#define SHA512_BLK2(i) (W[(i) & 15] += SHA512_s1(W[((i) - 2) & 15]) + W[((i) - 7) & 15] + \
                        SHA512_s0(W[((i) - 15) & 15]))

/* Working variables a..h of round i (renamed every round, never moved). */
#define SHA512_A(i) S[(0 - (i)) & 7]
#define SHA512_B(i) S[(1 - (i)) & 7]
#define SHA512_C(i) S[(2 - (i)) & 7]
#define SHA512_D(i) S[(3 - (i)) & 7]
#define SHA512_E(i) S[(4 - (i)) & 7]
#define SHA512_F(i) S[(5 - (i)) & 7]
#define SHA512_G(i) S[(6 - (i)) & 7]
#define SHA512_H(i) S[(7 - (i)) & 7]

/* Rounds 0..15 (constant K[i]). */
#define SHA512_R0(i)                                                                \
    do {                                                                            \
        SHA512_H(i) += SHA512_S1(SHA512_E(i)) +                                     \
                       SHA512_CH(SHA512_E(i), SHA512_F(i), SHA512_G(i)) +           \
                       sha512_K[i] + SHA512_BLK0(i);                                \
        SHA512_D(i) += SHA512_H(i);                                                 \
        SHA512_H(i) += SHA512_S0(SHA512_A(i)) +                                     \
                       SHA512_MAJ(SHA512_A(i), SHA512_B(i), SHA512_C(i));           \
    } while (0)

/* Rounds 16..79: round i of the pass starting at round j (j = 16, 32, 48, 64). */
#define SHA512_R(i)                                                                 \
    do {                                                                            \
        SHA512_H(i) += SHA512_S1(SHA512_E(i)) +                                     \
                       SHA512_CH(SHA512_E(i), SHA512_F(i), SHA512_G(i)) +           \
                       sha512_K[(i) + j] + SHA512_BLK2(i);                          \
        SHA512_D(i) += SHA512_H(i);                                                 \
        SHA512_H(i) += SHA512_S0(SHA512_A(i)) +                                     \
                       SHA512_MAJ(SHA512_A(i), SHA512_B(i), SHA512_C(i));           \
    } while (0)

// FUNC 0x010010F0 sha512_digest_compare
/* Word-wise unsigned compare of two 8 x u64 digests (word 0 first):
 * returns -1 at the first a[i] < b[i], 1 at the first a[i] > b[i], else 0.
 * DEAD CODE: no static caller and no pointer to it except its own FDE
 * (0x0102718C); ImageLoader::verify compares bignums (0x01007408) instead. */
int sha512_digest_compare(const u64 *a, const u64 *b)
{
    u32 i = 0;

    do {
        if (a[i] < b[i])        /* sltu */
            return -1;
        if (b[i] < a[i])        /* sltu */
            return 1;
        i++;
    } while (i < 8);            /* sltiu */
    return 0;
}

// FUNC 0x01001140 sha512_store_digest_be
/* Serialise state[0..7] to 64 big-endian bytes.  Caller: 0x010043A8
 * (digest -> bytes before the bignum import). */
void sha512_store_digest_be(u8 *out, const u64 *state)
{
    u32 i = 0;

    do {
        u64 w = *state++;
        out[6] = (u8)(w >> 8);
        out[7] = (u8)w;
        out[5] = (u8)(w >> 16);
        out[4] = (u8)(w >> 24);
        out[3] = (u8)(w >> 32);
        out[2] = (u8)(w >> 40);
        out[1] = (u8)(w >> 48);
        out[0] = (u8)(w >> 56);
        i++;
        out += 8;
    } while (i < 8);            /* sltiu */
}

// FUNC 0x010011D0 sha512_init_state
/* Load the SHA-512 initial hash value H(0) (built from lui/ori/dsll
 * immediates, not from .rodata).  Callers: sha512_reset, and 0x010043A8
 * directly on its stack context's state before calling sha512_reset. */
void sha512_init_state(u64 *state)
{
    state[0] = 0x6A09E667F3BCC908ULL;
    state[1] = 0xBB67AE8584CAA73BULL;
    state[2] = 0x3C6EF372FE94F82BULL;
    state[3] = 0xA54FF53A5F1D36F1ULL;
    state[4] = 0x510E527FADE682D1ULL;
    state[5] = 0x9B05688C2B3E6C1FULL;
    state[6] = 0x1F83D9ABFB41BD6BULL;
    state[7] = 0x5BE0CD19137E2179ULL;
}

// FUNC 0x010012B8 sha512_load_be64
/* Big-endian 64-bit load from an arbitrarily aligned pointer (byte loads,
 * two 32-bit halves merged with dsll32/dsrl32).  Only caller:
 * sha512_transform (16 calls per block). */
u64 sha512_load_be64(const u8 *p)
{
    /* int arithmetic (no u32 casts): egcs then pairs the ORs as
     * (b0<<24 | b3) | (b2<<8 | b1<<16), as in the binary.  b0 << 24 with
     * b0 <= 0xFF is well defined (C++14; GCC documents it for every mode)
     * and converts to the same u32 value as the unsigned form. */
    u32 hi = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    u32 lo = (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7];

    return ((u64)hi << 32) | lo;
}

// FUNC 0x01001320 sha512_transform
/* SHA-512 compression of one 128-byte block (any alignment) into state[0..7].
 * Callers: sha512_update, sha512_pad, sha512_final.
 * Frame 0x240: W[16] at sp+0x00, S[8] at sp+0x80; the rest are register
 * spills (state pointer 0xC0, j 0xC4, the strength-reduced &K[j] cursor
 * 0x190, GCSE's hoisted copies of W[] / state[] / &S in between).
 * Rounds 0..15: 0x01001394..0x01002018 (16 calls to sha512_load_be64).
 * Rounds 16..79: loop body 0x01002100..0x010031C4, 16 rounds per pass.
 * Epilogue: state[i] += S[i]; memset(S, 0, 0x40); memset(W, 0, 0x80). */
void sha512_transform(u64 *state, const u8 *block)
{
    u64 W[16];
    u64 S[8];
    unsigned int j;

    S[0] = state[0];
    S[1] = state[1];
    S[2] = state[2];
    S[3] = state[3];
    S[4] = state[4];
    S[5] = state[5];
    S[6] = state[6];
    S[7] = state[7];

    SHA512_R0( 0); SHA512_R0( 1); SHA512_R0( 2); SHA512_R0( 3);
    SHA512_R0( 4); SHA512_R0( 5); SHA512_R0( 6); SHA512_R0( 7);
    SHA512_R0( 8); SHA512_R0( 9); SHA512_R0(10); SHA512_R0(11);
    SHA512_R0(12); SHA512_R0(13); SHA512_R0(14); SHA512_R0(15);

    j = 16;
    do {
        SHA512_R( 0); SHA512_R( 1); SHA512_R( 2); SHA512_R( 3);
        SHA512_R( 4); SHA512_R( 5); SHA512_R( 6); SHA512_R( 7);
        SHA512_R( 8); SHA512_R( 9); SHA512_R(10); SHA512_R(11);
        SHA512_R(12); SHA512_R(13); SHA512_R(14); SHA512_R(15);
        j += 16;
    } while (j < 80);           /* sltiu */

    state[0] += S[0];
    state[1] += S[1];
    state[2] += S[2];
    state[3] += S[3];
    state[4] += S[4];
    state[5] += S[5];
    state[6] += S[6];
    state[7] += S[7];

    /* Wipe the working variables and the message schedule.  The original
     * really makes both calls (0x01003234, 0x01003244); so does the GCC 14
     * behaviour build (-fno-builtin).  Only a compile with builtin memset
     * (tools/cc.sh) deletes them as dead stores to locals. */
    memset(S, 0, sizeof(S));    /* 0x40 */
    memset(W, 0, sizeof(W));    /* 0x80 */
}

// FUNC 0x01003280 sha512_reset
/* Initialise / re-initialise a context: bit count 0, H(0), empty buffer.
 * Callers: sha512_final (after the digest is copied out), 0x010043A8. */
void sha512_reset(SHA512Ctx *ctx)
{
    ctx->total_bits = 0;
    sha512_init_state(ctx->state);
    ctx->buf_pos = ctx->block;
}

// FUNC 0x010032B8 sha512_update
/* Absorb len bytes: add len*8 to the 64-bit bit counter ((u64)(u32)len << 3,
 * dsll32/dsrl 29), top up a pending partial block first (if the data still
 * does not fill it, just append and return), then compress whole 128-byte
 * blocks straight from the caller's buffer, and buffer the remainder.
 * Pointer / length compares are unsigned (sltu/sltiu).
 * Caller: 0x010043A8 (one call over the whole core.gt3 payload). */
void sha512_update(SHA512Ctx *ctx, const u8 *data, u32 len)
{
    u32 space;

    ctx->total_bits += (u64)len << 3;

    if (ctx->block < ctx->buf_pos) {                        /* partial block pending */
        space = 0x80 - (u32)(ctx->buf_pos - ctx->block);
        if (len < space) {
            memcpy(ctx->buf_pos, data, len);
            ctx->buf_pos = ctx->buf_pos + len;
            return;
        }
        memcpy(ctx->buf_pos, data, space);
        len  -= space;
        data += space;
        sha512_transform(ctx->state, ctx->block);
        ctx->buf_pos = ctx->block;
    }

    while (len >= 0x80) {
        sha512_transform(ctx->state, data);
        len  -= 0x80;
        data += 0x80;
    }

    if (len != 0) {
        memcpy(ctx->block, data, len);
        ctx->buf_pos = ctx->block + len;
    }
}

// FUNC 0x010033C8 sha512_pad
/* Message padding: append 0x80, zero the rest of the block; if fewer than 16
 * bytes were left after the 0x80 byte (no room for the 128-bit length
 * field), compress this block and start a fresh all-zero one.  Finally
 * store the 64-bit bit length big-endian in the last 8 bytes (the upper 64
 * bits of the 128-bit length field stay zero).  The final block is NOT
 * compressed here.  Only caller: sha512_final. */
void sha512_pad(SHA512Ctx *ctx)
{
    u32 left;

    /* Stored through a signed-char lvalue: the binary materialises the byte
     * as -0x80 (li $v0,-128 / sb); the stored byte is 0x80 either way. */
    *(s8 *)ctx->buf_pos++ = (s8)0x80;
    left = 0x80 - (u32)(ctx->buf_pos - ctx->block);
    memset(ctx->buf_pos, 0, left);

    if (left >= 16) {                                       /* sltiu / bnez */
        sha512_store_bitlen(ctx);
    } else {
        sha512_transform(ctx->state, ctx->block);
        ctx->buf_pos = ctx->block;
        memset(ctx->block, 0, 0x80);    /* full 0x80, as in the original and the
                                           -fno-builtin GCC 14 build (builtin memset
                                           would trim it to 0x78 after inlining
                                           store_bitlen) */
        sha512_store_bitlen(ctx);
    }
}

// FUNC 0x01003468 sha512_store_bitlen
/* block[0x78..0x7F] (ctx+0xC4..0xCB) = total_bits big-endian.  The code
 * addresses them as 0x7FF8..0x7FFF off ctx-0x7F34 and re-reads the counter
 * with lbu/ld/lwu/lhu for each byte; stores go from the LSB (0x7F) down. */
void sha512_store_bitlen(SHA512Ctx *ctx)
{
    u8 *p = ctx->block + 0x80;  /* end of the block: stores go through a
                                   second pointer, so egcs cannot tell them
                                   from total_bits and re-reads it */

    p[-1] = (u8)(ctx->total_bits);
    p[-2] = (u8)(ctx->total_bits >> 8);
    p[-3] = (u8)(ctx->total_bits >> 16);
    p[-4] = (u8)(ctx->total_bits >> 24);
    p[-5] = (u8)(ctx->total_bits >> 32);
    p[-6] = (u8)(ctx->total_bits >> 40);
    p[-7] = (u8)(ctx->total_bits >> 48);
    p[-8] = (u8)(ctx->total_bits >> 56);
}

// FUNC 0x010034E0 sha512_final
/* Finish: pad, compress the last block, copy the 8 state words to out[]
 * (sd x8 - out must be 8-byte aligned; native word order, the caller
 * serialises with sha512_store_digest_be), then re-initialise the context
 * with sha512_reset.  Only caller: 0x010043A8 (out = its sp+0). */
void sha512_final(SHA512Ctx *ctx, u64 *out)
{
    u64 *st = ctx->state;

    sha512_pad(ctx);
    sha512_transform(st, ctx->block);
    out[0] = st[0];
    out[1] = st[1];
    out[2] = st[2];
    out[3] = st[3];
    out[4] = st[4];
    out[5] = st[5];
    out[6] = st[6];
    out[7] = st[7];
    sha512_reset(ctx);
}
