/* pjoy.c - the USB joystick class driver ('PJOY' SIF RPC client),
 * Logitech classic force-feedback command builders, and the USB class /
 * logical-port mapper.
 *
 * Flow: usbcls_get(2) builds the PJOY client (pjoy_init), points it at the
 * class-2 mapper and registers it with the USB manager.  On attach events
 * the manager calls pjoy_connect (refresh attach states and device info);
 * every dispatcher pass calls pjoy_poll (read the input state of attached
 * devices, send the queued FF commands).  The mapper's handler
 * usbcls_event keeps a key-sorted device list and rebuilds the logical port
 * -> device id map.  The game reads through pjoy_read_state / pjoy_get_info
 * and queues FF commands with ff_send_download_play / ff_build_constant
 * (T479 USBSteeringPort::autoCentre / update, 0x00230938 / 0x00230A90).
 *
 * Result codes of the public calls: PJOY_OK, PJOY_NOT_ATTACHED,
 * PJOY_NO_DEVICE (no device on that logical port), PJOY_BUSY (lock taken /
 * RPC in flight), PJOY_QUEUE_FULL (4 commands queued, or one for that
 * device already).
 *
 * The FF commands / force types are those of the Logitech classic protocol
 * (the GT Force wheel): FF_CMD_* in the low nibble of byte 0 (the high
 * nibble is the mask of the four force slots) and FF_TYPE_* in byte 1 of a
 * download; the byte layouts of the builders match the protocol's force
 * parameters.  Nothing in GT3 calls the commands other than download and
 * play: the names of 0xA, 0xC, 0xD and 0xF are the protocol's (?), 0xE takes
 * the three bytes of Set Default Spring.
 */
#include "engine/system/system_drivers_internal.h"

/* .rodata 0x00345AB8..0x00345AE8: the two locks' parameters (C `const' with
 * external linkage: written at the definition). */
const struct SemaParam g_pjoy_sema_param = { 0, 1, 1, 0, 0, 0 };    /* 0x00345AB8 max 1, init 1 */
const struct SemaParam g_usbcls_sema_param = { 0, 1, 1, 0, 0, 0 };  /* 0x00345AD0 max 1, init 1 */

/* .bss 0x004A9840..0x004A9EC0 and .sbss 0x003532DC: file statics (in link
 * order after the USB manager's .bss and u122's .sbss words, before u125's;
 * not commons).  The RPC buffers are 64-aligned (0x004A9BC0: 0x30 bytes
 * after the client's end). */
static Pjoy g_pjoy;                         /* 0x004A9840 */
static u8 g_pjoy_send_buf[0x40] __attribute__((aligned(64)));  /* 0x004A9BC0 */
static u8 g_pjoy_recv_buf[0x80] __attribute__((aligned(64)));  /* 0x004A9C00 */
static UsbClass g_usb_classes[USBD_NUM_CLASSES];               /* 0x004A9C80 */
static int g_pjoy_flush_forward;            /* 0x003532DC gp-0x4714 FF queue order, toggled every flush */

// FUNC 0x00265D80 pjoy_init
/* Once: send / receive buffers, server id 'PJOY', class 2, a lock (max 1,
 * init 1), bind.  On a bind failure it "frees" the two static buffers and
 * the static client itself (a leftover of a malloc'd version) and returns
 * NULL; otherwise installs the connect / poll callbacks and returns the
 * client. */
Pjoy *pjoy_init(void)
{
    Pjoy *pj = &g_pjoy;

    /* An early return (retail sets the result before the last store). */
    if (pj->inited != 0)
        return pj;
    pj->send = (PjoyMsg *)g_pjoy_send_buf;
    pj->recv = g_pjoy_recv_buf;
    pj->server_id = PJOY_RPC_ID;
    pj->cls = USBD_CLASS_PJOY;
    pj->sema = CreateSema((struct SemaParam *)&g_pjoy_sema_param);
    if (pjoy_bind(pj) != 0) {
        free(pj->send);
        free(pj->recv);
        free(pj);
        return NULL;
    }
    pj->connect = (UsbConnectFn)pjoy_connect;
    pj->poll = (UsbPollFn)pjoy_poll;
    pj->inited = 1;
    return pj;
}

