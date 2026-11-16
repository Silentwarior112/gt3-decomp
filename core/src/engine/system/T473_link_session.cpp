/* T473 - the i.LINK (IEEE 1394) session of the link battle.
 * 0x0022CB58..0x0022DA98.
 *
 * A thread (link_thread) runs exchanges through the pdi1394 driver: every
 * node sends one 0x7C-byte frame (g_linkSend) and receives the frames of all
 * eight nodes (g_linkRecv).  Two semaphores hand the frames over to the game
 * (F_0022D0F0: g_linkFrameOut out, g_linkFramesIn in).  Byte 0x7B of a frame
 * carries a command: LINK_CMD_END (0x80) or LINK_CMD_PAUSE (0x81); one
 * received switches the session to a command exchange (mode 3), which waits
 * until the game took its frames (F_0022CCD8).  A bus reset (the driver's
 * callback) stops the session with LINK_CMD_END.
 *
 * The includes give retail's type names (.rodata 0x00341810..0x00341908):
 * egcs writes them in reverse definition order, so the original saw the
 * GameStatus's garage lists first, then pdi1394, whose destructor was
 * inline there (its name without a vtable: T541_pdi1394.h, read after
 * gtsound_internal.h; of u104's files only this one sees it, as retail). */
#include "game/save/T429_game_status.h"          /* getGameStatus, MenuStatus::options[8]; the garage lists */
#include "game/sound/gtsound_internal.h"
#include "engine/pdi/T541_pdi1394.h"              /* pdi1394 (g_link) */
#include "engine/system/T473_link_session.h"

/* The data, in retail's order.  .sdata: the semaphores, the Random (a
 * public object with a constructor: written at its definition), then the
 * session words (initialised: written where defined, in address order).
 * .data: the link's Control (constructor), then the frames (no initialiser:
 * zeros at the end of the output, in the order of their first declaration,
 * gtsound_internal.h's).  .rodata: the semaphores' parameters first (extern: written at
 * the definition), the type names last.  The objects with constructors are
 * built by the compiler's _GLOBAL_$I$ function (T473_static_init below), the
 * Control destroyed by its _GLOBAL_$D$. */
int      g_linkGoSema = -1;                 /* 0x00352C98 */
int      g_linkDoneSema = -1;               /* 0x00352C9C */
Random   g_linkRandom;                      /* 0x00352CA0 (seed 0x87654321) */
volatile int g_linkThreadQuit = 0;          /* 0x00352CA4 */
int      g_00352CA8 = 0;                    /* 0x00352CA8 */
int      g_linkRandomSum = 0;               /* 0x00352CAC */
int      g_00352CB0 = 0;                    /* 0x00352CB0 */
int      g_linkState = LINK_STATE_RUN;      /* 0x00352CB4 */
pdi1394 *g_link = NULL;                     /* 0x00352CB8 */
int      g_linkResetNodes = 0;              /* 0x00352CBC */
int      g_linkSeq = 0;                     /* 0x00352CC0 */
int      g_linkCmdIn = LINK_CMD_NONE;       /* 0x00352CC4 */
int      g_linkCmdOut = LINK_CMD_NONE;      /* 0x00352CC8 */
int      g_linkFramesTaken = 0;             /* 0x00352CCC */
int      g_linkCmdXchgDue = 0;              /* 0x00352CD0 */
int      g_linkFirstXchgDue = 0;            /* 0x00352CD4 */
int      g_linkEnabled = 0;                 /* 0x00352CD8 */
int      g_00352CDC = 0;                    /* 0x00352CDC */
Control  g_linkControl;                     /* 0x002E9820 the link's Control (pad slot 4) */
LinkPlayerInfo g_linkPlayers[LINK_NODES];   /* 0x002E98E8 */
LinkFrameSet   g_linkFramesIn;              /* 0x002E9910 */
LinkFrame      g_linkFrameOut;              /* 0x002E9CF0 */
/* count 0, max 1, init 0 */
const SemaParam g_linkSemaParam = { 0, 1, 0, 0, 0, 0 };    /* 0x003417F8 */
/* (64-aligned: retail's 0x0049ED20..0x0049ED40 is padding; the IOP's
 * exchange writes it) */
