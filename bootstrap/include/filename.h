/* filename.h - FileName_Base and its device-specific subclasses (filename.cxx)
 * of the GT3 US bootstrap (BOOTSTRAP_SCUS-97102).
 *
 * A FileName holds a bare file name ("core.gt3") and lazily builds the full
 * device path for one boot source:
 *
 *   DiskFileName   "cdrom0:\" + UPPERCASE(name) + ";1"   -> "cdrom0:\CORE.GT3;1"
 *   MCardFileName  "/BASCUS-97102GAMEDATA" + "/" + name  -> "/BASCUS-97102GAMEDATA/core.gt3"
 *   HostFileName   "/tmp/" + name                        -> "/tmp/core.gt3"
 *                  (the "host:" device prefix is added later, by the HostSource
 *                   helpers / host_file_exists() in imageloader.h, not by the FileName)
 *   FileName_Base  name unchanged
 *
 * The path is built once by the virtual GetPath() and cached in +0x04; it is
 * freed by the destructor and by the assignment operators.
 *
 * Original object layout (GCC 2.9x-ee), sizeof == 0xC (operator new(0xC) in
 * ImageLoader::{Host,Disk,MCard}Source::open):
 *   +0x00 char *name   heap copy (new char[]) of the file name, may be NULL
 *   +0x04 char *path   cached device path built by GetPath() (new char[]), NULL until built
 *   +0x08 vptr         GCC 2.9x places the vptr AFTER the data members here
 * (Modern g++ puts the vptr at +0; the size is still 0xC on the EE.)
 *
 * Vtables (.rodata), GCC 2.9x format: 8-byte entries {s16 delta, s16 index, pfn},
 * all deltas 0; vptr points at entry 0.
 *                    vptr        [0] __tf (+0x04)   [1] dtor (+0x0C)   [2] GetPath (+0x14)
 *   FileName_Base    0x0103E200  0x01004180         0x01003570         0x01003708
 *   HostFileName     0x0103E1B8  0x01003DC0         0x01003E10         0x01003BF0
 *   MCardFileName    0x0103E1D0  0x01003EF0         0x01003F40         0x01003960
 *   DiskFileName     0x0103E1E8  0x01004020         0x01004070         0x01003710
 * RTTI names: "12HostFileName" 0x0103E218, "13MCardFileName" 0x0103E228,
 *   "12DiskFileName" 0x0103E238, "13FileName_Base" 0x0103E248.
 * Typeinfo objects: __ti13FileName_Base (__user_type_info) @0x01041E90 .sbss,
 *   __ti12DiskFileName @0x0104DD48, __ti13MCardFileName @0x0104DD58,
 *   __ti12HostFileName @0x0104DD68 (__si_type_info, .bss).
 *
 * The constructors, GetName(), GetFullPath() and assign() are inline (defined
 * below): ImageLoader code inlines them (e.g. HostSource::open builds a
 * HostFileName on its stack and calls strdup_alloc directly), and
 * filename.cxx additionally emits out-of-line copies of all of them after its
 * static constructor - the #pragma interface (here) / #pragma implementation
 * (src/filename.cpp) pattern, used again by the egcs build.  The three
 * derived classes declare no destructor: their implicit virtual destructors
 * (_$_12HostFileName, ...) only forward (this, flags) to ~FileName_Base and,
 * being compiler-generated with an empty body, do not reset the vptr.  All
 * their FUNC markers are in src/filename.cpp.
 *
 * GCC 2.9x destructors take a hidden `int flags` (bit 0 = operator delete this).
 * Automatic FileName objects are destroyed with flags 2, heap ones through the
 * vtable with flags 3 (ImageLoader::Source::~Source).
 */
#ifndef BOOT_FILENAME_H
#define BOOT_FILENAME_H

#include "types.h"

/* Static constructor / destructor of filename.cxx (_GLOBAL_$I$ / _GLOBAL_$D$,
 * compiler-generated in the original).  Entries of the collect2 ctor/dtor
 * lists in src/crtbegin.c (C), hence C linkage and visible to C.  Modern
 * build: written out in src/filename.cpp.  egcs build: assembler aliases of
 * the generated _GLOBAL_$I$s_disk_prefix / _GLOBAL_$D$s_disk_prefix. */
