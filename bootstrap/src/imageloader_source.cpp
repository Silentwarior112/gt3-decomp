/* imageloader_source.cpp - the three concrete boot sources of the GT3 US
 * bootstrap (BOOTSTRAP_SCUS-97102) and the "host:" file helpers.
 *
 * Object files, recovered from the .eh_frame CIE/FDE chains (one CIE per
 * object file; see include/imageloader.h):
 *
 *   hostsource.cxx   0x01004BF8..0x01004EF0  CIE 0x01027BDC "eh", except table 0x01041154
 *   disksource.cxx   0x01004EF0..0x010053C8  CIE 0x01027CE0 "eh", except table 0x010411C8
 *   mcardsource.cxx  0x010053C8..0x01005970  CIE 0x01027DF8 "eh", except table 0x0104129C
 *   host file I/O    0x01005970..0x01005BC0  CIE 0x01033C94, no EH augmentation
 *                    (host_read_file, host_write_file, host_file_size and
 *                    host_file_exists)
 *
 * Every Source::open follows the same pattern:
 *   XxxFileName fn(name);                   (inline ctor: strdup_alloc)
 *   probe/open the device path fn.GetPath() (direct, non-virtual call)
 *   buffer.resize(file size); read the whole file into buffer.data
 *   delete fileName; fileName = new XxxFileName(fn);  return true
 * and destroys `fn` with FileName_Base::~FileName_Base(flags 2) (0x01003570;
 * the empty derived dtors are inlined away in the original).  All landing
 * pads in these functions are cleanups (file close, ~FileName, operator delete
 * of a half-built FileName) followed by __throw / terminate, i.e. plain RAII.
 * Every failure path returns false (0); nothing is retried here.
 *
 * Callers: main's source selector (0x010001C8) calls DiskSource::getLoadFlags
 * and MCardSource::getLoadFlags (port 0 and 1, slot 0) on "core.gt3", keeps
 * the highest u16 key, creates the winning source and calls open() through
 * vtable slot 3 (close(), slot 2, when open fails).  The HOST record is never
 * probed in this build: nothing creates a HostSource and
 * HostSource::getLoadFlags has no callers.
 */
#include "types.h"
#include "libc.h"
#include "sifdev.h"       /* sceOpen / sceLseek / sceRead / sceClose (disk) */
#include "libmc.h"        /* sceMcTblGetDir, sceMcTypePS2 */
#include "boot_io.h"      /* mc_*_sync, host_* wrappers, HostFileStat */
#include "gs_driver.h"    /* gfx_vsync (0x0100D9A8) */
#include "filename.h"
#include "imageloader.h"

/* ======================================================================== */
/* File-local RAII file handles (inline in the original; never emitted)     */
/* ======================================================================== */
/* A one-word object on the stack (sp+0x10) holding the open result.  Its
 * inlined destructor, seen on every exit path after a successful open:
 *
 *     if (fd >= 0) { if (fd >= 0) close(fd); }   (second test is a redundant
 *     fd = -1;                                     bltzl, never taken)
 *
 * The close result is discarded.  On the "open failed" path the compiler
 * folded the destructor down to the bare `fd = -1` store (sw -1 to sp+0x10).
 * The shape below reproduces that: close()'s guard is a bool helper (a
 * store-flag value, which combine turns back into `bltz` only after CSE), so
 * CSE1 cannot fold it against the destructor's own `fd >= 0` test.  The same
 * break keeps the object's address in an s-register for the cleanups
 * (s0, copied to s1), as in retail.  Written with one plain `fd >= 0` test,
 * both disappear (DiskSource::getLoadFlags 81.9%, open 83.2%; exact now). */
/* ? */ /* member names isOpen / close are inferences */
class DiskFile {                                /* sce fileio ("cdrom0:") */
public:
    explicit DiskFile(int d) : fd(d) {}
    bool isOpen() const { return fd >= 0; }
    void close()
    {
        if (isOpen())
            sceClose(fd);                       /* 0x0101F440 */
    }
    /* after close(): egcs inlines only bodies it has already compiled */
    ~DiskFile()
    {
        if (fd >= 0)
            close();
        fd = -1;
    }
    int fd;                                     /* +0x00 */
};

