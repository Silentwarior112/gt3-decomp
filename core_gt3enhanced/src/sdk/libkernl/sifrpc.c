/* sifrpc.c - Sony libkernl 2240 sifrpc.o, core
 * 0x00288328..0x00289287: SIF RPC client (sceSifInitRpc, packet pools,
 * RPC END / RDATA / BIND handlers, sceSifGetOtherData, sceSifBindRpc,
 * sceSifCallRpc, sceSifCheckStatRpc) and the EE-side RPC server
 * (sceSifSetRpcQueue .. sceSifRpcLoop, the _request_call handler).  Same
 * machine code as the sifrpc.o parts of decomp/bootstrap/sdk/libkernl/
 * sifcmd.c and sifrpc.c.  The library state sif_rpc_state (its struct is
 * sif.h's) is at 0x004AF940 in the core.
 */
#include "runtime/newlib/G007_stdio.h"
#include "sdk/libkernl/libkernl_part1_internal.h"

/* sifrpc.o's data: file statics (the .bss block lies between sifcmd.o's
 * and fileio.o's statics in the original), in this order: the three packet
 * pools, RPC_POOL_SLOTS x RPC_PKT_SIZE bytes each, used through their
 * uncached views (SIF DMA buffers, 64-byte aligned), then the library state;
 * _sif_rpc_inited is initialised data. */
static u8  _rpc_pkt_pool[RPC_POOL_SLOTS * RPC_PKT_SIZE] __attribute__((aligned(64)));     /* 0x004AE140 */
static u8  _rpc_rdata_pool[RPC_POOL_SLOTS * RPC_PKT_SIZE] __attribute__((aligned(64)));   /* 0x004AE940 */
static u8  _rpc_client_pool[RPC_POOL_SLOTS * RPC_PKT_SIZE] __attribute__((aligned(64)));  /* 0x004AF140 */
static struct _sif_rpc_state sif_rpc_state;     /* 0x004AF940 */
static int _sif_rpc_inited = 0;                 /* 0x003081C8 */

/* The system command handlers sceSifInitRpc installs (file-local, as in the
 * bootstrap's source); _request_call is sif.h's.                          */
static void _request_end(void *data, void *harg);
static void _request_rdata(void *data, void *harg);
static void _request_bind(void *data, void *harg);

/* Sony's nopdelay(), inlined into sceSifExecRequest's SIF DMA retry loop:
 * i = 0x100000; loop { i--; } while (i != -1), i.e. 0x100001 passes.  The
 * nops are the old compiler's short-loop padding; GCC 14 gets them written
 * out (and the asm keeps it from deleting the loop).                        */
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

// FUNC 0x00288328 sceSifInitRpc
//   Once-only (flag @0x003081C8, set under DIntr).  sceSifInitCmd(); then
//   under DIntr sets up sif_rpc_state (pid 1, three uncached 32-slot pools,
//   rotation index 0; active_queue is NOT touched) and registers the system
//   handlers END/BIND/CALL/RDATA (0x80000008/9/A/C) with harg = &sif_rpc_state.
//   If sysreg RPCINIT (0x80000002) is still 0: send INIT_CMD (opt = 1) from the
//   2nd packet-pool slot (CACHED address 0x004AE180, header only, 0x10 bytes),
//   spin until sreg 0 becomes non-zero, then set sysreg RPCINIT = 1 (tail call).
//   `mode` is ignored.  10 callers (libcdvd, sceMcInit, _fio_init, ...).
void sceSifInitRpc(unsigned int mode)
{
    sceSifCmdHdr *init_pkt;

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
    sif_rpc_state.pkt_table_len = RPC_POOL_SLOTS;
    sif_rpc_state.unused0C = 0;
    sif_rpc_state.unused10 = 0;
    sif_rpc_state.rdata_table = UNCACHED(_rpc_rdata_pool);
    sif_rpc_state.rdata_table_len = RPC_POOL_SLOTS;
    sif_rpc_state.client_table = UNCACHED(_rpc_client_pool);
    sif_rpc_state.client_table_len = RPC_POOL_SLOTS;
    sif_rpc_state.rdata_table_idx = 0;
    sif_rpc_state.pid = 1;
    sceSifAddCmdHandler(SIF_CMDC_RPC_END, _request_end, &sif_rpc_state);
    sceSifAddCmdHandler(SIF_CMDC_RPC_BIND, _request_bind, &sif_rpc_state);
    sceSifAddCmdHandler(SIF_CMDC_RPC_CALL, _request_call, &sif_rpc_state);
    sceSifAddCmdHandler(SIF_CMDC_RPC_RDATA, _request_rdata, &sif_rpc_state);
    EIntr();

    if (sceSifGetReg(SIF_SYSREG_RPCINIT) != 0)
        return;

    init_pkt = (sceSifCmdHdr *)&_rpc_pkt_pool[RPC_PKT_SIZE];    /* slot 1, cached */
    init_pkt->opt = 1;
    sceSifSendCmd(SIF_CMDC_INIT_CMD, init_pkt, sizeof(sceSifCmdHdr), NULL, NULL, 0);
    while (sceSifGetSreg(SIF_SREG_RPCINIT) == 0)
        ;
    sceSifSetReg(SIF_SYSREG_RPCINIT, 1);
}

