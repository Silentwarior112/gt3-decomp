/* sifcmd.c - Sony libkernl 2240 sifcmd.o, core
 * 0x00287BE0..0x00288327: the SIF command layer (system handlers, software
 * sregs, sceSifInitCmd/ExitCmd, handler tables, _sif_cmd_send /
 * sceSifSendCmd, the SIF0 DMA interrupt handler, sceSifWriteBackDCache).
 * Same machine code as the sifcmd.o part of
 * decomp/bootstrap/sdk/libkernl/sifcmd.c.
 *
 * The stray `jr $ra` after sceSifWriteBackDCache (0x00288324) is a function
 * of its own in funcs.json and has a marker.
 */
#include "runtime/newlib/G007_stdio.h"
#include "sdk/libkernl/libkernl_part1_internal.h"

/* sifcmd.o's data: file statics (the .bss block lies between the printf
 * object's and sifrpc.o's statics in the original), in this order.  The
 * receive buffer the IOP DMAs its packets into, the command buffer and the
 * handshake packet (a SIF DMA source, written back by cache lines) are
 * 64-byte aligned; aligned(16) puts the handler table at 0x004ADFC0, 8 bytes
 * after _sif_cmd_data's end (nothing references the gap; 16 is the smallest
 * alignment that gives it, 64 would too).  _sif_cmd_inited is initialised
 * data. */
static u8                   _sif_pktbuf[0x80] __attribute__((aligned(64)));   /* 0x004ADEC0 */
static u8                   _sif_cmdbuf[0x40] __attribute__((aligned(64)));   /* 0x004ADF40 */
static struct _sif_ca_pkt   _sif_init_pkt __attribute__((aligned(64)));       /* 0x004ADF80 */
static int                  _sif0_handler_id;                                 /* 0x004ADF94 */
/* (ps2sdk's sif_cmd_main.o calls the object _sif_cmd_data too) */
static struct _sif_cmd_data _sif_cmd_data;                                    /* 0x004ADF98 */
static sceSifCmdData        _sif_sys_cmd_handlers[SIF_NR_SYS_HANDLERS] __attribute__((aligned(16)));  /* 0x004ADFC0 */
static unsigned int         _sif_sregs[SIF_NR_SREGS];                         /* 0x004AE0C0 */
static int                  _sif_cmd_inited = 0;                              /* 0x003081C4 */

static int _sif_cmd_intr_handler(int channel);

/* The sregs are written by set_sreg in SIF-command interrupt context, so a
 * read must be a real load on every call (sceSifInitRpc's wait loop).  The
 * binary's sceSifGetSreg is out of line with a plain load; GCC 14 inlines
 * it, so the access is volatile there.                                      */
#ifdef BOOT_OLD_GCC
#define SIF_SREG(rno)  (_sif_sregs[rno])
#else
#define SIF_SREG(rno)  (((volatile unsigned int *)_sif_sregs)[rno])
#endif

/* Handler-table entry update for sceSifAddCmdHandler: the binary stores data
 * first and func last (in the `jr` delay slot), so the SIF0 interrupt
 * handler never sees a new func with a stale harg.  Written in that order,
 * with a compiler barrier, for GCC 14.                                      */
#ifdef BOOT_OLD_GCC
#define SET_CMD_HANDLER(e, f, d) ((e).func = (f), (e).data = (d))
#else
#define SET_CMD_HANDLER(e, f, d)                                              \
    do {                                                                      \
        sceSifCmdData *ent_ = &(e);                                           \
        ent_->data = (d);                                                     \
        __asm__ __volatile__("" : : : "memory");                              \
        ent_->func = (f);                                                     \
    } while (0)
#endif

// FUNC 0x00287BE0 set_sreg
/*   System handler for SIF_CMDC_SET_SREG (0x80000001), installed in slot 1:
 *   harg->sregs[pkt->rno] = pkt->value (no bounds check).  A local symbol
 *   in ps2sdk's sif_cmd_main.o too.                                         */
static void set_sreg(void *pkt, void *harg)
{
    sceSifCmdSRData *sreg_pkt = (sceSifCmdSRData *)pkt;
    struct _sif_cmd_data *cmd_data = (struct _sif_cmd_data *)harg;

    cmd_data->sregs[sreg_pkt->rno] = sreg_pkt->value;
}

// FUNC 0x00287C00 change_addr
//   System handler for SIF_CMDC_CHANGE_SADDR (0x80000000), installed in slot
//   0: harg->iopbuf = pkt->newaddr (the IOP moved its command buffer).
static void change_addr(void *pkt, void *harg)
{
    ((struct _sif_cmd_data *)harg)->iopbuf = (void *)((sceSifCmdCSData *)pkt)->newaddr;
}

