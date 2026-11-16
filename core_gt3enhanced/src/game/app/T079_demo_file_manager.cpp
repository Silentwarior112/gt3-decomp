/* T079 - gt2k_menu::MemorycardDemoFileManager: the demo replays of the disc
 * as a memory card file list (game/app/T079_demo_file_manager.h), and T079's
 * copies of the inline members of MemorycardFileManagerBase (T109).
 *
 * Data: .rodata 0x003206B8..0x00320A77: the string, the vtables of
 * MemorycardDemoFileManager and (T079's copy) MemorycardFileManagerBase, then
 * the type names (reverse definition order): MemorycardDemoFileManager,
 * MemorycardFileManagerBase, List / Cell of MemorycardFile, the garage's
 * eight.  So the GameStatus first, then the own header (T109, T108); no
 * <new> (retail T079 has no "bad_alloc").  .sdata: "demo/%s". */
#include "game/save/T429_game_status.h"          /* getGameStatus (seen: the garage) */
#include "game/app/T079_demo_file_manager.h"    /* MemorycardDemoFileManager (T109, T108) */
#include "game/data/T435_product_config.h"       /* product_config, ProductConfig::cardDemo* */
#include "engine/io/T503_vol_thread.h"           /* file_read_blocking, vol_device_object */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "game/menu/memorycard/memorycard_icon_internal.h"

namespace gt2k_menu {

// FUNC 0x0015C360 gt2k_menu::MemorycardDemoFileManager::~MemorycardDemoFileManager
MemorycardDemoFileManager::~MemorycardDemoFileManager()
{
    /* Slot 0.  egcs: the vptr, releaseList(), then the base's inline
     * destructor (its vptr 0x0031C498) and operator delete (flags & 1). */
    releaseList();
    /* implicit: operator delete (the deleting destructor, flags & 1) */
}

/* The list (MemorycardControl::begin, mode MODE_DEMO_REPLAY_LOAD): for each
 * unlocked demo, its thumbnail "demo/thumbnail/<name>" decides the file class
 * (TYPE_REPLAY: a replay, TYPE_BESTLAP: a best lap replay); the file gets the
 * thumbnail, an empty directory entry named "demo/<name>" and an invalid
 * size.  Always 1. */
// FUNC 0x0015C3B8 gt2k_menu::MemorycardDemoFileManager::buildList
int MemorycardDemoFileManager::buildList()
{
    char path[0x40];
    /* The thumbnail is read into the 64-aligned part of this buffer (the
     * frame's locals area after path: 0x40..0x120, so 0xD1..0xE0 bytes in
     * the source). */
    u8 thumbBuf[0xE0];
    gt2k_game::GameThumbnail *thumb;
    MemorycardGT3File *file;
    const char *name;
    McDirEntry *entry;
    McDateTime noDate = { 0 };
    int i;

    /* (The news: operator new[] for the array; operator new(0x298), then the
     * constructor, for a file, with operator delete if the constructor
     * throws.) */
    /* implicit: operator new[], operator new, operator delete (the array, the
     * files and their landing pads) */
    count = countDemos();
    files = new MemorycardFile *[count];
    for (i = 0; i < count; i++) {
        thumb = (gt2k_game::GameThumbnail *)(((u32)thumbBuf + 0x3F) & ~0x3F);
        file = NULL;
        name = getDemoName(i);
        sprintf(path, "demo/thumbnail/%s", name);
        file_read_blocking(path, thumb, 0, 1, &vol_device_object);
        switch (thumb->type) {
        case gt2k_game::GameThumbnail::TYPE_REPLAY:
            file = new MemorycardReplayFile;
            break;
        case gt2k_game::GameThumbnail::TYPE_BESTLAP:
            file = new MemorycardBestlapReplayFile;
            break;
        }
        /* (another type: file stays NULL and is written through) */
        files[i] = file;
        file->thumbnail = *thumb;
        entry = &file->dir.entry;
        entry->create = noDate;                 /* one sd $zero each */
        file->dir.id = 0;
        entry->modify = noDate;
        entry->attr = 0;
        sprintf(entry->name, "demo/%s", name);
        file->setSizeInvalid();
    }
    return 1;
}

/* Deletes the files and the array. */
// FUNC 0x0015C5B8 gt2k_menu::MemorycardDemoFileManager::releaseList
void MemorycardDemoFileManager::releaseList()
{
    int i;

    /* each file through its virtual destructor (flags 3), the array with
     * operator delete[] */
    /* implicit: operator delete[] (the array) */
    for (i = 0; i < count; i++) {
        if (files[i] != NULL)
            delete files[i];
    }
    if (files != NULL)
        delete[] files;
    files = NULL;
}

/* The demos of the product configuration that the game status has unlocked
 * (GameStatus::isLicenseSPassed() >= the demo's level).  `this' is not
 * used. */
// FUNC 0x0015C650 gt2k_menu::MemorycardDemoFileManager::countDemos
int MemorycardDemoFileManager::countDemos()
{
    int i = 0;
    int unlocked = 0;
    int demoCount = product_config()->cardDemoCount();
    int level = getGameStatus()->isLicenseSPassed();

    for (; i < demoCount; i++) {
        if (!(level < product_config()->cardDemoLevel(i)))
            unlocked++;
    }
    return unlocked;
}

/* The name of the index-th unlocked demo, NULL if there are fewer. */
// FUNC 0x0015C6E8 gt2k_menu::MemorycardDemoFileManager::getDemoName
const char *MemorycardDemoFileManager::getDemoName(int index)
{
    int unlocked = 0;
    int i = 0;
    int demoCount = product_config()->cardDemoCount();
    int level = getGameStatus()->isLicenseSPassed();

    for (; i < demoCount; i++) {
        if (!(level < product_config()->cardDemoLevel(i))) {
            if (unlocked == index)
                return product_config()->cardDemoName(i);
            unlocked++;
        }
    }
    return NULL;
}

// FUNC 0x0015C7B0 gt2k_menu::MemorycardDemoFileManager::getFile(int)
MemorycardFile *MemorycardDemoFileManager::getFile(int i)
{
    return files[i];
}

// FUNC 0x0015C7C8 gt2k_menu::MemorycardDemoFileManager::getFileCount(void)
int MemorycardDemoFileManager::getFileCount()
{
    return count;
}

/* Not a card: the files load from the volume (MemorycardGT3File::
 * loadFromVolume). */
// FUNC 0x0015C7D0 gt2k_menu::MemorycardDemoFileManager::getPort(void)
int MemorycardDemoFileManager::getPort()
{
    return 0x100;
}

} /* namespace gt2k_menu */

