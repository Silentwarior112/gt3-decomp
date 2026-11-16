/* T109_memorycard_file_manager.h - gt2k_menu::MemorycardFileManagerBase /
 * MemorycardFileManager, the files of the memory card in one port
 * (translation unit T109, unit u025), 0x00147870..0x001499B8.
 *
 * MemorycardControl (T068) holds two managers (ports 0 and 1) and the
 * demo-replay manager of T079; the menus (T069..T076) use the virtual
 * interface of MemorycardFileManagerBase only.  A worker thread (threadMain)
 * watches the card, lists its directory through the T110 cache, keeps the
 * order of the entries (sorted / filtered by setSort, setFilter,
 * setEffectiveFilter and the select* setters) and reads the files' information,
 * size and icon in the order of their distance to the visible window
 * (select, screenTop, screenSize).
 *
 * Slot names.  index.txt's names of the base's slots come from another build
 * and are two slots off from slot 12 on; the bodies show it (slot 13 tests
 * the card-changed flag, 14 stores the next select, 17 / 18 store 64-bit
 * codes, 21 starts the thread, ...), and slot 30 is getXIconNum (the Lupo 2
 * name of T079's copy 0x0015C848; the menus use it as icons per row).  So
 * the owner's names: index.txt's moved down by two (slot 12 clearChangeCard
 * .. slot 28 setFilter), slot 29 setEffectiveFilter (Lupo 2's name: its
 * .gnu.linkonce catalogue has the base's slots in order, and its slot 31,
 * between setFilter and getXIconNum, is setEffectiveFilter(bool); Lupo 2's
 * slots 12 / 13 removeFileCache / addDeletedEntry are not in this build, so
 * it is this slot 29, and its argument is a bool; the first pass called it
 * setMatchOnly).  u026 / u027 / u028
 * named slots 29 / 30 virtual_29 / virtual_30 and slot 27 / 28 setSort(int)
 * / setFilter(int).  isConnect returns bool (as u026 / u027 / u030 had it:
 * T068's name_entry_abort 0x0014A0F8 negates it with xori 1, which egcs does
 * for a bool only; egcs gives T109 the same code either way).
 * updateFileId / updateFileIdForDelete return int (u026 / u027 / u030 had
 * void): the bodies return 0 / 1, although no caller reads it.
 * updateFileId takes no argument (Lupo 2's bool is not passed here). */
#ifndef GAME_T109_MEMORYCARD_FILE_MANAGER_H
#define GAME_T109_MEMORYCARD_FILE_MANAGER_H

#include "game/game.h"
#include "game/menu/memorycard/T108_memorycard_file.h"      /* MemorycardFileList, MemorycardDummyFile,
                                               MemorycardDeletedFile (fileList, dummyFile,
                                               deletedFile); MemorycardFile */
#include "game/menu/memorycard/T110_memorycard_dir_cache.h" /* gt2k_menu::MemorycardDirCache (dirCache) */

namespace gt2k_menu {

/* vtables 0x0031C498 (T109) and 0x003207D0 (T079): two identical copies,
 * 31 slots ([2] / [3] __pure_virtual).  typeinfo
 * __tfQ29gt2k_menu25MemorycardFileManagerBase 0x00149978 (__user_type_info
 * 0x00353350 gp-0x46A0).  No data, vptr +0x00; size 4.
 *
 * No key function: every member is inline in the original (Lupo 2 has each
 * in a .gnu.linkonce.t section).  Both T109 and T079
 * (MemorycardDemoFileManager) emit the vtable and out-of-line copies of the
 * members (T109's 0x00149868..0x00149948 in reverse slot order, the ones the
 * vtables use; T079's 0x0015C848..0x0015C928, unreferenced).  The defaults
 * are those of a manager without a card. */
class MemorycardFileManagerBase {
public:
    /* setFilter: the search pattern of the listing (? names). */
    enum FILTER {
        FILTER_ALL     = 0,                 /* every name, "/" "*" (1 and 6.. too) */
        FILTER_GT3     = 2,                 /* "/BASCUS-97102*" */
        FILTER_REPLAY  = 3,                 /* "/BASCUS-97102R*" */
        FILTER_BESTLAP = 4,                 /* "/BASCUS-97102B*" */
        FILTER_SETTING = 5,                 /* "/BASCUS-97102S*" */
        FILTER_INIT    = 6                  /* the constructor's first value */
    };
    /* setSort: 7..12 sort the list (the matching GT3 files first, other
     * names last, each group newest first); the others keep the directory
     * order (? names). */
    enum SORT {
        SORT_NONE      = 0,
        SORT_GAME      = 7,                 /* 'G' game data after the rest */
        SORT_REPLAY    = 8,                 /* 'R' / 'B' replays */
        SORT_BESTLAP   = 9,                 /* 'B' of the selected course / category / sub-category */
        SORT_SETTING   = 10,                /* 'S' of the selected car */
        SORT_DATE      = 11,                /* no matching, dates only */
        SORT_BESTLAP2  = 12,                /* as 9; category 0x11 never matches */
        SORT_INIT      = 13                 /* the constructor's first value */
    };

