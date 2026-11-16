/* libkernl_part1_internal.h - unit u140 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x002869E8..0x0028A950: Sony libkernl 2240 (PsIIlibkernl2240), C side,
 * part of the no-FDE stretch G007.  The code is the bootstrap's
 * decomp/bootstrap/sdk/libkernl/{kernel,deci2_tty,sifcmd,sifrpc,fileio}.c:
 * ref/libmatch.json pairs 64 of the 71 functions; the other seven
 * (sce_printf_deci2, sceSifGetSreg, sceSifSetCmdBuffer and sceSifBindRpc,
 * whose funcs.json ranges hold a second function; sceSifWriteBackDCache and
 * its stray `jr $ra'; _deci2_tty_write) were read against the listing.
 *
 * Link (object) order in the core differs from the bootstrap's: tty.o (the
 * DECI2 TTYP console driver) sits between thread.o and the SIO/printf
 * object here, while deci2.o (the Deci2Call glue, 0x0028D778..) is after
 * initsys.o, outside this unit.  Modules of this unit:
 *   thread.c   0x002869E8  thread.o
 *   tty.c      0x00286D48  tty.o
 *   kprintf.c  0x00287268  SIO putc / kernel printf
 *   sifcmd.c   0x00287BE0  sifcmd.o (set_sreg .. sceSifWriteBackDCache)
 *   sifrpc.c   0x00288328  sifrpc.o
 *   fileio_part1.c   0x00289288  fileio.o (up to sceIoctl)
 *
 * The functions other units call, the syscall stubs, the deci2.o stubs and
 * libc are declared by runtime/newlib/G007_stdio.h (the canonical header of G007, core
 * addresses).  The Sony SDK headers it and this header include (eekernel.h,
 * sif.h, sifdev.h, libc.h) declare the types, the constants and the rest of
 * the libkernl API with the BOOTSTRAP's addresses; the core addresses of
 * this unit's functions declared only there:
 *   eekernel.h  iRotateThreadReadyQueue 0x00286C30, iSuspendThread 0x00286CB0
 *   sif.h       sceSifGetSreg 0x00287C10, sceSifSetSreg 0x00287C28,
 *               sceSifGetDataTable 0x00287C48, sceSifInitCmd 0x00287C58,
 *               sceSifExitCmd 0x00287ED8, sceSifSetCmdBuffer 0x00287F10,
 *               sceSifSetSysCmdBuffer 0x00287F28, sceSifSendCmd 0x002880D0,
 *               isceSifSendCmd 0x00288110, _rpc_get_packet 0x002884F0,
 *               _rpc_packet_free 0x00288598, _rpc_get_fpacket 0x002885B8,
 *               _rpc_get_fpacket2 0x002885E8, sceSifGetOtherData 0x00288740,
 *               _request_call 0x00288AD8, sceSifRemoveRpc 0x00288F00,
 *               sceSifRemoveRpcQueue 0x00288F98, sceSifGetNextRequest
 *               0x00289028, sceSifExecRequest 0x00289080 (and the
 *               struct of sif_rpc_state, sifrpc's static at 0x004AF940)
 *   sifdev.h    sceIoctl 0x0028A600
 * The file-local helpers (topThread, the tty ring and handler, deci2_sink,
 * _dtoi, _print_double, sce_doprnt, change_addr, set_sreg, _sif_cmd_send,
 * _sif_cmd_intr_handler, _request_end/_rdata/_bind, _search_svdata and
 * four fileio helpers) are `static', as in the bootstrap's sources (and
 * ps2sdk's libkernel.a has topThread, change_addr and set_sreg as local
 * symbols); each file declares its own.
 *
 * Data (ref/data/u140/notes.md): each file defines its object's data, as
 * file statics except fileio.o's, whose .data the second half (u141) shares
 * (globals) and whose .bss is C commons (see that file).
 *
 * Outside calls hidden in the C: double arithmetic in _print_double goes to
 * libgcc's fp-bit (runtime/libgcc/G005_dp_bit.h: dpsub 0x00279418, dpmul 0x00279480,
 * dpdiv 0x00279728, dpcmp 0x002799A8; fptodp 0x0027AFA0 in sce_doprnt;
 * __fixunsdfdi 0x0028D9B8), 64-bit division in sce_doprnt to __divdi3
 * 0x00276FD0, __moddi3 0x002788E0, __udivdi3 0x0028DAA8, __umoddi3
 * 0x0028E078.  memcmp is 0x0027D8FC (symbols.csv: "memcpy_small").
 */
