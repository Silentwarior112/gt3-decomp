/* mcard_io.cpp - blocking memory-card wrappers over Sony libmc (unit "io").
 * Functions 0x0100F130..0x0100F538.
 *
 * Pattern of every *_sync wrapper (all identical apart from the libmc call):
 *
 *     int cmd, result;                        // sp+0x00, sp+0x04
 *     WaitSema(s_mc_lock_sema);               // return value ignored
 *     while (sceMcXxx(args) != 0)             // libmc busy (-200) / not bound
 *         yield_low_priority();               //   -> yield and re-issue
 *     mc_sync_wait(&cmd, &result);            // spin until sceMcSync == 1
 *     <per-function unlock stub>();           // iSignalSema(s_mc_lock_sema)
 *     return result;
 *
 * Note: WaitSema is issued even if mc_init_once never ran (sema id -1); the
 * kernel then just returns an error and the wrapper runs unlocked.
 *
 * Unlock stubs: every wrapper calls (jal) its OWN 2-instruction copy of the
 * unlock placed right after it (0x0100F198, 0x0100F288, 0x0100F338,
 * 0x0100F440, 0x0100F4D0; mc_init_once's copy is at 0x0100F3A0, inside its
 * ref/funcs.json range, and mc_init_once reaches it with `j`, i.e. as a
 * sibling call).  A single shared static function would exist only once, so
 * the original most likely gave each wrapper its own unlock routine emitted
 * right after it (?) - e.g. a static helper per wrapper source/macro.  It was
 * not C++ with a lock object: like the rest of this unit these functions have
 * no .eh_frame/metadata records.  They are written here as explicit static
 * helpers so every call in the binary has a counterpart.
 *
 * Tail jumps: the stubs are `j signal_sema_helper` with the semaphore load in
 * the delay slot, and mc_init_once ends with `ld ra; j stub; addiu sp`.  egcs
 * turns a final call whose value is unused into such a jump only without
 * exception support (as in src/pdistd/pdistd_misc.cpp), so the stubs return
 * void here and this file matches only when compiled with -fno-exceptions,
 * which the missing .eh_frame records say the original was (compile_flags
 * rule requested; with exceptions every stub keeps a jal + frame).
 */
#include "types.h"
#include "eekernel.h"
#include "libmc.h"
#include "boot_io.h"

/* Product code, the middle part of the memory-card directory name
 * "/BASCUS-97102GAMEDATA" built by McBaseDir::McBaseDir (src/filename.cpp, the
 * only user).  A global array of some object between imageloader's and gs's
 * .rodata in the original (0x0103E350 follows ImageLoader::MCardSource's RTTI
 * name); which one is unknown, so it is defined here, with the other
 * memory-card code. */
extern const char s_mc_product[] = "SCUS-97102";   // @0x0103E350 (.rodata)

/* MC lock semaphore id, -1 = not created. */
static int s_mc_lock_sema = -1;   // @0x01041E38 (.sdata, gp-0x7EB8)

static void mc_close_sync_unlock(void);
static void mc_getdir_sync_unlock(void);
static void mc_getinfo_sync_unlock(void);
static void mc_open_sync_unlock(void);
static void mc_read_sync_unlock(void);
static void mc_init_once_unlock(void);

// FUNC 0x0100F130 mc_close_sync
// Blocking sceMcClose(fd).  Returns the mcserv result (0 or negative
// sceMcRes*).  Callers: ImageLoader::MCardSource::open (0x01005458) and
// MCardSource::getLoadFlags (0x01005748), always only when fd >= 0.
int mc_close_sync(int fd)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(s_mc_lock_sema);
    while (sceMcClose(fd) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_close_sync_unlock();
    return result;
}

// FUNC 0x0100F198 mc_close_sync_unlock
// Out-of-line unlock copy used only by mc_close_sync:
// `j signal_sema_helper` with `lw $a0, s_mc_lock_sema` in the delay slot.
static void mc_close_sync_unlock(void)
{
    signal_sema_helper(s_mc_lock_sema);
}

// FUNC 0x0100F1A0 mc_shutdown
// Deletes the MC lock semaphore if it exists and marks it unset (-1).
// Nothing else - no libmc call.  Caller: shutdown_iop_services (0x010009D8),
// right before core.gt3 is launched.
void mc_shutdown(void)
{
    if (s_mc_lock_sema >= 0) {          /* bltz -> skip */
        DeleteSema(s_mc_lock_sema);
        s_mc_lock_sema = -1;
    }
}

// FUNC 0x0100F1D0 mc_getdir_sync
// Blocking sceMcGetDir(port, slot, name, mode, maxent, table) (6th argument in
// $t1).  Returns the mcserv result: number of entries written, or negative
// sceMcRes*.  Caller: ImageLoader::MCardSource::open (0x01005458) with
// mode 0, maxent 1 and a 64-byte-aligned table (used as a stat: size @+0x10).
int mc_getdir_sync(int port, int slot, const char *name, unsigned int mode,
                   int maxent, sceMcTblGetDir *table)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(s_mc_lock_sema);
    while (sceMcGetDir(port, slot, name, mode, maxent, table) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_getdir_sync_unlock();
    return result;
}