// FUNC 0x002884C8 sceSifExitRpc
//   sceSifExitCmd(), clear the RPC init flag.  One caller outside the unit
//   (the IOP reboot).
void sceSifExitRpc(void)
{
    sceSifExitCmd();
    _sif_rpc_inited = 0;
}

// FUNC 0x002884F0 _rpc_get_packet
//   Under DIntr/EIntr: first pool slot without RPC_REC_BUSY gets
//   rec_id = (slot << 16) | RPC_REC_HAS_SLOT | RPC_REC_BUSY (= | 5),
//   pkt_addr = itself and rpc_id = ++rd->pid, where a result of 1 bumps
//   rd->pid once more (the packet still gets 1).  Returns NULL when all
//   slots are busy.
sceSifRpcPkt *_rpc_get_packet(struct _sif_rpc_state *rd)
{
    sceSifRpcPkt *pkt;
    int slot, len, rpc_id;

    DIntr();
    len = rd->pkt_table_len;
    pkt = rd->pkt_table;
    for (slot = 0; slot < len; slot++, pkt++) {
        if (pkt->rec_id & RPC_REC_BUSY)
            continue;
        pkt->rec_id = (slot << RPC_REC_SLOT_SHIFT) | RPC_REC_HAS_SLOT | RPC_REC_BUSY;
        if (++rd->pid == 1) {
            rd->pid++;
            rpc_id = 1;
        } else
            rpc_id = rd->pid;
        pkt->pkt_addr = pkt;
        pkt->rpc_id = rpc_id;
        EIntr();
        return pkt;
    }
    EIntr();
    return NULL;
}

// FUNC 0x00288598 _rpc_packet_free
//   rpc_id = 0, clear RPC_REC_BUSY (slot free).
void _rpc_packet_free(sceSifRpcPkt *pkt)
{
    pkt->rpc_id = 0;
    pkt->rec_id &= ~(unsigned int)RPC_REC_BUSY;     /* 0xFFFFFFFE (lui/ori) */
}

// FUNC 0x002885B8 _rpc_get_fpacket
//   Rotating reply buffer: slot = rdata_table_idx % rdata_table_len (signed
//   div, `break 7` on /0), idx = slot + 1, returns rdata_table + slot * 0x40.
void *_rpc_get_fpacket(struct _sif_rpc_state *rd)
{
    int slot;
    void *packet;

    slot = rd->rdata_table_idx % rd->rdata_table_len;
    packet = rd->rdata_table + slot * RPC_PKT_SIZE;
    rd->rdata_table_idx = slot + 1;
    return packet;
}

// FUNC 0x002885E8 _rpc_get_fpacket2
//   client_table + rid * 0x40 when 0 <= rid < client_table_len (signed),
//   else _rpc_get_fpacket(rd).  Caller: sceSifExecRequest.
void *_rpc_get_fpacket2(struct _sif_rpc_state *rd, int rid)
{
    if (rid < 0 || rid >= rd->client_table_len)
        return _rpc_get_fpacket(rd);
    else
        return rd->client_table + rid * RPC_PKT_SIZE;
}

