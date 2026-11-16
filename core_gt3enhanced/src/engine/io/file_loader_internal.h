/* file_loader_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u111,
 * 0x00240AF0..0x00243850: translation units T502 .. T509, T512.
 *
 *   T502  file_resolve: a path -> its location (FileLoc) on the device the
 *         volume is mounted from (the gt3.vol TOC, plain ISO 9660 files,
 *         a host: volume file or loose host:data/ files).
 *   T503  the volume reader thread (vol_thread_object): one request at a
 *         time (file_request / file_req_wait / file_req_result), callers
 *         waiting on a list of FileWaiter records; .gz files are inflated
 *         on the fly (GZIP::InflateLoader); sound banks straight to SPU RAM;
 *         vol_init / vol_shutdown; the static ctor / dtor of
 *         vol_device_object.
 *   T504  the queued (callback) loads: file_req_alloc / file_req_run.
 *   T505  host: file helpers (same code as the bootstrap's).
 *   T506  image packs (.imgs, ? name): relocation, lookup by name.
 *   T507  the background loader object AsyncLoader (? name), over T508.
 *   T508  the loader thread (? name): load / exit commands, a 16-entry ring
 *         of deferred calls.
 *   T509  Serialize: Base, the registry of serialisable classes (ClassInfo),
 *         headers, checksums (GZIP::crc32) and the byte inversion.
 *   T512  the render context g_002F4230 (? name): screen mode, the GS
 *         buffers.
 *
 * The shared classes, functions and globals come from the canonical headers
 * (include/game/).  This header keeps what only u111 uses: the reader
 * thread's records, the queue entry, the loader ring, the unit-internal
 * functions and the unit's globals.
 *
 * Offsets in comments are the ORIGINAL ones (egcs layout).
 */
#ifndef U111_H
#define U111_H

#include "types.h"
#include "libc.h"
#include "eekernel.h"
#include "sifdev.h"                             /* SCE_RDONLY .., SCE_SEEK_*, SCE_STM_D */
/* No <new>: its inline bad_alloc::what() would put "bad_alloc" first in
 * every file's .rodata, and no u111 original has it (data pass). */

/* _gp with a size, declared before G000's unsized declaration (u001's
 * pending note): egcs then counts it as small data (-G8) and loads &_gp
 * with one `la' (GAS: lui / addiu) kept in source order, as retail's
 * loader_thread_start has it; unsized, it is split into %hi / %lo insns
 * scheduled with the other addresses. */
extern "C" char _gp[4];                         /* 0x003579F0 */
#include "runtime/startup/G000_collect2_ctors.h"           /* _gp */
#include "engine/pgl/G001_flare.h"                    /* Tex1Image, tex1_relocate, gfx_* */
#include "engine/media/G002_mpeg_frameq.h"              /* intr_suspend, semaphores, worker threads
                                                   (ThreadCtx), host: fileio, strlen */
#include "game/sound/T468_gtsound.h"                  /* GTSOUNDINSTRUMENT, SoundBankHeader */
#include "engine/io/T500_vol_toc.h"                  /* VolToc, VolDir, vol_toc_lookup */
#include "engine/io/T501_memorycard_manager.h"       /* mc_manager_init / shutdown, min_int */
#include "engine/io/T510_vol_device.h"               /* VolDevice, VolExtent, vol_mount */
#include "engine/io/T511_inflate_loader.h"           /* GZIP::InflateLoader; T537 GZIP::crc32 */
#include "engine/pdi/T523_pdipipe.h"                  /* pdipipe_open / read / read2 / close */
#include "engine/io/T525_sector_cache.h"             /* sector_cache_init */
#include "engine/io/T526_iso9660.h"                  /* IsoDirRecord, iso9660_find */