static LinkRecvBuffer g_linkRecv __attribute__((aligned(64)));  /* 0x0049ED40 the frames received (the thread's) */
static LinkFrame      g_linkSend;           /* 0x0049F140 the frame to send (the thread's) */
static int            g_linkLastCmd;        /* 0x00353270 the command seen last time */

/* The driver, its init, the bus reset callback. */
// FUNC 0x0022CB58 link_create
void link_create(void)
{
    /* implicit: operator new(0xC4) 0x0027C138; operator delete 0x0027C0C0 if the
     * constructor throws */
    g_link = new pdi1394(0x20002);
    g_link->init();
    pdi1394_set_callback(link_bus_reset_cb, NULL);      /* 0x0026ECB8 */
}

/* The driver's callback (called after every EE server call the driver does
 * not handle itself; `fno' is not read): a bus reset. */
// FUNC 0x0022CBD8 link_bus_reset_cb
void link_bus_reset_cb(void *arg, unsigned int fno)
{
    LinkStatus st;                          /* sp+0x00 (0x50 bytes reserved) */

    pdi1394_get_status(&st);                /* 0x0026EE38 */
    g_linkResetNodes = st.nodeCount;
    g_00352CDC = 1;
    if (g_linkGoSema == -1)
        g_linkState = LINK_STATE_RUN;       /* no session */
    else if (g_linkState == LINK_STATE_RUN) {
        if (st.unk48 != 0)
            g_linkState = LINK_STATE_RESET48;
        else
            g_linkState = LINK_STATE_RESET;
    }
}

// FUNC 0x0022CC38 link_enable
void link_enable(void)
{
    g_linkEnabled = 1;
}

// FUNC 0x0022CC48 link_disable
void link_disable(void)
{
    g_linkEnabled = 0;
}

// FUNC 0x0022CC50 F_0022CC50
int F_0022CC50(void)
{
    return g_linkEnabled;
}

// FUNC 0x0022CC58 F_0022CC58
u8 *F_0022CC58(void)
{
    return g_linkFrameOut.data;
}

// FUNC 0x0022CC68 F_0022CC68
LinkFrameSet *F_0022CC68(void)
{
    return &g_linkFramesIn;
}

/* The data slot `slot' sent (its frame is the one of its IOP node). */
// FUNC 0x0022CC78 F_0022CC78
u8 *F_0022CC78(int slot)
{
    int node = g_link->nodeId[slot];

    if (node >= 0)
        return g_linkFramesIn.node[node].data;
    return NULL;
}

// FUNC 0x0022CCB0 F_0022CCB0
int F_0022CCB0(void)
{
    LinkStatus st;

    pdi1394_get_status(&st);                /* 0x0026EE38 */
    return st.unk48 != 0;
}

/* The game took the frames of a command exchange: lets the thread go on
 * (a PAUSE received is forgotten, an END stays). */
// FUNC 0x0022CCD8 F_0022CCD8
int F_0022CCD8(void)
{
    if (g_linkFramesTaken != 0)
        return 1;
    if (g_linkCmdIn != LINK_CMD_END)
        g_linkCmdIn = LINK_CMD_NONE;
    g_linkFramesTaken = 1;
    SignalSema(g_linkGoSema);
    return 1;
}

/* 1 while a command exchange waits for the game (F_0022CCD8). */
// FUNC 0x0022CD18 F_0022CD18
int F_0022CD18(void)
{
    return g_linkFramesTaken ^ 1;
}

// FUNC 0x0022CD28 F_0022CD28
int F_0022CD28(void)
{
    return g_linkState;
}

/* Asks the others for a pause (RaceLinkBattle's pause button, u047). */
// FUNC 0x0022CD30 F_0022CD30
void F_0022CD30(void)
{
    g_linkCmdOut = LINK_CMD_PAUSE;
}

// FUNC 0x0022CD40 F_0022CD40
int F_0022CD40(void)
{
    return g_linkCmdIn == LINK_CMD_PAUSE;
}

