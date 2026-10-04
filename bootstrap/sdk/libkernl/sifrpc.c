/* sifrpc.c - Sony libkernl 2240 (PsIIlibkernl2240) sifrpc.o, second half:
 * 0x0101E0C0..0x0101E9AC of BOOTSTRAP_SCUS-97102.
 *
 * The first half of sifrpc.o (sceSifInitRpc, sceSifExitRpc, the packet
 * allocators _rpc_get_packet/_rpc_get_fpacket*, sceSifGetOtherData and the
 * END/BIND/RDATA command handlers) belongs to the kernl_sys unit; the RPC
 * library state `sif_rpc_state` (@0x0104C280) is defined there (sif.h).
 *
 * Client side: sceSifBindRpc / sceSifCallRpc / sceSifCheckStatRpc are what
 * every IOP client in the bootstrap (libcdvd, libpad, libmc, fileio,
 * loadfile) uses.  Server side (sceSifSetRpcQueue .. sceSifRpcLoop and the
 * _request_call command handler) is linked in but nothing in the bootstrap
 * registers an EE RPC server, so it is dead code here.
 */
#include "types.h"
#include "eekernel.h"
#include "sif.h"

/* Sony's nopdelay(), inlined into sceSifExecRequest's SIF DMA retry loop
 * (0x0101E920..0x0101E944): i = 0x100000; loop { i--; } while (i != -1),
 * i.e. 0x100001 passes.  The loop is empty in the source: the four nops in
 * the binary are the old compiler's padding of a short loop (R5900
 * short-loop erratum; same helper as libcdvd's _sceCdNopDelay).  The GCC 14
 * build writes the nops out, so each pass costs what it did before, and the
 * asm keeps GCC 14 from deleting the loop.                                  */
#ifdef BOOT_OLD_GCC
#define SIF_NOPDELAY_PASS()  __asm__ __volatile__("")
#else
#define SIF_NOPDELAY_PASS()  __asm__ __volatile__("nop\n\tnop\n\tnop\n\tnop")
#endif
static __inline__ void _sif_nopdelay(void)
{
    int i = 0x100000;

    do
        SIF_NOPDELAY_PASS();
    while (i-- != 0);
}

// FUNC 0x0101E0C0 sceSifBindRpc
// Bind client `cd` to IOP RPC server `command`: SIF_CMDC_RPC_BIND with a
// packet from the RPC pool.  Blocking mode waits on a private semaphore that
// the RPC_END handler signals; cd->serve (+0x24) is non-NULL once bound
// (callers such as _fio_init spin on it and re-bind).
// Returns 0, -1 no packet, -2 sceSifSendCmd failed, -3 CreateSema failed.
int sceSifBindRpc(sceSifClientData *cd, unsigned int command, unsigned int mode)
{
    struct SemaParam sp;
    sceSifRpcBindPkt *bind;

    cd->command = 0;
    cd->serve   = NULL;

    bind = (sceSifRpcBindPkt *)_rpc_get_packet(&sif_rpc_state);
    if (bind == NULL)
        return -1;

    cd->rpcd.paddr = bind;
    cd->rpcd.pid   = bind->rpc_id;
    bind->command  = command;
    bind->pkt_addr = bind;
    bind->client   = cd;

    if (!(mode & SIF_RPCM_NOWAIT)) {
        /* only these two fields are written; the rest of sp is stack garbage */
        sp.initCount = 0;
        sp.maxCount  = 1;
        cd->rpcd.tid = CreateSema(&sp);
        if (cd->rpcd.tid < 0) {
            _rpc_packet_free((sceSifRpcPkt *)bind);
            return -3;
        }

        if (sceSifSendCmd(SIF_CMDC_RPC_BIND, bind, 0x40, NULL, NULL, 0) == 0) {
            _rpc_packet_free((sceSifRpcPkt *)bind);   /* order: free, then DeleteSema */
            DeleteSema(cd->rpcd.tid);
            return -2;
        }

        WaitSema(cd->rpcd.tid);
        DeleteSema(cd->rpcd.tid);
    } else {
        cd->rpcd.tid = -1;
        if (sceSifSendCmd(SIF_CMDC_RPC_BIND, bind, 0x40, NULL, NULL, 0) == 0) {
            _rpc_packet_free((sceSifRpcPkt *)bind);
            return -2;
        }
    }
    return 0;
}

