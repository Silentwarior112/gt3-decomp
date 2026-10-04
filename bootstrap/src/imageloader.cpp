/* imageloader.cpp - core.gt3 loading and signature check of the GT3 US
 * bootstrap (BOOTSTRAP_SCUS-97102): buffer.cxx, imageloader.cxx and the
 * MCardSource_<> template instances emitted in main.cxx.
 * (The three boot sources and the host: file helpers are in
 * src/imageloader_source.cpp.)
 *
 * Boot flow through this file: main (0x01000538) picks a Source, builds
 * `ImageLoader loader(src)` on its stack and calls loader.load():
 *
 *   load()        gfx_vsync(0); if (!verify()) return 0; copy the sections;
 *                 return entryPoint.  main spins forever on 0.
 *   verify()      decompress(); then
 *                   modpow(signature, 66001, modulus) == SHA-512(payload)
 *   decompress()  inflate the raw DEFLATE body into loader.image and slice
 *                 it into modulus / signature / payload.
 *
 * ==== The core.gt3 signature check (ImageLoader::verify, 0x010044A8) ====
 * - The RSA modulus is the byte string at body+2 (u16 length at body+0),
 *   taken FROM THE FILE.  verify never compares it (or a hash of it) with
 *   anything baked into the bootstrap; there is no second modulus, no key
 *   table and no constant bignum anywhere in the verify path.
 * - The exponent is the immediate 66001 (0x101D1, lui 1 / ori 0x1D1 at
 *   0x01004554/0x0100455C).
 * - The signature is the byte string after the modulus (u16 length).
 * - The digest is plain SHA-512 of payload[0..payloadSize), imported as a
 *   64-byte big-endian integer.  The comparison is full signed bignum
 *   equality between sig^66001 mod n and that integer: no PKCS#1 / EMSA
 *   padding, no length checks on modLen / sigLen.
 * - Not covered by the hash: the container header (u16 loadSourceFlags,
 *   u32 rawSize), the modulus and the signature.  The payload (section
 *   count, entry point, section targets and data) is covered.
 * Consequence: the check gives integrity against accidental damage only.
 * Anyone can produce an accepted image with a key of their own choosing
 * (any modulus n > digest for which they know a 66001-th root, e.g. a prime
 * n with gcd(66001, n-1) = 1 and sig = digest^d mod n).  With the Montgomery
 * code requiring only an odd modulus, even n = digest + 1 with
 * sig = digest works whenever the digest is even ((-1)^66001 = -1 = digest
 * mod n).  (Derived from the code; not run on hardware.)
 * - decompress() runs BEFORE any check, on unauthenticated data, with no
 *   output bound (see gzip.h), and the section copy in load() writes to
 *   arbitrary addresses taken from the (verified) payload.
 */
#include "types.h"
#include "libc.h"
#include "imageloader.h"
#include "sha512.h"
#include "bignum.h"
#include "montgomery.h"   /* modpow (0x0100B238) */
#include "gzip.h"         /* GZIP::InflateBase */
#include "gs_driver.h"    /* gfx_vsync (0x0100D9A8) */

/* Little-endian loads from unaligned bytes.  The original code does these
 * byte by byte (lbu/sll/or), i.e. through inline helpers or macros. */
static inline u32 get_le16(const u8 *p)
{
    return p[0] | (p[1] << 8);
}

