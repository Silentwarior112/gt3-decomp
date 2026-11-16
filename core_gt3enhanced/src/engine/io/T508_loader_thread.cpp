/* T508 - the loader thread (? name) (u111).  0x00242650..0x00242B38.
 *
 * A thread (priority 0x10, 0x1000-byte static stack) started and stopped
 * around the menus.  Commands in g_00352FB0: load g_002F3D00 (unpacked, into
 * g_00352FAC, or into a new buffer stored there), or exit; while idle it
 * waits a vsync and runs the deferred calls queued in a 16-entry ring
 * (loader_defer).
 */
#include "engine/io/file_loader_internal.h"
#include "engine/io/T508_loader_thread.h"

#define LOADER_THREAD_PRIORITY  0x10
#define LOADER_STACK_SIZE       0x1000

/* The thread's stack: a file static (.bss, the original's
 * 0x004A4240..0x004A5240).  16-aligned, as the SDK samples' thread stacks:
 * T504's entries before it end at 0x004A4238 (0x40 alignment would place it
 * there as well). */
static u8 g_004A4240[LOADER_STACK_SIZE] __attribute__((aligned(16)));  /* 0x004A4240 */

/* A volatile view of a variable (the bootstrap's CD_VI pattern): the other
 * threads' writes of the command and the buffer (loader_request,
 * loader_thread_stop) stay in source order and out of delay slots, as
 * retail has them; the thread itself reads and writes them plainly (egcs's
 * PRE keeps the command in a register there, which a volatile variable
 * would not allow). */
#define LOADER_VOLATILE(x)      (*(volatile __typeof__(x) *)&(x))

// FUNC 0x00242650 loader_init
//   No path, no command, not running, no buffer; the ring cleared.  Caller:
//   game init (0x00102C00).
void loader_init(void)
{
    g_002F3D00[0] = '\0';
    g_00352FB4 = 0;
    g_00352FB0 = LOADER_CMD_NONE;
    g_00352FAC = NULL;
    loader_ring_clear(&g_002F3C30);
}

// FUNC 0x00242688 loader_thread_main
//   ? name.  The thread: dispatches on the command word every pass (an
//   unknown command spins).  egcs's PRE keeps the word in a register: read
//   before the loop, re-read after an idle step and after the exit (which
//   does not return), 0 after a load.
void loader_thread_main(void *arg)
{
    for (;;) {
        switch (g_00352FB0) {
        case LOADER_CMD_NONE:
            gfx_vsync(2);
            loader_run_deferred();
            break;
        case LOADER_CMD_LOAD:
            if (g_00352FAC != NULL)
                file_read_blocking(g_002F3D00, g_00352FAC, 0, 1, &vol_device_object);
            else
                g_00352FAC = file_read_blocking(g_002F3D00, NULL, 0, 1,
                                                &vol_device_object);
            g_00352FB0 = LOADER_CMD_NONE;
            break;
        case LOADER_CMD_EXIT:
            g_00352FB4 = 0;
            g_00352FB0 = LOADER_CMD_NONE;
            ExitDeleteThread();
            break;
        }
    }
}

// FUNC 0x00242750 loader_thread_start
//   running = 1, no buffer; CreateThread (entry loader_thread_main, stack
//   g_004A4240 0x1000 bytes, $gp, priority 0x10, option 0; the other fields
//   of the block are left as they are) and StartThread.  Callers:
//   0x00116508, 0x00116748, 0x001274E0.
void loader_thread_start(void)
{
    struct ThreadParam param;                   /* sp+0x00 */

    g_00352FB4 = 1;
    param.entry = loader_thread_main;
    param.stack = g_004A4240;
    param.stackSize = LOADER_STACK_SIZE;
    param.initPriority = LOADER_THREAD_PRIORITY;
    param.gpReg = _gp;
    g_00352FAC = NULL;
    param.option = 0;
    g_00352FB8 = CreateThread(&param);
    StartThread(g_00352FB8, NULL);
}

// FUNC 0x002427C0 loader_thread_stop
//   Cancels a load in progress (file_req_cancel) and waits for it, then
//   sends the exit command and waits for it to be taken (one vsync per
//   poll).
void loader_thread_stop(void)
{
    if (g_00352FB0 == LOADER_CMD_LOAD) {
        file_req_cancel();
        while (!loader_idle())
            gfx_vsync(1);
    }
    gfx_vsync(2);
    LOADER_VOLATILE(g_00352FB0) = LOADER_CMD_EXIT;
    while (!loader_idle())
        gfx_vsync(1);
    g_00352FB4 = 0;
}