// FUNC 0x00288628 _request_end
//   System handler SIF_CMDC_RPC_END (0x80000008), IOP -> EE completion.
//   Switch on the completed command: CALL -> run client end_func(end_para)
//   if set; BIND -> client->serve/buff/cbuff from the packet; RDATA and
//   anything else: nothing extra.  Then iSignalSema(client->rpcd.tid) if
//   >= 0, free the client's packet and clear rpcd.paddr.  pkt->client is
//   re-read after the end function returns.  harg unused.
static void _request_end(void *data, void *harg)
{
    _sif_rpc_end_pkt *pkt = (_sif_rpc_end_pkt *)data;
    _sif_rpc_client *client;

    switch (pkt->cid) {
    case SIF_CMDC_RPC_CALL:
        client = (_sif_rpc_client *)pkt->client;
        if (client->func != NULL)
            client->func(client->para);
        break;
    case SIF_CMDC_RPC_BIND:
        client = (_sif_rpc_client *)pkt->client;
        client->serve = pkt->serve;
        client->buff = pkt->buff;
        client->cbuff = pkt->cbuff;
        break;
    case SIF_CMDC_RPC_RDATA:    /* the data came with the packet: nothing to do */
        break;
    }
    client = (_sif_rpc_client *)pkt->client;
    if (client->rpcd.tid >= 0)
        iSignalSema(client->rpcd.tid);
    _rpc_packet_free((sceSifRpcPkt *)client->rpcd.paddr);
    client->rpcd.paddr = NULL;
}

// FUNC 0x002886E0 _request_rdata
//   System handler SIF_CMDC_RPC_RDATA (0x8000000C) received as a server: takes
//   a rotating reply packet, copies pkt_addr and the requester's receive-data
//   pointer (+0x1C), cid = RDATA, and isceSifSendCmd(END, reply, 0x40,
//   pkt->src, pkt->dest, pkt->size) - the requested memory travels with the
//   END packet.  Tail call; harg = &sif_rpc_state.
static void _request_rdata(void *data, void *harg)
{
    _sif_rpc_rdata_pkt *pkt = (_sif_rpc_rdata_pkt *)data;
    _sif_rpc_end_pkt *reply;

    reply = (_sif_rpc_end_pkt *)_rpc_get_fpacket((struct _sif_rpc_state *)harg);
    reply->pkt_addr = (unsigned int)pkt->pkt_addr;
    reply->client = (unsigned int)pkt->receive;
    reply->cid = SIF_CMDC_RPC_RDATA;
    isceSifSendCmd(SIF_CMDC_RPC_END, reply, RPC_PKT_SIZE, (void *)pkt->src, (void *)pkt->dest, pkt->size);
}

// FUNC 0x00288740 sceSifGetOtherData
//   Read remote (IOP) memory: allocates a packet (-1 if none), links it to rd
//   (paddr, pid), fills src/dest/size, pkt_addr, receive = rd.  NOWAIT
//   (mode & 1): tid = -1, send RDATA (-2 + free on failure), return 0.
//   Otherwise CreateSema{max 1, init 0} into rd->rpcd.tid (-3 + free on
//   failure), send (-2 + free + DeleteSema on failure), WaitSema, DeleteSema,
//   return 0.  No callers.
int sceSifGetOtherData(sceSifReceiveData *rd, void *src, void *dest, int size, unsigned int mode)
{
    sceSifRpcOtherDataPkt *pkt;
    struct SemaParam sema_param;    /* sp+0x00; only +0x04/+0x08 written */

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
        sema_param.maxCount = 1;
        sema_param.initCount = 0;
        rd->rpcd.tid = CreateSema(&sema_param);
        if (rd->rpcd.tid < 0) {
            _rpc_packet_free((sceSifRpcPkt *)pkt);
            return -3;
        }
        if (sceSifSendCmd(SIF_CMDC_RPC_RDATA, pkt, RPC_PKT_SIZE, NULL, NULL, 0) == 0) {
            _rpc_packet_free((sceSifRpcPkt *)pkt);
            DeleteSema(rd->rpcd.tid);
            return -2;
        }
        WaitSema(rd->rpcd.tid);
        DeleteSema(rd->rpcd.tid);
        return 0;
    } else {
        rd->rpcd.tid = -1;
        if (sceSifSendCmd(SIF_CMDC_RPC_RDATA, pkt, RPC_PKT_SIZE, NULL, NULL, 0) == 0) {
            _rpc_packet_free((sceSifRpcPkt *)pkt);
            return -2;
        }
        return 0;
    }
}