// FUNC 0x0015C7D8 gt2k_menu::MemorycardDemoFileManager::MemorycardDemoFileManager
/* inline in game/app/T079_demo_file_manager.h; egcs emits this copy
 * (unreferenced: F_0014AE00 expands it): the vptr 0x003206D0, files and
 * count cleared. */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0015C7F8 __tfQ29gt2k_menu25MemorycardDemoFileManager
extern "C" void *__tfQ29gt2k_menu25MemorycardDemoFileManager(void)
{
    if (__tiQ29gt2k_menu25MemorycardDemoFileManager[0] == 0) {   /* _name not set yet */
        __tfQ29gt2k_menu25MemorycardFileManagerBase();          /* T109's, the base's node first */
        __rtti_si(__tiQ29gt2k_menu25MemorycardDemoFileManager,
                  "Q29gt2k_menu25MemorycardDemoFileManager",
                  (const type_info *)__tiQ29gt2k_menu25MemorycardFileManagerBase);
    }
    return __tiQ29gt2k_menu25MemorycardDemoFileManager;
}
#endif

/* ======================================================================== */
/* T079's out-of-line copies of MemorycardFileManagerBase's inline members  */
/* (game/menu/memorycard/T109_memorycard_file_manager.h), in reverse slot order as T109's   */
/* 0x00149868..0x00149948.  Unreferenced: the vtables (T079's copy          */
/* 0x003207D0 too) hold T109's.                                             */
/* ======================================================================== */

// FUNC 0x0015C848 gt2k_menu::MemorycardFileManagerBase::getXIconNum(void)
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 30, returns 8. */

// FUNC 0x0015C850 gt2k_menu::MemorycardFileManagerBase::setEffectiveFilter
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 29, empty. */

// FUNC 0x0015C858 gt2k_menu::MemorycardFileManagerBase::setFilter
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 28, empty. */

