/* core decomp: copied from bootstrap/include/sif.h. Address comments in it are the
 * bootstrap's (0x010xxxxx); the core addresses of shared code are in ref/libmatch.json. */
/* sif.h - Sony EE SIF interface (libkernl, PsIIlibkernl2240) as linked into
 * the GT3 US bootstrap (BOOTSTRAP_SCUS-97102): SIF DMA syscalls, SIF command
 * layer, SIF RPC, loadfile, IOP reboot, IOP heap.
 *
 * Sony SDK 2.2-era names (Sony's sif.h / sifcmd.h / sifrpc.h / sifdev.h split
 * is merged here; the fileio client is in sifdev.h).  Structure layouts are
 * confirmed against the binary (offsets in comments); ps2sdk's sifcmd.h /
 * sifrpc.h use the same wire formats under different names.
 *
 * Owners: kernl_sys (syscall stubs, sifcmd, first half of sifrpc up to
 * 0x0101E0C0), kernl_rpc_fio (rest of sifrpc from sceSifBindRpc), kernl_rest
 * (loadfile, IOP reboot).
 */
#ifndef BOOT_SIF_H
#define BOOT_SIF_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================== */
/* SIF DMA (syscalls 0x76..0x7A, 0x6B)                                      */
/* ======================================================================== */

typedef struct {
    unsigned int data;    /* +0x00 EE source address                      */
    unsigned int addr;    /* +0x04 IOP destination address                */
    unsigned int size;    /* +0x08 bytes                                   */
    unsigned int mode;    /* +0x0C SIF_DMA_* (0x44 = ERT|INT_O for cmd pkts) */
} sceSifDmaData;          /* 0x10 */

#define SIF_DMA_INT_I   0x02
#define SIF_DMA_INT_O   0x04
#define SIF_DMA_ERT     0x40

unsigned int sceSifSetDma(sceSifDmaData *sdd, int len);     /* 0x0101BFA0  syscall 0x77 */
unsigned int isceSifSetDma(sceSifDmaData *sdd, int len);    /* 0x0101BFB0  syscall -0x77 */
int   sceSifDmaStat(unsigned int id);                       /* 0x0101BF80  syscall 0x76 (<0 = done) */
int   isceSifDmaStat(unsigned int id);                      /* 0x0101BF90  syscall -0x76 */
int   sceSifSetDChain(void);                                /* 0x0101BFC0  syscall 0x78 */
int   isceSifSetDChain(void);                               /* 0x0101BFD0  syscall -0x78 (int: core u140's _sif_cmd_intr_handler) */
int   sceSifSetReg(unsigned int reg, unsigned int value);   /* 0x0101BFE0  syscall 0x79 */
int   sceSifGetReg(unsigned int reg);                       /* 0x0101BFF0  syscall 0x7A */
void  sceSifStopDma(void);                                  /* 0x0101BEB0  syscall 0x6B */

/* Write back the D-cache lines covering [ptr, ptr+size) before a SIF DMA.  */
void  sceSifWriteBackDCache(void *ptr, int size);           /* 0x0101D9A0 */

/* SIF hardware registers (sceSifGetReg/SetReg numbers 1..4) and the kernel's
 * software "system" registers.                                              */
#define SIF_REG_MSCOM          1            /* main->sub command buffer addr */
#define SIF_REG_SMCOM          2            /* sub->main command buffer addr */
#define SIF_REG_MSFLAG         3
#define SIF_REG_SMFLAG         4            /* sceSifIsAliveIop / sceSifSyncIop poll this */
#define SIF_REG_MAINADDR       SIF_REG_MSCOM   /* ps2sdk spellings */
#define SIF_REG_SUBADDR        SIF_REG_SMCOM
#define SIF_REG_ID_SYSTEM      0x80000000
#define SIF_SYSREG_SUBADDR     (SIF_REG_ID_SYSTEM | 0)   /* IOP cmd buffer (sifcmd iopbuf) */
#define SIF_SYSREG_MAINADDR    (SIF_REG_ID_SYSTEM | 1)
#define SIF_SYSREG_RPCINIT     (SIF_REG_ID_SYSTEM | 2)

