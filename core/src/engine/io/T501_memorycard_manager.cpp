/* T501 - the memory-card manager.  0x0023E268..0x00240AF0.
 *
 * g_00352F60 (McManager, 0x1880 bytes) is created by mc_manager_init (from
 * vol_init) and keeps, per card id (port * 4 + slot), the card state that a
 * poll with sceMcGetSlotMax / sceMcGetInfo finds (type, free clusters,
 * formatted, "changed" flag) and the status of the last operation.
 *
 * Operations (all on an id; status codes MC_* in vol_internal.h):
 *   blocking  mc_create_file mc_delete mc_check_file mc_format mc_get_dir
 *             mc_get_ent_space mc_mkdir mc_read_file mc_rename
 *             mc_set_file_info mc_unformat mc_write_file mc_save_files;
 *             they hold lock_sema (mc_do_* are the bodies, called with it
 *             held).  Each runs the libmc steps in turn (the G002 *_sync
 *             wrappers); `status' holds the code of the step being tried,
 *             and mc_status_from_result turns the last libmc result into the
 *             final code.
 *   async     mc_async_*: mc_request stores the request (one slot, no
 *             queueing: a new request overwrites a pending one), the worker
 *             thread mc_thread_main runs it, stores the status in `result',
 *             calls the callback and wakes the threads in mc_async_wait.
 *   polling   mc_manager_tick (every frame from render_thread_main) queues a
 *             poll (MC_CMD_POLL) every 4th call while idle: one slot of one
 *             port per poll (mc_ports_poll).
 * The getters (mc_slot_*) read the state with interrupts off. */
#include "engine/io/vol_internal.h"
#include "engine/io/T501_memorycard_manager.h"

/* T501's .sdata (0x00352F60..0x00352F72): the manager, mc_manager_tick's
 * counter, the id of the last mc_get_dir, then (8-aligned) mc_save_files's
 * "/".  Initialised: egcs writes them at their definition, before the
 * literal (an uninitialised global would come after it, at the end). */
McManager *g_00352F60 = NULL;                   /* 0x00352F60 */
s32 g_00352F64 = 0;                             /* 0x00352F64 */
s32 g_00352F68 = -1;                            /* 0x00352F68 */

/* max(a, b).  The libmc results are clamped with it: imax(res, MC_RES_NO_CARD).
 * An inline helper (its parameters are pseudos: retail keeps -11 in a register
 * of its own and compares the switch case against it). */
static inline int imax(int a, int b)
{
    if (a <= b)
        return b;
    return a;
}

// FUNC 0x0023E268 mc_slot_reset
//   Forgets the slot's state (interrupts off).  (Retail stores `changed'
//   first: the last store in the source holds the pointer's death, and
//   sched1 puts it first.)
void mc_slot_reset(McSlot *slot)
{
    int intrOn = intr_suspend();
    slot->name[0] = 0;
    slot->dir_pos = 0;
    slot->dir_count = 0;
    slot->need_info = 0;
    slot->type = 0;
    slot->free = 0;
    slot->format = 0;
    slot->changed = 0;
    if (intrOn)
        __asm__ volatile("ei");
}

// FUNC 0x0023E2B8 mc_slot_set_result
//   Stores the status; on a libmc error forgets the listing and, for -11
//   (no card) resets the slot, for -1 (sceMcResChangedCard) resets it and
//   flags it changed and in need of a full info poll.
void mc_slot_set_result(McSlot *slot, int res, int status)
{
    int intrOn = intr_suspend();
    slot->status = status;
    if (res < 0) {
        slot->name[0] = 0;
        int clamped = imax(res, MC_RES_NO_CARD);
        slot->dir_pos = 0;
        switch (clamped) {
        case sceMcResChangedCard:
            mc_slot_reset(slot);
            slot->need_info = 1;
            slot->changed = 1;
            break;
        case MC_RES_NO_CARD:
            mc_slot_reset(slot);
            break;
        }
    }
    if (intrOn)
        __asm__ volatile("ei");
}

// FUNC 0x0023E360 mc_slot_set_info
//   Stores the result of an info poll.  res 0 / -1: type, free, formatted
//   (-1 also sets changed).  Other errors: type 0 (changed when it was not
//   0), except -2 (sceMcResNoFormat) which keeps the new type; free and
//   formatted 0.
void mc_slot_set_info(McSlot *slot, int res, int type, int free, int format,
                      int changed)
{
    int intrOn = intr_suspend();
    int clamped = imax(res, MC_RES_NO_CARD);
    switch (clamped) {
    case sceMcResChangedCard:
        changed = 1;
        /* fall through */
    case sceMcResSucceed:
        slot->type = type;
        slot->free = free;
        slot->format = format != 0;
        break;
    default: {
        int oldType = slot->type;
        slot->type = 0;
        if (oldType != 0)
            changed = 1;
        if (res == sceMcResNoFormat)            /* the raw result, not the clamped one */
            slot->type = type;
        slot->free = 0;
        slot->format = 0;
        break;
    }
    }
    slot->changed = changed;
    if (intrOn)
        __asm__ volatile("ei");
}