// FUNC 0x0015C860 gt2k_menu::MemorycardFileManagerBase::setSort
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 27, empty. */

// FUNC 0x0015C868 gt2k_menu::MemorycardFileManagerBase::releaseIcon
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 26, empty. */

// FUNC 0x0015C870 gt2k_menu::MemorycardFileManagerBase::restart
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 25, returns 1. */

// FUNC 0x0015C878 gt2k_menu::MemorycardFileManagerBase::pause
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 24, returns 1. */

// FUNC 0x0015C880 gt2k_menu::MemorycardFileManagerBase::isThreadActive
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 23, returns 0. */

// FUNC 0x0015C888 gt2k_menu::MemorycardFileManagerBase::endThread
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 22, empty. */

// FUNC 0x0015C890 gt2k_menu::MemorycardFileManagerBase::startThread
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 21, empty. */

// FUNC 0x0015C898 gt2k_menu::MemorycardFileManagerBase::setSelectSubCategory
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 20, empty. */

// FUNC 0x0015C8A0 gt2k_menu::MemorycardFileManagerBase::setSelectCategory
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 19, empty. */

// FUNC 0x0015C8A8 gt2k_menu::MemorycardFileManagerBase::setSelectCar
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 18, empty. */

// FUNC 0x0015C8B0 gt2k_menu::MemorycardFileManagerBase::setSelectCourse
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 17, empty. */

// FUNC 0x0015C8B8 gt2k_menu::MemorycardFileManagerBase::setScreenSize
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 16, empty. */

// FUNC 0x0015C8C0 gt2k_menu::MemorycardFileManagerBase::setScreenTop
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 15, empty. */

// FUNC 0x0015C8C8 gt2k_menu::MemorycardFileManagerBase::setSelect
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 14, empty. */

// FUNC 0x0015C8D0 gt2k_menu::MemorycardFileManagerBase::isChangeCard
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 13, returns 0. */

// FUNC 0x0015C8D8 gt2k_menu::MemorycardFileManagerBase::clearChangeCard
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 12, empty. */

// FUNC 0x0015C8E0 gt2k_menu::MemorycardFileManagerBase::updateFileIdForDelete
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 11, returns 1. */

// FUNC 0x0015C8E8 gt2k_menu::MemorycardFileManagerBase::updateFileId
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 10, returns 1. */

// FUNC 0x0015C8F0 gt2k_menu::MemorycardFileManagerBase::clearError
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 9, empty. */

// FUNC 0x0015C8F8 gt2k_menu::MemorycardFileManagerBase::clear
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 8, empty. */

// FUNC 0x0015C900 gt2k_menu::MemorycardFileManagerBase::isReading
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 7, returns 0. */

// FUNC 0x0015C908 gt2k_menu::MemorycardFileManagerBase::isError
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 6, returns 0. */

// FUNC 0x0015C910 gt2k_menu::MemorycardFileManagerBase::isFormat
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 5, returns 1. */

// FUNC 0x0015C918 gt2k_menu::MemorycardFileManagerBase::isConnect
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 4, returns true. */

// FUNC 0x0015C920 gt2k_menu::MemorycardFileManagerBase::getPort
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 1, returns 0. */

// FUNC 0x0015C928 gt2k_menu::MemorycardFileManagerBase::~MemorycardFileManagerBase
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy:
 * slot 0, the vptr (T109's vtable 0x0031C498), then the operator delete if
 * flags & 1. */
/* implicit: operator delete (the inline destructor's copy, flags & 1) */

#ifndef BOOT_OLD_GCC
/* T079's copy of the typeinfo function (unreferenced; T109's 0x00149978 is
 * the one called).  Weak: T109's definition wins at link time. */
// FUNC 0x0015C958 __tfQ29gt2k_menu25MemorycardFileManagerBase
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menu25MemorycardFileManagerBase(void)
{
    if (__tiQ29gt2k_menu25MemorycardFileManagerBase[0] == 0)    /* _name not set yet */
        __rtti_user(__tiQ29gt2k_menu25MemorycardFileManagerBase,
                    "Q29gt2k_menu25MemorycardFileManagerBase");
    return __tiQ29gt2k_menu25MemorycardFileManagerBase;
}
#endif