    virtual ~MemorycardFileManagerBase() {}                     /* [ 0] +0x0C 0x00149948 */
    virtual int  getPort() { return 0; }                        /* [ 1] +0x14 0x00149940 T501 card id */
    virtual int  getFileCount() = 0;                            /* [ 2] +0x1C < 0 while the list is
                                                                   not read */
    virtual MemorycardFile *getFile(int i) = 0;                 /* [ 3] +0x24 NULL out of range */
    virtual bool isConnect() { return true; }                   /* [ 4] +0x2C 0x00149938 bool: T068's
                                                                   0x0014A0F8 negates it with xori 1 */
    virtual int  isFormat() { return 1; }                       /* [ 5] +0x34 0x00149930 */
    virtual int  isError() { return 0; }                        /* [ 6] +0x3C 0x00149928 */
    virtual int  isReading() { return 0; }                      /* [ 7] +0x44 0x00149920 */
    virtual void clear() {}                                     /* [ 8] +0x4C 0x00149918 */
    virtual void clearError() {}                                /* [ 9] +0x54 0x00149910 */
    /* No argument in this build: its callers (T074, 0x001554C8 / 0x00155B58
     * / 0x00156820) leave $a1 as it is (Lupo 2: updateFileId(bool)). */
    virtual int  updateFileId() { return 1; }                   /* [10] +0x5C 0x00149908 lists the card
                                                                   again */
    virtual int  updateFileIdForDelete() { return 1; }          /* [11] +0x64 0x00149900 */
    virtual void clearChangeCard() {}                           /* [12] +0x6C 0x001498F8 */
    virtual int  isChangeCard() { return 0; }                   /* [13] +0x74 0x001498F0 */
    virtual void setSelect(int i) {}                            /* [14] +0x7C 0x001498E8 */
    virtual void setScreenTop(int i) {}                         /* [15] +0x84 0x001498E0 */
    virtual void setScreenSize(int n) {}                        /* [16] +0x8C 0x001498D8 */
    virtual void setSelectCourse(u64 course) {}                 /* [17] +0x94 0x001498D0 */
    virtual void setSelectCar(u64 car) {}                       /* [18] +0x9C 0x001498C8 */
    virtual void setSelectCategory(int c) {}                    /* [19] +0xA4 0x001498C0 */
    virtual void setSelectSubCategory(int c) {}                 /* [20] +0xAC 0x001498B8 */
    virtual void startThread() {}                               /* [21] +0xB4 0x001498B0 */
    virtual void endThread() {}                                 /* [22] +0xBC 0x001498A8 */
    virtual int  isThreadActive() { return 0; }                 /* [23] +0xC4 0x001498A0 */
    virtual int  pause() { return 1; }                          /* [24] +0xCC 0x00149898 requests it;
                                                                   nonzero once paused */
    virtual int  restart() { return 1; }                        /* [25] +0xD4 0x00149890 requests it;
                                                                   nonzero once running */
    virtual void releaseIcon(bool b) {}                         /* [26] +0xDC 0x00149888 */
    virtual void setSort(SORT s) {}                             /* [27] +0xE4 0x00149880 */
    virtual void setFilter(FILTER f) {}                         /* [28] +0xEC 0x00149878 */
    virtual void setEffectiveFilter(bool on) {}                 /* [29] +0xF4 0x00149870 */
    virtual int  getXIconNum() { return 8; }                    /* [30] +0xFC 0x00149868 icons per row */
};
EGCS_SIZE(gt2k_menu::MemorycardFileManagerBase, 4);

/* One line of the manager's list: the directory entry shown there. */
struct MemorycardFileOrder {
    int id;                                 /* 0x00 directory entry number; < 0: deleted */
    int pad_04;                             /* 0x04 */
    union {
        u64 key;                            /* 0x08 sort key (buildSortedIndex, sortIndex) */
        int oldId;                          /* 0x08 updateFileIdForDelete: the id before the update */
    };
};
EGCS_SIZE(gt2k_menu::MemorycardFileOrder, 0x10);

/* vtable 0x0031C398 (31 slots).  typeinfo
 * __tfQ29gt2k_menu21MemorycardFileManager 0x00149818 (si, base
 * MemorycardFileManagerBase; storage 0x004B1970).  vptr +0x00.  Size 0x17BE8
 * (the stride of MemorycardControl::files, T068).
 *
 * The twelve one-line overrides 0x00149758..0x00149808 are inline: they sit
 * at the end of T109 before the typeinfo functions, where egcs emits the
 * inline members of a class whose vtable it emits, in reverse declaration
 * order, and Lupo 2 has the same twelve in .gnu.linkonce.t sections.  Their
 * declaration order below (not the slot order) is the original one: Lupo 2's
 * address order, and with it egcs emits them in the order of the binary
 * (getXIconNum 0x00149758 .. getPort 0x00149808). */
class MemorycardFileManager : public MemorycardFileManagerBase {
public:
    /* dirCache(0x200, 0x20), fileList(200); then setFilter(FILTER_ALL),
     * setSort(SORT_NONE), setupScoreFunc() (direct calls). */
    MemorycardFileManager();                            /* 0x00147870 */
    virtual ~MemorycardFileManager();                   /* [ 0] 0x00147A28 deleteThread() if active */