// FUNC 0x00265E40 pjoy_set_port_map
void pjoy_set_port_map(UsbClass *cls)
{
    g_pjoy.port_map = cls;
}

// FUNC 0x00265E50 pjoy_get_event_count
/* Events seen for the device on logical `port`, -1 if none is mapped. */
int pjoy_get_event_count(int port)
{
    Pjoy *pj = &g_pjoy;
    u32 dev = usbcls_lookup(pj->port_map, port);

    if (dev >= PJOY_NUM_DEVICES)
        return -1;
    return pj->event_count[dev];
}

// FUNC 0x00265EA0 pjoy_read_state
/* Copies the last input state (0x38 bytes) of the device on `port`. */
int pjoy_read_state(int port, PjoyState *out)
{
    Pjoy *pj = &g_pjoy;
    u32 dev = usbcls_lookup(pj->port_map, port);
    int result;

    if (dev >= PJOY_NUM_DEVICES)
        return PJOY_NO_DEVICE;
    result = PJOY_OK;
    if (PollSema(pj->sema) == pj->sema) {
        if (pj->attached[dev] != 1)
            result = PJOY_NOT_ATTACHED;
        else
            *out = pj->state[dev];
        SignalSema(pj->sema);
    } else
        result = PJOY_BUSY;
    return result;
}

// FUNC 0x00265FC8 pjoy_send_command
/* Queues an 8-byte FF command for the device on `port` (sent by the next
 * pjoy_poll); at most 4 queued, one per device. */
int pjoy_send_command(int port, const FfCmd *cmd)
{
    Pjoy *pj = &g_pjoy;
    int result = PJOY_OK;
    u32 dev = usbcls_lookup(pj->port_map, port);

    if (dev >= PJOY_NUM_DEVICES)
        return PJOY_NO_DEVICE;
    if (PollSema(pj->sema) != pj->sema)
        return PJOY_BUSY;
    if (pj->attached[dev] != 1) {
        result = PJOY_NOT_ATTACHED;
    } else {
        int n = pj->ncmds;

        if (n >= 4) {
            result = PJOY_QUEUE_FULL;
        } else {
            int i;
            PjoyCmd *c;

            for (i = 0; i < n; i++) {
                if (pj->cmds[i].port == (int)dev) {
                    result = PJOY_QUEUE_FULL;
                    goto done;
                }
            }
            c = &pj->cmds[n];       /* pj + (n * 12 + 0x44), as retail */
            pj->ncmds = n + 1;
            c->port = dev;
            c->cmd = *cmd;
        }
    }
done:
    SignalSema(pj->sema);
    return result;
}

// FUNC 0x002660E0 pjoy_get_info
/* Copies the device info (0x70 bytes) of the device on `port`; waits for
 * the lock and does not check the attach state. */
int pjoy_get_info(int port, PjoyInfo *out)
{
    Pjoy *pj = &g_pjoy;
    u32 dev = usbcls_lookup(pj->port_map, port);

    if (dev >= PJOY_NUM_DEVICES)
        return PJOY_NO_DEVICE;
    WaitSema(pj->sema);
    *out = pj->info[dev];
    SignalSema(pj->sema);
    return PJOY_OK;
}

// FUNC 0x00266220 pjoy_connect
/* Connect callback: counts the event for data->id (0 when called with
 * NULL at registration).  Detach / error: mark that device detached.
 * Otherwise: query all attach states (PJOY_FNO_ATTACH; its status is not
 * looked at) and the info (PJOY_FNO_INFO) of every attached device; the
 * info is kept only when the query's status is 0. */
