/* T527 - PDIClientRpc.  0x0025B198..0x0025B490.
 *
 * The EE half of one RPC server of Polyphony's IOP modules (pdicdvd, pdispu2,
 * pdibgm, pdimpeg, pdipipe).  A call: WaitSema(sema), fill the derived send
 * area, call(fno, mode, recv, rsize).  The end function (endFunc) runs the
 * virtual onCallEnd, clears busy and releases sema again, so a NOWAIT call
 * keeps the object locked until the IOP has answered. */
#include "engine/pdi/T527_pdi_client_rpc.h"
#include "engine/pdi/pdi_internal.h"

/* Callers: PDIClientRpc::bind (mode 0).  sceSifBindRpc < 0: no free packet,
 * retried; then it waits until the server answered (cd->serve set). */
// FUNC 0x0025B198 pdi_bind_rpc
void pdi_bind_rpc(sceSifClientData *cd, u32 rpcId, u32 mode)
{
    cd->serve = NULL;
    do {
        while (sceSifBindRpc(cd, rpcId, mode) < 0)
            yield_low_priority();
        yield_low_priority();
    } while (cd->serve == NULL);
}

// FUNC 0x0025B208 PDIClientRpc::PDIClientRpc
PDIClientRpc::PDIClientRpc()
{
    sema = create_mutex_sema(1);
    busy = 0;
}

// FUNC 0x0025B248 PDIClientRpc::~PDIClientRpc
PDIClientRpc::~PDIClientRpc()
{
    DeleteSema(sema);
}

/* The end function given to every call (end parameter = this). */
// FUNC 0x0025B2A0 PDIClientRpc::endFunc
void PDIClientRpc::endFunc(void *self)
{
    PDIClientRpc *rpc = (PDIClientRpc *)self;

    rpc->onCallEnd();
    rpc->busy = 0;
    signal_sema_helper(rpc->sema);
}

// FUNC 0x0025B2E0 PDIClientRpc::bind
void PDIClientRpc::bind(u32 rpcId)
{
    pdi_bind_rpc(&cd, rpcId, 0);
}

/* Retries (yielding) until sif_call_rpc could send the request. */
// FUNC 0x0025B300 PDIClientRpc::call
int PDIClientRpc::call(u32 fno, u32 mode, void *recv, int rsize)
{
    busy = 1;
    while (sif_call_rpc(&cd, fno, mode, sendBuf, sendSize, recv, rsize,
                        PDIClientRpc::endFunc, this) != 0)
        yield_low_priority();
    return 0;
}

// FUNC 0x0025B3B0 PDIClientRpc::lockedCall
int PDIClientRpc::lockedCall(u32 fno, u32 mode)
{
    WaitSema(sema);
    return call(fno, mode, NULL, 0);
}

/* index.txt: PDIClientRpc::virtual_1.  Hook run by endFunc; the pdicdvd RPC
 * overrides it. */
// FUNC 0x0025B408 PDIClientRpc::onCallEnd
void PDIClientRpc::onCallEnd()
{
}

/* No callers (out-of-line copy of an inline method). */
// FUNC 0x0025B410 PDIClientRpc::unlock
void PDIClientRpc::unlock()
{
    signal_sema_helper(sema);
}

/* No callers (out-of-line copy of an inline method). */
// FUNC 0x0025B430 PDIClientRpc::lock
void PDIClientRpc::lock()
{
    WaitSema(sema);
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0025B450 __tf12PDIClientRpc
extern "C" void *__tf12PDIClientRpc(void)
{
    if (__ti12PDIClientRpc[0] == 0)
        __rtti_user(__ti12PDIClientRpc, "12PDIClientRpc");
    return __ti12PDIClientRpc;
}
#endif