// FUNC 0x00287C10 sceSifGetSreg
//   Returns software SIF register rno (_sif_sregs @0x004AE0C0, no bounds
//   check).  sceSifInitRpc polls sreg 0 for the IOP's RPC-ready flag.
int sceSifGetSreg(int rno)
{
    return SIF_SREG(rno);
}

// FUNC 0x00287C28 sceSifSetSreg
/*   _sif_sregs[rno] = value; returns value.  No callers.                   */
int sceSifSetSreg(int rno, unsigned int value)
{
    SIF_SREG(rno) = value;
    return value;
}

// FUNC 0x00287C48 sceSifGetDataTable
//   Returns &_sif_cmd_data (0x004ADF98), the command layer's state block
//   (lui/addiu of the address, nothing else).  No callers.  /* ? name: the
//   bootstrap's Sony-style guess, kept; ps2sdk has no such accessor (its
//   sif_cmd_main.o exports the _sif_cmd_data object itself) and the binary
//   holds no name.  The type is sif.h's void *: nothing reads the result. */
void *sceSifGetDataTable(void)
{
    return &_sif_cmd_data;
}

// FUNC 0x00287C58 sceSifInitCmd
//   Once-only (flag @0x003081C4, set inside the DIntr section).  Fills
//   _sif_cmd_data, clears the 32 system handlers and the 32 sregs (the sregs
//   loop runs backwards in the binary), installs change_addr/set_sreg,
//   EIntr.  Then FlushCache(0); acknowledges a pending SIF0 bit in D_STAT
//   (writes 0x20); arms the SIF0 receive chain unless D5_CHCR.STR is already
//   set; hooks DMAC ch.5 with _sif_cmd_intr_handler and enables it.
//   Handshake: if SIF sysreg SUBADDR (0x80000000) is already non-zero the IOP
//   side is up, so just send CHANGE_SADDR with our packet buffer; otherwise
//   wait for SMFLAG.CMDINIT, read SUBADDR (SIF reg 2), publish it and
//   &_sif_cmd_data in sysregs 0x80000000/0x80000001 and send INIT_CMD.  The
//   packet carries the CACHED address of the receive buffer (0x004ADEC0).
//   Both sends are tail calls.  Caller: sceSifInitRpc.
void sceSifInitCmd(void)
{
    int i;
    unsigned int iop_cmdbuf;    /* the IOP's command buffer (SIF reg SUBADDR) */

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
    _sif_cmd_data.nr_sys_handlers = SIF_NR_SYS_HANDLERS;
    _sif_cmd_data.usr_cmd_handlers = NULL;
    _sif_cmd_data.nr_usr_handlers = 0;
    _sif_cmd_data.sregs = _sif_sregs;
    for (i = 0; i < SIF_NR_SYS_HANDLERS; i++) {
        _sif_sys_cmd_handlers[i].func = NULL;
        _sif_sys_cmd_handlers[i].data = NULL;
    }
    for (i = 0; i < SIF_NR_SREGS; i++)
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
    /* jal 0x00285970: the first syscall-0x12 stub (symbols.csv: AddDmacHandler2) */
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
    iop_cmdbuf = sceSifGetReg(SIF_REG_SUBADDR);
    _sif_cmd_data.iopbuf = (void *)iop_cmdbuf;
    sceSifSetReg(SIF_SYSREG_SUBADDR, iop_cmdbuf);
    sceSifSetReg(SIF_SYSREG_MAINADDR, (unsigned int)&_sif_cmd_data);
    _sif_init_pkt.buf = _sif_pktbuf;
    _sif_init_pkt.chdr.opt = 0;
    sceSifSendCmd(SIF_CMDC_INIT_CMD, &_sif_init_pkt, sizeof(_sif_init_pkt), NULL, NULL, 0);
}

// FUNC 0x00287ED8 sceSifExitCmd
//   DisableDmac(5), RemoveDmacHandler(5, saved id), clear the init flag.
//   Callers: sceSifExitRpc and one outside the unit.
void sceSifExitCmd(void)
{
    DisableDmac(DMAC_SIF0);
    RemoveDmacHandler(DMAC_SIF0, _sif0_handler_id);
    _sif_cmd_inited = 0;
}

// FUNC 0x00287F10 sceSifSetCmdBuffer
//   Installs a user handler table (entries, count); returns the old table.
//   No callers.
sceSifCmdData *sceSifSetCmdBuffer(sceSifCmdData *db, int size)
{
    sceSifCmdData *old = _sif_cmd_data.usr_cmd_handlers;

    _sif_cmd_data.usr_cmd_handlers = db;
    _sif_cmd_data.nr_usr_handlers = size;
    return old;
}