/* SMFLAG status bits */
#define SIF_STAT_SIFINIT       0x10000      /* tested by sceSifIsAliveIop */
#define SIF_STAT_CMDINIT       0x20000
#define SIF_STAT_BOOTEND       0x40000      /* tested, then written back via sceSifSetReg, by sceSifSyncIop */

/* ======================================================================== */
/* SIF command layer (sifcmd.o in kernl_sys)                                */
/* ======================================================================== */

/* Header at the start of every SIF command packet (0x10..0x70 bytes).      */
typedef struct {
    unsigned int psize:8;     /* +0x00 packet size (bits 0-7)              */
    unsigned int dsize:24;    /* +0x00 extra-data size (bits 8-31)         */
    unsigned int dest;        /* +0x04 extra-data destination (IOP addr)   */
    unsigned int fcode;       /* +0x08 function (command) id               */
    unsigned int opt;         /* +0x0C user option                         */
} sceSifCmdHdr;               /* 0x10 */

typedef void (*sceSifCmdHandler)(void *pkt, void *harg);

/* One handler-table entry (32 system entries @0x0104A900). */
typedef struct {
    sceSifCmdHandler func;    /* +0x00 */
    void            *data;    /* +0x04 harg */
} sceSifCmdData;              /* 0x08 */

/* SIF_CMDC_CHANGE_SADDR packet */
typedef struct {
    sceSifCmdHdr  chdr;       /* +0x00 */
    unsigned int  newaddr;    /* +0x10 */
} sceSifCmdCSData;

/* SIF_CMDC_SET_SREG packet: sreg[rno] = value */
typedef struct {
    sceSifCmdHdr  chdr;       /* +0x00 */
    int           rno;        /* +0x10 */
    unsigned int  value;      /* +0x14 */
} sceSifCmdSRData;

/* SIF_CMDC_RESET_CMD packet (static instance @0x0104D880, sent by
 * sceSifResetIop with psize 0x68, attr 0x44).                               */
typedef struct {
    sceSifCmdHdr  chdr;       /* +0x00 */
    int           size;       /* +0x10 strlen(arg)+1 */
    int           flag;       /* +0x14 mode */
    char          arg[80];    /* +0x18 e.g. "rom0:UDNL cdrom0:\IOPRP224.IMG;1" */
} sceSifCmdResetData;         /* 0x68 */

#ifdef __cplusplus
static_assert(sizeof(sceSifCmdHdr) == 0x10, "sceSifCmdHdr");
static_assert(sizeof(sceSifCmdResetData) == 0x68, "sceSifCmdResetData");
#else
_Static_assert(sizeof(sceSifCmdHdr) == 0x10, "sceSifCmdHdr");
_Static_assert(sizeof(sceSifCmdResetData) == 0x68, "sceSifCmdResetData");
#endif

/* System command ids */
#define SIF_CMDI_SYSTEM         0x80000000
#define SIF_CMDC_CHANGE_SADDR   (SIF_CMDI_SYSTEM | 0x00)
#define SIF_CMDC_SET_SREG       (SIF_CMDI_SYSTEM | 0x01)
#define SIF_CMDC_INIT_CMD       (SIF_CMDI_SYSTEM | 0x02)
#define SIF_CMDC_RESET_CMD      (SIF_CMDI_SYSTEM | 0x03)
#define SIF_CMDC_RPC_END        (SIF_CMDI_SYSTEM | 0x08)
#define SIF_CMDC_RPC_BIND       (SIF_CMDI_SYSTEM | 0x09)
#define SIF_CMDC_RPC_CALL       (SIF_CMDI_SYSTEM | 0x0A)
#define SIF_CMDC_RPC_RDATA      (SIF_CMDI_SYSTEM | 0x0C)
/* Library-specific system commands seen in this binary */
#define SIF_CMDC_FILEIO_END     (SIF_CMDI_SYSTEM | 0x11)   /* fileio completion (_fio_intr) */
#define SIF_CMDC_CDVD_POFF      (SIF_CMDI_SYSTEM | 0x12)   /* libcdvd power-off notify */

/* Software SIF registers (sceSifGetSreg / SET_SREG), 32 words @0x0104AA00 */
#define SIF_SREG_RPCINIT        0