#include "engine/io/T502_file_resolve.h"
#include "engine/io/T503_vol_thread.h"
#include "engine/io/T504_file_queue.h"
#include "engine/io/T505_host_files.h"
#include "engine/io/T506_image_pack.h"
#include "engine/io/T507_async_loader.h"
#include "engine/io/T508_loader_thread.h"
#include "engine/io/T509_serialize.h"
#include "engine/gfx/T512_render_context.h"

/* What io_getstat (G002 0x00269F08) fills: it opens the file read-only,
 * size = lseek END, closes it, mode = 0x11 (SCE_STM_R | SCE_STM_F), always.
 * The callers reserve 16 bytes for it (the next local is 16-aligned).
 * IoStat itself: engine/media/G002_mpeg_frameq.h. */

/* ======================================================================== */
/* T502 / T503: the gt3.vol TOC and the reader thread                        */
/* ======================================================================== */

/* The TOC entry record VolTocEntry and vol_dir_entry: T500_vol_toc.h. */

/* VolDevice::type: VolDeviceType (VOL_DEV_*), engine/io/T510_vol_device.h. */

/* FileLoc::flags */
enum { FILE_LOC_FILE = 1, FILE_LOC_DIR = 2, FILE_LOC_PACKED = 4 };

#define VOL_SECTOR_SHIFT   11                   /* 0x800-byte sectors */
#define VOL_READ_CHUNK     0x40000              /* bytes per pdipipe_read, the pipe's buffer size */

/* Somebody waiting for the current request (on its stack, 0x14 bytes). */
struct FileWaiter {
    FileWaiter *next;                           /* 0x00 */
    int         status;                         /* 0x04 FILE_WAIT_* */
    void       *buf;                            /* 0x08 the data */
    int         size;                           /* 0x0C */
    int         thread;                         /* 0x10 the thread to wake, -1 none */
};
static_assert(sizeof(FileWaiter) == 0x14, "FileWaiter");
enum {
    FILE_WAIT_QUEUED    = 0,
    FILE_WAIT_READING   = 1,
    FILE_WAIT_CANCELLED = 2,
    FILE_WAIT_DONE      = 3
};

/* The request being served (copied by value by the thread, 0x28 bytes). */
struct FileRequest {
    VolDevice  *dev;                            /* 0x00 */
    const char *path;                           /* 0x04 */
    FileLoc     loc;                            /* 0x08 */
    int         allocated;                      /* 0x18 buf was allocated by file_request */
    void       *buf;                            /* 0x1C */
    int         size;                           /* 0x20 */
    int         unpack;                         /* 0x24 inflate a packed file */
};
static_assert(sizeof(FileRequest) == 0x28, "FileRequest");

/* vol_thread_object, 0x58 bytes. */
struct VolThread {
    ThreadCtx    worker;                        /* 0x00 */
    void        *stack;                         /* 0x08 0x8000 bytes */
    int          quit;                          /* 0x0C */
    int          sema;                          /* 0x10 signalled per request */
    int          pending;                       /* 0x14 a request is set */
    int          busy;                          /* 0x18 the thread owns it */
    int          cancel;                        /* 0x1C */
    FileWaiter  *waiters;                       /* 0x20 */
    FileRequest  req;                           /* 0x24 */
    int          handle;                        /* 0x4C the open pdipipe stream */
    void        *resultBuf;                     /* 0x50 */
    int          resultStatus;                  /* 0x54 */
};
static_assert(sizeof(VolThread) == 0x58, "VolThread");
/* T503 defines them (.data after vol_device_object / .sdata after its
 * strings: uninitialised C++ globals, written at the end of the file in
 * this declaration order); its sector cache vol_cache_buffer (0x004A0CB8)
 * is a static of T503 (.bss). */
extern VolThread vol_thread_object;             /* 0x002F33E0 */
extern int  vol_init_guard;                     /* 0x00352F94 gp-0x4A5C */

