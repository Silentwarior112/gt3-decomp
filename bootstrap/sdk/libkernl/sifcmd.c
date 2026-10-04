/* sifcmd.c - Sony libkernl (PsIIlibkernl2240) SIF command layer and the first
 * half of the SIF RPC client, as linked into the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102).  Unit kernl_sys.
 *
 * Address range 0x0101D308..0x0101E0BF:
 *   sifcmd.o  0x0101D308  system handlers, sregs, sceSifInitCmd/ExitCmd,
 *                         handler tables, _sif_cmd_send / sceSifSendCmd,
 *                         SIF0 DMA interrupt handler, sceSifWriteBackDCache
 *   sifrpc.o  0x0101DA50  sceSifInitRpc/ExitRpc, packet pools, RPC END /
 *                         RDATA / BIND handlers, sceSifGetOtherData
 * sifrpc.o continues at 0x0101E0C0 (sceSifBindRpc, _request_call, ...) in
 * sifrpc.c (unit kernl_rpc_fio).  The wire formats are the ones ps2sdk's
 * sifcmd/sifrpc use; the code matches Sony's 2.2.4 object code.
 */
#include <stddef.h>     /* offsetof (the compiler's freestanding header) */
#include "types.h"
#include "eekernel.h"
#include "sif.h"

/* ======================================================================== */
/* Hardware registers                                                       */
/* ======================================================================== */

#define D_STAT       0x1000E010   /* DMAC interrupt status (write 1 clears) */
#define D_STAT_SIF0  0x20         /* channel 5 (SIF0, IOP -> EE)             */
#define D5_CHCR      0x1000C000   /* DMAC channel 5 CHCR                     */
#define CHCR_STR     0x100        /* channel running                         */

/* ======================================================================== */
/* sifcmd.o state                                                           */
/* ======================================================================== */

/* _sif_cmd_data @0x0104A8D8 (matches ps2sdk's struct cmd_data). */
struct _sif_cmd_data {
    void          *pktbuf;            /* +0x00 0x0104A800|UC: IOP DMAs its packets here  */
    void          *cmdbuf;            /* +0x04 0x0104A880|UC (never used by this code)   */
    void          *iopbuf;            /* +0x08 IOP-side command buffer (SIF SUBADDR)     */
    sceSifCmdData *sys_cmd_handlers;  /* +0x0C 0x0104A900                                */
    int            nr_sys_handlers;   /* +0x10 0x20                                      */
    sceSifCmdData *usr_cmd_handlers;  /* +0x14 NULL until sceSifSetCmdBuffer             */
    int            nr_usr_handlers;   /* +0x18 0                                         */
    unsigned int  *sregs;             /* +0x1C 0x0104AA00                                */
};                                    /* 0x20 */
_Static_assert(sizeof(struct _sif_cmd_data) == 0x20, "_sif_cmd_data");

/* Handshake packet: header + EE receive-buffer address (0x14 bytes). */
struct _sif_ca_pkt {
    sceSifCmdHdr  chdr;               /* +0x00 */
    void         *buf;                /* +0x10 */
};
_Static_assert(sizeof(struct _sif_ca_pkt) == 0x14, "_sif_ca_pkt");

static u8                   _sif_pktbuf[0x80] __attribute__((aligned(64)));      /* @0x0104A800 (.bss) */
static u8                   _sif_cmdbuf[0x40] __attribute__((aligned(64)));      /* @0x0104A880 (.bss) */
static struct _sif_ca_pkt   _sif_init_pkt __attribute__((aligned(64)));          /* @0x0104A8C0 (.bss) */
static int                  _sif0_handler_id;                                    /* @0x0104A8D4 (.bss) */
static struct _sif_cmd_data _sif_cmd_data;                                       /* @0x0104A8D8 (.bss) */
/* aligned(16): the retail table starts at 0x0104A900, 8 bytes after the end
 * of _sif_cmd_data (0x0104A8F8), and nothing references the gap.  ? 16 is the
 * smallest alignment that reproduces it (64 would too).                     */
static sceSifCmdData        _sif_sys_cmd_handlers[32] __attribute__((aligned(16))); /* @0x0104A900 (.bss) */
static unsigned int         _sif_sregs[32];                                      /* @0x0104AA00 (.bss) */
static int                  _sif_cmd_inited = 0;                                 /* @0x0103DA34 (.data) */

static int _sif_cmd_intr_handler(int channel);

// FUNC 0x0101D308 set_sreg
//   System handler for SIF_CMDC_SET_SREG (0x80000001), installed in slot 1:
//   harg->sregs[pkt->rno] = pkt->value (no bounds check).
static void set_sreg(void *pkt, void *harg)
{
    sceSifCmdSRData *sr = (sceSifCmdSRData *)pkt;
    struct _sif_cmd_data *cd = (struct _sif_cmd_data *)harg;

    cd->sregs[sr->rno] = sr->value;
}

