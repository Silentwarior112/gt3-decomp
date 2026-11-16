/* T541 - pdi1394: the i.LINK (IEEE 1394) client class.
 * 0x0026F538..0x00270628.
 *
 * Wraps the G003 driver (u127).  Sending: beginPacket(cmd), putByte()...,
 * endPacket(), sendPacket(group) (retries every vsync while the RPC is busy).
 * Receiving: receive(group, cmd, ack) polls the queue of every slot in the
 * group (or of every IOP node for group PDI1394_STATE_NONE) for a message of
 * that command and counts the nodes that sent one; ack = 1 also removes the
 * messages.  updateTopology() rebuilds the slot table from the bus status,
 * sorted by GUID (descending).  Commands and states: engine/pdi/T541_pdi1394.h.
 *
 * ~pdi1394 was inline in the original (no key function: retail has the type
 * name "7pdi1394" in every user's .rodata); egcs writes it at the end of the
 * file, with the weak vtable, before the name and __tf7pdi1394, as retail
 * (0x002705B8, 0x002705E8): the header's inline form, for both compilers. */
#include "engine/pdi/T541_pdi1394.h"
#include "engine/pdi/pdi1394_internal.h"

/* GUIDs of the present nodes in IOP node order (updateTopology; the sort
 * permutes only nodeId[]).  Retail's .bss 0x004AB4D0..0x004AB510, so a file
 * static (libcdvd's .bss follows at 0x004AB540, 64-aligned). */
static Guid1394 g_004AB4D0[8];              /* 0x004AB4D0 */

// FUNC 0x0026F538 pdi1394::pdi1394
pdi1394::pdi1394(u32 sessionKey)
{
    int i;

    /* Quirk: the shift accumulates (0, 8, 24, 48 bits), so key[] =
     * {k, k >> 8, k >> 24, 0}, not the 4 bytes of k. */
    for (i = 0; i < 4; i++)
        key[i] = sessionKey >>= i * 8;
}

// FUNC 0x0026F570 pdi1394::init
void pdi1394::init()
{
    int i;

    pdi1394_msg_init();
    for (i = 3; i >= 0; i--)
        count[i] = 0;
    for (i = 0; i < 8; i++) {
        nodeState[i] = PDI1394_STATE_NONE;
        nodeId[i] = -1;
    }
    barrierPending = 0;
    group = PDI1394_STATE_LINKED;
    linked = 0;
    ownSlot = -1;
    curGroup = PDI1394_STATE_NONE;
    memset(pkt, 0, sizeof pkt);
    memset(rx, 0, sizeof rx);
    readPtr = 0;
}

// FUNC 0x0026F640 pdi1394::resetQueue
void pdi1394::resetQueue()
{
    pdi1394_msg_init();
}

/* Every present node goes to state 0 (PDI1394_STATE_LINKED), in slots sorted
 * by GUID; count[0] = the node count.  0 (and the empty hook F_00270550)
 * when the bus reports more than 8 nodes (nothing changed) or this console
 * is not among them (slot table rebuilt, ownSlot = -1).  linked is never
 * cleared here. */
// FUNC 0x0026F660 pdi1394::updateTopology
int pdi1394::updateTopology()
{
    Pdi1394Status status;
    int node, slot, n, mask;

    pdi1394_get_status(&status);
    if (status.nodeCount < 9) {
        for (slot = 0; slot < 8; slot++) {
            nodeId[slot] = -1;
            nodeState[slot] = PDI1394_STATE_LINKED;
        }
        n = 0;
        mask = status.nodeMask;
        for (node = 0; node < 8; node++) {
            if ((mask >> node) & 1) {
                nodeId[n] = node;
                g_004AB4D0[n] = status.guid[node];
                n++;
            }
        }
        sort_slots_by_guid(nodeId, g_004AB4D0, status.nodeCount);
        ownSlot = findSlot(0);
        if (ownSlot != -1) {
            linked = 1;
            count[PDI1394_STATE_LINKED] = status.nodeCount;
            return 1;
        }
    }
    F_00270550();
    return 0;
}