#ifndef U140_H
#define U140_H

#include <stddef.h>
#include <stdarg.h>
#include "types.h"
#include "libc.h"                       /* EBADF, EAGAIN, ENODEV */
#include "eekernel.h"                   /* ThreadParam, SemaParam, DMAC_SIF0, WRITEBACK_DCACHE,
                                           ExitHandler; DIntr / EIntr; iRotate- / iSuspendThread */
#include "runtime/newlib/G007_stdio.h"            /* own TU: sif.h, sifdev.h, the syscall stubs, deci2.o,
                                           memcpy / memset / memcmp, _libkernl_version,
                                           _fio_ioctl_arg, the shared fileio state and this
                                           unit's outside-called API */
#include "runtime/startup/G000_collect2_ctors.h"   /* _gp, the top thread's gpReg */

/* ======================================================================== */
/* Hardware registers                                                       */
/* ======================================================================== */

#define SIO_ISR         0x1000F130  /* EE SIO (debug UART) status                 */
#define SIO_ISR_TXFULL  0x8000      /* TX FIFO full                               */
#define SIO_TXFIFO      0x1000F180  /* EE SIO transmit FIFO (byte)                */
#define D_STAT          0x1000E010  /* DMAC interrupt status (write 1 clears)     */
#define D_STAT_SIF0     0x20        /* channel 5 (SIF0, IOP -> EE)                 */
#define D5_CHCR         0x1000C000  /* DMAC channel 5 CHCR                         */
#define CHCR_STR        0x100       /* channel running                             */

/* ======================================================================== */
/* thread.o - interrupt-safe thread operations via a "top" server thread    */
/* ======================================================================== */

/* Kernel limits (ps2sdk kernel.h): the queued thread id / priority is a byte. */
#define MAX_THREADS     256
#define MAX_PRIORITY    128

/* _top_request.mode: what topThread does with .data */
#define TOP_REQ_WAKEUP  0           /* WakeupThread(data)           */
#define TOP_REQ_ROTATE  1           /* RotateThreadReadyQueue(data) */
#define TOP_REQ_SUSPEND 2           /* SuspendThread(data)          */

#define TOP_QUEUE_LEN   512         /* request ring; the indices are masked on use */
#define TOP_QUEUE_MASK  (TOP_QUEUE_LEN - 1)

struct _top_request {
    u8 mode;                          /* +0 TOP_REQ_*                            */
    u8 data;                          /* +1 thread id / priority (8 bits)        */
};
struct _top_arg {
    u32                 requestOut;   /* +0x000 read index (masked on use)       */
    u32                 requestIn;    /* +0x004 write index (masked on use)      */
    struct _top_request request[TOP_QUEUE_LEN]; /* +0x008                       */
};                                    /* 0x408 */
_Static_assert(sizeof(struct _top_arg) == 0x408, "_top_arg");

/* thread.o's data, file statics of thread.c: topStack
 * 0x004AD260 (.bss, 16-aligned), topSema 0x004AD660, topArg 0x004AD668
 * (.bss), topId 0x003081B8 (.data, initially 0). */

/* ======================================================================== */
/* tty.o - DECI2 TTYP (protocol 0x210) console driver                       */
/* ======================================================================== */

/* DECI2 handler events (Sony deci2.h). */
#define DECI2_READ       1
#define DECI2_READDONE   2
#define DECI2_WRITE      3
#define DECI2_WRITEDONE  4

#define DECI2_PROTO_TTYP 0x210
#define DECI2_NODE_EE    'E'
#define DECI2_NODE_HOST  'H'

/* DECI2 packet header. */
typedef struct {
    u16 length;         /* +0x00 total length incl. headers */
    u16 reserved;       /* +0x02 */
    u16 protocol;       /* +0x04 */
    u8  source;         /* +0x06 */
    u8  destination;    /* +0x07 */
} sceDeci2Hdr;          /* 0x08 */

#define TTY_MAX_PAYLOAD  0x100      /* text bytes per TTYP packet          */
#define TTY_RING_SIZE    0x100      /* console input ring                  */
#define TTY_BUF_SIZE     0x140      /* send / receive packet buffers       */

