/* libkernl_part2_internal.h - unit u141 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x0028A950..0x0028D7C0, the tail of the no-FDE stretch G007: Sony libkernl
 * 2240 (PsIIlibkernl2240), the same library version as the bootstrap's.
 * Modules, in link order:
 *   fileio.o, second half  sceIoctl2 .. sceReadlink     fileio_part2.c
 *   iopheap.o              sceSifInitIopHeap ..         iopheap.c
 *                          (not linked into the bootstrap)
 *   loadfile.o             _sceSifLoadFileInit ..       loadfile.c
 *   iopreboot.o            sceSifResetIop .. RebootIop  iopreboot.c
 *   diei.o / initsys.o     DIntr .. _InitSys            initsys.c
 *   deci2.o                sceDeci2Open                 deci2_part1.c
 * 45 of the 47 functions are the bootstrap's code
 * (decomp/bootstrap/sdk/libkernl/); the other two are iopheap.o and
 * _sceSifLoadModuleBuffer (same logic, other code).
 *
 * runtime/newlib/G007_stdio.h (the canonical header of G007, core addresses) declares
 * this unit's functions that other units call (sceSifInitIopHeap,
 * sceSifLoadFileReset, sceSifLoadModule, sceSifSyncIop, sceSifRebootIop,
 * DIntr, EIntr, _InitSys, sceDeci2Open), everything it calls outside the
 * unit except printf (engine/pdistd/T533_printf.h), and the fileio state shared with
 * u140.  The Sony SDK headers (sifdev.h, sif.h, eekernel.h) declare the
 * types, the constants and the rest of this unit's API with the BOOTSTRAP's
 * addresses; the core addresses of the functions declared only there:
 *   sifdev.h  sceIoctl2 0x0028A950, _fio_pathcmd 0x0028AB28, sceRemove
 *             0x0028ACD0, sceMkdir 0x0028ACF0, sceRmdir 0x0028AEA0,
 *             sceFormat 0x0028AEC0, sceAddDrv 0x0028B130, sceDelDrv
 *             0x0028B250, sceDopen 0x0028B270, sceDclose 0x0028B310,
 *             sceDread 0x0028B478, sceGetstat 0x0028B5D0, sceChstat
 *             0x0028B770, sceRename 0x0028B9B0, sceChdir 0x0028BBA0,
 *             sceSync 0x0028BBC0, sceMount 0x0028BD58, sceUmount 0x0028BFC8,
 *             sceLseek64 0x0028BFE8, sceDevctl 0x0028C220, sceSymlink
 *             0x0028C450, sceReadlink 0x0028C630
 *   sif.h     sceSifAllocIopHeap 0x0028C880, sceSifFreeIopHeap 0x0028C8F0,
 *             sceSifLoadIopHeap 0x0028C968,
 *             sceSifLoadModuleBuffer 0x0028CE28, sceSifLoadStartModuleBuffer
 *             0x0028CE48, sceSifLoadStartModule 0x0028D0B0, sceSifLoadElfPart
 *             0x0028D1D8, sceSifLoadElf 0x0028D1F8, sceSifGetIopAddr
 *             0x0028D220, sceSifSetIopAddr 0x0028D310, sceSifResetIop
 *             0x0028D3F0, sceSifIsAliveIop 0x0028D520
 * The loadfile helpers (_sceSifLoadFileInit, _sceSifCheckLoadFileVersion,
 * _sceSifLoadModuleBuffer, _sceSifLoadModule, _sceSifLoadElfPart), initsys's
 * clear_user_syscalls and its private SetSyscall stub are file-local.
 *
 * Names, layouts and constants follow ps2sdk where it has them
 * (fileio-common.h, loadfile-common.h, loadfile.h, iopheap-common.h,
 * kernel.h); the listing wins where they differ.
 */
#ifndef U141_H
#define U141_H

#include <stddef.h>
#include "types.h"
#include "libc.h"               /* EBADF, EAGAIN, ENODEV, EINVAL */
#include "eekernel.h"           /* SemaParam, EE_COP0_STATUS_EIE, EI() */
#include "runtime/newlib/G007_stdio.h"    /* own TU: sif.h, sifdev.h, the syscall stubs,
                                   SIF RPC, u140's fileio helpers and state,
                                   _fio_ioctl_arg, memcmp / memcpy / memset /
                                   strncpy, _libkernl_version, InitAlarm /
                                   InitThread, tty_reinit_after_iop_reset and
                                   this unit's outside-called API */
