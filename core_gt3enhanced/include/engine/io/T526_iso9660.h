/* T526_iso9660.h - IsoDirRecord and the ISO 9660 path lookup over the sector
 * cache (translation unit T526, unit u119). */
#ifndef GAME_T526_ISO9660_H
#define GAME_T526_ISO9660_H

#include "game/game.h"

/* An ISO 9660 directory record, as on the disc: byte aligned (the u32 fields
 * are read with lwl / lwr).  The name is nameLen bytes long. */
typedef struct IsoDirRecord {
    u8   len;                   /* 0x00 */
    u8   extAttrLen;            /* 0x01 */
    u32  lsn;                   /* 0x02 extent (little endian) */
    u32  lsnBE;                 /* 0x06 */
    u32  size;                  /* 0x0A data length (little endian) */
    u32  sizeBE;                /* 0x0E */
    u8   date[7];               /* 0x12 */
    u8   flags;                 /* 0x19 bit 1: directory */
    u8   unitSize;              /* 0x1A */
    u8   gapSize;               /* 0x1B */
    u16  volSeq;                /* 0x1C */
    u16  volSeqBE;              /* 0x1E */
    u8   nameLen;               /* 0x20 */
    char name[1];               /* 0x21 */
} __attribute__((packed)) IsoDirRecord;
EGCS_SIZE(IsoDirRecord, 0x22);

/* A 32-bit little-endian field of a record (byte aligned), e.g.
 * iso_read_u32(&rec->size).  The original read them with an inline lwl /
 * lwr pair of two memory operands: in retail the lwl and the lwr of one read
 * can have different base registers (file_resolve 0x00240D5C, vol_mount,
 * iso9660_find: `addiu a0,a1,0xA; lwl v0,3(a0); lwr v0,0xA(a1)`), which
 * egcs never emits for a C read (a packed field is one ulw: both halves on
 * one base; a block move too).  Hand-written in the original, as the
 * bootstrap's asm helpers (MATCHING.md): egcs only; GCC 14 assembles the
 * bytes.  A volatile asm (u119's matching pass): retail never merges two
 * reads of one field (vol_mount, file_resolve read the size twice), and
 * iso9660_find / iso9660_search_file schedule nothing across a read
 * (iso9660_find's `lui' of ";1" stays after the root lsn read, in the
 * loop-entry `b''s delay slot).  vol_mount and file_resolve score the same. */
static __inline__ u32 iso_read_u32(const void *p)
{
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
    u32 v;
    __asm__ __volatile__("lwl %0,%1\n\tlwr %0,%2"
            : "=&r"(v)
            : "m"(*((const u8 *)p + 3)), "m"(*(const u8 *)p));
    return v;
#else
    const u8 *b = (const u8 *)p;
    return (u32)b[0] | ((u32)b[1] << 8) | ((u32)b[2] << 16) | ((u32)b[3] << 24);
#endif
}

/* What iso9660_search_file returns (sceCdSearchFile-like). */
typedef struct IsoFileLoc {
    u32 lsn;                    /* 0x0 */
    u32 size;                   /* 0x4 */
} IsoFileLoc;
EGCS_SIZE(IsoFileLoc, 8);

/* Weighted byte sum of a sector: sum of sector[i] * (i + 1). */
int  iso9660_checksum(const u8 *sector);                    /* 0x0025E100 */
const char *iso9660_skip_separators(const char *p);         /* 0x0025E140 skips leading '/' and '\\' */
const char *iso9660_component_end(const char *p);           /* 0x0025E180 end of the first path component */
/* Finds the primary volume descriptor (from lsn 16), keeps its root
 * directory record and sends {lsn, checksum} to pdicdvd.  1 = found. */
int  iso9660_read_pvd(void);                                /* 0x0025E1F0 */
/* The record called name in the directory at lsn (in the 0x800-byte buffer
 * 0x002FE280), NULL at a zero-length record. */
IsoDirRecord *iso9660_find_in_dir(const char *name, u32 lsn); /* 0x0025E320 */
/* The directory record of path (";1" appended to its last component), NULL
 * when not found. */
IsoDirRecord *iso9660_find(const char *path);               /* 0x0025E3F8 */
/* out = {lsn, size} of path; 1 = found, else 0 (out zeroed). */
int  iso9660_search_file(const char *path, IsoFileLoc *out); /* 0x0025E4F0 */

#endif /* GAME_T526_ISO9660_H */
