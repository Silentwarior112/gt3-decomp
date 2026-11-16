/* mcard_io.c - blocking memory-card wrappers over Sony libmc.
 *
 * The bootstrap's src/mcard_io.cpp, extended to the whole libmc API (the
 * bootstrap has close / getdir / getinfo / open / read; 9 functions are the
 * same machine code).  Pattern of every *_sync wrapper:
 *
 *     int cmd, result;                        // sp+0x00, sp+0x04
 *     WaitSema(g_mc_lock_sema);               // MC lock, return ignored
 *     while (sceMcXxx(args) != 0)             // busy / not bound: yield, retry
 *         yield_low_priority();
 *     mc_sync_wait(&cmd, &result);            // spin until sceMcSync == 1
 *     <its own unlock stub>();                // signal_sema_helper(lock)
 *     return result;
 *
 * Each wrapper calls its own 2-instruction unlock copy placed right after it
 * (`j signal_sema_helper` with the lock id load in the delay slot).  The
 * names of the BOOT-matched stubs come from the bootstrap, whose wrappers
 * are a subset: the copies after the sceMcGetSlotMax and sceMcSetFileInfo
 * wrappers carry the bootstrap names mc_getinfo_sync_unlock and
 * mc_read_sync_unlock (identical code).
 * Callers: the game's memory-card layer (T501 mc_*, 0x0023E430..0x00240280).
 */
#include "engine/system/system_drivers_internal.h"

/* .sdata: the lock, -1 until mc_init_once creates it. */
int g_mc_lock_sema = -1;                    /* 0x003531A0 */

static void mc_close_sync_unlock(void);
static void mc_delete_sync_unlock(void);
static void mc_format_sync_unlock(void);
static void mc_getdir_sync_unlock(void);
static void mc_getentspace_sync_unlock(void);
static void mc_getinfo_sync_unlock2(void);
static void mc_getinfo_sync_unlock(void);
static void mc_init_once_unlock(void);
static void mc_mkdir_sync_unlock(void);
static void mc_open_sync_unlock(void);
static void mc_read_sync_unlock2(void);
static void mc_rename_sync_unlock(void);
static void mc_seek_sync_unlock(void);
static void mc_read_sync_unlock(void);
static void mc_unformat_sync_unlock(void);
static void mc_write_sync_unlock(void);

// FUNC 0x002670F0 mc_close_sync
/* bootstrap 0x0100F130.  Blocking sceMcClose(fd). */
int mc_close_sync(int fd)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(g_mc_lock_sema);
    while (sceMcClose(fd) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_close_sync_unlock();
    return result;
}

// FUNC 0x00267158 mc_close_sync_unlock
/* bootstrap 0x0100F198. */
static void mc_close_sync_unlock(void)
{
    signal_sema_helper(g_mc_lock_sema);
}

// FUNC 0x00267160 mc_delete_sync
int mc_delete_sync(int port, int slot, const char *name)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(g_mc_lock_sema);
    while (sceMcDelete(port, slot, name) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_delete_sync_unlock();
    return result;
}

// FUNC 0x002671E8 mc_delete_sync_unlock
static void mc_delete_sync_unlock(void)
{
    signal_sema_helper(g_mc_lock_sema);
}

// FUNC 0x002671F0 mc_format_sync
int mc_format_sync(int port, int slot)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(g_mc_lock_sema);
    while (sceMcFormat(port, slot) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_format_sync_unlock();
    return result;
}

// FUNC 0x00267268 mc_format_sync_unlock
static void mc_format_sync_unlock(void)
{
    signal_sema_helper(g_mc_lock_sema);
}

// FUNC 0x00267270 mc_getdir_sync
/* bootstrap 0x0100F1D0.  Blocking sceMcGetDir (6th argument in $t1).
 * Returns the number of entries written or a negative sceMcRes*. */