// FUNC 0x00287F28 sceSifSetSysCmdBuffer
/*   Replaces the system handler table; returns the old one.  No callers.   */
sceSifCmdData *sceSifSetSysCmdBuffer(sceSifCmdData *db, int size)
{
    sceSifCmdData *old = _sif_cmd_data.sys_cmd_handlers;

    _sif_cmd_data.sys_cmd_handlers = db;
    _sif_cmd_data.nr_sys_handlers = size;
    return old;
}

// FUNC 0x00287F40 sceSifAddCmdHandler
//   fcode bit 31 set -> system table, else user table; the entry is at byte
//   offset fcode << 3 (bits 29..31 shift out), no bounds check against the
//   table size.  Stores harg then func (see SET_CMD_HANDLER).
//   Callers: sceSifInitRpc, _fio_init, one outside the unit (libcdvd).
void sceSifAddCmdHandler(unsigned int fcode, sceSifCmdHandler func, void *data)
{
    if (fcode & SIF_CMDI_SYSTEM) {
        SET_CMD_HANDLER(_sif_cmd_data.sys_cmd_handlers[fcode & ~SIF_CMDI_SYSTEM], func, data);
    } else {
        SET_CMD_HANDLER(_sif_cmd_data.usr_cmd_handlers[fcode], func, data);
    }
}

// FUNC 0x00287F70 sceSifRemoveCmdHandler
//   Same table/offset selection; clears only the func word.  One caller
//   outside the unit (libcdvd _sceCdEeCbExit).
void sceSifRemoveCmdHandler(unsigned int fcode)
{
    if (fcode & SIF_CMDI_SYSTEM)
        _sif_cmd_data.sys_cmd_handlers[fcode & ~SIF_CMDI_SYSTEM].func = NULL;
    else
        _sif_cmd_data.usr_cmd_handlers[fcode].func = NULL;
}

// FUNC 0x00287F98 _sif_cmd_send
//   Core of (i)sceSifSendCmd.  Returns 0 unless SIF_CMD_PKT_MIN (0x10) <=
//   pktsize <= SIF_CMD_PKT_MAX (0x70).  With size > 0: header dsize = size,
//   dest = dest, first DMA tag {src, dest, size, 0}, and write back src's
//   D-cache lines only if mode & SIF_CMD_M_WBDC (4, never set by the two
//   wrappers).  Otherwise dsize = dest = 0.  Then fcode, psize, packet tag
//   {pkt, iopbuf, pktsize, ERT|INT_O = 0x44}, write back the packet, and
//   queue the 1-2 tags with isceSifSetDma (mode & SIF_CMD_M_INTR, interrupt
//   context) or sceSifSetDma.  Returns the DMA id (0 = not queued).
//   (Mode bit names: ps2sdk sifcmd.h.)
#define SIF_CMD_M_INTR  0x01
#define SIF_CMD_M_WBDC  0x04
static unsigned int _sif_cmd_send(unsigned int fcode, unsigned int mode, sceSifCmdHdr *pkt,
                                  int pktsize, void *src, void *dest, int size)
{
    sceSifDmaData dma[2];       /* sp+0x00 */
    int n;                      /* DMA tags used */

    if (!((unsigned int)(pktsize - SIF_CMD_PKT_MIN) < SIF_CMD_PKT_MAX - SIF_CMD_PKT_MIN + 1))
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
        if (mode & SIF_CMD_M_WBDC)
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

    if (mode & SIF_CMD_M_INTR)
        return isceSifSetDma(dma, n);
    return sceSifSetDma(dma, n);
}

// FUNC 0x002880D0 sceSifSendCmd
//   Thread-context send: _sif_cmd_send(fcode, 0, ...).
unsigned int sceSifSendCmd(unsigned int fcode, void *pkt, int pktsize, void *src, void *dest, int size)
{
    return _sif_cmd_send(fcode, 0, (sceSifCmdHdr *)pkt, pktsize, src, dest, size);
}

// FUNC 0x00288110 isceSifSendCmd
//   Interrupt-context send: _sif_cmd_send(fcode, SIF_CMD_M_INTR, ...).
//   Callers: the tail calls in _request_rdata and _request_bind.
unsigned int isceSifSendCmd(unsigned int fcode, void *pkt, int pktsize, void *src, void *dest, int size)
{
    return _sif_cmd_send(fcode, SIF_CMD_M_INTR, (sceSifCmdHdr *)pkt, pktsize, src, dest, size);
}