/* T503, not in the canonical header (only u111 calls them) */
int   vol_waiter_listed(FileWaiter *w);         /* 0x00240FA0 */
void  vol_set_waiters_status(int status);       /* 0x00241018 */
void  vol_open_name(VolDevice *dev, const char **path, int *lsn, int unpack,
                    char *nameBuf, int nameBufSize);    /* 0x00241078 */
void  vol_add_waiter(FileWaiter *w, int thread);        /* 0x002414A8 */
int   file_request(VolDevice *dev, const char *path, void *buf, int size,
                   int unpack, FileWaiter *w);  /* 0x00241530 */
void  file_req_wait(void);                      /* 0x00241690 */
void *file_req_result(FileWaiter *w);           /* 0x002416C0 */
void  file_req_cancel(void);                    /* 0x00241778 */
int   file_read_async(const char *path, void *buf, int size, int unpack,
                      VolDevice *dev);          /* 0x00241858 */
void  file_req_wait_idle(void);                 /* 0x002418B0 */
int   F_00241B98(int a, int b);                 /* 0x00241B98 T503's unreferenced min(a, b) */

/* ======================================================================== */
/* T504: the queued loads                                                   */
/* ======================================================================== */
/* FileQueueEntry::kind */
enum { FILE_QUEUE_FILE = 0, FILE_QUEUE_SOUND_BANK = 1 };

/* One queued load, 0x20 bytes, in g_004A3E38[]. */
struct FileQueueEntry {
    FileQueueEntry *next;                       /* 0x00 sorted by lsn */
    int         kind;                           /* 0x04 FILE_QUEUE_* */
    int         lsn;                            /* 0x08 */
    const char *path;                           /* 0x0C */
    void       *buf;                            /* 0x10 */
    int         size;                           /* 0x14 */
    FileDoneFn  done;                           /* 0x18 */
    void       *arg;                            /* 0x1C (sound bank: the GTSOUNDINSTRUMENT) */
};
static_assert(sizeof(FileQueueEntry) == 0x20, "FileQueueEntry");
/* Volatile (changed with interrupts off, read by the loads' callers):
 * retail keeps the count store out of the intr_resume branch's delay slot
 * and file_req_run's head store, count load and count store in order.
 * T504 defines them (.sdata, in this declaration order); its entries
 * g_004A3E38 (0x004A3E38) are a static of T504 (.bss). */
extern volatile int    g_00352F98;              /* gp-0x4A58 entries in use */
extern FileQueueEntry *volatile g_00352F9C;     /* gp-0x4A54 the queue */

/* ======================================================================== */
/* T505: host: files                                                        */
/* ======================================================================== */
bool  host_file_exists(const char *path);       /* 0x00242000 (no callers) */

/* ======================================================================== */
/* T506: image packs                                                        */
/* ======================================================================== */
void  image_pack_free(ImagePack *pack);                     /* 0x002420A0 */
Tex1Image *image_pack_nth(ImagePack *pack, int index);      /* 0x002420C0 */
void  image_pack_entry_relocate(ImagePackEntry *entry, ImagePack *pack); /* 0x002420F0 */
void  image_pack_relocate(ImagePack *pack);                 /* 0x00242128 */
ImagePackEntry *image_pack_bsearch(ImagePackEntry *entries, int count,
                                   const char *name);       /* 0x00242200 */

/* ======================================================================== */
/* T507: the background loader object                                       */
/* ======================================================================== */
/* AsyncLoader::state */
enum {
    ASYNC_IDLE       = 0,
    ASYNC_WAITING    = 1,
    ASYNC_LOADING    = 2,
    ASYNC_CANCELLING = 3
};
#define ASYNC_REQUEST_DELAY 3                   /* polls before a request goes to the thread */
extern AsyncLoader *g_00352FA8;                 /* gp-0x4A48 the loader (T507 .sdata, defined there) */