/* TTYP packet: DECI2 header + 4-byte TTYP sub-header + text. */
typedef struct {
    sceDeci2Hdr h;                  /* +0x00 */
    u32  ttyp_info;                 /* +0x08 always 0 */
    u8   data[TTY_MAX_PAYLOAD];     /* +0x0C */
} TtyPacket;                        /* 0x10C */
#define TTY_HDR_SIZE     ((int)offsetof(TtyPacket, data))   /* 12: DECI2 + TTYP headers */

/* Console input ring (filled by the handler, drained by read()). */
typedef struct {
    int           size;             /* +0x00 capacity (TTY_RING_SIZE) */
    volatile int  count;            /* +0x04 bytes available (read() spins on it) */
    u8           *rp;               /* +0x08 read pointer */
    u8           *wp;               /* +0x0C write pointer */
    u8            data[TTY_RING_SIZE]; /* +0x10 */
} TtyRing;                          /* 0x110 */

/* TTY state - passed to sceDeci2Open as the handler's `opt'.  Volatility per
 * field as in the bootstrap source (sd/wlen/rlen/busy are re-read).        */
typedef struct {
    volatile int sd;    /* +0x00 DECI2 socket from sceDeci2Open */
    volatile int wlen;  /* +0x04 bytes of the current packet still to send */
    volatile int rlen;  /* +0x08 bytes of the incoming packet received so far */
    volatile int busy;  /* +0x0C send in flight; cleared by the handler */
    u8      *wbuf;      /* +0x10 send cursor (uncached mirror of s_tty_sendbuf) */
    u8      *rbuf;      /* +0x14 receive buffer (uncached mirror of s_tty_recvbuf) */
    TtyRing *ring;      /* +0x18 */
} TtyCtx;               /* 0x1C */

_Static_assert(sizeof(TtyRing) == 0x110, "TtyRing");
_Static_assert(sizeof(TtyCtx) == 0x1C, "TtyCtx");
_Static_assert(offsetof(TtyPacket, data) == 0x0C, "TtyPacket");

/* tty.o's data, file statics of tty.c (.bss): s_tty_ring
 * 0x004ADA80, s_tty 0x004ADB90, s_tty_sendbuf 0x004ADBC0 and s_tty_recvbuf
 * 0x004ADD00 (both 64-aligned). */

/* ======================================================================== */
/* SIO putc / kernel printf                                                 */
/* ======================================================================== */

#define KPUTS_BUF_SIZE  0x80
/* The object's data, file statics of kprintf.c:
 * _kputs_count 0x003081BC (.data, initially 0), _putc_hook 0x003081C0
 * (.data, the printf engine's output hook, initially sio_putchar_crlf),
 * _kputs_buf[KPUTS_BUF_SIZE] 0x004ADE40 (.bss). */

int  sio_putchar(int c);                                      /* 0x00287268 (bootstrap sio_putc) */
/* The kernel printf through the CURRENT hook (the bootstrap's _printf). */
void sce_printf_sio(const char *fmt, ...);                    /* 0x00287B48 */

/* ======================================================================== */
/* sifcmd.o                                                                 */
/* ======================================================================== */

#define SIF_NR_SYS_HANDLERS  32         /* _sif_sys_cmd_handlers[]          */
#define SIF_NR_SREGS         32         /* software SIF registers           */
#define SIF_CMD_PKT_MIN      0x10       /* packet sizes _sif_cmd_send takes */
#define SIF_CMD_PKT_MAX      0x70

/* _sif_cmd_data (ps2sdk's sif_cmd_main.o calls the object _sif_cmd_data
 * too).  sceSifGetDataTable returns its address. */
struct _sif_cmd_data {
    void          *pktbuf;            /* +0x00 _sif_pktbuf|UC: IOP DMAs its packets here */
    void          *cmdbuf;            /* +0x04 _sif_cmdbuf|UC (never used by this code)  */
    void          *iopbuf;            /* +0x08 IOP-side command buffer (SIF SUBADDR)     */
    sceSifCmdData *sys_cmd_handlers;  /* +0x0C _sif_sys_cmd_handlers                     */
    int            nr_sys_handlers;   /* +0x10 SIF_NR_SYS_HANDLERS                       */
    sceSifCmdData *usr_cmd_handlers;  /* +0x14 NULL until sceSifSetCmdBuffer             */
    int            nr_usr_handlers;   /* +0x18 0                                         */
    unsigned int  *sregs;             /* +0x1C _sif_sregs                                */
};                                    /* 0x20 */
_Static_assert(sizeof(struct _sif_cmd_data) == 0x20, "_sif_cmd_data");

