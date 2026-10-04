/* gzip_inflate.cpp - GZIP::InflateBase and crc32 (.text 0x01011700..0x01012F34)
 *
 * Polyphony's C++ port of gzip-1.2.4's inflate.c (Mark Adler), see include/gzip.h
 * for the object layout and the list of differences.  The gzip-1.2.4 lineage is
 * certain from huft_build: the `if (j < z)` guard that zlib later added before
 * `while (++j < z)` is absent, and the value-list end is `v + n` (not x[g]).
 *
 * The translation unit is C++ with EH (every function has an .eh_frame FDE at
 * .data 0x0103A1A0..0x0103A39C), preceded by printf.cxx/allocator.cxx and
 * followed by the non-EH pdistd helpers at 0x01012F38.
 *
 * Nothing in this file verifies anything: no gzip magic/CM check, no stored-
 * block NLEN check, no invalid-code exit, no CRC32/ISIZE check.  core.gt3's
 * integrity check (SHA-512 + RSA) is entirely in ImageLoader::verify - which
 * inflates the body FIRST, so this code runs on unauthenticated input.  There
 * is no output limit (the output cursor just advances), LZ distances are not
 * checked against the output start, and the 0x3000-byte table arena has no
 * bounds check (the fields at +0x3000.. and, in the original layout, the vptr
 * at +0x3030 follow it directly).
 *
 * Bit reader (registers b, k, next, end, op in the functions below):
 *   b    = 64-bit bit buffer, LSB first
 *   k    = (valid bits in b) - 32, i.e. b holds k+32 >= 32 valid bits after the
 *          first DUMPBITS, so up to 32 bits can be peeked without a NEEDBITS
 *          (initBitStream starts at k = -8 * (in & 3): fewer than 32 valid bits
 *          until the first refill)
 *   DUMPBITS(n): k -= n; b >>= n; if k < 0 { k += 32; [refill hook]; b |= *next++ << k }
 *   The refill hook (next == end) writes b, k and the output cursor back to the
 *   object - but NOT next/end - calls the virtual fetchInput() (vtable slot 2,
 *   default no-op), then reloads all five fields and reads *next regardless.
 *   With the default hook, next is thereby reset to the stale this->inNext
 *   (the value at function entry / last write-back).  inEnd = in + size is not
 *   word aligned, so the hook only ever fires when in + size is 4-aligned; the
 *   (out, in) constructor passes size = 0xFFFFFFFF so it never fires there.
 */
#include "types.h"
#include "gzip.h"