#include "engine/pdistd/T533_printf.h"   /* printf 0x0026C690 (sceSifRebootIop) */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef E2BIG
#define E2BIG 7                 /* sceFormat / sceMount: option data too long */
#endif

/* Body of an empty busy-wait loop (Sony's nopdelay(), inlined; ps2sdk
 * kernel.h's nopdelay() is the same 0x100000-pass loop with its nops written
 * out).  egcs keeps an empty loop as written; GCC 14 would delete it, so
 * there it gets an empty volatile asm.  The four nops of the binary's loop
 * are the R5900 short-loop erratum padding.                                */
#ifdef BOOT_OLD_GCC
#define SPIN_BODY()
#else
#define SPIN_BODY()     __asm__ __volatile__("")
#endif
#define BIND_RETRY_SPIN 0x100000        /* delay between two bind attempts */

/* ======================================================================== */
/* fileio.o (second half)                                                   */
/* ======================================================================== */

/* FIO_PATH_MAX / FIO_DATA_MAX (0x400): G007_stdio.h.                       */

/* The requests built in _fio_cmd_buf, after the _fio_rpc_hdr (sifdev.h);
 * FIO_F_IOCTL2's is _fio_ioctl_arg (G007_stdio.h).  ps2sdk's
 * fileio-common.h names the dread / getstat / chstat ones the same way (its
 * structures hold only the argument fields, with 256-byte paths).          */
struct _fio_path_arg {          /* remove, rmdir, deldrv, dopen, chdir, umount */
    _fio_rpc_hdr  h;                    /* +0x000 */
    char          path[FIO_PATH_MAX];   /* +0x00C only strlen + 1 bytes sent */
};

struct _fio_mkdir_arg {         /* FIO_F_MKDIR */
    _fio_rpc_hdr  h;                    /* +0x000 */
    int           mode;                 /* +0x00C */
    char          path[FIO_PATH_MAX];   /* +0x010 only strlen + 1 bytes sent */
};

struct _fio_format_arg {        /* FIO_F_FORMAT, 0xC10 bytes */
    _fio_rpc_hdr  h;                    /* +0x000 */
    char          dev[FIO_PATH_MAX];    /* +0x00C device to format (sceFormat's path) */
    char          blockdev[FIO_PATH_MAX]; /* +0x40C "" when NULL */
    u8            arg[FIO_DATA_MAX];    /* +0x80C */
    int           arglen;               /* +0xC0C */
};

struct _fio_adddrv_arg {        /* FIO_F_ADDDRV, 0x10 bytes */
    _fio_rpc_hdr  h;                    /* +0x000 */
    int           device;               /* +0x00C IOP address of the device
                                           descriptor the IOP passes to its
                                           AddDrv (ps2sdk ioman.h:
                                           AddDrv(iop_device_t *)) */
};

struct _fio_dclose_arg {        /* FIO_F_DCLOSE, 0x14 bytes sent */
    _fio_rpc_hdr  h;                    /* +0x000 */
    int           fd;                   /* +0x00C IOP fd */
    int           unused10;             /* +0x010 sent, never written (sceClose's
                                           request has its efd here) */
};

struct _fio_dread_arg {         /* FIO_F_DREAD, 0x20 bytes sent */
    _fio_rpc_hdr  h;                    /* +0x000 */
    int           fd;                   /* +0x00C IOP fd */
    struct sce_dirent *buf;             /* +0x010 _fio_intr copies the entry here */
    u8            unused14[0xC];        /* +0x014 sent, never written */
};

struct _fio_getstat_arg {       /* FIO_F_GETSTAT */
    _fio_rpc_hdr  h;                    /* +0x000 */
    struct sce_stat *buf;               /* +0x00C _fio_intr copies the stat here */
    u8            name[FIO_PATH_MAX];   /* +0x010 only strlen + 1 bytes sent; unsigned
                                           (the copy loop tests the byte without
                                           sign extension, unlike the char paths) */
};

