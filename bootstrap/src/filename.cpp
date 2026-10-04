#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/filename.cpp"
#pragma implementation "src/filename.cpp"
#pragma implementation "filename.h"
#endif
/* filename.cpp - filename.cxx of the GT3 US bootstrap (BOOTSTRAP_SCUS-97102).
 *
 * FileName_Base / HostFileName / MCardFileName / DiskFileName (see
 * include/filename.h for the class layout and vtables), the two file-local
 * directory-string holders McBaseDir and DirPrefix and the TU's static
 * constructor/destructor.
 *
 * Text range 0x01003570..0x010041C0 (one TU, .eh_frame CIE 0x01027308).
 * host_file_exists (0x01005B68), which the unit split also filed here, lives
 * in the binary in the "host file I/O" object after the ImageLoader TUs and
 * shares its "host:" literal (.sdata 0x01041E18) with host_read_file
 * 0x01005970, host_write_file 0x01005A78 and host_file_size 0x01005B08; it is
 * written with them in src/imageloader_source.cpp.
 *
 * Callers elsewhere: the Source::open / getLoadFlags functions of the
 * imageloader unit construct Host/Disk/MCard FileNames (ctors inlined there),
 * call the GetPath overrides directly (0x01003710, 0x01003960, 0x01003BF0) and
 * destroy with ~FileName_Base(flags 2) or virtually with flags 3.
 *
 * Allocation: `new char[n]` = operator new[] (0x01019B28), `delete[] p` =
 * operator delete[] (0x01019AB0), deleting destructors call operator delete
 * (0x01019910).  Every delete[] in the binary is guarded by an explicit
 * `if (p)` test, reproduced here.
 *
 * #pragma implementation (egcs only, first lines of this file): filename.cxx
 * paired it with #pragma interface in filename.h, which is why the inline
 * members of the FileName classes have out-of-line copies after the static
 * constructor.  egcs accepts the pragma only in the "main" file, and the SCE
 * driver compiles a temporary .ii whose name never matches the source's, so a
 * #line directive first names this file by its repo-relative path (the same
 * path the scorer looks for) and the pragma repeats that name; the second
 * pragma then adds "filename.h".  Keep the #line number equal to the line it
 * precedes.
 */
#include "types.h"
#include "libc.h"
#include "filename.h"
#ifndef BOOT_OLD_GCC
#include <new>          /* placement new in global_ctor_filename */
#endif

/* ------------------------------------------------------------------------ */
/* File-local directory-string holders (only used inside filename.cxx)      */
/* ------------------------------------------------------------------------ */

/* Memory-card save directory "/BASCUS-97102GAMEDATA".  sizeof 4.
 * Only a default ctor and the dtor exist. */
class McBaseDir {
public:
    McBaseDir();                          /* 0x01003880 */
    ~McBaseDir();                         /* 0x01003830 */
    char *path;                           /* +0x00 new char[] */
};

/* A directory string that is always stored with a trailing '/'.  sizeof 4.
 * Holds the HostFileName root "/tmp/".  Copy ctor and both assignment
 * operators exist but are never called. */
class DirPrefix {
public:
    DirPrefix();                          /* 0x01003A60 */
    DirPrefix(const char *dir);           /* 0x01003A70 */
    DirPrefix(const DirPrefix &o);        /* 0x01003AA0 */
    ~DirPrefix();                         /* 0x01003A10 */
    DirPrefix &operator=(const char *dir);        /* 0x01003AD0 */
    DirPrefix &operator=(const DirPrefix &o);     /* 0x01003B20 */
    char *path;                           /* +0x00 new char[], slash-terminated */
};

static char *strdup_with_trailing_slash(const char *s);   /* 0x01003B70 */

/* ------------------------------------------------------------------------ */
/* Globals (.sdata), in definition order.  The four with dynamic            */
/* initialisers are set up by the static constructor (_GLOBAL_$I$,          */
/* 0x01003CC0) and the two objects destroyed by the static destructor       */
/* (_GLOBAL_$D$, 0x01003C90), both run from the collect2 ctor/dtor lists in */
/* src/crtbegin.c.  In the original, and in the egcs build, these are       */
/* ordinary initialised globals and egcs generates those two functions.     */
/* In the modern build the dynamic initialisation is written out explicitly */
/* (global_ctor_filename / global_dtor_filename at the end of the file, as  */
/* the allocator unit does for its global object), so that the modern       */
/* compiler neither emits its own .init_array constructor nor registers the */
/* destructors with __cxa_atexit - the original has neither.               */
/* ------------------------------------------------------------------------ */