// FUNC 0x00288898 _search_svdata
//   Walks rd->active_queue (queue->next) and each queue's registered-server
//   list (queue->link, serve->link) for serve->command == sid; returns it or
//   NULL.  Caller: _request_bind.
static sceSifServeData *_search_svdata(unsigned int sid, struct _sif_rpc_state *rd)
{
    sceSifQueueData *queue;
    sceSifServeData *serve;

    for (queue = rd->active_queue; queue != NULL; queue = queue->next) {
        for (serve = queue->link; serve != NULL; serve = serve->link) {
            if (serve->command == sid)
                return serve;
        }
    }
    return NULL;
}

// FUNC 0x002888E8 _request_bind
//   System handler SIF_CMDC_RPC_BIND (0x80000009) received as a server: reply
//   packet from the rotating pool with client/pkt_addr copied, cid = BIND,
//   serve = _search_svdata(pkt->command) and its buff/cbuff (all three 0 when
//   not found); isceSifSendCmd(END, reply, 0x40, NULL, NULL, 0) as a tail call.
static void _request_bind(void *data, void *harg)
{
    sceSifRpcBindPkt *pkt = (sceSifRpcBindPkt *)data;
    struct _sif_rpc_state *rd = (struct _sif_rpc_state *)harg;
    _sif_rpc_end_pkt *reply;
    sceSifServeData *sd;

    reply = (_sif_rpc_end_pkt *)_rpc_get_fpacket(rd);
    reply->pkt_addr = (unsigned int)pkt->pkt_addr;
    reply->client = (unsigned int)pkt->client;
    reply->cid = SIF_CMDC_RPC_BIND;
    sd = _search_svdata(pkt->command, rd);
    if (sd == NULL) {
        reply->serve = NULL;
        reply->buff = NULL;
        reply->cbuff = NULL;
    } else {
        reply->serve = sd;
        reply->buff = sd->buff;
        reply->cbuff = sd->cbuff;
    }
    isceSifSendCmd(SIF_CMDC_RPC_END, reply, RPC_PKT_SIZE, NULL, NULL, 0);
}

// FUNC 0x00288998 sceSifBindRpc
//   (Unnamed in index.txt; the bootstrap's sceSifBindRpc 0x0101E0C0.)
//   Bind client `cd` to IOP RPC server `command`: SIF_CMDC_RPC_BIND with a
//   packet from the RPC pool.  Blocking mode waits on a private semaphore
//   that the RPC_END handler signals; cd->serve (+0x24) is non-NULL once
//   bound (callers such as _fio_init spin on it and re-bind).  Returns 0,
//   -1 no packet, -2 sceSifSendCmd failed, -3 CreateSema failed.  14 callers.
int sceSifBindRpc(sceSifClientData *cd, unsigned int command, unsigned int mode)
{
    struct SemaParam sema_param;
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
        /* only these two fields are written; the rest is stack garbage */
        sema_param.initCount = 0;
        sema_param.maxCount  = 1;
        cd->rpcd.tid = CreateSema(&sema_param);
        if (cd->rpcd.tid < 0) {
            _rpc_packet_free((sceSifRpcPkt *)bind);
            return -3;
        }

        if (sceSifSendCmd(SIF_CMDC_RPC_BIND, bind, RPC_PKT_SIZE, NULL, NULL, 0) == 0) {
            _rpc_packet_free((sceSifRpcPkt *)bind);   /* order: free, then DeleteSema */
            DeleteSema(cd->rpcd.tid);
            return -2;
        }

        WaitSema(cd->rpcd.tid);
        DeleteSema(cd->rpcd.tid);
    } else {
        cd->rpcd.tid = -1;
        if (sceSifSendCmd(SIF_CMDC_RPC_BIND, bind, RPC_PKT_SIZE, NULL, NULL, 0) == 0) {
            _rpc_packet_free((sceSifRpcPkt *)bind);
            return -2;
        }
    }
    return 0;
}

// FUNC 0x00288AD8 _request_call
/*   SIF_CMDC_RPC_CALL (0x8000000A) command handler, registered by
 *   sceSifInitRpc; runs in interrupt context.  Appends the addressed serve
 *   data to its queue's pending list and copies the call parameters into it,
 *   then wakes the queue thread if it is not already processing.  harg is
 *   not used.  Ends in a tail call of iWakeupThread.                         */
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