// FUNC 0x0101E200 _request_call
// SIF_CMDC_RPC_CALL (0x8000000A) command handler, registered by sceSifInitRpc
// (0x0101DB3C); runs in interrupt context.  Appends the addressed serve data
// to its queue's pending list and copies the call parameters into it, then
// wakes the queue thread if it is not already processing.  The harg argument
// is not used.  Ends in a tail call of iWakeupThread (0x0101C7E0).
// (The function detector merged this into sceSifBindRpc's tail.)
void _request_call(void *pkt, void *harg)
{
    sceSifRpcCallPkt *req = (sceSifRpcCallPkt *)pkt;
    sceSifServeData  *sd  = req->u.call.serve;
    sceSifQueueData  *qd  = sd->base;

    if (qd->start == NULL) {
        qd->start = sd;
        qd->end   = sd;
    } else {
        qd->end->next = sd;
        qd->end       = sd;
    }

    sd->paddr   = req->pkt_addr;
    sd->client  = req->client;
    sd->fno     = req->u.call.fno;
    sd->size    = req->u.call.ssize;
    sd->receive = req->u.call.receive;
    sd->rsize   = req->u.call.rsize;
    sd->rmode   = req->u.call.rmode;
    sd->rid     = req->rec_id;

    if (qd->key >= 0 && qd->active == 0)
        iWakeupThread(qd->key);
}

// FUNC 0x0101E290 sceSifCallRpc
// Issue RPC function `fno` on the bound client `cd`: fills a CALL packet,
// writes back the D-cache of the send/receive buffers unless SIF_RPCM_NOWBDC,
// and sends SIF_CMDC_RPC_CALL with `ssize` bytes of `send` DMAd to the
// server's buffer (cd->buff).  NOWAIT mode returns at once (rmode = 1 only if
// an end function was given, so the IOP sends RPC_END); blocking mode waits
// on a private semaphore.  Returns 0, -1 no packet, -2 send failed,
// -3 CreateSema failed.  64 call sites in the binary.
int sceSifCallRpc(sceSifClientData *cd, unsigned int fno, unsigned int mode,
                  void *send, int ssize, void *receive, int rsize,
                  sceSifEndFunc end_func, void *end_para)
{
    struct SemaParam sp;
    sceSifRpcCallPkt *call;

    call = (sceSifRpcCallPkt *)_rpc_get_packet(&sif_rpc_state);
    if (call == NULL)
        return -1;

    cd->para       = end_para;
    cd->rpcd.paddr = call;
    cd->rpcd.pid   = call->rpc_id;
    cd->func       = end_func;
    call->u.call.fno      = fno;
    call->u.call.ssize    = ssize;
    call->u.call.receive  = receive;
    call->u.call.rsize    = rsize;
    call->pkt_addr = call;
    call->client   = cd;
    call->u.call.serve    = cd->serve;

    if (!(mode & SIF_RPCM_NOWBDC)) {
        if (send == receive) {
            /* shared buffer: one write-back of the larger size (no > 0 test) */
            sceSifWriteBackDCache(send, (ssize < rsize) ? rsize : ssize);
        } else {
            if (ssize > 0)
                sceSifWriteBackDCache(send, ssize);
            if (rsize > 0)
                sceSifWriteBackDCache(receive, rsize);
        }
    }

    if (mode & SIF_RPCM_NOWAIT) {
        if (end_func == NULL)
            call->u.call.rmode = 0;
        else
            call->u.call.rmode = 1;
        cd->rpcd.tid = -1;
        if (sceSifSendCmd(SIF_CMDC_RPC_CALL, call, 0x40, send, cd->buff, ssize) != 0)
            return 0;
        _rpc_packet_free((sceSifRpcPkt *)call);
        return -2;
    }

    sp.initCount = 0;
    sp.maxCount  = 1;
    cd->rpcd.tid = CreateSema(&sp);
    if (cd->rpcd.tid < 0) {
        _rpc_packet_free((sceSifRpcPkt *)call);
        return -3;
    }

    call->u.call.rmode = 1;
    if (sceSifSendCmd(SIF_CMDC_RPC_CALL, call, 0x40, send, cd->buff, ssize) == 0) {
        DeleteSema(cd->rpcd.tid);                 /* order: DeleteSema, then free */
        _rpc_packet_free((sceSifRpcPkt *)call);
        return -2;
    }

    WaitSema(cd->rpcd.tid);
    DeleteSema(cd->rpcd.tid);
    return 0;
}