// FUNC 0x0023E430 mc_slot_poll_info
//   sceMcGetInfo for the slot.  free / format are fetched only when the
//   slot needs it (need_info) or after a -1 (card changed: the call is
//   repeated with them); a 0 answer to a call with them is followed by one
//   more call without them.
void mc_slot_poll_info(McSlot *slot)
{
    int  type;                                  /* sp+0x0 */
    int  free;                                  /* sp+0x4 */
    int  format;                                /* sp+0x8 */
    int *wantType, *wantFree, *wantFormat;

    int intrOn = intr_suspend();
    wantType = &type;                           /* first address taken: type gets sp+0x0 */
    wantFree = NULL;
    wantFormat = NULL;
    free = slot->free;
    format = slot->format;
    if (slot->need_info != 0) {
        slot->need_info = 0;
        wantFree = &free;
        wantFormat = &format;
    }
    if (intrOn)
        __asm__ volatile("ei");

    int changed = 0;
    int res, again;
    do {
        res = mc_getinfo_sync(slot->port, slot->slot, wantType, wantFree, wantFormat);
        res = imax(res, MC_RES_NO_CARD);
        again = 0;
        switch (res) {
        case sceMcResSucceed:
            if (wantFree != NULL) {
                again = 1;
                wantFree = NULL;
                wantFormat = NULL;
            }
            break;
        case sceMcResChangedCard:
            again = 1;
            wantFree = &free;
            wantFormat = &format;
            changed = 1;
            break;
        case MC_RES_NO_CARD:
            type = 0;
            free = 0;
            format = 0;
            break;
        case sceMcResNoFormat:
            free = 0;
            changed = 1;
            format = 0;
            break;
        }
    } while (again);
    mc_slot_set_info(slot, res, type, free, format, changed);
}

// FUNC 0x0023E598 mc_port_poll
//   Re-reads the port's slot count (resetting all 4 slots when it changed),
//   polls the current slot and advances; 1 when it wrapped to slot 0.
int mc_port_poll(McPort *port)
{
    int oldCount = port->nslots;
    int count = min_int(MC_SLOTS, mc_getslotmax_sync(port->port));
    if (oldCount != count) {
        int intrOn = intr_suspend();
        port->nslots = count;
        port->cur = 0;
        for (int i = 0; i < MC_SLOTS; i++)      /* egcs counts it down, 3..0 */
            mc_slot_reset(&port->slots[i]);
        if (intrOn)
            __asm__ volatile("ei");
    }
    mc_slot_poll_info(&port->slots[port->cur]);
    if (++port->cur >= port->nslots) {
        port->cur = 0;
        return 1;
    }
    return 0;
}

// FUNC 0x0023E670 mc_ports_poll
//   Polls the current port; moves to the other port when it wrapped.
void mc_ports_poll(McPorts *ports)
{
    if (mc_port_poll(&ports->port[ports->cur])) {
        if (++ports->cur >= MC_PORTS)
            ports->cur = 0;
    }
}

/* The end of a card operation run by the worker thread: the status for
 * mc_async_wait and the request's callback.  (Called per case: retail has
 * the sequence twice, after mc_unformat and after mc_write_file, and the
 * other cases jump into the first copy.) */
static inline void mc_request_done(McManager *mgr, const McRequest *req, int status)
{
    McCallback cb = req->cb;
    void *cbArg = req->cb_arg;
    mgr->result = status;
    if (cb != NULL)
        cb(cbArg, status);
}

// FUNC 0x0023E6C8 mc_thread_main
//   The worker thread (priority 4, stack McManager+0x800, 0xC00 bytes).
//   Per request: copy it (interrupts off), run it, store the status and
//   call the callback (not for a poll, nor for unknown commands), then
//   clear `busy' and wake the waiting threads with the status.
void mc_thread_main(void *arg)
{
    McManager *mgr = (McManager *)arg;

    for (;;) {
        WaitSema(mgr->req_sema);
        if (mgr->quit != 0)
            break;

        int intrOn = intr_suspend();
        McRequest *req = &mgr->cur;
        mgr->cur = mgr->req;
        mgr->req.cmd = MC_CMD_NONE;
        if (intrOn)
            __asm__ volatile("ei");

        switch (mgr->cur.cmd) {
        case MC_CMD_NONE:
            break;
        case MC_CMD_POLL:
            /* Note: the lock is taken through the global, the ports of `mgr'. */
            WaitSema(g_00352F60->lock_sema);
            mc_ports_poll(&mgr->ports);
            signal_sema_helper(g_00352F60->lock_sema);
            break;
        case MC_CMD_CREATE_FILE:
            mc_request_done(mgr, req, mc_create_file(req->id, req->path, (const void *)req->ptr,
                                                     req->size));
            break;
        case MC_CMD_DELETE:
            mc_request_done(mgr, req, mc_delete(req->id, req->path));
            break;
        case MC_CMD_CHECK_FILE:
            mc_request_done(mgr, req, mc_check_file(req->id, req->path));
            break;
        case MC_CMD_FORMAT:
            mc_request_done(mgr, req, mc_format(req->id));
            break;
        case MC_CMD_GET_DIR:
            mc_request_done(mgr, req, mc_get_dir(req->id, req->path, (McDirEntry *)req->ptr,
                                                 req->offset, req->size));
            break;
        case MC_CMD_GET_ENT_SPACE:
            mc_request_done(mgr, req, mc_get_ent_space(req->id, req->path, (int *)req->ptr));
            break;
        case MC_CMD_MKDIR:
            mc_request_done(mgr, req, mc_mkdir(req->id, req->path));
            break;
        case MC_CMD_READ_FILE:
            mc_request_done(mgr, req, mc_read_file(req->id, req->path, (void *)req->ptr,
                                                   req->offset, req->size));
            break;
        case MC_CMD_RENAME:
            mc_request_done(mgr, req, mc_rename(req->id, req->path, (const char *)req->ptr));
            break;
        case MC_CMD_SET_FILE_INFO:
            mc_request_done(mgr, req, mc_set_file_info(req->id, req->path, (void *)req->ptr,
                                                       req->offset));
            break;
        case MC_CMD_UNFORMAT:                   /* the order of the functions in this file */
            mc_request_done(mgr, req, mc_unformat(req->id));
            break;
        case MC_CMD_WRITE_FILE:
            mc_request_done(mgr, req, mc_write_file(req->id, req->path, (const void *)req->ptr,
                                                    req->offset, req->size));
            break;
        }

        intrOn = intr_suspend();
        McWaiter *waiter = mgr->waiters;
        mgr->waiters = NULL;
        mgr->busy = 0;
        if (intrOn)
            __asm__ volatile("ei");
        while (waiter != NULL) {
            int tid = waiter->tid;
            waiter->result = mgr->result;
            waiter = waiter->next;
            WakeupThread(tid);
        }
    }
    worker_thread_exit(&mgr->thread);
}

