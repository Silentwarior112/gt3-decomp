/* pdi1394_rpc.c - the rest of the no-FDE stretch G003,
 * .text 0x0026E3E0..0x0026F538: the C core of the EE side of the i.LINK
 * (IEEE 1394) driver, under the C++ class pdi1394 (T541, which follows).
 *
 * One context, s_pdi (Pdi1394Ctx, 0x004AB140), locked by its semaphore
 * ctx.sema:
 *  - pdi1394_init creates the semaphore and a thread running an EE SIF RPC
 *    server 'PFWE' (pdi1394_rpc_server) that the IOP module calls with
 *    fno -1 (a new 0x4C-byte status block) or -4 (an 8-byte message for one
 *    of 8 queues); each call wakes the thread in pdi1394_wait_event, and
 *    any other fno runs the user callback (pdi1394_set_callback);
 *  - pdi1394_bind binds the IOP servers 'PFIM' (ctx.cdCtrl: messages and
 *    control commands) and 'PFIS' (ctx.cdData: pdi1394_exchange, frames of
 *    up to 0x3E0 bytes back);
 *  - the message queues: s_pdiQueues holds a free list and 8 channel lists
 *    (BSD TAILQs) over the 32 nodes of s_pdiNodes, at most 4 messages per
 *    channel (a push to a full channel drops its oldest).
 *
 * Every function reaches the context through a local `ctx = &s_pdi' and the
 * queues through `q = &s_pdiQueues' (retail keeps the base in a register
 * and indexes from it: `q->chan[chan]' is (chan << 3) + q, then +8; a
 * global access folds the member offset into the symbol instead).  In
 * pdi1394_exchange, where all callee-saved registers are taken, reload
 * rematerialises ctx (lui / addiu) at each use, as retail shows.
 */
#include "engine/pdi/G003_pdi1394_rpc.h"
#include "engine/pdi/pdi1394_rpc_internal.h"

/* <sys/queue.h> TAILQ operations on Pdi1394MsgList / Pdi1394MsgNode, in the
 * FreeBSD form (statement macros in do { } while (0), TAILQ_FIRST /
 * TAILQ_NEXT / TAILQ_LAST): the store order of the listing matches them,
 * and the brace-only 4.4BSD-Lite form costs msg_init / pop / clear / take.
 * MQ_FIRST(&q->chan[chan]) (an `->' on an address) gives retail's
 * (chan << 3) + q address with the +8 in the load; q->chan[chan].first is
 * q + (chan << 3) (pop, clear, find, take). */
#define MQ_FIRST(head) ((head)->first)
#define MQ_NEXT(elm) ((elm)->next)
#define MQ_INIT(head) do {                                  \
        (head)->first = NULL;                               \
        (head)->last = &(head)->first;                      \
    } while (0)
#define MQ_INSERT_TAIL(head, elm) do {                      \
        (elm)->next = NULL;                                 \
        (elm)->prev = (head)->last;                         \
        *(head)->last = (elm);                              \
        (head)->last = &(elm)->next;                        \
    } while (0)
#define MQ_REMOVE(head, elm) do {                           \
        if ((elm)->next != NULL)                            \
            (elm)->next->prev = (elm)->prev;                \
        else                                                \
            (head)->last = (elm)->prev;                     \
        *(elm)->prev = (elm)->next;                         \
    } while (0)
/* TAILQ_LAST: the last element's tqe_prev, seen through a head. */
#define MQ_LAST(head) (*(((Pdi1394MsgList *)((head)->last))->last))

/* fno values of the control server 'PFIM' (the top two bits select the
 * command).  What the IOP does with 0x40000000 and 0xC0000000 cannot be seen
 * from the EE side (? the IOP module is not part of the executable). */
#define PDI1394_CTRL_MSG        0x00000000u /* data = one Pdi1394Msg (pdi1394_send_msg) */
#define PDI1394_CTRL_40         0x40000000u /* no data, SIF_RPCM_NOWAIT (F_0026EB90, ? link reset) */
#define PDI1394_CTRL_NODE       0xC0000000u /* | node << 27, no data (F_0026EC20) */
#define PDI1394_CTRL_NODE_SHIFT 27

/* fno of the 'PFIS' exchange calls: bit 31 set, a 3-bit mode at bits 27..29,
 * a 27-bit sequence number below. */