// FUNC 0x0026F768 pdi1394::getOwnSlot
int pdi1394::getOwnSlot()
{
    if (!linked)
        return -1;
    return ownSlot;
}

// FUNC 0x0026F788 pdi1394::getGroupCount
int pdi1394::getGroupCount(int group)
{
    if (!linked)
        return -1;
    return count[group];
}

// FUNC 0x0026F7A8 pdi1394::getGroup
int pdi1394::getGroup()
{
    if (!linked)
        return PDI1394_STATE_NONE;
    return curGroup;
}

// FUNC 0x0026F7C8 pdi1394::slotInGroup
int pdi1394::slotInGroup(int slot, int group)
{
    int state = nodeState[slot];

    if (group == PDI1394_GROUP_ACTIVE)
        return state == PDI1394_STATE_PLAY || state == PDI1394_STATE_LIVE;
    return state == group;
}

// FUNC 0x0026F7F8 pdi1394::sendState
void pdi1394::sendState(int state)
{
    beginPacket(PDI1394_CMD_DATA);
    putByte(state);
    endPacket();
    sendPacket(PDI1394_STATE_LINKED);
}

/* Moves the slot to another state and counts it there.  count[3] (play +
 * live) is not updated, and an empty slot's state 0xF would index past
 * count[] (no check). */
// FUNC 0x0026F850 pdi1394::setSlotGroup
void pdi1394::setSlotGroup(int slot, int group)
{
    if (nodeState[slot] != group) {
        count[nodeState[slot]]--;
        count[group]++;
        nodeState[slot] = group;
        curGroup = group;
    }
}

/* After waitPackets() on the 0x10 messages: byte 1 of each slot's message is
 * that node's announced state.  The counts are added to (not reset first, and
 * count[0] is left as it was). */
// FUNC 0x0026F8A8 pdi1394::collectStates
void pdi1394::collectStates()
{
    SlotTable oldIds;
    SlotTable oldStates;
    int i, j, state;

    for (i = 0; i < 8; i++) {
        openMessage(i);
        state = getByte();
        switch (state) {
        case PDI1394_STATE_PLAY:
            count[PDI1394_GROUP_ACTIVE]++;
            count[PDI1394_STATE_PLAY]++;
            nodeState[i] = state;
            break;
        case PDI1394_STATE_LIVE:
            count[PDI1394_GROUP_ACTIVE]++;
            count[PDI1394_STATE_LIVE]++;
            nodeState[i] = state;
            break;
        }
        closeMessage();
    }

    /* Reorder the slots: the play nodes first, then the live nodes; the
     * rest are dropped.  i is reused as the next free slot (its 0 set before
     * the clearing loop: retail keeps it in the first loop's s-register and
     * computes the reorder loops' start addresses from it). */
    oldIds = *(SlotTable *)nodeId;
    oldStates = *(SlotTable *)nodeState;
    i = 0;
    for (j = 0; j < 8; j++) {
        nodeState[j] = PDI1394_STATE_NONE;
        nodeId[j] = -1;
    }
    for (j = 0; j < 8; j++) {
        if (oldStates.v[j] == PDI1394_STATE_PLAY) {
            nodeId[i] = oldIds.v[j];
            nodeState[i] = oldStates.v[j];
            i++;
        }
    }
    for (j = 0; j < 8; j++) {
        if (oldStates.v[j] == PDI1394_STATE_LIVE) {
            nodeId[i] = oldIds.v[j];
            nodeState[i] = oldStates.v[j];
            i++;
        }
    }
    ownSlot = findSlot(0);
    F_00270558();
}

// FUNC 0x0026FB00 pdi1394::setGroupAndSync
int pdi1394::setGroupAndSync(int group)
{
    this->group = group;
    return barrier(group);
}

// FUNC 0x0026FB28 pdi1394::setGroup
void pdi1394::setGroup(int group)
{
    this->group = group;
}

// FUNC 0x0026FB30 pdi1394::sendByte
void pdi1394::sendByte(u8 b)
{
    beginPacket(PDI1394_CMD_BYTE);
    putByte(b);
    endPacket();
    sendPacket(group);
}