// FUNC 0x0023E9F8 mc_manager_init
//   Once: mc_init_once, the manager (memalign(0x40, 0x1880), zeroed), its
//   semaphores (request: 0, lock: 1), the 2 x 4 slots, 8 polls, and the
//   worker thread.  Caller: vol_init.
void mc_manager_init(void)
{
    if (g_00352F60 == NULL) {
        mc_init_once();
        McManager *mgr = (McManager *)memalign(0x40, sizeof(McManager));
        memset(mgr, 0, sizeof(McManager));
        McPorts *ports = &mgr->ports;
        int i, s;                               /* i: the ports, then the polls */
        mgr->req_sema = create_mutex_sema(0);
        mgr->lock_sema = create_mutex_sema(1);
        mgr->quit = 0;
        for (i = 0; i < MC_PORTS; i++) {
            McPort *port = &ports->port[i];
            port->port = i;
            port->nslots = 0;
            McSlot *slot = port->slots;
            for (s = 0; s < MC_SLOTS; s++) {
                slot->slot = s;
                slot->port = i;
                mc_slot_reset(slot);
                slot++;
            }
        }
        g_00352F60 = mgr;
        for (i = 0; i < MC_PORTS * MC_SLOTS; i++)   /* egcs counts it down, 7..0 */
            mc_ports_poll(ports);
        worker_thread_start(&mgr->thread, mc_thread_main, mgr, 4, mgr->stack,
                            sizeof(mgr->stack));
    }
}

// FUNC 0x0023EB48 mc_manager_shutdown
//   Stops the worker thread, deletes the semaphores and frees the manager.
//   Caller: vol_shutdown.
void mc_manager_shutdown(void)
{
    McManager *mgr = g_00352F60;
    if (mgr != NULL) {
        mgr->quit = 1;
        signal_sema_helper(mgr->req_sema);
        worker_thread_join(&mgr->thread);
        DeleteSema(mgr->req_sema);
        DeleteSema(mgr->lock_sema);
        free(g_00352F60);
        g_00352F60 = NULL;
    }
}

// FUNC 0x0023EBA0 mc_request
//   Stores a request for the worker thread (path copied, "" for NULL),
//   marks the manager busy and signals the thread.  Does not wait for or
//   check a pending request.
void mc_request(int cmd, int id, const char *path, int ptr, int offset, int size,
                McCallback cb, void *cbArg)
{
    McManager *mgr = g_00352F60;
    McRequest *req = &mgr->req;
    int intrOn = intr_suspend();
    mgr->req.cmd = cmd;
    req->id = id;
    if (path != NULL)
        strcpy(mgr->req.path, path);
    else
        req->path[0] = 0;
    req->cb_arg = cbArg;
    req->ptr = ptr;
    req->offset = offset;
    req->size = size;
    mgr->busy = 1;
    req->cb = cb;
    if (intrOn)
        __asm__ volatile("ei");
    signal_sema_helper(mgr->req_sema);
}

// FUNC 0x0023EC88 mc_manager_tick
//   Every 4th call while idle: queue a poll.  Caller: render_thread_main.
void mc_manager_tick(void)
{
    if (g_00352F60 != NULL && g_00352F60->busy == 0) {
        if (++g_00352F64 >= 4) {
            g_00352F64 = 0;
            mc_request(MC_CMD_POLL, 0, NULL, 0, 0, 0, NULL, NULL);
        }
    }
}

// FUNC 0x0023ECF0 mc_port_slot_count
//   No callers.
int mc_port_slot_count(int port)
{
    if (port < MC_PORTS)
        return g_00352F60->ports.port[port].nslots;
    return 0;
}

// FUNC 0x0023ED18 mc_slot_valid
//   1 when the id's slot exists on its port.  (Negative ids are not
//   rejected.)
int mc_slot_valid(int id)
{
    int valid = 0;
    McManager *mgr = g_00352F60;
    int intrOn = intr_suspend();
    if (id < MC_PORTS * MC_SLOTS) {
        int port = id / MC_SLOTS;
        int slot = id % MC_SLOTS;
        valid = slot < mgr->ports.port[port].nslots;
    }
    if (intrOn)
        __asm__ volatile("ei");
    return valid;
}