int mc_getdir_sync(int port, int slot, const char *name, unsigned int mode,
                   int maxent, sceMcTblGetDir *table)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(g_mc_lock_sema);
    while (sceMcGetDir(port, slot, name, mode, maxent, table) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_getdir_sync_unlock();
    return result;
}

// FUNC 0x00267328 mc_getdir_sync_unlock
/* bootstrap 0x0100F288. */
static void mc_getdir_sync_unlock(void)
{
    signal_sema_helper(g_mc_lock_sema);
}

// FUNC 0x00267330 mc_getentspace_sync
int mc_getentspace_sync(int port, int slot, const char *path)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(g_mc_lock_sema);
    while (sceMcGetEntSpace(port, slot, path) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_getentspace_sync_unlock();
    return result;
}

// FUNC 0x002673B8 mc_getentspace_sync_unlock
static void mc_getentspace_sync_unlock(void)
{
    signal_sema_helper(g_mc_lock_sema);
}

// FUNC 0x002673C0 mc_getinfo_sync
/* Blocking sceMcGetInfo (5th argument in $t0).  Not the bootstrap's
 * mc_getinfo_sync code. */
int mc_getinfo_sync(int port, int slot, int *type, int *freeSpace, int *format)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(g_mc_lock_sema);
    while (sceMcGetInfo(port, slot, type, freeSpace, format) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_getinfo_sync_unlock2();
    return result;
}

// FUNC 0x00267468 mc_getinfo_sync_unlock2
/* The unlock copy after mc_getinfo_sync (the bootstrap name
 * mc_getinfo_sync_unlock belongs to the copy at 0x002674D8). */
static void mc_getinfo_sync_unlock2(void)
{
    signal_sema_helper(g_mc_lock_sema);
}

// FUNC 0x00267470 mc_getslotmax_sync
/* sceMcGetSlotMax(port) is synchronous: retried with yields while it
 * returns a negative value; no mc_sync_wait.  Returns the slot count. */
int mc_getslotmax_sync(int port)
{
    int slots;

    WaitSema(g_mc_lock_sema);
    while ((slots = sceMcGetSlotMax(port)) < 0)
        yield_low_priority();
    mc_getinfo_sync_unlock();
    return slots;
}

// FUNC 0x002674D8 mc_getinfo_sync_unlock
/* bootstrap 0x0100F338 (same code); here the unlock copy of
 * mc_getslotmax_sync. */
static void mc_getinfo_sync_unlock(void)
{
    signal_sema_helper(g_mc_lock_sema);
}

// FUNC 0x002674E0 mc_init_once
/* bootstrap 0x0100F340.  If the MC lock does not exist yet: create it
 * (count 1), take it, retry sceMcInit() with yields until it returns 0, then
 * release it through the tail jump to mc_init_once_unlock.  Caller:
 * mc_manager_init (T501, 0x0023E9F8). */
void mc_init_once(void)
{
    if (g_mc_lock_sema < 0) {
        int sema = create_mutex_sema(1);
        g_mc_lock_sema = sema;
        WaitSema(sema);
        while (sceMcInit() != 0)
            yield_low_priority();
        mc_init_once_unlock();              /* tail jump */
    }
}

// FUNC 0x00267540 mc_init_once_unlock
/* bootstrap 0x0100F3A0.  Reached only by mc_init_once's tail jump. */
static void mc_init_once_unlock(void)
{
    signal_sema_helper(g_mc_lock_sema);
}

// FUNC 0x00267548 mc_mkdir_sync
int mc_mkdir_sync(int port, int slot, const char *name)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(g_mc_lock_sema);
    while (sceMcMkdir(port, slot, name) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_mkdir_sync_unlock();
    return result;
}

// FUNC 0x002675D0 mc_mkdir_sync_unlock
static void mc_mkdir_sync_unlock(void)
{
    signal_sema_helper(g_mc_lock_sema);
}

// FUNC 0x002675D8 mc_open_sync
/* Blocking sceMcOpen; returns the fd or a negative sceMcRes*. */
int mc_open_sync(int port, int slot, const char *name, int mode)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(g_mc_lock_sema);
    while (sceMcOpen(port, slot, name, mode) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_open_sync_unlock();
    return result;
}