// FUNC 0x00242840 loader_request
//   ? name.  If there is no command: copies path, sets the buffer (NULL: the
//   thread allocates), command load: 1.  Else 0.
int loader_request(const char *path, void *buf)
{
    if (g_00352FB0 != LOADER_CMD_NONE)
        return 0;

    int wasEnabled = intr_suspend();
    strcpy(g_002F3D00, path);
    LOADER_VOLATILE(g_00352FAC) = buf;
    LOADER_VOLATILE(g_00352FB0) = LOADER_CMD_LOAD;
    intr_resume(wasEnabled);
    return 1;
}

// FUNC 0x002428C0 loader_cancel
//   ? name.  file_req_cancel; 1.
int loader_cancel(void)
{
    file_req_cancel();
    return 1;
}

// FUNC 0x002428E0 loader_result
//   ? name.  The buffer / the data of the last load.  No callers.
void *loader_result(void)
{
    return g_00352FAC;
}

// FUNC 0x002428E8 loader_idle
//   ? name.  No command pending.
int loader_idle(void)
{
    return g_00352FB0 == LOADER_CMD_NONE;
}

// FUNC 0x002428F8 loader_defer
//   Queues func(arg, arg2) for the thread's next idle step (interrupts off;
//   no overflow check).  Caller: 0x001B83F0.
void loader_defer(LoaderFn func, void *arg, int arg2)
{
    int wasEnabled = intr_suspend();
    LoaderCallback cb;                          /* sp+0x00 */

    cb.func = func;
    cb.arg = arg;
    cb.arg2 = (void *)arg2;
    loader_ring_push(&g_002F3C30, &cb);
    intr_resume(wasEnabled);
}

// FUNC 0x00242968 loader_run_deferred
//   ? name.  Runs the calls queued when it starts, oldest first, with
//   interrupts off throughout.
void loader_run_deferred(void)
{
    int wasEnabled = intr_suspend();
    int n = g_002F3C30.count;

    for (int i = 0; i < n; i++) {
        LoaderCallback cb = loader_ring_pop(&g_002F3C30);   /* sp+0x00 */
        cb.func(cb.arg, (int)cb.arg2);
    }
    intr_resume(wasEnabled);
}

// FUNC 0x002429E8 loader_ring_clear
//   ? name.  Empty, every entry zeroed.
void loader_ring_clear(LoaderRing *ring)
{
    ring->count = 0;
    ring->rd = 0;
    ring->wr = 0;
    for (int i = 0; i < LOADER_RING_SIZE; i++)
        ring->e[i].clear();
}

// FUNC 0x00242A28 loader_ring_push
//   ? name.  Stores *cb at wr (wrapping at 16), count + 1.
void loader_ring_push(LoaderRing *ring, const LoaderCallback *cb)
{
    int slot = ring->wr;

    ring->wr++;
    ring->e[slot] = *cb;
    if (ring->wr >= LOADER_RING_SIZE)
        ring->wr = 0;
    ring->count++;
}

// FUNC 0x00242A78 loader_ring_pop
//   ? name.  Takes the entry at rd (zeroing it, wrapping at 16), count - 1.
LoaderCallback loader_ring_pop(LoaderRing *ring)
{
    int slot = ring->rd;

    ring->rd++;
    LoaderCallback cb = ring->e[slot];         /* sp+0x00 */
    ring->e[slot].clear();
    if (ring->rd >= LOADER_RING_SIZE)
        ring->rd = 0;
    ring->count--;
    return cb;
}

/* The ring (.data 0x002F3C30, at the start of T508's data: egcs puts a
 * public object that has a constructor in .data, zero-filled, not in .bss). */
LoaderRing g_002F3C30;

/* The path, the buffer, the command, the running flag and the thread id:
 * uninitialised globals, which egcs writes as zeros at the end of the file
 * (.data after the ring, .sdata), in file_loader_internal.h's declaration order. */
char  g_002F3D00[0x40];                         /* 0x002F3D00 */
void *g_00352FAC;                               /* 0x00352FAC gp-0x4A44 */
int   g_00352FB0;                               /* 0x00352FB0 gp-0x4A40 */
int   g_00352FB4;                               /* 0x00352FB4 gp-0x4A3C */
int   g_00352FB8;                               /* 0x00352FB8 gp-0x4A38 */

/* Compiler-generated: egcs's static initialiser of g_002F3C30 (.ctors
 * entry 0x0028F1A8), emitted at the end of the unit: LoaderCallback() on
 * its 16 entries (count / rd / wr are not touched). */
// FUNC 0x00242B00 T508_static_init (_GLOBAL_$I$loader_init__Fv)
/* implicit: LoaderCallback (the static initialiser of g_002F3C30) */