// FUNC 0x0023EDA0 mc_slot_refresh
//   Polls the slot now (under the lock); returns and clears its changed
//   flag (0 for an invalid id).  Callers: the gt2k_menu memory-card menus.
int mc_slot_refresh(int id)
{
    if (!mc_slot_valid(id))
        return 0;
    mc_lock();
    mc_slot_poll_info(mc_slot_of(id));
    int changed = mc_slot_take_changed(id);
    mc_unlock();
    return changed;
}

// FUNC 0x0023EE30 mc_slot_take_changed
//   Returns and clears the changed flag (no id check).
int mc_slot_take_changed(int id)
{
    int intrOn = intr_suspend();
    McSlot *slot = mc_slot_of(id);
    int changed = slot->changed;
    slot->changed = 0;
    if (intrOn)
        __asm__ volatile("ei");
    return changed;
}

// FUNC 0x0023EEA8 mc_slot_status
//   Status of the last operation; MC_STATUS_NO_SLOT for an invalid id.
int mc_slot_status(int id)
{
    int status = MC_STATUS_NO_SLOT;
    int intrOn = intr_suspend();
    if (mc_slot_valid(id))
        status = mc_slot_of(id)->status;
    if (intrOn)
        __asm__ volatile("ei");
    return status;
}

// FUNC 0x0023EF40 mc_slot_type
int mc_slot_type(int id)
{
    int type = 0;
    int intrOn = intr_suspend();
    if (mc_slot_valid(id))
        type = mc_slot_of(id)->type;
    if (intrOn)
        __asm__ volatile("ei");
    return type;
}

// FUNC 0x0023EFD8 mc_slot_free
int mc_slot_free(int id)
{
    int free = 0;
    int intrOn = intr_suspend();
    if (mc_slot_valid(id))
        free = mc_slot_of(id)->free;
    if (intrOn)
        __asm__ volatile("ei");
    return free;
}

// FUNC 0x0023F070 mc_slot_formatted
bool mc_slot_formatted(int id)
{
    bool formatted = 0;
    int intrOn = intr_suspend();
    if (mc_slot_valid(id))
        formatted = mc_slot_of(id)->format;
    if (intrOn)
        __asm__ volatile("ei");
    return formatted;
}

// FUNC 0x0023F108 mc_slot_take_status
//   Returns and clears the status of the last operation; MC_STATUS_NO_SLOT for an
//   invalid id.
int mc_slot_take_status(int id)
{
    int status = MC_STATUS_NO_SLOT;
    int intrOn = intr_suspend();
    if (mc_slot_valid(id)) {
        McSlot *slot = mc_slot_of(id);
        status = slot->status;
        slot->status = MC_STATUS_OK;
    }
    if (intrOn)
        __asm__ volatile("ei");
    return status;
}

// FUNC 0x0023F1A8 mc_status_from_result
//   The status of an operation from the libmc result `res' (clamped at
//   -11): -4 (no entry) -> MC_STATUS_NO_ENTRY, -11 / -2 / -1 -> MC_STATUS_CARD_CHANGED,
//   anything else (success included) -> `status'.
int mc_status_from_result(int status, int res)
{
    int clamped = imax(res, MC_RES_NO_CARD);
    switch (clamped) {
    case MC_RES_NO_CARD:
    case sceMcResNoFormat:
    case sceMcResChangedCard:
        return MC_STATUS_CARD_CHANGED;
    case sceMcResNoEntry:
        return MC_STATUS_NO_ENTRY;
    default:
        return status;
    }
}

// FUNC 0x0023F208 mc_do_create_file
//   Open with 0x702 (SCE_WRONLY | 0x100 | SCE_CREAT | SCE_TRUNC), write,
//   close.  Lock held by the caller (mc_create_file).
int mc_do_create_file(int id, const char *path, const void *buf, int size)
{
    int status = MC_STATUS_NO_SLOT;
    if (mc_slot_valid(id)) {
        status = MC_STATUS_ERR_CREATE;
        int fd = mc_open_sync(id / MC_SLOTS, id % MC_SLOTS, path,
                              SCE_WRONLY | SCE_APPEND | SCE_CREAT | SCE_TRUNC);
        int res = fd;
        if (res >= 0) {
            status = MC_STATUS_ERR_WRITE;
            res = mc_write_sync(fd, buf, size);
            if (res >= 0) {
                status = MC_STATUS_ERR_CLOSE;
                res = mc_close_sync(fd);
                if (res == 0)
                    status = MC_STATUS_OK;
            }
        }
        status = mc_status_from_result(status, res);
        mc_slot_set_result(mc_slot_of(id), res, status);
        mc_slot_of(id)->need_info = 1;
        mc_slot_of(id)->name[0] = 0;
    }
    return status;
}

// FUNC 0x0023F358 mc_do_check_file
//   Opens `path' read-only and closes it: MC_STATUS_OK when it exists, MC_STATUS_NO_ENTRY
//   when not.  Lock held by the caller.
int mc_do_check_file(int id, const char *path)
{
    if (!mc_slot_valid(id))
        return MC_STATUS_NO_SLOT;
    int status = MC_STATUS_ERR_OPEN;
    int port = id / MC_SLOTS;
    int slot = id % MC_SLOTS;
    int res = mc_open_sync(port, slot, path, SCE_RDONLY);
    if (res >= 0) {
        status = MC_STATUS_ERR_CLOSE;
        res = mc_close_sync(res);
        if (res == 0)
            status = MC_STATUS_OK;
    }
    status = mc_status_from_result(status, res);
    mc_slot_set_result(mc_slot_of(id), res, status);
    mc_slot_of(id)->name[0] = 0;
    return status;
}

