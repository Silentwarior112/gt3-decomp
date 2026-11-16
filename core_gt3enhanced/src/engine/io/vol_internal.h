/* vol_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u110,
 * 0x0023D690..0x00240AF0: translation units T499, T500, T510, T511, T501.
 *
 *   T499  font metrics of a Font (T496): character width / height of its
 *         current face (Font +0x244, a G001 PglFont) and the width / height
 *         of a string through G001's font_set / font_text_width / height.
 *         engine/text/T499_font_metrics.h
 *   T500  the gt3.vol table of contents ("RoFS"): decoding the stored TOC
 *         (bytes inverted, magic ~"RoFS"), path lookup ("/", "..", binary
 *         search of a directory's sorted entries).  engine/io/T500_vol_toc.h
 *   T510  VolDevice: a mounted volume (gt3.vol on the disc through ISO 9660,
 *         or a host: file), in a global list; reads the whole TOC into a
 *         64-byte aligned buffer.  engine/io/T510_vol_device.h
 *   T511  GZIP::InflateLoader: an InflateBase whose input is refilled from
 *         an IOP stream (pdipipe_read) into a fixed buffer.
 *         engine/io/T511_inflate_loader.h
 *   T501  the memory-card manager: one 0x1880-byte object (g_00352F60) with
 *         a worker thread, 2 ports x 4 slots of card state polled with
 *         sceMcGetSlotMax / sceMcGetInfo, blocking operations (create,
 *         write, read, delete, mkdir, getdir, format, rename, set file info,
 *         unformat) under a lock semaphore, and an asynchronous request
 *         queue of depth one (mc_request + the worker thread, completion
 *         callback and waiting threads).  engine/io/T501_memorycard_manager.h
 *
 * This header keeps what only u110 uses: the functions without callers
 * outside the unit, the manager's own structures and the unit's globals.
 * Offsets in comments are the original (egcs) ones.
 */
#ifndef U110_H
#define U110_H

#include "types.h"
#include "libc.h"
#include "cxxrt.h"                      /* __rtti_si */
#include "eekernel.h"                   /* WaitSema, DeleteSema, WakeupThread, SleepThread, GetThreadId */
#include "libmc.h"                      /* sceMcTblGetDir, sceMcRes* */
#include "sifdev.h"                     /* SCE_RDONLY .. SCE_TRUNC (sceMcOpen modes) */

#include "engine/pgl/G001_flare.h"            /* PglFont, font_set, font_text_width / height */
#include "engine/media/G002_mpeg_frameq.h"      /* intr_suspend, the semaphore / worker-thread helpers, ThreadCtx
                                           (held by value in McManager), mc_*_sync */
#include "engine/text/T496_font.h"             /* Font (face at +0x244) */
#include "engine/io/T505_host_files.h"       /* host_read_file, host_file_size */
#include "engine/pdi/T519_pdicdvd.h"          /* pdicdvd_read */
#include "engine/pdi/T523_pdipipe.h"          /* pdipipe_read */
#include "engine/io/T526_iso9660.h"          /* IsoDirRecord, iso9660_find */

#include "engine/text/T499_font_metrics.h"
#include "engine/io/T500_vol_toc.h"
#include "engine/io/T510_vol_device.h"
#include "engine/io/T511_inflate_loader.h"
#include "engine/io/T501_memorycard_manager.h"

/* ======================================================================== */
/* T499 - font metrics: the accessors nobody outside T499 calls             */
/* ======================================================================== */
u8  text_kanji_width(Font *f);          /* 0x0023D6D0 face->kanjiW (u8 +0x10), 0 without a face */
u8  text_kanji_height(Font *f);         /* 0x0023D6F0 face->kanjiH (u8 +0x11), 0 without a face */
f32 text_string_height(Font *f, const char *s); /* 0x0023D730 = font_measure_height(f, s) */

/* ======================================================================== */
/* T500 / T510 - the volume                                                 */
/* ======================================================================== */
/* (The TOC's words are read with T500's inline vol_rec_word(toc, off).) */
#define VOL_ROOT_DIR    0x14           /* the root directory record */
#define VOL_SECTOR_SHIFT 11             /* 0x800-byte sectors */