// FUNC 0x00288150 _sif_cmd_intr_handler
//   DMAC ch.5 (SIF0) handler.  Calls EIntr() first (nested interrupts on).
//   If the packet's psize byte is 0, returns 0 at once (no sync/ei).  Else
//   zeroes the psize byte, copies (psize + 15) / 16 quadwords from the
//   uncached receive buffer to a 0x70-byte stack copy (no clamp: a psize >
//   0x70 overruns the frame), re-arms SIF0 (isceSifSetDChain) and dispatches
//   on fcode: bit 31 set -> system table index fcode & 0x7FFFFFFF, else user
//   table; index must be (signed) < table size and the func non-NULL; calls
//   func(stack copy, harg).  Ends with `sync.l; ei`, returns 0.  Because the
//   psize byte is cleared BEFORE the copy, handlers always see psize == 0.
//   Matching: one variable `n` holds psize, then the quadword count, then the
//   handler index (one pseudo that is live while nr_*_handlers sits in $a0,
//   so global-alloc keeps it out of $a0: retail's psize/count in $a1, hdr in
//   $a3, src in $a2; separate variables gave psize $a0 and hdr $a2, 94.3%).
//   The $v1 load of fcode after isceSifSetDChain needs the call's int
//   prototype (G007_stdio.h / sif.h; void: 99.3%).  15 partitions of
//   psize / count / loop counter / index into variables were scored; only
//   this one is exact.
static int _sif_cmd_intr_handler(int channel)
{
    u128 packet[SIF_CMD_PKT_MAX / 16];  /* sp+0x00..0x6F (0x70 = max valid psize) */
    u128 *src, *dst;
    volatile sceSifCmdHdr *hdr;
    struct _sif_cmd_data *cmd_data = &_sif_cmd_data;
    sceSifCmdHandler func;
    int n, left;

    EIntr();
    hdr = (sceSifCmdHdr *)cmd_data->pktbuf;
    n = hdr->psize;
    if (n == 0)
        return 0;
    hdr->psize = 0;
    n = (n + 15) / 16;                          /* quadwords */
    src = (u128 *)_sif_cmd_data.pktbuf;
    if (n > 0) {
        dst = packet;
        left = n;
        do {
            *dst++ = *src++;                    /* lq / sq */
        } while (--left);
    }

    isceSifSetDChain();

    hdr = (sceSifCmdHdr *)packet;
    if (hdr->fcode & SIF_CMDI_SYSTEM) {
        n = hdr->fcode & ~SIF_CMDI_SYSTEM;      /* handler index */
        if (n < cmd_data->nr_sys_handlers) {
            func = cmd_data->sys_cmd_handlers[n].func;
            if (func != NULL)
                func(packet, cmd_data->sys_cmd_handlers[n].data);
        }
    } else {
        n = hdr->fcode;
        if (n < cmd_data->nr_usr_handlers) {
            func = cmd_data->usr_cmd_handlers[n].func;
            if (func != NULL)
                func(packet, cmd_data->usr_cmd_handlers[n].data);
        }
    }
    ExitHandler();                              /* sync.l; ei */
    return 0;
}

// FUNC 0x00288278 sceSifWriteBackDCache
//   (Unnamed in index.txt; the bootstrap's sceSifWriteBackDCache 0x0101D9A0.)
//   Hand-written assembly in Sony's library ($t9/$t0..$t3, no frame), so it
//   is transcribed as such.  For size > 0, covers the 64-byte lines of
//   [ptr & ~63, (ptr + size - 1) & ~63] with cache 0x18 (DHWBIN, hit
//   write-back + INVALIDATE): first (lines & 7) single lines, then
//   (lines >> 3) blocks of 8 lines unrolled, each op bracketed by sync.l.
//   No syscall, no interrupt masking.  22 callers (libcdvd, libmc, ...).
//   C equivalent:
//       if (size <= 0) return;
//       p = (u32)ptr & ~63;  last = ((u32)ptr + size - 1) & ~63;
//       lines = ((last - p) >> 6) + 1;
//       for (rem = lines & 7; rem > 0; rem--, p += 0x40)  line op at p;
//       for (blk = lines >> 3; blk > 0; blk--, p += 0x200) line ops at p + 0..0x1C0;
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
    "\t.set pop\n"
    "\t.size sceSifWriteBackDCache, .-sceSifWriteBackDCache\n"
    "\t.previous\n");

// FUNC 0x00288324 sceSifWriteBackDCache_tail
//   A stray, unreachable `jr $ra` right after sceSifWriteBackDCache's
//   `jr $ra; nop` (no delay slot of its own; the next word is sceSifInitRpc's
//   first instruction).  Part of Sony's hand-written source; funcs.json
//   counts it as a function.
__asm__(
    "\t.section .text\n"
    "\t.globl sceSifWriteBackDCache_tail\n"
    "\t.type sceSifWriteBackDCache_tail, @function\n"
    "sceSifWriteBackDCache_tail:\n"
    "\t.set push\n"
    "\t.set noreorder\n"
    "\tjr    $31\n"                          /* stray, unreachable          */
    "\t.set pop\n"
    "\t.size sceSifWriteBackDCache_tail, .-sceSifWriteBackDCache_tail\n"
    "\t.previous\n");
