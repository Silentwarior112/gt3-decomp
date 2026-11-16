/* T477 - PSPort and the pad thread.  0x0022F2A8..0x0022FFE8.
 *
 * The pad thread (priority 4, stack 0x0049F9C0, 0x1000 bytes) is woken once
 * a frame by PadManager::update.  For pads 0 and 4 (port 1 / port 2, slot
 * 0) it reads the pad and, while the pad is FindCTP1 / Stable, runs three
 * small state machines in turn: the main mode (switch to the DualShock
 * type 7, locked), the pressure mode (enter it unless the pad already
 * reports 0x79), and the actuators (align them once, then send the motor
 * values whenever they change).  A disconnected / error pad starts over.
 * The main thread reads its own copy of the pad data (padReadAll) and the
 * PSPorts translate it: the mode id gives the PS type (4 digital, 7
 * DualShock, 8 DualShock 2), the button bits give GT3's buttons, the data
 * bytes from [4] on are the analog channels. */
#include "engine/input/control_internal.h"
#include "engine/input/T477_ps_port.h"
#include "engine/input/T472_analog_curve.h"         /* g_003416F8, the analog stick curve (retail's
                                               T477 did not see the logger windows) */

/* .bss (file statics, in this order): the scePadPortOpen DMA areas, one per
 * pad index (256 bytes each, up to the stack): only pads 0 and 4 are opened
 * (padStart indexes the array with the pad index: `sll 8'); libpad wants
 * them 64-byte aligned (retail's address is). */
static u128 g_0049F1C0[PAD_COUNT][scePadDmaBufferMax] __attribute__((aligned(64)));  /* 0x0049F1C0 */
static u8 g_0049F9C0[PAD_STACK_SIZE];       /* 0x0049F9C0 the pad thread's stack */
static PadThread g_004A09C0;                /* 0x004A09C0 the pad thread's state */

/* The PS types by mode id: {0x41, 4}, {0x73, 7}, {0x79, 8}, {0, 0}.
 * Internal linkage: egcs defers it to the end of .rodata, after the vtable,
 * as retail. */
static const PadTypeEntry g_00341A38[4] = { /* 0x00341A38 */
    { PADID_DIGITAL,     { 0, 0, 0 }, PS_PAD_DIGITAL },
    { PADID_DUALSHOCK,   { 0, 0, 0 }, PS_PAD_DUALSHOCK },
    { PADID_DUALSHOCK2,  { 0, 0, 0 }, PS_PAD_DUALSHOCK2 },
    { 0,                 { 0, 0, 0 }, PS_PAD_NONE }
};

/* (? name) Restarts the state machines of a pad (interrupts off): no data,
 * not stable, main mode to set, pressure mode to check, actuators to align;
 * the last sent motors 0xFF so the next ones differ.  The stores in an
 * order that gives retail's schedule. */
// FUNC 0x0022F2A8 padResetState
void padResetState(PadState *pad)
{
    int intrWasOn = intr_suspend();
    pad->pressState = PAD_PRESS_CHECK;
    pad->modeState  = PAD_MAINMODE_SET;
    pad->actState   = PAD_ACT_ALIGN;
    pad->rdata.b[0] = 0xFF;
    pad->stable     = 0;
    pad->rdata.b[1] = 0;
    memset(&pad->actSent, -1, sizeof(PadActDirect));
    memset(&pad->actDirect, 0, sizeof(PadActDirect));
    if (intrWasOn)
        EI();
}

/* (? name) The main mode's SET state: look the DualShock mode (type 7) up in
 * the pad's mode table and set it, locked (the state machine then waits for
 * the request), or give up (done) if the pad has none or is in it already.
 * 0: this pad is done for the frame, 1: go on.  Inline (its parameters'
 * copies show in retail: the port / slot of the first three
 * scePadInfoMode calls are other registers than the loop's). */
static inline int padSetMainMode(PadState *pad, int port, int slot)
{
    int modeCount = scePadInfoMode(port, slot, InfoModeIdTable, -1);
    int curType   = scePadInfoMode(port, slot, InfoModeCurID, 0);
    int curExType = scePadInfoMode(port, slot, InfoModeCurExID, 0);
    if (curExType != 0)
        curType = curExType;
    for (int offs = 0; offs < modeCount; offs++) {
        int modeType = scePadInfoMode(port, slot, InfoModeIdTable, offs);
        if (modeType == PS_PAD_DUALSHOCK && curType != modeType) {
            if (scePadSetMainMode(port, slot, offs, PAD_MMODE_LOCK) == 1) {
                pad->modeState  = PAD_MAINMODE_WAIT;
                pad->pressState = PAD_PRESS_CHECK;
            }
            return 0;
        }
    }
    pad->modeState = PAD_MAINMODE_DONE;
    return 1;
}

