/* libpad_part2.c - the tail of Sony EE libpad "PsIIlibpad  2200"
 * (0x00272B40..0x00272E08).  The rest of libpad (scePadInit ..
 * scePadEnterPressMode) is in u129.
 *
 * Same machine code as the bootstrap's sdk/libpad/libpad.c
 * (0x010154F0..0x010157B8).  All commands go through the one 0x80-byte
 * buffer s_padRpcBuf (word 0 = command, word 1 = port, word 2 = slot, words
 * 3.. arguments / result), server 0, fno 1, mode 0 (sdk/libcdvd/G004_libcdvd.h).
 */
#include "sdk/libmc/libmc_internal.h"

// FUNC 0x00272B40 scePadExitPressMode
/* 0 if not open; else scePadSetButtonInfo(port, slot, 0). */
int scePadExitPressMode(int port, int slot)
{
    if (s_padCtl[port][slot].open == 0)
        return 0;
    return scePadSetButtonInfo(port, slot, 0);
}

// FUNC 0x00272B98 scePadSetVrefParam
/* RPC SET_VREF (0xB) with 12 bytes of data at buffer bytes 0x0C..0x17
 * (inline unaligned 8+4-byte copy, ldl/ldr + lwl/lwr, no memcpy call).
 * 0 on RPC failure; else if reply word 7 == 1 sets reqState Busy; returns
 * word 7 (re-read after the scePadSetReqState call).  No open check. */
int scePadSetVrefParam(int port, int slot, const unsigned char *data)
{
    s_padRpcBuf[1] = port;
    s_padRpcBuf[0] = PAD_RPCCMD_SET_VREF;
    s_padRpcBuf[2] = slot;
    __builtin_memcpy(&s_padRpcBuf[3], data, PAD_VREF_PARAM_SIZE);
    if (sceSifCallRpc(&s_padClient[0], PAD_RPC_FNO, 0, s_padRpcBuf, sizeof(s_padRpcBuf),
                      s_padRpcBuf, sizeof(s_padRpcBuf), NULL, NULL) < 0)
        return 0;
    if (s_padRpcBuf[7] == 1)
        scePadSetReqState(port, slot, scePadReqStateBusy);
    return s_padRpcBuf[7];
}

// FUNC 0x00272C68 scePadGetPortMax
/* RPC GET_PORTMAX (0xC): reply word 3, or 0 if the call fails. */
int scePadGetPortMax(void)
{
    s_padRpcBuf[0] = PAD_RPCCMD_GET_PORTMAX;
    if (sceSifCallRpc(&s_padClient[0], PAD_RPC_FNO, 0, s_padRpcBuf, sizeof(s_padRpcBuf),
                      s_padRpcBuf, sizeof(s_padRpcBuf), NULL, NULL) < 0)
        return 0;
    return s_padRpcBuf[3];
}

// FUNC 0x00272CD0 scePadGetSlotMax
/* RPC GET_SLOTMAX (0xD) with word 1 = port: reply word 3, or 0 on failure. */
int scePadGetSlotMax(int port)
{
    s_padRpcBuf[1] = port;
    s_padRpcBuf[0] = PAD_RPCCMD_GET_SLOTMAX;
    if (sceSifCallRpc(&s_padClient[0], PAD_RPC_FNO, 0, s_padRpcBuf, sizeof(s_padRpcBuf),
                      s_padRpcBuf, sizeof(s_padRpcBuf), NULL, NULL) < 0)
        return 0;
    return s_padRpcBuf[3];
}

// FUNC 0x00272D38 scePadGetModVersion
/* RPC GET_MODVER (0x12): reply word 3 = padman version (major << 8 |
 * minor), or 0 on failure.  Caller: scePadInit (0x00271B90). */
int scePadGetModVersion(void)
{
    s_padRpcBuf[0] = PAD_RPCCMD_GET_MODVER;
    if (sceSifCallRpc(&s_padClient[0], PAD_RPC_FNO, 0, s_padRpcBuf, sizeof(s_padRpcBuf),
                      s_padRpcBuf, sizeof(s_padRpcBuf), NULL, NULL) < 0)
        return 0;
    return s_padRpcBuf[3];
}

// FUNC 0x00272DA0 pad_rpc_cmd_0x14
/* Unidentified padman command 0x14: word 1 = arg; returns reply word 2
 * (not word 3 like the others), or 0 if the call fails.  Sony name unknown
 * (0x14 is not in ps2sdk's command list).  In the core it has one caller:
 * the game's pad setup (T477, 0x0022F810) calls it with 0 right after
 * scePadInit(0), before opening the 8 port/slot pairs (port = i >> 2,
 * slot = i & 3: multitap layout). */
int pad_rpc_cmd_0x14(int arg)
{
    s_padRpcBuf[1] = arg;
    s_padRpcBuf[0] = PAD_RPCCMD_14;
    if (sceSifCallRpc(&s_padClient[0], PAD_RPC_FNO, 0, s_padRpcBuf, sizeof(s_padRpcBuf),
                      s_padRpcBuf, sizeof(s_padRpcBuf), NULL, NULL) < 0)
        return 0;
    return s_padRpcBuf[2];
}