// FUNC 0x0026FB88 pdi1394::sendZero
void pdi1394::sendZero()
{
    sendByte(0);
}

/* 1 when a 0x20 message is queued for the current group: then (except for
 * group 0xF) a barrier of the group, the messages are removed and *out = the
 * byte of the last sender seen.  Quirk: fromByte is only written by the
 * group-0xF receive, so for the other groups *out is a stale value. */
// FUNC 0x0026FBA8 pdi1394::recvByte
int pdi1394::recvByte(u8 *out)
{
    if (receive(group, PDI1394_CMD_BYTE, 0) == 0)
        return 0;
    if (group != PDI1394_STATE_NONE)
        barrier(group);
    receive(group, PDI1394_CMD_BYTE, 1);
    *out = fromByte;
    return 1;
}

// FUNC 0x0026FC28 pdi1394::pollByte
int pdi1394::pollByte()
{
    u8 ignored;

    return recvByte(&ignored);
}

// FUNC 0x0026FC48 pdi1394::ackByte
int pdi1394::ackByte()
{
    return receive(group, PDI1394_CMD_BYTE, 1);
}

// FUNC 0x0026FC70 pdi1394::sendKey
void pdi1394::sendKey()
{
    int i;

    beginPacket(PDI1394_CMD_KEY);
    for (i = 0; i < 4; i++)
        putByte(key[i]);
    endPacket();
    sendPacket(PDI1394_STATE_LINKED);
}

// FUNC 0x0026FCE0 pdi1394::pollKey
int pdi1394::pollKey()
{
    return receive(PDI1394_STATE_LINKED, PDI1394_CMD_KEY, 0);
}

/* 0 if an IOP node has a queued 0x40 (group 0) message whose key differs
 * from ours. */
// FUNC 0x0026FD08 pdi1394::checkKeys
int pdi1394::checkKeys()
{
    int node, i;

    for (node = 0; node < 8; node++) {
        pdi1394_msg_find_locked(node, (Pdi1394Msg *)pkt, PDI1394_CMD_KEY);
        if (pkt[0] == PDI1394_CMD_KEY) {
            for (i = 0; i < 4; i++) {
                if (pkt[i + 1] != key[i])
                    return 0;
            }
        }
    }
    return 1;
}

// FUNC 0x0026FDB8 pdi1394::ackKey
int pdi1394::ackKey()
{
    return receive(group, PDI1394_CMD_KEY, 1);
}

/* Sends a 0x30 message to the group, then waits until every node of the
 * group (this console included) has sent one, allowing 2 driver timeouts.
 * The messages found are removed either way.  1 = all arrived. */
// FUNC 0x0026FDE0 pdi1394::barrier
int pdi1394::barrier(int group)
{
    int timeoutsLeft;

    beginPacket(PDI1394_CMD_BARRIER);
    timeoutsLeft = 2;
    endPacket();
    sendPacket(group);
    do {
        if (receive(group, PDI1394_CMD_BARRIER, 0) == count[group])
            break;
        if (pdi1394_wait_event())           /* 1 = vblank timeout */
            timeoutsLeft--;
    } while (timeoutsLeft != 0);
    receive(group, PDI1394_CMD_BARRIER, 1);
    return timeoutsLeft > 0;
}

// FUNC 0x0026FE98 pdi1394::cancelBarrier
void pdi1394::cancelBarrier()
{
    barrierPending = 0;
    pdi1394_msg_init();
}

/* Non-blocking barrier: the first call sends the 0x30 message; later calls
 * return 1 (and remove the messages) once every node of the group sent one. */
// FUNC 0x0026FEB8 pdi1394::pollBarrier
int pdi1394::pollBarrier(int group)
{
    if (barrierPending) {
        if (receive(group, PDI1394_CMD_BARRIER, 0) != count[group])
            return 0;
        receive(group, PDI1394_CMD_BARRIER, 1);
        barrierPending = 0;
        return 1;
    }
    beginPacket(PDI1394_CMD_BARRIER);
    endPacket();
    sendPacket(group);
    barrierPending = 1;
    return 0;
}