#define XFNO_BASE           0x80000000u
#define XFNO_MODE_SHIFT     27
#define XFNO_MODE_CLEAR     0xC7FFFFFFu     /* & clears the mode field */
#define XFNO_SEQ_CLEAR      0xF8000000u     /* & clears the sequence field */
#define XFNO_SEQ_MASK       0x07FFFFFFu
#define XFNO_MODE_1         0x08000000u     /* mode 1 << 27 */
#define XFNO_MODE_3         0x18000000u     /* mode 3 << 27 */

/* pdi1394_exchange modes (the caller, T473, passes 0, 1 and 3). */
#define XMODE_1             1
#define XMODE_1_NOWAIT      2               /* sent as mode 1 with SIF_RPCM_NOWAIT */
#define XMODE_RESEND        3

/* Reply buffer of every exchange: 8 node frames of 0x7C bytes. */
#define XREPLY_SIZE         0x3E0

/* pdi1394_vblank_handler ends a wait on the 181st vblank (3 s at 60 Hz). */
#define WAIT_TIMEOUT_VBLANKS 180

/* The module's data, all file statics (no other object uses them).
 * .rodata (0x003477D8, the object's only .rodata; the pdi1394 vtable of T541
 * follows): the semaphore parameters, max 1 / init 1, a mutex. */
static const struct SemaParam s_pdiSemaParam = {   /* 0x003477D8 */
    0, 1, 1, 0, 0, 0
};

/* .sbss (-G8): the vblank count of the timeout handler.  A file static: it
 * is the last object .sbss word (after T531's static g_random) before the
 * linker's commons (the __ti nodes from 0x003532E8). */
static int s_pdiVblankCount;                    /* 0x003532E4 */

/* .bss 0x004A9F00..0x004AB4D0, in definition order, between T532's
 * heap_object (0x004A9EC0, 0x18 bytes) and T541's g_004AB4D0: file statics
 * (C commons are allocated after every object's .bss).  The section starts
 * 0x28 bytes after heap_object's end, at the next 64-byte boundary, so it is
 * at least 64-aligned: the SIF buffers (DMA) are aligned(64) (every one of
 * them is at a 64-byte boundary; the bytes cannot tell which of them carried
 * the attribute), the thread stack aligned(16). */
static u8 s_pdiServerStack[0x1000] __attribute__((aligned(16)));    /* 0x004A9F00 stack of the EE server thread */
static u8 s_pdiSendBuf[0x40] __attribute__((aligned(64)));          /* 0x004AAF00 ctx.rpcSendBuf */
static u8 s_pdiRecvBuf[0x100] __attribute__((aligned(64)));         /* 0x004AAF40 ctx.rpcRecvBuf */
static u8 s_pdiLastSend[0x80] __attribute__((aligned(64)));         /* 0x004AB040 ctx.lastSend */
static u8 s_pdiLastRecv[0x80] __attribute__((aligned(64)));         /* 0x004AB0C0 ctx.lastRecv */
static Pdi1394Ctx s_pdi;                                            /* 0x004AB140 */
static Pdi1394MsgQueues s_pdiQueues;                                /* 0x004AB268 */
static Pdi1394MsgNode s_pdiNodes[PDI1394_NNODES];                   /* 0x004AB2D0 node pool (0x200) */

// FUNC 0x0026E3E0 pdi1394_init
// Called once from RaceIOPLoadModules::cmdInitClients (T009) with priority
// 1, before the IOP modules are loaded.  Resets the message queues, sets
// the guard words and the four buffer pointers, creates the semaphore from
// the .data SemaParam s_pdiSemaParam (max 1, init 1: a mutex), then creates
// and starts the EE RPC server thread (entry pdi1394_server_thread, stack
// s_pdiServerStack, 0x1000 bytes, &_gp, the given priority, option 0) with
// the context as its argument.  CreateThread's result is not checked
// (unlike thread_create_retry); $a1..$a4 still hold the buffer addresses at
// the CreateSema call (register reuse, not arguments).
void pdi1394_init(int priority)
{
    Pdi1394Ctx *ctx = &s_pdi;
    struct ThreadParam param;

    pdi1394_msg_init();
    ctx->magic0 = PDI1394_MAGIC_OK;
    ctx->magic1 = PDI1394_MAGIC_BAD;
    ctx->rpcSendBuf = s_pdiSendBuf;
    ctx->rpcRecvBuf = s_pdiRecvBuf;
    ctx->lastSend = s_pdiLastSend;
    ctx->lastRecv = s_pdiLastRecv;
    ctx->sema = CreateSema((struct SemaParam *)&s_pdiSemaParam);
    ctx->waitThread = -1;

    param.entry = pdi1394_server_thread;
    param.stack = s_pdiServerStack;
    param.stackSize = sizeof(s_pdiServerStack);
    param.gpReg = _gp;
    param.initPriority = priority;
    param.option = 0;
    StartThread(CreateThread(&param), ctx);
}

