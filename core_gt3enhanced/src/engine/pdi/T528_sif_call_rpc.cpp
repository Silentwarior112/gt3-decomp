/* T528 - sif_call_rpc.  0x0025B0C8..0x0025B198.
 *
 * A wrapper of sceSifCallRpc that writes the D-cache back itself (the send
 * size rounded up to 16 bytes) and then calls with SIF_RPC_M_NOWBDC (2).
 * Callers: PDIClientRpc::call (T527), pjoy_rpc_query (0x00266698),
 * pjoy_rpc_command (0x00266750). */
#include "engine/pdi/T528_sif_call_rpc.h"     /* C linkage */
#include "engine/pdi/pdi_internal.h"

// FUNC 0x0025B0C8 sif_call_rpc
int sif_call_rpc(sceSifClientData *cd, u32 fno, u32 mode, void *send, int ssize,
                 void *recv, int rsize, sceSifEndFunc endFunc, void *endParam)
{
    if (send != NULL) {
        ssize = (ssize + 0xF) & ~0xF;
        SyncDCache(send, (u8 *)send + ssize);
    }
    if (recv != NULL)
        SyncDCache(recv, (u8 *)recv + rsize);
    return sceSifCallRpc(cd, fno, mode | SIF_RPC_M_NOWBDC, send, ssize, recv, rsize,
                         endFunc, endParam);
}
