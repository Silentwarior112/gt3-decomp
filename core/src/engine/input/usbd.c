/* usbd.c - EE side of Polyphony's USB device manager.
 *
 * usbd_init starts two threads at the caller's priority:
 *   - the RPC server thread (usbd_rpc_thread, 0x00265880): SIF RPC server
 *     'PUSB' whose receive buffer is the manager itself; the IOP driver calls
 *     it with fno = event type (USBD_EV_ATTACH / DETACH / ERROR) and a
 *     message {class, data word, 8 key bytes}; usbd_rpc_event queues an
 *     event;
 *   - the dispatcher thread (usbd_dispatch_thread): woken from the vblank
 *     handler, it calls each class driver's connect callback with the first
 *     queued event of its class (when new events arrived), each driver's poll
 *     callback, then delivers the queued events to the class handlers.
 * Class drivers register with usbd_register_driver (only the PJOY joystick
 * class 2 exists, pjoy.c).  Callers: usbd_init from
 * IOPLoadModule's cmdInitClients (0x00102A90, priority 1), usbjoy_start from
 * cmdBindClients (0x00102AB8).
 */
#include "engine/system/system_drivers_internal.h"

/* .sdata 0x00353170..0x00353184, then usbd_rpc_event's three names (8-aligned,
 * 0x00353188..): initialised words, so the original's .sdata (an
 * uninitialised C global would be a common).  The ids are -1 until
 * usbd_init. */
int g_usbd_started = 0;                     /* 0x00353170 */
int g_usbd_wake_sema = -1;                  /* 0x00353174 */
int g_usbd_lock_sema = -1;                  /* 0x00353178 */
int g_usbd_dispatch_tid = -1;               /* 0x0035317C */
int g_usbd_rpc_tid = -1;                    /* 0x00353180 */

/* .bss 0x004A7640..0x004A9840: file statics (G002's .bss is in link order,
 * between u121 / u122's statics and the PJOY client's, not among the
 * commons).  The stacks are 64-aligned: the first one starts at 0x004A7840,
 * 0x30 bytes after the manager's end (0x004A7810). */
static UsbMgr g_usbd;                       /* 0x004A7640 */
static u8 g_usbd_rpc_stack[USBD_STACK_SIZE] __attribute__((aligned(64)));      /* 0x004A7840 RPC server thread stack */
static u8 g_usbd_dispatch_stack[USBD_STACK_SIZE] __attribute__((aligned(64))); /* 0x004A8840 dispatcher thread stack */

// FUNC 0x00265530 usbd_event_alloc
/* First free of the 5 event slots: cleared, marked in use.  NULL if none. */
UsbEvent *usbd_event_alloc(UsbMgr *m)
{
    int i;

    for (i = 0; i < USBD_NUM_EVENTS; i++) {
        if (m->events[i].in_use == 0) {
            memset(&m->events[i], 0, sizeof(UsbEvent));
            m->events[i].in_use = 1;
            return &m->events[i];
        }
    }
    return NULL;
}

// FUNC 0x002655A8 usbd_event_free
void usbd_event_free(UsbMgr *m, UsbEvent *ev)
{
    ev->in_use = 0;
}

// FUNC 0x002655B0 pjoy_bind
/* Binds the PJOY client to its IOP server: sceSifBindRpc is re-issued after
 * every vblank until cd.serve is set (not the usual single bind + spin).
 * Returns 1 if a bind request failed, else 0 after one more vblank. */
int pjoy_bind(Pjoy *pj)
{
    for (;;) {
        if (sceSifBindRpc(&pj->cd, pj->server_id, 0) < 0)
            return 1;
        wait_vblank_start();
        if (pj->cd.serve != NULL)
            break;
    }
    wait_vblank_start();
    return 0;
}

// FUNC 0x00265610 usbd_init
/* Once: two semaphores (vblank wake: max 1, init 0; dispatcher lock: max 1,
 * init 1; the other SemaParam fields stay stack garbage), empty lists, the
 * caller's thread id, a vblank-start INTC handler (enabled), then the RPC
 * server thread and the dispatcher thread, 0x1000-byte stacks, both at
 * `priority` and started with the manager as argument.  Returns 0. */
