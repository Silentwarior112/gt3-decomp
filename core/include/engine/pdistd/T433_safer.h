/* T433_safer.h - the SAFER SK block cipher (translation unit T433, unit u099).
 *
 * J. L. Massey's SAFER as in R. De Moliner's reference safer.c: 8-byte
 * blocks, at most 13 rounds, the strengthened key schedule; the exp / log
 * tables are passed in (SaferTables) instead of being globals, and two
 * functions run the cipher over a buffer one byte at a time (every 8-byte
 * window in turn).  User: the ranking code (T431).  ? names (u099). */
#ifndef GAME_T433_SAFER_H
#define GAME_T433_SAFER_H

#include "game/game.h"

#ifdef __cplusplus                          /* a C++ translation unit (C++ linkage) */

/* The exp / log tables of 45^x mod 257 (safer_tables_init). */
struct SaferTables {
    u8 log[0x100];                          /* 0x000 log[exp[i]] = i */
    u8 exp[0x100];                          /* 0x100 45^i mod 257 (256 stored as 0) */
};
EGCS_SIZE(SaferTables, 0x200);

/* An expanded SAFER key (safer_key_init128): k[0] = the number of rounds,
 * then 8 * (2 * rounds + 1) subkey bytes; 1 + 8 * (1 + 2 * 13) bytes for
 * the 13 rounds at most. */
struct SaferKey {
    u8 k[0xD9];                             /* 0x00 */
};
EGCS_SIZE(SaferKey, 0xD9);

/* What the buffer functions take (u098 calls it SaferCipher).  The caller
 * builds it on the stack. */
struct SaferCtx {
    SaferTables *tables;                    /* 0x0 */
    SaferKey    *key;                       /* 0x4 */
};
EGCS_SIZE(SaferCtx, 0x8);

SaferTables *safer_tables_init(SaferTables *t);                 /* 0x0021C220 builds both tables; returns t */
SaferKey *safer_key_init_zero(SaferKey *key, const SaferTables *t,
                              u32 rounds);                      /* 0x0021C2C8 an all-zero user key; no callers */
SaferKey *safer_key_init64(SaferKey *key, const SaferTables *t, const u8 *k1,
                           u32 rounds);                         /* 0x0021C330 k1 and a zero half; no callers */
SaferKey *safer_key_init128(SaferKey *key, const SaferTables *t, const u8 *k1,
                            const u8 *k2, u32 rounds);          /* 0x0021C3A0 the key k1:k2 */
SaferKey *F_0021C3C8(SaferKey *dst, const SaferKey *src);       /* 0x0021C3C8 *dst = *src (? the copy ctor); no callers */
SaferKey *F_0021C4B0(SaferKey *dst, const SaferKey *src);       /* 0x0021C4B0 *dst = *src (? operator=) */
void safer_expand_key(SaferKey *key, const SaferTables *t, const u8 *k1,
                      const u8 *k2, u32 rounds);                /* 0x0021C598 Safer_Expand_Userkey (strengthened) */
void safer_encrypt_block(u8 *out, const u8 *in, const SaferTables *t,
                         const SaferKey *key);                  /* 0x0021C820 out may be in */
void safer_decrypt_block(u8 *out, const u8 *in, const SaferTables *t,
                         const SaferKey *key);                  /* 0x0021CB38 out may be in */
void safer_encrypt_buffer(SaferCtx *ctx, u8 *buf, int len);     /* 0x0021CE78 in place, offsets 0 .. len - 8 */
void safer_decrypt_buffer(SaferCtx *ctx, u8 *buf, int len);     /* 0x0021CEE0 offsets len - 8 .. 0; no callers */

#endif /* __cplusplus */

#endif /* GAME_T433_SAFER_H */
