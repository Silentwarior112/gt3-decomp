/* core decomp: copied from bootstrap/include/sifdev.h. Address comments in it are the
 * bootstrap's (0x010xxxxx); the core addresses of shared code are in ref/libmatch.json. */
/* sifdev.h - Sony EE fileio client (libkernl fileio.o, IOP FILEIO RPC server
 * 0x80000001, completion via SIF command 0x80000011) as linked into the GT3 US
 * bootstrap (BOOTSTRAP_SCUS-97102).  Sony SDK 2.2-era names.
 *
 * Owners: kernl_rpc_fio (sdk/libkernl/fileio.c: 0x0101E9B0..0x01020978) and
 * kernl_rest (sdk/libkernl/filedev.c: sceDelDrv..sceReadlink).
 *
 * Callers outside libkernl: ImageLoader Host/DiskSource (sceOpen/sceLseek/
 * sceRead/sceClose on "host:" and "cdrom0:" paths), the io unit's host_*
 * wrappers (0x0100F6D0..0x0100F858), main (sceFsReset).  Everything from
 * sceDelDrv on has no caller in the binary.
 *
 * Return convention: >= 0 success, negative -errno on failure
 * (-1 not initialised, -EBADF bad fd, -EINVAL size checks, ...).
 */
#ifndef BOOT_SIFDEV_H
#define BOOT_SIFDEV_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- sceOpen flags ----------------------------------------------------- */
#define SCE_RDONLY      0x0001
#define SCE_WRONLY      0x0002
#define SCE_RDWR        0x0003
#define SCE_NBLOCK      0x0010
#define SCE_APPEND      0x0100
#define SCE_CREAT       0x0200
#define SCE_TRUNC       0x0400
#define SCE_EXCL        0x0800
#define SCE_NOBUF       0x4000
#define SCE_NOWAIT      0x8000      /* async op: tracked in the pending table @0x0103DA40 */
/* EE-side-only bit kept in the fd slot's mode word (never sent to the IOP). */
#define SCE_NOWBDC      0x20000000  /* ? name from the mapping pass: skip D-cache write-back */

/* ---- sceLseek whence --------------------------------------------------- */
#define SCE_SEEK_SET    0
#define SCE_SEEK_CUR    1
#define SCE_SEEK_END    2

/* ---- sce_stat.st_mode -------------------------------------------------- */
/* memory-card style attribute bits */
#define SCE_STM_R       0x0001
#define SCE_STM_W       0x0002
#define SCE_STM_X       0x0004
#define SCE_STM_C       0x0008
#define SCE_STM_F       0x0010      /* file */
#define SCE_STM_D       0x0020      /* directory */
/* generic file-type field */
#define SCE_STM_FMT     (0xF << 12)
#define SCE_STM_FLNK    (0x4 << 12)
#define SCE_STM_FREG    (0x2 << 12)
#define SCE_STM_FDIR    (0x1 << 12)

/* ---- sceChstat change bits --------------------------------------------- */
#define SCE_CST_MODE    0x0001
#define SCE_CST_ATTR    0x0002
#define SCE_CST_SIZE    0x0004
#define SCE_CST_CT      0x0008
#define SCE_CST_AT      0x0010
#define SCE_CST_MT      0x0020
#define SCE_CST_PRVT    0x0040

/* File status: 0x40 bytes (the completion handler copies 0x40 for fno 12). */
struct sce_stat {
    unsigned int  st_mode;        /* +0x00 */
    unsigned int  st_attr;        /* +0x04 */
    unsigned int  st_size;        /* +0x08 low 32 bits */
    unsigned char st_ctime[8];    /* +0x0C */
    unsigned char st_atime[8];    /* +0x14 */
    unsigned char st_mtime[8];    /* +0x1C */
    unsigned int  st_hisize;      /* +0x24 high 32 bits */
    unsigned int  st_private[6];  /* +0x28 */
};                                /* 0x40 */

/* Directory entry: the completion handler copies the full 0x144 bytes
 * (0x140 in 32-byte blocks + a final word, so d_private too) for fno 11.    */
struct sce_dirent {
    struct sce_stat d_stat;       /* +0x000 */
    char            d_name[256];  /* +0x040 */
    void           *d_private;    /* +0x140 */
};                                /* 0x144 */

#ifdef __cplusplus
static_assert(sizeof(struct sce_stat) == 0x40, "sce_stat");
static_assert(sizeof(struct sce_dirent) == 0x144, "sce_dirent");
#else
_Static_assert(sizeof(struct sce_stat) == 0x40, "sce_stat");
_Static_assert(sizeof(struct sce_dirent) == 0x144, "sce_dirent");
#endif

/* ---- API (RPC fno in brackets) ------------------------------------------ */
int   sceFsReset(void);                                                   /* 0x0101F180 (main) */
/* Variadic like Sony's: the body spills a2..t3 and reads the mode from the
 * first va slot.  flag = SCE_RDONLY... ; fd is an index into the 32-slot table. */