class MCardFile {                               /* libmc via the io wrappers */
public:
    explicit MCardFile(int d) : fd(d) {}
    bool isOpen() const { return fd >= 0; }
    void close()
    {
        if (isOpen())
            mc_close_sync(fd);                  /* 0x0100F130 */
    }
    /* after close(): egcs inlines only bodies it has already compiled */
    ~MCardFile()
    {
        if (fd >= 0)
            close();
        fd = -1;
    }
    int fd;                                     /* +0x00 */
};

static inline u16 get_le16(const u8 *p)
{
    return (u16)(p[0] | (p[1] << 8));
}

/* ======================================================================== */
/* hostsource.cxx (0x01004BF8..0x01004EF0)                                  */
/* ======================================================================== */

// FUNC 0x01004BF8 ImageLoader::HostSource::HostSource
//   buffer.data = 0, vptr (+0xC) = 0x0103E290, buffer.size = 0, fileName =
//   NULL; returns this.  No callers (nothing creates a HostSource).
ImageLoader::HostSource::HostSource()
{
}

// FUNC 0x01004C18 ImageLoader::HostSource::close
//   Vtable slot 2 (0x0103E2A4).  `delete fileName` (NULL-checked, virtual
//   deleting dtor with flags 3), then fileName = NULL (unconditional store).
void ImageLoader::HostSource::close()
{
    delete fileName;
    fileName = NULL;
}

// FUNC 0x01004C60 ImageLoader::HostSource::open
//   Vtable slot 3 (0x0103E2AC).  HostFileName fn(name) at sp+0 (path
//   "/tmp/<name>", 0x01003BF0).  size = host_file_size(path) (0 if the file
//   cannot be opened - not checked), buffer.resize(size), then
//   host_read_file(path, buffer.data, 0, 0) (len 0 = whole file from offset
//   0).  NULL result -> false.  Success: delete the old fileName (flags 3),
//   fileName = new HostFileName(fn) (operator new(0xC), name re-strdup'ed,
//   path NULL), true.  The file is "host:/tmp/<name>" on the host PC.
bool ImageLoader::HostSource::open(const char *name)
{
    HostFileName fn(name);

    buffer.resize(host_file_size(fn.GetPath()));
    if (host_read_file(fn.GetPath(), buffer.data, 0, 0) == NULL)
        return false;

    delete fileName;
    fileName = new HostFileName(fn);
    return true;
}

// FUNC 0x01004DB0 ImageLoader::HostSource::getLoadFlags
//   Static probe: HostFileName fn(name); if !host_file_exists(fn.GetPath())
//   return 0.  Otherwise host_read_file(path, hdr, 0, 2) into a 2-byte stack
//   buffer (sp+0x10) and return LE16(hdr) = the core.gt3 u16 loadSourceFlags.
//   QUIRK: the host_read_file result is ignored - if the read fails or is
//   short, stack garbage is returned as the flags.  No callers in this build.
u16 ImageLoader::HostSource::getLoadFlags(const char *name)
{
    HostFileName fn(name);
    u8 hdr[2];                                  /* sp+0x10, uninitialised */

    if (!host_file_exists(fn.GetPath()))
        return 0;
    host_read_file(fn.GetPath(), hdr, 0, 2);   /* result ignored */
    return get_le16(hdr);
}

// FUNC 0x01004E80 __tfQ211ImageLoader10HostSource (compiler-generated)
//   typeinfo function, slot 0 of vtable 0x0103E290.  If the __si_type_info
//   object at 0x0104DD78 (.bss) is not built yet (first word == 0): call
//   __tfQ211ImageLoader6Source (0x01004BB8, result ignored), then
//   __rtti_si(0x0104DD78, "Q211ImageLoader10HostSource" (.rodata 0x0103E2B0),
//   &__ti Source @0x01041E98 (gp-0x7E58)) (0x01019C38).  Returns 0x0104DD78.

