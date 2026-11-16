/* T527_pdi_client_rpc.h - PDIClientRpc (translation unit T527, unit u119). */
#ifndef GAME_T527_PDI_CLIENT_RPC_H
#define GAME_T527_PDI_CLIENT_RPC_H

#include "game/game.h"
#include "sif.h"                                /* sceSifClientData, held by value */

/* The EE half of one client of a Polyphony pdi IOP RPC server (pdicdvd,
 * pdispu2, pdibgm, pdimpeg, pdipipe: the RPC classes of those files derive
 * from it).  A call: lock (WaitSema(sema)), fill the derived send area, then
 * call(fno, mode, recv, rsize).  The end function runs onCallEnd, clears busy
 * and releases sema, so a NOWAIT call keeps the object locked until the IOP
 * has answered.
 *
 * vtable 0x003457D8 (2 slots), typeinfo __tf12PDIClientRpc 0x0025B450.
 * vptr +0x38: its last word, so the derived classes' members start at +0x3C.
 * Size 0x3C. */
class PDIClientRpc {
public:
    PDIClientRpc();                                         /* 0x0025B208 */
    virtual ~PDIClientRpc();                                /* [0] +0x0C 0x0025B248 */
    virtual void onCallEnd();                               /* [1] +0x14 0x0025B408 (empty; pdicdvd's RPC overrides it) */

    static void endFunc(void *self);                        /* 0x0025B2A0 the sceSifCallRpc end function (param = this) */
    void bind(u32 rpcId);                                   /* 0x0025B2E0 */
    /* sema must be held; mode 0 = wait, 1 = SIF_RPC_M_NOWAIT.  Returns 0. */
    int  call(u32 fno, u32 mode, void *recv, int rsize);    /* 0x0025B300 */
    int  lockedCall(u32 fno, u32 mode);                     /* 0x0025B3B0 WaitSema(sema) + call without data */
    void unlock();                                          /* 0x0025B410 */
    void lock();                                            /* 0x0025B430 */

    int              sema;      /* 0x00 call lock, create_mutex_sema(1) */
    sceSifClientData cd;        /* 0x04 */
    void            *sendBuf;   /* 0x2C the derived object's send area (+0x40 or +0x50) */
    int              sendSize;  /* 0x30 */
    int              busy;      /* 0x34 1 while a call is in flight */
    /*  vptr                       0x38 */
};
EGCS_SIZE(PDIClientRpc, 0x3C);

/* Binds cd to the server rpcId, retrying (and yielding) until it is bound. */
void pdi_bind_rpc(sceSifClientData *cd, u32 rpcId, u32 mode);   /* 0x0025B198 */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
extern "C" {
void *__tf12PDIClientRpc(void);                 /* 0x0025B450 */
extern u32 __ti12PDIClientRpc[2];               /* 0x003534D8 gp-0x4518 (__user_type_info) */
}
#endif

#endif /* GAME_T527_PDI_CLIENT_RPC_H */