// FUNC 0x0026E4C8 pdi1394_server_thread
// Thread body (argument: the context).  Makes this thread the context's
// SIF RPC queue, registers the 'PFWE' server (pdi1394_rpc_server, receive
// buffer ctx.rpcRecvBuf, no cfunc) and serves forever (tail jump to
// sceSifRpcLoop).
void pdi1394_server_thread(void *context)
{
    Pdi1394Ctx *ctx = (Pdi1394Ctx *)context;

    sceSifSetRpcQueue(&ctx->queue, GetThreadId());
    sceSifRegisterRpc(&ctx->serve, PDI1394_RPC_EE, pdi1394_rpc_server,
                      ctx->rpcRecvBuf, NULL, NULL, &ctx->queue);
    sceSifRpcLoop(&ctx->queue);
}

// FUNC 0x0026E530 pdi1394_bind
// Called from RaceIOPLoadModules::cmdBindClients (T009) after the IOP
// modules are loaded.  Binds 'PFIM' to ctx.cdCtrl, once per vblank until it
// is bound (cd.serve != NULL), then 'PFIS' to ctx.cdData the same way.  Bug
// in the original: the second loop also tests ctx.cdCtrl.serve (+0x8C, not
// cdData's +0xB4), so 'PFIS' is bound exactly once, bound or not.
void pdi1394_bind(void)
{
    Pdi1394Ctx *ctx = &s_pdi;

    do {
        sceSifBindRpc(&ctx->cdCtrl, PDI1394_RPC_CTRL, 0);
        wait_vblank_start();
    } while (ctx->cdCtrl.serve == NULL);
    do {
        sceSifBindRpc(&ctx->cdData, PDI1394_RPC_DATA, 0);
        wait_vblank_start();
    } while (ctx->cdCtrl.serve == NULL);   /* sic: cdCtrl */
}

// FUNC 0x0026E5C0 pdi1394_rpc_server
// The 'PFWE' server function, on the server thread.  Under the lock:
//   fno -1: memcpy the data over ctx.status (size as sent, not checked
//           against the 0x4C bytes of the block);
//   fno -4: push the message to its channel queue, wake the waiter, unlock
//           and return (no callback);
// every fno but -4 then unlocks, wakes the waiter and calls
// ctx.callback(ctx.cbArg, fno) if one is set (outside the lock).  Returns
// NULL (no reply data).  fno -2 and -3 have case labels of their own that do
// nothing (retail's decision tree tests the range -3..-2 apart from the
// default); they behave as any other fno.
void *pdi1394_rpc_server(unsigned int fno, void *data, int size)
{
    Pdi1394Ctx *ctx = &s_pdi;

    WaitSema(ctx->sema);
    switch (fno) {
    case PDI1394_FNO_STATUS:
        memcpy(&ctx->status, data, size);
        break;
    case PDI1394_FNO_2:
    case PDI1394_FNO_3:
        break;
    case PDI1394_FNO_MSG: {
        Pdi1394MsgPacket *packet = (Pdi1394MsgPacket *)data;

        pdi1394_msg_push(packet->chan, &packet->msg);
        pdi1394_wake_waiter();
        SignalSema(ctx->sema);
        return NULL;
    }
    }
    SignalSema(ctx->sema);
    pdi1394_wake_waiter();
    if (ctx->callback != NULL)
        ctx->callback(ctx->cbArg, fno);
    return NULL;
}

// FUNC 0x0026E6B0 pdi1394_msg_find_locked
// pdi1394_msg_find under the lock.  The tail jump to SignalSema discards
// the find's result: callers see only *out (zeroed when nothing matched).
// Callers: pdi1394::checkKeys, pdi1394::receive (T541).
void pdi1394_msg_find_locked(int chan, Pdi1394Msg *out, int type)
{
    Pdi1394Ctx *ctx = &s_pdi;

    WaitSema(ctx->sema);
    pdi1394_msg_find(chan, out, type);
    SignalSema(ctx->sema);
}