/* Handshake packet: header + EE receive-buffer address (0x14 bytes). */
struct _sif_ca_pkt {
    sceSifCmdHdr  chdr;               /* +0x00 */
    void         *buf;                /* +0x10 */
};
_Static_assert(sizeof(struct _sif_ca_pkt) == 0x14, "_sif_ca_pkt");

/* sifcmd.o's data, file statics of sifcmd.c: .bss
 * _sif_pktbuf 0x004ADEC0, _sif_cmdbuf 0x004ADF40, _sif_init_pkt 0x004ADF80
 * (the three 64-aligned), _sif0_handler_id 0x004ADF94, _sif_cmd_data
 * 0x004ADF98, _sif_sys_cmd_handlers 0x004ADFC0, _sif_sregs 0x004AE0C0;
 * .data _sif_cmd_inited 0x003081C4 (initially 0). */

/* ======================================================================== */
/* sifrpc.o                                                                 */
/* ======================================================================== */

#define RPC_POOL_SLOTS   0x20               /* packets per pool                      */
#define RPC_PKT_SIZE     0x40               /* every RPC packet: sizeof(sceSifRpcPkt) */
/* sceSifRpcPkt.rec_id: (slot << RPC_REC_SLOT_SHIFT) | flags */
#define RPC_REC_BUSY       0x01             /* slot in use / request in flight       */
#define RPC_REC_HAS_SLOT   0x04             /* slot index valid: the server answers
                                               into the client-table slot of the
                                               same index (sceSifExecRequest)        */
#define RPC_REC_SLOT_SHIFT 16

/* sifrpc.o's own views of what its system handlers read and build (the
 * field TYPES are the ones the bootstrap source chose to reproduce egcs's
 * alias-based schedule; the layouts are sif.h's).                          */
typedef struct {                /* SIF_CMDC_RPC_END (IOP <-> EE)            */
    sceSifCmdHdr  chdr;
    int           rec_id;       /* +0x10 */
    unsigned int  pkt_addr;     /* +0x14 requester's packet                 */
    int           rpc_id;       /* +0x18 */
    unsigned int  client;       /* +0x1C sceSifClientData / sceSifReceiveData */
    unsigned int  cid;          /* +0x20 command that completed             */
    void         *serve;        /* +0x24 */
    void         *buff;         /* +0x28 */
    void         *cbuff;        /* +0x2C */
} _sif_rpc_end_pkt;

typedef struct {                /* SIF_CMDC_RPC_RDATA                       */
    sceSifCmdHdr        chdr;
    int                 rec_id;     /* +0x10 */
    void               *pkt_addr;   /* +0x14 */
    int                 rpc_id;     /* +0x18 */
    sceSifReceiveData  *receive;    /* +0x1C */
    unsigned int        src;        /* +0x20 */
    unsigned int        dest;       /* +0x24 */
    int                 size;       /* +0x28 */
} _sif_rpc_rdata_pkt;

typedef struct {                /* sceSifClientData as _request_end writes it */
    sceSifRpcData  rpcd;        /* +0x00 */
    unsigned int   command;     /* +0x10 */
    void          *buff;        /* +0x14 */
    void          *cbuff;       /* +0x18 */
    sceSifEndFunc  func;        /* +0x1C */
    void          *para;        /* +0x20 */
    void          *serve;       /* +0x24 */
} _sif_rpc_client;

