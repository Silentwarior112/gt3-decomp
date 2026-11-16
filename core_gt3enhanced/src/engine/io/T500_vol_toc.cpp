/* T500 - the gt3.vol table of contents: decoding and path lookup.
 * 0x0023D7E0..0x0023DCA8.
 *
 * Path lookup works on VolDir {toc, record offset}.  vol_toc_lookup and
 * vol_dir_lookup return their VolDir through the pointer in $a0 and give it
 * back in $v0: egcs's return of a struct by value, so the originals were
 * probably `VolDir VolToc::lookup(const char *)' / `VolDir
 * VolDir::lookup(const char *) const' (?); vol_dir_root likewise looks like
 * a VolDir(VolToc *) constructor (it returns `this'). */
#include "engine/io/vol_internal.h"
#include "engine/io/T500_vol_toc.h"

/* The empty TOC (.sdata 0x00352F50, 8 zero bytes: egcs writes a public
 * object with a constructor at its definition, under -G8 in .sdata).  Its
 * first declaration (engine/io/T500_vol_toc.h, before VolToc is complete) keeps
 * the code's lui / addiu.  Its static destructor and initialiser are
 * compiler-generated, at the end of the unit. */
VolToc g_00352F50;                              /* 0x00352F50 */

// FUNC 0x0023D7E0 vol_toc_decrypt
//   A TOC stored with the magic ~"RoFS" (AD 90 B9 AC) has its body inverted:
//   flip every byte of [data + hdr[3], data + hdr[3] + hdr[4]) and write the
//   plain magic "RoFS".  Called by vol_mount when vol_toc_is_plain fails.
void vol_toc_decrypt(VolToc *toc)
{
    u8 *data = toc->data;
    if (data[0] == 0xAD && data[1] == 0x90 && data[2] == 0xB9 && data[3] == 0xAC) {
        u8 *p = data + ((u32 *)data)[3];            /* header size */
        u8 *end = p + ((u32 *)data)[4];             /* body size */
        for (; p < end; p++)
            *p = ~*p;
        toc->data[0] = 'R';
        toc->data[1] = 'o';
        toc->data[2] = 'F';
        toc->data[3] = 'S';
    }
}

// FUNC 0x0023D890 vol_dir_root
//   The root directory (record 0x14) of `toc'.
VolDir *vol_dir_root(VolDir *dir, VolToc *toc)
{
    dir->toc = toc;
    dir->off = VOL_ROOT_DIR;
    return dir;
}

// FUNC 0x0023D8A8 vol_toc_lookup
//   Looks `path' up in `toc': from the root when it starts with '/', else
//   from the TOC's current directory.  Callers: course_table_init (courses/US),
//   car_list_load (cars/day), file_resolve.
VolDir *vol_toc_lookup(VolDir *out, VolToc *toc, const char *path)
{
    VolDir start;                               /* sp+0x00 */
    if (path[0] == '/') {
        vol_dir_root(&start, toc);
        vol_dir_lookup(out, &start, path + 1);
    } else {
        start.toc = toc;
        start.off = toc->cwd;
        vol_dir_lookup(out, &start, path);
    }
    return out;
}

/* The TOC word at byte offset `off' (the out-of-line copy is T510's
 * vol_toc_word): an inline helper in the original.  Its parameter is a
 * register, so the address is computed off + data (retail's operand order),
 * and an entry's offset dir->off + 0xC + 4 * index goes in whole. */
static inline u32 vol_rec_word(const VolToc *toc, int off)
{
    return *(u32 *)(toc->data + off);
}