// FUNC 0x0026E718 pdi1394_msg_take_locked
// pdi1394_msg_take under the lock (result discarded, as above).  Caller:
// pdi1394::receive (T541).
void pdi1394_msg_take_locked(int chan, Pdi1394Msg *out, int type)
{
    Pdi1394Ctx *ctx = &s_pdi;

    WaitSema(ctx->sema);
    pdi1394_msg_take(chan, out, type);
    SignalSema(ctx->sema);
}

// FUNC 0x0026E780 pdi1394_msg_peek_last_locked
// pdi1394_msg_peek_last under the lock (result discarded; *out is left
// untouched when the channel is empty).  No direct caller.
void pdi1394_msg_peek_last_locked(int chan, Pdi1394Msg *out)
{
    Pdi1394Ctx *ctx = &s_pdi;

    WaitSema(ctx->sema);
    pdi1394_msg_peek_last(chan, out);
    SignalSema(ctx->sema);
}

// FUNC 0x0026E7D8 pdi1394_msg_clear_locked
// pdi1394_msg_clear under the lock.  No direct caller.
void pdi1394_msg_clear_locked(int chan)
{
    Pdi1394Ctx *ctx = &s_pdi;

    WaitSema(ctx->sema);
    pdi1394_msg_clear(chan);
    SignalSema(ctx->sema);
}

// FUNC 0x0026E820 pdi1394_msg_remove_if_locked
// pdi1394_msg_remove_if under the lock.  No direct caller.
void pdi1394_msg_remove_if_locked(int chan,
                                  int (*pred)(void *arg, Pdi1394Msg *msg),
                                  void *arg)
{
    Pdi1394Ctx *ctx = &s_pdi;

    WaitSema(ctx->sema);
    pdi1394_msg_remove_if(chan, pred, arg);
    SignalSema(ctx->sema);
}

// FUNC 0x0026E888 pdi1394_send_msg
// Sends an 8-byte message to 'PFIM' (fno 0, waiting, from ctx.rpcSendBuf,
// no reply) and also pushes it to the local channel 0 under the lock.
// Returns 1 without sending while a 'PFIM' call is still in flight, else
// sceSifCallRpc's result.  The call itself is made outside the lock.
// Caller: pdi1394::sendPacket (T541), which retries every vsync on 1.
int pdi1394_send_msg(const Pdi1394Msg *msg)
{
    Pdi1394Ctx *ctx = &s_pdi;
    sceSifClientData *cd = &ctx->cdCtrl;
    int result;

    if (sceSifCheckStatRpc(&cd->rpcd) != 0)
        return 1;
    *(Pdi1394Msg *)ctx->rpcSendBuf = *msg;
    result = sceSifCallRpc(cd, PDI1394_CTRL_MSG, 0,
                           ctx->rpcSendBuf, sizeof(Pdi1394Msg),
                           NULL, 0, NULL, NULL);
    WaitSema(ctx->sema);
    pdi1394_msg_push(0, msg);
    SignalSema(ctx->sema);
    return result;
}