// FUNC 0x0023F460 mc_do_mkdir
//   Lock held by the caller.
int mc_do_mkdir(int id, const char *path)
{
    int status = MC_STATUS_NO_SLOT;
    if (mc_slot_valid(id)) {
        status = MC_STATUS_ERR_MKDIR;
        int res = mc_mkdir_sync(id / MC_SLOTS, id % MC_SLOTS, path);
        if (res == 0)
            status = MC_STATUS_OK;
        status = mc_status_from_result(status, res);
        mc_slot_set_result(mc_slot_of(id), res, status);
        mc_slot_of(id)->need_info = 1;
        mc_slot_of(id)->name[0] = 0;
    }
    return status;
}

// FUNC 0x0023F540 mc_do_write_file
//   Opens `path' with 0x103 (SCE_RDWR | 0x100); when it does not exist and
//   offset is 0, creates it with mode 0x200 (SCE_CREAT alone, no access
//   bits).  Seeks to `offset' (if not 0), writes, closes.  Lock held by the
//   caller.
int mc_do_write_file(int id, const char *path, const void *buf, int offset,
                     int size)
{
    int status = MC_STATUS_NO_SLOT;
    if (mc_slot_valid(id)) {
        int port = id / MC_SLOTS;
        int slot = id % MC_SLOTS;
        status = MC_STATUS_ERR_OPEN;
        int res = mc_open_sync(port, slot, path, SCE_RDWR | SCE_APPEND);
        int fd = res;
        if (res == sceMcResNoEntry && offset == 0) {
            res = mc_open_sync(port, slot, path, SCE_CREAT);
            status = MC_STATUS_ERR_CREATE;
            fd = res;
        }
        if (res >= 0) {
            status = MC_STATUS_ERR_SEEK;
            if (offset == 0 || (res = mc_seek_sync(fd, offset, 0 /* SEEK_SET */)) == offset) {
                status = MC_STATUS_ERR_WRITE;
                res = mc_write_sync(fd, buf, size);
                if (res >= 0) {
                    status = MC_STATUS_ERR_CLOSE;
                    res = mc_close_sync(fd);
                    if (res == 0)
                        status = MC_STATUS_OK;
                }
            }
        }
        status = mc_status_from_result(status, res);
        mc_slot_set_result(mc_slot_of(id), res, status);
        mc_slot_of(id)->need_info = 1;
        mc_slot_of(id)->name[0] = 0;
    }
    return status;
}

// FUNC 0x0023F700 mc_create_file
int mc_create_file(int id, const char *path, const void *buf, int size)
{
    WaitSema(g_00352F60->lock_sema);
    int status = mc_do_create_file(id, path, buf, size);
    signal_sema_helper(g_00352F60->lock_sema);
    return status;
}

// FUNC 0x0023F778 mc_delete
int mc_delete(int id, const char *path)
{
    int status = MC_STATUS_NO_SLOT;
    WaitSema(g_00352F60->lock_sema);
    if (mc_slot_valid(id)) {
        status = MC_STATUS_ERR_DELETE;
        int res = mc_delete_sync(id / MC_SLOTS, id % MC_SLOTS, path);
        if (res == 0)
            status = MC_STATUS_OK;
        status = mc_status_from_result(status, res);
        mc_slot_set_result(mc_slot_of(id), res, status);
        mc_slot_of(id)->need_info = 1;
        mc_slot_of(id)->name[0] = 0;
    }
    signal_sema_helper(g_00352F60->lock_sema);
    return status;
}

// FUNC 0x0023F870 mc_check_file
int mc_check_file(int id, const char *path)
{
    WaitSema(g_00352F60->lock_sema);
    int status = mc_do_check_file(id, path);
    signal_sema_helper(g_00352F60->lock_sema);
    return status;
}

// FUNC 0x0023F8C8 mc_format
int mc_format(int id)
{
    int status = MC_STATUS_NO_SLOT;
    WaitSema(g_00352F60->lock_sema);
    if (mc_slot_valid(id)) {
        status = MC_STATUS_ERR_FORMAT;
        int res = mc_format_sync(id / MC_SLOTS, id % MC_SLOTS);
        if (res >= 0)
            status = MC_STATUS_OK;
        status = mc_status_from_result(status, res);
        mc_slot_set_result(mc_slot_of(id), res, status);
        mc_slot_of(id)->need_info = 1;
        mc_slot_of(id)->name[0] = 0;
    }
    signal_sema_helper(g_00352F60->lock_sema);
    return status;
}