// FUNC 0x0023D920 vol_dir_lookup
//   Resolves `path' relative to `dir', one component per recursion level.
//   "/" restarts at the root, ".." goes to the parent (record +8; the root's
//   parent 0 stays at the directory itself), any other component is found by
//   vol_dir_find.  Not found: {&g_00352F50, 0}.  No "." handling.
VolDir *vol_dir_lookup(VolDir *out, const VolDir *dir, const char *path)
{
    VolDir next;                                /* sp+0x00 the directory of the next level */

    if (path[0] == '/') {
        vol_dir_root(&next, dir->toc);
        vol_dir_lookup(out, &next, path + 1);
    } else {
        const char *end = path;
        if (*end != 0 && *end != '/') {
            do
                end++;
            while (*end != 0 && *end != '/');
        }
        if (end - path == 2 && path[0] == '.' && path[1] == '.') {
            VolToc *toc = dir->toc;
            if (*end == 0) {
                u32 parent = vol_rec_word(toc, dir->off + 8);
                if (parent == 0) {
                    out->toc = toc;
                    out->off = dir->off;
                } else {
                    out->toc = toc;
                    out->off = parent;
                }
            } else {
                u32 parent = vol_rec_word(toc, dir->off + 8);
                if (parent == 0) {
                    next.toc = toc;
                    next.off = dir->off;
                } else {
                    next.toc = toc;
                    next.off = parent;
                }
                vol_dir_lookup(out, &next, end + 1);
            }
        } else {
            int index = vol_dir_find(dir, path, end);
            if (index < 0) {
                out->off = 0;
                out->toc = &g_00352F50;
            } else if (*end == 0) {
                VolToc *toc = dir->toc;
                out->toc = toc;
                out->off = vol_rec_word(toc, dir->off + 0xC + index * 4);
            } else {
                VolToc *toc = dir->toc;
                next.toc = toc;
                next.off = vol_rec_word(toc, dir->off + 0xC + index * 4);
                vol_dir_lookup(out, &next, end + 1);
            }
        }
    }
    return out;
}

// FUNC 0x0023DAE0 vol_name_cmp
//   Compares the name [name, nameEnd) with the NUL-terminated `s': 0 equal,
//   1 when `s' ends first, -1 when [name, nameEnd) ends first, else the
//   difference of the first differing bytes (unsigned).
int vol_name_cmp(const u8 *name, const u8 *nameEnd, const char *s)
{
    while (name != nameEnd) {
        if (*s == 0)
            return 1;
        int diff = *name - (*s & 0xFF);
        name++;
        if (diff != 0)
            return diff;
        s++;
    }
    return *s == 0 ? 0 : -1;
}

// FUNC 0x0023DB28 vol_dir_find
//   Index of the entry of `dir' named [begin, end), -1 if none: binary
//   search while the range is 8 or more entries wide, then a linear scan of
//   [lo, hi].  Entry i: record dir->off + 0xC + 4i holds the entry record's
//   offset; that record's low 24 bits locate its name.
int vol_dir_find(const VolDir *dir, const char *begin, const char *end)
{
    u32 hi = vol_rec_word(dir->toc, dir->off + 4) - 2;     /* last index (the word is count + 1) */
    u32 lo = 0;

    while (hi - lo >= 8) {
        u8 *data = dir->toc->data;
        u32 mid = (hi + lo) >> 1;
        u32 entry = vol_rec_word(dir->toc, dir->off + 0xC + mid * 4);
        int cmp = vol_name_cmp((const u8 *)begin, (const u8 *)end,
                               (const char *)(data + (*(u32 *)(data + entry) & 0xFFFFFF)));
        if (cmp == 0)
            return mid;
        if (cmp < 0)
            hi = mid;
        else
            lo = mid;
    }
    while (lo <= hi) {
        u8 *data = dir->toc->data;
        u32 entry = vol_rec_word(dir->toc, dir->off + 0xC + lo * 4);
        if (vol_name_cmp((const u8 *)begin, (const u8 *)end,
                         (const char *)(data + (*(u32 *)(data + entry) & 0xFFFFFF))) == 0)
            return lo;
        lo++;
    }
    return -1;
}

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_00352F50 (.dtors entry 0x0028F274, .ctors entry 0x0028F1A0), emitted at
 * the end of the unit, the destructor first.  egcs names them after the
 * file's first public object assembled, the empty TOC defined above. */
// FUNC 0x0023DC88 T500_static_destroy (_GLOBAL_$D$g_00352F50)
/* implicit: ~VolToc (empty) of g_00352F50 */

// FUNC 0x0023DC90 T500_static_init (_GLOBAL_$I$g_00352F50)
/* implicit: VolToc() of g_00352F50: data = 0, cwd = 0 */