int   sceOpen(const char *filename, int flag, ...);                       /* 0x0101F1B8 [0] */
int   sceClose(int fd);                                                   /* 0x0101F440 [1] */
int   sceRead(int fd, void *buf, int nbyte);                              /* 0x0101F7F8 [2] */
int   sceWrite(int fd, const void *buf, int nbyte);                       /* 0x0101FA68 [3] */
int   sceLseek(int fd, int offset, int whence);                           /* 0x0101F5C0 [4] */
int   sceIoctl(int fd, int req, void *arg);                               /* 0x0101FD28 [5] */
int   sceRemove(const char *filename);                                    /* 0x010203F8 [6] */
int   sceMkdir(const char *dirname, int mode);                            /* 0x01020418 [7] */
int   sceRmdir(const char *dirname);                                      /* 0x010205C8 [8] */
int   sceDopen(const char *dirname);                                      /* 0x01020998 [9] */
int   sceDclose(int fd);                                                  /* 0x01020A38 [10] */
int   sceDread(int fd, struct sce_dirent *buf);                           /* 0x01020BA0 [11] */
int   sceGetstat(const char *name, struct sce_stat *buf);                 /* 0x01020CF8 [12] */
int   sceChstat(const char *name, struct sce_stat *buf, unsigned int cbit); /* 0x01020E98 [13] */
int   sceFormat(const char *path, const char *blockdev, void *arg, int arglen); /* 0x010205E8 [14] */
int   sceAddDrv(int arg);                                                 /* 0x01020858 [15] ? Sony arg type */
int   sceDelDrv(const char *devname);                                     /* 0x01020978 [16] */
int   sceRename(const char *oldname, const char *newname);                /* 0x010210D8 */
int   sceChdir(const char *name);                                         /* 0x010212C8 */
int   sceSync(const char *devname, int flag);                             /* 0x010212E8 */
int   sceMount(const char *fsdevname, const char *blockdevname, int flag,
               void *arg, int arglen);                                    /* 0x01021480 */
int   sceUmount(const char *fsdevname);                                   /* 0x010216F0 */
s64   sceLseek64(int fd, s64 offset, int whence);                         /* 0x01021710 (Sony: long, 64-bit on ee-gcc) */
int   sceDevctl(const char *devname, int cmd, void *arg, unsigned int arglen,
                void *bufp, unsigned int buflen);                         /* 0x01021948 */
int   sceSymlink(const char *existing, const char *newname);              /* 0x01021B78 */
int   sceReadlink(const char *path, char *buf, unsigned int bufsize);     /* 0x01021D58 */
int   sceIoctl2(int fd, int request, void *argp, unsigned int arglen,
                void *bufp, unsigned int buflen);                         /* 0x01020078 */

/* ------------------------------------------------------------------------ */
/* fileio internals shared by the kernl_rpc_fio and kernl_rest units.      */
/* ------------------------------------------------------------------------ */
/*
 * Shared state (defined in kernl_rpc_fio; addresses for both units):
 *   0x0103DA40  int  pending[32]      async (SCE_NOWAIT) table, -1 = free (.data)
 *   0x0103DAC0  int  init flag        (_fio_init done)
 *   0x0103DAC4  int  lock sema        (_fio_lock / _fio_unlock)
 *   0x0103DAC8  int  fd-table sema    (_fio_fd_alloc / _fio_fd_to_slot)
 *   0x0103DACC  int  pending-table sema
 *   0x0104C300       RPC send buffer  {+0 completion sema id (negated = async),
 *                                      +4 result ptr, +8 result size, args from +0xC;
 *                                      path fields 0x400 bytes}
 *   0x0104CF40       RPC recv buffer  (read through |0x20000000)
 *   0x0104CF80       completion packet buffer (_fio_intr), result @0x0104CF90
 *   0x0104D3C0  _fio_slot fd table[32]
 *   0x0104D5C0  sceSifClientData  FILEIO client
 *   0x0104D5E8  int  IOP fileio version (compared with "2240" @0x0103D284)
 */
typedef struct _fio_slot {
    int           iop_fd;   /* +0x00 IOP-side descriptor                                  */
    unsigned int  mode;     /* +0x04 0 = free, 0x10000000 = reserved by _fio_fd_alloc,
                                     else sceOpen flags (| SCE_NOWAIT, | SCE_NOWBDC)       */
    unsigned int  _08;      /* +0x08 ? not seen accessed                                   */
    unsigned int  _0C;      /* +0x0C ?                                                     */
} _fio_slot;                /* 0x10; EE fd = slot index = (slot - table) >> 4              */