// FUNC 0x00288B68 sceSifCallRpc
//   Issue RPC function `fno` on the bound client `cd`: fills a CALL packet,
//   writes back the D-cache of the send/receive buffers unless
//   SIF_RPCM_NOWBDC, and sends SIF_CMDC_RPC_CALL with `ssize` bytes of `send`
//   DMAd to the server's buffer (cd->buff).  NOWAIT mode returns at once
//   (rmode = 1 only if an end function was given, so the IOP sends RPC_END);
//   blocking mode waits on a private semaphore.  Returns 0, -1 no packet,
//   -2 send failed, -3 CreateSema failed.  71 call sites in the binary.
int sceSifCallRpc(sceSifClientData *cd, unsigned int fno, unsigned int mode,
                  void *send, int ssize, void *receive, int rsize,
                  sceSifEndFunc end_func, void *end_para)
{
    struct SemaParam sema_param;
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
        if (sceSifSendCmd(SIF_CMDC_RPC_CALL, call, RPC_PKT_SIZE, send, cd->buff, ssize) != 0)
            return 0;
        _rpc_packet_free((sceSifRpcPkt *)call);
        return -2;
    }

    sema_param.initCount = 0;
    sema_param.maxCount  = 1;
    cd->rpcd.tid = CreateSema(&sema_param);
    if (cd->rpcd.tid < 0) {
        _rpc_packet_free((sceSifRpcPkt *)call);
        return -3;
    }

    call->u.call.rmode = 1;
    if (sceSifSendCmd(SIF_CMDC_RPC_CALL, call, RPC_PKT_SIZE, send, cd->buff, ssize) == 0) {
        DeleteSema(cd->rpcd.tid);                 /* order: DeleteSema, then free */
        _rpc_packet_free((sceSifRpcPkt *)call);
        return -2;
    }

    WaitSema(cd->rpcd.tid);
    DeleteSema(cd->rpcd.tid);
    return 0;
}

// FUNC 0x00288D58 sceSifCheckStatRpc
//   Leaf.  1 while the client's last request is still in flight: it has a
//   packet, the packet still carries the same id, and the packet's in-use
//   flag (rec_id bit 0) is set.  Polled by libcdvd / libpad (9 callers).
int sceSifCheckStatRpc(sceSifRpcData *cd)
{
    sceSifRpcPkt *pkt = (sceSifRpcPkt *)cd->paddr;

    if (pkt == NULL || cd->pid != (unsigned int)pkt->rpc_id || !(pkt->rec_id & RPC_REC_BUSY))
        return 0;
    return 1;
}

// FUNC 0x00288D98 sceSifSetRpcQueue
//   Server side.  Initialises queue `qd` for thread `thread_id` and appends
//   it to the global queue list (sif_rpc_state.active_queue @0x004AF968,
//   linked through qd->next) with interrupts disabled.  Ends in a tail call
//   of EIntr.  2 callers outside the unit.
void sceSifSetRpcQueue(sceSifQueueData *qd, int thread_id)
{
    sceSifQueueData *last;

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
        last = sif_rpc_state.active_queue;
        while (last->next != NULL)
            last = last->next;
        last->next = qd;
    }
    EIntr();
}

// FUNC 0x00288E30 sceSifRegisterRpc
//   Server side.  Fills serve data `sd` for server id `command` and appends
//   it to qd's registered-server list (qd->link, linked through sd->link)
//   with interrupts disabled.  sd->size/csize/... are left as is.  Ends in a
//   tail call of EIntr.  2 callers outside the unit.
void sceSifRegisterRpc(sceSifServeData *sd, unsigned int command,
                       sceSifRpcFunc func, void *buff,
                       sceSifRpcFunc cfunc, void *cbuff,
                       sceSifQueueData *qd)
{
    sceSifServeData *last;

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
        last = qd->link;
        while (last->link != NULL)
            last = last->link;
        last->link = sd;
    }
    EIntr();
}

// FUNC 0x00288F00 sceSifRemoveRpc
//   Server side (no callers).  Unlinks `sd` from qd's registered-server list
//   under DIntr/EIntr.  Quirk: returns the list cursor, i.e. `sd` itself when
//   it was the head, otherwise its predecessor, or NULL if not found.
sceSifServeData *sceSifRemoveRpc(sceSifServeData *sd, sceSifQueueData *qd)
{
    sceSifServeData *cursor;

    DIntr();
    cursor = qd->link;
    if (cursor == sd) {
        qd->link = cursor->link;
    } else {
        while (cursor != NULL) {
            if (cursor->link == sd) {
                cursor->link = sd->link;
                break;
            }
            cursor = cursor->link;
        }
    }
    EIntr();
    return cursor;
}