// FUNC 0x0101D328 change_addr
//   System handler for SIF_CMDC_CHANGE_SADDR (0x80000000), installed in slot
//   0: harg->iopbuf = pkt->newaddr (the IOP moved its command buffer).
static void change_addr(void *pkt, void *harg)
{
    ((struct _sif_cmd_data *)harg)->iopbuf = (void *)((sceSifCmdCSData *)pkt)->newaddr;
}

/* The sregs are written by set_sreg in SIF-command interrupt context, so a
 * read must be a real load on every call.  In the binary sceSifGetSreg is
 * out of line and uses a plain load (it sits in the `jr` delay slot, where
 * egcs never puts a volatile access); egcs at -O2 does not inline it, so
 * sceSifInitRpc's wait loop re-reads memory on every pass.  GCC 14 inlines it
 * into that loop and would hoist a plain load (spinning forever), so there
 * the access is volatile.                                                   */
#ifdef BOOT_OLD_GCC
#define SIF_SREG(rno)  (_sif_sregs[rno])
#else
#define SIF_SREG(rno)  (((volatile unsigned int *)_sif_sregs)[rno])
#endif

// FUNC 0x0101D338 sceSifGetSreg
//   Returns software SIF register rno (array @0x0104AA00, no bounds check).
//   sceSifInitRpc polls sreg 0 for the IOP's RPC-ready flag (see SIF_SREG).
int sceSifGetSreg(int rno)
{
    return SIF_SREG(rno);
}

// FUNC 0x0101D350 sceSifSetSreg
//   _sif_sregs[rno] = value; returns value.  No callers.
int sceSifSetSreg(int rno, unsigned int value)
{
    SIF_SREG(rno) = value;
    return value;
}

// FUNC 0x0101D370 sceSifGetDataTable
//   Returns &_sif_cmd_data (0x0104A8D8).  No callers.  /* ? name: Sony-style
//   guess (mapping pass: "cmd-data accessor") */
void *sceSifGetDataTable(void)
{
    return &_sif_cmd_data;
}

// FUNC 0x0101D380 sceSifInitCmd
//   Once-only (flag @0x0103DA34, set inside the DIntr section).  Fills
//   _sif_cmd_data, clears the 32 system handlers and the 32 sregs (egcs runs
//   the sregs loop backwards: a loop with a single store is reversed, and the
//   counter's final value 32 is what the D_STAT write below stores), installs
//   change_addr/set_sreg, EIntr.  Then
//   FlushCache(0); acknowledges a pending SIF0 bit in D_STAT; arms the SIF0
//   receive chain unless D5_CHCR.STR is already set; hooks DMAC ch.5 with
//   _sif_cmd_intr_handler and enables it.  Handshake: if SIF sysreg SUBADDR
//   (0x80000000) is already non-zero the IOP side is up, so just send
//   CHANGE_SADDR with our packet buffer; otherwise wait for SMFLAG.CMDINIT,
//   read SUBADDR (SIF reg 2), publish it and &_sif_cmd_data in sysregs
//   0x80000000/0x80000001 and send INIT_CMD.  The packet carries the CACHED
//   address of the receive buffer (0x0104A800).  Both sends are tail calls.
//   Caller: sceSifInitRpc.
void sceSifInitCmd(void)
{
    int i;
    unsigned int addr;

    DIntr();
    if (_sif_cmd_inited) {
        EIntr();
        return;
    }
    _sif_cmd_inited = 1;
    _sif_cmd_data.pktbuf = UNCACHED(_sif_pktbuf);
    _sif_cmd_data.cmdbuf = UNCACHED(_sif_cmdbuf);
    _sif_cmd_data.iopbuf = NULL;
    _sif_cmd_data.sys_cmd_handlers = _sif_sys_cmd_handlers;
    _sif_cmd_data.nr_sys_handlers = 32;
    _sif_cmd_data.usr_cmd_handlers = NULL;
    _sif_cmd_data.nr_usr_handlers = 0;
    _sif_cmd_data.sregs = _sif_sregs;
    for (i = 0; i < 32; i++) {
        _sif_sys_cmd_handlers[i].func = NULL;
        _sif_sys_cmd_handlers[i].data = NULL;
    }
    for (i = 0; i < 32; i++)
        _sif_sregs[i] = 0;
    _sif_sys_cmd_handlers[0].func = change_addr;
    _sif_sys_cmd_handlers[0].data = &_sif_cmd_data;
    _sif_sys_cmd_handlers[1].func = set_sreg;
    _sif_sys_cmd_handlers[1].data = &_sif_cmd_data;
    EIntr();

    FlushCache(WRITEBACK_DCACHE);
    if (REG32(D_STAT) & D_STAT_SIF0)
        REG32(D_STAT) = D_STAT_SIF0;
    if (!(REG32(D5_CHCR) & CHCR_STR))
        sceSifSetDChain();
    _sif0_handler_id = AddDmacHandler(DMAC_SIF0, _sif_cmd_intr_handler, 0);
    EnableDmac(DMAC_SIF0);

    _sif_cmd_data.iopbuf = (void *)sceSifGetReg(SIF_SYSREG_SUBADDR);
    if (_sif_cmd_data.iopbuf != NULL) {
        _sif_init_pkt.buf = _sif_pktbuf;
        sceSifSendCmd(SIF_CMDC_CHANGE_SADDR, &_sif_init_pkt, sizeof(_sif_init_pkt), NULL, NULL, 0);
        return;
    }

    while (!(sceSifGetReg(SIF_REG_SMFLAG) & SIF_STAT_CMDINIT))
        ;
    addr = sceSifGetReg(SIF_REG_SUBADDR);
    _sif_cmd_data.iopbuf = (void *)addr;
    sceSifSetReg(SIF_SYSREG_SUBADDR, addr);
    sceSifSetReg(SIF_SYSREG_MAINADDR, (unsigned int)&_sif_cmd_data);
    _sif_init_pkt.buf = _sif_pktbuf;
    _sif_init_pkt.chdr.opt = 0;
    sceSifSendCmd(SIF_CMDC_INIT_CMD, &_sif_init_pkt, sizeof(_sif_init_pkt), NULL, NULL, 0);
}

