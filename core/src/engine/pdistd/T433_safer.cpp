/* T433 - SAFER SK (? name), 0x0021C220..0x0021CF48: J. L. Massey's SAFER
 * block cipher as in R. De Moliner's reference safer.c (8-byte blocks, at
 * most 13 rounds, the strengthened key schedule), with the exp / log tables
 * passed in instead of being globals, and two functions that run it over a
 * buffer one byte at a time.  T431 uses it for the ranking codes
 * (RankingHash::compress, RankingCodeBlock::RankingCodeBlock).
 *
 * Pass 2: safer_tables_init, the key schedule and both block functions were
 * checked against the listing instruction by instruction (key byte order,
 * which table each byte goes through, the PHT / IPHT pairs and the
 * permutations, the output store order). */
#include "game/data/gcf_internal.h"
#include "engine/pdistd/T433_safer.h"

#define EXP(x)      (t->exp[(u8)(x)])
#define LOG(x)      (t->log[(u8)(x)])
#define PHT(x, y)   { y += x; x += y; }
#define IPHT(x, y)  { x -= y; y -= x; }

#define SAFER_BLOCK_LEN       8
#define SAFER_MAX_NOF_ROUNDS  13

/* x rotated left by n bits (an inline function: retail shifts the promoted
 * byte right arithmetically, sra, which egcs keeps only through the
 * parameter copy). */
static inline u8 rol8(u8 x, int n)
{
    return (x << n) | (x >> (8 - n));
}

/* Caller: RankingCodeBlock::RankingCodeBlock (T431).  exp[i] = 45^i mod 257
 * (256 stored as 0) and its inverse log.  Returns t (? a constructor). */
// FUNC 0x0021C220 safer_tables_init
SaferTables *safer_tables_init(SaferTables *t)
{
    u32 power;
    u32 i;

    memset(t->exp, 0, 0x100);
    power = 1;
    memset(t->log, 0, 0x100);
    for (i = 0; i < 0x100; i++) {
        u8 v = power;

        t->exp[i] = v;
        t->log[v] = i;
        power = power * 45 % 257;
    }
    return t;
}

/* No callers.  The key schedule of an all-zero 128-bit user key. */
// FUNC 0x0021C2C8 safer_key_init_zero
SaferKey *safer_key_init_zero(SaferKey *key, const SaferTables *t, u32 rounds)
{
    u8 zero[SAFER_BLOCK_LEN];

    memset(zero, 0, SAFER_BLOCK_LEN);
    safer_expand_key(key, t, zero, zero, rounds);
    return key;
}

/* No callers.  The key schedule of k1 with a zero second half (note: not
 * SAFER SK-64's k1, k1). */
// FUNC 0x0021C330 safer_key_init64
SaferKey *safer_key_init64(SaferKey *key, const SaferTables *t, const u8 *k1,
                           u32 rounds)
{
    u8 zero[SAFER_BLOCK_LEN];

    memset(zero, 0, SAFER_BLOCK_LEN);
    safer_expand_key(key, t, k1, zero, rounds);
    return key;
}

/* Callers: RankingHash::compress, RankingCodeBlock::RankingCodeBlock
 * (T431).  The key schedule of k1:k2. */
// FUNC 0x0021C3A0 safer_key_init128
SaferKey *safer_key_init128(SaferKey *key, const SaferTables *t, const u8 *k1,
                            const u8 *k2, u32 rounds)
{
    safer_expand_key(key, t, k1, k2, rounds);
    return key;
}

/* No callers.  *dst = *src (0xC0 bytes in 8-byte pieces, unaligned-safe,
 * then 0x19), returns dst (? the copy constructor). */
// FUNC 0x0021C3C8 F_0021C3C8
SaferKey *F_0021C3C8(SaferKey *dst, const SaferKey *src)
{
    *dst = *src;
    return dst;
}

/* Caller: RankingHash::compress (T431, its `k1 = k2').  The same copy
 * (? operator=). */
// FUNC 0x0021C4B0 F_0021C4B0
SaferKey *F_0021C4B0(SaferKey *dst, const SaferKey *src)
{
    *dst = *src;
    return dst;
}

/* Callers: the three key inits.  Safer_Expand_Userkey(k1, k2, rounds,
 * strengthened = 1, key); the work buffers are cleared with memset. */