// FUNC 0x0101E480 sceSifCheckStatRpc
// Leaf.  1 while the client's last request is still in flight: it has a
// packet, the packet still carries the same id, and the packet's
// in-use flag (rec_id bit 0) is set.  Polled by libcdvd / libpad.
int sceSifCheckStatRpc(sceSifRpcData *cd)
{
    sceSifRpcPkt *pkt = (sceSifRpcPkt *)cd->paddr;

    if (pkt == NULL || cd->pid != (unsigned int)pkt->rpc_id || !(pkt->rec_id & 1))
        return 0;
    return 1;
}

// FUNC 0x0101E4C0 sceSifSetRpcQueue
// Server side (no callers in the bootstrap).  Initialises queue `qd` for
// thread `thread_id` and appends it to the global queue list
// (sif_rpc_state.active_queue @0x0104C2A8, linked through qd->next) with
// interrupts disabled.  Ends in a tail call of EIntr.
void sceSifSetRpcQueue(sceSifQueueData *qd, int thread_id)
{
    sceSifQueueData *q;

    DIntr();
    qd->key    = thread_id;
    qd->active = 0;
    qd->link   = NULL;
    qd->start  = NULL;
    qd->end    = NULL;
    qd->next   = NULL;

    if (sif_rpc_state.active_queue == NULL) {
        sif_rpc_state.active_queue = qd;
    } else {
        q = sif_rpc_state.active_queue;
        while (q->next != NULL)
            q = q->next;
        q->next = qd;
    }
    EIntr();
}

// FUNC 0x0101E558 sceSifRegisterRpc
// Server side (no callers).  Fills serve data `sd` for server id `command`
// and appends it to qd's registered-server list (qd->link, linked through
// sd->link) with interrupts disabled.  sd->size/csize/... are left as is.
// Ends in a tail call of EIntr.
void sceSifRegisterRpc(sceSifServeData *sd, unsigned int command,
                       sceSifRpcFunc func, void *buff,
                       sceSifRpcFunc cfunc, void *cbuff,
                       sceSifQueueData *qd)
{
    sceSifServeData *s;

    DIntr();
    sd->next    = NULL;
    sd->link    = NULL;
    sd->command = command;
    sd->func    = func;
    sd->buff    = buff;
    sd->cfunc   = cfunc;
    sd->cbuff   = cbuff;
    sd->base    = qd;

    if (qd->link == NULL) {
        qd->link = sd;
    } else {
        s = qd->link;
        while (s->link != NULL)
            s = s->link;
        s->link = sd;
    }
    EIntr();
}

// FUNC 0x0101E628 sceSifRemoveRpc
// Server side (no callers).  Unlinks `sd` from qd's registered-server list
// under DIntr/EIntr.  Quirk: returns the list cursor, i.e. `sd` itself when
// it was the head, otherwise its predecessor, or NULL if not found.
sceSifServeData *sceSifRemoveRpc(sceSifServeData *sd, sceSifQueueData *qd)
{
    sceSifServeData *s;

    DIntr();
    s = qd->link;
    if (s == sd) {
        qd->link = s->link;
    } else {
        while (s != NULL) {
            if (s->link == sd) {
                s->link = sd->link;
                break;
            }
            s = s->link;
        }
    }
    EIntr();
    return s;
}

