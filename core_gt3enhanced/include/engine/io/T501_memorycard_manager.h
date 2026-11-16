/* T501_memorycard_manager.h - the memory-card manager (translation unit T501,
 * unit u110), 0x0023E268..0x00240AF0.
 *
 * One manager object (0x00352F60, gp-0x4A90: 0x1880 bytes, memalign(0x40), made
 * by mc_manager_init) with a worker thread polls 2 ports x 4 slots
 * (sceMcGetSlotMax / sceMcGetInfo) and keeps per card its type, free
 * clusters, formatted flag, a "changed" flag and the status of the last
 * operation.  The operations block under a lock semaphore (the libmc
 * wrappers of G002); mc_async_* (not used outside T501) queue one request
 * for the worker thread.
 *
 * A card is addressed by id = port * 4 + slot (port 0..1, slot 0..3).
 * The operations return (and mc_slot_status gives) an MC_STATUS_* code. */
#ifndef GAME_T501_MEMORYCARD_MANAGER_H
#define GAME_T501_MEMORYCARD_MANAGER_H

#include "game/game.h"

namespace gt2k_menu {

/* sceMcStDateTime as the memory card code keeps it: 8-aligned (one ld / sd
 * copies it, and MemorycardDir::entry (T108) is at +0x08 of the dir that
 * MemorycardFile holds at +0x10). */
struct McDateTime {
    u8  resv;                                   /* 0x0 */
    u8  sec;                                    /* 0x1 */
    u8  min;                                    /* 0x2 */
    u8  hour;                                   /* 0x3 */
    u8  day;                                    /* 0x4 */
    u8  month;                                  /* 0x5 */
    u16 year;                                   /* 0x6 */
} __attribute__((aligned(8)));
EGCS_SIZE(gt2k_menu::McDateTime, 0x8);
} /* namespace gt2k_menu */

/* A directory entry as mc_get_dir hands it out (each sceMcTblGetDir
 * converted): 0x40 bytes.  Also u023 / u027 / u030 / u032's
 * gt2k_menu::McFileEntry (a typedef in T108) and u025 / u028's
 * gt2k_menu::McDirEntry (T110). */
struct McDirEntry {
    gt2k_menu::McDateTime create;               /* 0x00 */
    gt2k_menu::McDateTime modify;               /* 0x08 (u025 compares it as one u64) */
    u32  size;                                  /* 0x10 bytes */
    u16  attr;                                  /* 0x14 sceMcFileAttr*: 0x1000 PS1 save, 8 copy prohibited */
    u16  unk_16;                                /* 0x16 */
    u32  pdaAplNo;                              /* 0x18 */
    char name[0x24];                            /* 0x1C */
};
EGCS_SIZE(McDirEntry, 0x40);

/* Operation status codes (McSlot::status, the operations' results): one of
 * the first four, or the step that failed. */
enum {
    MC_STATUS_OK           = 0,
    MC_STATUS_NO_SLOT      = 1,                 /* no such slot (also the result before any step) */
    MC_STATUS_NO_ENTRY     = 2,                 /* sceMcResNoEntry: the file is not on the card */
    MC_STATUS_CARD_CHANGED = 3,                 /* sceMcResChangedCard / NoFormat / <= -11 */
    MC_STATUS_ERR_CLOSE    = 4,
    MC_STATUS_ERR_CREATE   = 5,                 /* the open that creates the file */
    MC_STATUS_ERR_DELETE   = 6,
    MC_STATUS_ERR_FORMAT   = 7,                 /* also rename and set file info */
    MC_STATUS_ERR_GETDIR   = 8,
    MC_STATUS_ERR_MKDIR    = 9,
    MC_STATUS_ERR_OPEN     = 0xA,
    MC_STATUS_ERR_READ     = 0xB,
    MC_STATUS_ERR_SEEK     = 0xD,
    MC_STATUS_ERR_UNFORMAT = 0xE,
    MC_STATUS_ERR_WRITE    = 0xF
};

/* Once (from vol_init): mc_init_once, the manager, its semaphores, the
 * 2 x 4 slots (8 polls) and the worker thread. */
void mc_manager_init(void);                     /* 0x0023E9F8 */
void mc_manager_shutdown(void);                 /* 0x0023EB48 stops the thread, frees the manager */
void mc_manager_tick(void);                     /* 0x0023EC88 every frame: a poll every 4th call while idle */

/* Slot state (interrupts off while read). */
int  mc_slot_refresh(int id);                   /* 0x0023EDA0 polls now (locked); returns and clears changed (0: bad id) */
int  mc_slot_take_changed(int id);              /* 0x0023EE30 returns and clears changed (no id check) */
int  mc_slot_status(int id);                    /* 0x0023EEA8 status of the last operation (1: bad id) */
int  mc_slot_type(int id);                      /* 0x0023EF40 sceMcGetInfo type */
int  mc_slot_free(int id);                      /* 0x0023EFD8 free clusters */
bool mc_slot_formatted(int id);                 /* 0x0023F070 formatted (bool: T075 stores it as is
                                                   into a bool, u031) */
int  mc_slot_take_status(int id);               /* 0x0023F108 returns and clears the status (1: bad id) */
int  mc_slot_dir_count(int id);                 /* 0x002409B0 entries returned by the last mc_get_dir */
int  mc_slot_read_size(int id);                 /* 0x00240A48 bytes read by the last mc_read_file (-1 while reading) */

/* Blocking operations: the status code. */
int  mc_create_file(int id, const char *path, const void *buf, int size);          /* 0x0023F700 */
int  mc_delete(int id, const char *path);                                           /* 0x0023F778 */
int  mc_format(int id);                                                             /* 0x0023F8C8 */
/* Lists path (a sceMcGetDir pattern) into out[0 .. count) from entry skip
 * on (continuing the previous listing of the same id and pattern when it can);
 * the entries not returned are zeroed. */
int  mc_get_dir(int id, const char *path, struct McDirEntry *out, int skip,
                u32 count);                                                         /* 0x0023F9C0 */
int  mc_mkdir(int id, const char *path);                                            /* 0x0023FE28 */
int  mc_read_file(int id, const char *path, void *buf, int offset, int size);      /* 0x0023FE80 */
int  mc_rename(int id, const char *path, const char *newname);                      /* 0x00240050 */
int  mc_set_file_info(int id, const char *path, void *info, unsigned int valid);   /* 0x00240160 sceMcSetFileInfo */
int  mc_write_file(int id, const char *path, const void *buf, int offset, int size); /* 0x00240378 */

/* min(a, b), signed: an out-of-line copy of an inline helper; T503's
 * vol_thread_main calls this one (T503 has an unreferenced copy of its own,
 * 0x00241B98). */
int  min_int(int a, int b);                     /* 0x00240AE0 */

#endif /* GAME_T501_MEMORYCARD_MANAGER_H */
