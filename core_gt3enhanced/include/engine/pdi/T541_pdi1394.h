/* T541_pdi1394.h - pdi1394, Polyphony's i.LINK (IEEE 1394) client class (translation unit T541, unit u128).
 *
 * Over the C driver of G003 (G003_pdi1394_rpc.h).  Up to 8 nodes, numbered by
 * the IOP; the class keeps a slot table sorted by the nodes' 64-bit bus GUIDs
 * (descending), so every console sees the same slot order.  Packets are 8
 * bytes: byte 0 = command (high nibble) | group (low nibble), bytes 1..7 =
 * payload.  Commands: 0x10 state announce / data (waitPackets, collectStates),
 * 0x20 one byte to the current group (sendByte / recvByte), 0x30 barrier,
 * 0x40 the 4-byte session key (sendKey / checkKeys).  A group is a node state
 * (nodeState[]: 0, 1, 2; 0xF = none); group 0xF polls the 8 IOP nodes
 * directly, group 3 matches states 1 and 2.  Users: T473 (link battle
 * session), the menus.
 */
#ifndef GAME_T541_PDI1394_H
#define GAME_T541_PDI1394_H

#include "game/game.h"

/* Packet commands: the high nibble of a packet's byte 0. */
enum {
    PDI1394_CMD_DATA    = 0x10,                 /* waitPackets / collectStates; sendState; T473's exchanges */
    PDI1394_CMD_BYTE    = 0x20,                 /* sendByte / recvByte */
    PDI1394_CMD_BARRIER = 0x30,                 /* barrier / pollBarrier */
    PDI1394_CMD_KEY     = 0x40                  /* sendKey / checkKeys: the 4 key bytes */
};

/* Node states (nodeState[]) and groups (count[], a packet's low nibble): a
 * group addresses the nodes in that state.  Play / live are GT3's use (T013
 * announces 1 for "i_link_play", 2 for "i_link_live"). */
enum {
    PDI1394_STATE_LINKED = 0,                   /* on the bus: updateTopology puts every slot here */
    PDI1394_STATE_PLAY   = 1,
    PDI1394_STATE_LIVE   = 2,
    PDI1394_GROUP_ACTIVE = 3,                   /* group only: states 1 and 2 */
    PDI1394_STATE_NONE   = 0xF                  /* empty slot; as a group: the 8 IOP nodes, not the slots */
};

/* waitPackets flags (T013 passes 5, T473 1). */
enum {
    PDI1394_WAIT_ALL     = 1,                   /* from every node of the group (else from at least one) */
    PDI1394_WAIT_POLL    = 2,                   /* look once, do not wait */
    PDI1394_WAIT_TIMEOUT = 4                    /* give up after 2 driver timeouts (else wait for ever) */
};

/* vtable _vt$7pdi1394 0x003477F0 (1 slot), vptr +0xC0 (egcs).  typeinfo
 * __tf7pdi1394 0x002705E8, __user_type_info 0x00353320.  Size 0xC4 (operator
 * new in link_create 0x0022CB58).  Static helpers of T541:
 * sort_slots_by_guid 0x00270408, guid_greater 0x00270560.
 *
 * The destructor is inline, `virtual ~pdi1394() {}`, as in the original:
 * with no key function egcs writes the type name "7pdi1394" (and a __ti
 * common, no code) into every object that sees the class, and retail has it
 * in the .rodata of exactly the users T034, T022, T025, T028, T013, T014,
 * T473 and T541 (u003's data pass, ref/data/u003/notes.md), the only files
 * that include this header (keep it so: a unit header that includes it
 * gives the name to files whose originals do not have it).  T541's weak
 * copy 0x002705B8 comes right before __tf, as egcs emits a class's inline
 * members; the constructor is out of line, so the users get no vtable copy.
 * (Until u104's data revisit, 2026-10-17, the inline form was behind a
 * GAME_T541_INLINE_DTOR switch: gtsound_internal.h included this header for all of
 * u104's files.) */
class pdi1394 {
public:
    /* Quirk: the shift accumulates (0, 8, 24, 48 bits), so key[] =
     * {k, k >> 8, k >> 24, 0}, not the 4 bytes of k. */
    pdi1394(u32 key);                           /* 0x0026F538 */
    virtual ~pdi1394() {}                       /* [0] +0x0C 0x002705B8 (see above) */