int usbd_init(int priority)
{
    UsbMgr *m = &g_usbd;
    struct ThreadParam tp;          /* sp+0x00 */
    struct SemaParam sema;          /* sp+0x30 */

    /* The early return (not an if around the body) gives retail's
     * `li v0,0' ahead of the epilogue at the shared return label; the
     * second SemaParam is set initCount first (maxCount first: 99.4%). */
    if (g_usbd_started != 0)
        return 0;
    g_usbd_started = 1;
    sema.maxCount = 1;
    sema.initCount = 0;
    g_usbd_wake_sema = CreateSema(&sema);
    sema.initCount = 1;
    sema.maxCount = 1;
    g_usbd_lock_sema = CreateSema(&sema);

    UQ_INIT(m->ev_head, m->ev_tail);
    UQ_INIT(m->conn_head, m->conn_tail);
    UQ_INIT(m->poll_head, m->poll_tail);
    m->thread_id = GetThreadId();
    m->intc_id = AddIntcHandler2(INTC_VBLANK_S, usbd_vblank_handler, 0, m);
    EnableIntc(INTC_VBLANK_S);

    tp.entry = usbd_rpc_thread;
    tp.stack = g_usbd_rpc_stack;
    tp.stackSize = USBD_STACK_SIZE;
    tp.initPriority = priority;
    tp.gpReg = _gp;
    tp.option = 0;
    g_usbd_rpc_tid = CreateThread(&tp);
    StartThread(g_usbd_rpc_tid, m);

    tp.entry = usbd_dispatch_thread;
    tp.stack = g_usbd_dispatch_stack;
    tp.stackSize = USBD_STACK_SIZE;
    tp.initPriority = priority;
    tp.gpReg = _gp;
    tp.option = 0;
    g_usbd_dispatch_tid = CreateThread(&tp);
    StartThread(g_usbd_dispatch_tid, m);
    return 0;
}

// FUNC 0x00265770 usbjoy_start
/* Opens USB class 2 (the PJOY joystick class) and maps its ports by key
 * side (mode 1). */
void usbjoy_start(void)
{
    usbcls_set_mode(usbcls_get(USBD_CLASS_PJOY), USBCLS_MAP_BY_SIDE);  /* tail call */
}

// FUNC 0x00265798 usbd_exit
/* Undo usbd_init: close class 2, clear the started flag, remove the vblank
 * handler, terminate and delete the dispatcher and RPC server threads.  The
 * semaphores are not deleted and the RPC server is not unregistered.  No
 * caller. */
void usbd_exit(void)
{
    UsbMgr *m = &g_usbd;

    if (g_usbd_started != 0) {
        usbcls_close(USBD_CLASS_PJOY);
        g_usbd_started = 0;
        RemoveIntcHandler(INTC_VBLANK_S, m->intc_id);
        TerminateThread(g_usbd_dispatch_tid);
        TerminateThread(g_usbd_rpc_tid);
        DeleteThread(g_usbd_dispatch_tid);
        DeleteThread(g_usbd_rpc_tid);           /* tail call */
    }
}

// FUNC 0x00265808 usbd_vblank_handler
/* Vblank start: if the dispatcher is idle (iPollSema of its lock returns the
 * id), give the lock back and wake the dispatcher.  `sync; ei`, returns 0. */
int usbd_vblank_handler(int cause, void *arg, void *addr)
{
    if (g_usbd_lock_sema == iPollSema(g_usbd_lock_sema)) {     /* this operand order: bne lock, result */
        iSignalSema(g_usbd_lock_sema);
        iSignalSema(g_usbd_wake_sema);
    }
    U123_SYNC();
    U123_EI();
    return 0;
}

// FUNC 0x00265850 usbd_dispatch_thread
/* Forever: wait for the vblank wake-up, take the lock, dispatch, release. */
void usbd_dispatch_thread(void *arg)
{
    for (;;) {
        WaitSema(g_usbd_wake_sema);
        WaitSema(g_usbd_lock_sema);
        usbd_dispatch();
        SignalSema(g_usbd_lock_sema);
    }
}

