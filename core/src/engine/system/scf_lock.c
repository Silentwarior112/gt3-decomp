/* scf_lock.c - two libscf clock conversions serialised by a lock
 * (same pattern as the memory-card wrappers: each locked call has its own
 * unlock copy after it, reached by a tail jump).  Callers: scf_lock_init
 * from game_init (0x00102C00); the conversions from T519's
 * pdicdvd_time_to_clock_gmt / pdicdvd_time_to_clock_local.
 */
#include "engine/system/system_drivers_internal.h"

/* .sdata: the lock, -1 until scf_lock_init creates it. */
int g_scf_lock_sema = -1;                   /* 0x003531A4 */

static void scf_clock_to_gmt_unlock(void);
static void scf_clock_to_local_unlock(void);

// FUNC 0x00267A58 scf_lock_init
/* Creates the lock (count 1) if it does not exist yet. */
void scf_lock_init(void)
{
    if (g_scf_lock_sema < 0)
        g_scf_lock_sema = create_mutex_sema(1);
}

// FUNC 0x00267A80 scf_clock_to_gmt
/* libscf RTC (JST) -> GMT conversion in place, under the lock. */
void scf_clock_to_gmt(sceCdCLOCK *rtc)
{
    WaitSema(g_scf_lock_sema);
    sceScfGetGMTfromRTC(rtc);
    scf_clock_to_gmt_unlock();              /* tail jump */
}

// FUNC 0x00267AB0 scf_clock_to_gmt_unlock
static void scf_clock_to_gmt_unlock(void)
{
    signal_sema_helper(g_scf_lock_sema);
}

// FUNC 0x00267AB8 scf_clock_to_local
/* libscf RTC (JST) -> local time conversion in place, under the lock. */
void scf_clock_to_local(sceCdCLOCK *rtc)
{
    WaitSema(g_scf_lock_sema);
    sceScfGetLocalTimefromRTC(rtc);
    scf_clock_to_local_unlock();            /* tail jump */
}

// FUNC 0x00267AE8 scf_clock_to_local_unlock
static void scf_clock_to_local_unlock(void)
{
    signal_sema_helper(g_scf_lock_sema);
}
