/* imageloader.h - ImageLoader (core.gt3 loader + RSA/SHA-512 check), its boot
 * sources (Host / Disk / MCard) and the Buffer helper class of the GT3 US
 * bootstrap (BOOTSTRAP_SCUS-97102).
 *
 * Object files, as recovered from the .eh_frame CIE/FDE chains (one CIE per
 * object file; an FDE's pc_begin is the "data xref" the listings show):
 *
 *   (CIEs, re-walked in this pass: main.cxx 0x01026CD8 "eh"; buffer.cxx
 *    0x01027034 no EH; imageloader.cxx 0x01027978 "eh" -> except table
 *    0x01041074; hostsource 0x01027BDC "eh"; disksource 0x01027CE0 "eh";
 *    mcardsource 0x01027DF8 "eh"; host file I/O 0x01033C94 no EH.  Every
 *    except-table entry of this unit is a cleanup that ends in __throw or
 *    terminate - there is no catch handler anywhere in verify / load.)
 *
 *   buffer.cxx       0x01000ED8..0x010010F0  Buffer out-of-line members
 *   main.cxx         0x010006B0..0x01000790  MCardSource_<1,0> / <0,0> __tf and
 *                                            synthesised dtor, instantiated where
 *                                            main uses them (the rest of main.cxx
 *                                            is the "main" unit)
 *   imageloader.cxx  0x010041C0..0x01004BF8  ~Source, decompress, the digest
 *                                            helper, verify, load, the bignum
 *                                            lock hook, then the inline Source
 *                                            members emitted out of line (this
 *                                            object holds ~Source, the key
 *                                            function) and __tf Source.
 *                                            (0x010041C0/0x010041C8,
 *                                            two empty functions at the head of
 *                                            this object, lie in the "filename"
 *                                            unit's address range but are
 *                                            written in src/imageloader.cpp.)
 *   hostsource.cxx   0x01004BF8..0x01004EF0
 *   disksource.cxx   0x01004EF0..0x010053C8
 *   mcardsource.cxx  0x010053C8..0x01005970
 *   host file I/O    0x01005970..0x01005BC0  host_read_file / host_write_file /
 *                                            host_file_size / host_file_exists
 *                                            (the last in the "filename" unit's
 *                                            address list; all four in
 *                                            src/imageloader_source.cpp)
 *
 * core.gt3 container (all multi-byte fields little-endian, read byte by byte):
 *
 *   file  +0  u16 loadSourceFlags   (the boot-source "version key" main compares;
 *                                    read by the getLoadFlags probes)
 *         +2  u32 rawSize           (size of the inflated body)
 *         +6  raw DEFLATE stream    (no zlib/gzip header)
 *   body  +0  u16 modLen, u8 modulus[modLen]      (big-endian bignum)
 *             u16 sigLen, u8 signature[sigLen]    (big-endian bignum)
 *             payload[rawSize - modLen - sigLen - 4]:
 *               u32 nSection, u32 entryPoint,
 *               nSection x { u32 target, u32 size, u8 data[size] }
 *
 * ImageLoader::verify accepts the image iff
 *     signature ^ 66001 mod modulus == SHA-512(payload)   (as integers)
 * where BOTH the modulus and the signature come from the (inflated) file and
 * the digest is compared as a bare 512-bit integer (no PKCS#1 padding).
 *
 * GCC 2.9x layout notes: the Source classes keep the vptr AFTER the data
 * members (Source: +0x00 Buffer, +0x08 FileName_Base*, +0x0C vptr); modern g++
 * puts it at +0.  The sizes still match (Source 0x10 = operator new(0x10) in
 * main for DiskSource, MCardSource 0x18 = operator new(0x18)).  Vtables are
 * arrays of 8-byte entries {s16 delta, s16 index, pfn}, all deltas 0, vptr ->
 * entry 0.  GCC 2.9x destructors take a hidden int flags (bit 0 = operator
 * delete this; 3 = `delete p`, 2 = automatic/member object).
 */
#ifndef BOOT_IMAGELOADER_H
#define BOOT_IMAGELOADER_H

#include "types.h"
#include "libc.h"
#include "filename.h"
#include "bignum.h"

/* ======================================================================== */
/* Buffer - owning {data, size} heap block (buffer.cxx)                     */
/* ======================================================================== */
/* Memory comes from the pdistd allocator (malloc 0x01011390, free 0x01011368,
 * realloc 0x010113C8, memalign 0x01011410).  No vtable, 8 bytes.
 * Embedded in ImageLoader (+0x04, the inflated body) and in every Source
 * (+0x00, the raw file image); MCardSource::open also builds a temporary one
 * over a 64-byte aligned directory-entry block. */
class Buffer {
public:
    /* Inline (never emitted out of line): data = NULL, size = 0.  Seen
     * expanded in main (ImageLoader at sp+0x80) and in the Source ctors. */
    Buffer() : data(NULL), size(0) {}