// FUNC 0x0023F9C0 mc_get_dir
//   Lists `path' (a sceMcGetDir pattern) into out[0 .. count): skips to
//   entry `skip' first, continuing the previous listing when it is of the
//   same id and pattern and not past `skip', else restarting it; reads up to
//   0x20 entries per sceMcGetDir into the manager's 0x800-byte table and
//   repacks them (McDirEntry); entries not returned are zeroed.  dir_count
//   = entries returned.  MC_STATUS_ERR_GETDIR on a getdir error.
int mc_get_dir(int id, const char *path, McDirEntry *out, int skip, u32 count)
{
    int status = MC_STATUS_NO_SLOT;                    /* sp+0x4 */
    WaitSema(g_00352F60->lock_sema);
    if (mc_slot_valid(id)) {
        McManager *mgr = g_00352F60;            /* sp+0x8 */
        status = MC_STATUS_ERR_GETDIR;
        McSlot *slot = &mgr->ports.port[id / MC_SLOTS].slots[id % MC_SLOTS];
        slot->dir_count = 0;
        if (g_00352F68 != id) {
            g_00352F68 = id;
            slot->dir_pos = MC_DIR_POS_RESTART;
        }
        if (strcmp(slot->name, path) != 0) {
            strcpy(slot->name, path);
            slot->dir_pos = MC_DIR_POS_RESTART;
        }
        int n = skip - slot->dir_pos;           /* entries to skip, then the entry index, then the
                                                   zeroed entries (one register in retail) */
        int first = skip == 0;                  /* the next sceMcGetDir starts the listing (mode 0) */
        if (n < 0) {
            slot->dir_pos = 0;
            n = skip;
            first = 1;
        }

        int res;
        /* skip forward */
        while (n != 0) {
            int chunk = n <= MC_GETDIR_MAX ? n : MC_GETDIR_MAX;
            res = mc_getdir_sync(slot->port, slot->slot, path, first ^ 1, chunk, mgr->dir);
            if (res != chunk)
                goto finish;
            first = 0;
            n -= res;
            slot->dir_pos += res;
        }

        /* read */
        u32 chunk;
        do {
            memset(mgr->dir, 0, sizeof(mgr->dir));
            chunk = count <= MC_GETDIR_MAX ? count : MC_GETDIR_MAX;
            res = mc_getdir_sync(slot->port, slot->slot, path, first ^ 1, chunk, mgr->dir);
            if (res < 0)
                goto finish;
            const sceMcTblGetDir *src = mgr->dir;
            for (n = 0; n < res; n++) {
                memset(out, 0, sizeof(*out));
                out->create.sec   = src->_Create.Sec;
                out->create.min   = src->_Create.Min;
                out->create.hour  = src->_Create.Hour;
                out->create.day   = src->_Create.Day;
                out->create.month = src->_Create.Month;
                out->create.year  = src->_Create.Year;
                out->modify.sec   = src->_Modify.Sec;
                out->modify.min   = src->_Modify.Min;
                out->modify.hour  = src->_Modify.Hour;
                out->modify.day   = src->_Modify.Day;
                out->modify.month = src->_Modify.Month;
                out->modify.year  = src->_Modify.Year;
                out->size         = src->FileSizeByte;
                out->attr         = src->AttrFile;
                out->pdaAplNo     = src->PdaAplNo;
                strcpy(out->name, (const char *)src->EntryName);
                src++;
                out++;
                slot->dir_count++;
                slot->dir_pos++;
            }
            first = 0;
            count -= chunk;
            if ((u32)res != chunk)
                break;
        } while (count != 0);
        for (n = 0; (u32)n < count; n++) {
            memset(out, 0, sizeof(*out));
            out++;
        }

    finish:
        if (res >= 0)
            status = MC_STATUS_OK;
        status = mc_status_from_result(status, res);
        mc_slot_set_result(slot, res, status);
    }
    signal_sema_helper(g_00352F60->lock_sema);
    return status;
}

// FUNC 0x0023FD08 mc_get_ent_space
//   *out = sceMcGetEntSpace(path) (0 on failure).  A failure keeps the
//   status MC_STATUS_NO_SLOT (no code of its own).
int mc_get_ent_space(int id, const char *path, int *out)
{
    int status = MC_STATUS_NO_SLOT;
    WaitSema(g_00352F60->lock_sema);
    *out = 0;
    if (mc_slot_valid(id)) {
        int port = id / MC_SLOTS;
        int slot = id % MC_SLOTS;
        int res = mc_getentspace_sync(port, slot, path);
        if (res >= 0) {
            *out = res;
            status = MC_STATUS_OK;
        }
        status = mc_status_from_result(status, res);
        mc_slot_set_result(mc_slot_of(id), res, status);
        mc_slot_of(id)->name[0] = 0;
    }
    signal_sema_helper(g_00352F60->lock_sema);
    return status;
}

// FUNC 0x0023FE28 mc_mkdir
int mc_mkdir(int id, const char *path)
{
    WaitSema(g_00352F60->lock_sema);
    int status = mc_do_mkdir(id, path);
    signal_sema_helper(g_00352F60->lock_sema);
    return status;
}

// FUNC 0x0023FE80 mc_read_file
//   Open read-only, seek to `offset' (if not 0), read `size' bytes, close.
//   read_size = bytes read (-1 until the read succeeded).
int mc_read_file(int id, const char *path, void *buf, int offset, int size)
{
    int status = MC_STATUS_NO_SLOT;
    WaitSema(g_00352F60->lock_sema);
    if (mc_slot_valid(id)) {
        int port = id / MC_SLOTS;
        int slot = id % MC_SLOTS;
        status = MC_STATUS_ERR_OPEN;
        mc_slot_of(id)->read_size = -1;
        int res = mc_open_sync(port, slot, path, SCE_RDONLY);
        int fd = res;
        if (res >= 0) {
            status = MC_STATUS_ERR_SEEK;
            if (offset == 0 || (res = mc_seek_sync(fd, offset, 0 /* SEEK_SET */)) == offset) {
                status = MC_STATUS_ERR_READ;
                res = mc_read_sync(fd, buf, size);
                if (res >= 0) {
                    status = MC_STATUS_ERR_CLOSE;
                    mc_slot_of(id)->read_size = res;
                    res = mc_close_sync(fd);
                    if (res == 0)
                        status = MC_STATUS_OK;
                }
            }
        }
        status = mc_status_from_result(status, res);
        mc_slot_set_result(mc_slot_of(id), res, status);
        mc_slot_of(id)->name[0] = 0;
    }
    signal_sema_helper(g_00352F60->lock_sema);
    return status;
}