// FUNC 0x0101E6C0 sceSifRemoveRpcQueue
// Server side (no callers).  Unlinks `qd` from the global queue list
// (sif_rpc_state.active_queue, next at +0x14) under DIntr/EIntr.  Same
// return quirk as sceSifRemoveRpc: qd if it was the head, else its
// predecessor, else NULL.
sceSifQueueData *sceSifRemoveRpcQueue(sceSifQueueData *qd)
{
    sceSifQueueData *q;

    DIntr();
    q = sif_rpc_state.active_queue;
    if (q == qd) {
        sif_rpc_state.active_queue = q->next;
    } else {
        while (q != NULL) {
            if (q->next == qd) {
                q->next = qd->next;
                break;
            }
            q = q->next;
        }
    }
    EIntr();
    return q;
}

// FUNC 0x0101E750 sceSifGetNextRequest
// Pops the head of qd's pending-request list (linked through serve->next)
// under DIntr/EIntr; qd->active = 1 if one was taken, 0 if the list was
// empty.  Only caller: sceSifRpcLoop.
sceSifServeData *sceSifGetNextRequest(sceSifQueueData *qd)
{
    sceSifServeData *sd;

    DIntr();
    sd = qd->start;
    if (sd == NULL) {
        qd->active = 0;
    } else {
        qd->active = 1;
        qd->start  = sd->next;
    }
    EIntr();
    return sd;
}

// FUNC 0x0101E7A8 sceSifExecRequest
// Runs server function sd->func(fno, buff, size), writes back the D-cache of
// the request buffer and of the returned data, takes a reply packet
// (_rpc_get_fpacket2 with the client's packet index when rid bit 2 is set,
// else _rpc_get_fpacket) and answers the client:
//  - rmode != 0: SIF_CMDC_RPC_END carrying the result (retried until the
//    command is queued);
//  - rmode == 0: raw sceSifSetDma of {result -> sd->receive} (if any) and
//    {reply packet -> client's packet address sd->paddr}, retried with a
//    busy-wait until the DMA is queued.
// Only caller: sceSifRpcLoop (dead code here).
void sceSifExecRequest(sceSifServeData *sd)
{
    sceSifDmaData     dma[2];
    sceSifRpcRendPkt *rend;
    void             *rec;
    int               rsize = 0;
    int               n;
    unsigned int      id;

    rec = sd->func(sd->fno, sd->buff, sd->size);
    if (rec != NULL)
        rsize = sd->rsize;

    if (sd->size > 0)
        sceSifWriteBackDCache(sd->buff, sd->size);
    if (rsize > 0)
        sceSifWriteBackDCache(rec, rsize);

    DIntr();
    if (sd->rid & 4)
        rend = (sceSifRpcRendPkt *)_rpc_get_fpacket2(&sif_rpc_state, sd->rid >> 16);
    else
        rend = (sceSifRpcRendPkt *)_rpc_get_fpacket(&sif_rpc_state);
    EIntr();

    rend->cid    = SIF_CMDC_RPC_CALL;
    rend->client = sd->client;

    if (sd->rmode) {
        while (sceSifSendCmd(SIF_CMDC_RPC_END, rend, 0x40, rec, sd->receive, rsize) == 0)
            ;
        return;
    }

    rend->rpc_id = 0;
    rend->rec_id = 0;

    n = 0;
    if (rsize > 0) {
        n = 1;
        dma[0].data = (unsigned int)rec;
        dma[0].addr = (unsigned int)sd->receive;
        dma[0].size = rsize;
        dma[0].mode = 0;
    }
    dma[n].data = (unsigned int)rend;
    dma[n].addr = (unsigned int)sd->paddr;
    dma[n].size = 0x40;
    dma[n].mode = 0;
    n++;

    do {
        if ((id = sceSifSetDma(dma, n)) == 0)
            _sif_nopdelay();
    } while (id == 0);
}

// FUNC 0x0101E970 sceSifRpcLoop
// Server thread body: drain the queue with sceSifExecRequest, then
// SleepThread until _request_call wakes it.  Never returns.
void sceSifRpcLoop(sceSifQueueData *qd)
{
    sceSifServeData *sd;

    for (;;) {
        while ((sd = sceSifGetNextRequest(qd)) != NULL)
            sceSifExecRequest(sd);
        SleepThread();
    }
}