struct _fio_chstat_arg {        /* FIO_F_CHSTAT */
    _fio_rpc_hdr  h;                    /* +0x000 */
    unsigned int  cbit;                 /* +0x00C fields of stat to change */
    struct sce_stat stat;               /* +0x010 */
    u8            name[FIO_PATH_MAX];   /* +0x050 only strlen + 1 bytes sent; unsigned
                                           as in _fio_getstat_arg */
};

struct _fio_rename_arg {        /* FIO_F_RENAME and FIO_F_SYMLINK, 0x80C bytes */
    _fio_rpc_hdr  h;                    /* +0x000 */
    char          oldname[FIO_PATH_MAX];  /* +0x00C (symlink: the existing name) */
    char          newname[FIO_PATH_MAX];  /* +0x40C */
};

struct _fio_sync_arg {          /* FIO_F_SYNC, 0x414 bytes */
    _fio_rpc_hdr  h;                    /* +0x000 */
    int           unused0C;             /* +0x00C sent, never written */
    int           flag;                 /* +0x010 */
    char          dev[FIO_PATH_MAX];    /* +0x014 */
};

struct _fio_mount_arg {         /* FIO_F_MOUNT, 0xC14 bytes */
    _fio_rpc_hdr  h;                    /* +0x000 */
    char          fsdev[FIO_PATH_MAX];  /* +0x00C */
    char          blockdev[FIO_PATH_MAX]; /* +0x40C */
    u8            arg[FIO_DATA_MAX];    /* +0x80C */
    int           flag;                 /* +0xC0C */
    int           arglen;               /* +0xC10 */
};

struct _fio_lseek64_arg {       /* FIO_F_LSEEK64, 0x20 bytes */
    _fio_rpc_hdr  h;                    /* +0x000 */
    int           fd;                   /* +0x00C IOP fd */
    s64           offset;               /* +0x010 */
    int           whence;               /* +0x018 */
    int           efd;                  /* +0x01C EE fd (slot index) */
};

struct _fio_devctl_arg {        /* FIO_F_DEVCTL, 0x81C bytes */
    _fio_rpc_hdr  h;                    /* +0x000 */
    char          dev[FIO_PATH_MAX];    /* +0x00C */
    u8            arg[FIO_DATA_MAX];    /* +0x40C */
    int           cmd;                  /* +0x80C */
    unsigned int  arglen;               /* +0x810 */
    void         *bufp;                 /* +0x814 _fio_intr copies the output here */
    unsigned int  buflen;               /* +0x818 */
};

struct _fio_readlink_arg {      /* FIO_F_READLINK, 0x414 bytes (0x80C sent) */
    _fio_rpc_hdr  h;                    /* +0x000 */
    unsigned int  bufsize;              /* +0x00C */
    char         *buf;                  /* +0x010 _fio_intr copies the link here */
    char          path[FIO_PATH_MAX];   /* +0x014 */
};

_Static_assert(sizeof(struct _fio_format_arg)   == 0xC10, "format arg");
_Static_assert(sizeof(struct _fio_adddrv_arg)   == 0x10,  "adddrv arg");
_Static_assert(sizeof(struct _fio_dclose_arg)   == 0x14,  "dclose arg");
_Static_assert(sizeof(struct _fio_dread_arg)    == 0x20,  "dread arg");
_Static_assert(offsetof(struct _fio_chstat_arg, name) == 0x50, "chstat arg");
_Static_assert(sizeof(struct _fio_chstat_arg)   == 0x450, "chstat arg");
_Static_assert(sizeof(struct _fio_rename_arg)   == 0x80C, "rename arg");
_Static_assert(sizeof(struct _fio_sync_arg)     == 0x414, "sync arg");
_Static_assert(sizeof(struct _fio_mount_arg)    == 0xC14, "mount arg");
_Static_assert(sizeof(struct _fio_lseek64_arg)  == 0x20,  "lseek64 arg");
_Static_assert(sizeof(struct _fio_devctl_arg)   == 0x81C, "devctl arg");
_Static_assert(offsetof(struct _fio_readlink_arg, path) == 0x14, "readlink arg");