/* The pad thread (thread_start_retry(padThreadMain, &g_004A09C0, 4, stack,
 * 0x1000)).  Runs until padStop puts a semaphore in quitSema; then signals
 * it and exits.  The states in switches (retail's compare trees and case
 * order: the main mode SET before WAIT, the pressure mode CHECK, ENTER,
 * WAIT, the actuators ALIGN before SEND); a `default' that leaves a machine
 * by goto keeps its loop unrotated, as retail.  78.5%: retail keeps two
 * more copies of port / slot (spilled, and fp) and the constant 3 in $v1
 * across the calls (sq / lq); not reached (see ref/matching/u105/notes.md). */
// FUNC 0x0022F328 padThreadMain
void padThreadMain(void *arg)
{
    /* implicit: scePadInfoMode, scePadSetMainMode (padSetMainMode, inline) */
    PadThread *thread = (PadThread *)arg;

    for (;;) {
        WaitSema(thread->sema);
        if (thread->quitSema >= 0)
            break;
        for (int i = 0; i < PAD_COUNT; i += PAD_STEP) {
            int port = i >> 2;
            int slot = i & 3;
            PadState *pad = &thread->pads[i];
            if (scePadRead(port, slot, pad->rdata.b) == 0) {
                pad->rdata.b[1] = 0;
                pad->rdata.b[0] = 0xFF;
            }
            switch (scePadGetState(port, slot)) {
            case scePadStateDiscon:
            case scePadStateError:
                padResetState(pad);
                continue;
            case scePadStateFindCTP1:
            case scePadStateStable:
                break;
            default:
                continue;
            }
            int again;

            /* 1. the main mode: DualShock (type 7), locked */
            for (;;) {
                switch (pad->modeState) {
                case PAD_MAINMODE_SET:
                    again = padSetMainMode(pad, port, slot);
                    break;
                case PAD_MAINMODE_WAIT: {
                    int req = scePadGetReqState(port, slot);
                    if (req == scePadReqStateComplete) {
                        pad->modeState = PAD_MAINMODE_DONE;
                        again = 1;
                    } else if (req == scePadReqStateFaild) {
                        pad->modeState = PAD_MAINMODE_SET;
                        again = 1;
                    } else {
                        again = 0;
                    }
                    break;
                }
                default:
                    goto mode_done;
                }
                if (again == 0)
                    goto next_pad;
            }
        mode_done:;

            /* 2. the pressure mode */
            for (;;) {
                switch (pad->pressState) {
                case PAD_PRESS_CHECK: {
                    int inPressMode = 0;
                    if (pad->rdata.b[0] == 0) {
                        if (pad->rdata.b[1] == PADID_DUALSHOCK2)
                            inPressMode = 1;
                        else
                            inPressMode = 0;
                    }
                    if (inPressMode) {
                        pad->pressState = PAD_PRESS_DONE;
                        again = 1;
                    } else if (scePadInfoPressMode(port, slot) != 1) {
                        pad->pressState = PAD_PRESS_DONE;
                        again = 1;
                    } else {
                        pad->pressState = PAD_PRESS_ENTER;
                        again = 0;
                    }
                    break;
                }
                case PAD_PRESS_ENTER:
                    if (scePadEnterPressMode(port, slot) == 1)
                        pad->pressState = PAD_PRESS_WAIT;
                    goto next_pad;
                case PAD_PRESS_WAIT: {
                    int req = scePadGetReqState(port, slot);
                    if (req == scePadReqStateComplete) {
                        pad->pressState = PAD_PRESS_DONE;
                        again = 1;
                    } else if (req == scePadReqStateFaild) {
                        pad->pressState = PAD_PRESS_ENTER;
                        again = 1;
                    } else {
                        again = 0;
                    }
                    break;
                }
                default:
                    goto press_done;
                }
                if (again == 0)
                    goto next_pad;
            }
        press_done:;

            /* 3. the actuators */
            switch (pad->actState) {
            case PAD_ACT_ALIGN:
                pad->actCount = scePadInfoAct(port, slot, -1, 0);
                if (pad->actCount == 0) {
                    pad->actState = PAD_ACT_NONE;
                } else {
                    for (int k = 0; k < 6; k++)
                        pad->actAlign[k] = (k < pad->actCount) ? k : 0xFF;
                    if (scePadSetActAlign(port, slot, pad->actAlign) == 1)
                        pad->actState = PAD_ACT_SEND;
                }
                break;
            case PAD_ACT_SEND: {
                int intrWasOn = intr_suspend();
                memset(&pad->actDirect, 0, sizeof(PadActDirect));
                switch (pad->rdata.b[1]) {
                case PADID_DIGITAL:
                case PADID_ANALOG:
                case PADID_DUALSHOCK:
                case PADID_DUALSHOCK2: {
                    int small = pad->act[0];
                    int large = pad->act[1];
                    if (pad->actCount == 1) {
                        if (small != 0) {
                            large = 0x40;
                        } else {
                            small = (large >= 0x61);
                            large = 0x40;
                        }
                    }
                    pad->actDirect.b[0] = small;
                    pad->actDirect.b[1] = large;
                    break;
                }
                }
                if (intrWasOn)
                    EI();
                if (memcmp(&pad->actSent, &pad->actDirect, sizeof(PadActDirect)) != 0) {
                    scePadSetActDirect(port, slot, pad->actDirect.b);
                    pad->actSent = pad->actDirect;
                }
                break;
            }
            }
            {
                int state = scePadGetState(port, slot);
                if (state == scePadStateFindCTP1 || state == scePadStateStable)
                    pad->stable = 1;
            }

        next_pad:;
        }
        thread->pending = 0;
    }
    signal_sema_helper(thread->quitSema);
    ExitDeleteThread();
}