// FUNC 0x00267670 mc_open_sync_unlock
static void mc_open_sync_unlock(void)
{
    signal_sema_helper(g_mc_lock_sema);
}

// FUNC 0x00267678 mc_read_sync
int mc_read_sync(int fd, void *buf, int size)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(g_mc_lock_sema);
    while (sceMcRead(fd, buf, size) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_read_sync_unlock2();
    return result;
}

// FUNC 0x00267700 mc_read_sync_unlock2
/* The unlock copy after mc_read_sync (the bootstrap name
 * mc_read_sync_unlock belongs to the copy at 0x002678E0). */
static void mc_read_sync_unlock2(void)
{
    signal_sema_helper(g_mc_lock_sema);
}

// FUNC 0x00267708 mc_rename_sync
int mc_rename_sync(int port, int slot, const char *org, const char *newname)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(g_mc_lock_sema);
    while (sceMcRename(port, slot, org, newname) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_rename_sync_unlock();
    return result;
}

// FUNC 0x002677A0 mc_rename_sync_unlock
static void mc_rename_sync_unlock(void)
{
    signal_sema_helper(g_mc_lock_sema);
}

// FUNC 0x002677A8 mc_seek_sync
int mc_seek_sync(int fd, int offset, int origin)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(g_mc_lock_sema);
    while (sceMcSeek(fd, offset, origin) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_seek_sync_unlock();
    return result;
}

// FUNC 0x00267830 mc_seek_sync_unlock
static void mc_seek_sync_unlock(void)
{
    signal_sema_helper(g_mc_lock_sema);
}

// FUNC 0x00267838 mc_setfileinfo_sync
/* Blocking sceMcSetFileInfo (5th argument in $t0). */
int mc_setfileinfo_sync(int port, int slot, const char *name,
                        const char *info, unsigned int valid)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(g_mc_lock_sema);
    while (sceMcSetFileInfo(port, slot, name, info, valid) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_read_sync_unlock();
    return result;
}

// FUNC 0x002678E0 mc_read_sync_unlock
/* bootstrap 0x0100F4D0 (same code); here the unlock copy of
 * mc_setfileinfo_sync. */
static void mc_read_sync_unlock(void)
{
    signal_sema_helper(g_mc_lock_sema);
}

// FUNC 0x002678E8 mc_sync_wait
/* bootstrap 0x0100F4D8.  sceMcSync(0 = wait, cmd, result), yield and retry
 * until it returns exactly 1 (sceMcExecFinish).  Quirk: with nothing
 * pending sceMcSync returns -1 and this would spin forever; the wrappers
 * only call it after an issued request. */
void mc_sync_wait(int *cmd, int *result)
{
    while (sceMcSync(MC_SYNC_WAIT, cmd, result) != sceMcExecFinish)
        yield_low_priority();
}

// FUNC 0x00267948 mc_unformat_sync
int mc_unformat_sync(int port, int slot)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(g_mc_lock_sema);
    while (sceMcUnformat(port, slot) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_unformat_sync_unlock();
    return result;
}

// FUNC 0x002679C0 mc_unformat_sync_unlock
static void mc_unformat_sync_unlock(void)
{
    signal_sema_helper(g_mc_lock_sema);
}

// FUNC 0x002679C8 mc_write_sync
int mc_write_sync(int fd, const void *buf, int size)
{
    int cmd;      /* sp+0x00 */
    int result;   /* sp+0x04 */

    WaitSema(g_mc_lock_sema);
    while (sceMcWrite(fd, buf, size) != 0)
        yield_low_priority();
    mc_sync_wait(&cmd, &result);
    mc_write_sync_unlock();
    return result;
}

// FUNC 0x00267A50 mc_write_sync_unlock
static void mc_write_sync_unlock(void)
{
    signal_sema_helper(g_mc_lock_sema);
}