// FUNC 0x01004ED0 _$_Q211ImageLoader10HostSource (compiler-generated)
//   ImageLoader::HostSource::~HostSource, synthesised (HostSource declares
//   no destructor; emitted weak next to the vtable).  Vtable slot 1:
//   forwards (this, flags) unchanged to ImageLoader::Source::~Source
//   (0x010041D0), which also does the flags & 1 operator delete.

/* ======================================================================== */
/* disksource.cxx (0x01004EF0..0x010053C8)                                  */
/* ======================================================================== */

// FUNC 0x01004EF0 ImageLoader::DiskSource::DiskSource
//   buffer.data = 0, vptr (+0xC) = 0x0103E2D0, buffer.size = 0, fileName =
//   NULL; returns this.  Caller: main's source selector (0x010001C8) after
//   operator new(0x10).
ImageLoader::DiskSource::DiskSource()
{
}

// FUNC 0x01004F10 ImageLoader::DiskSource::close
//   Vtable slot 2 (0x0103E2E4).  Same code as HostSource::close.
void ImageLoader::DiskSource::close()
{
    delete fileName;
    fileName = NULL;
}

// FUNC 0x01004F58 ImageLoader::DiskSource::open
//   Vtable slot 3 (0x0103E2EC).  DiskFileName fn(name) at sp+0 (path
//   "cdrom0:\<NAME>;1", 0x01003710), gfx_vsync(0) (0x0100D9A8; with n == 0
//   it only returns the vblank count - no wait, no side effect, result
//   ignored - so all three gfx_vsync(0) calls here are effectively no-ops),
//   fd = sceOpen(path, SCE_RDONLY) (fd object at sp+0x10).  fd < 0 -> false.  gfx_vsync(0); size = sceLseek(fd, 0, END);
//   size < 0 -> false; sceLseek(fd, 0, SET) (result ignored);
//   buffer.resize(size); gfx_vsync(0); r = sceRead(fd, buffer.data, size);
//   r < 0 or r != size -> false.  Success: delete the old fileName (flags 3),
//   fileName = new DiskFileName(fn), true.  The fd is closed on every path
//   that opened it (after the new DiskFileName on success).
bool ImageLoader::DiskSource::open(const char *name)
{
    DiskFileName fn(name);

    gfx_vsync(0);
    DiskFile f(sceOpen(fn.GetPath(), SCE_RDONLY));
    if (f.fd < 0)
        return false;

    gfx_vsync(0);
    int size = sceLseek(f.fd, 0, SCE_SEEK_END);
    if (size < 0)
        return false;
    sceLseek(f.fd, 0, SCE_SEEK_SET);           /* result ignored */

    buffer.resize(size);
    gfx_vsync(0);
    int r = sceRead(f.fd, buffer.data, size);
    if (r < 0 || r != size)                     /* bltz, then beql r == size */
        return false;

    delete fileName;
    fileName = new DiskFileName(fn);
    return true;
}

// FUNC 0x010051F8 ImageLoader::DiskSource::getLoadFlags
//   Static probe (no gfx_vsync here): DiskFileName fn(name); fd =
//   sceOpen(path, SCE_RDONLY); fd < 0 -> 0.  flags = 0; if sceRead(fd, hdr,
//   2) == 2 (hdr at sp+0x20) flags = LE16(hdr); close; return flags.
//   Caller: main's source selector, name = "core.gt3" -> "cdrom0:\CORE.GT3;1".
u16 ImageLoader::DiskSource::getLoadFlags(const char *name)
{
    DiskFileName fn(name);
    DiskFile f(sceOpen(fn.GetPath(), SCE_RDONLY));
    if (f.fd < 0)
        return 0;

    u16 flags = 0;
    u8 hdr[2];                                  /* sp+0x20 */
    if (sceRead(f.fd, hdr, 2) == 2)
        flags = get_le16(hdr);
    return flags;
}