#ifdef __cplusplus
extern "C" {
#endif
void global_ctor_filename(void);            /* 0x01003CC0 */
void global_dtor_filename(void);            /* 0x01003C90 */
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#ifdef BOOT_OLD_GCC
/* See above: egcs emits the out-of-line copies of the inline members, the
 * vtables and the typeinfo functions only in src/filename.cpp. */
#pragma interface
#endif

/* 0x010035E0: NULL-safe strdup into new char[strlen(s) + 1]. */
char *strdup_alloc(const char *s);

/* host_file_exists (0x01005B68) is declared in imageloader.h (host file I/O
 * object, src/imageloader_source.cpp). */

class FileName_Base {
public:
    FileName_Base();                                   /* 0x01004160 */
    FileName_Base(const char *name);                   /* 0x01004120 */
    FileName_Base(const FileName_Base &o);             /* 0x010040E0 */
    virtual ~FileName_Base();                          /* 0x01003570 vtable slot 1 */

    FileName_Base &operator=(const char *name);        /* 0x01003648 (no callers) */
    FileName_Base &operator=(const FileName_Base &o);  /* 0x010036A8 */
    FileName_Base &assign(const FileName_Base &o);     /* 0x010040C0 forwards to operator= */

    const char *GetName();                             /* 0x010040B8 returns name */
    const char *GetFullPath();                         /* 0x01004090 non-virtual, calls GetPath() */
    virtual const char *GetPath();                     /* 0x01003708 vtable slot 2: returns name */

    char *name;                                        /* +0x00 */
    char *path;                                        /* +0x04 cached GetPath() result */
    /* +0x08 vptr in the original */
};

class DiskFileName : public FileName_Base {
public:
    DiskFileName();                                    /* 0x01004000 */
    DiskFileName(const char *name);                    /* 0x01003FB0 */
    DiskFileName(const FileName_Base &o);              /* 0x01003F60 */
    /* ~DiskFileName: implicit (compiler-generated, 0x01004070) */
    virtual const char *GetPath();                     /* 0x01003710 "cdrom0:\" + UPPER(name) + ";1" */
};

class MCardFileName : public FileName_Base {
public:
    MCardFileName();                                   /* 0x01003ED0 */
    MCardFileName(const char *name);                   /* 0x01003E80 */
    MCardFileName(const FileName_Base &o);             /* 0x01003E30 */
    /* ~MCardFileName: implicit (compiler-generated, 0x01003F40) */
    virtual const char *GetPath();                     /* 0x01003960 "/BASCUS-97102GAMEDATA/" + name */
};

class HostFileName : public FileName_Base {
public:
    HostFileName();                                    /* 0x01003DA0 */
    HostFileName(const char *name);                    /* 0x01003D50 */
    HostFileName(const FileName_Base &o);              /* 0x01003D00 */
    /* ~HostFileName: implicit (compiler-generated, 0x01003E10) */
    virtual const char *GetPath();                     /* 0x01003BF0 "/tmp/" + name */
};

/* ------------------------------------------------------------------------ */
/* Inline members (out-of-line copies and FUNC markers in src/filename.cpp) */
/* ------------------------------------------------------------------------ */

/* The vptr is set before the member initialisers run (GCC 2.9x). */
inline FileName_Base::FileName_Base()
    : name(NULL), path(NULL)
{
}

inline FileName_Base::FileName_Base(const char *n)
    : name(strdup_alloc(n)), path(NULL)
{
}

/* The cached path is not copied. */
inline FileName_Base::FileName_Base(const FileName_Base &o)
    : name(strdup_alloc(o.name)), path(NULL)
{
}

inline FileName_Base &FileName_Base::assign(const FileName_Base &o)
{
    return operator=(o);
}

inline const char *FileName_Base::GetName()
{
    return name;
}

/* Non-virtual: dispatches through the vtable (slot 2) to GetPath(). */
inline const char *FileName_Base::GetFullPath()
{
    return GetPath();
}

inline DiskFileName::DiskFileName()
    : FileName_Base()
{
}

inline DiskFileName::DiskFileName(const char *n)
    : FileName_Base(n)
{
}

inline DiskFileName::DiskFileName(const FileName_Base &o)
    : FileName_Base(o)
{
}

inline MCardFileName::MCardFileName()
    : FileName_Base()
{
}

inline MCardFileName::MCardFileName(const char *n)
    : FileName_Base(n)
{
}

inline MCardFileName::MCardFileName(const FileName_Base &o)
    : FileName_Base(o)
{
}

inline HostFileName::HostFileName()
    : FileName_Base()
{
}

inline HostFileName::HostFileName(const char *n)
    : FileName_Base(n)
{
}

inline HostFileName::HostFileName(const FileName_Base &o)
    : FileName_Base(o)
{
}

#if defined(__cplusplus) && __cplusplus >= 201103L
static_assert(sizeof(FileName_Base) == 0xC, "FileName_Base is 0xC bytes (operator new(0xC))");
static_assert(sizeof(HostFileName) == 0xC, "HostFileName adds no members");
static_assert(sizeof(MCardFileName) == 0xC, "MCardFileName adds no members");
static_assert(sizeof(DiskFileName) == 0xC, "DiskFileName adds no members");
#endif

#endif /* __cplusplus */

#endif /* BOOT_FILENAME_H */