/* Three name strings are global char arrays, not literals: "/tmp" and "BA" sit
 * in .sdata right after s_host_dir (0x01041E08, 0x01041E10), and
 * "SCUS-97102" lies outside this object's .rodata (0x0103E350, after
 * imageloader's), yet all three are addressed with lui/addiu.  That is what
 * egcs does for an array first declared with an incomplete type (no
 * small-data flag) and defined later (placed by its final size): the
 * declarations below come first, the two .sdata definitions follow
 * s_host_dir_obj, and the product code is defined in another object
 * (src/mcard_io.cpp here; its original object is unknown).  "/tmp" is not
 * const: egcs would fold a const array into a second "/tmp" literal in the
 * static constructor, which is generated after the definition. */
extern char s_host_dir_name[];           /* "/tmp"       @0x01041E08 (.sdata) */
extern const char s_mc_region[];         /* "BA"         @0x01041E10 (.sdata) */
extern const char s_mc_product[];        /* "SCUS-97102" @0x0103E350 (.rodata) */

#ifdef BOOT_OLD_GCC
/* Non-static globals in the original: egcs emits a global at its definition,
 * in order, into .sdata (a dynamically initialised one zero-filled with
 * .align 3; a string literal of up to 8 bytes right before the pointer it
 * initialises), which is the retail layout 0x01041DD8..0x01041E04.  (Statics
 * would be emitted at the end of the file, in reverse order, into .sbss.)
 * The two pointers are const, so the code uses the literals directly; the two
 * lengths are const too (dynamically initialised, so still .sdata), which
 * lets DiskFileName::GetPath read each only once. */
extern const char *const s_disk_prefix = "cdrom0:\\";      /* @0x01041DD8 (.sdata) -> .rodata 0x0103E198 */
extern const size_t s_disk_prefix_len = strlen(s_disk_prefix); /* @0x01041DE0 (.sdata) = 8 */
extern const char *const s_disk_suffix = ";1";             /* @0x01041DEC (.sdata) -> 0x01041DE8 */
extern const size_t s_disk_suffix_len = strlen(s_disk_suffix); /* @0x01041DF0 (.sdata) = 2 */
McBaseDir s_mc_base_dir_obj;                               /* @0x01041DF8 (.sdata) "/BASCUS-97102GAMEDATA" */
DirPrefix s_host_dir_obj(s_host_dir_name);                 /* @0x01041E00 (.sdata) "/tmp" + '/' */
char s_host_dir_name[] = "/tmp";                           /* @0x01041E08 (.sdata) */
const char s_mc_region[] = "BA";                           /* @0x01041E10 (.sdata) */

static inline McBaseDir &s_mc_base_dir(void)
{
    return s_mc_base_dir_obj;
}

static inline DirPrefix &s_host_dir(void)
{
    return s_host_dir_obj;
}
#else
/* @0x01041DD8 -> "cdrom0:\" (.rodata 0x0103E198).  Code uses the literal
 * address directly (the pointer is const-propagated). */
static const char *const s_disk_prefix = "cdrom0:\\";
/* @0x01041DE0, 0 in the image; global_ctor_filename stores
 * strlen(s_disk_prefix), folded to 8. */
static size_t s_disk_prefix_len;
/* @0x01041DEC -> ";1" (.sdata literal 0x01041DE8). */
static const char *const s_disk_suffix = ";1";
/* @0x01041DF0, 0 in the image; global_ctor_filename stores
 * strlen(s_disk_suffix), folded to 2. */
static size_t s_disk_suffix_len;
/* @0x01041DF8 (gp-0x7EF8): McBaseDir s_mc_base_dir, "/BASCUS-97102GAMEDATA",
 * built by global_ctor_filename.  Raw storage (path == NULL until then). */
alignas(McBaseDir) static u8 s_mc_base_dir_storage[sizeof(McBaseDir)];
/* @0x01041E00 (gp-0x7EF0): DirPrefix s_host_dir, "/tmp" (.sdata 0x01041E08)
 * + '/' = "/tmp/", built by global_ctor_filename.  Raw storage. */