// FUNC 0x01005358 __tfQ211ImageLoader10DiskSource (compiler-generated)
//   typeinfo function, slot 0 of vtable 0x0103E2D0.  If the __si_type_info
//   object at 0x0104DD88 (.bss) is not built yet (first word == 0): call
//   __tfQ211ImageLoader6Source (0x01004BB8), then __rtti_si(0x0104DD88,
//   "Q211ImageLoader10DiskSource" (.rodata 0x0103E2F0), &__ti Source
//   @0x01041E98) (0x01019C38).  Returns 0x0104DD88.

// FUNC 0x010053A8 _$_Q211ImageLoader10DiskSource (compiler-generated)
//   ImageLoader::DiskSource::~DiskSource, synthesised.  Vtable slot 1:
//   forwards (this, flags) to ImageLoader::Source::~Source.

/* ======================================================================== */
/* mcardsource.cxx (0x010053C8..0x01005970)                                 */
/* ======================================================================== */

// FUNC 0x010053C8 mcard_is_usable
//   type = 0 (sp+0), format = 0 (sp+4); r = mc_getinfo_sync(port, slot,
//   &type, NULL, &format) (0x0100F290, free not requested).  Returns false if
//   r > 0 or r < -1 (signed), i.e. only sceMcGetInfo results 0 (same card)
//   and -1 (new formatted card) pass; then true iff type == sceMcTypePS2 (2)
//   and format != 0 (unsigned test).  Callers: MCardSource::open,
//   MCardSource::getLoadFlags.
bool mcard_is_usable(int port, int slot)
{
    int type = 0;
    int format = 0;
    int r = mc_getinfo_sync(port, slot, &type, NULL, &format);

    if (r > 0)
        return false;
    if (r < -1)
        return false;
    return (type == sceMcTypePS2) && (format != 0);
}

// FUNC 0x01005428 ImageLoader::MCardSource::MCardSource
//   port (+0x10) = port, slot (+0x14) = slot, vptr (+0xC) = 0x0103E310,
//   buffer.data = buffer.size = 0, fileName = NULL; returns this.  Called
//   (out of line) by main's source selector for MCardSource_<0,0> / <1,0>,
//   which then overwrite the vptr with their own vtable.
ImageLoader::MCardSource::MCardSource(int port, int slot)
    : port(port), slot(slot)
{
}

// FUNC 0x01005450 ImageLoader::MCardSource::close
//   Vtable slot 2 of MCardSource (0x0103E324) and of both MCardSource_<>
//   vtables (0x0103DE24, 0x0103DE44).  Empty (`jr ra; nop`): unlike the Host
//   and Disk versions it does NOT delete fileName.  Harmless in practice since
//   open() only sets fileName on success and main calls close() only after a
//   failed open.
void ImageLoader::MCardSource::close()
{
}

// FUNC 0x01005458 ImageLoader::MCardSource::open
//   Vtable slot 3 (0x0103E32C, also 0x0103DE2C / 0x0103DE4C).
//   mcard_is_usable(port, slot) else false.  MCardFileName fn(name) at sp+0
//   (path "/BASCUS-97102GAMEDATA/<name>", 0x01003960).  A temporary Buffer
//   at sp+0x10 {memalign(0x40, 0x40), 0x40} receives one sceMcGetDir entry:
//   mc_getdir_sync(port, slot, path, mode 0, maxent 1, entry) < 0 -> false;
//   size = entry->FileSizeByte (+0x10); the Buffer is freed (flags 2).
//   gfx_vsync(0) (no-op read of the vblank count, as in DiskSource::open);
//   fd = mc_open_sync(port, slot, path, 1 = sceMcFileAttrReadable
//   / O_RDONLY) (fd object at sp+0x10); fd < 0 -> false.  buffer.resize(size);
//   gfx_vsync(0); r = mc_read_sync(fd, buffer.data, buffer.size); r < 0 or
//   r != buffer.size -> false.  Success: delete the old fileName (flags 3),
//   fileName = new MCardFileName(fn), close the card file, true.
//   port/slot are re-read from this for every call.
bool ImageLoader::MCardSource::open(const char *name)
{
    if (!mcard_is_usable(port, slot))
        return false;

    MCardFileName fn(name);
    u32 size;
    {
        Buffer dir(0x40, 0x40);                 /* memalign(0x40, 0x40), size 0x40 */
        /* the entry pointer is taken before fn.GetPath() is called: retail
         * passes the memalign result from its register, no reload */
        sceMcTblGetDir *ent = (sceMcTblGetDir *)dir.data;
        if (mc_getdir_sync(port, slot, fn.GetPath(), 0, 1, ent) < 0)
            return false;
        size = ent->FileSizeByte;
    }

    gfx_vsync(0);
    MCardFile f(mc_open_sync(port, slot, fn.GetPath(), 1));
    if (f.fd < 0)
        return false;

    buffer.resize(size);
    gfx_vsync(0);
    int r = mc_read_sync(f.fd, buffer.data, buffer.size);
    if (r < 0 || r != (int)buffer.size)         /* bltz, then beql r == size */
        return false;

    delete fileName;
    fileName = new MCardFileName(fn);
    return true;
}