void pjoy_connect(Pjoy *pj, int type, UsbEventData *data)
{
    int dev = 0;
    int status;                     /* sp+0x00 */
    u8 *reply;
    int i;

    if (data != NULL)
        dev = data->id;
    WaitSema(pj->sema);
    pj->event_count[dev]++;
    if ((u32)(type - USBD_EV_DETACH) < 2) {     /* detach, error */
        pj->attached[dev] = 0;
        SignalSema(pj->sema);
        return;
    }
    reply = pjoy_rpc_query(pj, PJOY_ALL_DEVICES, &status, PJOY_FNO_ATTACH);
    *(PjoyAttach *)pj->attached = *(PjoyAttach *)reply;
    for (i = 0; i < PJOY_NUM_DEVICES; i++) {
        if (pj->attached[i] == 1) {
            reply = pjoy_rpc_query(pj, i, &pj->status[i], PJOY_FNO_INFO);
            if (pj->status[i] == 0)
                pj->info[i] = *(PjoyInfo *)reply;
        }
    }
    SignalSema(pj->sema);
}

// FUNC 0x00266430 pjoy_flush_command
/* Sends queued command i (PJOY_FNO_FF) and frees its slot (port = -1).
 * Returns PJOY_NO_DEVICE for a free slot.  Quirk: if the device is no
 * longer attached the slot is freed and the uninitialised status local is
 * returned (pjoy_poll ignores the result). */
int pjoy_flush_command(Pjoy *pj, FfCmd *buf, int i)
{
    PjoyCmd *queued = &pj->cmds[i];
    int status;                     /* sp+0x00 */

    if (queued->port == -1)
        return PJOY_NO_DEVICE;
    if (pj->attached[queued->port] != 1) {
        queued->port = -1;
        return status;              /* uninitialised, as in the binary */
    }
    *buf = queued->cmd;
    pjoy_rpc_command(pj, queued->port, buf, &status);
    queued->port = -1;
    return status;
}

// FUNC 0x002664C0 pjoy_poll
/* Poll callback: for each attached device (lock released around the RPC)
 * read the input state (PJOY_FNO_STATE); kept when the status is 0.  Then
 * send the queued FF commands, alternating forward / backward order on
 * every pass (g_pjoy_flush_forward toggles), and empty the queue. */
void pjoy_poll(Pjoy *pj)
{
    int dev, j;
    int status;                     /* sp+0x00 */

    for (dev = 0; dev < PJOY_NUM_DEVICES; dev++) {
        WaitSema(pj->sema);
        if (pj->attached[dev] == 1) {
            u8 *reply;

            SignalSema(pj->sema);
            reply = pjoy_rpc_query(pj, dev, &status, PJOY_FNO_STATE);
            WaitSema(pj->sema);
            pj->status[dev] = status;
            if (status == 0)
                pj->state[dev] = *(PjoyState *)reply;
        }
        SignalSema(pj->sema);
    }

    WaitSema(pj->sema);
    if (pj->ncmds > 0) {
        FfCmd *buf = &pj->send->cmd;

        g_pjoy_flush_forward ^= 1;
        if (g_pjoy_flush_forward != 0) {
            for (j = 0; j < pj->ncmds; j++)
                pjoy_flush_command(pj, buf, j);
        } else {
            for (j = pj->ncmds - 1; j >= 0; j--)
                pjoy_flush_command(pj, buf, j);
        }
        pj->ncmds = 0;
    }
    SignalSema(pj->sema);
}

// FUNC 0x00266698 pjoy_rpc_query
/* Synchronous call `fno` for `port` (send 0x20 bytes, receive 0x80): the
 * status word of the reply goes to *status and the payload (recv + 8) is
 * returned.  If the previous call is still in flight: *status = PJOY_BUSY
 * and the old payload is returned. */
u8 *pjoy_rpc_query(Pjoy *pj, int port, int *status, int fno)
{
    PjoyMsg *msg = pj->send;

    if (sceSifCheckStatRpc(&pj->cd.rpcd) == 1) {
        *status = PJOY_BUSY;
        return pj->recv + 8;
    }
    msg->port = port;
    msg->fno = fno;
    sif_call_rpc(&pj->cd, fno, 0, msg, PJOY_SEND_SIZE, pj->recv, PJOY_RECV_SIZE, NULL, NULL);
    *status = *(int *)pj->recv;
    return pj->recv + 8;
}