/* (? name) Called by PadManager::init: the pad states reset, libpad up,
 * ports 1 and 2 (slot 0) opened on the DMA areas of pads 0 and 4
 * (g_0049F1C0[i], 256 bytes each), the thread started. */
// FUNC 0x0022F810 padStart
void padStart(void)
{
    PadThread *thread = &g_004A09C0;
    thread->quitSema = -1;
    thread->sema     = create_mutex_sema(0);
    thread->pending  = 0;
    for (int i = 0; i < PAD_COUNT; i++)
        padResetState(&thread->pads[i]);
    scePadInit(0);
    pad_rpc_cmd_0x14(0);
    for (int i = 0; i < PAD_COUNT; i += PAD_STEP)
        scePadPortOpen(i >> 2, i & 3, g_0049F1C0[i]);
    thread_start_retry(padThreadMain, thread, PAD_THREAD_PRIO, g_0049F9C0, PAD_STACK_SIZE);
}

/* (? name) Stops the thread (it signals quitSema on its way out), deletes
 * both semaphores, scePadEnd.  The ports are not closed.  Caller:
 * PadManager::term (no caller). */
// FUNC 0x0022F8D8 padStop
void padStop(void)
{
    g_004A09C0.quitSema = create_mutex_sema(0);
    signal_sema_helper(g_004A09C0.sema);
    WaitSema(g_004A09C0.quitSema);
    DeleteSema(g_004A09C0.quitSema);
    DeleteSema(g_004A09C0.sema);
    scePadEnd();
}

/* (? name) The main thread's read of pads 0 and 4 (into data, not rdata);
 * a failed read gives "no data, no buttons". */
// FUNC 0x0022F938 padReadAll
void padReadAll(void)
{
    PadThread *thread = &g_004A09C0;
    for (int i = 0; i < PAD_COUNT; i += PAD_STEP) {
        PadState *pad = &thread->pads[i];
        if (scePadRead(i >> 2, i & 3, pad->data.b) == 0) {
            pad->data.b[0] = 0xFF;
            pad->data.b[1] = 0;
            pad->data.b[2] = 0xFF;
            pad->data.b[3] = 0xFF;
        }
    }
}

/* (? name) Wakes the pad thread unless it has not run since the last time. */
// FUNC 0x0022F9B0 padWakeThread
void padWakeThread(void)
{
    if (g_004A09C0.pending == 0) {
        g_004A09C0.pending = 1;
        signal_sema_helper(g_004A09C0.sema);
    }
}

/* (? name) Copies pad idx's data; returns its mode id once the pad has been
 * seen stable, else 0. */
// FUNC 0x0022F9E8 padGetData
u8 padGetData(int idx, PadData *out)
{
    PadState *pad = &g_004A09C0.pads[idx];
    *out = pad->data;
    u8 modeId = 0;
    if (pad->stable != 0)
        modeId = pad->data.b[1];
    return modeId;
}

/* (? name) The motor values for the pad thread (interrupts off).  memcpy:
 * retail loads the three bytes (lb) before it stores them. */
