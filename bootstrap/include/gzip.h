/* gzip.h - GZIP::InflateBase, Polyphony's raw-DEFLATE / gzip inflater used by the
 * GT3 US bootstrap (BOOTSTRAP_SCUS-97102).
 *
 * Implementation: src/gzip_inflate.cpp (.text 0x01011700..0x01012F34).
 * The algorithm is Mark Adler's gzip-1.2.4 inflate.c (huft_build /
 * inflate_codes / inflate_stored / inflate_fixed / inflate_dynamic /
 * inflate_block), reworked into a C++ class:
 *   - Huffman tables are bump-allocated from a 0x3000-byte arena at the start of
 *     the object (heap_clear() rewinds it) instead of malloc/huft_free;
 *   - the input is read 32 bits at a time into a 64-bit bit buffer; when the
 *     input words run out a virtual refill hook (vtable slot 2) is called;
 *   - output goes straight to a caller buffer (no 32K sliding window);
 *   - every error check of the original is gone: no magic/CM check, no
 *     NLEN check, no "invalid code" (e == 99) exit, no oversubscription check,
 *     no CRC32 / ISIZE check (crc32() exists but nothing calls it).
 *
 * Users in the bootstrap:
 *   startup_screen_start (0x01000A50): InflateBase inf(buf, startupa_img_gz);
 *       - the (out, in) constructor inflates a gzip file (with header) at once.
 *   ImageLoader::decompress (0x01004248): InflateBase inf; inf.initBitStream(
 *       data + 6, size - 6); inf.setOutputData(out); inf.inflate_main(true);
 *       - raw DEFLATE body of core.gt3.
 * Both construct the object on the stack (frames 0x3060 / 0x3080) and destroy
 * it with the GCC 2.9x in-charge flag 2 (no delete).
 */
#ifndef BOOT_GZIP_H
#define BOOT_GZIP_H

#include "types.h"
#include <stddef.h>

namespace GZIP {

/* Huffman table entry, 8 bytes (gzip's struct huft, fields rearranged and the
 * v.n / v.t union split into two fields: link entries write n = 0 explicitly). */
struct Huft {
    Huft *t;  /* +0 link entries: next-level table                           */
    u16   n;  /* +4 literal value / length base / distance base              */
    u8    b;  /* +6 bits to dump for this (sub)code                          */
    u8    e;  /* +7 16 = literal, 15 = end of block, 0..13 = extra bits,
                    16+j = link to a j-bit sub-table, 99 = invalid code       */
};
static_assert(sizeof(Huft) == 8, "Huft is 8 bytes");

/* Data members of GZIP::InflateBase at their original offsets.
 * In the GCC 2.9x original these are direct members of InflateBase and the
 * vptr follows them at +0x3030 (object size 0x3034, 8-aligned 0x3038); the
 * typeinfo function __tfQ24GZIP11InflateBase (0x01012EF8, a plain
 * __user_type_info with no base) proves there is no base class.  The matching
 * build (BOOT_OLD_GCC) declares them that way.  The GCC 14 build keeps them in
 * a standard-layout base InflateState so the offsets can be checked with
 * offsetof there (modern g++ puts the vptr in front instead). */
#define GZIP_INFLATE_MEMBERS                                                   \
    u8         heap[0x3000]; /* +0x0000 Huft arena; tables are bump-allocated   */ \
    u64        bitbuf;       /* +0x3000 bit buffer, LSB = next bit             */ \
    s32        bitcount;     /* +0x3008 valid bits in bitbuf MINUS 32 (one word   \
                                        is refilled when it goes < 0; starts at   \
                                        -8 * (in & 3) after initBitStream)      */ \
    const u32 *inNext;       /* +0x300C next input word (32-bit aligned)       */ \
    const u32 *inEnd;        /* +0x3010 in + size (NOT aligned); refill hook is   \
                                        called when inNext == inEnd            */ \
    u8        *out;          /* +0x3014 output cursor                          */ \
    Huft      *heapPtr;      /* +0x3018 arena bump pointer                     */ \
    Huft      *tl;           /* +0x301C literal/length table                   */ \
    Huft      *td;           /* +0x3020 distance table                         */ \
    u32        bl;           /* +0x3024 lookup bits of tl (inflate_codes          \
                                        overwrites it with (1<<bl)-1)          */ \
    u32        bd;           /* +0x3028 lookup bits of td (same quirk)         */ \
    u8        *outBase;      /* +0x302C output buffer start                    */

#ifndef BOOT_OLD_GCC
struct InflateState {
    GZIP_INFLATE_MEMBERS
};
static_assert(offsetof(InflateState, bitbuf)   == 0x3000, "layout");
static_assert(offsetof(InflateState, bitcount) == 0x3008, "layout");
static_assert(offsetof(InflateState, inNext)   == 0x300C, "layout");
static_assert(offsetof(InflateState, inEnd)    == 0x3010, "layout");
static_assert(offsetof(InflateState, out)      == 0x3014, "layout");
static_assert(offsetof(InflateState, heapPtr)  == 0x3018, "layout");
static_assert(offsetof(InflateState, tl)       == 0x301C, "layout");
static_assert(offsetof(InflateState, td)       == 0x3020, "layout");
static_assert(offsetof(InflateState, bl)       == 0x3024, "layout");
static_assert(offsetof(InflateState, bd)       == 0x3028, "layout");
static_assert(offsetof(InflateState, outBase)  == 0x302C, "layout");
static_assert(sizeof(InflateState) == 0x3030, "layout");
#endif

/* GZIP::InflateBase - RTTI name "Q24GZIP11InflateBase" @0x0103FD08.
 * vtable _vt$Q24GZIP11InflateBase: vptr value 0x0103FCF0 (stored at +0x3030),
 * entries {s16 delta, s16 index, pfn}:
 *   [0] 0x0103FCF0  __tfQ24GZIP11InflateBase   0x01012EF8
 *   [1] 0x0103FCF8  ~InflateBase(int)          0x010117C8
 *   [2] 0x0103FD00  fetchInput()               0x01012E78 (default: no-op)
 * No subclass exists in the bootstrap (Lupo 2 has GZIP::InflateLoader). */
#ifdef BOOT_OLD_GCC
class InflateBase {
public:
    GZIP_INFLATE_MEMBERS
#else
class InflateBase : public InflateState {
#endif
public:
    InflateBase();                                       /* 0x01011750 */
    InflateBase(void *out, const void *in);              /* 0x01011768 inflates a gzip file */
    virtual ~InflateBase();                              /* 0x010117C8 vtable slot 1 */

    void initBitStream(const void *in, int size);        /* 0x010117F8 */
    int  huft_build(const u32 *b, u32 n, u32 s, const u16 *d, const u16 *e,
                    Huft **t, u32 *m, u32 bits);         /* 0x01011838 */
    void inflate_codes(void);                            /* 0x01011E40 */
    void inflate_main(bool raw);                         /* 0x01012180 raw: no gzip header */
    virtual void fetchInput(void);                       /* 0x01012E78 vtable slot 2 */
    void heap_clear(void);                               /* 0x01012E80 */
    void setOutputData(void *out);                       /* 0x01012E88 */
    void inflate(void *out, const void *in, u32 size, bool raw); /* 0x01012E98 (no callers) */
};
#ifdef BOOT_OLD_GCC
static_assert(sizeof(InflateBase) == 0x3038, "layout: members, vptr at +0x3030");
#endif

/* 0x01011700: standard table-driven CRC-32 (init ~0, final ~). No callers. */
u32 crc32(const u8 *buf, u32 len);

} /* namespace GZIP */

#endif /* BOOT_GZIP_H */
