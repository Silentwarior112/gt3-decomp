/* T537_gzip_inflate.h - GZIP::InflateBase, Polyphony's C++ port of gzip-1.2.4's inflate.c
 * (translation unit T537, unit u126).
 *
 * Same machine code as the bootstrap's src/gzip_inflate.cpp.  Huffman tables
 * are bump-allocated in a 0x3000-byte arena inside the object, the bit buffer
 * is 64 bits wide, a virtual hook refills the input; no error checks.
 */
#ifndef GAME_T537_GZIP_INFLATE_H
#define GAME_T537_GZIP_INFLATE_H

#include "game/game.h"

#ifdef BOOT_OLD_GCC
/* As pdistd/allocator.h: #pragma interface here, #pragma implementation in
 * T537_gzip_inflate.cpp, so egcs emits the out-of-line copies of the inline
 * members heap_clear, setOutputData and inflate (and the vtable) only there,
 * at the end of T537 before __tf, as the binary has them (Lupo 2: linkonce
 * functions). */
#pragma interface
#endif

namespace GZIP {

/* Huffman table entry, 8 bytes (gzip's struct huft, fields rearranged and the
 * v.n / v.t union split into two fields: link entries write n = 0). */
struct Huft {
    Huft *t;                                    /* 0x0 link entries: next-level table */
    u16   n;                                    /* 0x4 literal value / length base / distance base */
    u8    b;                                    /* 0x6 bits to dump for this (sub)code */
    u8    e;                                    /* 0x7 16 literal, 15 end of block, 0..13 extra bits,
                                                       16+j link to a j-bit sub-table, 99 invalid */
};
EGCS_SIZE(GZIP::Huft, 0x8);

/* The data members of InflateBase, at their original offsets (bitcount:
 * valid bits MINUS 32, -8 * (in & 3) after initBitStream).  egcs: in
 * InflateBase itself, as the original has them: no base class, so its
 * typeinfo is an 8-byte __user_type_info (T537 builds it with __rtti_user,
 * T511 reaches it gp-relative); a base struct makes it a 12-byte
 * __si_type_info.  GCC 14: InflateState, a base struct of their own, so
 * that the vptr at +0 does not move them.  (As the bootstrap's
 * include/gzip.h.) */
#define GZIP_INFLATE_STATE_MEMBERS                                                              \
    u8         heap[0x3000]; /* 0x0000 Huft arena; tables are bump-allocated */                 \
    u64        bitbuf;       /* 0x3000 bit buffer, LSB = next bit */                            \
    s32        bitcount;     /* 0x3008 valid bits in bitbuf MINUS 32 (refilled when < 0) */     \
    const u32 *inNext;       /* 0x300C next input word (32-bit aligned) */                      \
    const u32 *inEnd;        /* 0x3010 in + size (not aligned): fetchInput at inNext == inEnd */\
    u8        *out;          /* 0x3014 output cursor */                                         \
    Huft      *heapPtr;      /* 0x3018 arena bump pointer */                                    \
    Huft      *tl;           /* 0x301C literal/length table */                                  \
    Huft      *td;           /* 0x3020 distance table */                                        \
    u32        bl;           /* 0x3024 lookup bits of tl (inflate_codes: (1 << bl) - 1) */      \
    u32        bd;           /* 0x3028 lookup bits of td (same quirk) */                        \
    u8        *outBase;      /* 0x302C output buffer start */

struct InflateState {                           /* GCC 14: InflateBase's base */
    GZIP_INFLATE_STATE_MEMBERS
};
EGCS_SIZE(GZIP::InflateState, 0x3030);

/* vtable _vt$Q24GZIP11InflateBase 0x003477A8 (2 slots), vptr +0x3030 (egcs).
 * typeinfo __tfQ24GZIP11InflateBase 0x0026E288 (__user_type_info 0x00353508:
 * no polymorphic base), RTTI name 0x003477C0.  Size 0x3038 (on the stack of
 * the (out, in) constructor's users).  Subclass: GZIP::InflateLoader (T511),
 * which refills the input from a stream. */
#ifdef BOOT_OLD_GCC
class InflateBase {
public:
    GZIP_INFLATE_STATE_MEMBERS
#else
class InflateBase : public InflateState {
public:
#endif
    InflateBase();                              /* 0x0026CAE0 vptr only (InflateLoader) */
    InflateBase(void *out, const void *in);     /* 0x0026CAF8 inflates a whole gzip file */
    virtual ~InflateBase();                     /* [0] +0x0C 0x0026CB58 */
    virtual void fetchInput(void);              /* [1] +0x14 0x0026E208 refill hook (default: nothing) */