void          sceSifInitCmd(void);                                        /* 0x0101D380 */
void          sceSifExitCmd(void);                                        /* 0x0101D600 */
int           sceSifGetSreg(int rno);                                     /* 0x0101D338 */
int           sceSifSetSreg(int rno, unsigned int value);                 /* 0x0101D350 */
sceSifCmdData *sceSifSetCmdBuffer(sceSifCmdData *db, int size);           /* 0x0101D638 (user table) */
sceSifCmdData *sceSifSetSysCmdBuffer(sceSifCmdData *db, int size);        /* 0x0101D650 (system table) */
void          sceSifAddCmdHandler(unsigned int fcode, sceSifCmdHandler func, void *data); /* 0x0101D668 */
void          sceSifRemoveCmdHandler(unsigned int fcode);                 /* 0x0101D698 */
unsigned int  sceSifSendCmd(unsigned int fcode, void *pkt, int pktsize,
                            void *src, void *dest, int size);            /* 0x0101D7F8 */
unsigned int  isceSifSendCmd(unsigned int fcode, void *pkt, int pktsize,
                             void *src, void *dest, int size);           /* 0x0101D838 */
/* (appended by kernl_sys) returns &_sif_cmd_data (0x0104A8D8); unused.     */
void         *sceSifGetDataTable(void);                                   /* 0x0101D370 ? name */

/* ======================================================================== */
/* SIF RPC (sifrpc.o: kernl_sys up to 0x0101E0C0, kernl_rpc_fio after)       */
/* ======================================================================== */

/* call/bind mode bits */
#define SIF_RPCM_NOWAIT   0x01    /* do not wait for completion (end_func runs later) */
#define SIF_RPCM_NOWBDC   0x02    /* do not write back the D-cache of send buffer     */
#define SIF_RPC_M_NOWAIT  SIF_RPCM_NOWAIT   /* ps2sdk spelling */
#define SIF_RPC_M_NOWBDC  SIF_RPCM_NOWBDC

typedef void  (*sceSifEndFunc)(void *end_para);
typedef void *(*sceSifRpcFunc)(unsigned int fno, void *data, int size);

struct _sif_serve_data;
struct _sif_queue_data;

/* Common head of client / receive data. */
typedef struct _sif_rpc_data {
    void         *paddr;      /* +0x00 RPC packet in flight            */
    unsigned int  pid;        /* +0x04 packet id (copy of pkt+0x18)    */
    int           tid;        /* +0x08 EE: semaphore id, -1 = NOWAIT   */
    unsigned int  mode;       /* +0x0C call mode                       */
} sceSifRpcData;              /* 0x10 */

typedef struct _sif_client_data {
    sceSifRpcData            rpcd;     /* +0x00                                     */
    unsigned int             command;  /* +0x10 server id (cleared on bind)          */
    void                    *buff;     /* +0x14 server receive buffer (IOP)          */
    void                    *cbuff;    /* +0x18                                      */
    sceSifEndFunc            func;     /* +0x1C end function                         */
    void                    *para;     /* +0x20 end parameter                        */
    struct _sif_serve_data  *serve;    /* +0x24 non-NULL once bound (callers spin on it) */
} sceSifClientData;                    /* 0x28 */

typedef struct _sif_receive_data {
    sceSifRpcData  rpcd;      /* +0x00 */
    void          *src;       /* +0x10 */
    void          *dest;      /* +0x14 */
    int            size;      /* +0x18 */
} sceSifReceiveData;          /* 0x1C */

typedef struct _sif_serve_data {
    unsigned int             command;  /* +0x00 sid                              */
    sceSifRpcFunc            func;     /* +0x04                                  */
    void                    *buff;     /* +0x08                                  */
    int                      size;     /* +0x0C request data size                */
    sceSifRpcFunc            cfunc;    /* +0x10                                  */
    void                    *cbuff;    /* +0x14                                  */
    int                      csize;    /* +0x18                                  */
    sceSifClientData        *client;   /* +0x1C                                  */
    void                    *paddr;    /* +0x20 client packet echo               */
    unsigned int             fno;      /* +0x24                                  */
    void                    *receive;  /* +0x28 EE dest for reply                */
    int                      rsize;    /* +0x2C                                  */
    int                      rmode;    /* +0x30 0 = raw sceSifSetDma reply        */
    unsigned int             rid;      /* +0x34                                  */
    struct _sif_serve_data  *link;     /* +0x38 registered-server list            */
    struct _sif_serve_data  *next;     /* +0x3C pending-request list              */
    struct _sif_queue_data  *base;     /* +0x40 owning queue                      */
} sceSifServeData;                     /* 0x44 */