// FUNC 0x00288F98 sceSifRemoveRpcQueue
//   Server side (no callers).  Unlinks `qd` from the global queue list
//   (sif_rpc_state.active_queue, next at +0x14) under DIntr/EIntr.  Same
//   return quirk as sceSifRemoveRpc: qd if it was the head, else its
//   predecessor, else NULL.
sceSifQueueData *sceSifRemoveRpcQueue(sceSifQueueData *qd)
{
    sceSifQueueData *cursor;

    DIntr();
    cursor = sif_rpc_state.active_queue;
    if (cursor == qd) {
        sif_rpc_state.active_queue = cursor->next;
    } else {
        while (cursor != NULL) {
            if (cursor->next == qd) {
                cursor->next = qd->next;
                break;
            }
            cursor = cursor->next;
        }
    }
    EIntr();
    return cursor;
}

// FUNC 0x00289028 sceSifGetNextRequest
//   Pops the head of qd's pending-request list (linked through serve->next)
//   under DIntr/EIntr; qd->active = 1 if one was taken, 0 if the list was
//   empty.  Only caller: sceSifRpcLoop.
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

// FUNC 0x00289080 sceSifExecRequest
//   Runs server function sd->func(fno, buff, size), writes back the D-cache
//   of the request buffer and of the returned data, takes a reply packet
//   (_rpc_get_fpacket2 with the client's packet index when the request's
//   rec_id has RPC_REC_HAS_SLOT, else _rpc_get_fpacket) and answers the
//   client:
//    - rmode != 0: SIF_CMDC_RPC_END carrying the result (retried until the
//      command is queued);
//    - rmode == 0: raw sceSifSetDma of {result -> sd->receive} (if any) and
//      {reply packet -> client's packet address sd->paddr}, retried with a
//      busy-wait until the DMA is queued.
//   Only caller: sceSifRpcLoop.
void sceSifExecRequest(sceSifServeData *sd)
{
    sceSifDmaData     dma[2];
    sceSifRpcRendPkt *rend;
    void             *result;   /* the server function's reply data */
    int               rsize = 0;
    int               n;        /* DMA tags used */
    unsigned int      dma_id;

    result = sd->func(sd->fno, sd->buff, sd->size);
    if (result != NULL)
        rsize = sd->rsize;

    if (sd->size > 0)
        sceSifWriteBackDCache(sd->buff, sd->size);
    if (rsize > 0)
        sceSifWriteBackDCache(result, rsize);

    DIntr();
    if (sd->rid & RPC_REC_HAS_SLOT)
        rend = (sceSifRpcRendPkt *)_rpc_get_fpacket2(&sif_rpc_state, sd->rid >> RPC_REC_SLOT_SHIFT);
    else
        rend = (sceSifRpcRendPkt *)_rpc_get_fpacket(&sif_rpc_state);
    EIntr();

    rend->cid    = SIF_CMDC_RPC_CALL;
    rend->client = sd->client;

    if (sd->rmode) {
        while (sceSifSendCmd(SIF_CMDC_RPC_END, rend, RPC_PKT_SIZE, result, sd->receive, rsize) == 0)
            ;
        return;
    }

    /* Raw reply: the packet overwrites the client's own request packet, so
     * rec_id = 0 clears its RPC_REC_BUSY (what sceSifCheckStatRpc polls). */
    rend->rpc_id = 0;
    rend->rec_id = 0;

    n = 0;
    if (rsize > 0) {
        n = 1;
        dma[0].data = (unsigned int)result;
        dma[0].addr = (unsigned int)sd->receive;
        dma[0].size = rsize;
        dma[0].mode = 0;
    }
    dma[n].data = (unsigned int)rend;
    dma[n].addr = (unsigned int)sd->paddr;
    dma[n].size = RPC_PKT_SIZE;
    dma[n].mode = 0;
    n++;

    do {
        if ((dma_id = sceSifSetDma(dma, n)) == 0)
            _sif_nopdelay();
    } while (dma_id == 0);
}

// FUNC 0x00289248 sceSifRpcLoop
//   Server thread body: drain the queue with sceSifExecRequest, then
//   SleepThread until _request_call wakes it.  Never returns.
void sceSifRpcLoop(sceSifQueueData *qd)
{
    sceSifServeData *sd;

    for (;;) {
        while ((sd = sceSifGetNextRequest(qd)) != NULL)
            sceSifExecRequest(sd);
        SleepThread();
    }
}