// FUNC 0x0021C598 safer_expand_key
/* 82.9%: left is the third loop: retail computes 18 * i in it again (a
 * mult per byte, the constants 18 and 9 hoisted); here GCSE reuses the
 * second loop's product and loop.c strength-reduces 18 * i + j + 10 (and
 * the registers that follow).  Tried: int / u32 i, j and index, i * 18,
 * the + 10 regrouped; the reference compiler gives the same. */
void safer_expand_key(SaferKey *key, const SaferTables *t, const u8 *k1,
                      const u8 *k2, u32 rounds)
{
    u8 ka[SAFER_BLOCK_LEN + 1];             /* sp+0x00 */
    u8 kb[SAFER_BLOCK_LEN + 1];             /* sp+0x10 */
    int n = 0;                              /* the next key byte */
    u32 i, j;

    memset(key, 0, sizeof(SaferKey));
    memset(ka, 0, SAFER_BLOCK_LEN + 1);
    memset(kb, 0, SAFER_BLOCK_LEN + 1);
    if (rounds >= SAFER_MAX_NOF_ROUNDS + 1)
        rounds = SAFER_MAX_NOF_ROUNDS;
    key->k[n++] = rounds;
    for (j = 0; j < SAFER_BLOCK_LEN; j++) {
        ka[SAFER_BLOCK_LEN] ^= ka[j] = rol8(k1[j], 5);
        kb[SAFER_BLOCK_LEN] ^= kb[j] = key->k[n++] = k2[j];
    }
    for (i = 1; i <= rounds; i++) {
        for (j = 0; j < SAFER_BLOCK_LEN + 1; j++) {
            ka[j] = rol8(ka[j], 6);
            kb[j] = rol8(kb[j], 6);
        }
        for (j = 0; j < SAFER_BLOCK_LEN; j++)
            key->k[n++] = ka[(j + 2 * i - 1) % (SAFER_BLOCK_LEN + 1)] +
                          EXP(EXP(18 * i + j + 1));
        for (j = 0; j < SAFER_BLOCK_LEN; j++)
            key->k[n++] = kb[(j + 2 * i) % (SAFER_BLOCK_LEN + 1)] +
                          EXP(EXP(18 * i + j + 10));
    }
    memset(ka, 0, SAFER_BLOCK_LEN + 1);
    memset(kb, 0, SAFER_BLOCK_LEN + 1);
}

/* Callers: RankingHash::compress (T431), safer_encrypt_buffer.
 * Safer_Encrypt_Block; out may be in. */
/* The subkey bytes by index from the key start (retail: key + i with the
 * offsets of one round, i += 16 per round), each round's key byte folded
 * into the exp / log index expression. */
#define K(n) (key->k[i + (n)])
// FUNC 0x0021C820 safer_encrypt_block
/* 36.0% (was 21.2% with the reference's `*++k'): the shape is retail's
 * (the key by index, i from 1 by 16; out, rounds and round spilled to
 * sp+0 / +4 / +8; a ^ k, b + k, ... as temporaries of each exp / log
 * index), the schedule and registers of the 120-insn round are not.
 * Tried (build/scratch/u099/v_enc*.py): `a ^= k' and `a = EXP(a) + k' as
 * separate statements (37.6%), the key through a local pointer, i int /
 * u32, round int / u32, the last key bytes folded into the output stores;
 * the reference compiler gives the same. */
void safer_encrypt_block(u8 *out, const u8 *in, const SaferTables *t,
                         const SaferKey *key)
{
    u8 a, b, c, d, e, f, g, h, tmp;
    u32 rounds;
    u32 round;
    int i = 1;

    rounds = key->k[0];
    if (rounds >= SAFER_MAX_NOF_ROUNDS + 1)
        rounds = SAFER_MAX_NOF_ROUNDS;
    a = in[0]; b = in[1]; c = in[2]; d = in[3];
    e = in[4]; f = in[5]; g = in[6]; h = in[7];
    for (round = 0; round < rounds; round++) {
        a = EXP(a ^ K(0)) + K(8);
        b = LOG(b + K(1)) ^ K(9);
        c = LOG(c + K(2)) ^ K(10);
        d = EXP(d ^ K(3)) + K(11);
        e = EXP(e ^ K(4)) + K(12);
        f = LOG(f + K(5)) ^ K(13);
        g = LOG(g + K(6)) ^ K(14);
        h = EXP(h ^ K(7)) + K(15);
        PHT(a, b); PHT(c, d); PHT(e, f); PHT(g, h);
        PHT(a, c); PHT(e, g); PHT(b, d); PHT(f, h);
        PHT(a, e); PHT(b, f); PHT(c, g); PHT(d, h);
        tmp = b; b = e; e = c; c = tmp;
        tmp = d; d = f; f = g; g = tmp;
        i += 16;
    }
    a ^= K(0); b += K(1); c += K(2); d ^= K(3);
    e ^= K(4); f += K(5); g += K(6); h ^= K(7);
    out[7] = h; out[3] = d; out[0] = a; out[1] = b;
    out[2] = c; out[4] = e; out[5] = f; out[6] = g;
}