// FUNC 0x0101D600 sceSifExitCmd
//   DisableDmac(5), RemoveDmacHandler(5, saved id), clear the init flag.
//   Callers: sceSifExitRpc, game code 0x010009D8.
void sceSifExitCmd(void)
{
    DisableDmac(DMAC_SIF0);
    RemoveDmacHandler(DMAC_SIF0, _sif0_handler_id);
    _sif_cmd_inited = 0;
}

// FUNC 0x0101D638 sceSifSetCmdBuffer
//   Installs a user handler table (entries, count); returns the old table.  Unused.
sceSifCmdData *sceSifSetCmdBuffer(sceSifCmdData *db, int size)
{
    sceSifCmdData *old = _sif_cmd_data.usr_cmd_handlers;

    _sif_cmd_data.usr_cmd_handlers = db;
    _sif_cmd_data.nr_usr_handlers = size;
    return old;
}

// FUNC 0x0101D650 sceSifSetSysCmdBuffer
//   Replaces the system handler table; returns the old one.  Unused.
sceSifCmdData *sceSifSetSysCmdBuffer(sceSifCmdData *db, int size)
{
    sceSifCmdData *old = _sif_cmd_data.sys_cmd_handlers;

    _sif_cmd_data.sys_cmd_handlers = db;
    _sif_cmd_data.nr_sys_handlers = size;
    return old;
}

/* Handler-table entry update for sceSifAddCmdHandler.  Sony wrote func then
 * data; egcs's scheduler emits the data store first and the func store last
 * (in the `jr` delay slot), so the SIF0 interrupt handler, which tests func
 * and then calls func(pkt, harg), never sees a new func with a stale harg.
 * GCC 14 keeps source order, so there the stores are written data first,
 * with a compiler barrier, to give the same order.  The entry address is
 * taken once before the barrier: the binary loads the table pointer once
 * (re-reading it after the barrier would be a second load).                */
#ifdef BOOT_OLD_GCC
#define SET_CMD_HANDLER(e, f, d) ((e).func = (f), (e).data = (d))  /* (a do/while(0) changes egcs's code) */
#else
#define SET_CMD_HANDLER(e, f, d)                                              \
    do {                                                                      \
        sceSifCmdData *ent_ = &(e);                                           \
        ent_->data = (d);                                                     \
        __asm__ __volatile__("" : : : "memory");                              \
        ent_->func = (f);                                                     \
    } while (0)
#endif

// FUNC 0x0101D668 sceSifAddCmdHandler
//   fcode bit 31 set -> system table, else user table; the entry is at byte
//   offset fcode << 3 (bits 29..31 shift out), no bounds check against the
//   table size.  Stores harg then func (see SET_CMD_HANDLER).
//   Callers: sceSifInitRpc, libcdvd (0x01013560), fileio init (0x0101EF50).
void sceSifAddCmdHandler(unsigned int fcode, sceSifCmdHandler func, void *data)
{
    if (fcode & 0x80000000) {
        SET_CMD_HANDLER(_sif_cmd_data.sys_cmd_handlers[fcode & 0x7FFFFFFF], func, data);
    } else {
        SET_CMD_HANDLER(_sif_cmd_data.usr_cmd_handlers[fcode], func, data);
    }
}

// FUNC 0x0101D698 sceSifRemoveCmdHandler
//   Same table/offset selection; clears only the func word.  Caller: libcdvd 0x01013430.
void sceSifRemoveCmdHandler(unsigned int fcode)
{
    if (fcode & 0x80000000)
        _sif_cmd_data.sys_cmd_handlers[fcode & 0x7FFFFFFF].func = NULL;
    else
        _sif_cmd_data.usr_cmd_handlers[fcode].func = NULL;
}