    /* inline (see above) */
    virtual int  getPort() { return port; }             /* [ 1] 0x00149808 */
    /* Also asks the thread to run again (an error stopped it). */
    virtual void clearError() { restartRequest = 1; error = 0; }   /* [ 9] 0x001497F0 */
    virtual void clearChangeCard() { changeCard = 0; }  /* [12] 0x001497E0 */
    /* Requests the thread takes on its next pass. */
    virtual void setSelect(int i) { nextSelect = i; }   /* [14] 0x001497D0 */
    virtual void setScreenTop(int i) { nextScreenTop = i; }    /* [15] 0x001497C0 */
    virtual void setScreenSize(int n) { nextScreenSize = n; }  /* [16] 0x001497B0 */
    virtual int  isError() { return error; }            /* [ 6] 0x001497A0 */
    virtual int  isReading() { return reading; }        /* [ 7] 0x00149790 */
    virtual int  isFormat() { return format; }          /* [ 5] 0x00149780 */
    virtual int  isThreadActive() { return threadActive; }     /* [23] 0x00149770 */
    virtual int  getFileCount() { return fileCount; }   /* [ 2] 0x00149760 */
    virtual int  getXIconNum() { return 4; }            /* [30] 0x00149758 */

    /* out of line, in slot order */
    /* Line i: its file, &deletedFile for a deleted entry, &dummyFile while
     * the file is not loaded; NULL out of range. */
    virtual MemorycardFile *getFile(int i);             /* [ 3] 0x00147B70 */
    virtual bool isConnect();                           /* [ 4] 0x00148138 a PS2 memory card (T501
                                                           mc_slot_type 2) */
    virtual void clear();                               /* [ 8] 0x00147AD8 */
    /* Lists the directory again and gives the loaded files their new entry
     * numbers (by name); 0 if the listing failed. */
    virtual int  updateFileId();                        /* [10] 0x00147BE8 */
    virtual int  updateFileIdForDelete();               /* [11] 0x00147D58 the same, order[] kept */
    virtual int  isChangeCard();                        /* [13] 0x00147EE0 */
    virtual void setSelectCourse(u64 course);           /* [17] 0x00148760 */
    virtual void setSelectCar(u64 car);                 /* [18] 0x00148770 */
    virtual void setSelectCategory(int c);              /* [19] 0x00148780 */
    virtual void setSelectSubCategory(int c);           /* [20] 0x00148790 */
    virtual void startThread();                         /* [21] 0x00147F98 */
    virtual void endThread();                           /* [22] 0x00147FF8 */
    virtual int  pause();                               /* [24] 0x00148180 1 once paused */
    virtual int  restart();                             /* [25] 0x001481C0 1 once running */
    virtual void releaseIcon(bool b);                   /* [26] 0x00147F20 every file's */
    virtual void setSort(SORT s);                       /* [27] 0x001487A0 order[] built again */
    virtual void setFilter(FILTER f);                   /* [28] 0x00148668 a new pattern: the files
                                                           are released, the listing starts again */
    virtual void setEffectiveFilter(bool on);           /* [29] 0x00148738 the sorted modes keep the
                                                           matching group only */