typedef struct _sif_queue_data {
    int                      key;      /* +0x00 thread id (woken on request)      */
    int                      active;   /* +0x04                                   */
    struct _sif_serve_data  *link;     /* +0x08 registered servers                */
    struct _sif_serve_data  *start;    /* +0x0C pending head                      */
    struct _sif_serve_data  *end;      /* +0x10 pending tail                      */
    struct _sif_queue_data  *next;     /* +0x14 next queue                        */
} sceSifQueueData;                     /* 0x18 */

#ifdef __cplusplus
static_assert(sizeof(sceSifClientData) == 0x28, "sceSifClientData");
static_assert(sizeof(sceSifServeData) == 0x44, "sceSifServeData");
#else
_Static_assert(sizeof(sceSifClientData) == 0x28, "sceSifClientData");
_Static_assert(sizeof(sceSifServeData) == 0x44, "sceSifServeData");
#endif

void  sceSifInitRpc(unsigned int mode);                                   /* 0x0101DA50 */
void  sceSifExitRpc(void);                                                /* 0x0101DBF0 */
int   sceSifGetOtherData(sceSifReceiveData *rd, void *src, void *dest,
                         int size, unsigned int mode);                   /* 0x0101DE68 */
int   sceSifBindRpc(sceSifClientData *cd, unsigned int command,
                    unsigned int mode);                                  /* 0x0101E0C0 */
/* (bd, fno, mode, send, ssize, recv, rsize, end_func, end_para): a0-a3, t0-t3, sp[0] */
int   sceSifCallRpc(sceSifClientData *cd, unsigned int fno, unsigned int mode,
                    void *send, int ssize, void *receive, int rsize,
                    sceSifEndFunc end_func, void *end_para);             /* 0x0101E290 */
/* Non-zero while the call on cd is still in flight (packet flag bit 0).  */
int   sceSifCheckStatRpc(sceSifRpcData *cd);                              /* 0x0101E480 */
void  sceSifSetRpcQueue(sceSifQueueData *qd, int thread_id);              /* 0x0101E4C0 */
void  sceSifRegisterRpc(sceSifServeData *sd, unsigned int command,
                        sceSifRpcFunc func, void *buff,
                        sceSifRpcFunc cfunc, void *cbuff,
                        sceSifQueueData *qd);                            /* 0x0101E558 */
sceSifServeData *sceSifRemoveRpc(sceSifServeData *sd, sceSifQueueData *qd); /* 0x0101E628 */
sceSifQueueData *sceSifRemoveRpcQueue(sceSifQueueData *qd);               /* 0x0101E6C0 */
sceSifServeData *sceSifGetNextRequest(sceSifQueueData *qd);               /* 0x0101E750 */
void  sceSifExecRequest(sceSifServeData *sd);                             /* 0x0101E7A8 */
void  sceSifRpcLoop(sceSifQueueData *qd);                                 /* 0x0101E970 */

/* ---- sifrpc internals shared by kernl_sys and kernl_rpc_fio ------------ */

/* All RPC packets are 0x40-byte slots from the uncached pools below.       */
typedef struct {
    sceSifCmdHdr  chdr;       /* +0x00                                          */
    int           rec_id;     /* +0x10 (id<<16)|flags; bit0 = in flight          */
    void         *pkt_addr;   /* +0x14 self                                      */
    int           rpc_id;     /* +0x18 packet id                                 */
    unsigned int  body[9];    /* +0x1C command-specific (see below)              */
} sceSifRpcPkt;               /* 0x40 */