// FUNC 0x0101D6C0 _sif_cmd_send
//   Core of (i)sceSifSendCmd.  Returns 0 unless 0x10 <= pktsize <= 0x70.
//   With size > 0: header dsize = size, dest = dest, first DMA tag
//   {src, dest, size, 0}, and write back src's D-cache lines only if mode & 4
//   (never set by the two wrappers).  Otherwise dsize = dest = 0.  Then
//   fcode, psize, packet tag {pkt, iopbuf, pktsize, ERT|INT_O = 0x44}, write
//   back the packet, and queue the 1-2 tags with isceSifSetDma (mode & 1,
//   interrupt context) or sceSifSetDma.  Returns the DMA id.
static unsigned int _sif_cmd_send(unsigned int fcode, unsigned int mode, sceSifCmdHdr *pkt,
                                  int pktsize, void *src, void *dest, int size)
{
    sceSifDmaData dma[2];       /* sp+0x00 */
    int n;

    if (!((unsigned int)(pktsize - 0x10) < 0x61))
        return 0;

    n = 0;
    if (size > 0) {
        n = 1;
        dma[0].data = (unsigned int)src;
        dma[0].addr = (unsigned int)dest;
        dma[0].size = size;
        dma[0].mode = 0;
        pkt->dsize = size;
        pkt->dest = (unsigned int)dest;
        if (mode & 4)
            sceSifWriteBackDCache(src, size);
    } else {
        pkt->dsize = 0;
        pkt->dest = 0;
    }

    dma[n].data = (unsigned int)pkt;
    dma[n].addr = (unsigned int)_sif_cmd_data.iopbuf;
    dma[n].size = pktsize;
    pkt->fcode = fcode;
    pkt->psize = pktsize;
    dma[n].mode = SIF_DMA_ERT | SIF_DMA_INT_O;
    sceSifWriteBackDCache(pkt, pktsize);
    n++;

    if (mode & 1)
        return isceSifSetDma(dma, n);
    return sceSifSetDma(dma, n);
}

// FUNC 0x0101D7F8 sceSifSendCmd
//   Thread-context send: _sif_cmd_send(fcode, 0, ...).
//   Callers: sceSifInitCmd, sceSifInitRpc, sceSifGetOtherData, sceSifBindRpc,
//   sceSifCallRpc, sceSifExecRequest.
unsigned int sceSifSendCmd(unsigned int fcode, void *pkt, int pktsize, void *src, void *dest, int size)
{
    return _sif_cmd_send(fcode, 0, (sceSifCmdHdr *)pkt, pktsize, src, dest, size);
}

// FUNC 0x0101D838 isceSifSendCmd
//   Interrupt-context send: _sif_cmd_send(fcode, 1, ...).  Only caller-sites:
//   the tail calls in _request_rdata (0x0101DE60) and _request_bind (0x0101E0B8).
unsigned int isceSifSendCmd(unsigned int fcode, void *pkt, int pktsize, void *src, void *dest, int size)
{
    return _sif_cmd_send(fcode, 1, (sceSifCmdHdr *)pkt, pktsize, src, dest, size);
}

// FUNC 0x0101D878 _sif_cmd_intr_handler
//   DMAC ch.5 (SIF0) handler.  Calls EIntr() first (nested interrupts on).
//   If the packet's psize byte is 0, returns 0 at once (no sync/ei).  Else
//   zeroes the psize byte, copies (psize + 15) / 16 quadwords from the
//   uncached receive buffer to a 0x70-byte stack copy at sp+0 (no clamp: a
//   psize > 0x70 overruns it into the saved $s0/$ra at sp+0x70/0x80 of the
//   0x90 frame), re-arms SIF0 (isceSifSetDChain) and dispatches on fcode:
//   bit 31 set -> system table index fcode & 0x7FFFFFFF, else user table;
//   index must be (signed) < table size and the func non-NULL; calls
//   func(stack copy, harg).  Ends with `sync.l; ei`, returns 0.
//   Because the psize byte is cleared in the receive buffer BEFORE the copy,
//   handlers always see psize == 0 in their packet.
static int _sif_cmd_intr_handler(int channel)
{
    u128 packet[7];             /* sp+0x00..0x6F (0x70 = max valid psize) */
    u128 *src, *dst;
    volatile sceSifCmdHdr *hdr;
    struct _sif_cmd_data *cd = &_sif_cmd_data;
    sceSifCmdHandler func;
    int size, n, i, id;

    EIntr();
    hdr = (sceSifCmdHdr *)cd->pktbuf;
    size = hdr->psize;
    if (size == 0)
        return 0;
    hdr->psize = 0;
    n = (size + 15) / 16;
    src = (u128 *)_sif_cmd_data.pktbuf;
    if (n > 0) {
        dst = packet;
        i = n;
        do {
            *dst++ = *src++;                    /* lq / sq */
        } while (--i);
    }

    isceSifSetDChain();

    hdr = (sceSifCmdHdr *)packet;
    if (hdr->fcode & 0x80000000) {
        id = hdr->fcode & 0x7FFFFFFF;
        if (id < cd->nr_sys_handlers) {
            func = cd->sys_cmd_handlers[id].func;
            if (func != NULL)
                func(packet, cd->sys_cmd_handlers[id].data);
        }
    } else {
        id = hdr->fcode;
        if (id < cd->nr_usr_handlers) {
            func = cd->usr_cmd_handlers[id].func;
            if (func != NULL)
                func(packet, cd->usr_cmd_handlers[id].data);
        }
    }
    ExitHandler();                              /* sync.l; ei */
    return 0;
}