// FUNC 0x0026E938 pdi1394_exchange
// One frame exchange with 'PFIS' (no lock).  Only caller: the link session
// thread (T473, 0x0022CE70), with size 0x7C and modes 0, 1 and 3.  The
// reply buffer is always 0x3E0 bytes (8 node frames of 0x7C).  Returns 1
// if a 'PFIS' call is still in flight.  Mode 2 is mode 1 sent with
// SIF_RPCM_NOWAIT (the reply word is then read before it can have
// arrived).  fno = 0x80000000 | mode << 27 | (seq & 0x07FFFFFF).
// Mode 3 (a resend) makes four calls: ctx.lastRecv as mode 3 / seq-1, then
// as mode 1 / seq; then, if the reply starts with PDI1394_MAGIC_OK,
// ctx.lastSend as mode 3 / seq and mode 1 / seq+1 (the result is the last
// call's), else the result is 2.  The first two calls' results are dropped
// (the register holding the first is reused for XFNO_MODE_1).  Finally a
// reply starting with PDI1394_MAGIC_BAD gives 3; any other keeps a copy of
// the frame sent (ctx.lastSend) and of the reply (ctx.lastRecv), size bytes
// each (into 0x80-byte buffers).
// Matching (88.9%, 147 of 149 instructions): fno starts as XFNO_BASE (retail
// keeps 0x80000000 in fno's register); the two clear masks are variables
// set after the first call (retail loads both before the mode-3 branch:
// modeClear in $s4, seqClear in a stack slot).  Left: retail spills seqClear
// to 0x18($sp) (sw after the lui, lw at its two uses), so its pseudo had no
// REG_EQUIV; here it has one (set once from a constant) and reload loads the
// constant at each use instead.  Tried: both masks as literals, declared /
// assigned in other places and orders, an inner block, int / unsigned,
// ~XFNO_SEQ_MASK, ~seqMask, set twice, a do { } while (0) block with break,
// volatile (a local slot at 0x10, not a spill slot) - none spills it.
int pdi1394_exchange(void *send, void *recv, int size, int mode, int seq)
{
    Pdi1394Ctx *ctx = &s_pdi;
    unsigned int rpcMode;
    unsigned int fno;
    void *src;
    int result;
    unsigned int modeClear, seqClear;

    if (sceSifCheckStatRpc(&ctx->cdData.rpcd) != 0)
        return 1;
    rpcMode = 0;
    if (mode == XMODE_1_NOWAIT) {
        mode = XMODE_1;
        rpcMode = SIF_RPCM_NOWAIT;
    }
    src = send;
    if (mode == XMODE_RESEND) {
        seq--;
        src = ctx->lastRecv;
    }
    fno = XFNO_BASE;
    fno = (fno | ((unsigned int)mode << XFNO_MODE_SHIFT)) | (seq & XFNO_SEQ_MASK);
    result = sceSifCallRpc(&ctx->cdData, fno, rpcMode, src, size,
                           recv, XREPLY_SIZE, NULL, NULL);
    modeClear = XFNO_MODE_CLEAR;
    seqClear = XFNO_SEQ_CLEAR;
    if (mode == XMODE_RESEND) {
        fno = (fno & modeClear) | XFNO_MODE_1;
        seq++;
        fno = (fno & seqClear) | (seq & XFNO_SEQ_MASK);
        sceSifCallRpc(&ctx->cdData, fno, rpcMode, src, size,
                      recv, XREPLY_SIZE, NULL, NULL);
        if (*(u32 *)recv == PDI1394_MAGIC_OK) {
            src = ctx->lastSend;
            fno = (fno & modeClear) | XFNO_MODE_3;
            sceSifCallRpc(&ctx->cdData, fno, rpcMode, src, size,
                          recv, XREPLY_SIZE, NULL, NULL);
            seq++;
            fno = (fno & modeClear) | XFNO_MODE_1;
            result = sceSifCallRpc(&ctx->cdData,
                                   (fno & seqClear) | (seq & XFNO_SEQ_MASK),
                                   rpcMode, src, size, recv, XREPLY_SIZE,
                                   NULL, NULL);
        } else {
            result = 2;
        }
    }
    if (*(u32 *)recv == PDI1394_MAGIC_BAD) {
        result = 3;
    } else {
        memcpy(ctx->lastSend, send, size);
        memcpy(ctx->lastRecv, recv, size);
    }
    return result;
}

// FUNC 0x0026EB90 F_0026EB90
// Under the lock: unless a 'PFIM' call is in flight (then 1), sends the
// control command fno 0x40000000 to 'PFIM' with SIF_RPCM_NOWAIT, no data,
// and returns sceSifCallRpc's result.  Callers: ArcadeControl::selectCourse
// (the cancel input while linked, ? u005's unkEC), ArcadeControl::
// runConnectPage and ArcadeControl::runWaitOtherPage (when they end without
// a link), RaceLinkBattle::virtual_27 (after 601 frames of waiting for the
// link); no caller uses the result (? the link reset, from the callers).
int F_0026EB90(void)
{
    Pdi1394Ctx *ctx = &s_pdi;
    sceSifClientData *cd = &ctx->cdCtrl;
    int result;

    WaitSema(ctx->sema);
    if (sceSifCheckStatRpc(&cd->rpcd) != 0) {
        SignalSema(ctx->sema);
        return 1;
    }
    result = sceSifCallRpc(cd, PDI1394_CTRL_40, SIF_RPCM_NOWAIT,
                           NULL, 0, NULL, 0, NULL, NULL);
    SignalSema(ctx->sema);
    return result;
}