/* Puts the pending command into the frame to send.  (Without one, it clears
 * the command byte of the game's copy, not of the frame to send.) */
// FUNC 0x0022CD50 link_put_command
void link_put_command(void)
{
    if (g_linkCmdOut != LINK_CMD_NONE) {
        u8 cmd = (u8)g_linkCmdOut;
        g_linkCmdOut = LINK_CMD_NONE;
        g_linkSend.cmd = cmd;
        return;
    }
    g_linkFrameOut.cmd = LINK_CMD_NONE;
}

/* A byte of the frames received, by its offset.  (An inline accessor:
 * retail keeps the offset node * 0x7C + 0x7B as its own induction variable
 * and adds the buffer to it at every access; written in place, loop.c
 * strength-reduces the whole address instead.) */
static inline u8 *link_recv_byte(int off)
{
    return &((u8 *)&g_linkRecv)[off];
}

/* Looks for commands in the frames received.
 * 83.2%, data references right: what is left is the allocation of the
 * loops' registers (retail: cmd a3, base v0, offset a1, count a2; here a2,
 * t0, a0, a1) and retail's copy of the byte read into cmd (daddu) where CSE
 * here substitutes the constant the test proved (li 0x81 / li 0x80).
 * Tried: cmd / c as u8 or int, *slot read twice, switch and
 * do { } while (0) forms, a `last' local, all 120 orders of the bus-reset
 * stores (and the best ones with the end command in cmd). */
// FUNC 0x0022CD78 link_take_commands
void link_take_commands(void)
{
    int cmd;
    int node;

    if (g_00352CDC != 0) {                  /* a bus reset: end */
        cmd = LINK_CMD_END;
        g_linkCmdIn = cmd;
        g_linkState = LINK_STATE_COMMAND;
        g_linkCmdXchgDue = 1;
        g_linkLastCmd = LINK_CMD_END;
        g_linkFramesTaken = 0;
        return;
    }
    if (g_linkLastCmd != LINK_CMD_NONE) {   /* drop the command seen last time */
        cmd = g_linkCmdIn;
        for (node = 0; node < LINK_NODES; node++) {
            u8 *p = link_recv_byte(node * LINK_FRAME_SIZE + 0x7B);     /* frames.node[node].cmd */

            if (*p == g_linkLastCmd)
                *p = LINK_CMD_NONE;
        }
        g_linkLastCmd = LINK_CMD_NONE;
    } else {
        cmd = g_linkCmdIn;
    }
    for (node = 0; node < LINK_NODES; node++) {
        u8 *slot = link_recv_byte(node * LINK_FRAME_SIZE + 0x7B);      /* frames.node[node].cmd */
        u8 c = *slot;

        if (cmd == LINK_CMD_END)
            continue;
        if (c == LINK_CMD_PAUSE) {
            cmd = c;
            g_linkState = LINK_STATE_COMMAND;
            g_linkCmdIn = cmd;
            g_linkCmdXchgDue = 1;
            g_linkFramesTaken = 0;
            *slot = LINK_CMD_NONE;
        }
        if (c == LINK_CMD_END) {
            *slot = LINK_CMD_NONE;
            cmd = c;
            g_linkCmdIn = cmd;
            g_linkCmdXchgDue = 1;
            g_linkFramesTaken = 0;
        }
    }
    g_linkLastCmd = cmd;
}