    /* Inline, seen once (MCardSource::open): data = memalign(align, n),
     * size = n.  (?) The argument order of the original inline ctor is
     * unknown - the only instance is memalign(0x40, 0x40). */
    Buffer(u32 n, u32 align) : data(memalign(align, n)), size(n) {}

    explicit Buffer(u32 n);                 /* 0x01000F20 (no callers) */
    Buffer(const Buffer &o);                /* 0x01000F58 (no callers) */
    ~Buffer();                              /* 0x01000ED8 free(data) */
    Buffer &operator=(const Buffer &o);     /* 0x01000FB8 (no callers) */
    Buffer &adopt(void *p, u32 n);          /* 0x01001020 (no callers) */
    void resize(u32 n);                     /* 0x01001070 */

    void *data;                             /* +0x00 */
    u32   size;                             /* +0x04 */
};
static_assert(sizeof(Buffer) == 8, "Buffer is {data, size}");

/* ======================================================================== */
/* ImageLoader                                                              */
/* ======================================================================== */
/* The loader object main builds on its stack (sp+0x80) around the chosen
 * boot source; no vtable, sizeof 0x24.  main inlines the ctor (stores src,
 * zeroes image.data/size, leaves +0x0C..+0x20 uninitialised) and the dtor
 * (`delete src` through vtable slot 1 with flags 3, then ~Buffer(&image, 2)).
 *
 *   +0x00 Source *src          boot source holding the raw core.gt3 bytes
 *   +0x04 Buffer  image        inflated body (size = header rawSize)
 *   +0x0C const u8 *modulus    -> image.data + 2
 *   +0x10 u32  modLen
 *   +0x14 const u8 *signature  -> image.data + 4 + modLen
 *   +0x18 u32  sigLen
 *   +0x1C const u8 *payload    -> image.data + 4 + modLen + sigLen (hashed)
 *   +0x20 u32  payloadSize     = image.size - (modLen + sigLen + 4)
 */
class ImageLoader {
public:
    /* -------------------------------------------------------------------- */
    /* Source - abstract boot source (RTTI "Q211ImageLoader6Source")         */
    /* -------------------------------------------------------------------- */
    /* vtable _vt$Q211ImageLoader6Source @0x0103E258 (.rodata):
     *   [0] +0x04 __tfQ211ImageLoader6Source   0x01004BB8
     *   [1] +0x0C ~Source(int)                 0x010041D0
     *   [2] +0x14 close()                      __pure_virtual 0x010180D0
     *   [3] +0x1C open(const char *)           __pure_virtual 0x010180D0
     * Typeinfo object: __user_type_info @0x01041E98 (.sbss, gp-0x7E58).
     *
     * main's source selector (0x010001C8) calls open(name) (slot 3) on the
     * best-scoring source and close() (slot 2) when open fails.
     *
     * The inline members below are emitted out of line by imageloader.cxx,
     * the object that holds the key function ~Source (GCC 2.9x emits every
     * inline member of a class in the object that emits its vtable), in
     * REVERSE declaration order: GetPath 0x01004B08 first ... Source()
     * 0x01004B98 last.  The declaration order here reproduces the retail
     * addresses; their FUNC markers are in src/imageloader.cpp.  The three
     * `data + size` copies (0x01004B48/58/68) and the three `data` copies
     * (0x01004B78/80/88) are byte-identical: their original names and
     * signatures are unknown (?) - modelled as non-const / const /
     * integer-address variants. */
    class Source {
    public:
        Source() : fileName(NULL) {}                /* 0x01004B98 */
        virtual ~Source();                          /* 0x010041D0 slot 1 */
        virtual void close() = 0;                   /* slot 2 */
        virtual bool open(const char *name) = 0;    /* slot 3 */

        /* No callers of the out-of-line copies; main inlines GetPath(). */
        u32 GetSize() const          { return buffer.size; }                      /* 0x01004B90 */
        u32 GetDataAddr() const      { return (u32)buffer.data; }                 /* 0x01004B88 */
        const u8 *GetData() const    { return (const u8 *)buffer.data; }          /* 0x01004B80 */
        u8 *GetData()                { return (u8 *)buffer.data; }                /* 0x01004B78 */
        u32 GetDataEndAddr() const   { return (u32)buffer.data + buffer.size; }   /* 0x01004B68 */
        const u8 *GetDataEnd() const { return (const u8 *)buffer.data + buffer.size; } /* 0x01004B58 */
        u8 *GetDataEnd()             { return (u8 *)buffer.data + buffer.size; }  /* 0x01004B48 */
        /* fileName->name (the inline FileName_Base::GetName expanded).  No
         * NULL check. */
        const char *GetName()        { return fileName->name; }                   /* 0x01004B38 */
        /* fileName->GetPath() through the FileName vtable (slot 2: lh delta
         * @vt+0x10, lw pfn @vt+0x14).  No NULL check.  main inlines it for
         * argv[0] of ExecPS2. */
        const char *GetPath()        { return fileName->GetPath(); }              /* 0x01004B08 */