// FUNC 0x00240050 mc_rename
int mc_rename(int id, const char *path, const char *newname)
{
    int status = MC_STATUS_NO_SLOT;
    WaitSema(g_00352F60->lock_sema);
    if (mc_slot_valid(id)) {
        status = MC_STATUS_ERR_FORMAT;
        int res = mc_rename_sync(id / MC_SLOTS, id % MC_SLOTS, path, newname);
        if (res >= 0)
            status = MC_STATUS_OK;
        status = mc_status_from_result(status, res);
        mc_slot_set_result(mc_slot_of(id), res, status);
        mc_slot_of(id)->need_info = 1;
        mc_slot_of(id)->name[0] = 0;
    }
    signal_sema_helper(g_00352F60->lock_sema);
    return status;
}

// FUNC 0x00240160 mc_set_file_info
//   info: a sceMcTblGetDir (Sony's prototype types it const char *).
int mc_set_file_info(int id, const char *path, void *info, unsigned int valid)
{
    int status = MC_STATUS_NO_SLOT;
    WaitSema(g_00352F60->lock_sema);
    if (mc_slot_valid(id)) {
        status = MC_STATUS_ERR_FORMAT;
        int res = mc_setfileinfo_sync(id / MC_SLOTS, id % MC_SLOTS, path,
                                      (const char *)info, valid);
        if (res >= 0)
            status = MC_STATUS_OK;
        status = mc_status_from_result(status, res);
        mc_slot_set_result(mc_slot_of(id), res, status);
        mc_slot_of(id)->need_info = 1;
        mc_slot_of(id)->name[0] = 0;
    }
    signal_sema_helper(g_00352F60->lock_sema);
    return status;
}

// FUNC 0x00240280 mc_unformat
int mc_unformat(int id)
{
    int status = MC_STATUS_NO_SLOT;
    WaitSema(g_00352F60->lock_sema);
    if (mc_slot_valid(id)) {
        status = MC_STATUS_ERR_UNFORMAT;
        int res = mc_unformat_sync(id / MC_SLOTS, id % MC_SLOTS);
        if (res >= 0)
            status = MC_STATUS_OK;
        status = mc_status_from_result(status, res);
        mc_slot_set_result(mc_slot_of(id), res, status);
        mc_slot_of(id)->need_info = 1;
        mc_slot_of(id)->name[0] = 0;
    }
    signal_sema_helper(g_00352F60->lock_sema);
    return status;
}

// FUNC 0x00240378 mc_write_file
int mc_write_file(int id, const char *path, const void *buf, int offset, int size)
{
    WaitSema(g_00352F60->lock_sema);
    int status = mc_do_write_file(id, path, buf, offset, size);
    signal_sema_helper(g_00352F60->lock_sema);
    return status;
}

// FUNC 0x00240400 mc_save_files
//   Writes a save under the lock: "/<dir>" (created when it does not
//   exist), then "/<dir>/<file>" for file0 and the optional file1 / file2,
//   then "/<dir>/icon.sys" (0x3C4 bytes); stops at the first failure.
//   No callers.
int mc_save_files(int id, const McSaveSet *set)
{
    char path[0x40];                            /* sp+0x00 */
    int status;

    WaitSema(g_00352F60->lock_sema);
    path[0] = '/';
    strcpy(path + 1, set->dir);
    status = mc_do_check_file(id, path);
    if (status == MC_STATUS_OK ||
        (status == MC_STATUS_NO_ENTRY && (status = mc_do_mkdir(id, path)) == MC_STATUS_OK)) {
        strcat(path, "/");
        char *fileName = path + strlen(path);
        strcpy(fileName, set->file0->name);
        status = mc_do_write_file(id, path, set->file0->data, 0, set->file0->size);
        if (status != MC_STATUS_OK)
            goto end;
        if (set->file1 != NULL) {               /* NULL: straight to file2 (retail's beql) */
            strcpy(fileName, set->file1->name);
            status = mc_do_write_file(id, path, set->file1->data, 0, set->file1->size);
            if (status != MC_STATUS_OK)
                goto end;
        }
        if (set->file2 != NULL) {
            strcpy(fileName, set->file2->name);
            status = mc_do_write_file(id, path, set->file2->data, 0, set->file2->size);
            if (status != MC_STATUS_OK)
                goto end;
        }
        strcpy(fileName, "icon.sys");
        status = mc_do_write_file(id, path, set->icon_sys, 0, MC_ICON_SYS_SIZE);
    }
end:
    signal_sema_helper(g_00352F60->lock_sema);
    return status;
}

// FUNC 0x002405B8 mc_lock
void mc_lock(void)
{
    WaitSema(g_00352F60->lock_sema);
}