    void setPort(int p);                                /* 0x00147B40 port = p * 4 (also fileList's
                                                           and dirCache's) */
    int  findIndexByOldId(int oldId);                   /* 0x00147D10 -1 if none */
    static void threadEntry(void *arg);                 /* 0x00148038 arg: the manager */
    void createThread();                                /* 0x00148058 priority 0x10, threadStack */
    void deleteThread();                                /* 0x001480C8 */
    int  isRunState();                                  /* 0x00148168 */
    int  stop();                                        /* 0x00148200 1 once stopped */
    void threadMain();                                  /* 0x00148240 */
    void process();                                     /* 0x00148390 one step of the thread's work */
    static int scoreCallback(void *ctx, MemorycardFile *f); /* 0x00148558 fileList's score (ctx: the
                                                           manager) */
    int  getFileDistance(MemorycardFile *f);            /* 0x00148578 */
    int  getIndexDistance(int i);                       /* 0x001485B0 lines to the visible window */
    void setupScoreFunc();                              /* 0x001485F0 */
    int  findIndexById(int id);                         /* 0x00148620 -1 if none */
    int  buildIndex();                                  /* 0x001487D8 */
    int  buildSortedIndex();                            /* 0x001488A0 */
    void sortIndex(int lo, int hi);                     /* 0x00148CC8 */
    int  readScreenInfo();                              /* 0x00148E40 */
    int  readOtherInfo();                               /* 0x00148F70 */
    int  readInfo(int i, int force);                    /* 0x00149020 */
    int  readAllSizes();                                /* 0x00149310 */
    int  readSize(int i);                               /* 0x001493B8 */
    int  readScreenIcons();                             /* 0x00149488 */
    int  readAllIcons();                                /* 0x00149558 */
    int  readIcon(int i);                               /* 0x00149600 */

    /*                    vptr                     0x00000 */
    int                   sorted;               /* 0x00004 order[] is built */
    MemorycardFileOrder   order[4000];          /* 0x00008 */
    MemorycardDirCache    dirCache;             /* 0x0FA08 (0x200 entries, pages of 0x20) */
    MemorycardFileList    fileList;             /* 0x0FA64 (200 cells) */
    MemorycardDummyFile   dummyFile;            /* 0x0FA90 getFile: the entry is not loaded */
    MemorycardDeletedFile deletedFile;          /* 0x0FAF8 getFile: the entry is deleted */
    int                   port;                 /* 0x0FB60 T501 card id (port << 2) */
    int                   dirCount;             /* 0x0FB64 entries listed; -1 not yet */
    int                   fileCount;            /* 0x0FB68 lines of order[]; -1 invalid */
    int                   sortMode;             /* 0x0FB6C SORT */
    int                   filter;               /* 0x0FB70 FILTER */
    int                   effectiveFilter;      /* 0x0FB74 the sorted modes keep the matching group only */
                                                /* (setEffectiveFilter; first pass: matchOnly) */
    u64                   selectCourse;         /* 0x0FB78 course code (0: none) */
    u64                   selectCar;            /* 0x0FB80 car code (0: none) */
    int                   selectCategory;       /* 0x0FB88 (<= 0: any) */
    int                   selectSubCategory;    /* 0x0FB8C (<= 0: any) */
    int                   changeCard;           /* 0x0FB90 */
    int                   error;                /* 0x0FB94 */
    int                   connect;              /* 0x0FB98 isConnect() of the last thread pass */
    int                   format;               /* 0x0FB9C */
    int                   reading;              /* 0x0FBA0 */
    int                   select;               /* 0x0FBA4 the selected line */
    int                   screenTop;            /* 0x0FBA8 the first visible line */
    int                   screenSize;           /* 0x0FBAC visible lines */
    int                   nextSelect;           /* 0x0FBB0 set by the menu, taken by the thread; -1 none */
    int                   nextScreenTop;        /* 0x0FBB4 the same */
    int                   nextScreenSize;       /* 0x0FBB8 the same */
    int                   threadActive;         /* 0x0FBBC */
    int                   threadState;          /* 0x0FBC0 0 running, 2 paused, 3 stopped, 4 exit */
    int                   pauseRequest;         /* 0x0FBC4 */
    int                   stopRequest;          /* 0x0FBC8 */
    int                   restartRequest;       /* 0x0FBCC */
    u8                    threadStack[0x8010];  /* 0x0FBD0 0x8000 bytes used, from its first 16-byte boundary */
    int                   threadId;             /* 0x17BE0 */
};
EGCS_SIZE(gt2k_menu::MemorycardFileManager, 0x17BE8);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu21MemorycardFileManager(void);       /* 0x00149818 */
void *__tfQ29gt2k_menu25MemorycardFileManagerBase(void);   /* 0x00149978 (also called by
                                                               __tfQ29gt2k_menu25MemorycardDemoFileManager,
                                                               T079) */

/* The __user_type_info of MemorycardFileManagerBase (2 words, filled by
 * its __tf function). */
extern u32 __tiQ29gt2k_menu25MemorycardFileManagerBase[2]; /* 0x00353350 gp-0x46A0 */
}
#endif

#endif /* GAME_T109_MEMORYCARD_FILE_MANAGER_H */