_Static_assert(sizeof(_sif_rpc_end_pkt) == 0x30, "_sif_rpc_end_pkt");
_Static_assert(offsetof(_sif_rpc_end_pkt, cbuff) == offsetof(sceSifRpcRendPkt, cbuff), "end.cbuff");
_Static_assert(offsetof(_sif_rpc_rdata_pkt, size) == offsetof(sceSifRpcOtherDataPkt, size), "rdata.size");
_Static_assert(sizeof(_sif_rpc_client) == sizeof(sceSifClientData), "_sif_rpc_client");
_Static_assert(sizeof(sceSifRpcPkt) == RPC_PKT_SIZE, "sceSifRpcPkt");
_Static_assert(offsetof(sceSifClientData, serve) == 0x24, "ClientData.serve");
_Static_assert(offsetof(sceSifServeData, link) == 0x38, "ServeData.link");
_Static_assert(offsetof(sceSifQueueData, next) == 0x14, "QueueData.next");
_Static_assert(offsetof(struct _sif_rpc_state, active_queue) == 0x28, "rpc_state.active_queue");

/* sifrpc.o's data, file statics of sifrpc.c: .bss the
 * packet pools (RPC_POOL_SLOTS x 0x40 bytes, used through their uncached
 * views, 64-aligned) _rpc_pkt_pool 0x004AE140, _rpc_rdata_pool 0x004AE940,
 * _rpc_client_pool 0x004AF140, then sif_rpc_state 0x004AF940 (its struct is
 * sif.h's); .data _sif_rpc_inited 0x003081C8 (initially 0). */

/* ======================================================================== */
/* fileio.o                                                                 */
/* ======================================================================== */

/* libkernl's 16-byte NUL-less version tag "PsIIlibkernl2240" (G007_stdio.h:
 * _libkernl_version); fileio compares the IOP FILEIO version against its
 * last four bytes.                                                         */
#define FIO_LIB_VERSION   (&_libkernl_version[12])    /* 0x00307A0C "2240" */

#define FIO_NR_SLOTS      32        /* _fio_slots[]: EE fds 0..31            */
#define FIO_NR_PENDING    32        /* _fio_pending[]                        */
#define FIO_PENDING_FREE  (-1)      /* free _fio_pending entry               */
/* FIO_PATH_MAX / FIO_DATA_MAX (0x400) and the extended error codes
 * SCE_ECALLMISS (_fio_init) / SCE_EVERSIONMISS (sceOpen's IOP FILEIO version
 * check): runtime/newlib/G007_stdio.h, shared with u141. */
#define FIO_FLAG_EE_ONLY  0xF0000000    /* sceOpen flag bits kept on the EE (SCE_NOWBDC,
                                           FIO_SLOT_RESERVED); stripped from the request */

/* The request layouts in _fio_cmd_buf, after the _fio_rpc_hdr (sifdev.h);
 * FIO_F_IOCTL's is _fio_ioctl_arg (G007_stdio.h).  ps2sdk's fileio-common.h
 * has the same argument fields, without the header and the efd.           */
struct _fio_open_arg {              /* FIO_F_OPEN, 0x418 bytes */
    _fio_rpc_hdr  h;                /* +0x000 */
    int           flag;             /* +0x00C flag & ~FIO_FLAG_EE_ONLY */
    int           mode;             /* +0x010 */
    char          path[FIO_PATH_MAX]; /* +0x014 */
    int           efd;              /* +0x414 EE fd (slot index) */
};

struct _fio_close_arg {             /* FIO_F_CLOSE, 0x14 bytes */
    _fio_rpc_hdr  h;
    int           fd;               /* +0x0C IOP fd */
    int           efd;              /* +0x10 */
};

struct _fio_read_arg {              /* FIO_F_READ, 0x20 bytes */
    _fio_rpc_hdr  h;
    int           fd;               /* +0x0C */
    void         *buf;              /* +0x10 */
    int           nbyte;            /* +0x14 */
    void         *read_data;        /* +0x18 ps2sdk: the EE address of the head/tail
                                       block (_fio_read_data); never written here,
                                       FIO_F_SETUP announced the completion block */
    int           efd;              /* +0x1C */
};

struct _fio_write_arg {             /* FIO_F_WRITE, 0x30 bytes */
    _fio_rpc_hdr  h;
    int           fd;               /* +0x0C */
    const void   *buf;              /* +0x10 */
    int           nbyte;            /* +0x14 */
    int           head_len;         /* +0x18 bytes up to the next 16-byte boundary (ps2sdk: mis) */
    u8            head[16];         /* +0x1C those bytes, sent inline (ps2sdk: aligned) */
    int           efd;              /* +0x2C */
};