// FUNC 0x002405D8 mc_unlock
void mc_unlock(void)
{
    signal_sema_helper(g_00352F60->lock_sema);
}

/* The asynchronous front-ends: mc_request(cmd, id, path, ptr, offset, size,
 * cb, cbArg).  None of them has a caller. */

// FUNC 0x002405F8 mc_async_write_file
//   A write at offset 0.
void mc_async_write_file(int id, const char *path, const void *buf, int size,
                         McCallback cb, void *cbArg)
{
    mc_request(MC_CMD_WRITE_FILE, id, path, (int)buf, 0, size, cb, cbArg);
}

// FUNC 0x00240640 mc_async_delete
void mc_async_delete(int id, const char *path, McCallback cb, void *cbArg)
{
    mc_request(MC_CMD_DELETE, id, path, 0, 0, 0, cb, cbArg);
}

// FUNC 0x00240680 mc_async_check_file
void mc_async_check_file(int id, const char *path, McCallback cb, void *cbArg)
{
    mc_request(MC_CMD_CHECK_FILE, id, path, 0, 0, 0, cb, cbArg);
}

// FUNC 0x002406C0 mc_async_format
void mc_async_format(int id, McCallback cb, void *cbArg)
{
    mc_request(MC_CMD_FORMAT, id, NULL, 0, 0, 0, cb, cbArg);
}

// FUNC 0x002406F8 mc_async_get_dir
void mc_async_get_dir(int id, const char *path, McDirEntry *out, int skip,
                      u32 count, McCallback cb, void *cbArg)
{
    mc_request(MC_CMD_GET_DIR, id, path, (int)out, skip, (int)count, cb, cbArg);
}

// FUNC 0x00240748 mc_async_get_dir0
//   A listing with skip 0 and count 0.
void mc_async_get_dir0(int id, const char *path, McDirEntry *out,
                       McCallback cb, void *cbArg)
{
    mc_request(MC_CMD_GET_DIR, id, path, (int)out, 0, 0, cb, cbArg);
}

// FUNC 0x00240788 mc_async_mkdir
void mc_async_mkdir(int id, const char *path, McCallback cb, void *cbArg)
{
    mc_request(MC_CMD_MKDIR, id, path, 0, 0, 0, cb, cbArg);
}

// FUNC 0x002407C8 mc_async_read_file
void mc_async_read_file(int id, const char *path, void *buf, int offset, int size,
                        McCallback cb, void *cbArg)
{
    mc_request(MC_CMD_READ_FILE, id, path, (int)buf, offset, size, cb, cbArg);
}

// FUNC 0x00240818 mc_async_rename
void mc_async_rename(int id, const char *path, const char *newname,
                     McCallback cb, void *cbArg)
{
    mc_request(MC_CMD_RENAME, id, path, (int)newname, 0, 0, cb, cbArg);
}

// FUNC 0x00240858 mc_async_set_file_info
void mc_async_set_file_info(int id, const char *path, void *info, unsigned int valid,
                            McCallback cb, void *cbArg)
{
    mc_request(MC_CMD_SET_FILE_INFO, id, path, (int)info, (int)valid, 0, cb, cbArg);
}

// FUNC 0x002408A0 mc_async_unformat
void mc_async_unformat(int id, McCallback cb, void *cbArg)
{
    mc_request(MC_CMD_UNFORMAT, id, NULL, 0, 0, 0, cb, cbArg);
}

// FUNC 0x002408D8 mc_async_write_file_at
void mc_async_write_file_at(int id, const char *path, const void *buf, int offset,
                            int size, McCallback cb, void *cbArg)
{
    mc_request(MC_CMD_WRITE_FILE, id, path, (int)buf, offset, size, cb, cbArg);
}

// FUNC 0x00240928 mc_async_wait
//   While a request is queued or running: put the calling thread on the
//   waiter list and sleep until the worker wakes it with the status.  Else
//   the status of the last request.  No callers.
int mc_async_wait(void)
{
    McManager *mgr = g_00352F60;
    McWaiter self;                              /* sp+0x00 */
    int mustSleep;

    int intrOn = intr_suspend();
    if (mgr->busy != 0) {
        self.next = mgr->waiters;
        int tid = GetThreadId();
        mgr->waiters = &self;                   /* on the stack: unlinked by the worker before the wakeup */
        self.tid = tid;
        mustSleep = 1;
    } else {
        mustSleep = 0;
        self.result = mgr->result;
    }
    if (intrOn)
        __asm__ volatile("ei");
    if (mustSleep)
        SleepThread();
    return self.result;
}

// FUNC 0x002409B0 mc_slot_dir_count
int mc_slot_dir_count(int id)
{
    int count = 0;
    int intrOn = intr_suspend();
    if (mc_slot_valid(id))
        count = mc_slot_of(id)->dir_count;
    if (intrOn)
        __asm__ volatile("ei");
    return count;
}

// FUNC 0x00240A48 mc_slot_read_size
int mc_slot_read_size(int id)
{
    int size = 0;
    int intrOn = intr_suspend();
    if (mc_slot_valid(id))
        size = mc_slot_of(id)->read_size;
    if (intrOn)
        __asm__ volatile("ei");
    return size;
}

// FUNC 0x00240AE0 min_int
//   min(a, b).  Also called by vol_thread_main (T503).
int min_int(int a, int b)
{
    return a < b ? a : b;
}