// FUNC 0x0026FF58 pdi1394::beginPacket
void pdi1394::beginPacket(int cmd)
{
    memset(pkt, 0, sizeof pkt);
    pkt[0] = cmd;
    pos = 1;
}

// FUNC 0x0026FFA0 pdi1394::putByte
void pdi1394::putByte(u8 b)
{
    pkt[pos++] = b;
}

// FUNC 0x0026FFB8 pdi1394::endPacket
void pdi1394::endPacket()
{
}

// FUNC 0x0026FFC0 pdi1394::sendPacket
void pdi1394::sendPacket(int group)
{
    pkt[0] |= group & 0xF;
    while (pdi1394_send_msg((const Pdi1394Msg *)pkt) != 0)     /* 1 = a call still in flight */
        gfx_vsync(1);
}

/* Starts reading rx[slot]; slot -1 = the first slot holding a 0x1x message.
 * Quirk: when none does, slot 8 is used: readPtr = &rx[8] = (u8 *)&readPtr.
 * Returns the message's byte 0. */
// FUNC 0x00270010 pdi1394::openMessage
u8 pdi1394::openMessage(int slot)
{
    int i;

    pos = 0;
    if (slot == -1) {
        for (i = 0; i < 8; i++) {
            if ((rx[i][0] & 0xF0) == PDI1394_CMD_DATA)
                break;
        }
        readPtr = rx[i];
    } else {
        readPtr = rx[slot];
    }
    return readPtr[pos++];
}

// FUNC 0x002700A8 pdi1394::getByte
/* The next byte of the open message, 0..255.  Returns int: T473's
 * LinkSession compares a result with -1 (0x0022D540), a test a u8 result
 * would have folded away. */
int pdi1394::getByte()
{
    return readPtr[pos++];
}

// FUNC 0x002700C8 pdi1394::closeMessage
void pdi1394::closeMessage()
{
}

/* Counts the nodes with a queued message of command cmd (high nibble) for
 * the group.  Group 0xF: polls IOP nodes 0..7 into pkt (fromSlot / fromByte
 * keep the last sender); else polls the node of every slot in the group into
 * rx[slot] (empty slots are cleared, slots of other groups left alone).
 * ack: also remove the messages found. */
// FUNC 0x002700D0 pdi1394::receive
int pdi1394::receive(int group, int cmd, int ack)
{
    int type = (group & 0xF) | (cmd & 0xF0);
    int found = 0;
    int node, slot, id;

    if (group == PDI1394_STATE_NONE) {
        for (node = 0; node < 8; node++) {
            pdi1394_msg_find_locked(node, (Pdi1394Msg *)pkt, type);
            if (pkt[0] == type) {
                found++;
                fromSlot = findSlot(node);
                fromByte = pkt[1];
                if (ack)
                    pdi1394_msg_take_locked(node, (Pdi1394Msg *)pkt, type);
            }
        }
    } else {
        for (slot = 0; slot < 8; slot++) {
            id = nodeId[slot];
            if (id == -1) {
                memset(rx[slot], 0, 8);
            } else if (slotInGroup(slot, group)) {
                pdi1394_msg_find_locked(id, (Pdi1394Msg *)rx[slot], type);
                if (rx[slot][0] == type) {
                    found++;
                    if (ack)
                        pdi1394_msg_take_locked(id, (Pdi1394Msg *)rx[slot], type);
                }
            }
        }
    }
    return found;
}

/* Waits for 0x10 messages from the group (rx[] cleared first), flags
 * PDI1394_WAIT_*.  1 = received: the messages are removed from the queues
 * and kept in rx[slot] (in pkt for group 0xF).  The timeout count goes
 * below 0 when WAIT_TIMEOUT is not set. */