/* The IOP's immediate reply word (non-zero = request accepted), read through
 * the uncached mirror between sceSifCallRpc() and _fio_unlock() (as u140's
 * FIO_RPC_ACK).                                                            */
#define FIO_RPC_ACK()   (*(int *)UNCACHED(_fio_recv_buf))

/* Copy a path of at most FIO_PATH_MAX bytes including the NUL; len ends as
 * strlen(src), or FIO_PATH_MAX when no NUL was found (callers then force
 * dst[FIO_PATH_MAX - 1] = 0).                                              */
#define FIO_COPY_PATH(dst, src, len)                        \
    for ((len) = 0; (len) < FIO_PATH_MAX; (len)++) {        \
        if (((dst)[len] = (src)[len]) == 0)                 \
            break;                                          \
    }

/* Fresh binary semaphore: only maxCount/initCount/option are written.     */
#define FIO_SEMA_PARAM(sp) \
    ((sp).maxCount = 1, (sp).initCount = 0, (sp).option = 0)

/* ======================================================================== */
/* iopheap.o (IOP heap RPC server 0x80000003) - not in the bootstrap        */
/* ======================================================================== */
#define IH_SERVER_ID    0x80000003
#define IH_F_ALLOC      1       /* function numbers (descriptive names;     */
#define IH_F_FREE       2       /*  ps2sdk's iopheap uses the same numbers  */
#define IH_F_LOAD       3       /*  without naming them)                    */
#define IH_PATH_MAX     0xFC    /* ps2sdk iopheap-common.h LIH_PATH_MAX     */

typedef struct {                /* IH_F_LOAD request (ps2sdk _iop_load_heap_arg) */
    void *addr;                         /* +0x00 IOP heap address to load into */
    char  path[IH_PATH_MAX];            /* +0x04 */
} _ih_load_arg_t;

/* iopheap.o's state, file statics of iopheap.c: _ih_bind_state
 * (0x00308264 .data), _ih_client (0x004B0D00 .bss), _ih_recv, _ih_alloc_arg,
 * _ih_free_arg, _ih_load_arg (0x004B0D40 / 0x004B0D80 / 0x004B0DC0 /
 * 0x004B0E00 .bss, 64-byte-aligned slots).                                 */

/* ======================================================================== */
/* loadfile.o (IOP LOADFILE RPC server 0x80000006)                          */
/* ======================================================================== */
#define LF_SERVER_ID    0x80000006

/* Function numbers (ps2sdk loadfile-common.h enum _lf_functions).          */
#define LF_F_MOD_LOAD       0
#define LF_F_ELF_LOAD       1
#define LF_F_SET_ADDR       2
#define LF_F_GET_ADDR       3
#define LF_F_MOD_BUF_LOAD   6
#define LF_F_GET_VERSION    0xFF

/* Access widths of LF_F_SET_ADDR / LF_F_GET_ADDR (ps2sdk enum _lf_val_types). */
#define LF_VAL_BYTE     0
#define LF_VAL_SHORT    1
#define LF_VAL_LONG     2

/* Error codes, returned negated (ps2sdk loadfile.h has 0x10000, 0x10001 and
 * 0x10003; SCE_ECALLMISS 0x10001 and SCE_EVERSIONMISS 0x10004, shared with
 * fileio.o, are in G007_stdio.h).                                          */
#define SCE_EBINDMISS     0x10000   /* LOADFILE server bind failed          */
#define SCE_EBADWIDTH     0x10002   /* ? name (not in ps2sdk loadfile.h):
                                       peek/poke width not LF_VAL_*         */
#define SCE_ELOADMISS     0x10003   /* ELF load returned epc 0              */

#define LF_PATH_MAX     0xFC    /* ps2sdk loadfile-common.h */
#define LF_ARG_MAX      0xFC

/* LOADFILE transfer buffer (send AND receive), 0x200 bytes.  The arms are
 * ps2sdk loadfile-common.h's request structures (same layouts, same field
 * names): the first words are the arguments on the way in and the results
 * on the way out.                                                          */