// FUNC 0x0026EC20 F_0026EC20
// Under the lock: waits (one vblank at a time) until no 'PFIM' call is in
// flight, then sends the control command 0xC0000000 | node << 27 (node not
// masked) and waits for it.  Only caller: pdi1394::F_00270398 (T541), with
// the IOP node of every slot in state 1 or 2, before the session exchanges
// the race selections (T473 0x0022D3C0).  What the IOP does with it is not
// known (?).  (The client pointer cd: retail computes &ctx->cdCtrl before
// the WaitSema call.)
void F_0026EC20(int node)
{
    Pdi1394Ctx *ctx = &s_pdi;
    sceSifClientData *cd = &ctx->cdCtrl;

    WaitSema(ctx->sema);
    while (sceSifCheckStatRpc(&cd->rpcd) != 0)
        wait_vblank_start();
    sceSifCallRpc(cd,
                  ((unsigned int)node << PDI1394_CTRL_NODE_SHIFT) | PDI1394_CTRL_NODE,
                  0, NULL, 0, NULL, 0, NULL, NULL);
    SignalSema(ctx->sema);
}

// FUNC 0x0026ECB8 pdi1394_set_callback
// Sets the callback the EE server runs for the fnos it does not handle
// itself (argument stored first, then the function), under the lock.
// Returns func, the new callback (not the old one).  Caller: T473
// (0x0022CB58, link_create).
Pdi1394Callback pdi1394_set_callback(Pdi1394Callback func, void *arg)
{
    Pdi1394Ctx *ctx = &s_pdi;

    WaitSema(ctx->sema);
    ctx->cbArg = arg;
    ctx->callback = func;
    SignalSema(ctx->sema);
    return func;
}

// FUNC 0x0026ED10 pdi1394_clear_callback
// Clears the callback and its argument under the lock.  No direct caller.
void pdi1394_clear_callback(void)
{
    Pdi1394Ctx *ctx = &s_pdi;

    WaitSema(ctx->sema);
    ctx->callback = NULL;
    ctx->cbArg = NULL;
    SignalSema(ctx->sema);
}

// FUNC 0x0026ED48 pdi1394_wake_waiter
// If a thread sleeps in pdi1394_wait_event, WakeupThread it and forget it.
void pdi1394_wake_waiter(void)
{
    Pdi1394Ctx *ctx = &s_pdi;

    if (ctx->waitThread != -1) {
        WakeupThread(ctx->waitThread);
        ctx->waitThread = -1;
    }
}

// FUNC 0x0026ED90 pdi1394_wait_event
// Sleeps until the EE server is called (pdi1394_wake_waiter: returns 0) or
// pdi1394_vblank_handler counts its timeout (returns 1).  Returns 1 at
// once if another thread is already waiting.  The vblank handler is
// installed for the wait only (AddIntcHandler(INTC_VBLANK_S, ..., 0),
// retried while it returns -1; RemoveIntcHandler retried the same way).
// No lock is taken.  Callers: pdi1394::barrier, pdi1394::waitPackets (T541).
int pdi1394_wait_event(void)
{
    int tid = GetThreadId();
    Pdi1394Ctx *ctx = &s_pdi;
    int handlerId;

    if (ctx->waitThread != -1)
        return 1;
    ctx->waitThread = tid;
    ctx->timedOut = 0;
    while ((handlerId = AddIntcHandler(INTC_VBLANK_S, pdi1394_vblank_handler, 0)) == -1)
        ;
    SleepThread();
    while (RemoveIntcHandler(INTC_VBLANK_S, handlerId) == -1)
        ;
    return ctx->timedOut;
}

// FUNC 0x0026EE38 pdi1394_get_status
// Copies the 0x4C-byte status block out under the lock (the inlined memcpy:
// 32-byte steps, aligned ld/sd or ldl/ldr chosen at run time, then an
// unaligned 8 + 4 bytes; the lwl/lwr of the last word shows the destination
// was a `void *', not the 4-aligned struct type).  Callers:
// gt2k_menu::ConnectStatus::virtual_2, ArcadeControl::runConnectPage, T473
// (0x0022CBD8, 0x0022CCB0), pdi1394::updateTopology (T541).
void pdi1394_get_status(void *out)
{
    Pdi1394Ctx *ctx = &s_pdi;

    WaitSema(ctx->sema);
    memcpy(out, &ctx->status, sizeof(ctx->status));
    SignalSema(ctx->sema);
}