// FUNC 0x0022FA58 padSetActuators
void padSetActuators(int idx, const u8 *act)
{
    int intrWasOn = intr_suspend();
    PadState *pad = &g_004A09C0.pads[idx];
    memcpy(pad->act, act, 3);
    if (intrWasOn)
        EI();
}

/* (? name) The table entry of mode id `id', or the terminating {0, 0}. */
// FUNC 0x0022FAC0 padFindType
const PadTypeEntry *padFindType(u8 id)
{
    const PadTypeEntry *entry = g_00341A38;
    while (entry->id != 0 && id != entry->id)
        entry++;
    return entry;
}

/* Caller: PadManager::PadManager (ps[] and the IEEE1394Ports' psPort). */
// FUNC 0x0022FB00 PSPort::PSPort
PSPort::PSPort()
{
    /* implicit: PortBase (compiler-generated: bases, members, deleting form) */
    /* (egcs: PortBase::PortBase(), then the vptr; no member is set) */
}

/* Index: PSPort::virtual_0. */
// FUNC 0x0022FB38 PSPort::~PSPort
PSPort::~PSPort()
{
    /* implicit: ~PortBase (compiler-generated: bases, members, deleting form) */
    /* (egcs: the vptr, then PortBase::~PortBase with the flags passed on) */
}

/* (? name) Status 0, no pad id, actuators enabled.  PadManager::init passes
 * an int (no s8 conversion at the call). */
// FUNC 0x0022FB60 PSPort::init
void PSPort::init(int port)
{
    status     = PORT_NONE;
    this->port = port;
    actEnable  = 1;
    st.padId   = 0;
}

/* (? name) Once a frame (PadManager::update). */
// FUNC 0x0022FB78 PSPort::update
void PSPort::update()
{
    setPadId(padGetData(port, &st.data));
}

/* (? name) id 0: no pad.  Else connected; a new id looks its PS type up.
 * Then this frame's motor values go to the pad thread (if enabled) and are
 * cleared (stopActuators, virtual).  The id is an int (no andi at the
 * entry; the u8 conversion is padFindType's). */
// FUNC 0x0022FBB0 PSPort::setPadId
void PSPort::setPadId(int id)
{
    if (id == 0) {
        status   = PORT_NONE;
        st.padId = 0;
        return;
    }
    status = PORT_CONNECTED;
    if (st.padId != id) {
        st.padId  = id;
        st.psType = padFindType(id)->type;
    }
    if (actEnable != 0)
        padSetActuators(port, act);
    stopActuators();
}

/* Index: PSPort::virtual_1.  No pad or a mode id the table does not know:
 * the Control's type, buttons, mask and channels are cleared.  Analog pads
 * (types 5, 7, 8) also give the directions from the left stick (through the
 * curve g_003416F8); the neGcon type 2 gives decide / cancel from channels
 * 2 / 1.  The types in a switch on st.psType (retail's compare tree: 5,
 * then 2 below it, 7..8 above). */