alignas(DirPrefix) static u8 s_host_dir_storage[sizeof(DirPrefix)];
/* (0x01041E08 / 0x01041E10) */
char s_host_dir_name[] = "/tmp";
const char s_mc_region[] = "BA";

static inline McBaseDir &s_mc_base_dir(void)
{
    return *reinterpret_cast<McBaseDir *>(s_mc_base_dir_storage);
}

static inline DirPrefix &s_host_dir(void)
{
    return *reinterpret_cast<DirPrefix *>(s_host_dir_storage);
}
#endif

/* ------------------------------------------------------------------------ */
/* FileName_Base                                                            */
/* ------------------------------------------------------------------------ */

// FUNC 0x01003570 FileName_Base::~FileName_Base
//   Virtual dtor (vtable slot 1, +0xC).  Resets vptr (+8) to FileName_Base's
//   vtable 0x0103E200, delete[]s name and the cached path (each only if non-NULL),
//   then operator delete(this) if (flags & 1) (GCC 2.9x hidden dtor flag).
//   Also the whole body of the three derived dtors, which just forward here.
FileName_Base::~FileName_Base()
{
    if (name)
        delete[] name;
    if (path)
        delete[] path;
}

// FUNC 0x010035E0 strdup_alloc
//   NULL -> NULL; otherwise new char[strlen(s) + 1] and memcpy including the NUL.
//   Used by every FileName constructor/assignment (also inlined ones in ImageLoader).
char *strdup_alloc(const char *s)
{
    if (s == NULL)
        return NULL;
    size_t n = strlen(s) + 1;
    char *p = new char[n];
    memcpy(p, s, n);
    return p;
}

// FUNC 0x01003648 FileName_Base::operator=(const char *)
//   Replace the name with a copy of s and drop the cached path.  No callers.
FileName_Base &FileName_Base::operator=(const char *s)
{
    if (name)
        delete[] name;
    name = strdup_alloc(s);
    if (path)
        delete[] path;
    path = NULL;
    return *this;
}

// FUNC 0x010036A8 FileName_Base::operator=(const FileName_Base &)
//   Same as above with o.name.  Quirk: no self-assignment check - `x = x`
//   frees name and then duplicates the freed buffer (use after free).
//   The cached path is dropped, not copied.  Only caller: assign() 0x010040C0.
FileName_Base &FileName_Base::operator=(const FileName_Base &o)
{
    if (name)
        delete[] name;
    name = strdup_alloc(o.name);
    if (path)
        delete[] path;
    path = NULL;
    return *this;
}

// FUNC 0x01003708 FileName_Base::GetPath
//   Virtual (vtable slot 2, +0x14): the base class path is the bare name.
//   Does not touch the path cache.
const char *FileName_Base::GetPath()
{
    return name;
}

/* ------------------------------------------------------------------------ */
/* DiskFileName path                                                        */
/* ------------------------------------------------------------------------ */

// FUNC 0x01003710 DiskFileName::GetPath
//   Virtual (DiskFileName vtable 0x0103E1E8 slot 2).  Builds once and caches in
//   +4: "cdrom0:\" + name with 'a'..'z' upper-cased + ";1" (e.g.
//   "cdrom0:\CORE.GT3;1").  Called directly by DiskSource code (0x01004F58,
//   0x010051F8).  The prefix/suffix lengths come from the .sdata globals set by
//   the static ctor; the suffix copy includes its NUL.
const char *DiskFileName::GetPath()
{
    char *result = path;

    if (result == NULL) {
        /* The two lengths are const globals: egcs reads each once, after
         * strlen, and reuses the value across the calls (unchanging memory),
         * which is the retail code. */
        size_t len = strlen(name);
        char *p = new char[s_disk_prefix_len + (s_disk_suffix_len + len + 1)];
        size_t i;

        memcpy(p, s_disk_prefix, s_disk_prefix_len);
        for (i = 0; i < len; i++) {                 /* unsigned compare (sltu) */
            char c = name[i];                        /* lb: signed char */
            if ((unsigned int)(c - 'a') < 26)        /* 'a' <= c <= 'z' */
                c = (char)(c - 0x20);
            p[s_disk_prefix_len + i] = c;
        }
        memcpy(p + s_disk_prefix_len + len, s_disk_suffix, s_disk_suffix_len + 1);

        path = p;
        result = p;
    }
    return result;
}

/* ------------------------------------------------------------------------ */
/* McBaseDir (memory-card directory)                                        */
/* ------------------------------------------------------------------------ */

