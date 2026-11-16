/* iopheap.c - Sony libkernl 2240 iopheap.o: EE client of the IOP
 * heap RPC server 0x80000003 (0x0028C7F8..0x0028CA58).  Not linked into the
 * bootstrap.
 *
 * Four functions: only sceSifInitIopHeap has a caller (jal from the game's
 * IOPLoadModuleBase::cmdInitIopHeap, T524); the other three are uncalled
 * (their starts are in ref/extra_starts.txt).
 *
 * Shared state: _ih_bind_state (0x00308264, .data, -1 until bound), the
 * client _ih_client (0x004B0D00), the 4-byte reply _ih_recv (0x004B0D40) and
 * one 64-byte-aligned request block per call (0x004B0D80, 0x004B0DC0,
 * 0x004B0E00).  Sony's names (sif.h); ps2sdk: SifInitIopHeap /
 * SifAllocIopHeap / SifFreeIopHeap / SifLoadIopHeap.
 */
#include "runtime/newlib/G007_stdio.h"
#include "sdk/libkernl/libkernl_part2_internal.h"

/* iopheap.o's data, all file statics (only this file uses them).  .data:
 * the bound mark.  .bss (0x004B0D00..0x004B0F00, between fileio.o's and
 * loadfile.o's statics, so statics, not commons): the client, then the reply
 * and the three request blocks, each in its own 64-byte-aligned slot (the
 * SIF DMA works in whole cache lines).                                     */
static int              _ih_bind_state = -1;           /* 0x00308264 .data, -1 = not bound */
static sceSifClientData _ih_client;                    /* 0x004B0D00 .bss */
static int              _ih_recv[0x10] __attribute__((aligned(64)));       /* 0x004B0D40 .bss reply: word 0 = result */
static unsigned int     _ih_alloc_arg[0x10] __attribute__((aligned(64)));  /* 0x004B0D80 .bss IH_F_ALLOC request: word 0 = size */
static void            *_ih_free_arg[0x10] __attribute__((aligned(64)));   /* 0x004B0DC0 .bss IH_F_FREE request: word 0 = addr */
static _ih_load_arg_t   _ih_load_arg __attribute__((aligned(64)));         /* 0x004B0E00 .bss IH_F_LOAD request */

// FUNC 0x0028C7F8 sceSifInitIopHeap
//   Bind the IOP heap server: loop sceSifBindRpc(0x80000003) until the server
//   answers (0x100000-pass busy delay between tries), then mark the client
//   bound and return 0; -1 if a bind call fails.  Unlike
//   _sceSifLoadFileInit there is no "already bound" early-out: every call
//   binds again.  The only caller is IOPLoadModuleBase::cmdInitIopHeap (T524).
//   The bound mark is set inside the loop before the break: written after the
//   loop, it shares a block with `return 0` and sched1 sets $v0 between the
//   lui and the sw (96.3%); retail stores first, then sets $v0.
int sceSifInitIopHeap(void)
{
    int spin;

    while (1) {
        if (sceSifBindRpc(&_ih_client, IH_SERVER_ID, 0) < 0)
            return -1;
        if (_ih_client.serve != NULL) {
            _ih_bind_state = 0;
            break;
        }
        spin = BIND_RETRY_SPIN;
        while (spin--)
            SPIN_BODY();
    }
    return 0;
}

// FUNC 0x0028C880 sceSifAllocIopHeap
//   No callers.
//   IH_F_ALLOC: allocate size bytes on the IOP heap.  Returns the IOP address
//   (reply word 0), or NULL if the server is not bound or the call failed.
void *sceSifAllocIopHeap(unsigned int size)
{
    if (_ih_bind_state < 0)
        return NULL;
    _ih_alloc_arg[0] = size;
    if (sceSifCallRpc(&_ih_client, IH_F_ALLOC, 0, _ih_alloc_arg, 4, _ih_recv, 4, NULL, NULL) < 0)
        return NULL;
    return (void *)_ih_recv[0];
}

// FUNC 0x0028C8F0 sceSifFreeIopHeap
//   No callers.
//   IH_F_FREE: free an IOP heap block.  Returns the server's result (reply
//   word 0), -1 if the call failed - but 0 (not an error code) if the server
//   was never bound.
int sceSifFreeIopHeap(void *addr)
{
    if (_ih_bind_state < 0)
        return 0;
    _ih_free_arg[0] = addr;
    if (sceSifCallRpc(&_ih_client, IH_F_FREE, 0, _ih_free_arg, 4, _ih_recv, 4, NULL, NULL) < 0)
        return -1;
    return _ih_recv[0];
}

// FUNC 0x0028C968 sceSifLoadIopHeap
//   No callers.
//   IH_F_LOAD: load a file into IOP heap memory at addr.  The path (at most
//   0xFC bytes with the NUL) is copied into the request, which is sent with
//   size strlen + 5 (addr word + path + NUL).  Returns the server's result,
//   -1 if the call failed, 0 if the server was never bound.
//   The forced NUL at path[0xFB] is stored twice when the path was
//   truncated (once in the truncation branch, once unconditionally).
int sceSifLoadIopHeap(const char *filename, void *addr)
{
    int len;

    if (_ih_bind_state < 0)
        return 0;
    for (len = 0; len < IH_PATH_MAX; len++) {
        if ((_ih_load_arg.path[len] = filename[len]) == 0)
            break;
    }
    if (len == IH_PATH_MAX) {
        _ih_load_arg.path[IH_PATH_MAX - 1] = 0;
        len = IH_PATH_MAX - 1;
    }
    _ih_load_arg.addr = addr;
    _ih_load_arg.path[IH_PATH_MAX - 1] = 0;
    if (sceSifCallRpc(&_ih_client, IH_F_LOAD, 0, &_ih_load_arg, offsetof(_ih_load_arg_t, path) + len + 1,
                      _ih_recv, 4, NULL, NULL) < 0)
        return -1;
    return _ih_recv[0];
}