/* VolDevice::type */
enum {
    VOL_DISC_VOLUME = 0,                /* gt3.vol on the disc (an ISO 9660 file) */
    VOL_HOST_VOLUME = 2                 /* a host: volume file */
};

/* g_00352F50, the empty TOC (lookups that fail point here; T500 defines it,
 * .sdata gp-0x4AA0, reached with lui / addiu): engine/io/T500_vol_toc.h. */
extern VolDevice *g_00352F58;           /* 0x00352F58 gp-0x4A98 the list of the volumes (head; T510 defines it) */

/* T500 */
void    vol_toc_decrypt(VolToc *toc);                                       /* 0x0023D7E0 */
VolDir *vol_dir_root(VolDir *dir, VolToc *toc);                             /* 0x0023D890 */
VolDir *vol_dir_lookup(VolDir *out, const VolDir *dir, const char *path);   /* 0x0023D920 */
int     vol_name_cmp(const u8 *name, const u8 *nameEnd, const char *s);     /* 0x0023DAE0 */
int     vol_dir_find(const VolDir *dir, const char *begin, const char *end);/* 0x0023DB28 */
/* T510 */
VolDevice *vol_find_device(const char *name);                               /* 0x0023DCA8 */
u32  vol_toc_word(const VolToc *toc, int off);                              /* 0x0023E080 */
void vol_toc_set_root(VolToc *toc);                                         /* 0x0023E090 */
int  vol_toc_is_plain(const VolToc *toc);                                   /* 0x0023E0A0 */

/* ======================================================================== */
/* T501 - the memory-card manager                                           */
/* ======================================================================== */
/* A card is addressed by id = port * 4 + slot (port 0..1, slot 0..3). */
#define MC_PORTS 2
#define MC_SLOTS 4

/* Operation status codes MC_STATUS_*: T501_memorycard_manager.h. */
/* libmc results at or below this one count as "no card" (clamped to it). */
#define MC_RES_NO_CARD (-11)

/* McRequest::cmd: the operations of mc_thread_main. */
enum {
    MC_CMD_NONE = 0,
    MC_CMD_POLL = 1,                    /* one slot of one port (mc_ports_poll) */
    MC_CMD_CREATE_FILE = 2,
    MC_CMD_DELETE = 3,
    MC_CMD_CHECK_FILE = 4,
    MC_CMD_FORMAT = 5,
    MC_CMD_GET_DIR = 6,
    MC_CMD_GET_ENT_SPACE = 7,
    MC_CMD_MKDIR = 8,
    MC_CMD_READ_FILE = 9,
    MC_CMD_RENAME = 10,
    MC_CMD_SET_FILE_INFO = 11,
    MC_CMD_WRITE_FILE = 12,
    MC_CMD_UNFORMAT = 13
};

#define MC_DIR_POS_RESTART 0x7FFFFFFF   /* McSlot::dir_pos: the next listing starts over */
#define MC_GETDIR_MAX      0x20         /* entries per sceMcGetDir (the manager's table) */
#define MC_ICON_SYS_SIZE   0x3C4        /* sizeof(sceMcIconSys) */

