/* T431 - the ranking code (0x0021AFA0..0x0021B790).
 *
 * RankingRecord::encode (called by make_ranking_code, T102) packs a race
 * result into 80 bits (car, race, power, weight, kind, time, tyres), puts
 * in front of them 4 bits of a hash of the clock's seconds (clock_seconds)
 * and the 12 bits of a folded CRC-32 of the 12 bytes, encrypts the 12
 * bytes with SAFER (T433: 13 rounds, a fixed 128-bit key at 0x00340A28,
 * every 8-byte window in turn), writes them as digits (bignum_to_decimal,
 * '0' as the fill) and, when there is room for 30 characters, replaces the
 * first character with a check digit of the others.
 *
 * The hash (RankingHash) is a double-length construction on SAFER (10
 * rounds) over 8-byte blocks; its update() has a bug: the whole blocks of
 * a long input all compress the same (first) block.
 */
#include "game/save/game_status_internal.h"
#include "libc.h"                               /* memcpy, memset, strlen (0x00269A68, the MMI strlen) */
#include "game/save/T431_ranking_code.h"
#include "engine/pdistd/T432_bignum.h"                   /* bits_put, bignum_to_decimal */
#include "engine/pdistd/T433_safer.h"                    /* SaferTables / SaferKey / SaferCtx and the cipher */
#include "game/sound/T465_clock.h"                    /* clock_seconds */
#include "engine/pdistd/T537_gzip_inflate.h"             /* GZIP::crc32 */

/* .rodata, the file's only data: the SAFER key of the ranking code, two
 * 8-byte halves (RankingCodeBlock). */
extern const u8 g_00340A28[16];
const u8 g_00340A28[16] = {                     /* 0x00340A28 */
    0xC4, 0x27, 0x87, 0x64, 0x06, 0xE4, 0xA7, 0x06,
    0x86, 0x26, 0x26, 0x06, 0x64, 0x87, 0x27, 0xAA,
};

// FUNC 0x0021AFA0 RankingDigest::RankingDigest
//   All 16 bytes 0.
RankingDigest::RankingDigest()
{
    memset(this, 0, sizeof(RankingDigest));
}

// FUNC 0x0021AFD0 RankingDigest::RankingDigest
//   ? (the copy constructor or the assignment: the same code, no callers)
//   A 16-byte copy (unaligned ldl / ldr, sdl / sdr); returns this.
RankingDigest::RankingDigest(const RankingDigest &o)
{
    __builtin_memcpy(this, &o, sizeof(RankingDigest));
}

// FUNC 0x0021AFF8 RankingDigest::operator=
//   ? (see above) A 16-byte copy; returns *this.
RankingDigest &RankingDigest::operator=(const RankingDigest &o)
{
    __builtin_memcpy(this, &o, sizeof(RankingDigest));
    return *this;
}

// FUNC 0x0021B020 RankingHash::RankingHash
//   The SAFER tables to use, a zero digest (the member's RankingDigest()),
//   an empty buffer.
RankingHash::RankingHash(SaferTables *t) : tables(t), digest()
{
    /* implicit: RankingDigest (compiler-generated: bases, members, deleting form) */
    len = 0;
    memset(buf, 0, SAFER_BLOCK);
}

// FUNC 0x0021B068 RankingHash::compress
//   One 8-byte block: t = E[h2:block](h1); h1 ^= t; then
//   h2 ^= E[block:h1_old](~h2) (the second key is built from the old h1).
void RankingHash::compress(const u8 *block)
{
    u8 tmp[SAFER_BLOCK];                        /* sp+0x00 */
    SaferKey key;                               /* sp+0x10 */
    SaferKey key2;                              /* sp+0xF0 */

    safer_key_init128(&key, tables, digest.h2, block, RANKING_HASH_ROUNDS);
    safer_encrypt_block(tmp, digest.h1, tables, &key);
    safer_key_init128(&key2, tables, block, digest.h1, RANKING_HASH_ROUNDS);
    F_0021C4B0(&key, &key2);                    /* key = key2 */
    for (u32 i = 0; i < SAFER_BLOCK; i++) {
        digest.h1[i] ^= tmp[i];
        tmp[i] = ~digest.h2[i];
    }
    safer_encrypt_block(tmp, tmp, tables, &key);
    for (u32 i = 0; i < SAFER_BLOCK; i++)
        digest.h2[i] ^= tmp[i];
}