// FUNC 0x00270258 pdi1394::waitPackets
int pdi1394::waitPackets(int group, int flags)
{
    int timeoutsLeft;

    memset(rx, 0, sizeof rx);
    timeoutsLeft = 2;
    if (flags & PDI1394_WAIT_ALL) {
        for (;;) {
            if (receive(group, PDI1394_CMD_DATA, 0) == getGroupCount(group))
                break;
            if (flags & PDI1394_WAIT_POLL)
                return 0;
            if (pdi1394_wait_event())       /* 1 = vblank timeout */
                timeoutsLeft--;
            if (timeoutsLeft == 0 && (flags & PDI1394_WAIT_TIMEOUT))
                return 0;
        }
    } else {
        for (;;) {
            if (receive(group, PDI1394_CMD_DATA, 0) != 0)
                break;
            if (flags & PDI1394_WAIT_POLL)
                return 0;
            if (pdi1394_wait_event())
                timeoutsLeft--;
            if (timeoutsLeft == 0 && (flags & PDI1394_WAIT_TIMEOUT))
                return 0;
        }
    }
    receive(group, PDI1394_CMD_DATA, 1);
    return 1;
}

/* F_0026EC20 (the 'PFIM' control command 0xC0000000 | node << 27) for the
 * IOP node of every play / live slot.  Called by T473 before its data
 * exchange; what the IOP does with the command is not known. */
// FUNC 0x00270398 pdi1394::F_00270398
void pdi1394::F_00270398()
{
    int slot;

    for (slot = 0; slot < 8; slot++) {
        if (nodeState[slot] == PDI1394_STATE_PLAY || nodeState[slot] == PDI1394_STATE_LIVE) {
            if (nodeId[slot] >= 0)
                F_0026EC20(nodeId[slot]);
        }
    }
}

/* Bubble sort of the slots by GUID, descending; ids[] is permuted with them
 * (guids[] itself is not: the sort works on a table of pointers). */
// FUNC 0x00270408 sort_slots_by_guid
void sort_slots_by_guid(int *ids, Guid1394 *guids, int n)
{
    Guid1394 *order[8];
    Guid1394 *tmpGuid;
    int i, j, tmpId;

    for (i = 0; i < n; i++)
        order[i] = &guids[i];
    for (i = 0; i < n; i++) {
        for (j = 0; j < n - 1; j++) {
            if (guid_greater(order[j + 1]->b, order[j]->b, 8)) {
                tmpId = ids[j];
                ids[j] = ids[j + 1];
                ids[j + 1] = tmpId;
                tmpGuid = order[j];
                order[j] = order[j + 1];
                order[j + 1] = tmpGuid;
            }
        }
    }
}

// FUNC 0x00270520 pdi1394::findSlot
int pdi1394::findSlot(int node)
{
    int slot;

    for (slot = 0; slot < 8; slot++) {
        if (nodeId[slot] == node)
            return slot;
    }
    return -1;
}

/* Empty hooks (? debug output compiled out): F_00270550 when updateTopology
 * fails, F_00270558 at the end of collectStates. */
// FUNC 0x00270550 pdi1394::F_00270550
void pdi1394::F_00270550()
{
}

// FUNC 0x00270558 pdi1394::F_00270558
void pdi1394::F_00270558()
{
}

/* 1 if a > b, comparing n unsigned bytes from the first. */
// FUNC 0x00270560 guid_greater
int guid_greater(const u8 *a, const u8 *b, int n)
{
    int i;

    for (i = 0; i < n; i++) {
        if (a[i] > b[i])
            return 1;
        if (a[i] < b[i])
            return 0;
    }
    return 0;
}

// FUNC 0x002705B8 pdi1394::~pdi1394 (_$_7pdi1394)
/* inline in engine/pdi/T541_pdi1394.h, a weak copy here as retail's.  Nothing to
 * destroy; the egcs deleting form then calls operator delete(this) when
 * (flags & 1). */
/* implicit: operator delete (compiler-generated: bases, members, deleting form) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (egcs emits it with the vtable). */
// FUNC 0x002705E8 __tf7pdi1394
extern "C" void *__tf7pdi1394(void)
{
    if (__ti7pdi1394[0] == 0)
        __rtti_user(__ti7pdi1394, "7pdi1394");
    return __ti7pdi1394;
}
#endif
