/* T526 - ISO 9660 lookup.  0x0025E100..0x0025E548.
 *
 * iso9660_find re-reads the primary volume descriptor (from lsn 16 on), sends
 * the disc identity {lsn, checksum} to the pdicdvd server, then walks the
 * path one component at a time from the root directory record, through the
 * sector cache.  The last component gets ";1" appended.  Users: vol_mount
 * (0x0023DE10), file_resolve (0x00240AF0). */
#include "engine/io/T526_iso9660.h"
#include "engine/pdi/pdi_internal.h"

#define ISO_FIRST_DESCRIPTOR_LSN 16     /* the volume descriptor set starts here */
#define ISO_PVD_ROOT_RECORD      0x9C   /* offset of the root directory record in the PVD */
#define ISO_FLAG_DIRECTORY       0x02   /* IsoDirRecord::flags */

/* Volume descriptor types (byte 0 of a descriptor) */
enum {
    ISO_VD_BOOT          = 0,
    ISO_VD_PRIMARY       = 1,
    ISO_VD_SUPPLEMENTARY = 2,
    ISO_VD_PARTITION     = 3,
    ISO_VD_TERMINATOR    = 0xFF
};

/* Labels of the four dates of a primary volume descriptor (creation,
 * modification, expiration, effective), padded to 12 characters.  No code
 * of the image reads them; in the original they follow
 * T524's type name (0x003459E0..0x00345A20), which no public data of T524
 * can do (egcs emits a file's vtables and type names after everything else
 * of its .rodata), so they open the next object's .rodata: this one's, as
 * the only file about volume descriptors.  A public const array (pdi_internal.h's
 * extern: a C++ const one without it has internal linkage, and egcs drops
 * it when unused). */
// DATA_START 0x003459E0
const char g_003459E0[4][16] = {        /* 0x003459E0 */
    "creation    ",
    "modification",
    "expiration  ",
    "effective   ",
};

/* File statics: retail has them in .bss / .sbss (a public C++ global without
 * an initialiser would be in .data). */
static IsoDirRecord g_004A75E0; /* 0x004A75E0 root directory record of the primary volume descriptor */
static PdiDiscId    g_00353298; /* 0x00353298 gp-0x4758 the disc identity sent to pdicdvd */

/* Weighted byte sum of a sector: sum of sector[i] * (i + 1) (madd on LO). */
// FUNC 0x0025E100 iso9660_checksum
int iso9660_checksum(const u8 *sector)
{
    int sum = 0;
    int i;

    for (i = 0; i < SECTOR_SIZE; i++)
        sum += sector[i] * (i + 1);
    return sum;
}

/* Skips leading '/' and '\\'.  Callers: iso9660_component_end,
 * iso9660_find_in_dir. */
// FUNC 0x0025E140 iso9660_skip_separators
const char *iso9660_skip_separators(const char *p)
{
    while (*p == '/' || *p == '\\')
        p++;
    return p;
}

/* End of the first path component (after the leading separators): the
 * next NUL, '/' or '\\'. */
// FUNC 0x0025E180 iso9660_component_end
const char *iso9660_component_end(const char *p)
{
    p = iso9660_skip_separators(p);
    while (*p != 0 && *p != '/' && *p != '\\')
        p++;
    return p;
}

/* Finds the primary volume descriptor, keeps its root directory record in
 * g_004A75E0 and sends {lsn, checksum} to the pdicdvd server.  Returns 1, or
 * 0 on a read error / a sector that is not "CD001" / the set terminator.
 * (s3 = 0xFF is loaded but unused: the `case ISO_VD_TERMINATOR', merged
 * with the default after its constant was hoisted.) */
// FUNC 0x0025E1F0 iso9660_read_pvd
int iso9660_read_pvd(void)
{
    u32 lsn;

    for (lsn = ISO_FIRST_DESCRIPTOR_LSN;; lsn++) {
        u8 *desc = sector_cache_read(lsn, 0);
        if (desc == NULL)
            return 0;
        if (strncmp((const char *)desc + 1, "CD001", 5) != 0)
            return 0;
        switch (desc[0]) {
        case ISO_VD_BOOT:
        case ISO_VD_SUPPLEMENTARY:
        case ISO_VD_PARTITION:
            continue;
        case ISO_VD_PRIMARY:
            g_004A75E0 = *(const IsoDirRecord *)(desc + ISO_PVD_ROOT_RECORD);
            g_00353298.pvdLsn = lsn;
            g_00353298.checksum = iso9660_checksum(desc);
            pdicdvd_set_disc_id(&g_00353298);
            return 1;
        case ISO_VD_TERMINATOR: /* its own case: the 0xFF the listing loads */
            return 0;
        default:
            return 0;
        }
    }
}