// FUNC 0x01005748 ImageLoader::MCardSource::getLoadFlags
//   Static probe: mcard_is_usable(port, slot) else 0.  MCardFileName fn(name);
//   fd = mc_open_sync(port, slot, path, 1); fd < 0 -> 0.  mc_read_sync(fd,
//   hdr, 2) (hdr at sp+0x20) != 2 -> close, 0.  Otherwise close and return
//   LE16(hdr).  Caller: main's source selector with (name "core.gt3", port
//   0 / 1, slot 0).
//   Return type int: main masks this result with `andi 0xffff`
//   (0x01000264, 0x010002D4) but uses DiskSource::getLoadFlags' u16 result
//   unmasked (0x01000338), so the original declared a wider type here.  The
//   value is 0..0xFFFF on every path (0, or two bytes combined).
int ImageLoader::MCardSource::getLoadFlags(const char *name, int port, int slot)
{
    if (!mcard_is_usable(port, slot))
        return 0;

    MCardFileName fn(name);
    MCardFile f(mc_open_sync(port, slot, fn.GetPath(), 1));
    if (f.fd < 0)
        return 0;

    u8 hdr[2];                                  /* sp+0x20 */
    if (mc_read_sync(f.fd, hdr, 2) != 2)
        return 0;
    return get_le16(hdr);
}

// FUNC 0x01005900 __tfQ211ImageLoader11MCardSource (compiler-generated)
//   typeinfo function, slot 0 of vtable 0x0103E310.  If the __si_type_info
//   object at 0x0104DD98 (.bss) is not built yet (first word == 0): call
//   __tfQ211ImageLoader6Source (0x01004BB8), then __rtti_si(0x0104DD98,
//   "Q211ImageLoader11MCardSource" (.rodata 0x0103E330), &__ti Source
//   @0x01041E98) (0x01019C38).  Returns 0x0104DD98.  Also called by the
//   MCardSource_<> __tf functions (0x010006B0, 0x01000720) for their base.

// FUNC 0x01005950 _$_Q211ImageLoader11MCardSource (compiler-generated)
//   ImageLoader::MCardSource::~MCardSource, synthesised.  Vtable slot 1:
//   forwards (this, flags) to ImageLoader::Source::~Source.

/* ======================================================================== */
/* host: file helpers (0x01005970..0x01005B68, own object file, no EH)       */
/* ======================================================================== */
/* Each builds "host:" + path in a 128-byte stack buffer: the "host:" copy
 * is GCC's inlined strcpy (lw + lhu of the 6-byte .sdata literal 0x01041E18,
 * sw + sh), followed by strcat (0x0101B264) with no length check.  The I/O
 * goes through the io unit's lock-protected wrappers (host_open ...). */