/* One step of the link thread. */
// FUNC 0x0022CE70 link_thread_step
void link_thread_step(void)
{
    int result;

    WaitSema(g_linkGoSema);
    if (g_linkGoSema == -1)
        return;                             /* the session ended */

    if (g_linkFirstXchgDue != 0) {          /* the first exchange */
        memset(&g_linkRecv, 0, sizeof(g_linkRecv));
        g_linkSeq = 1;
        pdi1394_exchange(&g_linkSend, &g_linkRecv, LINK_FRAME_SIZE, LINK_XCHG_FIRST, 1);  /* 0x0026E938 */
        g_linkFirstXchgDue = 0;
        g_linkSeq += 1;
        SignalSema(g_linkDoneSema);
        return;
    }
    if (g_linkCmdXchgDue != 0) {            /* a command exchange */
        if (g_linkFramesTaken == 0)
            return;
        result = pdi1394_exchange(&g_linkSend, &g_linkRecv, LINK_FRAME_SIZE, LINK_XCHG_COMMAND,
                                  g_linkSeq++);
        if (result == 0)
            g_linkSeq += 1;
        g_linkCmdXchgDue = 0;
        g_linkState = LINK_STATE_RUN;
        g_linkFramesIn = g_linkRecv.frames;
        if (result != 0)
            SignalSema(g_linkGoSema);
        else
            SignalSema(g_linkDoneSema);
        return;
    }
    if (g_linkCmdIn != LINK_CMD_NONE)
        return;

    link_put_command();                     /* a normal exchange */
    result = pdi1394_exchange(&g_linkSend, &g_linkRecv, LINK_FRAME_SIZE, LINK_XCHG_NORMAL, g_linkSeq);
    link_take_commands();
    if (g_linkCmdIn != LINK_CMD_NONE) {
        g_linkSeq += 1;
        return;
    }
    switch (result) {
    case 0:
        g_linkSeq += 1;
        SignalSema(g_linkDoneSema);
        break;
    case 1:                                 /* (retail's switch tests 1 too) */
        break;
    case 3:                               /* a bad reply: a command exchange next */
        g_linkCmdXchgDue = 1;
        g_linkFramesTaken = 0;
        break;
    }
}

/* Called by the game every frame: when an exchange is done, takes the
 * frames received, gives the frame to send and lets the thread run the next
 * exchange; returns 1 then. */
// FUNC 0x0022D0F0 F_0022D0F0
int F_0022D0F0(void)
{
    if (g_linkCmdIn != LINK_CMD_NONE)
        return 0;
    if (g_linkFirstXchgDue != 0) {
        SignalSema(g_linkGoSema);
        return 0;
    }
    if (g_linkDoneSema != -1) {
        if (PollSema(g_linkDoneSema) == g_linkDoneSema) {
            g_linkFramesIn = g_linkRecv.frames;
            g_linkSend = g_linkFrameOut;
            SignalSema(g_linkGoSema);
            return 1;
        }
    }
    return 0;
}

/* Clears car, color, tyre and transmission (carClass is kept). */
// FUNC 0x0022D308 link_player_info_clear
void link_player_info_clear(LinkPlayerInfo *p)
{
    p->car = 0;
    p->color = 0;
    p->tyre = 0;
    p->transmission = 0;
}

/* Called by the game init (0x00102C00). */
// FUNC 0x0022D320 F_0022D320
void F_0022D320(void)
{
    link_create();
    F_0022D360();
    g_002EA7F8.attach(&g_linkControl, 4);   /* 0x0022EA88 */
}

// FUNC 0x0022D360 F_0022D360
void F_0022D360(void)
{
    int slot;

    for (slot = 0; slot < LINK_NODES; slot++)
        link_player_info_clear(&g_linkPlayers[slot]);
    g_00352CB0 = 0;
    g_00352CDC = 0;
    g_linkRandomSum = 0;
}

// FUNC 0x0022D3B8 F_0022D3B8
pdi1394 *F_0022D3B8(void)
{
    return g_link;
}

/* Exchanges the race selections, then the game options, with the other
 * nodes (PDI1394_GROUP_ACTIVE: the slots in state play or live).  Each
 * node sends car, carClass, color, tyre, transmission, a random byte and
 * the course; the first five bytes of every slot go to g_linkPlayers, the
 * random bytes are summed (g_linkRandomSum), and an undecided course (0xFF)
 * takes a slot's course.  Then the serialised options go out in messages of
 * 7 bytes, and each slot's options are loaded into MenuStatus::options[slot]
 * of the GameStatus.  0: the exchange failed (quirk: once the options
 * buffers are allocated, they are leaked). */