struct McSlot {                         /* 0x6C */
    char name[0x40];                    /* 0x00 pattern of the last listing ("" = none) */
    s32  dir_pos;                       /* 0x40 entries of `name' already listed (MC_DIR_POS_RESTART: start over) */
    s32  dir_count;                     /* 0x44 entries returned by the last listing */
    s32  read_size;                     /* 0x48 bytes read by the last file read (-1 while reading) */
    s32  port;                          /* 0x4C */
    s32  slot;                          /* 0x50 */
    s32  need_info;                     /* 0x54 1: the next poll also fetches free / format */
    s32  type;                          /* 0x58 sceMcGetInfo type (sceMcType*) */
    s32  free;                          /* 0x5C free clusters */
    bool format;                        /* 0x60 formatted (bool, 4 bytes under egcs; GCC 14 pads to
                                           0x64: mc_slot_formatted returns it as is, u031) */
    s32  changed;                       /* 0x64 card changed (cleared when asked) */
    s32  status;                        /* 0x68 status of the last operation */
};
struct McPort {                         /* 0x1BC */
    s32    port;                        /* 0x0 */
    s32    nslots;                      /* 0x4 min(4, sceMcGetSlotMax) */
    s32    cur;                         /* 0x8 slot polled next */
    McSlot slots[MC_SLOTS];             /* 0xC */
};
struct McPorts {
    s32    cur;                         /* 0x0 port polled next */
    McPort port[MC_PORTS];              /* 0x4 */
};
typedef void (*McCallback)(void *arg, int status);
/* A queued request (cmd: MC_CMD_*): the operation's arguments. */
struct McRequest {                      /* 0x5C */
    s32        cmd;                     /* 0x00 */
    s32        id;                      /* 0x04 */
    char       path[0x40];              /* 0x08 */
    s32        ptr;                     /* 0x48 buf / out / newname / info (a pointer) */
    s32        offset;                  /* 0x4C file offset; get_dir: skip; set_file_info: valid */
    s32        size;                    /* 0x50 byte count; get_dir: entry count */
    McCallback cb;                      /* 0x54 */
    void      *cb_arg;                  /* 0x58 */
};
/* A thread waiting in mc_async_wait (on its stack). */
struct McWaiter {
    McWaiter *next;                     /* 0x0 */
    s32       tid;                      /* 0x4 */
    s32       result;                   /* 0x8 */
};
struct McManager {                      /* 0x1880, memalign(0x40) */
    sceMcTblGetDir dir[MC_GETDIR_MAX];  /* 0x0000 getdir buffer */
    u8         stack[0xC00];            /* 0x0800 worker thread stack */
    ThreadCtx  thread;                  /* 0x1400 */
    s32        req_sema;                /* 0x1408 signalled per request */
    s32        lock_sema;               /* 0x140C serialises the card operations */
    s32        quit;                    /* 0x1410 */
    McPorts    ports;                   /* 0x1414 */
    McRequest  req;                     /* 0x1790 the queued request */
    McRequest  cur;                     /* 0x17EC the request being run */
    s32        busy;                    /* 0x1848 a request is queued or running */
    s32        result;                  /* 0x184C status of the last request */
    McWaiter  *waiters;                 /* 0x1850 */
    u8         pad_1854[0x2C];
};
static_assert(sizeof(McSlot) == 0x6C, "McSlot");
static_assert(sizeof(McPort) == 0x1BC, "McPort");
static_assert(sizeof(McRequest) == 0x5C, "McRequest");
static_assert(offsetof(McManager, ports) == 0x1414, "McManager.ports");
static_assert(offsetof(McManager, req) == 0x1790, "McManager.req");
static_assert(offsetof(McManager, busy) == 0x1848, "McManager.busy");
static_assert(sizeof(McManager) == 0x1880, "McManager");

/* A save: a directory with up to three files and its icon.sys. */
struct McSaveFile {
    const char *name;                   /* 0x0 */
    const void *data;                   /* 0x4 */
    int         size;                   /* 0x8 */
};
struct McSaveSet {
    const char *dir;                    /* 0x00 directory (without '/') */
    const void *icon_sys;               /* 0x04 icon.sys image (MC_ICON_SYS_SIZE bytes) */
    McSaveFile *file0;                  /* 0x08 */
    McSaveFile *file1;                  /* 0x0C or NULL */
    McSaveFile *file2;                  /* 0x10 or NULL */
};

/* T501 defines them (.sdata). */
extern McManager *g_00352F60;           /* 0x00352F60 gp-0x4A90 the manager (NULL before mc_manager_init) */
extern s32 g_00352F64;                  /* 0x00352F64 gp-0x4A8C tick counter of mc_manager_tick */
extern s32 g_00352F68;                  /* 0x00352F68 gp-0x4A88 (=-1) id of the last mc_get_dir */