// FUNC 0x00266750 pjoy_rpc_command
/* Sends one FF command (PJOY_FNO_FF, no reply).  *status = PJOY_BUSY if
 * the previous call is still in flight, else PJOY_OK. */
void pjoy_rpc_command(Pjoy *pj, int port, const FfCmd *cmd, int *status)
{
    PjoyMsg *msg = pj->send;

    if (sceSifCheckStatRpc(&pj->cd.rpcd) == 1) {
        *status = PJOY_BUSY;
        return;
    }
    msg->port = port;
    msg->fno = PJOY_FNO_FF;
    msg->cmd = *cmd;
    sif_call_rpc(&pj->cd, msg->fno, 0, msg, PJOY_SEND_SIZE, NULL, 0, NULL, NULL);
    *status = PJOY_OK;
}

// FUNC 0x00266810 ff_send
/* A built command as it is (command 0: download). */
int ff_send(int port, FfCmd *cmd)
{
    return pjoy_send_command(port, cmd);
}

// FUNC 0x00266830 ff_send_download_play
/* Download and play a built force (USBSteeringPort::autoCentre / update). */
int ff_send_download_play(int port, FfCmd *cmd)
{
    cmd->b[0] |= FF_CMD_DOWNLOAD_PLAY;
    return pjoy_send_command(port, cmd);
}

// FUNC 0x00266858 ff_send_refresh
/* Command 0xC into a built force (? Refresh Force: new parameters for a
 * downloaded force). */
int ff_send_refresh(int port, FfCmd *cmd)
{
    cmd->b[0] |= FF_CMD_REFRESH;
    return pjoy_send_command(port, cmd);
}

// FUNC 0x00266880 ff_set_default_spring
/* Set Default Spring: K1, K2, clip in bytes 2..4.  Bytes 1, 5..7 are stack
 * garbage. */
int ff_set_default_spring(int port, int slots, int k1, int k2, int clip)
{
    FfCmd c;                        /* sp+0x00 */

    c.b[0] = slots | FF_CMD_SET_DEFAULT_SPRING;
    c.b[2] = k1;
    c.b[3] = k2;
    c.b[4] = clip;
    return pjoy_send_command(port, &c);
}

// FUNC 0x002668B0 ff_play_stop
/* Play (play == 1) or stop the forces in `slots` (high nibble).  Only
 * byte 0 is set. */
int ff_play_stop(int port, int slots, int play)
{
    FfCmd c;                        /* sp+0x00 */

    c.b[0] = (play == 1) ? (slots | FF_CMD_PLAY) : (slots | FF_CMD_STOP);
    return pjoy_send_command(port, &c);
}

// FUNC 0x002668E8 ff_default_spring
/* Default Spring On (on == 1) / Off. */
int ff_default_spring(int port, int slots, int on)
{
    FfCmd c;                        /* sp+0x00 */

    c.b[0] = (on == 1) ? (slots | FF_CMD_DEFAULT_SPRING_ON)
                       : (slots | FF_CMD_DEFAULT_SPRING_OFF);
    return pjoy_send_command(port, &c);
}

// FUNC 0x00266920 ff_set_dead_band
/* Command 0xF, parameter in byte 1 (? Set Dead Band: 0 off, 1 on). */
int ff_set_dead_band(int port, int on)
{
    FfCmd c;                        /* sp+0x00 */

    c.b[1] = on;
    c.b[0] = FF_CMD_SET_DEAD_BAND;
    return pjoy_send_command(port, &c);
}

// FUNC 0x00266948 ff_fixed_time_loop
/* Command 0xD, parameter in byte 1 (? Fixed Time Loop: 0 off, 1 on). */
int ff_fixed_time_loop(int port, int on)
{
    FfCmd c;                        /* sp+0x00 */

    c.b[1] = on;
    c.b[0] = FF_CMD_FIXED_TIME_LOOP;
    return pjoy_send_command(port, &c);
}

// FUNC 0x00266970 ff_set_led
/* Command 0xA, parameter in byte 1 (? Set LED: the LED pattern). */
int ff_set_led(int port, int leds)
{
    FfCmd c;                        /* sp+0x00 */

    c.b[1] = leds;
    c.b[0] = FF_CMD_SET_LED;
    return pjoy_send_command(port, &c);
}