// FUNC 0x0022D3C0 F_0022D3C0
int F_0022D3C0(int car, int carClass, int color, int tyre, int transmission, int *course)
{
    u8   *in[LINK_NODES];                   /* sp+0x00 each slot's options */
    int   got[LINK_NODES];                  /* sp+0x20 bytes received per slot */
    u32   last;                             /* sp+0x40 bytes of the last message, 0: 7 */
    u8   *out;                              /* sp+0x44 this console's options */
    LinkPlayerInfo *info;
    GameOptions *options;
    u32   size;
    int   nmsgs, msg, slot, i, n, pos;

    g_link->F_00270398();
    if (g_link->setGroupAndSync(PDI1394_GROUP_ACTIVE) == 0)
        return 0;
    g_link->beginPacket(PDI1394_CMD_DATA);
    g_link->putByte((u8)car);
    g_link->putByte((u8)carClass);
    g_link->putByte((u8)color);
    g_link->putByte((u8)tyre);
    g_link->putByte((u8)transmission);
    g_link->putByte(g_linkRandom.next() & 0x7F);    /* 0x0026C118 */
    g_link->putByte((u8)*course);
    g_link->endPacket();
    g_link->sendPacket(PDI1394_GROUP_ACTIVE);
    if (g_link->waitPackets(PDI1394_GROUP_ACTIVE, PDI1394_WAIT_ALL) == 0)
        return 0;

    g_linkRandomSum = 0;
    info = g_linkPlayers;
    for (slot = 0; slot < LINK_NODES; slot++) {
        int theirs;

        g_link->openMessage(slot);
        info->car          = g_link->getByte();
        info->carClass     = g_link->getByte();
        info->color        = g_link->getByte();
        info->tyre         = g_link->getByte();
        info->transmission = g_link->getByte();
        g_linkRandomSum += g_link->getByte();
        theirs = g_link->getByte();
        /* getByte gives 0..255, so the -1 test is never false (the listing
         * keeps it, 0x0022D540) */
        if (theirs != -1 && *course == 0xFF && theirs != *course)
            *course = theirs;
        g_link->closeMessage();
        info++;
    }

    options = getGameStatus()->getGameOptions();
    size = (u32)GameOptions::getSize();
    last = size % 7;
    nmsgs = (int)((size - 1) / 7) + 1;
    for (i = 0; i < LINK_NODES; i++)
        in[i] = (u8 *)malloc(size);
    pos = 0;
    out = (u8 *)malloc(size);
    options->save(out);
    memset(got, 0, sizeof(got));

    for (msg = 0; msg < nmsgs; msg++) {
        int k;

        n = 7;
        if (last != 0 && msg == nmsgs - 1)
            n = last;
        g_link->beginPacket(PDI1394_CMD_DATA);
        for (k = n; k != 0; k--)
            g_link->putByte(out[pos++]);
        g_link->endPacket();
        g_link->sendPacket(PDI1394_GROUP_ACTIVE);
        if (g_link->waitPackets(PDI1394_GROUP_ACTIVE, PDI1394_WAIT_ALL) == 0)
            return 0;                       /* leaks in[] and out */
        for (slot = 0; slot < LINK_NODES; slot++) {
            u8 *dst;
            int j;

            g_link->openMessage(slot);
            dst = in[slot];
            for (j = 0; j < n; j++) {
                u8 b = g_link->getByte();
                int at = got[slot];
                dst[at] = b;
                got[slot] = at + 1;
            }
            g_link->closeMessage();
        }
    }

    options = getGameStatus()->getMenuStatus()->options;
    for (i = 0; i < LINK_NODES; i++) {
        options->load(in[i]);
        options++;
        free(in[i]);
    }
    free(out);
    return 1;
}

/* NULL if the slot has no node. */
// FUNC 0x0022D7A0 F_0022D7A0
LinkPlayerInfo *F_0022D7A0(int slot)
{
    if (g_link->nodeId[slot] >= 0)
        return &g_linkPlayers[slot];
    return NULL;
}

// FUNC 0x0022D7D0 F_0022D7D0
int F_0022D7D0(int slot)
{
    return g_link->nodeState[slot] == PDI1394_STATE_PLAY;
}