// FUNC 0x0101D9A0 sceSifWriteBackDCache
//   Hand-written assembly in Sony's library ($t9/$t0..$t3, no frame), so it
//   is transcribed as such.  For size > 0, covers the 64-byte lines of
//   [ptr & ~63, (ptr + size - 1) & ~63] with cache 0x18 (DHWBIN, hit
//   write-back + INVALIDATE): first (lines & 7) single lines, then
//   (lines >> 3) blocks of 8 lines unrolled, each op bracketed by sync.l
//   (the nop in the first loop is in the binary; it keeps that loop longer
//   than the R5900 short-loop erratum limit).  No syscall, no interrupt
//   masking.  Ends with a stray unreachable `jr $ra` at 0x0101DA4C (no delay
//   slot of its own).  Only caller-saved registers are used and there is no
//   stack access, so the code is equally valid under the n32 ABI of the
//   GCC 14 build.  22 callers.  C equivalent:
//       if (size <= 0) return;
//       p = (u32)ptr & ~63;  last = ((u32)ptr + size - 1) & ~63;
//       lines = ((last - p) >> 6) + 1;
//       for (rem = lines & 7; rem > 0; rem--, p += 0x40)  line op at p;
//       for (blk = lines >> 3; blk > 0; blk--, p += 0x200) line ops at p + 0..0x1C0;
void sceSifWriteBackDCache(void *ptr, int size);
__asm__(
    "\t.section .text\n"
    "\t.align 3\n"
    "\t.globl sceSifWriteBackDCache\n"
    "\t.type sceSifWriteBackDCache, @function\n"
    "sceSifWriteBackDCache:\n"
    "\t.set push\n"
    "\t.set noreorder\n"
    "\tlui   $25, 0xffff\n"
    "\tori   $25, $25, 0xffc0\n"            /* $t9 = ~63                   */
    "\tblez  $5, 9f\n"                       /* size <= 0: return           */
    "\taddu  $10, $4, $5\n"
    "\tand   $8, $4, $25\n"                  /* $t0 = p = ptr & ~63         */
    "\taddiu $10, $10, -1\n"
    "\tand   $9, $10, $25\n"                 /* last = (ptr+size-1) & ~63   */
    "\tsubu  $10, $9, $8\n"
    "\tsrl   $11, $10, 6\n"
    "\taddiu $11, $11, 1\n"                  /* $t3 = lines                 */
    "\tandi  $9, $11, 7\n"                   /* $t1 = lines & 7             */
    "\tbeqz  $9, 2f\n"
    "\tsrl   $10, $11, 3\n"                  /* $t2 = lines >> 3            */
    "1:\tsync.l\n"
    "\tcache 0x18, 0($8)\n"
    "\tsync.l\n"
    "\taddiu $9, $9, -1\n"
    "\tnop\n"
    "\tbgtz  $9, 1b\n"
    "\taddiu $8, $8, 0x40\n"
    "2:\tbeqz  $10, 9f\n"
    "3:\taddiu $10, $10, -1\n"               /* the block loop re-enters here */
    "\tsync.l\n"
    "\tcache 0x18, 0x000($8)\n"
    "\tsync.l\n"
    "\tcache 0x18, 0x040($8)\n"
    "\tsync.l\n"
    "\tcache 0x18, 0x080($8)\n"
    "\tsync.l\n"
    "\tcache 0x18, 0x0c0($8)\n"
    "\tsync.l\n"
    "\tcache 0x18, 0x100($8)\n"
    "\tsync.l\n"
    "\tcache 0x18, 0x140($8)\n"
    "\tsync.l\n"
    "\tcache 0x18, 0x180($8)\n"
    "\tsync.l\n"
    "\tcache 0x18, 0x1c0($8)\n"
    "\tsync.l\n"
    "\tbgtz  $10, 3b\n"
    "\taddiu $8, $8, 0x200\n"
    "9:\tjr    $31\n"
    "\tnop\n"
    "\tjr    $31\n"                          /* stray, unreachable          */
    "\t.set pop\n"
    "\t.size sceSifWriteBackDCache, .-sceSifWriteBackDCache\n"
    "\t.previous\n");

/* ======================================================================== */
/* sifrpc.o (first half)                                                    */
/* ======================================================================== */

/* Packet pools, 32 slots x 0x40 bytes each, used through their uncached views. */
static u8 _rpc_pkt_pool[32 * 0x40] __attribute__((aligned(64)));      /* @0x0104AA80 (.bss) */
static u8 _rpc_rdata_pool[32 * 0x40] __attribute__((aligned(64)));    /* @0x0104B280 (.bss) */
static u8 _rpc_client_pool[32 * 0x40] __attribute__((aligned(64)));   /* @0x0104BA80 (.bss) */