// FUNC 0x00266998 ff_build_constant
/* Constant force in all four slots (command 0, download): levels f0..f3 in
 * bytes 2..5.  Returns c. */
FfCmd *ff_build_constant(FfCmd *c, int f0, int f1, int f2, int f3)
{
    c->b[2] = f0;
    c->b[3] = f1;
    c->b[4] = f2;
    c->b[5] = f3;
    c->b[0] = FF_ALL_SLOTS | FF_CMD_DOWNLOAD;
    c->b[1] = FF_TYPE_CONSTANT;
    return c;
}

// FUNC 0x002669C0 ff_build_spring
/* Spring: D1, D2, K2 << 4 | K1, S2 << 4 | S1, clip (9th argument, on the
 * stack). */
FfCmd *ff_build_spring(FfCmd *c, int slots, int d1, int d2, int k1, int k2,
                       int s1, int s2, int clip)
{
    c->b[1] = FF_TYPE_SPRING;
    c->b[0] = slots;
    c->b[2] = d1;
    c->b[3] = d2;
    c->b[4] = (k2 << 4) | k1;
    c->b[5] = (s2 << 4) | s1;
    c->b[6] = clip;
    return c;
}

// FUNC 0x00266A00 ff_build_damper
/* Damper: K1, S1, K2, S2 in bytes 2..5 (argument order k1, k2, s1, s2). */
FfCmd *ff_build_damper(FfCmd *c, int slots, int k1, int k2, int s1, int s2)
{
    c->b[0] = slots;
    c->b[1] = FF_TYPE_DAMPER;
    c->b[2] = k1;
    c->b[3] = s1;
    c->b[4] = k2;
    c->b[5] = s2;
    return c;
}

// FUNC 0x00266A28 ff_build_autocenter
/* Auto-centering spring: bytes 2..4. */
FfCmd *ff_build_autocenter(FfCmd *c, int slots, int p2, int p3, int p4)
{
    c->b[0] = slots;
    c->b[1] = FF_TYPE_AUTO_CENTER;
    c->b[2] = p2;
    c->b[3] = p3;
    c->b[4] = p4;
    return c;
}

// FUNC 0x00266A48 ff_build_sawtooth_up
/* Bytes 2..4 = p3, p4, p2, byte 5 = 0, byte 6 = p5 << 4 | p6. */
FfCmd *ff_build_sawtooth_up(FfCmd *c, int slots, int p2, int p3, int p4, int p5, int p6)
{
    c->b[0] = slots;
    c->b[1] = FF_TYPE_SAWTOOTH_UP;
    c->b[2] = p3;
    c->b[3] = p4;
    c->b[4] = p2;
    c->b[6] = (p5 << 4) | p6;
    c->b[5] = 0;
    return c;
}

// FUNC 0x00266A78 ff_build_sawtooth_down
/* The same layout as sawtooth up. */
FfCmd *ff_build_sawtooth_down(FfCmd *c, int slots, int p2, int p3, int p4, int p5, int p6)
{
    c->b[0] = slots;
    c->b[1] = FF_TYPE_SAWTOOTH_DOWN;
    c->b[2] = p3;
    c->b[3] = p4;
    c->b[4] = p2;
    c->b[6] = (p5 << 4) | p6;
    c->b[5] = 0;
    return c;
}

// FUNC 0x00266AA8 ff_build_trapezoid
/* Bytes 2..5 = p2..p5, byte 6 = p6 << 4 | p7. */
FfCmd *ff_build_trapezoid(FfCmd *c, int slots, int p2, int p3, int p4, int p5, int p6, int p7)
{
    c->b[0] = slots;
    c->b[1] = FF_TYPE_TRAPEZOID;
    c->b[2] = p2;
    c->b[3] = p3;
    c->b[4] = p4;
    c->b[5] = p5;
    c->b[6] = (p6 << 4) | p7;
    return c;
}