// FUNC 0x01003830 McBaseDir::~McBaseDir
//   delete[] path if non-NULL; operator delete(this) if (flags & 1).  Only
//   called from global_dtor_filename with flags 2 for s_mc_base_dir.
McBaseDir::~McBaseDir()
{
    if (path)
        delete[] path;
}

// FUNC 0x01003880 McBaseDir::McBaseDir
//   Default ctor (mapping pass: mc_basedir_build).  path = NULL, then builds
//   "/" + "BA" + "SCUS-97102" + "GAMEDATA" = "/BASCUS-97102GAMEDATA" (22 bytes
//   with NUL) in new char[strlen("BA") + strlen("SCUS-97102") + 10] and stores it.
//   strlen of s_mc_region "BA" (.sdata 0x01041E10) and s_mc_product
//   "SCUS-97102" (.rodata 0x0103E350) are real runtime calls (0x0100F538); the GAMEDATA part is a constant
//   9-byte inline copy (ldl/ldr + sdl/sdr + lb/sb from .rodata 0x0103E1A8),
//   i.e. a builtin strcpy; the 10 = '/' + 8 + NUL.  Returns this.
McBaseDir::McBaseDir()
    : path(NULL)
{
    const char *region = s_mc_region;
    size_t region_len = strlen(region);
    const char *product = s_mc_product;
    size_t product_len = strlen(product);
    char *p = new char[region_len + product_len + 10];

    p[0] = '/';
    memcpy(p + 1, region, region_len);
    memcpy(p + 1 + region_len, product, product_len);
    strcpy(p + 1 + region_len + product_len, "GAMEDATA");  /* inlined 9-byte copy */
    path = p;
}

/* ------------------------------------------------------------------------ */
/* MCardFileName path                                                       */
/* ------------------------------------------------------------------------ */

// FUNC 0x01003960 MCardFileName::GetPath
//   Virtual (MCardFileName vtable 0x0103E1D0 slot 2).  Builds once and caches in
//   +4: s_mc_base_dir.path + "/" + name ("/BASCUS-97102GAMEDATA/core.gt3").
//   Called directly by MCardSource code (0x01005458, 0x01005748).
const char *MCardFileName::GetPath()
{
    if (path != NULL)
        return path;

    const char *dir = s_mc_base_dir().path;
    size_t dir_len = strlen(dir);
    size_t name_len = strlen(name);
    char *p = new char[dir_len + (name_len + 2)];
    memcpy(p, dir, dir_len);
    p[dir_len] = '/';
    memcpy(p + dir_len + 1, name, name_len + 1);

    path = p;
    return p;
}

/* ------------------------------------------------------------------------ */
/* DirPrefix (host root directory)                                          */
/* ------------------------------------------------------------------------ */

// FUNC 0x01003A10 DirPrefix::~DirPrefix
//   delete[] path if non-NULL; operator delete(this) if (flags & 1).  Only
//   called from global_dtor_filename with flags 2 for s_host_dir.
DirPrefix::~DirPrefix()
{
    if (path)
        delete[] path;
}

// FUNC 0x01003A60 DirPrefix::DirPrefix()
//   path = NULL; returns this.  No callers.
DirPrefix::DirPrefix()
    : path(NULL)
{
}

// FUNC 0x01003A70 DirPrefix::DirPrefix(const char *)
//   path = strdup_with_trailing_slash(dir).  Used by the static ctor with "/tmp".
DirPrefix::DirPrefix(const char *dir)
    : path(strdup_with_trailing_slash(dir))
{
}

// FUNC 0x01003AA0 DirPrefix::DirPrefix(const DirPrefix &)
//   path = strdup_with_trailing_slash(o.path).  No callers.
DirPrefix::DirPrefix(const DirPrefix &o)
    : path(strdup_with_trailing_slash(o.path))
{
}

// FUNC 0x01003AD0 DirPrefix::operator=(const char *)
//   Frees the old string, stores a slash-terminated copy of dir.  No callers.
DirPrefix &DirPrefix::operator=(const char *dir)
{
    if (path)
        delete[] path;
    path = strdup_with_trailing_slash(dir);
    return *this;
}

// FUNC 0x01003B20 DirPrefix::operator=(const DirPrefix &)
//   Same with o.path; no self-assignment check (x = x would copy freed memory).
//   No callers.
DirPrefix &DirPrefix::operator=(const DirPrefix &o)
{
    if (path)
        delete[] path;
    path = strdup_with_trailing_slash(o.path);
    return *this;
}

