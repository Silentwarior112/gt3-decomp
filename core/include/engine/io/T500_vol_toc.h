/* T500_vol_toc.h - the gt3.vol table of contents (translation unit T500, unit u110),
 * 0x0023D7E0..0x0023DCA8.
 *
 * The TOC ("RoFS") of a volume: a header (magic "RoFS", word 3 = header size,
 * word 4 = body size), then directory records.  A TOC stored with the magic
 * ~"RoFS" (AD 90 B9 AC) has its body inverted (decoded once at mount time).
 * A directory record at byte offset `off`:
 *   +0x0  low 24 bits: offset of its name (NUL-terminated)
 *   +0x4  number of entries + 1
 *   +0x8  offset of the parent directory record (0 at the root)
 *   +0xC  offsets of the entry records, sorted by name (binary search).
 * The root is record 0x14.  An entry record: +0x3 type (1 directory, 2
 * gzip-packed file, else a file), +0x4 first sector (relative to the
 * volume), +0x8 size, +0xC packed size (T502 reads them).
 *
 * Path lookup works on a VolDir {VolToc *, record offset} (its word 0 is the
 * VolToc, whose word 0 is the TOC data).  vol_toc_lookup returns its VolDir
 * through the pointer in $a0 and gives it back in $v0: egcs's return of a
 * struct by value.  Not found: {&the empty TOC 0x00352F50, 0}.  C++ only. */
#ifndef GAME_T500_VOL_TOC_H
#define GAME_T500_VOL_TOC_H

#include "game/game.h"

#ifdef __cplusplus

/* The empty TOC that failed lookups point at (T500 defines it: 8 zero bytes
 * in T500's .sdata, -G8).  Declared before VolToc is complete: egcs decides
 * $gp-relative addressing at a variable's first declaration and cannot for
 * an object of incomplete type, so T500's code reaches it with lui / addiu
 * (retail's 0x0023DA50, 0x0023DC94) although it is in .sdata.  (Perhaps a
 * static member VolToc of VolToc in the original: its in-class declaration
 * is incomplete the same way.) */
struct VolToc;
extern VolToc g_00352F50;                       /* 0x00352F50 */

/* A TOC in memory (VolDevice +0x10) and its current directory.  The
 * constructor and the empty destructor are inline: T500's static constructor
 * of the empty TOC stores the two zeroes, its static destructor is empty,
 * and VolDevice's constructor builds it in place. */
struct VolToc {
    u8  *data;                                  /* 0x0 the TOC image (NULL: none) */
    u32  cwd;                                   /* 0x4 current directory record (0x14 = the root) */

    VolToc() : data(0), cwd(0) {}
    ~VolToc() {}
};
EGCS_SIZE(VolToc, 0x8);                         /* VolDevice: toc +0x10, the next member +0x18 */

/* A directory of a TOC: the TOC it is in and the offset of its record in the
 * TOC image.  vol_dir_root / vol_toc_lookup / vol_dir_lookup (T500) fill one;
 * its users: the car list (T446, u100), u059, u111. */
struct VolDir {
    VolToc *toc;                                /* 0x0 (its first word: the TOC image) */
    u32     off;                                /* 0x4 the directory record's offset in the image */
};
EGCS_SIZE(VolDir, 0x8);

/* An entry record of the TOC, at VolDir::off in VolToc::data (file_resolve,
 * T502, reads it). */
struct VolTocEntry {
    u8  unk_00[3];                              /* 0x0 (the name offset, 24 bits) */
    u8  type;                                   /* 0x3 VOL_ENTRY_*, else a file */
    u32 lsn;                                    /* 0x4 first sector, relative to the volume */
    u32 size;                                   /* 0x8 the (unpacked) size */
    u32 packedSize;                             /* 0xC the size of the packed data (VOL_ENTRY_PACKED) */
};
EGCS_SIZE(VolTocEntry, 0x10);
enum { VOL_ENTRY_DIR = 1, VOL_ENTRY_PACKED = 2 };   /* VolTocEntry::type: a directory, a gzip-packed file */

/* The entry record a lookup found.  Read through dir->toc->data each time:
 * file_resolve re-loads the image pointer after every store into its
 * FileLoc (they may alias). */
inline const VolTocEntry *vol_dir_entry(const VolDir *dir)
{
    return (const VolTocEntry *)(dir->toc->data + dir->off);
}

/* path from the root if it starts with '/', else from toc->cwd; "/"
 * restarts at the root, ".." goes to the parent (none: stays), no ".". */
VolDir *vol_toc_lookup(VolDir *out, VolToc *toc, const char *path);    /* 0x0023D8A8 */

/* T500's static destructor 0x0023DC88 (.dtors 0x0028F274: ~VolToc, nothing)
 * and initialiser 0x0023DC90 (.ctors 0x0028F1A0: VolToc()) of the empty TOC
 * g_00352F50 are compiler-generated (_GLOBAL_$D$ / _GLOBAL_$I$). */

#endif /* __cplusplus */

#endif /* GAME_T500_VOL_TOC_H */