// FUNC 0x00266AD8 ff_build_rectangle
/* Bytes 2..6 = p2..p6. */
FfCmd *ff_build_rectangle(FfCmd *c, int slots, int p2, int p3, int p4, int p5, int p6)
{
    c->b[0] = slots;
    c->b[1] = FF_TYPE_RECTANGLE;
    c->b[2] = p2;
    c->b[3] = p3;
    c->b[4] = p4;
    c->b[5] = p5;
    c->b[6] = p6;
    return c;
}

// FUNC 0x00266B00 ff_build_variable
/* Bytes 2, 3 = p2, p3, byte 4 = p4 << 4 | p6, byte 5 = p5 << 4 | p7, byte 6
 * = p9 << 4 | p8 (p8, p9 on the stack). */
FfCmd *ff_build_variable(FfCmd *c, int slots, int p2, int p3, int p4, int p5, int p6,
                         int p7, int p8, int p9)
{
    c->b[0] = slots;
    c->b[1] = FF_TYPE_VARIABLE;
    c->b[2] = p2;
    c->b[3] = p3;
    c->b[4] = (p4 << 4) | p6;
    c->b[5] = (p5 << 4) | p7;
    c->b[6] = (p9 << 4) | p8;
    return c;
}

// FUNC 0x00266B48 ff_build_ramp
/* Bytes 2, 3 = p2, p3, byte 4 = p6, byte 5 = p4 << 4 | p5. */
FfCmd *ff_build_ramp(FfCmd *c, int slots, int p2, int p3, int p4, int p5, int p6)
{
    c->b[0] = slots;
    c->b[1] = FF_TYPE_RAMP;
    c->b[2] = p2;
    c->b[3] = p3;
    c->b[4] = p6;
    c->b[5] = (p4 << 4) | p5;
    return c;
}

// FUNC 0x00266B78 usbcls_event
/* Class handler (arg = the class): attach inserts a device sorted by key
 * (ascending; equal keys go after), detach / error unlinks and frees the
 * device with that id; 0 and >= 4 are ignored.  Then the port map is
 * rebuilt.  Returns 0.  The device slot from usbcls_dev_alloc is not
 * checked for NULL. */
int usbcls_event(UsbClass *cls, u32 type, UsbEventData *data)
{
    UsbDevList *head = &cls->list;
    UsbDev *n;
    int id = data->id;
    u64 key = data->key;

    WaitSema(cls->sema);
    switch (type) {             /* unsigned: retail's `beqz; sltiu 4' range test */
    case USBD_EV_ATTACH: {
        UsbDev *dev = usbcls_dev_alloc(cls);

        dev->data = *data;
        if ((n = head->first) == NULL) {
            TQ_INSERT_HEAD(head->first, head->last, dev);
        } else {
            for (; n != NULL; n = n->next) {
                if (key < n->data.key) {
                    TQ_INSERT_BEFORE(n, dev);
                    goto done;
                }
            }
            TQ_INSERT_TAIL(head->last, dev);
        }
        break;
    }
    case USBD_EV_DETACH:
    case USBD_EV_ERROR:
        for (n = head->first; n != NULL; n = n->next) {
            if (n->data.id == id) {
                TQ_REMOVE(head->last, n);
                usbcls_dev_free(cls, n);
                break;
            }
        }
        break;
    }
done:
    usbcls_rebuild_map(cls);
    SignalSema(cls->sema);
    return 0;
}

// FUNC 0x00266CF0 usbcls_slot
/* Logical port used for side n in mode 1: n itself. */
int usbcls_slot(int *map, int n)
{
    return n;
}

// FUNC 0x00266CF8 usbcls_rebuild_map
/* All 4 ports unmapped, then USBCLS_MAP_BY_ID: port = device id;
 * USBCLS_MAP_BY_SIDE: a device whose key bits 56-57 are 2 goes to port 0,
 * 1 to port 1 (later devices win); other modes: none.  All 4 changed flags
 * are set.  The list is walked once first with no effect (an unused
 * count). */