static inline u32 get_le32(const u8 *p)
{
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

/* ======================================================================== */
/* buffer.cxx (0x01000ED8..0x010010F0, own object file, no EH regions)      */
/* ======================================================================== */

// FUNC 0x01000ED8 Buffer::~Buffer
//   free(data) (0x01011368, NULL-safe); GCC 2.9x flags & 1 -> operator
//   delete(this) (0x01019910).  Callers: main (loader.image, flags 2),
//   ImageLoader::Source::~Source (flags 2), MCardSource::open (flags 2).
Buffer::~Buffer()
{
    free(data);
}

// FUNC 0x01000F20 Buffer::Buffer(u32)
//   data = NULL (dead store), size = n, data = malloc(n).  Returns this.
//   No callers.
Buffer::Buffer(u32 n)
    : data(NULL), size(n)
{
    data = malloc(n);
}

// FUNC 0x01000F58 Buffer::Buffer(const Buffer &)
//   data = NULL, size = o.size; if o.data: data = malloc(o.size) and copy
//   `size` bytes.  No callers.
Buffer::Buffer(const Buffer &o)
    : data(NULL), size(o.size)
{
    if (o.data) {
        data = malloc(o.size);
        memcpy(data, o.data, size);
    }
}

// FUNC 0x01000FB8 Buffer::operator=(const Buffer &)
//   p = realloc(data, o.size) (0x010113C8); memcpy(p, o.data, o.size);
//   data = p; size = o.size.  No self-assignment check: `b = b` reads from
//   the old (possibly freed) block when realloc moves it.  No callers.
Buffer &Buffer::operator=(const Buffer &o)
{
    void *p = realloc(data, o.size);
    memcpy(p, o.data, o.size);
    data = p;
    size = o.size;
    return *this;
}

// FUNC 0x01001020 Buffer::adopt
//   free(data), then take ownership of (p, n).  Returns *this.  No callers.
Buffer &Buffer::adopt(void *p, u32 n)
{
    free(data);
    data = p;
    size = n;
    return *this;
}

// FUNC 0x01001070 Buffer::resize
//   New block of n bytes; copies the OLD `size` bytes into it (QUIRK: when
//   shrinking, this overruns the new block by size - n bytes), frees the old
//   block, zero-fills the grown tail (unsigned size < n), then data = new
//   block, size = n (the two stores are emitted size first).  Callers:
//   ImageLoader::decompress and the three Source::open implementations - all
//   on empty buffers in practice.
void Buffer::resize(u32 n)
{
    void *p = malloc(n);
    memcpy(p, data, size);
    free(data);
    if (size < n)
        memset((u8 *)p + size, 0, n - size);
    data = p;
    size = n;
}

/* The MCardSource_<> template instances (0x010006B0..0x0100078F: typeinfo
 * functions and synthesised destructors) are emitted by main.cpp, which
 * instantiates them with `new` as main.cxx did; see the end of
 * src/main.cpp. */

/* ======================================================================== */
/* imageloader.cxx (0x010041C0..0x01004BF8)                                 */
/* ======================================================================== */

// FUNC 0x010041C0 F_010041C0
//   Empty function (jr ra; nop).  No callers; the only data reference is its
//   FDE (pc_begin word 0x0102799C), the FIRST FDE under imageloader.cxx's "eh"
//   CIE (.data 0x01027978) - so it heads this object, not filename.cxx
//   (moved here from src/filename.cpp at integration).  Identity unknown
//   /* ? */ (an empty inline or compiler-emitted function of this TU).
void F_010041C0(void)
{
}

// FUNC 0x010041C8 F_010041C8
//   Empty function (jr ra; nop), same situation: second FDE (pc_begin word
//   0x010279AC) of imageloader.cxx.  Identity unknown /* ? */.
void F_010041C8(void)
{
}

/* Three-way compare of two BigNum handles, inline in the original (expanded
 * in verify; the montgomery unit's file-local bn_compare is the same code):
 * a zero `a` (NULL rep or nwords == 0) gives -sign(b), a zero `b` gives
 * sign(a), otherwise the signed BigNumRep::cmp (0x01007408).
 * The retail code computes the sign in two inline levels (the rep's sign in
 * its own register, then copied: `move $v0, $a0`), and passes b's rep to
 * cmp from the register that tested it - hence these helpers. */
static inline int bn_rep_sign(const BigNumRep *r)
{
    return r->nwords ? (r->negative ? -1 : 1) : 0;
}

static inline int bn_sign(const BigNum &a)
{
    return a.rep ? bn_rep_sign(a.rep) : 0;
}

static inline int bn_compare(const BigNum &a, const BigNum &b)
{
    if (a.isZero())
        return -bn_sign(b);
    if (b.isZero())
        return bn_sign(a);
    const BigNumRep &br = *b.rep;
    return a.rep->cmp(br);
}

/* SHA-512 context set-up, an inline helper (or constructor) in the
 * original: the context address is recomputed for sha512_reset. */
static inline void sha512_ctx_init(SHA512Ctx *ctx)
{
    sha512_init_state(ctx->state);
    sha512_reset(ctx);
}

// FUNC 0x010041D0 ImageLoader::Source::~Source
//   vptr = 0x0103E258; `delete fileName` (NULL-checked, virtual dtor slot 1
//   with flags 3); ~Buffer(&buffer, 2) (0x01000ED8); flags & 1 -> operator
//   delete(this).  All derived dtors (Host/Disk/MCard, MCardSource_<>)
//   forward here with their flags unchanged.
ImageLoader::Source::~Source()
{
    delete fileName;
}

// FUNC 0x01004248 ImageLoader::decompress
//   Once only (skipped while image.size != 0):
//     image.resize(rawSize)  with rawSize = LE32 at file+2 (the Source data);
//     GZIP::InflateBase on the stack (sp+0; ctor 0x01011750, dtor 0x010117C8
//     with flags 2), raw DEFLATE from file+6 (length size-6) straight into
//     image.data, inflate_main(true).  The inflated byte count is not
//     checked against rawSize; resize zero-filled the whole buffer, so a
//     short stream leaves zeros and a long one overruns the heap block.
//   Then slices the body (u16 lengths are unsigned, little-endian):
//     modulus   = body + 2,                 modLen  = LE16(body)
//     signature = body + 4 + modLen,        sigLen  = LE16(body + 2 + modLen)
//     payload   = body + 4 + modLen + sigLen,
//     payloadSize = image.size - (modLen + sigLen + 4)   (no underflow check)
//   The parse runs while the inflater is still in scope (its dtor follows).
//   EH: cleanup-only landing pads (~InflateBase(2) then __throw) = RAII.
//   (`img` is how the original reaches image.size: &image lives in $s2.)
void ImageLoader::decompress()
{
    Buffer &img = image;
    if (img.size != 0)
        return;

    img.resize(get_le32((const u8 *)src->buffer.data + 2));

    GZIP::InflateBase inf;
    u8 *out = (u8 *)image.data;     /* read before initBitStream */
    inf.initBitStream(src->GetData() + 6, src->GetSize() - 6);
    inf.setOutputData(out);
    inf.inflate_main(true);

    const u8 *body = (const u8 *)image.data;
    modLen      = get_le16(body);
    modulus     = body + 2;
    sigLen      = get_le16(&body[modLen + 2]);
    signature   = &modulus[modLen + 2];
    payloadSize = img.size - (modLen + 4 + sigLen);
    payload     = &body[modLen + 4 + sigLen];
}

// FUNC 0x010043A8 sha512_digest_to_bignum
//   Returns (hidden return slot $a0) SHA-512(data[0..len)) as a BigNum:
//     stack: u64 digest[8] @sp+0x00, SHA512Ctx @sp+0x40, u8 bytes[64] @sp+0x110
//     sha512_init_state(&ctx.state) + sha512_reset(&ctx)  (inlined ctx ctor?)
//     sha512_update, sha512_final(&ctx, digest)  (8 native u64 words)
//     sha512_store_digest_be(bytes, digest)      (standard 64-byte digest)
//     new BigNumRep(bytes, 0x40) (operator new(0x18) + 0x01007190), handle
//     stores rep then addRef (bignum_lock_stub(rep, 0); ++refcount).
//   Plain SHA-512: no salt, no key, standard IV and constants (sha512 unit).
//   EH: operator delete of the rep if its ctor throws, then __throw.
BigNum sha512_digest_to_bignum(const u8 *data, u32 len)
{
    u64 digest[SHA512_DIGEST_WORDS];
    SHA512Ctx ctx;
    u8 bytes[SHA512_DIGEST_WORDS * 8];

    sha512_ctx_init(&ctx);
    sha512_update(&ctx, data, len);
    sha512_final(&ctx, digest);
    sha512_store_digest_be(bytes, digest);
    return BigNum(new BigNumRep(bytes, 0x40));
}

// FUNC 0x010044A8 ImageLoader::verify
//   THE core.gt3 SIGNATURE CHECK.  Exact sequence:
//     decompress();
//     sig = BigNum(signature, sigLen) = new BigNumRep(signature, sigLen)
//           (0x01007190) + addref                    handle @sp+0x10
//     e   = BigNum(66001)   inline BigNumRep(int): nwords 1, refcount 0,
//           capacity 0, negative 0, words 0, reserve(1, false) (0x01007568),
//           words[0] = negative ? -66001 : 66001 (movn with 0xFFFEFE2F)  @sp+0x20
//     n   = BigNum(modulus, modLen)                  handle @sp+0x30
//     m   = modpow(sig, e, n)   (0x0100B238, hidden return @sp+0x00)
//     release n, e, sig (in that order; they are temporaries of this
//       full-expression - sig's slot sp+0x10 is then reused for h)
//     h   = sha512_digest_to_bignum(payload, payloadSize)   @sp+0x10
//     result = (bn_compare(m, h) == 0)  (sltiu 1)
//     release h, then m; return result.
//   Every handle construction is `if (rep) { bignum_lock_stub(rep, 0);
//   ++rep->refcount; }`, every release `bignum_lock_stub(rep, 1); if
//   (--refcount <= 0) __dt(rep, 3)` (0x01007148).  No other data enters the
//   check: the modulus is the in-file one and is not validated.  The return
//   value of decompress is void and nothing about the inflate is checked.
//   EH: all landing pads are cleanups (operator delete of a half-built rep,
//   handle releases) followed by __throw / terminate = RAII.
bool ImageLoader::verify()
{
    decompress();

    /* GCC 2.9x built the three argument temporaries left to right
     * (signature, exponent, modulus) and destroyed them right after the
     * call. */
    BigNum decrypted = modpow(BigNum(signature, sigLen),
                              BigNum(66001),
                              BigNum(modulus, modLen));
    BigNum digest = sha512_digest_to_bignum(payload, payloadSize);
    return bn_compare(decrypted, digest) == 0;
}

// FUNC 0x010049C8 ImageLoader::load
//   gfx_vsync(0) (0x0100D9A8: with n == 0 it only returns the vblank count,
//   result ignored); if verify() fails return 0.  Otherwise walk the
//   (hash-covered) payload: LE32 nSection, LE32 entryPoint, then nSection
//   records {LE32 target, LE32 size, data[size]}, each memcpy'd to the raw
//   target address (no range checks).  Loop counter compare is unsigned
//   (sltu), guarded by nSection != 0.  Returns entryPoint (main passes it to
//   ExecPS2 via 0x01016F70 with argv {source path, "hot"}).
u32 ImageLoader::load()
{
    gfx_vsync(0);
    if (!verify())
        return 0;

    u32 nSection = get_le32(payload);
    u32 entry    = get_le32(payload + 4);
    const u8 *p  = payload + 8;
    for (u32 i = 0; i < nSection; i++) {
        u32 target = get_le32(p);
        u32 size   = get_le32(p + 4);
        memcpy((void *)target, p + 8, size);
        p += size + 8;
    }
    return entry;
}

// FUNC 0x01004B00 bignum_lock_stub
//   `jr ra; nop`: empty hook called as (rep, 0) before every BigNumRep
//   refcount++ and (rep, 1) before every refcount-- (verify, the digest
//   helper and the bignum / Montgomery code).  A compiled-out lock or debug
//   hook of the bignum library.  Declared in bignum.h.
void bignum_lock_stub(void *obj, int unlock)
{
}

/* ------------------------------------------------------------------------ */
/* Out-of-line copies of the inline ImageLoader::Source members             */
/* ------------------------------------------------------------------------ */
/* Emitted here because this object holds Source's key function (~Source),
 * after the other functions of the file and in reverse declaration order
 * (see the class in include/imageloader.h, where they are defined).  None
 * has a caller.  The overloaded ones are named by their mangled symbol. */

// FUNC 0x01004B08 ImageLoader::Source::GetPath
//   fileName->GetPath() through the FileName vtable (slot 2: lh delta
//   @vt+0x10, lw pfn @vt+0x14).  No NULL check.

// FUNC 0x01004B38 ImageLoader::Source::GetName
//   fileName->name (+0 of the FileName).  No NULL check.

// FUNC 0x01004B48 GetDataEnd__Q211ImageLoader6Source
//   ImageLoader::Source::GetDataEnd(): buffer.data + buffer.size.

// FUNC 0x01004B58 GetDataEnd__CQ211ImageLoader6Source
//   ImageLoader::Source::GetDataEnd() const: same code as 0x01004B48.

// FUNC 0x01004B68 GetDataEndAddr__CQ211ImageLoader6Source
//   ImageLoader::Source::GetDataEndAddr() const: same code as 0x01004B48.

// FUNC 0x01004B78 GetData__Q211ImageLoader6Source
//   ImageLoader::Source::GetData(): buffer.data.

// FUNC 0x01004B80 GetData__CQ211ImageLoader6Source
//   ImageLoader::Source::GetData() const: same code as 0x01004B78.

// FUNC 0x01004B88 GetDataAddr__CQ211ImageLoader6Source
//   ImageLoader::Source::GetDataAddr() const: same code as 0x01004B78.

// FUNC 0x01004B90 ImageLoader::Source::GetSize
//   buffer.size.

// FUNC 0x01004B98 ImageLoader::Source::Source
//   buffer.data = 0, vptr = 0x0103E258, buffer.size = 0, fileName = NULL;
//   returns this.  Expanded inline in the Host/Disk/MCardSource constructors.

// FUNC 0x01004BB8 __tfQ211ImageLoader6Source (compiler-generated)
//   typeinfo function, slot 0 of vtable 0x0103E258.  If the __user_type_info
//   object at 0x01041E98 (.sbss, gp-0x7E58) is not built yet (first word ==
//   0): __rtti_user(&ti, "Q211ImageLoader6Source" (.rodata 0x0103E278))
//   (0x01019C58).  Returns 0x01041E98.  Also called by the Host/Disk/MCard
//   __tf functions for their base.
