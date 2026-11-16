/* memorycard_file_manager_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u025,
 * 0x00147870..0x001499B8, second pass.
 *
 * Translation unit T109 (whole): gt2k_menu::MemorycardFileManager, the files
 * of the memory card in one port behind the save / load screens, and its
 * interface MemorycardFileManagerBase (game/menu/memorycard/T109_memorycard_file_manager.h).
 * F_0014AE00 (T068) builds two of them (ports 0 and 1) and gives each its
 * port (setPort).  A worker thread (threadMain) watches the card, lists its
 * directory through the T110 cache, keeps the order of the entries (sorted /
 * filtered by setSort, setFilter, setEffectiveFilter and the select* setters) and
 * reads the files' information, size and icon in the order of their distance
 * to the visible window (select, screenTop, screenSize); the menus use the
 * virtual interface.
 *
 * Every class is defined in a canonical header; T109_memorycard_file_manager.cpp
 * includes the headers itself (their order gives retail's type names, the
 * data pass); this header has the unit's constants and the type_info storage
 * of MemorycardFileManager. */
#ifndef U025_H
#define U025_H

#include "types.h"
#include "game/menu/memorycard/T109_memorycard_file_manager.h"  /* gt2k_menu::MemorycardFileOrder, McDateTime
                                               (mcfm_set_date_key) */

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */
/* The members' sizes (the constructor). */
enum {
    MCFM_DIR_MAX_ENTRIES  = 0x200,          /* dirCache: entries it can hold */
    MCFM_DIR_PAGE_SIZE    = 0x20,           /* dirCache: entries per page */
    MCFM_FILE_CELLS       = 200,            /* fileList: files loaded at once */
    MCFM_SCREEN_SIZE      = 16              /* screenSize until the menu sets it */
};

/* The worker thread (createThread). */
enum {
    MCFM_THREAD_PRIORITY   = 0x10,
    MCFM_THREAD_STACK_SIZE = 0x8000         /* of threadStack, from its first 16-byte boundary */
};

/* MemorycardFileManager::threadState. */
enum McfmThreadState {
    MCFM_STATE_RUN     = 0,                 /* process() every pass */
    MCFM_STATE_PAUSED  = 2,                 /* pause() */
    MCFM_STATE_STOPPED = 3,                 /* stop(), or an error */
    MCFM_STATE_EXIT    = 4                  /* deleteThread: back to 0, then ExitDeleteThread */
};

/* The results of the thread's read steps (buildIndex, the read* functions);
 * process() sets the error flag for anything but MCFM_STEP_BUSY, MCFM_STEP_DONE
 * and 2 (which none of them returns). */
enum McfmStepResult {
    MCFM_STEP_BUSY     = 0,                 /* did one step (a read, or built order[]) */
    MCFM_STEP_DONE     = 1,                 /* nothing left to do there */
    MCFM_STEP_NO_ENTRY = 3,                 /* the directory entry cannot be listed */
    MCFM_STEP_NO_CELL  = 5                  /* no cell left for the file */
};

/* Failed reads of a file before it becomes a MemorycardDamageFile
 * (MemorycardFile::retry(), inline here: readCount++ < 5). */
#define MCFM_READ_RETRIES   5

/* MemorycardFile::sizeKB: the size could not be read (isSizeError /
 * setSizeError, inline here). */
#define MCFM_SIZE_ERROR     (-2)

/* T501 mc_slot_status MC_STATUS_NO_SLOT / MC_STATUS_NO_ENTRY: the last
 * operation found no such slot / no entry (the file is gone): the file is
 * replaced by a damaged one at once. */

/* The category SORT_BESTLAP2 never matches (T108 GameThumbnail::
 * category 0x11: the best lap file holds a top speed instead of a time). */
#define MCFM_CATEGORY_TOP_SPEED  0x11

/* The modification date of a directory entry as one number (one ld): the
 * bytes resv, sec, min, hour, day, month, year from the low end up, so a
 * later date is a larger number.  buildSortedIndex sorts a run of equal keys
 * by its complement (newest first).  A helper taking the line by reference
 * (? the original's form): the store then goes through the line's address
 * as retail's ($s0 = this + k * 16, `sd 0x10($s0)' beside the id's
 * `lw 0x8($s0)'); `order[k].key = ...' reduces a second address. */
static inline void mcfm_set_date_key(gt2k_menu::MemorycardFileOrder &line,
                                     const gt2k_menu::McDateTime &date)
{
    line.key = ~*(const u64 *)&date;
}

/* ======================================================================== */
/* RTTI                                                                     */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC                        /* egcs generates the __tf functions itself */
extern "C" {
/* The __si_type_info of MemorycardFileManager (3 words, filled by its __tf
 * function).  MemorycardFileManagerBase's is in the canonical header. */
extern u32 __tiQ29gt2k_menu21MemorycardFileManager[3];         /* 0x004B1970 (.bss) */
}
#endif /* !BOOT_OLD_GCC */

#endif /* U025_H */
