/* T528_sif_call_rpc.h - sif_call_rpc (translation unit T528, unit u119).
 *
 * A C header as well: the G002 RPC clients (u123: C) call sif_call_rpc, so it
 * has C linkage.  The two G001 .data objects the plan placed here
 * (g_002F72C0, g_gsDmaChan) are declared in G001_flare.h. */
#ifndef GAME_T528_SIF_CALL_RPC_H
#define GAME_T528_SIF_CALL_RPC_H

#include "game/game.h"
#include "sif.h"                                /* sceSifClientData, sceSifEndFunc (typedefs) */

#ifdef __cplusplus
extern "C" {
#endif

/* sceSifCallRpc with SIF_RPC_M_NOWBDC (2) forced on: the send buffer (size
 * rounded up to 16) and the receive buffer are written back here instead.
 * Returns what sceSifCallRpc returns (0 = sent). */
int sif_call_rpc(sceSifClientData *cd, u32 fno, u32 mode, void *send, int ssize,
                 void *recv, int rsize, sceSifEndFunc endFunc, void *endParam); /* 0x0025B0C8 */

#ifdef __cplusplus
}
#endif

#endif /* GAME_T528_SIF_CALL_RPC_H */