// FUNC 0x01003B70 strdup_with_trailing_slash
//   NULL -> NULL.  Otherwise new char[len + 2], copy s with its NUL, and if the
//   last character is not '/', append "/" + NUL.  Quirk: for s == "" (len 0)
//   it reads p[-1], one byte before the allocation.
static char *strdup_with_trailing_slash(const char *s)
{
    if (s == NULL)
        return NULL;
    size_t len = strlen(s);
    char *p = new char[len + 2];
    memcpy(p, s, len + 1);
    if (p[len - 1] != '/') {                       /* lb: signed compare with 0x2F */
        p[len] = '/';
        p[len + 1] = '\0';
    }
    return p;
}

/* ------------------------------------------------------------------------ */
/* HostFileName path                                                        */
/* ------------------------------------------------------------------------ */

// FUNC 0x01003BF0 HostFileName::GetPath
//   Virtual (HostFileName vtable 0x0103E1B8 slot 2).  Builds once and caches in
//   +4: s_host_dir.path + name ("/tmp/core.gt3"; the directory already ends in
//   '/').  No "host:" prefix here.  Called directly by HostSource code
//   (0x01004C60, 0x01004DB0).
const char *HostFileName::GetPath()
{
    if (path != NULL)
        return path;

    const char *dir = s_host_dir().path;
    size_t dir_len = strlen(dir);
    size_t name_len = strlen(name);
    char *p = new char[dir_len + name_len + 1];
    memcpy(p, dir, dir_len);
    memcpy(p + dir_len, name, name_len + 1);

    path = p;
    return p;
}

/* ------------------------------------------------------------------------ */
/* Static constructor / destructor of the TU.  In the original, and in the  */
/* egcs build, they are compiler-generated from the global definitions at   */
/* the top of this file: egcs names them after the first global symbol of   */
/* the file (here s_disk_prefix; the original's name is unknown) and puts   */
/* them after the last non-inline function, destructor first.  collect2     */
/* listed them in the ctor/dtor tables; src/crtbegin.c names them           */
/* global_ctor_filename / global_dtor_filename (C linkage, declared in      */
/* filename.h), which the egcs build defines as assembler aliases of the    */
/* generated functions (no code of their own).  The modern build writes     */
/* them out, see the note above the globals.                                */
/* ------------------------------------------------------------------------ */

// FUNC 0x01003C90 _GLOBAL_$D$s_disk_prefix global_dtor_filename (compiler-generated static destructor)
//   Entry of __DTOR_LIST__ at .data 0x01026CB4.  Destroys in reverse
//   construction order, both with GCC 2.9x flags 2 (destroy, do not delete):
//     DirPrefix::~DirPrefix(&s_host_dir /*0x01041E00*/, 2);      (0x01003A10)
//     McBaseDir::~McBaseDir(&s_mc_base_dir /*0x01041DF8*/, 2);   (0x01003830)
//   Never runs in practice (__do_global_dtors has no callers).

// FUNC 0x01003CC0 _GLOBAL_$I$s_disk_prefix global_ctor_filename (compiler-generated static constructor)
//   Entry of __CTOR_LIST__ at .data 0x01026C9C; __do_global_ctors runs it
//   last (after reg_frame and global_ctor_allocator).  In order:
//     s_disk_prefix_len = 8;   (.sdata 0x01041DE0, strlen("cdrom0:\") folded)
//     s_disk_suffix_len = 2;   (.sdata 0x01041DF0, strlen(";1") folded)
//     McBaseDir::McBaseDir(&s_mc_base_dir);                      (0x01003880)
//     DirPrefix::DirPrefix(&s_host_dir, "/tmp");                 (0x01003A70)
//   i.e. the dynamic initialisers of the globals defined above, in that order.

#ifdef BOOT_OLD_GCC
__asm__("\t.globl\tglobal_dtor_filename\n"
        "\tglobal_dtor_filename = _GLOBAL_$D$s_disk_prefix\n"
        "\t.globl\tglobal_ctor_filename\n"
        "\tglobal_ctor_filename = _GLOBAL_$I$s_disk_prefix\n");