// FUNC 0x0026EF38 pdi1394_vblank_handler
// VBLANK_S interrupt handler of pdi1394_wait_event.  Counts vblanks in
// s_pdiVblankCount; from the 181st (about 3 s at 60 Hz) on, if a thread is
// waiting: sets ctx.timedOut, iWakeupThread's it, forgets it and resets the
// count.  The count is only reset there: vblanks seen during earlier waits
// that ended by a wakeup still count towards the next timeout.  Ends with
// ExitHandler() (sync.l; ei) and returns 0 (continue the handler chain).
int pdi1394_vblank_handler(int cause)
{
    Pdi1394Ctx *ctx = &s_pdi;

    if (++s_pdiVblankCount > WAIT_TIMEOUT_VBLANKS) {
        if (ctx->waitThread != -1) {
            ctx->timedOut = 1;
            iWakeupThread(ctx->waitThread);
            ctx->waitThread = -1;
            s_pdiVblankCount = 0;
        }
    }
    ExitHandler();
    return 0;
}

// FUNC 0x0026EFA8 pdi1394_msg_init
// Clears the node pool and puts all 32 nodes on the free list, then empties
// the 8 channel lists and zeroes their counts.  Callers: pdi1394_init,
// T473 (0x0022D9A8), pdi1394::init, pdi1394::resetQueue,
// pdi1394::cancelBarrier (T541).
void pdi1394_msg_init(void)
{
    Pdi1394MsgQueues *q = &s_pdiQueues;
    struct Pdi1394MsgNode *node = s_pdiNodes;
    int i;

    memset(s_pdiNodes, 0, sizeof(s_pdiNodes));
    MQ_INIT(&q->freeList);
    for (i = 0; i < PDI1394_NNODES; i++)
        MQ_INSERT_TAIL(&q->freeList, &node[i]);
    for (i = 0; i < PDI1394_NCHAN; i++) {
        q->count[i] = 0;
        MQ_INIT(&q->chan[i]);
    }
}

// FUNC 0x0026F060 pdi1394_msg_push
// Appends a copy of *msg to channel chan (no lock: callers hold it).  A
// channel already counting 4 first drops its oldest message
// (pdi1394_msg_pop(chan, NULL)).  The node is the free list's first; there
// is no check that one is left.
// Matching (70.8%, 53 of 52 instructions): retail computes chan << 3 at the
// entry and keeps it in $s3 over the pop call (chan itself dies there),
// building &q->chan[chan] from it at the end; here chan is kept and shifted
// at the end, which also moves every register after it.  That needs a use
// of chan << 3 before the call that GCSE could share and that is dead
// afterwards; tried: a head pointer at the entry (local, `->' and `+'
// forms; used or unused: unused is deleted before GCSE), a count pointer,
// the head set after the count update, brace-only macros.
void pdi1394_msg_push(int chan, const Pdi1394Msg *msg)
{
    Pdi1394MsgQueues *q = &s_pdiQueues;
    struct Pdi1394MsgNode *node;

    if (q->count[chan] >= PDI1394_CHAN_MAX)
        pdi1394_msg_pop(chan, NULL);
    q->count[chan]++;
    node = MQ_FIRST(&q->freeList);
    node->msg = *msg;
    MQ_REMOVE(&q->freeList, node);
    MQ_INSERT_TAIL(&q->chan[chan], node);
}

// FUNC 0x0026F130 pdi1394_msg_find
// Zeroes *out (if given), then looks for the first message of channel chan
// whose type byte equals type and copies it to *out (returns 0); the
// message stays queued.  Quirk: with out == NULL a match is skipped like a
// mismatch, so the result is always 1 (not found).
int pdi1394_msg_find(int chan, Pdi1394Msg *out, int type)
{
    Pdi1394MsgQueues *q = &s_pdiQueues;
    struct Pdi1394MsgNode *node;

    if (out != NULL)
        memset(out, 0, sizeof(*out));
    for (node = MQ_FIRST(&q->chan[chan]); node != NULL; node = MQ_NEXT(node)) {
        if (node->msg.type == type && out != NULL) {
            *out = node->msg;
            return 0;
        }
    }
    return 1;
}

// FUNC 0x0026F1D8 pdi1394_msg_peek_last
// The newest message of channel chan (TAILQ_LAST): 1 if the channel is
// empty, else copies it to *out (if given) and returns 0.
int pdi1394_msg_peek_last(int chan, Pdi1394Msg *out)
{
    Pdi1394MsgQueues *q = &s_pdiQueues;
    struct Pdi1394MsgNode *node = MQ_LAST(&q->chan[chan]);

    if (node == NULL)
        return 1;
    if (out != NULL)
        *out = node->msg;
    return 0;
}