        Buffer         buffer;      /* +0x00 raw file image (whole core.gt3) */
        FileName_Base *fileName;    /* +0x08 set by a successful open()      */
        /* +0x0C vptr (GCC 2.9x) */
    };

    /* The concrete sources declare no destructor: GCC 2.9x synthesises one
     * (compiler-generated, weak, emitted next to the vtable) that calls
     * ~Source (0x010041D0) with the in-charge flags unchanged. */

    /* -------------------------------------------------------------------- */
    /* HostSource - "host:/tmp/<name>" via SIF fileio (hostsource.cxx)       */
    /* -------------------------------------------------------------------- */
    /* vtable @0x0103E290: [0] __tf 0x01004E80, [1] dtor 0x01004ED0,
     * [2] close 0x01004C18, [3] open 0x01004C60.  Typeinfo __si_type_info
     * @0x0104DD78.  Nothing in this build creates a HostSource (main's HOST
     * record is never probed); getLoadFlags has no callers. */
    class HostSource : public Source {
    public:
        HostSource();                                       /* 0x01004BF8 */
        virtual void close();                               /* 0x01004C18 */
        virtual bool open(const char *name);                /* 0x01004C60 */
        static u16 getLoadFlags(const char *name);          /* 0x01004DB0 */
    };

    /* -------------------------------------------------------------------- */
    /* DiskSource - "cdrom0:\NAME;1" via SIF fileio (disksource.cxx)         */
    /* -------------------------------------------------------------------- */
    /* vtable @0x0103E2D0: [0] __tf 0x01005358, [1] dtor 0x010053A8,
     * [2] close 0x01004F10, [3] open 0x01004F58.  Typeinfo __si_type_info
     * @0x0104DD88.  main: new DiskSource (operator new(0x10), ctor 0x01004EF0). */
    class DiskSource : public Source {
    public:
        DiskSource();                                       /* 0x01004EF0 */
        virtual void close();                               /* 0x01004F10 */
        virtual bool open(const char *name);                /* 0x01004F58 */
        static u16 getLoadFlags(const char *name);          /* 0x010051F8 */
    };

    /* -------------------------------------------------------------------- */
    /* MCardSource - "/BASCUS-97102GAMEDATA/<name>" on a memory card         */
    /* -------------------------------------------------------------------- */
    /* vtable @0x0103E310: [0] __tf 0x01005900, [1] dtor 0x01005950,
     * [2] close 0x01005450 (empty), [3] open 0x01005458.  Typeinfo
     * __si_type_info @0x0104DD98.  sizeof 0x18. */
    class MCardSource : public Source {
    public:
        MCardSource(int port, int slot);                    /* 0x01005428 */
        virtual void close();                               /* 0x01005450 */
        virtual bool open(const char *name);                /* 0x01005458 */
        /* Returns int, not u16 like the Host/Disk probes: main masks this
         * result with `andi 0xffff` (0x01000264, 0x010002D4) but trusts
         * DiskSource::getLoadFlags' u16 to be zero-extended already.  The
         * value is 0..0xFFFF on every path. */
        static int getLoadFlags(const char *name, int port, int slot); /* 0x01005748 */

        int port;                   /* +0x10 */
        int slot;                   /* +0x14 */
    };

    /* -------------------------------------------------------------------- */
    /* MCardSource_<PORT, SLOT>: RTTI "Q211ImageLoadert12MCardSource_2Ui1Ui0"
     * @0x0103DE50 and "...Ui0Ui0" @0x0103DE78 - the template is named
     * `MCardSource_` (12 characters); the "2" is its argument count.         */
    /* -------------------------------------------------------------------- */
    /* Only adds the fixed port/slot.  main creates <0,0> (MCARD 0) and <1,0>
     * (MCARD 1): operator new(0x18), MCardSource::MCardSource(p, 0, 0 / 1, 0)
     * (0x01005428), then vptr = 0x0103DE30 / 0x0103DE10 (inline ctor).
     *   <1,0> vtable 0x0103DE10: [0] __tf 0x010006B0, [1] dtor 0x01000700,
     *                            [2] MCardSource::close, [3] MCardSource::open
     *   <0,0> vtable 0x0103DE30: [0] __tf 0x01000720, [1] dtor 0x01000770, [2], [3] same
     * Typeinfo objects (__si_type_info, base __ti MCardSource @0x0104DD98):
     *   <1,0> @0x0104DD28, <0,0> @0x0104DD38.
     * The implicit instances (vtable, __tf, synthesised dtor) live in main.cxx
     * (0x010006B0..0x01000790); see src/imageloader.cpp. */
    template <unsigned int PORT, unsigned int SLOT>
    class MCardSource_ : public MCardSource {
    public:
        MCardSource_() : MCardSource(PORT, SLOT) {}
    };