#else
extern "C" void global_dtor_filename(void)
{
    /* qualified calls = direct jal, not virtual dispatch (neither class has
     * a vtable anyway) */
    s_host_dir().DirPrefix::~DirPrefix();
    s_mc_base_dir().McBaseDir::~McBaseDir();
}

extern "C" void global_ctor_filename(void)
{
    s_disk_prefix_len = strlen(s_disk_prefix);      /* folded to 8 */
    s_disk_suffix_len = strlen(s_disk_suffix);      /* folded to 2 */
    new (s_mc_base_dir_storage) McBaseDir;          /* 0x01003880 */
    new (s_host_dir_storage) DirPrefix(s_host_dir_name); /* 0x01003A70, "/tmp" @0x01041E08 */
}
#endif

/* ------------------------------------------------------------------------ */
/* Out-of-line copies of the inline members of filename.h, emitted after   */
/* the static constructor through #pragma implementation (egcs build).  The */
/* bodies are in include/filename.h.  The overloaded constructors are named */
/* by their mangled symbols so that the scorer can tell them apart.         */
/* ------------------------------------------------------------------------ */

// FUNC 0x01003D00 __12HostFileNameRC13FileName_Base HostFileName::HostFileName(const FileName_Base &) (inline, out-of-line copy)
//   vptr = FileName_Base vtable, name = strdup_alloc(o.name), vptr = HostFileName
//   vtable 0x0103E1B8, path = NULL (the cached path is not copied).
//   /* ? */ parameter may have been const HostFileName & - the code only reads o.name.

// FUNC 0x01003D50 __12HostFileNamePCc HostFileName::HostFileName(const char *) (inline, out-of-line copy)
//   name = strdup_alloc(name), path = NULL, vptr = 0x0103E1B8.

// FUNC 0x01003DA0 __12HostFileName HostFileName::HostFileName() (inline, out-of-line copy)
//   name = NULL, path = NULL, vptr = 0x0103E1B8.

// FUNC 0x01003DC0 __tf12HostFileName (compiler-generated)
//   typeinfo function, HostFileName vtable slot 0.  If __ti12HostFileName
//   (__si_type_info @0x0104DD68 .bss) is not built yet (first word == 0): call
//   __tf13FileName_Base() (0x01004180, result ignored), then
//   __si_type_info::__si_type_info(&__ti12HostFileName, "12HostFileName"
//   (.rodata 0x0103E218), __ti13FileName_Base @0x01041E90) (0x01019C38).
//   Returns &__ti12HostFileName.

// FUNC 0x01003E10 _$_12HostFileName HostFileName::~HostFileName (inline, out-of-line copy)
//   Virtual dtor (slot 1): tail-forwards (this, flags) unchanged to
//   FileName_Base::~FileName_Base (0x01003570), which also handles flags & 1.

// FUNC 0x01003E30 __13MCardFileNameRC13FileName_Base MCardFileName::MCardFileName(const FileName_Base &) (inline, out-of-line copy)
//   vptr = FileName_Base vtable, name = strdup_alloc(o.name), vptr = MCardFileName
//   vtable 0x0103E1D0, path = NULL.
//   /* ? */ parameter may have been const MCardFileName &.

// FUNC 0x01003E80 __13MCardFileNamePCc MCardFileName::MCardFileName(const char *) (inline, out-of-line copy)
//   name = strdup_alloc(name), path = NULL, vptr = 0x0103E1D0.

// FUNC 0x01003ED0 __13MCardFileName MCardFileName::MCardFileName() (inline, out-of-line copy)
//   name = NULL, path = NULL, vptr = 0x0103E1D0.

// FUNC 0x01003EF0 __tf13MCardFileName (compiler-generated)
//   typeinfo function, MCardFileName vtable slot 0.  If __ti13MCardFileName
//   (__si_type_info @0x0104DD58 .bss) is not built yet (first word == 0): call
//   __tf13FileName_Base() (0x01004180), then __si_type_info::__si_type_info(
//   &__ti13MCardFileName, "13MCardFileName" (.rodata 0x0103E228),
//   __ti13FileName_Base @0x01041E90) (0x01019C38).  Returns &__ti13MCardFileName.

// FUNC 0x01003F40 _$_13MCardFileName MCardFileName::~MCardFileName (inline, out-of-line copy)
//   Virtual dtor (slot 1): tail-forwards (this, flags) to FileName_Base::~FileName_Base.