_fio_slot *_fio_fd_alloc(void);                                           /* 0x0101EA10 NULL if full */
_fio_slot *_fio_fd_to_slot(int fd);                                       /* 0x0101EA98 NULL if fd >= 32 */
int        _fio_lock(int op);                                             /* 0x0101EF10 (op unused; WaitSema, returns 0) */
void       _fio_unlock(void);                                             /* 0x0101EF40 (tail SignalSema) */
int        _fio_init(void);                                               /* 0x0101EF50 bind + cmd handler */
/* Single-path command helper; callers: sceRemove, sceRmdir (kernl_rpc_fio),
 * sceDelDrv, sceDopen, sceChdir, sceUmount (kernl_rest).                    */
int        _fio_pathcmd(const char *path, int fno);                       /* 0x01020250 */

/* ---- (appended by kernl_rpc_fio) names for the shared state above -------- */
/* IOP FILEIO RPC function numbers (same order as ps2sdk fileio-common.h;
 * 17..26 are the iomanX-era extensions; 0xFF is the EE-side setup call).    */
#define FIO_F_OPEN       0
#define FIO_F_CLOSE      1
#define FIO_F_READ       2
#define FIO_F_WRITE      3
#define FIO_F_LSEEK      4
#define FIO_F_IOCTL      5
#define FIO_F_REMOVE     6
#define FIO_F_MKDIR      7
#define FIO_F_RMDIR      8
#define FIO_F_DOPEN      9
#define FIO_F_DCLOSE     10
#define FIO_F_DREAD      11
#define FIO_F_GETSTAT    12
#define FIO_F_CHSTAT     13
#define FIO_F_FORMAT     14
#define FIO_F_ADDDRV     15
#define FIO_F_DELDRV     16
#define FIO_F_RENAME     17
#define FIO_F_CHDIR      18
#define FIO_F_SYNC       19
#define FIO_F_MOUNT      20
#define FIO_F_UMOUNT     21
#define FIO_F_LSEEK64    22
#define FIO_F_DEVCTL     23
#define FIO_F_SYMLINK    24
#define FIO_F_READLINK   25
#define FIO_F_IOCTL2     26
#define FIO_F_SETUP      0xFF   /* _fio_init: send completion-buffer address, receive IOP version */

#define FIO_SERVER_ID      0x80000001   /* IOP FILEIO RPC server */
#define FIO_SLOT_RESERVED  0x10000000   /* _fio_slot.mode between _fio_fd_alloc and open completion */

/* Head of every request built in _fio_cmd_buf (op arguments follow at +0x0C). */
typedef struct {
    int    sema;         /* +0x00 per-call completion sema (CreateSema max 1 init 0);
                                  negated when recorded in _fio_pending (SCE_NOWAIT) */
    void  *result;       /* +0x04 EE address _fio_intr copies the result to (caller's stack) */
    int    result_size;  /* +0x08 4 for every op in kernl_rpc_fio                 */
} _fio_rpc_hdr;

extern volatile int _fio_pending[32];    /* 0x0103DA40 .data, all -1 = free */
extern int          _fio_initialized;    /* 0x0103DAC0 */
extern int          _fio_lock_sema;      /* 0x0103DAC4 (-1 until created) */
extern int          _fio_fd_sema;        /* 0x0103DAC8 (-1 until created) */
extern int          _fio_pending_sema;   /* 0x0103DACC (-1 until created) */
extern u8           _fio_cmd_buf[0xC40]; /* 0x0104C300 RPC send buffer (64-aligned); starts with
                                            _fio_rpc_hdr; largest request sceFormat 0xC10 */
extern int          _fio_recv_buf[0x10]; /* 0x0104CF40 RPC receive buffer; word 0 (read through
                                            |0x20000000) = IOP accept flag, 0 = rejected */
extern _fio_slot    _fio_slots[32];      /* 0x0104D3C0 EE fd table */
extern struct _sif_client_data _fio_client; /* 0x0104D5C0 bound to FIO_SERVER_ID */
/* Alternative spellings used briefly during integration (both units were
 * written concurrently); they resolve to the same objects.                 */
#define _fio_inited    _fio_initialized
#define _fio_sbuf      _fio_cmd_buf
#define _fio_rbuf      _fio_recv_buf
#define _fio_fd_table  _fio_slots
#define _fio_cd        _fio_client
/* Notes from the decompilation (kernl_rpc_fio):
 *  - _fio_intr copies the FULL 0x144-byte sce_dirent for fno 11 (0x140 bytes
 *    in 32-byte blocks plus a final word, so d_private is transferred too).
 *  - The completion block @0x0104CF80 (static in fileio.c) is
 *    {+0 sema, +4 fno, +8 dest, +0xC size, +0x10 result, +0x14 per-op data};
 *    sceIoctl(fd, 2/3) reads the last result from 0x0104CF90 (32/64-bit).
 *  - SCE_NOWAIT registration (lseek/read/write, and sceLseek64): under
 *    _fio_pending_sema put the request's hdr.sema into the first -1 slot of
 *    _fio_pending and negate hdr.sema; after an accepted RPC just
 *    DeleteSema and return 0.                                              */

#ifdef __cplusplus
}
#endif

#endif /* BOOT_SIFDEV_H */