// FUNC 0x00265880 usbd_rpc_thread
/* The thread entry stored by usbd_init.  Registers the 'PUSB' server on its
 * own queue, receive buffer = the manager, and runs the request loop (tail
 * call). */
void usbd_rpc_thread(void *arg)
{
    UsbMgr *m = (UsbMgr *)arg;

    sceSifSetRpcQueue(&m->qd, GetThreadId());
    sceSifRegisterRpc(&m->sd, USBD_RPC_ID, usbd_rpc_event, m, NULL, NULL, &m->qd);
    sceSifRpcLoop(&m->qd);
}

/* 8 bytes read big-endian. */
static __inline__ u64 usbd_be64(u8 *p)
{
    u64 v;
    int i;

    for (v = 0, i = 0; i < 8; i++)
        v |= (u64)p[i] << ((7 - i) * 8);
    return v;
}

// FUNC 0x002658E8 usbd_rpc_event
/* RPC function of 'PUSB': fno = event type, data = the manager (its first 16
 * bytes are the message).  Counts the event, takes a free event slot (not
 * checked for NULL: with 5 events outstanding it writes through NULL),
 * binds it to the class handler if one is registered (else remembers the
 * class in pending_cls), stores type, class, data word and the 8 key bytes
 * as a big-endian u64, and appends it to the queue.  Returns NULL (no reply
 * data).  The names table is copied to the stack and never used (a removed
 * debug print). */
void *usbd_rpc_event(unsigned int fno, void *data, int size)
{
    /* sp+0x00: an initialised local array, so egcs block-copies the
     * anonymous .rodata table (0x00345AA8; strings in .sdata) with ldl/ldr. */
    char *names[] = { NULL, "attach", "detach", "error" };
    UsbMgr *m = (UsbMgr *)data;
    int cls = m->msg_cls;
    UsbEvent *ev;
    int word;

    m->new_events++;
    ev = usbd_event_alloc(m);
    if (m->handler[cls] == NULL) {
        ev->pending_cls = cls;
    } else {
        ev->fn = m->handler[cls];
        ev->arg = m->handler_arg[cls];
    }
    /* `word' read first (retail's first load), the key through an inline
     * helper (its argument is set up before the stores, the loop's
     * invariants then come out in retail's order). */
    word = m->msg_word;
    ev->type = fno;
    ev->cls = cls;
    ev->data.id = word;
    ev->data.key = usbd_be64(m->msg_bytes);
    STQ_INSERT_TAIL(m->ev_tail, ev);
    return NULL;
}

// FUNC 0x002659E8 usbd_dispatch
/* Nothing while no driver is registered.  If events arrived since the last
 * pass: for every connect entry, the first queued event of its class is
 * passed to its connect callback (drv, type, &data).  Then every poll
 * callback runs.  Then the queue is drained in order: an event whose class
 * had no handler when it arrived is bound now, called (arg, type, &data),
 * unlinked and freed.
 * Quirk: if a pending event's class still has no handler, the loop retries
 * the same event forever (the lock stays taken; only a registration from
 * another thread ends it). */
