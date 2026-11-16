/* T431_ranking_code.h - the ranking code: RankingRecord and its encoder, the
 * SAFER tables / key (translation unit T431, unit u098).
 *
 * RankingRecord::encode (caller: make_ranking_code, T102) packs a race
 * result into 80 bits, puts 4 bits of a hash of the clock's seconds
 * (clock_seconds, T465)
 * and the 12 bits of a folded CRC-32 in front of them, encrypts the 12 bytes
 * with SAFER SK (T433: 13 rounds, the fixed 128-bit key at 0x00340A28, every
 * 8-byte window in turn) and writes them as digits with a check digit.  The
 * hash (RankingHash) is a double-length construction on SAFER (10 rounds).
 *
 * SaferTables and SaferKey (the SAFER tables and key) are T433's.
 * ? names, except RankingRecord (u015). */
#ifndef GAME_T431_RANKING_CODE_H
#define GAME_T431_RANKING_CODE_H

#include "game/game.h"
#include "engine/pdistd/T433_safer.h"               /* SaferTables (held by value), SaferKey */

#ifdef __cplusplus                          /* a C++ translation unit */

/* The 16-byte state of RankingHash: two 8-byte halves. */
struct RankingDigest {
    RankingDigest();                        /* 0x0021AFA0 all 0 */
    RankingDigest(const RankingDigest &o);  /* 0x0021AFD0 (? or the assignment: the same code, no callers) */
    RankingDigest &operator=(const RankingDigest &o);   /* 0x0021AFF8 */

    u8 h1[8];                               /* 0x0 */
    u8 h2[8];                               /* 0x8 */
};
EGCS_SIZE(RankingDigest, 0x10);

/* A double-length hash over 8-byte blocks built on SAFER (10 rounds):
 * t = E[h2:block](h1), h1 ^= t, h2 ^= E[block:h1_old](~h2).  update() has
 * a bug: the whole blocks of a long input all compress its first block. */
struct RankingHash {
    RankingHash(SaferTables *t);            /* 0x0021B020 */
    void compress(const u8 *block);         /* 0x0021B068 */
    void update(const void *data, u32 n);   /* 0x0021B198 */
    void final();                           /* 0x0021B2A0 pads a partial block with zeros */

    SaferTables  *tables;                   /* 0x00 */
    RankingDigest digest;                   /* 0x04 */
    u8            buf[8];                   /* 0x14 */
    u32           len;                      /* 0x1C bytes in buf */
};
EGCS_SIZE(RankingHash, 0x20);

/* The 80 packed bits of a record, at +1 (the encoder's stack layout). */
struct RankingBits {
    u8 unk_00;                              /* 0x0 */
    u8 bits[10];                            /* 0x1 */
};

/* The encrypted code: its 12 bytes at +1, the SAFER tables after them (the
 * encoder's stack layout). */
struct RankingCodeBlock {
    RankingCodeBlock(const RankingBits &in, const char *salt); /* 0x0021B3E8 salt: NULL here */

    u8          unk_00;                     /* 0x00 */
    u8          code[12];                   /* 0x01 */
    SaferTables tables;                     /* 0x0D */
};

/* RankingRecord::kind, make_ranking_code's kind (T102): what the record is
 * of.  The max speed test's `time' is the speed x 1000 (make_ranking_code
 * skips the time split for it). */
enum RankingKind {
    RANKING_KIND_TIME_TRIAL = 0,
    RANKING_KIND_FREE_RUN   = 1,
    RANKING_KIND_LICENSE    = 2,            /* a licence test */
    RANKING_KIND_400M       = 3,            /* the machine tests: 0-400 m */
    RANKING_KIND_1000M      = 4,            /* 0-1000 m */
    RANKING_KIND_MAX_SPEED  = 5             /* max speed */
};

/* A race result as the ranking code holds it (u015's name).  The width of
 * each field in the code in the comments.  Callers fill it on the stack:
 * make_ranking_code (T102) also stores its name argument at +0x28, a word
 * egcs would not keep for a dead local, so it is part of the record
 * (encode and operator== do not read it).  Size 0x2C. */
struct RankingRecord {
    int  operator==(const RankingRecord &o) const;  /* 0x0021B538 (? name) the ten words equal; no callers */
    void encode(char *out, u32 size) const;         /* 0x0021B5E8 the digits into out (size bytes) */

    int zero;                               /* 0x00 5 bits (0) */
    int carIndex;                           /* 0x04 9 bits (make_ranking_code: F_00239F30 of the car's code) */
    int courseIndex;                        /* 0x08 8 bits (db_course_index of the course) */
    int tuned;                              /* 0x0C 1 bit: the car is tuned (CarEquipments::isTuned) */
    int power;                              /* 0x10 12 bits the power of the car's spec (ps) */
    int weight;                             /* 0x14 12 bits its weight */
    int kind;                               /* 0x18 3 bits make_ranking_code's kind (RankingKind) */
    int time;                               /* 0x1C 22 bits (ms) */
    int tyreFront;                          /* 0x20 4 bits the tyre type, folded below 13 (12 on dirt) */
    int tyreRear;                           /* 0x24 4 bits */
    const char *name;                       /* 0x28 the name entered for the record (not encoded; u015 extra) */
};
EGCS_SIZE(RankingRecord, 0x2C);

/* The hash of the word clock_seconds() and of salt (unless NULL), folded
 * to 32 bits. */
u32  rankingHash32(SaferTables *t, const char *salt);  /* 0x0021B2F0 */
void F_0021B528(void);                      /* 0x0021B528 empty, no callers */
void F_0021B530(void);                      /* 0x0021B530 empty, no callers */
int  F_0021B5E0(void);                      /* 0x0021B5E0 returns 1, no callers */

#endif /* __cplusplus */

#endif /* GAME_T431_RANKING_CODE_H */