typedef union {
    struct {                            /* LF_F_MOD_LOAD (_lf_module_load_arg) */
        union {
            int arg_len;                /* +0x000 in: bytes of args          */
            int result;                 /* +0x000 out: module id             */
        } p;
        int  modres;                    /* +0x004 out: module start() result */
        char path[LF_PATH_MAX];         /* +0x008                            */
        char args[LF_ARG_MAX];          /* +0x104                            */
    } mod;
    struct {                            /* LF_F_MOD_BUF_LOAD (_lf_module_buffer_load_arg) */
        union {
            const void *ptr;            /* +0x000 in: IRX image in IOP memory */
            int result;                 /* +0x000 out: module id             */
        } p;
        union {
            int arg_len;                /* +0x004 in: bytes of args          */
            int modres;                 /* +0x004 out: module start() result */
        } q;
        char unused[LF_PATH_MAX];       /* +0x008                            */
        char args[LF_ARG_MAX];          /* +0x104                            */
    } buf;
    struct {                            /* LF_F_ELF_LOAD (_lf_elf_load_arg)  */
        u32  epc;                       /* +0x000 out: entry point, 0 = failed */
        u32  gp;                        /* +0x004 out                        */
        char path[LF_PATH_MAX];         /* +0x008                            */
        char secname[LF_ARG_MAX];       /* +0x104                            */
    } elf;
    struct {                            /* LF_F_SET_ADDR / LF_F_GET_ADDR (_lf_iop_val_arg) */
        union {
            u32 iop_addr;               /* +0x000 in                         */
            union { u8 b; u16 s; u32 l; } result;  /* +0x000 out (GET_ADDR): the value */
        } p;
        int  type;                      /* +0x004 LF_VAL_*                   */
        union { u8 b; u16 s; u32 l; } val;  /* +0x008 SET_ADDR: value to write */
    } val;
    char version[4];                    /* LF_F_GET_VERSION reply, e.g. "2240" */
    u8   raw[0x200];
} _lf_xfer_t;

_Static_assert(sizeof(_lf_xfer_t) == 0x200, "loadfile transfer buffer");
_Static_assert(offsetof(_lf_xfer_t, mod.args) == 0x104, "mod.args");
_Static_assert(offsetof(_lf_xfer_t, buf.q) == 0x004, "buf.q");
_Static_assert(offsetof(_lf_xfer_t, val.val) == 0x008, "val.val");

typedef struct { char c[4]; } _lf_ver_t;   /* copied with lwl/lwr + swl/swr */

/* loadfile.o's state, file statics of loadfile.c: _lf_bind_state
 * (0x00308268 .data), _lf_version_wildcard (0x0030826C .data -> "...."
 * 0x00349360), _lf_xfer (0x004B0F00 .bss, 64-aligned), _lf_client
 * (0x004B1100 .bss), _lf_iop_version (0x004B1128 .bss).                   */

/* libkernl's own version digits: the tail of the NUL-less version tag
 * "PsIIlibkernl2240" (G007_stdio.h: _libkernl_version, 0x00307A00).        */
#define LIBKERNL_VERSION_DIGITS (&_libkernl_version[12])     /* 0x00307A0C "2240" */

/* ======================================================================== */
/* iopreboot.o                                                              */
/* ======================================================================== */
#define IOP_REBOOT_PREFIX   "rom0:UDNL "    /* 0x00349370: the IOP's UDNL loader */
#define IOP_RESET_ARG_MAX   0x50            /* sceSifCmdResetData.arg[] */

/* The SIF_CMDC_RESET_CMD packet DMA'd to the IOP's SIF command buffer,
 * _sif_reset_pkt (0x004B1140 .bss): iopreboot.o's own static, defined in
 * iopreboot.c with aligned(64) (as the bootstrap's iopreboot.c).  */

/* ======================================================================== */
/* deci2.o                                                                  */
/* ======================================================================== */
/* Deci2Call function number of sceDeci2Open (the rest of deci2.o, in u142,
 * uses 2 .. 9, negated in interrupt context, and 0x10 for kputs).          */
#define DECI2_CALL_OPEN 1

/* Kernel-side status words handed (uncached) to every sceDeci2Open:
 * s_deci2_kstatus (0x004B11A8 .bss), a file static of deci2_part1.c.  */

#ifdef __cplusplus
}
#endif

#endif /* U141_H */