typedef struct {              /* SIF_CMDC_RPC_BIND, EE -> IOP                    */
    sceSifCmdHdr       chdr;
    int                rec_id;    /* +0x10 */
    void              *pkt_addr;  /* +0x14 */
    int                rpc_id;    /* +0x18 */
    sceSifClientData  *client;    /* +0x1C */
    unsigned int       command;   /* +0x20 sid */
} sceSifRpcBindPkt;

/* SIF_CMDC_RPC_CALL, EE -> IOP.  The call-specific body (+0x20..+0x37) sits
 * in a union after the common header.  That is what the retail code of
 * sceSifCallRpc and _request_call shows: egcs gives every access made
 * through a union alias set 0, so stores to the body stay ordered against
 * the other stores and loads of those functions, while the header fields
 * (pkt_addr, rpc_id, client) keep their types' alias sets and are
 * scheduled past them (MATCHING.md, strict aliasing).                       */
typedef struct {
    sceSifCmdHdr       chdr;
    int                rec_id;    /* +0x10 */
    void              *pkt_addr;  /* +0x14 */
    int                rpc_id;    /* +0x18 */
    sceSifClientData  *client;    /* +0x1C */
    union {
        struct {
            unsigned int     fno;       /* +0x20 */
            int              ssize;     /* +0x24 */
            void            *receive;   /* +0x28 */
            int              rsize;     /* +0x2C */
            int              rmode;     /* +0x30 */
            sceSifServeData *serve;     /* +0x34 */
        } call;
    } u;
} sceSifRpcCallPkt;

typedef struct {              /* SIF_CMDC_RPC_END, IOP -> EE                     */
    sceSifCmdHdr       chdr;
    int                rec_id;    /* +0x10 */
    void              *pkt_addr;  /* +0x14 */
    int                rpc_id;    /* +0x18 */
    sceSifClientData  *client;    /* +0x1C */
    unsigned int       cid;       /* +0x20 command that completed (BIND/CALL/RDATA) */
    sceSifServeData   *serve;     /* +0x24 */
    void              *buff;      /* +0x28 */
    void              *cbuff;     /* +0x2C */
} sceSifRpcRendPkt;

typedef struct {              /* SIF_CMDC_RPC_RDATA                              */
    sceSifCmdHdr        chdr;
    int                 rec_id;   /* +0x10 */
    void               *pkt_addr; /* +0x14 */
    int                 rpc_id;   /* +0x18 */
    sceSifReceiveData  *receive;  /* +0x1C */
    void               *src;      /* +0x20 */
    void               *dest;     /* +0x24 */
    int                 size;     /* +0x28 */
} sceSifRpcOtherDataPkt;

/* Library state @0x0104C280 (.bss), initialised by sceSifInitRpc:
 * packet pool 0x0104AA80|UC (32 x 0x40), rdata pool 0x0104B280|UC,
 * client-packet pool 0x0104BA80|UC.  Defined by the kernl_sys unit.         */
struct _sif_rpc_state {
    int               pid;              /* +0x00 packet id counter (init 1)   */
    sceSifRpcPkt     *pkt_table;        /* +0x04                               */
    int               pkt_table_len;    /* +0x08 0x20                          */
    int               unused0C;         /* +0x0C 0                             */
    int               unused10;         /* +0x10 0                             */
    u8               *rdata_table;      /* +0x14                               */
    int               rdata_table_len;  /* +0x18 0x20                          */
    u8               *client_table;     /* +0x1C                               */
    int               client_table_len; /* +0x20 0x20                          */
    int               rdata_table_idx;  /* +0x24 rotation counter              */
    sceSifQueueData  *active_queue;     /* +0x28 server queue list head        */
};                                      /* 0x2C */
/* sif_rpc_state itself (core 0x004AF940, bootstrap 0x0104C280) is a file
 * static of the core's src/sdk/libkernl/sifrpc.c: sifrpc.o's .bss
 * holds it after the three pools, between other objects' statics (u140's
 * data pass); nothing else uses it by name.                                */