// FUNC 0x01003F60 __12DiskFileNameRC13FileName_Base DiskFileName::DiskFileName(const FileName_Base &) (inline, out-of-line copy)
//   vptr = FileName_Base vtable, name = strdup_alloc(o.name), vptr = DiskFileName
//   vtable 0x0103E1E8, path = NULL.
//   /* ? */ parameter may have been const DiskFileName &.

// FUNC 0x01003FB0 __12DiskFileNamePCc DiskFileName::DiskFileName(const char *) (inline, out-of-line copy)
//   name = strdup_alloc(name), path = NULL, vptr = 0x0103E1E8.

// FUNC 0x01004000 __12DiskFileName DiskFileName::DiskFileName() (inline, out-of-line copy)
//   name = NULL, path = NULL, vptr = 0x0103E1E8.

// FUNC 0x01004020 __tf12DiskFileName (compiler-generated)
//   typeinfo function, DiskFileName vtable slot 0.  If __ti12DiskFileName
//   (__si_type_info @0x0104DD48 .bss) is not built yet (first word == 0): call
//   __tf13FileName_Base() (0x01004180), then __si_type_info::__si_type_info(
//   &__ti12DiskFileName, "12DiskFileName" (.rodata 0x0103E238),
//   __ti13FileName_Base @0x01041E90) (0x01019C38).  Returns &__ti12DiskFileName.

// FUNC 0x01004070 _$_12DiskFileName DiskFileName::~DiskFileName (inline, out-of-line copy)
//   Virtual dtor (slot 1): tail-forwards (this, flags) to FileName_Base::~FileName_Base.

// FUNC 0x01004090 GetFullPath__13FileName_Base FileName_Base::GetFullPath (inline, out-of-line copy)
//   Non-virtual wrapper: dispatches through the vtable to slot 2 (lh delta @vt+0x10,
//   lw pfn @vt+0x14, call with this + delta) = GetPath().  No callers (callers
//   inline the same sequence, e.g. ImageLoader::Source::GetPath 0x01004B08).
//   Name invented (mapping pass: filename_get_path).

// FUNC 0x010040B8 GetName__13FileName_Base FileName_Base::GetName (inline, out-of-line copy)
//   Returns the raw name (+0).

// FUNC 0x010040C0 assign__13FileName_BaseRC13FileName_Base FileName_Base::assign (inline, out-of-line copy)
//   Forwards (this, o) to operator=(const FileName_Base &) (0x010036A8) and
//   returns its result (this).  No callers.  /* ? */ exact original name unknown.

// FUNC 0x010040E0 __13FileName_BaseRC13FileName_Base FileName_Base::FileName_Base(const FileName_Base &) (inline, out-of-line copy)
//   vptr = 0x0103E200, name = strdup_alloc(o.name), path = NULL (cache not copied).

// FUNC 0x01004120 __13FileName_BasePCc FileName_Base::FileName_Base(const char *) (inline, out-of-line copy)
//   vptr = 0x0103E200, name = strdup_alloc(n), path = NULL.

// FUNC 0x01004160 __13FileName_Base FileName_Base::FileName_Base() (inline, out-of-line copy)
//   name = NULL, path = NULL, vptr = 0x0103E200.

// FUNC 0x01004180 __tf13FileName_Base (compiler-generated)
//   typeinfo function, FileName_Base vtable slot 0.  If __ti13FileName_Base
//   (__user_type_info @0x01041E90 .sbss, gp-0x7E60) is not built yet (first word
//   == 0): __user_type_info::__user_type_info(&__ti13FileName_Base,
//   "13FileName_Base" (.rodata 0x0103E248)) (0x01019C58).  Returns
//   &__ti13FileName_Base.  Also called by the three derived __tf functions.

/* 0x010041C0 / 0x010041C8 (two empty functions right after __tf13FileName_Base)
 * are NOT part of filename.cxx: filename.cxx's .eh_frame FDE list (CIE .data
 * 0x01027308) ends with __tf13FileName_Base and its zero terminator at
 * 0x01027974; their FDEs are the first two under imageloader.cxx's "eh" CIE
 * 0x01027978.  Moved to src/imageloader.cpp at integration. */

/* host_file_exists (0x01005B68) is the last function of the "host file I/O"
 * object (0x01005970..0x01005BC0, .eh_frame CIE 0x01033C94), after
 * host_file_size; it is written in src/imageloader_source.cpp with its
 * siblings and declared in imageloader.h (moved at integration). */