namespace GZIP {

/* ---- tables (.data; non-const in the original, never written) --------------
 * Extracted by tools/gzip_tables.py. */

static u32 crc_table[256] = { // @0x01039C70 (.data)
    0x00000000U, 0x77073096U, 0xEE0E612CU, 0x990951BAU, 0x076DC419U, 0x706AF48FU,
    0xE963A535U, 0x9E6495A3U, 0x0EDB8832U, 0x79DCB8A4U, 0xE0D5E91EU, 0x97D2D988U,
    0x09B64C2BU, 0x7EB17CBDU, 0xE7B82D07U, 0x90BF1D91U, 0x1DB71064U, 0x6AB020F2U,
    0xF3B97148U, 0x84BE41DEU, 0x1ADAD47DU, 0x6DDDE4EBU, 0xF4D4B551U, 0x83D385C7U,
    0x136C9856U, 0x646BA8C0U, 0xFD62F97AU, 0x8A65C9ECU, 0x14015C4FU, 0x63066CD9U,
    0xFA0F3D63U, 0x8D080DF5U, 0x3B6E20C8U, 0x4C69105EU, 0xD56041E4U, 0xA2677172U,
    0x3C03E4D1U, 0x4B04D447U, 0xD20D85FDU, 0xA50AB56BU, 0x35B5A8FAU, 0x42B2986CU,
    0xDBBBC9D6U, 0xACBCF940U, 0x32D86CE3U, 0x45DF5C75U, 0xDCD60DCFU, 0xABD13D59U,
    0x26D930ACU, 0x51DE003AU, 0xC8D75180U, 0xBFD06116U, 0x21B4F4B5U, 0x56B3C423U,
    0xCFBA9599U, 0xB8BDA50FU, 0x2802B89EU, 0x5F058808U, 0xC60CD9B2U, 0xB10BE924U,
    0x2F6F7C87U, 0x58684C11U, 0xC1611DABU, 0xB6662D3DU, 0x76DC4190U, 0x01DB7106U,
    0x98D220BCU, 0xEFD5102AU, 0x71B18589U, 0x06B6B51FU, 0x9FBFE4A5U, 0xE8B8D433U,
    0x7807C9A2U, 0x0F00F934U, 0x9609A88EU, 0xE10E9818U, 0x7F6A0DBBU, 0x086D3D2DU,
    0x91646C97U, 0xE6635C01U, 0x6B6B51F4U, 0x1C6C6162U, 0x856530D8U, 0xF262004EU,
    0x6C0695EDU, 0x1B01A57BU, 0x8208F4C1U, 0xF50FC457U, 0x65B0D9C6U, 0x12B7E950U,
    0x8BBEB8EAU, 0xFCB9887CU, 0x62DD1DDFU, 0x15DA2D49U, 0x8CD37CF3U, 0xFBD44C65U,
    0x4DB26158U, 0x3AB551CEU, 0xA3BC0074U, 0xD4BB30E2U, 0x4ADFA541U, 0x3DD895D7U,
    0xA4D1C46DU, 0xD3D6F4FBU, 0x4369E96AU, 0x346ED9FCU, 0xAD678846U, 0xDA60B8D0U,
    0x44042D73U, 0x33031DE5U, 0xAA0A4C5FU, 0xDD0D7CC9U, 0x5005713CU, 0x270241AAU,
    0xBE0B1010U, 0xC90C2086U, 0x5768B525U, 0x206F85B3U, 0xB966D409U, 0xCE61E49FU,
    0x5EDEF90EU, 0x29D9C998U, 0xB0D09822U, 0xC7D7A8B4U, 0x59B33D17U, 0x2EB40D81U,
    0xB7BD5C3BU, 0xC0BA6CADU, 0xEDB88320U, 0x9ABFB3B6U, 0x03B6E20CU, 0x74B1D29AU,
    0xEAD54739U, 0x9DD277AFU, 0x04DB2615U, 0x73DC1683U, 0xE3630B12U, 0x94643B84U,
    0x0D6D6A3EU, 0x7A6A5AA8U, 0xE40ECF0BU, 0x9309FF9DU, 0x0A00AE27U, 0x7D079EB1U,
    0xF00F9344U, 0x8708A3D2U, 0x1E01F268U, 0x6906C2FEU, 0xF762575DU, 0x806567CBU,
    0x196C3671U, 0x6E6B06E7U, 0xFED41B76U, 0x89D32BE0U, 0x10DA7A5AU, 0x67DD4ACCU,
    0xF9B9DF6FU, 0x8EBEEFF9U, 0x17B7BE43U, 0x60B08ED5U, 0xD6D6A3E8U, 0xA1D1937EU,
    0x38D8C2C4U, 0x4FDFF252U, 0xD1BB67F1U, 0xA6BC5767U, 0x3FB506DDU, 0x48B2364BU,
    0xD80D2BDAU, 0xAF0A1B4CU, 0x36034AF6U, 0x41047A60U, 0xDF60EFC3U, 0xA867DF55U,
    0x316E8EEFU, 0x4669BE79U, 0xCB61B38CU, 0xBC66831AU, 0x256FD2A0U, 0x5268E236U,
    0xCC0C7795U, 0xBB0B4703U, 0x220216B9U, 0x5505262FU, 0xC5BA3BBEU, 0xB2BD0B28U,
    0x2BB45A92U, 0x5CB36A04U, 0xC2D7FFA7U, 0xB5D0CF31U, 0x2CD99E8BU, 0x5BDEAE1DU,
    0x9B64C2B0U, 0xEC63F226U, 0x756AA39CU, 0x026D930AU, 0x9C0906A9U, 0xEB0E363FU,
    0x72076785U, 0x05005713U, 0x95BF4A82U, 0xE2B87A14U, 0x7BB12BAEU, 0x0CB61B38U,
    0x92D28E9BU, 0xE5D5BE0DU, 0x7CDCEFB7U, 0x0BDBDF21U, 0x86D3D2D4U, 0xF1D4E242U,
    0x68DDB3F8U, 0x1FDA836EU, 0x81BE16CDU, 0xF6B9265BU, 0x6FB077E1U, 0x18B74777U,
    0x88085AE6U, 0xFF0F6A70U, 0x66063BCAU, 0x11010B5CU, 0x8F659EFFU, 0xF862AE69U,
    0x616BFFD3U, 0x166CCF45U, 0xA00AE278U, 0xD70DD2EEU, 0x4E048354U, 0x3903B3C2U,
    0xA7672661U, 0xD06016F7U, 0x4969474DU, 0x3E6E77DBU, 0xAED16A4AU, 0xD9D65ADCU,
    0x40DF0B66U, 0x37D83BF0U, 0xA9BCAE53U, 0xDEBB9EC5U, 0x47B2CF7FU, 0x30B5FFE9U,
    0xBDBDF21CU, 0xCABAC28AU, 0x53B39330U, 0x24B4A3A6U, 0xBAD03605U, 0xCDD70693U,
    0x54DE5729U, 0x23D967BFU, 0xB3667A2EU, 0xC4614AB8U, 0x5D681B02U, 0x2A6F2B94U,
    0xB40BBE37U, 0xC30C8EA1U, 0x5A05DF1BU, 0x2D02EF8DU,
};

/* Copy lengths for literal codes 257..285 (+2 unused) */
static u16 cplens[31] = { // @0x0103A070 (.data)
    3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
    35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0,
};

/* Extra bits for literal codes 257..285 (99 = invalid) */
static u16 cplext[31] = { // @0x0103A0B0 (.data)
    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 99, 99,
};

/* Copy offsets for distance codes 0..29.  Entries 30 and 31 are the .data
 * alignment padding (0): there is no "nd > 30" check, so a dynamic block with
 * nd = 31/32 makes huft_build read them (distance base 0, 0 extra bits). */
static u16 cpdist[32] = { // @0x0103A0F0 (.data)
    1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129,
    193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577,
    0, 0,
};

/* Extra bits for distance codes (entries 30, 31: padding, read for nd = 31/32) */
static u16 cpdext[32] = { // @0x0103A130 (.data)
    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6,
    6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13,
    0, 0,
};

/* Order of the bit length code lengths */
static u8 border[19] = { // @0x0103A170 (.data)
    16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15,
};

#define BMAX  16    /* maximum bit length of any code */
#define N_MAX 288   /* maximum number of codes in any set */

/* Variable shift counts.  The EE's variable shifts use the count mod 64
 * (dsllv / dsrlv) or mod 32 (sllv), and the inflater depends on that (see
 * GZ_DUMPBITS and GZ_MASK).  The retail compiler (BOOT_OLD_GCC) emits exactly
 * those instructions for a plain variable shift, so there the count is used
 * as is, as in the original; for any other compiler the masking is written
 * out, giving the C the same, defined result. */
#ifdef BOOT_OLD_GCC
#define GZ_SH64(n) (n)
#define GZ_SH32(n) (n)
#else
#define GZ_SH64(n) ((n) & 63)
#define GZ_SH32(n) ((n) & 31)
#endif

/* (1 << n) - 1 as the machine computes it: sllv uses the shift amount mod 32
 * (only matters for the unchecked e == 99 "invalid code" entries). */
#define GZ_MASK(n) ((1U << GZ_SH32((u32)(n))) - 1)

/* Refill hook: write back bit buffer, bit count and output cursor (not
 * next/end), call the virtual input-refill, reload everything. */
#define GZ_REFILL_HOOK()                                                      \
    {                                                                         \
        bitbuf = b;                                                           \
        bitcount = k;                                                         \
        out = op;                                                             \
        fetchInput();                                                         \
        b = bitbuf;                                                           \
        k = bitcount;                                                         \
        next = inNext;                                                        \
        end = inEnd;                                                          \
        op = out;                                                             \
    }

/* Drop n bits and top the buffer back up to >= 32 valid bits.
 * The shifts are dsrlv / dsllv, which use the count mod 64 (GZ_SH64).
 * It matters: k can still be negative after `k += 32` (only one word is ever
 * refilled).  initBitStream leaves k = -8 * (in & 3), so with in & 3 == 3 the
 * gzip-header path's first DUMPBITS(16) gives k = -8 and the new word is
 * shifted left by 56, i.e. the header is mis-parsed (bytes are lost) - the
 * original does exactly that.  nb > 63 needs a table pointer gone wild
 * (e == 99 links). */
#define GZ_DUMPBITS(nbits)                                                    \
    {                                                                         \
        u32 nb_ = (nbits);                                                    \
        k -= (s32)nb_;                                                        \
        b >>= GZ_SH64(nb_);                                                   \
        if (k < 0) {                                                          \
            k += 32;                                                          \
            if (next == end)                                                  \
                GZ_REFILL_HOOK()                                              \
            b |= (u64)*next++ << GZ_SH64(k);                                  \
        }                                                                     \
    }

/* Take the low n bits of the buffer as a value, then drop them (GNU
 * statement expression): the value is extracted before the refill, and
 * whatever the caller does with it (add a base, store it) comes after. */
#define GZ_GETBITS(nbits)                                                     \
    ({                                                                        \
        s32 gb_ = b & ((1U << (nbits)) - 1);                                  \
        GZ_DUMPBITS(nbits);                                                   \
        gb_;                                                                  \
    })

/* Decode the block with the tables just built (gzip's `inflate_codes(tl, td,
 * bl, bd)` at the end of inflate_fixed / inflate_dynamic): write the state
 * back to the object, call inflate_codes, reload the state. */
#define GZ_INFLATE_CODES()                                                    \
    {                                                                         \
        bitbuf = b;                                                           \
        bitcount = k;                                                         \
        inNext = next;                                                        \
        inEnd = end;                                                          \
        out = op;                                                             \
        inflate_codes();                                                      \
        b = bitbuf;                                                           \
        k = bitcount;                                                         \
        next = inNext;                                                        \
        end = inEnd;                                                          \
        op = out;                                                             \
    }

// FUNC 0x01011700 GZIP::crc32
/* Classic reflected CRC-32 over buf[0..len): crc = ~0, per byte
 * crc = crc_table[(crc ^ c) & 0xff] ^ (crc >> 8), returns ~crc.
 * No callers anywhere: the inflater never checks the gzip CRC32/ISIZE trailer. */
u32 crc32(const u8 *buf, u32 len)
{
    const u32 *tab = crc_table;     /* (loaded before the length test) */
    u32 crc = 0xFFFFFFFFU;
    const u8 *end = buf + len;

    if (len != 0) {
        do {
            crc = tab[(crc ^ *buf++) & 0xFF] ^ (crc >> 8);
        } while (buf != end);
    }
    return ~crc;
}

// FUNC 0x01011750 GZIP::InflateBase::InflateBase(void)
/* Default constructor: only installs the vptr (0x0103FCF0 at +0x3030).
 * Used by ImageLoader::decompress (0x01004248), which then calls
 * initBitStream / setOutputData / inflate_main(true) itself. */
InflateBase::InflateBase()
{
}

// FUNC 0x01011768 GZIP::InflateBase::InflateBase(void*, const void*)
/* Construct-and-inflate: sets the vptr, initBitStream(in, 0xFFFFFFFF) (so
 * inEnd = in - 1 and the refill hook never fires), output base = cursor = out
 * (setOutputData inlined: +0x302C then +0x3014), then inflate_main(false),
 * i.e. the input is a gzip file with header.  Returns this.
 * Caller: startup_screen_start (0x01000A50) on the embedded startupa.img. */
InflateBase::InflateBase(void *out_, const void *in)
{
    initBitStream(in, -1);
    outBase = out = (u8 *)out_;
    inflate_main(false);
}

// FUNC 0x010117C8 GZIP::InflateBase::~InflateBase(int)
/* Virtual destructor (vtable slot 1). Re-installs the InflateBase vptr; GCC 2.9x
 * in-charge flag: if (flags & 1) operator delete(this) (0x01019910).
 * Both callers destroy stack objects with flags = 2. */
InflateBase::~InflateBase()
{
}

// FUNC 0x010117F8 GZIP::InflateBase::initBitStream(const void*, int)
/* Sets up the bit reader on in[0..size): inEnd = in + size (unaligned), aligns
 * in down to a word, preloads that word shifted right by the misalignment into
 * the 64-bit buffer and sets bitcount = -(8 * misalignment), i.e.
 * (32 - 8*mis) valid bits - 32.  inNext = next aligned word.
 * Callers: the (out,in) ctor, inflate(), ImageLoader::decompress. */
void InflateBase::initBitStream(const void *in, int size)
{
    u32 mis = (u32)(size_t)in & 3;
    const u32 *p;

    inEnd = (const u32 *)((const u8 *)in + size);
    p = (const u32 *)((const u8 *)in - mis);
    bitbuf = *p++ >> (mis * 8);
    inNext = p;
    bitcount = -(s32)(mis * 8);
}

// FUNC 0x01011838 GZIP::InflateBase::huft_build
/* gzip-1.2.4 huft_build() with Polyphony changes:
 *   - extra 9th argument `bits`: *m = bits is stored first, then read back as
 *     the requested lookup bits l (callers pass 7/9/6 or 7/5);
 *   - tables are bump-allocated from heapPtr (no malloc, no NULL check, no
 *     bounds check against the 0x3000-byte arena);
 *   - y is unsigned, so both `if ((y -= c[j]) < 0) return 2;` checks are gone
 *     (over-subscribed code sets are accepted);
 *   - link entries set r.n = 0 (t and n are separate fields here).
 * Keeps gzip-1.2.4's missing `if (j < z)` guard before `while (++j < z)`.
 * Returns 0 for an empty code set, else (y != 0 && g != 1) (incomplete set);
 * every caller ignores the result.
 * Stack: c[17] @sp+0, x[17] @sp+0x50, v[288] @sp+0xA0, u[16] @sp+0x520,
 * r @sp+0x560 (copied into tables with ldl/ldr + sdl/sdr, i.e. unaligned 8B).
 * Code shape (what the register use of the binary shows):
 *   - the count/offset/value loops walk pointers (the binary compares
 *     pointers, which this compiler never derives from an index loop), and
 *     each loop has its own pointer variables: one pointer and one end
 *     pointer for the clear and count loops (t4/t5), separate ones for the
 *     y, offset and value loops (the value loop walks b itself);
 *   - both length scans use their own signed counter (slti in the first);
 *     the minimum goes to mn, the maximum to g; the y loop ends at c + g,
 *     the offset loop at c + (scan counter): the second is the expression
 *     the scan loop already computed (shared register t2), the first is
 *     computed afresh, as in the binary;
 *   - h is unsigned (h = -1 is loaded as 0xFFFFFFFF);
 *   - *++xp is read into a temporary before the compare (one load, no copy);
 *   - the table link word is a local copy tp of t (the early exit stores
 *     through t's own argument register).  tp is assigned once more at the
 *     very end (a dead store, deleted by the compiler): the binary reloads tp
 *     from its stack slot for `*tp = NULL` instead of reusing the register
 *     that held heapPtr, which is the choice CSE makes when tp is mentioned
 *     after q's last use. */
int InflateBase::huft_build(const u32 *b,   /* code lengths in bits */
                            u32 n,          /* number of codes */
                            u32 s,          /* number of simple-valued codes (0..s-1) */
                            const u16 *d,   /* base values for non-simple codes */
                            const u16 *e,   /* extra bits for non-simple codes */
                            Huft **t,       /* result: starting table */
                            u32 *m,         /* lookup bits, returns actual */
                            u32 bits)       /* initial *m */
{
    Huft **tp = t;      /* link word to fill next (starts at *t) */
    u32 c[BMAX + 1];    /* bit length count table */
    u32 x[BMAX + 1];    /* bit offsets, then code stack */
    u32 v[N_MAX];       /* values in order of bit length */
    Huft *u[BMAX];      /* table stack */
    Huft r;             /* table entry for structure assignment */
    u32 f;              /* i repeats in table every f entries */
    s32 g;              /* maximum code length */
    u32 h;              /* table level */
    u32 i;              /* counter, current code */
    u32 j;              /* counter */
    u32 a;              /* counter for codes of length k */
    u32 k;              /* number of bits in current code */
    u32 l;              /* bits per table (returned in m) */
    const u32 *p;       /* pointer into v[] */
    Huft *q;            /* points to current table */
    u32 w;              /* bits before this table == (l * h) */
    u32 *xp;            /* pointer into c */
    u32 y;              /* number of dummy codes added (unsigned!) */
    u32 z;              /* number of entries in current table */
    u32 mn;             /* minimum code length */
    s32 sc;             /* length scan counter */

    *m = bits;

    /* Generate counts for each bit length */
    {
        u32 *cp = (u32 *)((size_t)c - sizeof(u32));  /* (pre-increment walk) */
        const u32 *ce = c + BMAX;
        do {
            *++cp = 0;
        } while (cp != ce);
        cp = (u32 *)b;
        ce = b + n;
        do {
            c[*cp]++;
            cp++;
        } while (cp != ce);
    }
    if (c[0] == n) {            /* null input--all zero length codes */
        *t = NULL;
        *m = 0;
        return 0;
    }

    /* Find minimum and maximum length, bound *m by those */
    l = *m;
    for (sc = 1; sc <= BMAX; sc++)
        if (c[sc])
            break;
    mn = sc;                    /* minimum code length */
    if (l < mn)
        l = mn;
    for (sc = BMAX; sc; sc--)
        if (c[sc])
            break;
    g = sc;                     /* maximum code length */
    if ((u32)g < l)
        l = g;
    *m = l;

    /* Adjust last length count to fill out codes, if needed */
    {
        u32 *yp = c + mn;
        u32 *ye = c + g;
        for (y = 1U << mn; yp != ye; yp++, y <<= 1)
            y -= *yp;           /* no "< 0 => return 2": y is unsigned */
        y -= *yp;
        *yp += y;
    }

    /* Generate starting offsets into the value table for each length */
    x[0] = 0;                   /* extra store, x[0] is set again below */
    {
        u32 sum;
        u32 *xq;
        const u32 *cp;
        const u32 *ce;

        x[1] = sum = 0;
        xq = x + 2;
        cp = c + 1;
        ce = c + sc;            /* (== c + g) */
        while (cp != ce)
            *xq++ = (sum += *cp++);
    }

    /* Make a table of values in order of bit lengths (b is walked; not
     * used after this) */
    {
        u32 vi = 0;
        const u32 *be;
        u32 len;

        for (be = b + n; b != be; vi++)
            if ((len = *b++) != 0)
                v[x[len]++] = vi;
    }
    /* (no `n = x[g]`: the value list end stays v + n, as in gzip-1.2.4.  With
     * zero-length codes AND an incomplete set (y dummy codes), p walks past the
     * x[g] filled words into stale v[] words - in the original the previous
     * huft_build call's values left in the same stack slot - and uses them as
     * symbols (unchecked index into d[] / e[]).  C cannot reproduce that stale
     * content: those v[] words are uninitialised here.  Nor the reads it
     * leads to: e.g. a stale lit/len value (up to 287) used as a distance
     * symbol indexes far past cpdist/cpdext, into border[] and the .eh_frame
     * that follow them in .data.  Only the dummy (never-decoded) entries of
     * such a table are affected, e.g. in the valid one-distance-code case.) */

    /* Generate the Huffman codes and for each, make the table entries */
    k = mn;                     /* shortest code length first */
    x[0] = 0;                   /* first Huffman code is zero */
    p = v;                      /* grab values in bit order */
    h = (u32)-1;                /* no tables yet--level -1 */
    w = -l;                     /* bits decoded == (l * h) */
    u[0] = NULL;
    i = 0;
    q = NULL;
    z = 0;

    /* go through the bit lengths (k already is bits in shortest code) */
    for (; k <= (u32)g; k++) {
        a = c[k];
        while (a--) {
            /* here i is the Huffman code of length k bits for value *p */
            /* make tables up to required level */
            while (k > w + l) {
                h++;
                w += l;         /* previous table always l bits */

                /* compute minimum size table less than or equal to l bits */
                z = (z = g - w) > l ? l : z;        /* upper limit on table size */
                if ((f = 1U << (j = k - w)) > a + 1) {  /* try a k-w bit table */
                    f -= a + 1;                     /* deduct codes from patterns left */
                    xp = c + k;
                    while (++j < z) {               /* try smaller tables up to z bits */
                        u32 cnt = *++xp;
                        if ((f <<= 1) <= cnt)
                            break;                  /* enough codes to use up j bits */
                        f -= cnt;                   /* else deduct codes from patterns */
                    }
                }
                z = 1U << j;    /* table entries for j-bit table */

                /* allocate (bump) and link in new table.  Store order as in
                 * the original (0x01011BD4 *tp, 0x01011BE8 heapPtr, 0x01011BF4
                 * u[h], 0x01011BFC *tp): it matters once the arena has run
                 * into the state fields, e.g. when the previous link word tp
                 * is &heapPtr itself. */
                q = heapPtr;
                *tp = q + 1;                /* link to list (vestige of huft_free) */
                heapPtr = q + z + 1;
                tp = &q->t;
                u[h] = ++q;                 /* table starts after link */
                *tp = NULL;

                /* connect to last table, if there is one */
                if (h) {
                    r.b = (u8)l;            /* bits to dump before this table */
                    r.e = (u8)(16 + j);     /* bits in this table */
                    x[h] = i;               /* save pattern for backing up */
                    r.t = q;                /* pointer to this table */
                    r.n = 0;
                    j = i >> (w - l);
                    u[h - 1][j] = r;        /* connect to last table */
                }
            }

            /* set up table entry in r */
            r.b = (u8)(k - w);
            if (p >= v + n) {
                r.e = 99;                   /* out of values--invalid code; r.t / r.n
                                               keep whatever they last held (in the
                                               original, possibly the previous call's
                                               r in the same stack slot), and
                                               inflate_codes follows r.t as a link */
            } else if (*p < s) {
                r.e = (u8)(*p < 256 ? 16 : 15);     /* 256 is end-of-block code */
                r.n = (u16)*p;              /* simple code is just the value */
                p++;
            } else {
                r.e = (u8)e[*p - s];        /* non-simple--look up in lists (low byte) */
                r.n = d[*p++ - s];
            }

            /* fill code-like entries with r */
            f = 1U << (k - w);
            for (j = i >> w; j < z; j += f)
                q[j] = r;

            /* backwards increment the k-bit code i */
            for (j = 1U << (k - 1); i & j; j >>= 1)
                i ^= j;
            i ^= j;

            /* backup over finished tables */
            while ((i & ((1U << w) - 1)) != x[h]) {
                h--;                        /* don't need to update q */
                w -= l;
            }
        }
    }
    tp = NULL;                  /* (dead store, see above) */

    /* Return true (1) if we were given an incomplete table */
    return y != 0 && g != 1;
}

// FUNC 0x01011E40 GZIP::InflateBase::inflate_codes(void)
/* Decode one Huffman-coded block with tl/bl and td/bd straight into the output
 * (gzip-1.2.4 inflate_codes without the window flush).  Quirks:
 *   - bl/bd are overwritten with the masks (1<<bl)-1 / (1<<bd)-1;
 *   - no `e == 99 -> return 1` check: an invalid code is followed as a link;
 *   - LZ copy is a byte-wise do/while from op - dist (overlap-safe).
 * Writes the state (out, bitbuf, bitcount, inNext, inEnd) back only at the
 * end-of-block code.  Caller: inflate_main.
 * Code shape: the table-link loops are written with goto (link part placed
 * before the lookup, as in the binary), the outer loop is a for (;;); this
 * is what lets the compiler hoist the single constant 1 used by every
 * (1 << n) - 1 mask out of the whole decode loop, as the original did.
 * e - 16 (xe) is computed before the `e > 16` test and the length's mask
 * before the end-of-block test: the binary has both in branch delay slots,
 * i.e. ahead of the branch. */
void InflateBase::inflate_codes(void)
{
    u32 e;              /* table entry flag/number of extra bits */
    u32 n, dist;        /* length and distance for copy */
    Huft *t;            /* pointer to table entry */
    u32 x;              /* table index */
    u32 xe;             /* e - 16: index bits of a linked sub-table */
    Huft *ltab = tl;    /* kept in a stack slot */
    Huft *dtab = td;
    u32 md, ml;         /* masks for bd and bl bits */
    u64 b = bitbuf;     /* bit buffer */
    s32 k = bitcount;   /* bits in bit buffer - 32 */
    const u32 *next = inNext;
    const u32 *end = inEnd;
    u8 *op = out;

    ml = bl;
    md = bd;
    ml = (1U << ml) - 1;
    md = (1U << md) - 1;
    bl = ml;            /* quirk: masks stored over the bit counts */
    bd = md;

    /* (dead store: the first table index is computed ahead of the loop, as
     * in the original, where (u32)b is then available on loop entry) */
    t = ltab + (ml & (u32)b);
    for (;;) {                  /* do until end of block */
        /* literal/length code: look up ml bits, follow links (e > 16) */
        t = ltab;
        x = ml & (u32)b;
        goto lit_lookup;
    lit_link:
        x = (u32)b;
        t = t->t;
        x &= GZ_MASK(xe);       /* no e == 99 check: followed as a link */
    lit_lookup:
        t += x;
        e = t->e;
        GZ_DUMPBITS(t->b);
        xe = e - 16;            /* sub-table index bits, if this is a link */
        if (e > 16)
            goto lit_link;
        n = t->n;
        if (e == 16) {          /* then it's a literal */
            *op++ = (u8)n;
            continue;
        }
        {
            /* the extra-bits mask is computed before the end-of-block test
             * (the binary fills both branch delay slots with it) */
            u32 m = GZ_MASK(e);

            if (e == 15)        /* exit if end of block */
                break;

            /* get length of block to copy */
            n += m & (u32)b;
        }
        GZ_DUMPBITS(e);

        /* decode distance of block to copy (same lookup on td/md) */
        t = dtab;
        x = md & (u32)b;
        goto dist_lookup;
    dist_link:
        x = (u32)b;
        t = t->t;
        x &= GZ_MASK(xe);
    dist_lookup:
        t += x;
        e = t->e;
        GZ_DUMPBITS(t->b);
        xe = e - 16;
        if (e > 16)
            goto dist_link;
        {
            u32 base = t->n;    /* (read before the possible refill call) */
            u32 xb = (u32)b & GZ_MASK(e);
            GZ_DUMPBITS(e);
            dist = base + xb;
        }

        /* do the copy (at least one byte) */
        {
            const u8 *src = op - dist;
            const u8 *stop = src + n;
            do {
                *op++ = *src++;
            } while (src != stop);
        }
    }

    /* restore the globals from the locals (in member order) */
    bitbuf = b;
    bitcount = k;
    inNext = next;
    inEnd = end;
    out = op;
}

// FUNC 0x01012180 GZIP::InflateBase::inflate_main(bool)
/* The whole decompressor: gzip-1.2.4 inflate() with inflate_block,
 * inflate_stored, inflate_fixed and inflate_dynamic inlined.
 *   raw == false: skip a gzip header first - 16 bits ID1/ID2 and 8 bits CM
 *     (unchecked), read FLG, skip MTIME/XFL/OS (6 bytes), and if FLG.FNAME (8)
 *     skip a NUL-terminated name.  FEXTRA/FCOMMENT/FHCRC are NOT handled.
 *   Then blocks until BFINAL: type 0 stored (align, LEN, NLEN unchecked, copy),
 *   1 fixed, 2 dynamic, 3 silently ignored.
 * Tables: heap_clear (inlined) before the code-length tree and again before the
 * lit/dist trees of a dynamic block, and before a fixed block.  huft_build
 * results are ignored.  State is written back to the object only around the
 * inflate_codes call and in the refill hook; on return nothing is written back
 * (this->out etc. are stale after a trailing stored block).
 * Stack: ll[316] @sp+0x10 (lengths; fixed lit/len too), dl[30] @sp+0x500
 * (fixed distance lengths), 9th huft_build arg @sp+0.
 * ll[] is unchecked: nl + nd can be 320 and a 16/17/18 repeat can run up to
 * 137 past n, so i reaches 456.  In the original frame (0x670) that runs over
 * dl[], two padding words (ll[346..347]), then the spilled locals that the
 * loop re-reads every iteration (tl copy ll[348], mask ll[349], nd/nl/last
 * ll[350..352], n ll[354], &tl/&td/&bl/&bd ll[355..358], ll base ll[360]), the
 * saved s0..s7/fp/ra (ra = ll[404]) and 0xC0 bytes of the caller's frame - all
 * with values 0..15.  The C below has the same unchecked stores; it models the
 * ll -> dl -> padding part (one array, indices 0..347) but not the aliasing
 * with the spilled locals beyond it (index >= 348): a stack smash that C
 * cannot express (and the retail compiler's spill slots for n, &tl, the dl
 * base and the ll base, 0x598..0x5B0, are not yet in the original order).
 * (The very first refill site, in the 16-bit ID skip, does not store the output
 * cursor before the hook - it is unchanged at that point, so the shared macro
 * storing it is equivalent.)
 * Code shape: one counter i serves the bit-length loops, the code-length loop,
 * the stored-block count and the fixed-table loops (one register in the
 * binary); field reads go through GZ_GETBITS (value taken, then the bits
 * dropped, then e.g. "+ 257" applied); the code-length loop is written with a
 * goto (same rotated loop as a while, but it gives the register allocator the
 * original weights: end in s6, the table pointer in s7).  As in gzip, the
 * dynamic and the fixed block each end with their own inflate_codes call
 * (GZ_INFLATE_CODES): the binary has one copy of the call sequence, which the
 * compiler's cross-jumping makes from two identical tails (the dynamic block
 * jumps into the fixed block's last huft_build call); stored and reserved
 * blocks simply fall through to the loop test.  The dynamic block's tl copy,
 * mask, nd and nl are declared before `last`: the stack slots of the spilled
 * locals follow that order (0x580 tl copy, 0x584 mask, 0x588 nd, 0x58C nl,
 * 0x590 last).
 * Callers: (out,in) ctor (false), inflate() (raw), ImageLoader::decompress (true). */
void InflateBase::inflate_main(bool raw)
{
    u64 b = bitbuf;             /* bit buffer */
    s32 k = bitcount;           /* bits in bit buffer - 32 */
    const u32 *next = inNext;
    const u32 *end = inEnd;
    u8 *op = out;
    /* ll[0..315] @sp+0x10: literal/length + distance code lengths;
     * ll[316..345] @sp+0x500: the fixed block's distance lengths (dl);
     * ll[346..347] @sp+0x578: the frame's padding before the spilled locals
     * (never read in the original).
     * Adjacent in the original frame, so an ll[] overrun lands in dl[], then
     * in the padding, and huft_build(ll + nl, nd) with nl + nd > 316 reads
     * dl[0..3]; one array keeps all that defined (the two padding words cost
     * nothing: the retail compiler's frame is the same).  Beyond index 347
     * see the function comment. */
    u32 ll[316 + 30 + 2];
#define dl (ll + 316)
    Huft *tlp;                  /* dynamic block: tl (code-length tree) */
    u32 mask;                   /* dynamic block: (1 << bl) - 1 */
    s32 nd, nl;                 /* dynamic block: distance / lit-len code counts */
    s32 last;                   /* last block flag */
    s32 type;                   /* block type */
    s32 i;                      /* shared counter / index */

    if (!raw) {
        s32 flg, cnt, c;

        GZ_DUMPBITS(16);        /* ID1, ID2 (not checked) */
        GZ_DUMPBITS(8);         /* CM (not checked) */
        flg = b & 0xFF;         /* FLG */
        GZ_DUMPBITS(8);
        cnt = 6;
        while (cnt--)           /* MTIME[4], XFL, OS */
            GZ_DUMPBITS(8);
        if (flg & 8) {          /* FNAME: skip the zero-terminated file name */
            do {
                c = b & 0xFF;
                GZ_DUMPBITS(8);
            } while (c != 0);
        }
    }

    do {
        last = GZ_GETBITS(1);   /* last block bit */
        type = GZ_GETBITS(2);   /* block type */

        if (type == 2) {
            /* ---- inflate_dynamic ---- */
            s32 j, l, n, nb;
            Huft *td_;

            /* read in table lengths */
            nl = 257 + GZ_GETBITS(5);   /* number of literal/length codes */
            nd = 1 + GZ_GETBITS(5);     /* number of distance codes */
            nb = 4 + GZ_GETBITS(4);     /* number of bit length codes */
            /* (no "nl > 286 || nd > 30 -> bad lengths" check) */

            heapPtr = (Huft *)heap;         /* heap_clear() inlined */

            /* read in bit-length-code lengths */
            for (i = 0; i < nb; i++)
                ll[border[i]] = GZ_GETBITS(3);
            for (; i < 19; i++)
                ll[border[i]] = 0;

            /* build decoding table for trees--single level, 7 bit lookup */
            huft_build(ll, 19, 19, NULL, NULL, &tl, &bl, 7);

            /* read in literal and distance code lengths */
            n = nl + nd;
            tlp = tl;
            mask = (1U << bl) - 1;
            i = l = 0;
            if (i < n) {                /* while (i < n) */
            cl_next:
                td_ = tlp + ((u32)b & mask);
                GZ_DUMPBITS(td_->b);
                j = td_->n;
                if (j < 16) {                   /* length of code in bits (0..15) */
                    ll[i++] = l = j;            /* save last length in l */
                } else if (j == 16) {           /* repeat last length 3 to 6 times */
                    j = 3 + GZ_GETBITS(2);
                    while (j--)                 /* (no "i + j > n" check) */
                        ll[i++] = l;
                } else if (j == 17) {           /* 3 to 10 zero length codes */
                    j = 3 + GZ_GETBITS(3);
                    while (j--)
                        ll[i++] = 0;
                    l = 0;
                } else {                        /* j == 18: 11 to 138 zero length codes */
                    j = 11 + GZ_GETBITS(7);
                    while (j--)
                        ll[i++] = 0;
                    l = 0;
                }
                if (i < n)
                    goto cl_next;
            }

            heapPtr = (Huft *)heap;         /* heap_clear() inlined (frees the bl tree) */

            /* build the decoding tables for literal/length and distance codes */
            huft_build(ll, nl, 257, cplens, cplext, &tl, &bl, 9);
            huft_build(ll + nl, nd, 0, cpdist, cpdext, &td, &bd, 6);

            /* decompress until an end-of-block code */
            GZ_INFLATE_CODES();
        } else if (type == 0) {
            /* ---- inflate_stored ---- */
            /* go to byte boundary */
            GZ_DUMPBITS(k & 7);

            /* get the length and its complement */
            i = GZ_GETBITS(16);
            GZ_DUMPBITS(16);    /* NLEN: not checked */

            /* read and output the compressed data (the store address is
             * taken before a possible refill reload of op) */
            while (i--) {
                u8 *dst = op;
                *dst = GZ_GETBITS(8);
                op++;
            }
        } else if (type == 1) {
            /* ---- inflate_fixed ---- */

            heapPtr = (Huft *)heap;         /* heap_clear() inlined */

            /* set up literal table (the machine code fills 0..143 downwards) */
            for (i = 0; i < 144; i++)
                ll[i] = 8;
            for (; i < 256; i++)
                ll[i] = 9;
            for (; i < 280; i++)
                ll[i] = 7;
            for (; i < 288; i++)        /* make a complete, but wrong code set */
                ll[i] = 8;

            /* distance lengths: separate array, filled (29..0) before either
             * huft_build call */
            for (i = 0; i < 30; i++)    /* make an incomplete code set */
                dl[i] = 5;

            huft_build(ll, 288, 257, cplens, cplext, &tl, &bl, 7);
            huft_build(dl, 30, 0, cpdist, cpdext, &td, &bd, 5);

            /* decompress until an end-of-block code */
            GZ_INFLATE_CODES();
        }
        /* (type 3: reserved, silently skipped) */
    } while (last == 0);
#undef dl
}

// FUNC 0x01012E78 GZIP::InflateBase::fetchInput(void)
/* Virtual input-refill hook (vtable slot 2, 0x0103FD04), called by the bit
 * reader when inNext == inEnd.  Default: does nothing (a derived loader would
 * reload inNext/inEnd here; none exists in the bootstrap). */
void InflateBase::fetchInput(void)
{
}

// FUNC 0x01012E80 GZIP::InflateBase::heap_clear(void)
/* Rewind the Huffman-table arena: heapPtr (+0x3018) = this (== &heap[0] in the
 * original layout).  Exact match for Lupo 2 heap_clear__Q24GZIP11InflateBase.
 * No callers: inflate_main inlines it. */
void InflateBase::heap_clear(void)
{
    heapPtr = (Huft *)heap;
}

// FUNC 0x01012E88 GZIP::InflateBase::setOutputData(void*)
/* outBase (+0x302C) = out cursor (+0x3014) = p.  Lupo 2 setOutputData__Q24GZIP11InflateBasePv.
 * Callers: inflate(), ImageLoader::decompress. */
void InflateBase::setOutputData(void *p)
{
    outBase = out = (u8 *)p;
}

// FUNC 0x01012E98 GZIP::InflateBase::inflate(void*, const void*, unsigned int, bool)
/* initBitStream(in, size); setOutputData(out); inflate_main(raw).
 * Lupo 2 inflate__Q24GZIP11InflateBasePvPCvUib.  No callers in the bootstrap
 * (ImageLoader::decompress inlines the same three calls). */
void InflateBase::inflate(void *out_, const void *in, u32 size, bool raw)
{
    initBitStream(in, size);
    setOutputData(out_);
    inflate_main(raw);
}

// FUNC 0x01012EF8 __tfQ24GZIP11InflateBase (compiler-generated)
//   typeinfo function for GZIP::InflateBase (vtable slot 0, 0x0103FCF4): if the
//   __user_type_info object at 0x01041EB0 (.sbss) is still zero, constructs it
//   with __user_type_info::__user_type_info (0x01019C58) and the name
//   "Q24GZIP11InflateBase" (@0x0103FD08); returns &that object.  Emitted by the
//   compiler for the polymorphic class; not written as C++.

} /* namespace GZIP */