/* The slot of a card id.  An inline function in the original, not an
 * expression written in place: retail adds port[0]'s offset (0x1418) to the
 * port's (index * 0x1BC) and keeps the sum, where a plain expression folds it
 * into the field offset. */
inline McSlot *mc_slot_of(int id)
{
    return &g_00352F60->ports.port[id / MC_SLOTS].slots[id % MC_SLOTS];
}

void mc_slot_reset(McSlot *slot);                                           /* 0x0023E268 */
void mc_slot_set_result(McSlot *slot, int res, int status);                 /* 0x0023E2B8 */
void mc_slot_set_info(McSlot *slot, int res, int type, int free, int format,
                      int changed);                                         /* 0x0023E360 */
void mc_slot_poll_info(McSlot *slot);                                       /* 0x0023E430 */
int  mc_port_poll(McPort *port);                                            /* 0x0023E598 */
void mc_ports_poll(McPorts *ports);                                         /* 0x0023E670 */
void mc_thread_main(void *arg);                                             /* 0x0023E6C8 */
void mc_request(int cmd, int id, const char *path, int ptr, int offset, int size,
                McCallback cb, void *cbArg);                                /* 0x0023EBA0 */
int  mc_port_slot_count(int port);                                          /* 0x0023ECF0 */
int  mc_slot_valid(int id);                                                 /* 0x0023ED18 */
int  mc_status_from_result(int status, int res);                            /* 0x0023F1A8 */
int  mc_do_create_file(int id, const char *path, const void *buf, int size);/* 0x0023F208 */
int  mc_do_check_file(int id, const char *path);                            /* 0x0023F358 */
int  mc_do_mkdir(int id, const char *path);                                 /* 0x0023F460 */
int  mc_do_write_file(int id, const char *path, const void *buf, int offset,
                      int size);                                            /* 0x0023F540 */
int  mc_check_file(int id, const char *path);                               /* 0x0023F870 */
int  mc_get_ent_space(int id, const char *path, int *out);                  /* 0x0023FD08 */
int  mc_unformat(int id);                                                   /* 0x00240280 */
int  mc_save_files(int id, const McSaveSet *set);                           /* 0x00240400 */
void mc_lock(void);                                                         /* 0x002405B8 */
void mc_unlock(void);                                                       /* 0x002405D8 */
void mc_async_write_file(int id, const char *path, const void *buf, int size,
                         McCallback cb, void *cbArg);                       /* 0x002405F8 */
void mc_async_delete(int id, const char *path, McCallback cb, void *cbArg); /* 0x00240640 */
void mc_async_check_file(int id, const char *path, McCallback cb, void *cbArg); /* 0x00240680 */
void mc_async_format(int id, McCallback cb, void *cbArg);                   /* 0x002406C0 */
void mc_async_get_dir(int id, const char *path, McDirEntry *out, int skip,
                      u32 count, McCallback cb, void *cbArg);               /* 0x002406F8 */
void mc_async_get_dir0(int id, const char *path, McDirEntry *out,
                       McCallback cb, void *cbArg);                         /* 0x00240748 */
void mc_async_mkdir(int id, const char *path, McCallback cb, void *cbArg);  /* 0x00240788 */
void mc_async_read_file(int id, const char *path, void *buf, int offset, int size,
                        McCallback cb, void *cbArg);                        /* 0x002407C8 */
void mc_async_rename(int id, const char *path, const char *newname,
                     McCallback cb, void *cbArg);                           /* 0x00240818 */
void mc_async_set_file_info(int id, const char *path, void *info, unsigned int valid,
                            McCallback cb, void *cbArg);                    /* 0x00240858 */
void mc_async_unformat(int id, McCallback cb, void *cbArg);                 /* 0x002408A0 */
void mc_async_write_file_at(int id, const char *path, const void *buf, int offset,
                            int size, McCallback cb, void *cbArg);          /* 0x002408D8 */
int  mc_async_wait(void);                                                   /* 0x00240928 */

#endif /* U110_H */