void usbd_dispatch(void)
{
    UsbMgr *m = &g_usbd;
    UsbEvent *ev;

    if (m->ndrivers == 0)
        return;

    if (m->new_events != 0) {
        UsbConn *conn = m->conn_head;

        m->new_events = 0;
        for (; conn != NULL; conn = conn->next) {
            for (ev = m->ev_head; ev != NULL; ev = ev->next) {
                if (ev->cls == conn->cls) {
                    conn->fn(conn->drv, ev->type, &ev->data);
                    break;
                }
            }
        }
    }

    {
        UsbPoll *poll;

        for (poll = m->poll_head; poll != NULL; poll = poll->next)
            poll->fn(poll->drv);
    }

    /* `ev = m->ev_head' as the for-increment: the `continue' path reuses
     * the head value loaded at the loop top (retail's $a1 copy). */
    for (ev = m->ev_head; ev != NULL; ev = m->ev_head) {
        int pending = ev->pending_cls;

        if (pending != 0) {
            if (m->handler[pending] == NULL)
                continue;               /* spins on the same event */
            ev->fn = m->handler[pending];
            ev->pending_cls = 0;
            ev->arg = m->handler_arg[pending];
        }
        ev->fn(ev->arg, ev->type, &ev->data);
        /* Remove the head (STAILQ): a statement macro with a pointer to the
         * head field, as retail computes &m->ev_head in the loop, ahead of
         * the ev->next load (written plainly, loop.c hoists it). */
        do {
            UsbEvent **hp = &m->ev_head;

            if ((*hp = ev->next) == NULL)
                m->ev_tail = hp;
        } while (0);
        usbd_event_free(m, ev);
    }
}

// FUNC 0x00265B30 usbd_register_driver
/* Installs `handler` (with `arg`) for drv->cls; NULL if the class already
 * has one.  Appends a connect entry (malloc 0x14) and a poll entry (malloc
 * 0x10) for the callbacks the driver has (malloc results not checked), calls
 * drv->connect(drv, USBD_EV_ATTACH, NULL) once, counts the driver and
 * returns `handler` (which callers keep as the registration handle, see
 * usbd_unregister_driver). */
UsbClassFn usbd_register_driver(UsbDriver *drv, UsbClassFn handler, void *arg)
{
    UsbMgr *m = &g_usbd;
    int cls = drv->cls;

    if (m->handler[cls] != NULL)
        return NULL;
    m->handler_arg[cls] = arg;
    m->handler[cls] = handler;

    if (drv->connect != NULL) {
        UsbConn *conn = (UsbConn *)malloc(sizeof(UsbConn));

        conn->cls = cls;
        conn->fn = drv->connect;
        conn->drv = drv;
        TQ_INSERT_TAIL(m->conn_tail, conn);
    }
    if (drv->poll != NULL) {
        UsbPoll *poll = (UsbPoll *)malloc(sizeof(UsbPoll));

        poll->fn = drv->poll;
        poll->drv = drv;
        TQ_INSERT_TAIL(m->poll_tail, poll);
    }
    if (drv->connect != NULL)
        drv->connect(drv, USBD_EV_ATTACH, NULL);
    m->ndrivers++;
    return handler;
}

// FUNC 0x00265C40 usbd_unregister_driver
/* Finds the class whose handler equals `drv` (3 classes), clears it, then
 * unlinks and frees the connect entry with drv->connect (returns if there is
 * none) and the poll entry with drv->poll.  The driver count only drops when
 * drv->poll is NULL.
 * Bug: the only caller (usbcls_close) passes the handle returned by
 * usbd_register_driver, which is the class handler function itself, so
 * `drv->connect` / `drv->poll` read code words of usbcls_event. */
void usbd_unregister_driver(UsbDriver *drv)
{
    UsbMgr *m = &g_usbd;
    int cls;

    /* The original's exits: `goto found' / `goto poll' out of the search
     * loops, the unlink inside them (retail's found blocks sit inside each
     * loop, the not-found paths return directly). */
    for (cls = 0; cls < USBD_NUM_CLASSES; cls++)
        if ((void *)m->handler[cls] == (void *)drv)
            goto found;
    return;
found:
    m->handler[cls] = NULL;

    if (drv->connect != NULL) {
        UsbConn *conn;

        for (conn = m->conn_head; conn != NULL; conn = conn->next) {
            if (conn->fn == drv->connect) {
                TQ_REMOVE(m->conn_tail, conn);
                free(conn);
                goto poll;
            }
        }
        return;
    }
poll:
    if (drv->poll != NULL) {
        UsbPoll *poll;

        for (poll = m->poll_head; poll != NULL; poll = poll->next) {
            if (poll->fn == drv->poll) {
                TQ_REMOVE(m->poll_tail, poll);
                free(poll);
                return;
            }
        }
    } else
        m->ndrivers--;
}
