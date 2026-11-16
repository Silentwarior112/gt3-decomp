/* T079_demo_file_manager.h - gt2k_menu::MemorycardDemoFileManager (translation
 * unit T079, unit u032), 0x0015C360..0x0015C998.
 *
 * The demo replays of the disc as a memory card file list: the demos of the
 * product configuration (gameconf/product.gcf, T435) that the game status
 * has unlocked, each a MemorycardReplayFile or MemorycardBestlapReplayFile
 * (T108) with the thumbnail "demo/thumbnail/<name>" and the directory name
 * "demo/<name>".  MemorycardControl (T068) holds one at +0xE40 and uses it
 * for both slots in mode 0x0C (load a demo replay, page "demo_loader"); its
 * port is 0x100 (the files load from the volume).
 *
 * The rest of T079 (0x0015C848..0x0015C990) is T079's unreferenced copy of
 * MemorycardFileManagerBase's inline members and typeinfo function (T109;
 * the vtables use T109's, and T079's own vtable copy 0x003207D0 is unused). */
#ifndef GAME_T079_DEMO_FILE_MANAGER_H
#define GAME_T079_DEMO_FILE_MANAGER_H

#include "game/game.h"
#include "game/menu/memorycard/T109_memorycard_file_manager.h"  /* base gt2k_menu::MemorycardFileManagerBase */

namespace gt2k_menu {
class MemorycardFile;                       /* T108 (pointers only) */

/* vtable 0x003206D0 (31 slots: [4] .. [30] MemorycardFileManagerBase's,
 * 0x00149938 .. 0x00149868).  typeinfo
 * __tfQ29gt2k_menu25MemorycardDemoFileManager 0x0015C7F8 (si, base
 * MemorycardFileManagerBase; storage 0x004B1BE0).  vptr +0x00 (the base has
 * no data).  Size 0x10 (MemorycardControl +0xE40 .. +0xE50). */
class MemorycardDemoFileManager : public MemorycardFileManagerBase {
public:
    /* Inline (F_0014AE00, T068, expands it; 0x0015C7D8 is the unreferenced
     * out-of-line copy).  unk0C is not set. */
    MemorycardDemoFileManager() : files(0), count(0) {}    /* 0x0015C7D8 */
    virtual ~MemorycardDemoFileManager();   /* [0] +0x0C 0x0015C360 releaseList() */
    virtual int  getPort();                 /* [1] +0x14 0x0015C7D0 0x100 (the volume) */
    virtual int  getFileCount();            /* [2] +0x1C 0x0015C7C8 count */
    virtual MemorycardFile *getFile(int i); /* [3] +0x24 0x0015C7B0 files[i] */

    /* The list (MemorycardControl::begin, mode 0x0C): per demo its
     * thumbnail decides the file class (type 2 a replay, 3 a best lap
     * replay); the file gets the thumbnail, an empty directory entry named
     * "demo/<name>" and size -3.  Always 1. */
    int  buildList();                       /* 0x0015C3B8 */
    void releaseList();                     /* 0x0015C5B8 deletes the files and the array */
    /* The demos whose unlock value the game status reaches
     * (GameStatus::F_00219D80, T429).  this is not used. */
    int  countDemos();                      /* 0x0015C650 */
    const char *getDemoName(int index);     /* 0x0015C6E8 of the index-th such demo, NULL past them */

    /*  vptr                                   0x00 */
    MemorycardFile **files;                 /* 0x04 count entries (new[]) */
    int              count;                 /* 0x08 */
    int              unk0C;                 /* 0x0C */
};
EGCS_SIZE(gt2k_menu::MemorycardDemoFileManager, 0x10);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu25MemorycardDemoFileManager(void); /* 0x0015C7F8 */
#endif

#endif /* GAME_T079_DEMO_FILE_MANAGER_H */