    void init();                                /* 0x0026F570 */
    void resetQueue();                          /* 0x0026F640 */
    int  updateTopology();                      /* 0x0026F660 1 = linked, 0 = link gone */
    int  getOwnSlot();                          /* 0x0026F768 -1 if not linked */
    int  getGroupCount(int group);              /* 0x0026F788 -1 if not linked */
    int  getGroup();                            /* 0x0026F7A8 0xF if not linked */
    int  slotInGroup(int slot, int group);      /* 0x0026F7C8 */
    void sendState(int state);                  /* 0x0026F7F8 */
    void setSlotGroup(int slot, int group);     /* 0x0026F850 */
    void collectStates();                       /* 0x0026F8A8 */
    int  setGroupAndSync(int group);            /* 0x0026FB00 */
    void setGroup(int group);                   /* 0x0026FB28 */
    void sendByte(u8 b);                        /* 0x0026FB30 */
    void sendZero();                            /* 0x0026FB88 */
    int  recvByte(u8 *out);                     /* 0x0026FBA8 */
    int  pollByte();                            /* 0x0026FC28 */
    int  ackByte();                             /* 0x0026FC48 */
    void sendKey();                             /* 0x0026FC70 */
    int  pollKey();                             /* 0x0026FCE0 */
    int  checkKeys();                           /* 0x0026FD08 */
    int  ackKey();                              /* 0x0026FDB8 */
    int  barrier(int group);                    /* 0x0026FDE0 */
    void cancelBarrier();                       /* 0x0026FE98 */
    int  pollBarrier(int group);                /* 0x0026FEB8 nonzero once every node reached the barrier */
    void beginPacket(int cmd);                  /* 0x0026FF58 (int: no andi of cmd at entry) */
    void putByte(u8 b);                         /* 0x0026FFA0 */
    void endPacket();                           /* 0x0026FFB8 (empty) */
    void sendPacket(int group);                 /* 0x0026FFC0 retries every vsync while the RPC is busy */
    u8   openMessage(int slot);                 /* 0x00270010 slot -1: the first slot with a 0x1x message */
    int  getByte();                             /* 0x002700A8 the next byte (lbu: 0..255); int: a caller tests -1 */
    void closeMessage();                        /* 0x002700C8 (empty) */
    int  receive(int group, int cmd, int ack);  /* 0x002700D0 ack = 1 also removes the packets */
    int  waitPackets(int group, int flags);     /* 0x00270258 */
    void F_00270398();                          /* 0x00270398 F_0026EC20 for every slot in state 1 or 2 */
    int  findSlot(int node);                    /* 0x00270520 -1 if none */
    void F_00270550();                          /* 0x00270550 (empty) */
    void F_00270558();                          /* 0x00270558 (empty) */

    u8   key[4];                                /* 0x00 session key (ctor) */
    u8   pkt[8];                                /* 0x04 send / direct-receive packet */
    u8   rx[8][8];                              /* 0x0C received packet per slot */
    u8  *readPtr;                               /* 0x4C message being read */
    int  pos;                                   /* 0x50 read / write position */
    int  ownSlot;                               /* 0x54 slot of IOP node 0 = this console (pdi1394_send_msg loops back to channel 0) */
    int  curGroup;                              /* 0x58 */
    int  count[4];                              /* 0x5C nodes per state; [3] = states 1 + 2 */
    int  group;                                 /* 0x6C group of sendByte / recvByte */
    int  barrierPending;                        /* 0x70 pollBarrier sent its packet */
    int  linked;                                /* 0x74 */
    int  nodeId[8];                             /* 0x78 IOP node of each slot, -1 none */
    int  nodeState[8];                          /* 0x98 0xF none, 0, 1, 2 */
    int  fromSlot;                              /* 0xB8 sender slot of the last direct receive */
    u8   fromByte;                              /* 0xBC its byte 1 */
    /*   vptr                                      0xC0 (egcs) */
};
EGCS_SIZE(pdi1394, 0xC4);

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" {
void *__tf7pdi1394(void);                               /* 0x002705E8 */
extern u32 __ti7pdi1394[2];                             /* 0x00353320 gp-0x46D0 (__user_type_info) */
}
#endif

#endif /* GAME_T541_PDI1394_H */