sceSifRpcPkt *_rpc_get_packet(struct _sif_rpc_state *rd);                 /* 0x0101DC18 */
void          _rpc_packet_free(sceSifRpcPkt *pkt);                        /* 0x0101DCC0 */
void         *_rpc_get_fpacket(struct _sif_rpc_state *rd);                /* 0x0101DCE0 */
void         *_rpc_get_fpacket2(struct _sif_rpc_state *rd, int rid);      /* 0x0101DD10 */
/* (appended by kernl_rpc_fio) SIF_CMDC_RPC_CALL handler, registered by
 * sceSifInitRpc (instruction 0x0101DB3C): appends the request's serve data to
 * its queue (start +0x0C / end +0x10, link serve+0x3C), copies the call packet
 * fields into it and iWakeupThread(qd->key) if the queue thread is idle.
 * Defined in sdk/libkernl/sifrpc.c.                                        */
void          _request_call(void *pkt, void *harg);                       /* 0x0101E200 */

/* ======================================================================== */
/* loadfile (kernl_rest; IOP LOADFILE server 0x80000006)                    */
/* ======================================================================== */

typedef struct {
    unsigned int epc;     /* +0x00 entry point */
    unsigned int gp;      /* +0x04 */
    unsigned int sp;      /* +0x08 */
    unsigned int dummy;   /* +0x0C */
} sceExecData;

int   sceSifLoadFileReset(void);                                          /* 0x010220B0 */
/* main's F_01000790 builds "cdrom0:\" + UPPER(name) + ";1" in an alloca
 * buffer and retries `while (sceSifLoadModule(path, 0, NULL) < 0)`.        */
int   sceSifLoadModule(const char *filename, int args, const char *argp); /* 0x01022558 */
int   sceSifLoadStartModule(const char *filename, int args,
                            const char *argp, int *result);              /* 0x01022578 */
int   sceSifLoadModuleBuffer(const void *iop_addr, int args, const char *argp); /* 0x010222F0 */
int   sceSifLoadStartModuleBuffer(const void *iop_addr, int args,
                                  const char *argp, int *result);        /* 0x01022310 */
int   sceSifLoadElf(const char *name, sceExecData *data);                 /* 0x010226C0 (unused) */
int   sceSifLoadElfPart(const char *name, const char *secname,
                        sceExecData *data);                              /* 0x010226A0 (unused) */
/* LOADFILE fno 3/2 IOP peek/poke; Sony names unconfirmed (ps2sdk SifIopGetVal/SetVal). */
int   sceSifGetIopAddr(unsigned int iop_addr, void *out, int width);      /* 0x010226E8 ? name */
int   sceSifSetIopAddr(unsigned int iop_addr, const void *value, int width); /* 0x010227D8 ? name */

/* ======================================================================== */
/* IOP reboot (kernl_rest)                                                  */
/* ======================================================================== */

/* Sends SIF_CMDC_RESET_CMD (packet sceSifCmdResetData) carrying arg.       */
int   sceSifResetIop(const char *arg, int mode);                          /* 0x010228B8 ? name */
/* (SMFLAG & SIF_STAT_SIFINIT) != 0 */
int   sceSifIsAliveIop(void);                                             /* 0x010229E8 (symbols.csv: sceSifIopIsAlive) */
/* If SMFLAG has BOOTEND: sceSifSetReg(SMFLAG, BOOTEND), re-init the DECI2
 * TTY (0x01022C40), return 1; else 0.  main: while (!sceSifSyncIop()) ;    */
int   sceSifSyncIop(void);                                                /* 0x01022A10 */
/* Builds "rom0:UDNL " + img on the stack (if too long: printf 'too long
 * parameter' and return 0), sceSifInitRpc(0), sceSifExitRpc(),
 * return sceSifResetIop(buf, 0).
 * main: while (!sceSifRebootIop("cdrom0:\\IOPRP224.IMG;1")) ;              */
int   sceSifRebootIop(const char *img);                                   /* 0x01022A58 */

/* ======================================================================== */
/* IOP heap - Sony API, NOT linked into this binary (no FUNC markers).      */
/* ======================================================================== */

int   sceSifInitIopHeap(void);
void *sceSifAllocIopHeap(unsigned int size);
int   sceSifFreeIopHeap(void *addr);
int   sceSifLoadIopHeap(const char *filename, void *addr);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_SIF_H */