#undef K

/* Caller: safer_decrypt_buffer.  Safer_Decrypt_Block; out may be in. */
#define K(n) (key->k[i - (n)])
// FUNC 0x0021CB38 safer_decrypt_block
/* 41.5% (was 16.7%): as safer_encrypt_block, the shape is retail's (i from
 * 8 * (1 + 2 * rounds) down by 16, the spills), the round's schedule and
 * registers are not.  Tried (build/scratch/u099/v_dec2.py): the key
 * operations as separate statements, the input bytes read in h..a or a..h
 * order with or without the first key bytes, the output stores in
 * retail's or in index order, i int / u32; the reference compiler gives
 * the same. */
void safer_decrypt_block(u8 *out, const u8 *in, const SaferTables *t,
                         const SaferKey *key)
{
    u8 a, b, c, d, e, f, g, h, tmp;
    u32 rounds;
    u32 round;
    int i;

    rounds = key->k[0];
    if (rounds >= SAFER_MAX_NOF_ROUNDS + 1)
        rounds = SAFER_MAX_NOF_ROUNDS;
    i = SAFER_BLOCK_LEN * (1 + 2 * rounds);
    a = in[0] ^ K(7); b = in[1] - K(6); c = in[2] - K(5); d = in[3] ^ K(4);
    e = in[4] ^ K(3); f = in[5] - K(2); g = in[6] - K(1); h = in[7] ^ K(0);
    i -= 8;
    for (round = 0; round < rounds; round++) {
        tmp = e; e = b; b = c; c = tmp;
        tmp = f; f = d; d = g; g = tmp;
        IPHT(a, e); IPHT(b, f); IPHT(c, g); IPHT(d, h);
        IPHT(a, c); IPHT(e, g); IPHT(b, d); IPHT(f, h);
        IPHT(a, b); IPHT(c, d); IPHT(e, f); IPHT(g, h);
        h = LOG(h - K(0)) ^ K(8);
        g = EXP(g ^ K(1)) - K(9);
        f = EXP(f ^ K(2)) - K(10);
        e = LOG(e - K(3)) ^ K(11);
        d = LOG(d - K(4)) ^ K(12);
        c = EXP(c ^ K(5)) - K(13);
        b = EXP(b ^ K(6)) - K(14);
        a = LOG(a - K(7)) ^ K(15);
        i -= 16;
    }
    out[0] = a; out[1] = b; out[2] = c; out[3] = d;
    out[4] = e; out[5] = f; out[6] = g; out[7] = h;
}

#undef K

/* Caller: RankingCodeBlock::RankingCodeBlock (T431).  Encrypts the 8 bytes
 * at every offset 0 .. len - 8 in turn, in place (each block overlaps the
 * previous one by 7 bytes). */
// FUNC 0x0021CE78 safer_encrypt_buffer
void safer_encrypt_buffer(SaferCtx *ctx, u8 *buf, int len)
{
    u8 *block = buf;
    u8 *last = block + len - SAFER_BLOCK_LEN;

    for (; block <= last; block++)
        safer_encrypt_block(block, block, ctx->tables, ctx->key);
}

/* No callers.  The inverse: decrypts at offsets len - 8 down to 0. */
// FUNC 0x0021CEE0 safer_decrypt_buffer
void safer_decrypt_buffer(SaferCtx *ctx, u8 *buf, int len)
{
    u8 *block;

    for (block = buf + len - SAFER_BLOCK_LEN; block >= buf; block--)
        safer_decrypt_block(block, block, ctx->tables, ctx->key);
}