    /* -------------------------------------------------------------------- */
    /* ImageLoader itself                                                    */
    /* -------------------------------------------------------------------- */
    explicit ImageLoader(Source *s) : src(s) {}     /* inline (main) */
    ~ImageLoader() { delete src; }                  /* inline (main) */

    void decompress();                              /* 0x01004248 */
    bool verify();                                  /* 0x010044A8 */
    u32  load();                                    /* 0x010049C8 returns entry point, 0 = rejected */

    Source   *src;           /* +0x00 */
    Buffer    image;         /* +0x04 */
    const u8 *modulus;       /* +0x0C */
    u32       modLen;        /* +0x10 */
    const u8 *signature;     /* +0x14 */
    u32       sigLen;        /* +0x18 */
    const u8 *payload;       /* +0x1C */
    u32       payloadSize;   /* +0x20 */
};

static_assert(sizeof(ImageLoader) == 0x24, "ImageLoader is 0x24 bytes");
static_assert(offsetof(ImageLoader, image) == 0x04, "layout");
static_assert(offsetof(ImageLoader, modulus) == 0x0C, "layout");
static_assert(offsetof(ImageLoader, signature) == 0x14, "layout");
static_assert(offsetof(ImageLoader, payload) == 0x1C, "layout");
static_assert(offsetof(ImageLoader, payloadSize) == 0x20, "layout");
static_assert(sizeof(ImageLoader::Source) == 0x10, "Source: operator new(0x10) for DiskSource");
static_assert(sizeof(ImageLoader::DiskSource) == 0x10, "DiskSource: operator new(0x10)");
static_assert(sizeof(ImageLoader::MCardSource) == 0x18, "MCardSource: operator new(0x18)");
/* These two also instantiate the class templates.  egcs emits the implicit
 * instances (vtable, typeinfo function and name, synthesised destructor) of
 * main.cpp in the reverse order of instantiation, so <0,0> is instantiated
 * first here: main.cxx's retail .rodata has the <1,0> vtable and name first
 * (0x0103DE10 / 0x0103DE50, then <0,0> at 0x0103DE30 / 0x0103DE78), and its
 * .text the <1,0> functions first (0x010006B0, then 0x01000720). */
static_assert(sizeof(ImageLoader::MCardSource_<0u, 0u>) == 0x18, "MCardSource_: operator new(0x18)");
static_assert(sizeof(ImageLoader::MCardSource_<1u, 0u>) == 0x18, "MCardSource_: operator new(0x18)");

/* ======================================================================== */
/* Free functions                                                           */
/* ======================================================================== */

/* 0x010043A8 (imageloader.cxx): SHA-512 of data[0..len) as a BigNum (the 64
 * digest bytes imported big-endian).  Only caller: ImageLoader::verify. */
BigNum sha512_digest_to_bignum(const u8 *data, u32 len);

/* 0x01004B00 bignum_lock_stub(void *obj, int unlock) is declared in bignum.h
 * and defined in src/imageloader.cpp (it sits in imageloader.cxx). */

/* 0x010053C8 (mcardsource.cxx): true iff sceMcGetInfo on (port, slot)
 * returns 0 or -1 (card present), type == sceMcTypePS2 and format != 0. */
bool mcard_is_usable(int port, int slot);

/* host: file helpers (0x01005970..0x01005BC0).  `path` gets "host:"
 * prepended in a 128-byte stack buffer (no length check). */

/* Read `len` bytes (0 = up to EOF; clamped to size - offset) from `offset`.
 * buf == NULL: malloc(len).  Returns buf iff exactly len bytes were read,
 * else NULL (a buffer malloc'ed here then leaks). */
void *host_read_file(const char *path, void *buf, u32 offset, u32 len);  /* 0x01005970 */

/* Create/truncate and write len bytes; returns false only if open fails
 * (the write result is ignored).  No callers. */
bool host_write_file(const char *path, const void *buf, u32 len);        /* 0x01005A78 */

/* File size via host_get_file_size (open + lseek END), 0 if it cannot be
 * opened. */
u32 host_file_size(const char *path);                                    /* 0x01005B08 */

/* True iff "host:" + name can be opened (host_get_file_size() >= 0).  name
 * is normally HostFileName::GetPath() ("/tmp/<file>").  Caller:
 * HostSource::getLoadFlags.  (Formerly declared in filename.h.) */
bool host_file_exists(const char *name);                                 /* 0x01005B68 */

#endif /* BOOT_IMAGELOADER_H */