// FUNC 0x0026F228 pdi1394_msg_take
// Like pdi1394_msg_find, but the matching message is also removed: count
// decremented, node back on the free list.  The original copies the
// message to *out twice in a row (two identical ldl/ldr/sdl/sdr pairs).
// Same out == NULL quirk: nothing is ever taken, result 1.
// Matching (80.1%, 68 of 69 instructions): retail places the remove's else
// arm (head->last = prev, with &q->chan[chan] rebuilt from chan << 3) and
// the rest of the found path after the loop, behind the `return 1', and
// keeps chan << 3 in $a0 over the loop; here the found path stays inside
// the loop and loop.c hoists &q->chan[chan] + 8.  Tried: the remove's arms
// swapped, written out without the macro, duplicated tails per arm, a
// `goto found' / break after the loop, a while loop, the type and out tests
// split; none moves the else arm out of the loop.
int pdi1394_msg_take(int chan, Pdi1394Msg *out, int type)
{
    Pdi1394MsgQueues *q = &s_pdiQueues;
    struct Pdi1394MsgNode *node;

    if (out != NULL)
        memset(out, 0, sizeof(*out));
    for (node = MQ_FIRST(&q->chan[chan]); node != NULL; node = MQ_NEXT(node)) {
        if (node->msg.type == type && out != NULL) {
            *out = node->msg;
            *out = node->msg;       /* sic: copied twice */
            q->count[chan]--;
            MQ_REMOVE(&q->chan[chan], node);
            MQ_INSERT_TAIL(&q->freeList, node);
            return 0;
        }
    }
    return 1;
}

// FUNC 0x0026F340 pdi1394_msg_pop
// Removes the oldest message of channel chan: 1 if the channel is empty,
// else copies it to *out (if given), decrements the count, puts the node
// back on the free list and returns 0.
int pdi1394_msg_pop(int chan, Pdi1394Msg *out)
{
    Pdi1394MsgQueues *q = &s_pdiQueues;
    struct Pdi1394MsgNode *node = MQ_FIRST(&q->chan[chan]);

    if (node == NULL)
        return 1;
    if (out != NULL)
        *out = node->msg;
    q->count[chan]--;
    MQ_REMOVE(&q->chan[chan], node);
    MQ_INSERT_TAIL(&q->freeList, node);
    return 0;
}

// FUNC 0x0026F3D8 pdi1394_msg_clear
// Moves every node of channel chan back to the free list, then zeroes its
// count.
void pdi1394_msg_clear(int chan)
{
    Pdi1394MsgQueues *q = &s_pdiQueues;
    struct Pdi1394MsgNode *node;

    while ((node = MQ_FIRST(&q->chan[chan])) != NULL) {
        MQ_REMOVE(&q->chan[chan], node);
        MQ_INSERT_TAIL(&q->freeList, node);
    }
    q->count[chan] = 0;
}

// FUNC 0x0026F468 pdi1394_msg_remove_if
// Walks channel chan and removes (back to the free list) every message for
// which pred(arg, msg) is non-zero.  Bug in the original: the channel's
// count is not decremented, so it overstates the queue afterwards (later
// pushes then drop the oldest message early).
// Matching (96.2%): the 0xD0 bytes of the function list end with a stray
// `jr $ra; nop' at 0x0026F530, after the epilogue, which no code reaches;
// it has the size and alignment of an empty function (? one with no caller,
// compiled into this object).  Everything before it is exact: the list
// pointer (retail (chan << 3) + 8 + q), and `if (pred(...) == 0)' first
// (retail's layout: bnel to the removal, the next-node load in the else).
void pdi1394_msg_remove_if(int chan, int (*pred)(void *arg, Pdi1394Msg *msg),
                           void *arg)
{
    Pdi1394MsgQueues *q = &s_pdiQueues;
    Pdi1394MsgList *list = &q->chan[chan];
    struct Pdi1394MsgNode *node = MQ_FIRST(list);
    struct Pdi1394MsgNode *next;

    while (node != NULL) {
        if (pred(arg, &node->msg) == 0) {
            node = MQ_NEXT(node);
        } else {
            next = MQ_NEXT(node);
            MQ_REMOVE(list, node);
            MQ_INSERT_TAIL(&q->freeList, node);
            node = next;
        }
    }
}

// FUNC 0x0026F530 F_0026F530
/* An empty function (jr ra; nop) at the end of the object, no callers;
 * probably a debug hook compiled out.  (Its own entry since the 2026-10-16
 * regen; it was counted into pdi1394_msg_remove_if.) */
void F_0026F530(void)
{
}