/* Searches the directory at lsn for the record called name (leading
 * separators skipped; strcasecmp against the record name at +0x21: nameLen
 * is not used, the compare relies on a zero byte after the name).  Returns
 * the record inside g_002FE280, or NULL at a zero-length record (the
 * padding at the end of a directory's sector).
 * When the records run to (or past) the end of the sector, the next sector
 * is read; the code meant to carry a partial record over computes
 * carry = last - end, which is negative there (? bug, kept):
 * memcpy(g_002FE280 - carry, last, carry), a huge size for memcpy. */
// FUNC 0x0025E320 iso9660_find_in_dir
IsoDirRecord *iso9660_find_in_dir(const char *name, u32 lsn)
{
    u8 *buf = g_002FE280;
    u8 *end = g_002FE280 + SECTOR_SIZE;
    u8 *rec = buf;
    u8 *last;
    const char *wanted = iso9660_skip_separators(name);

    for (;;) {
        /* through a void * local: a real memcpy call, as retail (egcs
         * expands a constant-size copy between known types) */
        void *sector = sector_cache_read(lsn, 1);
        memcpy(buf, sector, SECTOR_SIZE);
        last = rec;
        while (rec < end) {
            if (strcasecmp(wanted, ((IsoDirRecord *)rec)->name) == 0)
                return (IsoDirRecord *)rec;
            if (((IsoDirRecord *)rec)->len == 0)
                return NULL;
            last = rec;
            rec += ((IsoDirRecord *)rec)->len;
        }
        {
            int carry = last - end;
            lsn++;
            rec = buf - carry;
            memcpy(rec, last, carry);
        }
    }
}

/* Returns the directory record of path, or NULL (after F_0025B620; and
 * sector_cache_clear when the volume descriptor could not be read).
 * Intermediate components must be directories (flags bit 1).
 * Not exact (93.4%, the same with 991111-01): only registers; retail gives
 * lsn $s1, end $s2, more $s3 (here $s3, $s1, $s2: global-alloc ranks end and
 * more above lsn).  Tried: all 120 declaration orders, len / more / the
 * directory test as other forms, the lsn reads as packed fields. */
// FUNC 0x0025E3F8 iso9660_find
IsoDirRecord *iso9660_find(const char *path)
{
    char name[0x110];           /* the listing's frame has 0x110 bytes of locals */
    IsoDirRecord *rec;
    u32 lsn;

    if (iso9660_read_pvd()) {
        lsn = iso_read_u32(&g_004A75E0.lsn);
        for (;;) {
            const char *end = iso9660_component_end(path);
            int len = end - path;
            int more = *end != 0;

            strncpy(name, path, len);
            name[len] = 0;
            if (!more)
                strcat(name, ";1");
            rec = iso9660_find_in_dir(name, lsn);
            if (rec == NULL)
                break;
            if (!more)
                return rec;
            path = end;
            if (!(rec->flags & ISO_FLAG_DIRECTORY))
                break;
            lsn = iso_read_u32(&rec->lsn);
        }
    } else
        sector_cache_clear();
    F_0025B620();
    return NULL;
}

/* No callers.  sceCdSearchFile-like: out = {lsn, size}, returns 1 when
 * found, else 0 (out zeroed).
 * Not exact (87.5%): retail keeps rec in $a0 and reads the size straight
 * into $v1; here rec gets $v1 and the size read goes through $v0.  With
 * `return rec != NULL;' at the end rec gets $a0 (95.2%, an sltu instead of
 * retail's li 1); if / else, a found flag, locals, an inline helper: no. */
// FUNC 0x0025E4F0 iso9660_search_file
int iso9660_search_file(const char *path, IsoFileLoc *out)
{
    IsoDirRecord *rec;

    out->lsn  = 0;
    out->size = 0;
    rec = iso9660_find(path);
    if (rec == NULL)
        return 0;
    out->lsn  = iso_read_u32(&rec->lsn);
    out->size = iso_read_u32(&rec->size);
    return 1;
}