// FUNC 0x01005970 host_read_file
//   fd = host_open("host:" + path, SCE_RDONLY); fd < 0 -> NULL.
//   avail = host_lseek(fd, 0, END) - offset (unsigned, no error check);
//   host_lseek(fd, offset, SET); len = (len == 0 || avail < len) ? avail : len
//   (sltu).  buf == NULL: buf = malloc(len) (0x01011390), and if that fails
//   the read is skipped.  got = host_read(fd, buf, len); host_close(fd).
//   Returns buf iff got == len, else NULL (a buffer malloc'ed here then
//   leaks).  Callers: HostSource::open (buf = Source buffer, 0, 0) and
//   HostSource::getLoadFlags (2-byte stack buffer, 0, 2).
void *host_read_file(const char *path, void *buf, u32 offset, u32 len)
{
    char full[128];                             /* sp+0x00 */

    strcpy(full, "host:");
    strcat(full, path);
    int fd = host_open(full, SCE_RDONLY);
    if (fd < 0)
        return NULL;

    u32 avail = (u32)host_lseek(fd, 0, SCE_SEEK_END) - offset;
    host_lseek(fd, (int)offset, SCE_SEEK_SET);
    if (len == 0 || avail < len)
        len = avail;

    /* `got` ($s5) is left uninitialised on the malloc-failure path, as in
     * the original: buf is NULL there, so the result is NULL whatever `got`
     * holds (movz of buf on got == len). */
    u32 got;
    if (buf == NULL)
        buf = malloc(len);
    if (buf != NULL)
        got = (u32)host_read(fd, buf, (int)len);
    host_close(fd);
    return (got == len) ? buf : NULL;
}

// FUNC 0x01005A78 host_write_file
//   fd = host_open("host:" + path, SCE_WRONLY | SCE_CREAT | SCE_TRUNC
//   (0x602)); fd < 0 -> false.  host_write(fd, buf, len) (result ignored),
//   host_close(fd), true.  No callers (debug dump helper left in).
bool host_write_file(const char *path, const void *buf, u32 len)
{
    char full[128];                             /* sp+0x00 */

    strcpy(full, "host:");
    strcat(full, path);
    int fd = host_open(full, SCE_WRONLY | SCE_CREAT | SCE_TRUNC);
    if (fd < 0)
        return false;
    host_write(fd, buf, (int)len);
    host_close(fd);
    return true;
}

// FUNC 0x01005B08 host_file_size
//   st at sp+0 (HostFileStat, 16 bytes reserved), "host:" + path at sp+0x10.
//   Returns st.size if host_get_file_size(full, &st) >= 0 (0x0100F858:
//   open + lseek END + close), else 0.  Caller: HostSource::open.
//   Sibling of host_file_exists (0x01005B68, below).
u32 host_file_size(const char *path)
{
    HostFileStat st;                            /* sp+0x00 */
    char full[128];                             /* sp+0x10 */

    strcpy(full, "host:");
    strcat(full, path);
    if (host_get_file_size(full, &st) < 0)
        return 0;
    return (u32)st.size;
}

// FUNC 0x01005B68 host_file_exists
//   path = "host:" + name in a 128-byte stack buffer (sp+0x10; the "host:"
//   copy is GCC's inlined strcpy: lw+lhu from .sdata 0x01041E18, sw+sh), no
//   bounds check on strcat (0x0101B264).  Returns host_get_file_size(path, &st)
//   >= 0 (slt -1 < r), i.e. the file can be opened via the SIF fileio "host:"
//   device.  st (sp+0x00, 16 bytes reserved) is discarded.  Caller:
//   ImageLoader::HostSource::getLoadFlags (0x01004DB0), with name =
//   HostFileName::GetPath() ("/tmp/<file>").  Sibling of host_file_size
//   (0x01005B08), which returns st.size instead.  Last function of the host
//   file I/O object; in the "filename" unit's address list, moved here at
//   integration.
bool host_file_exists(const char *name)
{
    HostFileStat st;
    char path[128];

    strcpy(path, "host:");
    strcat(path, name);
    if (host_get_file_size(path, &st) < 0)
        return false;
    return true;
}