    void initBitStream(const void *in, int size);           /* 0x0026CB88 */
    int  huft_build(const u32 *b, u32 n, u32 s, const u16 *d, const u16 *e,
                    Huft **t, u32 *m, u32 bits);            /* 0x0026CBC8 */
    void inflate_codes(void);                               /* 0x0026D1D0 */
    void inflate_main(bool raw);                            /* 0x0026D510 raw: no gzip header */
    /* inline (below); the out-of-line copies have no callers */
    void heap_clear(void);                                  /* 0x0026E210 heapPtr = heap */
    void setOutputData(void *out);                          /* 0x0026E218 outBase = out = p */
    void inflate(void *out, const void *in, u32 size, bool raw);   /* 0x0026E228 */
    /*   vptr                                                  0x3030 (egcs) */
};
EGCS_SIZE(GZIP::InflateBase, 0x3038);

/* The inline members, in this order: egcs emits the #pragma implementation
 * copies in reverse definition order (heap_clear 0x0026E210, setOutputData
 * 0x0026E218, inflate 0x0026E228), and inflate, defined before
 * setOutputData, calls it (jal 0x0026E218) instead of inlining it. */

/* initBitStream(in, size); setOutputData(out); inflate_main(raw).  Lupo 2
 * inflate__Q24GZIP11InflateBasePvPCvUib.  No callers in the core. */
inline void InflateBase::inflate(void *outBuf, const void *in, u32 size, bool raw)
{
    initBitStream(in, size);
    setOutputData(outBuf);
    inflate_main(raw);
}

/* outBase (+0x302C) = out cursor (+0x3014) = p.  Lupo 2
 * setOutputData__Q24GZIP11InflateBasePv.  Inlined by the (out, in)
 * constructor and InflateLoader's (T511). */
inline void InflateBase::setOutputData(void *p)
{
    outBase = out = (u8 *)p;
}

/* Rewind the Huffman-table arena: heapPtr (+0x3018) = &heap[0].  Lupo 2
 * heap_clear__Q24GZIP11InflateBase.  Inlined by inflate_main. */
inline void InflateBase::heap_clear(void)
{
    heapPtr = (Huft *)heap;
}

/* Reflected CRC-32 (init ~0, final ~); table crc_table 0x00303608 (.data,
 * local to T537).  Callers: RankingCodeBlock::RankingCodeBlock (0x0021B3E8,
 * T431), Serialize::Base::unseal / seal (0x00242EE0 / 0x00242F60, T509). */
u32 crc32(const u8 *buf, u32 len);              /* 0x0026CA90 */

} /* namespace GZIP */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function and node itself (and rejects a hand
 * declaration). */
extern "C" {
void *__tfQ24GZIP11InflateBase(void);           /* 0x0026E288 */
/* The __user_type_info node, built by __tfQ24GZIP11InflateBase and read by
 * T511's __tfQ24GZIP13InflateLoader. */
extern u32 __tiQ24GZIP11InflateBase[2];         /* 0x00353508 gp-0x44E8 (.sbss) */
}
#endif

#endif /* GAME_T537_GZIP_INFLATE_H */