struct _sif_rpc_state sif_rpc_state;                                   /* @0x0104C280 (.bss) */
static int _sif_rpc_inited = 0;                                        /* @0x0103DA38 (.data) */

static void _request_end(void *data, void *harg);
static void _request_rdata(void *data, void *harg);
static void _request_bind(void *data, void *harg);

/* sifrpc.o's own declarations of what its system handlers read and build.
 * The layouts are sif.h's; the field TYPES are the ones Sony's sifrpc.c
 * evidently used.  egcs compiles with type-based alias analysis, so a load
 * may move above a store only when the two types differ, and the retail
 * schedules of _request_end, _request_rdata and _request_bind pin them down
 * (no other typing reproduces them):
 *  - the END packet carries the requester's packet and client addresses as
 *    integers, like cid; the bound server and its buffers as void *;
 *  - an RDATA request carries the source/destination addresses as integers;
 *  - the END handler stores the bound server through an untyped pointer.
 * Only the C types differ from sif.h: every access has the same address,
 * width and order.                                                         */
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
_Static_assert(offsetof(_sif_rpc_client, serve) == offsetof(sceSifClientData, serve), "client.serve");

/* Offsets relied on by the code below (see sif.h for the full layouts). */
_Static_assert(sizeof(sceSifRpcPkt) == 0x40, "sceSifRpcPkt");
_Static_assert(offsetof(sceSifRpcRendPkt, cid) == 0x20, "RendPkt.cid");
_Static_assert(offsetof(sceSifRpcRendPkt, cbuff) == 0x2C, "RendPkt.cbuff");
_Static_assert(offsetof(sceSifRpcOtherDataPkt, size) == 0x28, "OtherDataPkt.size");
_Static_assert(offsetof(sceSifClientData, serve) == 0x24, "ClientData.serve");
_Static_assert(offsetof(sceSifServeData, link) == 0x38, "ServeData.link");
_Static_assert(offsetof(sceSifQueueData, next) == 0x14, "QueueData.next");
_Static_assert(offsetof(struct _sif_rpc_state, active_queue) == 0x28, "rpc_state.active_queue");

// FUNC 0x0101DA50 sceSifInitRpc
//   Once-only (flag @0x0103DA38, set under DIntr).  sceSifInitCmd(); then
//   under DIntr sets up sif_rpc_state (pid 1, three uncached 32-slot pools,
//   rotation index 0; active_queue is NOT touched) and registers the system
//   handlers END/BIND/CALL/RDATA (0x80000008/9/A/C) with harg = &sif_rpc_state.
//   If sysreg RPCINIT (0x80000002) is still 0: send INIT_CMD (opt = 1) from the
//   2nd packet-pool slot (CACHED address 0x0104AAC0, header only, 0x10 bytes),
//   spin until sreg 0 becomes non-zero, then set sysreg RPCINIT = 1 (tail call).
//   `mode` is ignored.  10 callers (game init 0x01000880, 6 in libcdvd,
//   sceMcInit, fileio init 0x0101EF50, sceSifRebootIop).
void sceSifInitRpc(unsigned int mode)
{
    sceSifCmdHdr *pkt;

    DIntr();
    if (_sif_rpc_inited) {
        EIntr();
        return;
    }
    _sif_rpc_inited = 1;
    EIntr();

    sceSifInitCmd();

    DIntr();
    sif_rpc_state.pkt_table = UNCACHED(_rpc_pkt_pool);
    sif_rpc_state.pkt_table_len = 0x20;
    sif_rpc_state.unused0C = 0;
    sif_rpc_state.unused10 = 0;
    sif_rpc_state.rdata_table = UNCACHED(_rpc_rdata_pool);
    sif_rpc_state.rdata_table_len = 0x20;
    sif_rpc_state.client_table = UNCACHED(_rpc_client_pool);
    sif_rpc_state.client_table_len = 0x20;
    sif_rpc_state.rdata_table_idx = 0;
    sif_rpc_state.pid = 1;
    sceSifAddCmdHandler(SIF_CMDC_RPC_END, _request_end, &sif_rpc_state);
    sceSifAddCmdHandler(SIF_CMDC_RPC_BIND, _request_bind, &sif_rpc_state);
    sceSifAddCmdHandler(SIF_CMDC_RPC_CALL, _request_call, &sif_rpc_state);
    sceSifAddCmdHandler(SIF_CMDC_RPC_RDATA, _request_rdata, &sif_rpc_state);
    EIntr();

    if (sceSifGetReg(SIF_SYSREG_RPCINIT) != 0)
        return;

    pkt = (sceSifCmdHdr *)&_rpc_pkt_pool[0x40];
    pkt->opt = 1;
    sceSifSendCmd(SIF_CMDC_INIT_CMD, pkt, sizeof(sceSifCmdHdr), NULL, NULL, 0);
    while (sceSifGetSreg(SIF_SREG_RPCINIT) == 0)
        ;
    sceSifSetReg(SIF_SYSREG_RPCINIT, 1);
}