// FUNC 0x0100F288 mc_getdir_sync_unlock
// Out-of-line unlock copy used only by mc_getdir_sync (same code as 0x0100F198).
static void mc_getdir_sync_unlock(void)
{
    signal_sema_helper(s_mc_lock_sema);
}

// FUNC 0x0100F290 mc_getinfo_sync
// Blocking sceMcGetInfo(port, slot, &type, &free, &format).  libmc copies the
// three out-values in its RPC end-callback, i.e. by the time sceMcSync reports
// completion.  Returns the mcserv result (0 / -1 changed card / -2 no format
// ...).  Caller: mcard_is_usable (0x010053C8).
int mc_getinfo_sync(int port, int slot, int *type, int *free, int *format)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(s_mc_lock_sema);
    while (sceMcGetInfo(port, slot, type, free, format) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_getinfo_sync_unlock();
    return result;
}

// FUNC 0x0100F338 mc_getinfo_sync_unlock
// Out-of-line unlock copy used only by mc_getinfo_sync (same code as 0x0100F198).
static void mc_getinfo_sync_unlock(void)
{
    signal_sema_helper(s_mc_lock_sema);
}

// FUNC 0x0100F340 mc_init_once
// One-time memory-card setup, called from boot_iop_reboot_and_pad_init
// (0x01000880) after the IRX modules (mcman/mcserv) are loaded:
// if the MC lock does not exist yet, create it with count 1, take it
// (WaitSema on the freshly returned id), retry sceMcInit() with yields until
// it returns 0, then release via the inline unlock tail at 0x0100F3A0
// (`j signal_sema_helper`, re-reading s_mc_lock_sema from .sdata).
// Return value: none in practice (v0 is the sema id on the fast path and
// iSignalSema's result otherwise; the caller ignores it).
void mc_init_once(void)
{
    if (s_mc_lock_sema < 0) {           /* bgez -> plain return */
        int sid = create_mutex_sema(1);
        s_mc_lock_sema = sid;
        WaitSema(sid);
        while (sceMcInit() != 0)
            yield_low_priority();
        mc_init_once_unlock();              /* tail jump 0x0100F388 -> 0x0100F3A0 */
    }
}

// FUNC 0x0100F3A0 mc_init_once_unlock
// mc_init_once's own unlock copy (same code as 0x0100F198), a separate static
// function like the five mc_*_sync_unlock stubs; reached only by
// mc_init_once's tail jump.  Its start is in ref/extra_starts.txt.
static void mc_init_once_unlock(void)
{
    signal_sema_helper(s_mc_lock_sema);
}

// FUNC 0x0100F3A8 mc_open_sync
// Blocking sceMcOpen(port, slot, name, mode).  Returns the mcserv result =
// memory-card fd (>= 0) or negative sceMcRes*.  Callers:
// MCardSource::open (0x01005458) and MCardSource::getLoadFlags (0x01005748),
// both with mode 1 (SCE_RDONLY).
int mc_open_sync(int port, int slot, const char *name, int mode)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(s_mc_lock_sema);
    while (sceMcOpen(port, slot, name, mode) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_open_sync_unlock();
    return result;
}

// FUNC 0x0100F440 mc_open_sync_unlock
// Out-of-line unlock copy used only by mc_open_sync (same code as 0x0100F198).
static void mc_open_sync_unlock(void)
{
    signal_sema_helper(s_mc_lock_sema);
}

// FUNC 0x0100F448 mc_read_sync
// Blocking sceMcRead(fd, buf, size).  Returns the mcserv result = bytes read
// or negative sceMcRes*.  Callers: MCardSource::open (whole file into the
// image buffer) and MCardSource::getLoadFlags (2 bytes = u16 loadSourceFlags).
int mc_read_sync(int fd, void *buf, int size)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(s_mc_lock_sema);
    while (sceMcRead(fd, buf, size) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_read_sync_unlock();
    return result;
}

// FUNC 0x0100F4D0 mc_read_sync_unlock
// Out-of-line unlock copy used only by mc_read_sync (same code as 0x0100F198).
static void mc_read_sync_unlock(void)
{
    signal_sema_helper(s_mc_lock_sema);
}

// FUNC 0x0100F4D8 mc_sync_wait
// Calls sceMcSync(0 = wait mode, cmd, result) first, and yields + retries
// until it returns exactly 1 (sceMcExecFinish; compared with `bne v0, s2=1`).
// Quirk: if nothing is pending sceMcSync returns -1 (idle), which this loop
// would treat as "not finished" and spin on forever; the wrappers only call
// it after a successfully issued request.
void mc_sync_wait(int *cmd, int *result)
{
    while (sceMcSync(0, cmd, result) != sceMcExecFinish)
        yield_low_priority();
}