// FUNC 0x0021B198 RankingHash::update
//   Fills the buffer first (compressing it when full), then the whole
//   blocks, then keeps the rest.  Bug kept: the whole blocks all compress
//   the block at `src' (src is only advanced after the loop).
void RankingHash::update(const void *data, u32 n)
{
    const u8 *src = (const u8 *)data;

    if (len != 0) {
        u32 room = SAFER_BLOCK - len;
        if (n < room) {
            memcpy(buf + len, src, n);
            len += n;
            return;
        }
        memcpy(buf + len, src, room);
        src += room;
        n -= room;
        compress(buf);
        len = 0;
    }
    u32 blocks = n >> 3;
    for (u32 i = 0; i < blocks; i++)
        compress(src);                          /* (sic) */
    src += blocks * SAFER_BLOCK;
    n -= blocks * SAFER_BLOCK;
    if (n != 0) {
        memcpy(buf, src, n);
        len = n;
    }
}

// FUNC 0x0021B2A0 RankingHash::final
//   A partial block is padded with zeros and compressed.
void RankingHash::final()
{
    if (len != 0) {
        memset(buf + len, 0, SAFER_BLOCK - len);
        compress(buf);
        len = 0;
    }
}

// FUNC 0x0021B2F0 rankingHash32
//   ? name.  The hash of the clock's seconds (clock_seconds, a stack word)
//   and of salt (unless NULL), folded to 32 bits: byte i = d[i] ^ d[i + 4]
//   ^ d[i + 8] ^ d[i + 12].
u32 rankingHash32(SaferTables *t, const char *salt)
{
    RankingHash hash(t);                        /* sp+0x00 */
    int seconds = clock_seconds();              /* sp+0x20; 0x0022BB08 */
    hash.update(&seconds, 4);
    if (salt != NULL)
        hash.update(salt, strlen(salt));
    hash.final();

    /* (egcs regroups each four-term xor as (h1[i] ^ h2[i + 4]) ^ (h2[i] ^
     * h1[i + 4]), and the or chain as retail has it) */
    u32 b3 = hash.digest.h1[3] ^ hash.digest.h1[7] ^ hash.digest.h2[3] ^ hash.digest.h2[7];
    u32 b2 = hash.digest.h1[2] ^ hash.digest.h1[6] ^ hash.digest.h2[2] ^ hash.digest.h2[6];
    u32 b1 = hash.digest.h1[1] ^ hash.digest.h1[5] ^ hash.digest.h2[1] ^ hash.digest.h2[5];
    u32 b0 = hash.digest.h1[0] ^ hash.digest.h1[4] ^ hash.digest.h2[0] ^ hash.digest.h2[4];
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

// FUNC 0x0021B3E8 RankingCodeBlock::RankingCodeBlock
//   The 12-byte code: (a first copy of the bits at code[0..9] and code[10..11]
//   cleared, overwritten below) the SAFER tables built; code cleared, the 80
//   bits at code[2..11]; bits 0..3 = the hash of salt folded to 4 bits, bits
//   4..15 = the CRC-32 of the 12 bytes folded to 12 bits; then SAFER (13
//   rounds, key 0x00340A28) on every 8-byte window of the 12 bytes.
RankingCodeBlock::RankingCodeBlock(const RankingBits &in, const char *salt)
{
    u32 size = RANKING_BITS_BYTES;              /* a variable: retail calls memcpy */
    memcpy(code, in.bits, size);
    memset(code + RANKING_BITS_BYTES, 0, RANKING_CODE_BYTES - RANKING_BITS_BYTES);
    safer_tables_init(&tables);
    memset(code, 0, RANKING_CODE_BYTES);
    memcpy(code + 2, in.bits, size);

    u32 hash = rankingHash32(&tables, salt);
    hash ^= hash >> 16;
    hash ^= hash >> 8;
    hash ^= hash >> 4;
    bits_put(code, 0, hash & 0xF, 4);

    u32 crc = GZIP::crc32(code, RANKING_CODE_BYTES);
    bits_put(code, 4, (crc ^ (crc >> 12) ^ (crc >> 24)) & 0xFFF, 12);

    SaferCtx cipher;                            /* sp+0x00 (declared first: the lower slot) */
    SaferKey key;                               /* sp+0x10 */
    safer_key_init128(&key, &tables, g_00340A28, g_00340A28 + 8, RANKING_CODE_ROUNDS);
    cipher.tables = &tables;
    cipher.key = &key;
    safer_encrypt_buffer(&cipher, code, RANKING_CODE_BYTES);
}

// FUNC 0x0021B528 F_0021B528
//   Empty, no callers (? an empty destructor of this unit).
void F_0021B528(void)
{
}

// FUNC 0x0021B530 F_0021B530
//   Empty, no callers (? an empty destructor of this unit).
void F_0021B530(void)
{
}

// FUNC 0x0021B538 RankingRecord::operator==
//   ? name.  1 if the ten words are equal (name is not compared; no
//   callers).
int RankingRecord::operator==(const RankingRecord &o) const
{
    int equal = 0;
    if (zero == o.zero && carIndex == o.carIndex && courseIndex == o.courseIndex &&
        tuned == o.tuned && power == o.power && weight == o.weight &&
        kind == o.kind && time == o.time && tyreFront == o.tyreFront)
        equal = tyreRear == o.tyreRear;
    return equal;
}

// FUNC 0x0021B5E0 F_0021B5E0
//   Returns 1 (no callers; ? a validity check of the record).
int F_0021B5E0(void)
{
    return 1;
}

// FUNC 0x0021B5E8 RankingRecord::encode
//   The code of the record into out (size bytes): the fields packed into 80
//   bits, the code block (no salt), its 12 bytes as digits; with room for
//   30 characters, out[0] becomes the check digit of the others: h =
//   0xFFFFFFFF, h = h * 3 + c (signed chars out[1..n-1]), '0' + h % 10
//   (unsigned).  Caller: make_ranking_code.
//   90.6%: before each bits_put retail sets $a0 (daddu a0, s1) before
//   loading the field (lw a2); both schedulers here put the load first (its
//   latency gives it the higher priority).  Tried: a pointer local for the
//   bits, (u32) casts, an inline wrapper, a getter, a pointer copy of this,
//   the fields in locals (129 insns), the four flag sets.
void RankingRecord::encode(char *out, u32 size) const
{
    RankingBits bits;                           /* sp+0x00 (the bits at sp+0x01) */
    memset(bits.bits, 0, RANKING_BITS_BYTES);
    /*            bit   value      width */
    bits_put(bits.bits, 0,    zero,      5);
    bits_put(bits.bits, 5,    carIndex,  9);
    bits_put(bits.bits, 14,   courseIndex, 8);
    bits_put(bits.bits, 22,   tuned,     1);
    bits_put(bits.bits, 23,   power,     12);
    bits_put(bits.bits, 35,   weight,    12);
    bits_put(bits.bits, 47,   kind,      3);
    bits_put(bits.bits, 50,   time,      22);
    bits_put(bits.bits, 72,   tyreFront, 4);
    bits_put(bits.bits, 76,   tyreRear,  4);

    RankingCodeBlock block(bits, NULL);         /* sp+0x10 */
    u32 digits = bignum_to_decimal(out, size, block.code, RANKING_CODE_BYTES, '0');
    if (size >= RANKING_CHECK_SIZE) {
        u32 check = 0xFFFFFFFF;
        for (u32 i = 1; i < digits; i++)
            check = check * 3 + (s8)out[i];
        out[0] = (check % 10) | '0';
    }
}