// FUNC 0x0101DBF0 sceSifExitRpc
//   sceSifExitCmd(), clear the RPC init flag.  Caller: sceSifRebootIop (0x01022A58).
void sceSifExitRpc(void)
{
    sceSifExitCmd();
    _sif_rpc_inited = 0;
}

// FUNC 0x0101DC18 _rpc_get_packet
//   Under DIntr/EIntr: first pool slot with rec_id bit 0 clear gets
//   rec_id = (slot << 16) | 5, pkt_addr = itself and rpc_id = ++rd->pid, where
//   a result of 1 bumps rd->pid once more (the packet still gets 1).
//   Returns NULL when all slots are busy.
//   Callers: sceSifGetOtherData, sceSifBindRpc, sceSifCallRpc.
sceSifRpcPkt *_rpc_get_packet(struct _sif_rpc_state *rd)
{
    sceSifRpcPkt *pkt;
    int i, len, rid;

    DIntr();
    len = rd->pkt_table_len;
    pkt = rd->pkt_table;
    for (i = 0; i < len; i++, pkt++) {
        if (pkt->rec_id & 1)
            continue;
        pkt->rec_id = (i << 16) | 5;
        if (++rd->pid == 1) {
            rd->pid++;
            rid = 1;
        } else
            rid = rd->pid;
        pkt->pkt_addr = pkt;
        pkt->rpc_id = rid;
        EIntr();
        return pkt;
    }
    EIntr();
    return NULL;
}

// FUNC 0x0101DCC0 _rpc_packet_free
//   rpc_id = 0, clear rec_id bit 0 (slot free).
void _rpc_packet_free(sceSifRpcPkt *pkt)
{
    pkt->rpc_id = 0;
    pkt->rec_id &= 0xFFFFFFFE;
}

// FUNC 0x0101DCE0 _rpc_get_fpacket
//   Rotating reply buffer: slot = rdata_table_idx % rdata_table_len (signed
//   div, `break 7` on /0), idx = slot + 1, returns rdata_table + slot * 0x40.
//   Callers: _request_rdata, _request_bind, _rpc_get_fpacket2, sceSifExecRequest.
void *_rpc_get_fpacket(struct _sif_rpc_state *rd)
{
    int index;
    void *p;

    index = rd->rdata_table_idx % rd->rdata_table_len;
    p = rd->rdata_table + index * 0x40;
    rd->rdata_table_idx = index + 1;
    return p;
}

// FUNC 0x0101DD10 _rpc_get_fpacket2
//   client_table + rid * 0x40 when 0 <= rid < client_table_len (signed),
//   else _rpc_get_fpacket(rd).  Caller: sceSifExecRequest (0x0101E7A8).
void *_rpc_get_fpacket2(struct _sif_rpc_state *rd, int rid)
{
    if (rid < 0 || rid >= rd->client_table_len)
        return _rpc_get_fpacket(rd);
    else
        return rd->client_table + rid * 0x40;
}

// FUNC 0x0101DD50 _request_end
//   System handler SIF_CMDC_RPC_END (0x80000008), IOP -> EE completion.
//   Switch on the completed command: CALL -> run client end_func(end_para)
//   if set; BIND -> client->serve/buff/cbuff from the packet; RDATA and
//   anything else: nothing extra (the explicit RDATA case is what makes egcs
//   emit the binary's 3-way decision tree).  Then iSignalSema(client->rpcd.tid)
//   if >= 0, free the client's packet and clear rpcd.paddr.  pkt->client is
//   re-read after the end function returns.  harg unused.
static void _request_end(void *data, void *harg)
{
    _sif_rpc_end_pkt *pkt = (_sif_rpc_end_pkt *)data;
    _sif_rpc_client *cd;

    switch (pkt->cid) {
    case SIF_CMDC_RPC_CALL:
        cd = (_sif_rpc_client *)pkt->client;
        if (cd->func != NULL)
            cd->func(cd->para);
        break;
    case SIF_CMDC_RPC_BIND:
        cd = (_sif_rpc_client *)pkt->client;
        cd->serve = pkt->serve;
        cd->buff = pkt->buff;
        cd->cbuff = pkt->cbuff;
        break;
    case SIF_CMDC_RPC_RDATA:
        break;
    }
    cd = (_sif_rpc_client *)pkt->client;
    if (cd->rpcd.tid >= 0)
        iSignalSema(cd->rpcd.tid);
    _rpc_packet_free((sceSifRpcPkt *)cd->rpcd.paddr);
    cd->rpcd.paddr = NULL;
}