/* ======================================================================== */
/* T508: the loader thread                                                  */
/* ======================================================================== */
/* The command word g_00352FB0 */
enum { LOADER_CMD_NONE = 0, LOADER_CMD_LOAD = 1, LOADER_CMD_EXIT = 2 };

/* A deferred call, 12 bytes.  The members as the ring's code shows them:
 *  - arg2 is not an int here (loader_defer's int, stored as a pointer):
 *    retail's push and pop keep the ring index in a register over the entry
 *    stores, which an int store (int's alias set) would forbid;
 *  - memberwise copy constructor and assignment (word copies: retail copies
 *    an entry with three lw / sw, egcs's implicit copy is a block move with
 *    ldl / ldr) and an inline clear() (pop clears the entry through its own
 *    copy of the address). */
struct LoaderCallback {
    LoaderFn func;                              /* 0x0 */
    void    *arg;                               /* 0x4 */
    void    *arg2;                              /* 0x8 the int argument */
    LoaderCallback() : func(0), arg(0), arg2(0) {}
    LoaderCallback(const LoaderCallback &c) : func(c.func), arg(c.arg), arg2(c.arg2) {}
    LoaderCallback &operator=(const LoaderCallback &c)
    {
        func = c.func;
        arg = c.arg;
        arg2 = c.arg2;
        return *this;
    }
    void clear()
    {
        func = 0;
        arg = 0;
        arg2 = 0;
    }
};
static_assert(sizeof(LoaderCallback) == 0xC, "LoaderCallback");
#define LOADER_RING_SIZE 16
/* A ring of 16 of them (g_002F3C30). */
struct LoaderRing {
    LoaderCallback e[LOADER_RING_SIZE];         /* 0x00 */
    int            count;                       /* 0xC0 */
    int            rd;                          /* 0xC4 */
    int            wr;                          /* 0xC8 */
};
static_assert(sizeof(LoaderRing) == 0xCC, "LoaderRing");
/* T508 defines them (.data after the ring / .sdata, in this declaration
 * order); the thread's stack g_004A4240 (0x004A4240) is a static of T508
 * (.bss). */
extern LoaderRing g_002F3C30;                   /* 0x002F3C30 (T508 .data, defined there) */
extern char  g_002F3D00[];                      /* 0x002F3D00 the path to load (0x40 bytes) */
extern void *g_00352FAC;                        /* gp-0x4A44 the buffer, then the data */
extern int   g_00352FB0;                        /* gp-0x4A40 the command (LOADER_CMD_*) */
extern int   g_00352FB4;                        /* gp-0x4A3C the thread is running */
extern int   g_00352FB8;                        /* gp-0x4A38 its thread id */

void  loader_thread_main(void *arg);            /* 0x00242688 */
int   loader_request(const char *path, void *buf);  /* 0x00242840 */
int   loader_cancel(void);                      /* 0x002428C0 */
void *loader_result(void);                      /* 0x002428E0 */
int   loader_idle(void);                        /* 0x002428E8 */
void  loader_run_deferred(void);                /* 0x00242968 */
void  loader_ring_clear(LoaderRing *ring);      /* 0x002429E8 */
void  loader_ring_push(LoaderRing *ring, const LoaderCallback *cb);    /* 0x00242A28 */
LoaderCallback loader_ring_pop(LoaderRing *ring);   /* 0x00242A78 (returned through a hidden pointer) */

/* ======================================================================== */
/* T509: Serialize                                                          */
/* ======================================================================== */
extern Serialize::ClassInfo *g_00352FC0;        /* gp-0x4A30 the registry (T509 .sdata, defined there) */
extern Serialize::ClassInfo  g_002F3F18;        /* 0x002F3F18 "Base" (T509 .data, defined there) */
extern Serialize::Factory    g_00352FC8;        /* gp-0x4A28 Base's factory (T509 .sdata, defined there) */
Serialize::ClassInfo *serialize_find_class(const char *name);  /* 0x00242B38 */

#endif /* U111_H */