struct _fio_lseek_arg {             /* FIO_F_LSEEK, 0x1C bytes */
    _fio_rpc_hdr  h;
    int           fd;               /* +0x0C */
    int           offset;           /* +0x10 */
    int           whence;           /* +0x14 */
    int           efd;              /* +0x18 */
};

_Static_assert(sizeof(struct _fio_open_arg)   == 0x418, "open arg");
_Static_assert(sizeof(struct _fio_close_arg)  == 0x14,  "close arg");
_Static_assert(sizeof(struct _fio_read_arg)   == 0x20,  "read arg");
_Static_assert(sizeof(struct _fio_write_arg)  == 0x30,  "write arg");
_Static_assert(sizeof(struct _fio_lseek_arg)  == 0x1C,  "lseek arg");

/* Completion block written by the IOP (read only through |0x20000000). */
struct _fio_intr_hdr {
    int           sema;             /* +0x00 request's hdr.sema (negative = SCE_NOWAIT op) */
    int           fno;              /* +0x04 */
    void         *dest;             /* +0x08 request's hdr.result */
    int           size;             /* +0x0C request's hdr.result_size */
};

/* FIO_F_READ: the unaligned head/tail bytes (ps2sdk fileio-common.h
 * _fio_read_data {size1, size2, dest1, dest2, buf1, buf2}, "the updated
 * modules ... changed the size of the buffers from 16 to 64").            */
struct _fio_read_fixup {
    int           head_len;         /* +0x00 */
    int           tail_len;         /* +0x04 */
    u8           *head_dst;         /* +0x08 */
    u8           *tail_dst;         /* +0x0C */
    u8            head[0x40];       /* +0x10 */
    u8            tail[0x40];       /* +0x50 */
};

struct _fio_intr_data {
    struct _fio_intr_hdr hdr;       /* +0x000 @0x004B0640 */
    int           result;           /* +0x010 @0x004B0650 result (64-bit for sceIoctl 3) */
    union {                         /* +0x014 @0x004B0654 per-op extra data */
        struct _fio_read_fixup read;                                      /* fno 2 */
        struct { struct sce_dirent *dst; struct sce_dirent ent; } dread;  /* fno 11 */
        struct { struct sce_stat *dst; struct sce_stat st; } getstat;     /* fno 12 */
        struct { void *dst; unsigned int len; u8 data[FIO_DATA_MAX]; } buf; /* fno 23/25/26 */
    } u;
};

_Static_assert(offsetof(struct _fio_intr_data, u.read.tail) == 0x14 + 0x50, "read fixup");
_Static_assert(offsetof(struct _fio_intr_data, u.dread.ent) == 0x18, "dread ent");
_Static_assert(offsetof(struct _fio_intr_data, u.buf.data) == 0x1C, "buf data");

typedef union {
    struct _fio_intr_data d;
    u8                    raw[0x440];
} _fio_intr_block;

typedef struct { u8   b[FIO_DATA_MAX]; } _fio_blk400;  /* sceIoctl's inline 1 KB block copy */
typedef struct { char v[4]; }     _fio_version_t;     /* copied with lwl/lwr (4-byte inline copy) */

/* The state shared with u141 (_fio_pending, _fio_initialized, the three
 * semas, _fio_cmd_buf, _fio_recv_buf, _fio_slots, _fio_client) is declared
 * by G007_stdio.h with its core addresses; fileio_part1.c
 * defines it.  fileio.o's own (defined there too; _fio_alt_version is a file
 * static, the .bss ones commons, see the file):                            */
extern void           *_fio_ioctl_argp;    /* 0x004AF980 .bss sceIoctl's argp       */
extern _fio_intr_block _fio_intr_buf;      /* 0x004B0640 .bss (64-aligned) completion block
                                              (ps2sdk: _fio_intr_data)              */
extern _fio_version_t  _fio_iop_version;   /* 0x004B0CA8 .bss IOP FILEIO version    */
extern u8              _fio_intr_harg[0x50]; /* 0x004B0CB0 .bss ? size and use unknown:
                                                only its address is passed, as
                                                _fio_intr's (ignored) harg; 0x50
                                                runs to iopheap.o's .bss       */
/* _fio_alt_version 0x00308260 (.data, static) -> "...." 0x00349358.        */

#endif /* U140_H */