void usbcls_rebuild_map(UsbClass *cls)
{
    /* The map, the first device and the mode read up front (retail loads
     * mode before the -1 stores). */
    int *map = cls->map;
    UsbDev *first = cls->list.first;
    int mode = cls->mode;
    UsbDev *n;
    int i;

    for (i = 0; i < 4; i++)
        map[i] = -1;
    for (n = first; n != NULL; n = n->next)
        ;
    switch (mode) {
    case USBCLS_MAP_BY_ID:
        /* `i' reused for the id: it makes the counter one pseudo across the
         * whole function, so the constants get $v0 in the loops, as retail. */
        for (n = first; n != NULL; n = n->next) {
            i = n->data.id;
            map[i] = i;
        }
        break;
    case USBCLS_MAP_BY_SIDE:
        for (n = first; n != NULL; n = n->next) {
            u64 side = n->data.key & USBCLS_SIDE_MASK;

            if (side == USBCLS_SIDE_0)
                map[usbcls_slot(map, 0)] = n->data.id;
            if (side == USBCLS_SIDE_1)
                map[usbcls_slot(map, 1)] = n->data.id;
        }
        break;
    }
    for (i = 0; i < 4; i++)
        cls->changed[i] = 1;
}

// FUNC 0x00266E98 usbcls_get
/* Class idx (< 3), built on first use: for class 2 the PJOY client is
 * initialised, given this mapper and registered with usbcls_event as its
 * handler (not checked for a failed pjoy_init).  Empty device list, a lock
 * (max 1, init 1). */
UsbClass *usbcls_get(int idx)
{
    UsbClass *cls;

    if ((u32)idx >= USBD_NUM_CLASSES)
        return NULL;
    cls = &g_usb_classes[idx];
    if (cls->inited != 0)
        return cls;
    if (idx == USBD_CLASS_PJOY) {
        cls->drv = (UsbDriver *)pjoy_init();
        pjoy_set_port_map(cls);
        cls->reg = usbd_register_driver(cls->drv, (UsbClassFn)usbcls_event, cls);
    }
    UQ_INIT(cls->list.first, cls->list.last);
    cls->inited = 1;
    cls->sema = CreateSema((struct SemaParam *)&g_usbcls_sema_param);
    return cls;
}

// FUNC 0x00266F50 usbcls_close
/* Unregisters (with the handler as handle, see usbd_unregister_driver),
 * deletes the lock and clears the class (memset tail call).  No range
 * check of idx. */
void usbcls_close(int idx)
{
    UsbClass *cls = &g_usb_classes[idx];

    if (cls->inited == 0)
        return;
    if (cls->reg != NULL)
        usbd_unregister_driver((UsbDriver *)cls->reg);
    DeleteSema(cls->sema);
    memset(cls, 0, sizeof(UsbClass));
}

// FUNC 0x00266FC0 usbcls_set_mode
void usbcls_set_mode(UsbClass *cls, int mode)
{
    WaitSema(cls->sema);
    cls->mode = mode;
    usbcls_rebuild_map(cls);
    SignalSema(cls->sema);                      /* tail call */
}

// FUNC 0x00267008 usbcls_lookup
/* Device id on logical `port` (-1 = none); clears the port's changed flag.
 * No range check of port. */
int usbcls_lookup(UsbClass *cls, int port)
{
    int dev;

    WaitSema(cls->sema);
    if (cls->changed[port] != 0)
        cls->changed[port] = 0;
    dev = cls->map[port];
    SignalSema(cls->sema);
    return dev;
}

// FUNC 0x00267068 usbcls_dev_alloc
UsbDev *usbcls_dev_alloc(UsbClass *cls)
{
    UsbDev *dev = cls->devs;
    int i;

    for (i = 0; i < 4; i++, dev++) {
        if (dev->in_use == 0) {
            dev->in_use = 1;
            return dev;
        }
    }
    return NULL;
}

// FUNC 0x002670A8 usbcls_dev_free
void usbcls_dev_free(UsbClass *cls, UsbDev *dev)
{
    int i;

    for (i = 0; i < 4; i++) {
        if (&cls->devs[i] == dev) {
            memset(&cls->devs[i], 0, sizeof(UsbDev));
            return;
        }
    }
}