// FUNC 0x0101DE08 _request_rdata
//   System handler SIF_CMDC_RPC_RDATA (0x8000000C) received as a server: takes
//   a rotating reply packet, copies pkt_addr and the requester's receive-data
//   pointer (+0x1C), cid = RDATA, and isceSifSendCmd(END, reply, 0x40,
//   pkt->src, pkt->dest, pkt->size) - the requested memory travels with the
//   END packet.  Tail call; harg = &sif_rpc_state.
static void _request_rdata(void *data, void *harg)
{
    _sif_rpc_rdata_pkt *pkt = (_sif_rpc_rdata_pkt *)data;
    _sif_rpc_end_pkt *rpkt;

    rpkt = (_sif_rpc_end_pkt *)_rpc_get_fpacket((struct _sif_rpc_state *)harg);
    rpkt->pkt_addr = (unsigned int)pkt->pkt_addr;
    rpkt->client = (unsigned int)pkt->receive;
    rpkt->cid = SIF_CMDC_RPC_RDATA;
    isceSifSendCmd(SIF_CMDC_RPC_END, rpkt, 0x40, (void *)pkt->src, (void *)pkt->dest, pkt->size);
}

// FUNC 0x0101DE68 sceSifGetOtherData
//   Read remote (IOP) memory: allocates a packet (-1 if none), links it to rd
//   (paddr, pid), fills src/dest/size, pkt_addr, receive = rd.  NOWAIT
//   (mode & 1): tid = -1, send RDATA (-2 + free on failure), return 0.
//   Otherwise CreateSema{max 1, init 0} into rd->rpcd.tid (-3 + free on
//   failure), send (-2 + free + DeleteSema on failure), WaitSema, DeleteSema,
//   return 0.  No callers in this binary.
int sceSifGetOtherData(sceSifReceiveData *rd, void *src, void *dest, int size, unsigned int mode)
{
    sceSifRpcOtherDataPkt *pkt;
    struct SemaParam sema;      /* sp+0x00; only +0x04/+0x08 written */

    pkt = (sceSifRpcOtherDataPkt *)_rpc_get_packet(&sif_rpc_state);
    if (pkt == NULL)
        return -1;

    rd->rpcd.paddr = pkt;
    rd->rpcd.pid = pkt->rpc_id;
    pkt->src = src;
    pkt->dest = dest;
    pkt->size = size;
    pkt->pkt_addr = pkt;
    pkt->receive = rd;

    if (!(mode & SIF_RPCM_NOWAIT)) {
        sema.maxCount = 1;
        sema.initCount = 0;
        rd->rpcd.tid = CreateSema(&sema);
        if (rd->rpcd.tid < 0) {
            _rpc_packet_free((sceSifRpcPkt *)pkt);
            return -3;
        }
        if (sceSifSendCmd(SIF_CMDC_RPC_RDATA, pkt, 0x40, NULL, NULL, 0) == 0) {
            _rpc_packet_free((sceSifRpcPkt *)pkt);
            DeleteSema(rd->rpcd.tid);
            return -2;
        }
        WaitSema(rd->rpcd.tid);
        DeleteSema(rd->rpcd.tid);
        return 0;
    } else {
        rd->rpcd.tid = -1;
        if (sceSifSendCmd(SIF_CMDC_RPC_RDATA, pkt, 0x40, NULL, NULL, 0) == 0) {
            _rpc_packet_free((sceSifRpcPkt *)pkt);
            return -2;
        }
        return 0;
    }
}

// FUNC 0x0101DFC0 _search_svdata
//   Walks rd->active_queue (queue->next) and each queue's registered-server
//   list (queue->link, serve->link) for serve->command == sid; returns it or
//   NULL.  Caller: _request_bind.
static sceSifServeData *_search_svdata(unsigned int sid, struct _sif_rpc_state *rd)
{
    sceSifQueueData *q;
    sceSifServeData *s;

    for (q = rd->active_queue; q != NULL; q = q->next) {
        for (s = q->link; s != NULL; s = s->link) {
            if (s->command == sid)
                return s;
        }
    }
    return NULL;
}

// FUNC 0x0101E010 _request_bind
//   System handler SIF_CMDC_RPC_BIND (0x80000009) received as a server: reply
//   packet from the rotating pool with client/pkt_addr copied, cid = BIND,
//   serve = _search_svdata(pkt->command) and its buff/cbuff (all three 0 when
//   not found); isceSifSendCmd(END, reply, 0x40, NULL, NULL, 0) as a tail call.
static void _request_bind(void *data, void *harg)
{
    sceSifRpcBindPkt *pkt = (sceSifRpcBindPkt *)data;
    struct _sif_rpc_state *rd = (struct _sif_rpc_state *)harg;
    _sif_rpc_end_pkt *rpkt;
    sceSifServeData *sd;

    rpkt = (_sif_rpc_end_pkt *)_rpc_get_fpacket(rd);
    rpkt->pkt_addr = (unsigned int)pkt->pkt_addr;
    rpkt->client = (unsigned int)pkt->client;
    rpkt->cid = SIF_CMDC_RPC_BIND;
    sd = _search_svdata(pkt->command, rd);
    if (sd == NULL) {
        rpkt->serve = NULL;
        rpkt->buff = NULL;
        rpkt->cbuff = NULL;
    } else {
        rpkt->serve = sd;
        rpkt->buff = sd->buff;
        rpkt->cbuff = sd->cbuff;
    }
    isceSifSendCmd(SIF_CMDC_RPC_END, rpkt, 0x40, NULL, NULL, 0);
}
