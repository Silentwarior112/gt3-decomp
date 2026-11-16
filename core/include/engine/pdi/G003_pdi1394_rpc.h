/* G003_pdi1394_rpc.h - the no-FDE stretch G003 (unit u127, C): pdistd helpers and the C core
 * of the EE side of the i.LINK (IEEE 1394) driver.
 *
 * Modules (link order):
 *   pdistd_misc.c  thread_create_retry, then the small pdistd helpers that
 *                       are the same machine code as the bootstrap's
 *                       pdistd_misc.cpp (global lock, break stubs, printf_to_deci2)
 *   pdi1394_rpc.c  SIF RPC clients of the IOP servers 'PFIM' (control) and
 *                       'PFIS' (frame exchange), an EE RPC server 'PFWE' the IOP
 *                       calls back (status block, incoming 8-byte messages), a
 *                       vblank timeout wait and 8 message queues over a 32-node
 *                       pool.  The C++ wrapper class pdi1394 follows (T541).
 * ("pdi1394_" is the first pass's prefix (?): from the IOP module name and the
 * T541 class; the RPC ids are 'PF..' fourccs.)
 */
#ifndef GAME_G003_PDI1394_RPC_H
#define GAME_G003_PDI1394_RPC_H

#include "game/game.h"

#ifdef __cplusplus
extern "C" {
#endif

/* An 8-byte i.LINK message: byte 0 = command (high nibble) | group (low
 * nibble), bytes 1..7 = payload (T541).  Alignment 1: every copy of it is
 * ldl/ldr + sdl/sdr. */
typedef struct Pdi1394Msg {
    u8 type;                                    /* 0x00 key of find / take */
    u8 body[7];                                 /* 0x01 */
} Pdi1394Msg;
EGCS_SIZE(Pdi1394Msg, 0x8);

/* A node's 64-bit bus GUID, compared byte by byte (big-endian order).
 * Byte-aligned (copied with ldl/ldr). */
typedef struct Guid1394 {
    u8 b[8];                                    /* 0x0 */
} Guid1394;
EGCS_SIZE(Guid1394, 0x8);

/* The link status block the IOP pushes to the EE server (fno -1).
 * pdi1394_get_status copies 0x4C bytes (the callers' stack copies are 0x50). */
typedef struct Pdi1394Status {
    Guid1394 guid[8];                           /* 0x00 by IOP node number */
    int      nodeCount;                         /* 0x40 nodes on the bus (consoles connected) */
    int      nodeMask;                          /* 0x44 bit n = IOP node n present */
    int      unk48;                             /* 0x48 */
} Pdi1394Status;
EGCS_SIZE(Pdi1394Status, 0x4C);

/* Called by the EE server after every call it does not handle itself. */
typedef void (*Pdi1394Callback)(void *arg, unsigned int fno);

/* ---- pdistd_misc.c ------------------------------------------------- */

/* CreateThread (entry, stack, stackSize, &_gp, priority, option 0) until it
 * does not return -1, yielding between tries; returns the thread id.
 * Caller: thread_start_retry (0x00269C30). */
int  thread_create_retry(void (*entry)(void *), int priority, void *stack,
                         int stackSize);                    /* 0x0026E310 */
/* Binary semaphore created on first use (id at gp-0x4818, 0x003531D8, = -1)
 * serialising the "host:" fileio wrappers; returns WaitSema's result. */
int  global_lock_acquire(void);                             /* 0x0026E370 */
void global_lock_release(void);                             /* 0x0026E3A8 */
/* `break; return 0`.  .sdata pointers to them: 0x003531C0 / 0x003531C4.
 * abort() calls break_stub_1(6) (newlib's raise()). */
int  break_stub_0(void);                                    /* 0x0026E3B0 */
int  break_stub_1(int code);                                /* 0x0026E3C0 */
/* PrintTarget::flush's backend (T533): sce_printf_deci2("%s", s), the kernel
 * TTY printf (bootstrap: tty_puts). */
void printf_to_deci2(const char *s);                        /* 0x0026E3D0 */

/* ---- pdi1394_rpc.c ------------------------------------------------- */

/* Once, before the IOP modules are loaded (RaceIOPLoadModules, T009, with
 * priority 1): queues, semaphore, the EE server thread. */
void pdi1394_init(int priority);                            /* 0x0026E3E0 */
/* After the IOP modules are loaded: binds 'PFIM' and 'PFIS' (bug: the second
 * wait loop tests the 'PFIM' client, so 'PFIS' is bound exactly once). */
void pdi1394_bind(void);                                    /* 0x0026E530 */
/* Message queues 0..7 under the lock; *out is zeroed when nothing matched. */
void pdi1394_msg_find_locked(int chan, Pdi1394Msg *out, int type);  /* 0x0026E6B0 */
void pdi1394_msg_take_locked(int chan, Pdi1394Msg *out, int type);  /* 0x0026E718 */
/* Sends to 'PFIM' (fno 0) and pushes to the local channel 0; 1 = a 'PFIM'
 * call is still in flight (nothing sent), else sceSifCallRpc's result. */
int  pdi1394_send_msg(const Pdi1394Msg *msg);               /* 0x0026E888 */
/* One frame exchange with 'PFIS', fno 0x80000000 | mode << 27 | seq; the
 * reply is 8 frames of 0x7C bytes.  Returns 1 (busy), 2, 3 (bad reply) or
 * sceSifCallRpc's result.  Caller: the link session thread (T473). */
int  pdi1394_exchange(void *send, void *recv, int size, int mode,
                      int seq);                             /* 0x0026E938 */
/* 'PFIM' control command 0x40000000, SIF_RPCM_NOWAIT; 1 if a call is in
 * flight.  The link reset (? from the callers). */
int  F_0026EB90(void);                                      /* 0x0026EB90 */
/* Waits for 'PFIM' to be idle, then sends 0xC0000000 | node << 27 (node not
 * masked).  Caller: pdi1394::F_00270398 (T541), with the IOP node (nodeId[])
 * of every slot in state 1 or 2. */
void F_0026EC20(int node);                                  /* 0x0026EC20 */
/* Stores arg then func under the lock; returns func (the new one). */
Pdi1394Callback pdi1394_set_callback(Pdi1394Callback func, void *arg);   /* 0x0026ECB8 */
/* Sleeps until the EE server is called (0) or a vblank timeout (1); 1 at once
 * if another thread is waiting. */
int  pdi1394_wait_event(void);                              /* 0x0026ED90 */
/* Copies the Pdi1394Status block (0x4C bytes) to out under the lock.  `void *'
 * as in the original: retail's inlined memcpy uses lwl/lwr for the last
 * word, an alignment the struct type would not give (u127 matching notes). */
void pdi1394_get_status(void *out);                         /* 0x0026EE38 */
/* Empties the node pool and the 8 channels. */
void pdi1394_msg_init(void);                                /* 0x0026EFA8 */

#ifdef __cplusplus
}
#endif

#endif /* GAME_G003_PDI1394_RPC_H */