// FUNC 0x0022FC30 PSPort::translate
void PSPort::translate(Control &c)
{
    c.status = status;
    if (status == PORT_NONE || padFindType(st.padId)->id != st.padId) {
        c.type = 0;
        c.curButtons.buttons = 0;
        c.curButtons.mask    = 0;
        for (int ch = CHAN_COUNT - 1; ch >= 0; ch--)
            c.channels[ch] = 0;
        return;
    }
    if (status != PORT_CONNECTED)
        return;
    c.setPSType(st.psType);
    /* channels[i] = data[4 + i] for 32 bytes: the last 4 come from past the
     * 0x20-byte data (actEnable, +0x34..+0x37). */
#ifdef BOOT_OLD_GCC
    for (int ch = 0; ch < CHAN_COUNT; ch++)
        c.channels[ch] = st.data.b[4 + ch];
#else
    /* the same bytes without indexing past data.b, which GCC 14 may take
     * as the loop's end (PSPort +0x18, the same offset in both layouts) */
    const u8 *src = (const u8 *)this + 0x18;
    for (int ch = 0; ch < CHAN_COUNT; ch++)
        c.channels[ch] = src[ch];
#endif
    u32 raw = ((st.data.b[2] << 8) | st.data.b[3]) ^ 0xFFFF;   /* active low */
    u32 buttons = 0;
    switch (st.psType) {
    case PS_PAD_NEGCON:
        /* channels 1 and 2 (data[5], data[6]: ? the neGcon's I and II) */
        buttons = (c.channels[1] < 0x80) ? 0 : CONTROL_CANCEL;
        if (!(c.channels[2] < 0x80))
            buttons |= CONTROL_DECIDE;
        break;
    case PS_PAD_ANALOG:
    case PS_PAD_DUALSHOCK:
    case PS_PAD_DUALSHOCK2: {
        /* the left stick as the directions */
        int x = g_003416F8[(u8)c.channels[CHAN_LSTICK_X]] - STICK_CENTRE;
        if (x >= STICK_THRESHOLD)
            buttons = CONTROL_RIGHT;
        else if (!(-STICK_THRESHOLD < x))
            buttons = CONTROL_LEFT;
        int y = g_003416F8[(u8)c.channels[CHAN_LSTICK_Y]] - STICK_CENTRE;
        if (y >= STICK_THRESHOLD)
            buttons |= CONTROL_DOWN;
        else if (!(-STICK_THRESHOLD < y))
            buttons |= CONTROL_UP;
        break;
    }
    default:
        break;
    }
    if (raw & SCE_PADLup)    buttons |= CONTROL_UP;
    if (raw & SCE_PADLdown)  buttons |= CONTROL_DOWN;
    if (raw & SCE_PADLleft)  buttons |= CONTROL_LEFT;
    if (raw & SCE_PADLright) buttons |= CONTROL_RIGHT;
    c.curButtons.raw[0] = raw;
    if (raw & SCE_PADstart)  buttons |= CONTROL_START;
    if (raw & SCE_PADselect) buttons |= CONTROL_SELECT;
    if (raw & (SCE_PADRright | SCE_PADRdown)) buttons |= CONTROL_DECIDE;   /* circle, cross */
    if (raw & (SCE_PADRup | SCE_PADRleft))    buttons |= CONTROL_CANCEL;   /* triangle, square */
    c.curButtons.buttons = buttons;
}

/* Index: PSPort::virtual_2. */
// FUNC 0x0022FE58 PSPort::translateSync
void PSPort::translateSync(Control &c)
{
}

/* Index: PSPort::virtual_3.  No range check. */
// FUNC 0x0022FE60 PSPort::setActuator
void PSPort::setActuator(int ch, int value)
{
    act[ch] = value;
}

/* Index: PSPort::virtual_4. */
// FUNC 0x0022FE70 PSPort::stopActuators
void PSPort::stopActuators()
{
    for (int i = 0; i < 3; i++)
        act[i] = 0;
}

/* Index: PSPort::virtual_5. */
// FUNC 0x0022FEA0 PSPort::isReady
int PSPort::isReady()
{
    return 1;
}

/* (? name) Into a link frame: pad id, type and data (0x28 bytes). */
// FUNC 0x0022FEA8 PSPort::save
u8 *PSPort::save(u8 *buf)
{
    *(PSPortState *)buf = st;
    return buf + sizeof(PSPortState);
}

/* (? name) From a link frame; setPadId then sends this port's motors to
 * pad `port' (0 for the IEEE1394Ports) when enabled. */
// FUNC 0x0022FF00 PSPort::load
const u8 *PSPort::load(const u8 *buf)
{
    st = *(const PSPortState *)buf;
    setPadId(st.padId);
    return buf + sizeof(PSPortState);
}

/* actuatorEnable / getData / getPSType are inline in engine/input/T477_ps_port.h
 * (no callers); egcs emits these copies with the vtable, the last-declared
 * first. */
// FUNC 0x0022FF80 PSPort::actuatorEnable
/* inline: actEnable = on */

// FUNC 0x0022FF88 PSPort::getData
/* inline: &st.data */

// FUNC 0x0022FF90 PSPort::getPSType
/* inline: st.psType */

#ifndef BOOT_OLD_GCC
/* __si_type_info __ti6PSPort (0x004B2A10), name "6PSPort" (0x00352CE8),
 * base PortBase.  Only in the vtable. */
// FUNC 0x0022FF98 __tf6PSPort
extern "C" void *__tf6PSPort(void)
{
    if (__ti6PSPort[0] == 0) {
        __tf8PortBase();
        __rtti_si(__ti6PSPort, "6PSPort", (const type_info *)__ti8PortBase);
    }
    return __ti6PSPort;
}
#endif

/* .sdata 0x00352CF0: 8 zero bytes nothing in the program references, after
 * "6PSPort" and before T482's table; an uninitialised global of 8 bytes in a
 * -G8 C++ file comes out there (zeros at the end of the .sdata), which file
 * and name the bytes cannot tell (ref/data/u105/notes.md).  Defined here so
 * that the image's bytes come from the C. */
int g_00352CF0[2];                          /* 0x00352CF0 unreferenced */