// FUNC 0x0022D7F0 F_0022D7F0
int F_0022D7F0(int slot)
{
    return g_link->nodeState[slot] == PDI1394_STATE_LIVE;
}

/* nodeState play or live. */
// FUNC 0x0022D810 F_0022D810
int F_0022D810(int slot)
{
    return (u32)(g_link->nodeState[slot] - PDI1394_STATE_PLAY) < 2;
}

// FUNC 0x0022D830 F_0022D830
int F_0022D830(void)
{
    return g_link->getOwnSlot();
}

// FUNC 0x0022D850 F_0022D850
int F_0022D850(void)
{
    return g_link->getGroupCount(PDI1394_STATE_PLAY);
}

/* Starts the session: semaphores, cleared frames, the link thread
 * (priority 4, a 0x1000-byte stack it frees when it ends). */
// FUNC 0x0022D870 F_0022D870
void F_0022D870(void)
{
    void *stack;

    g_linkGoSema = CreateSema((SemaParam *)&g_linkSemaParam);
    g_linkDoneSema = CreateSema((SemaParam *)&g_linkSemaParam);
    memset(&g_linkFramesIn, 0, sizeof(g_linkFramesIn));
    memset(&g_linkFrameOut, 0, sizeof(g_linkFrameOut));
    memset(&g_linkRecv, 0, sizeof(g_linkRecv));
    memset(&g_linkSend, 0, sizeof(g_linkSend));
    g_linkCmdXchgDue = 0;
    g_linkFirstXchgDue = 1;
    g_linkFramesTaken = 1;
    g_linkCmdOut = LINK_CMD_NONE;
    g_linkCmdIn = LINK_CMD_NONE;
    g_linkState = LINK_STATE_RUN;
    link_enable();
    g_linkThreadQuit = 0;
    stack = memalign(0x10, 0x1000);
    thread_start_retry(link_thread, stack, 4, stack, 0x1000);  /* 0x00269C30 */
}

// FUNC 0x0022D948 link_thread
void link_thread(void *stack)
{
    while (g_linkThreadQuit == 0)
        link_thread_step();
    ChangeThreadPriority(GetThreadId(), 1);
    free(stack);
    ExitDeleteThread();
}

/* Ends the session. */
// FUNC 0x0022D9A8 F_0022D9A8
void F_0022D9A8(void)
{
    g_linkThreadQuit = 1;
    link_disable();
    if (g_linkGoSema != -1)
        DeleteSema(g_linkGoSema);
    g_linkGoSema = -1;
    if (g_linkDoneSema != -1)
        DeleteSema(g_linkDoneSema);
    g_linkDoneSema = -1;
    pdi1394_msg_init();                     /* 0x0026EFA8 */
}

// FUNC 0x0022DA08 F_0022DA08
int F_0022DA08(void)
{
    return g_linkRandomSum;
}

/* Tells the others the race ends (RaceLinkBattle, u047: the end of the
 * race, the pause menu's quit). */
// FUNC 0x0022DA10 F_0022DA10
void F_0022DA10(void)
{
    g_linkCmdOut = LINK_CMD_END;
}

// FUNC 0x0022DA20 F_0022DA20
int F_0022DA20(void)
{
    return g_linkCmdIn == LINK_CMD_END;
}

// FUNC 0x0022DA30 F_0022DA30
int F_0022DA30(void)
{
    return g_linkCmdIn == LINK_CMD_END;
}

/* Compiler-generated: egcs's static destructor and initialiser of the two
 * objects with constructors defined at the top (named after the unit's
 * first global, g_linkGoSema), emitted at the end of the unit, the
 * destructor first. */
// FUNC 0x0022DA40 T473_static_dtor (_GLOBAL_$D$g_linkGoSema)
//   (.dtors entry 0x0028F250) ~Control of g_linkControl (a direct call,
//   flags 2).
/* implicit: ~Control */
// FUNC 0x0022DA68 T473_static_init (_GLOBAL_$I$g_linkGoSema)
//   (.ctors entry 0x0028F17C) Random's inline constructor (seed
//   0x87654321), then Control::Control of g_linkControl.
/* implicit: Random, Control */
