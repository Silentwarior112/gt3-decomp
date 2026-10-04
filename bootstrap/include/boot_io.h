/* boot_io.h - game-side I/O helper layer of the GT3 US bootstrap
 * (BOOTSTRAP_SCUS-97102), unit "io", .text 0x0100F130..0x0100F918
 * (minus the 128-bit strlen at 0x0100F538, which belongs to the libc unit).
 *
 *   src/mcard_io.cpp    blocking memory-card wrappers over Sony libmc
 *                       (issue request under a lock, retry+yield while libmc
 *                       is busy, then spin on sceMcSync until finished)
 *   src/thread_util.cpp semaphore creation, cooperative yield, unlock helper,
 *                       calibrated busy-wait
 *   src/hostfs_io.cpp   "host:" fileio wrappers over the SIF fileio client,
 *                       serialised by the pdistd global lock (0x01041E60)
 *
 * None of these functions has an EH / per-function metadata record in .data
 * (unlike pdistd::Allocator right after them), so they are most likely plain
 * C in the original; they are declared with C linkage here.
 */
#ifndef BOOT_IO_H
#define BOOT_IO_H

#include "types.h"
#include "libmc.h"      /* sceMcTblGetDir */

#ifdef __cplusplus
extern "C" {
#endif

/* ======================================================================== */
/* thread_util.cpp                                                          */
/* ======================================================================== */

/* CreateSema{maxCount 1, initCount}; yields and retries while CreateSema
 * returns -1; if it returns id 0 it recurses once more (id 0 is leaked).
 * Used for the MC lock (0x01041E38) and the pdistd global lock (0x01041E60). */
int  create_mutex_sema(int initCount);                   /* 0x0100F5D0 */

/* Drop to priority 0x7E, 100 x {busy_wait_delay(10); RotateThreadReadyQueue(0x7E)},
 * restore the original priority.  Lets IOP/RPC helper threads run while the
 * main thread polls.  Also called by boot code at 0x010001C8.               */
void yield_low_priority(void);                           /* 0x0100F640 */

/* Unlock helper: reads COP0 Status (value unused) and calls iSignalSema(sid)
 * (syscall -0x43, NOT SignalSema).  Returns iSignalSema's result.  Reached
 * only by tail jumps (MC unlock stubs, pdistd global_lock_release).         */
int  signal_sema_helper(int semaId);                     /* 0x0100F6B0 */

/* Spin 75*n + 2 passes of a 6-nop loop (64-bit counter; n is a 64-bit
 * parameter in the original, `long` under ee-gcc).                         */
void busy_wait_delay(s64 n);                             /* 0x0100F8D8 */

/* ======================================================================== */
/* mcard_io.cpp                                                             */
/* ======================================================================== */

/* Lock semaphore id of the MC wrappers, -1 = not created.
 * Defined (static) in mcard_io.cpp:  0x01041E38 (.sdata, init 0xFFFFFFFF). */

/* First call: create the MC lock, take it, retry sceMcInit() (yielding) until
 * it returns 0, release.  Later calls: no-op.  Called from
 * boot_iop_reboot_and_pad_init (0x01000880).  The mapping pass declared an
 * int return; the only caller ignores v0 (fast path: the sema id; first call:
 * iSignalSema's result).                                                   */
void mc_init_once(void);                                 /* 0x0100F340 */

/* If the MC lock exists: DeleteSema it and set it back to -1.  Does not call
 * any libmc teardown.  Called from shutdown_iop_services (0x010009D8).     */
void mc_shutdown(void);                                  /* 0x0100F1A0 */

/* Blocking wrappers.  Each: WaitSema(lock); while (sceMcXxx(...) != 0)
 * yield_low_priority(); mc_sync_wait(&cmd, &result); unlock; return result
 * (the mcserv result word of the finished command).                        */
int  mc_getinfo_sync(int port, int slot, int *type, int *free, int *format); /* 0x0100F290 */
int  mc_getdir_sync(int port, int slot, const char *name, unsigned int mode,
                    int maxent, sceMcTblGetDir *table);  /* 0x0100F1D0 */
int  mc_open_sync(int port, int slot, const char *name, int mode); /* 0x0100F3A8 */
int  mc_read_sync(int fd, void *buf, int size);          /* 0x0100F448 */
int  mc_close_sync(int fd);                              /* 0x0100F130 */

/* while (sceMcSync(0, cmd, result) != sceMcExecFinish) yield_low_priority(); */
void mc_sync_wait(int *cmd, int *result);                /* 0x0100F4D8 */

/* ======================================================================== */
/* hostfs_io.cpp                                                            */
/* ======================================================================== */

/* Out-record of host_get_file_size (8 bytes; caller host_file_size 0x01005B08
 * reads .size from sp+4, host_file_exists 0x01005B68 only the return value). */
typedef struct HostFileStat {
    int mode;   /* +0x00 always 0x11 = SCE_STM_R | SCE_STM_F (?)  */
    int size;   /* +0x04 sceLseek(fd, 0, SCE_SEEK_END) result      */
} HostFileStat;

/* Each wrapper: global_lock_acquire(); r = sceXxx(...); global_lock_release(); return r. */
int  host_open(const char *path, int flags);             /* 0x0100F6D0 */
int  host_lseek(int fd, int offset, int whence);         /* 0x0100F718 */
int  host_read(int fd, void *buf, int len);              /* 0x0100F770 */
int  host_write(int fd, const void *buf, int len);       /* 0x0100F7C8 */
int  host_close(int fd);                                 /* 0x0100F820 */

/* stat-like probe: open(path, SCE_RDONLY); < 0 -> return -1; else
 * out->size = lseek(fd, 0, SEEK_END), close, out->mode = 0x11, return 0.   */
int  host_get_file_size(const char *path, HostFileStat *out); /* 0x0100F858 */

#ifdef __cplusplus
}
#endif

#endif /* BOOT_IO_H */
