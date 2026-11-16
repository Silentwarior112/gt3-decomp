/* T109 - gt2k_menu::MemorycardFileManager and MemorycardFileManagerBase.
 * 0x00147870..0x001499B8.
 *
 * The manager of one memory card port for the file screens.  Its order[]
 * maps the lines of the menu's list to directory entries of the T110
 * listing; the files themselves live in fileList (a pool of 200 cells, the
 * file farthest from the visible window is recycled first).  The worker
 * thread (threadMain / process) takes the menu's requests (nextSelect...),
 * watches the card (changed, connected, formatted), counts the directory,
 * builds order[] and then, one step per pass, reads the files' information,
 * size and icon nearest the selection first.  The steps return a
 * McfmStepResult (memorycard_file_manager_internal.h).
 *
 * The includes give retail's .rodata (0x0031C380..0x0031C66B): setFilter's
 * jump table, the vtables of MemorycardFileManager and of the base, then the
 * type names in reverse class-definition order: the two of this file,
 * List / Cell<MemorycardFile> (T108's header), then RegurationResult, which
 * retail's T109 saw before the memory card classes (as T108 did) without
 * using it.  No <new>: the "bad_alloc" after the names (0x0031C670) opens
 * T068's .rodata. */
#include "types.h"
#include <stddef.h>
#include "libc.h"                           /* strcpy */
#include "eekernel.h"                       /* ExitDeleteThread */
#include "cxxrt.h"                          /* type_info, __rtti_si, __rtti_user */

/* seen by retail's T109, not used (its type name only) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* RegurationResult */

/* outside the unit */
#include "engine/io/T501_memorycard_manager.h"   /* mc_slot_take_changed, mc_slot_status, mc_slot_type,
                                               mc_slot_formatted */
#include "game/data/T489_car_database.h"         /* db_car_index, db_course_index */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "engine/media/G002_mpeg_frameq.h"          /* thread_start_retry; libmc.h sceMcTypePS2 */

/* own translation unit */
#include "game/menu/memorycard/T109_memorycard_file_manager.h"  /* MemorycardFileManagerBase, MemorycardFileManager,
                                               MemorycardFileOrder; T108 MemorycardFile and its
                                               kinds, MemorycardFileList, MemorycardDir,
                                               ::McDirEntry, the mc* name helpers; T110
                                               MemorycardDirCache; menu_list.h ListIterator */
#include "game/menu/memorycard/memorycard_file_manager_internal.h"                           /* the unit's constants, mcfm_set_date_key */

namespace gt2k_menu {

/* filter = FILTER_INIT makes the setFilter(FILTER_ALL) below take effect;
 * sortMode is overwritten by setSort.  selectCar, selectCategory,
 * selectSubCategory and reading are not initialised (the thread words are,
 * by createThread). */
// FUNC 0x00147870 gt2k_menu::MemorycardFileManager::MemorycardFileManager
MemorycardFileManager::MemorycardFileManager()
    : dirCache(MCFM_DIR_MAX_ENTRIES, MCFM_DIR_PAGE_SIZE), fileList(MCFM_FILE_CELLS)
{
    /* The members: MemorycardDirCache(0x200, 0x20), MemorycardFileList(200),
     * MemorycardDummyFile() for dummyFile and for deletedFile (whose inline
     * MemorycardDeletedFile constructor then stores its vtable).  The landing
     * pads destroy the members already built (~MemorycardFile,
     * ~MemorycardFileList, ~MemorycardDirCache) and put back the base's
     * vtable. */
    /* implicit: MemorycardDirCache, MemorycardFileList, MemorycardDummyFile (member constructors), ~MemorycardFile, ~MemorycardFileList, ~MemorycardDirCache (exception cleanup) */
    /* The statement order (searched): after `sorted' (every later int store
     * waits for its store through `this') sched1 first takes, in source
     * order, the stores that end a register's life: screenSize (16),
     * fileCount (the last of the -1 stores), sortMode, filter, then
     * selectCourse (its sd ends this + 0x8000's life), then the plain zero
     * and -1 stores in source order.  So these five come in this order and
     * last; the window and the requests may stand anywhere before
     * fileCount. */
    format = 1;
    threadActive = 0;
    changeCard = 0;
    error = 0;
    connect = 0;
    effectiveFilter = 0;
    sorted = 0;
    port = 0;
    select = 0;
    screenTop = 0;
    screenSize = MCFM_SCREEN_SIZE;
    nextSelect = -1;
    nextScreenTop = -1;
    nextScreenSize = -1;
    dirCount = -1;
    fileCount = -1;
    sortMode = SORT_INIT;
    filter = FILTER_INIT;
    selectCourse = 0;
    setFilter(FILTER_ALL);          /* direct calls (constructor) */
    setSort(SORT_NONE);
    setupScoreFunc();
}

/* Stops the thread if it runs (without clearing threadActive). */
// FUNC 0x00147A28 gt2k_menu::MemorycardFileManager::~MemorycardFileManager
MemorycardFileManager::~MemorycardFileManager()
{
    if (threadActive != 0)
        deleteThread();
    /* Then the members: ~MemorycardFile (0x00142B18, called with flags 2) of
     * deletedFile and of dummyFile, ~MemorycardFileList, ~MemorycardDirCache;
     * the base's vtable; operator delete for the deleting form. */
    /* implicit: ~MemorycardFile, ~MemorycardFileList, ~MemorycardDirCache (member destructors), operator delete (deleting destructor) */
}

/* Forgets the listing and every loaded file (the thread starts over). */
// FUNC 0x00147AD8 gt2k_menu::MemorycardFileManager::clear
void MemorycardFileManager::clear()
{
    format = 1;
    connect = 0;
    sorted = 0;
    reading = 0;
    select = 0;
    screenTop = 0;
    dirCount = -1;                  /* (this order: sched1 then stores fileCount, the -1's */
    fileCount = -1;                 /* last use, early and dirCount in the jal's slot) */
    dirCache.clear();
    fileList.releaseAll();
}

/* T501 card ids are port << 2 | slot (slot 0 here).  Called by F_0014AE00
 * (T068). */
// FUNC 0x00147B40 gt2k_menu::MemorycardFileManager::setPort
void MemorycardFileManager::setPort(int p)
{
    port = p * 4;
    fileList.setUnk24(port);        /* inline (Lupo 2: MemorycardFileList::setPort(int);
                                       out-of-line copy 0x00146B28); port is read again
                                       after it (an int store through &fileList) */
    dirCache.setPort(port);         /* inline (T110 header) */
}

/* Line `line': its file, the deleted-file placeholder for a deleted entry,
 * the dummy one while the file is not loaded; NULL out of range. */
// FUNC 0x00147B70 gt2k_menu::MemorycardFileManager::getFile
MemorycardFile *MemorycardFileManager::getFile(int line)
{
    if (line < 0 || line >= fileCount)
        return NULL;
    int id = order[line].id;
    if (id < 0)
        return &deletedFile;
    MemorycardFile *file = fileList.findById(id);
    if (file == NULL)
        return &dummyFile;
    return file;
}

/* Lists the directory again and gives the loaded files their new entry
 * numbers (by name); the files not found any more are released, order[] is
 * built again.  Returns 0 if the listing failed. */
// FUNC 0x00147BE8 gt2k_menu::MemorycardFileManager::updateFileId
int MemorycardFileManager::updateFileId()
{
    int id = 0;
    ::McDirEntry *entry;

    dirCache.clear();
    fileList.markAll();
    while ((entry = dirCache.getEntry(id)) != NULL) {
        MemorycardFile *file = fileList.findByName(entry->name);
        if (file != NULL) {
            MemorycardDir *dir = &file->dir;
            dir->id = id;
            dir->entry = *entry;
        }
        id++;
    }
    if (dirCache.takeError() != 0)
        return 0;
    dirCount = id;
    if (effectiveFilter == 0)
        fileCount = id;
    fileList.removeMarked();
    buildIndex();
    return 1;
}

/* The line whose saved id (updateFileIdForDelete) is oldId, or -1.  (loop.c
 * hoists the fileCount load: `lw v0; ...; move a2, v0'.) */
// FUNC 0x00147D10 gt2k_menu::MemorycardFileManager::findIndexByOldId
int MemorycardFileManager::findIndexByOldId(int oldId)
{
    for (int line = 0; line < fileCount; line++) {
        if (order[line].oldId == oldId)
            return line;
    }
    return -1;
}

/* As updateFileId, but order[] keeps its lines (after a file was deleted):
 * every line becomes "deleted" (-1) unless its file is found again, and
 * order[] is not rebuilt.  The line of a found file is searched by its old
 * id; a file without a line (-1) would write order[-1].id, 8 bytes before
 * the object (not guarded in the original). */
// FUNC 0x00147D58 gt2k_menu::MemorycardFileManager::updateFileIdForDelete
int MemorycardFileManager::updateFileIdForDelete()
{
    int id;                         /* one variable for both loops (retail: $s3) */
    ::McDirEntry *entry;

    for (id = 0; id < fileCount; id++) {
        order[id].oldId = order[id].id;
        order[id].id = -1;
    }

    id = 0;
    dirCache.clear();
    fileList.markAll();
    while ((entry = dirCache.getEntry(id)) != NULL) {
        MemorycardFile *file = fileList.findByName(entry->name);
        if (file != NULL) {
            MemorycardDir *dir = &file->dir;
            int oldId = dir->getOldId();
            file->dir.id = id;
            dir->entry = *entry;
            order[findIndexByOldId(oldId)].id = id;
        }
        id++;
    }
    if (dirCache.takeError() != 0)
        return 0;
    dirCount = id;
    fileList.removeMarked();
    return 1;
}

/* The card-changed flag stays set until clearChangeCard. */
// FUNC 0x00147EE0 gt2k_menu::MemorycardFileManager::isChangeCard
int MemorycardFileManager::isChangeCard()
{
    if (mc_slot_take_changed(port) != 0)
        changeCard = 1;
    return changeCard;
}

/* Every loaded file releases its icon. */
// FUNC 0x00147F20 gt2k_menu::MemorycardFileManager::releaseIcon
void MemorycardFileManager::releaseIcon(bool noWait)
{
    ListIterator<MemorycardFile> it(&fileList);
    MemorycardFile *file;

    while ((file = it.next()) != NULL)
        file->releaseIcon(noWait);
}

// FUNC 0x00147F98 gt2k_menu::MemorycardFileManager::startThread
void MemorycardFileManager::startThread()
{
    if (threadActive == 0) {
        clear();                    /* virtual, slot 8 */
        threadActive = 1;
        createThread();
    }
}

// FUNC 0x00147FF8 gt2k_menu::MemorycardFileManager::endThread
void MemorycardFileManager::endThread()
{
    if (threadActive != 0) {
        deleteThread();
        threadActive = 0;
    }
}

/* The thread's entry: arg is the manager. */
// FUNC 0x00148038 gt2k_menu::MemorycardFileManager::threadEntry
void MemorycardFileManager::threadEntry(void *arg)
{
    ((MemorycardFileManager *)arg)->threadMain();
}

/* Priority 0x10, 0x8000 bytes of threadStack from its first 16-byte
 * boundary. */
// FUNC 0x00148058 gt2k_menu::MemorycardFileManager::createThread
void MemorycardFileManager::createThread()
{
    /* (the store order: a search over the 120 orders; sched1 puts the
     * threadState store first and error into the jal's slot) */
    pauseRequest = 0;
    stopRequest = 0;
    restartRequest = 0;
    error = 0;
    threadState = MCFM_STATE_RUN;
    /* int arithmetic: `addiu -0x10' for the mask (u32: lui / ori) */
    threadId = thread_start_retry(threadEntry, this, MCFM_THREAD_PRIORITY,
                                  (void *)(((int)threadStack + 15) & ~15),
                                  MCFM_THREAD_STACK_SIZE);
}

/* Stops the thread, asks it to exit and waits until it has (it writes
 * MCFM_STATE_RUN, then ExitDeleteThread). */
// FUNC 0x001480C8 gt2k_menu::MemorycardFileManager::deleteThread
void MemorycardFileManager::deleteThread()
{
    while (stop() == 0)
        gfx_vsync(1);
    threadState = MCFM_STATE_EXIT;
    while (isRunState() == 0)
        gfx_vsync(1);
}

/* A PS2 memory card is in the port. */
// FUNC 0x00148138 gt2k_menu::MemorycardFileManager::isConnect
bool MemorycardFileManager::isConnect()
{
    return mc_slot_type(port) == sceMcTypePS2;
}

/* (? the original's form) The worker's state as the menu's side reads it,
 * through its address: isRunState, pause, restart and stop load threadState
 * through a register holding this + 0xFBC0 (`ori 0xFBC0; addu; lw 0'),
 * while their request stores and the worker's own accesses add the 0x8000
 * part (`sw 0x7BC4(this + 0x8000)'), the split egcs makes of a direct field
 * access.  A reference (or pointer) gives that register; a direct read, an
 * inline member returning the value, a volatile field or a volatile cast
 * give the split form. */
static inline int mcfm_state(const int &state)
{
    return state;
}

// FUNC 0x00148168 gt2k_menu::MemorycardFileManager::isRunState
int MemorycardFileManager::isRunState()
{
    return mcfm_state(threadState) == MCFM_STATE_RUN;
}

/* Asks the thread to pause; 1 once it has. */
// FUNC 0x00148180 gt2k_menu::MemorycardFileManager::pause
int MemorycardFileManager::pause()
{
    if (mcfm_state(threadState) == MCFM_STATE_PAUSED) {
        pauseRequest = 0;
        return 1;
    }
    pauseRequest = 1;
    return 0;
}

/* Asks the thread to run again; 1 once it does. */
// FUNC 0x001481C0 gt2k_menu::MemorycardFileManager::restart
int MemorycardFileManager::restart()
{
    if (mcfm_state(threadState) == MCFM_STATE_RUN) {
        restartRequest = 0;
        return 1;
    }
    restartRequest = 1;
    return 0;
}

/* Asks the thread to stop; 1 once it has. */
// FUNC 0x00148200 gt2k_menu::MemorycardFileManager::stop
int MemorycardFileManager::stop()
{
    if (mcfm_state(threadState) == MCFM_STATE_STOPPED) {
        stopRequest = 0;
        return 1;
    }
    stopRequest = 1;
    return 0;
}

/* The worker thread.  Every pass takes the menu's window requests, then
 * the state requests (an error stops it like a stop request), refreshes
 * connect, and runs one process() step while running.  MCFM_STATE_EXIT:
 * back to MCFM_STATE_RUN and exit (ExitDeleteThread does not return; the
 * loop has no exit). */
// FUNC 0x00148240 gt2k_menu::MemorycardFileManager::threadMain
void MemorycardFileManager::threadMain()
{
    for (;;) {
        if (nextSelect >= 0) {
            select = nextSelect;
            nextSelect = -1;
        }
        if (nextScreenTop >= 0) {
            screenTop = nextScreenTop;
            nextScreenTop = -1;
        }
        if (nextScreenSize >= 0) {
            screenSize = nextScreenSize;
            nextScreenSize = -1;
        }
        if (threadState != MCFM_STATE_EXIT) {
            if (error != 0 || stopRequest != 0) {
                threadState = MCFM_STATE_STOPPED;
                stopRequest = 0;
            }
            if (pauseRequest != 0) {
                threadState = MCFM_STATE_PAUSED;
                pauseRequest = 0;
            }
            if (restartRequest != 0) {
                threadState = MCFM_STATE_RUN;
                restartRequest = 0;
            }
        }
        connect = isConnect();      /* virtual, slot 4 */
        switch (threadState) {
        case MCFM_STATE_EXIT:
            threadState = MCFM_STATE_RUN;
            ExitDeleteThread();
            continue;               /* (no vsync) */
        case MCFM_STATE_RUN:
            process();
            break;
        case MCFM_STATE_PAUSED:     /* the case tree tests < 4, then < 2 (2..3 */
        case MCFM_STATE_STOPPED:    /* one range), then 0 and 4 */
            break;
        }
        gfx_vsync(1);
    }
}

/* One step of the thread's work, in this order: the card was changed
 * (clear everything), no card, not formatted, count the directory, build
 * order[], then the information / size / icon reads (see the read*
 * functions).  The waits are the thread's own (gfx_vsync). */
// FUNC 0x00148390 gt2k_menu::MemorycardFileManager::process
void MemorycardFileManager::process()
{
    if (mc_slot_take_changed(port) != 0) {
        changeCard = 1;
        clear();                    /* virtual, slot 8 */
        gfx_vsync(10);
        return;
    }
    if (connect == 0) {
        gfx_vsync(10);
        return;
    }
    if (mc_slot_formatted(port) == 0) {
        gfx_vsync(10);
        format = 0;
        return;
    }
    format = 1;
    reading = 1;
    if (dirCount < 0) {
        dirCount = dirCache.countEntries();
        if (dirCount < 0)
            error = 1;
        return;
    }

    /* Each step checks its own result (retail repeats the check, each copy
     * storing the 1 of a register known to hold it: format's, then the
     * previous step's result); the assignment inside the condition (as a
     * statement of its own, 62.5%). */
    int result;
    if (sorted == 0) {
        result = buildIndex();
        if (result != MCFM_STEP_BUSY && result != 2)
            error = 1;
        return;
    }
    if ((result = readScreenInfo()) != MCFM_STEP_DONE) {
        if (result != MCFM_STEP_BUSY && result != 2)
            error = 1;
        return;
    }
    if ((result = readScreenIcons()) != MCFM_STEP_DONE) {
        if (result != MCFM_STEP_BUSY && result != 2)
            error = 1;
        return;
    }
    if ((result = readOtherInfo()) != MCFM_STEP_DONE) {
        if (result != MCFM_STEP_BUSY && result != 2)
            error = 1;
        return;
    }
    if ((result = readAllSizes()) != MCFM_STEP_DONE) {
        if (result != MCFM_STEP_BUSY && result != 2)
            error = 1;
        return;
    }
    if ((result = readAllIcons()) != MCFM_STEP_DONE) {
        if (result != MCFM_STEP_BUSY && result != 2)
            error = 1;
        return;
    }
    gfx_vsync(1);                   /* everything read */
}

/* fileList's score function (setupScoreFunc): ctx is the manager. */
// FUNC 0x00148558 gt2k_menu::MemorycardFileManager::scoreCallback
int MemorycardFileManager::scoreCallback(void *ctx, MemorycardFile *file)
{
    return ((MemorycardFileManager *)ctx)->getFileDistance(file);
}

/* getIndexDistance of the file's line (of line -1 if it has none). */
// FUNC 0x00148578 gt2k_menu::MemorycardFileManager::getFileDistance
int MemorycardFileManager::getFileDistance(MemorycardFile *file)
{
    return getIndexDistance(findIndexById(file->dir.id));
}

/* Lines between line `line' and the visible window [screenTop, screenTop +
 * screenSize): 0 inside it. */
// FUNC 0x001485B0 gt2k_menu::MemorycardFileManager::getIndexDistance
int MemorycardFileManager::getIndexDistance(int line)
{
    int offset = line - screenTop;
    if (offset < 0)
        return -offset;
    if (offset >= screenSize)
        return offset - screenSize;
    return 0;
}

/* The farther from the window, the sooner a file's cell is recycled. */
// FUNC 0x001485F0 gt2k_menu::MemorycardFileManager::setupScoreFunc
void MemorycardFileManager::setupScoreFunc()
{
    fileList.setScoreFunc(scoreCallback, this);
}

/* The line showing directory entry id, or -1 (`id == ...': the bnel's
 * operand order). */
// FUNC 0x00148620 gt2k_menu::MemorycardFileManager::findIndexById
int MemorycardFileManager::findIndexById(int id)
{
    for (int line = 0; line < fileCount; line++) {
        if (id == order[line].id)
            return line;
    }
    return -1;
}

/* A new search pattern: every file is released and the listing starts
 * again.  Filters 0, 1 and 6.. (unsigned) list everything (jump table
 * 0x0031C380, 6 entries). */
// FUNC 0x00148668 gt2k_menu::MemorycardFileManager::setFilter
void MemorycardFileManager::setFilter(FILTER f)
{
    char pattern[0x40];

    if (filter != f) {
        filter = f;
        switch (f) {
        case FILTER_GT3:
            mcMakeSearchPattern(pattern, -1);
            break;
        case FILTER_REPLAY:
            MemorycardReplayFile::makeSearchPattern(pattern);
            break;
        case FILTER_BESTLAP:
            MemorycardBestlapReplayFile::makeSearchPattern(pattern);
            break;
        case FILTER_SETTING:
            MemorycardSettingFile::makeSearchPattern(pattern);
            break;
        case FILTER_ALL:            /* (an explicit case: the jump table then */
        default:                    /* spans 0..5) */
            strcpy(pattern, "/*");  /* 0x00351790, copied inline */
            break;
        }
        fileList.releaseAll();
        dirCache.setPattern(pattern);
    }
}

/* True: the sorted modes keep only the first (matching, key 0) group of
 * lines (Lupo 2's name of the slot). */
// FUNC 0x00148738 gt2k_menu::MemorycardFileManager::setEffectiveFilter
void MemorycardFileManager::setEffectiveFilter(bool on)
{
    if (on != effectiveFilter) {
        effectiveFilter = on;
        fileCount = -1;
        sorted = 0;
    }
}

/* A 64-bit store. */
// FUNC 0x00148760 gt2k_menu::MemorycardFileManager::setSelectCourse
void MemorycardFileManager::setSelectCourse(u64 course)
{
    selectCourse = course;
}

/* A 64-bit store. */
// FUNC 0x00148770 gt2k_menu::MemorycardFileManager::setSelectCar
void MemorycardFileManager::setSelectCar(u64 car)
{
    selectCar = car;
}

// FUNC 0x00148780 gt2k_menu::MemorycardFileManager::setSelectCategory
void MemorycardFileManager::setSelectCategory(int c)
{
    selectCategory = c;
}

// FUNC 0x00148790 gt2k_menu::MemorycardFileManager::setSelectSubCategory
void MemorycardFileManager::setSelectSubCategory(int c)
{
    selectSubCategory = c;
}

/* order[] is built again on the thread's next pass. */
// FUNC 0x001487A0 gt2k_menu::MemorycardFileManager::setSort
void MemorycardFileManager::setSort(SORT s)
{
    if (sorted == 0 || sortMode != s) {
        sorted = 0;
        sortMode = s;
    }
}

/* Builds order[]: sorted for the modes SORT_GAME..SORT_BESTLAP2, else the
 * directory order.  Returns MCFM_STEP_BUSY, or buildSortedIndex's failure.
 * A switch: the case tree tests < 0, < 7, >= 13, the low end of a 0..6
 * range of cases (named like the others in the original, presumably; this
 * build has names for 0 and 7..13 only) beside default. */
// FUNC 0x001487D8 gt2k_menu::MemorycardFileManager::buildIndex
int MemorycardFileManager::buildIndex()
{
    switch (sortMode) {
    case SORT_GAME:
    case SORT_REPLAY:
    case SORT_BESTLAP:
    case SORT_SETTING:
    case SORT_DATE:
    case SORT_BESTLAP2:
        {
            int result = buildSortedIndex();
            if (result != MCFM_STEP_BUSY)
                return result;
        }
        break;
    case SORT_NONE:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    default:
        for (int id = 0; id < dirCount; id++) {
            order[id].id = id;
            order[id].key = 0;
        }
        fileCount = dirCount;
        break;
    }
    sorted = 1;
    return MCFM_STEP_BUSY;
}

/* The sorted order.  GT3 names go to the front with key 0 when they match
 * the mode and the select* values (1 when not, 2 for game data in
 * SORT_GAME), other names to the back with key ~0; both parts are sorted by
 * key, then every run of equal keys longer than one by date, newest first
 * (key = ~modify).  With effectiveFilter only the first run is kept if its key is
 * 0 (else nothing).  fileCount = the lines kept.  Returns MCFM_STEP_BUSY, or
 * MCFM_STEP_NO_ENTRY if the listing fails.  (Matching: frontCount,
 * backStart, courseIndex, carIndex spill to sp+0..0xC in declaration order;
 * the lines are indexed (loop.c reduces their addresses); runStart is
 * declared inside the `if': its 0 then fills the blez's slot.) */
// FUNC 0x001488A0 gt2k_menu::MemorycardFileManager::buildSortedIndex
int MemorycardFileManager::buildSortedIndex()
{
    int frontCount = 0;
    int backStart = dirCount;
    int courseIndex = -1;
    if (selectCourse != 0)
        courseIndex = db_course_index(selectCourse);
    int carIndex = -1;
    if (selectCar != 0)
        carIndex = db_car_index(selectCar);
    int line;                               /* also the entry number of the first loop
                                               ($s2 throughout) */

    for (line = 0; line < dirCount; line++) {
        ::McDirEntry *entry = dirCache.getEntry(line);
        if (entry == NULL)
            return MCFM_STEP_NO_ENTRY;
        const char *name = entry->name;
        if (mcIsGT3Name(name) != 0) {
            char type = mcGetNameType(name);
            int fileCourse = mcGetNameCourse(name);
            int fileCar = mcGetNameCar(name);
            int fileCategory = mcGetNameCategory(name);
            int fileSubCategory = mcGetNameSubCategory(name);
            u64 key = 0;

            if (sortMode == SORT_BESTLAP || sortMode == SORT_BESTLAP2) {
                if (courseIndex >= 0) {   /* (an if / else: `move a0, 1; movz') */
                    if (courseIndex != fileCourse)
                        key = 1;
                    else
                        key = 0;
                }
            }
            if (sortMode == SORT_SETTING) {
                if (carIndex >= 0 && carIndex != fileCar)
                    key = 1;
            }
            if (sortMode == SORT_BESTLAP || sortMode == SORT_BESTLAP2) {
                if (selectCategory > 0 && selectCategory != fileCategory)
                    key = 1;
            }
            if (sortMode == SORT_BESTLAP || sortMode == SORT_BESTLAP2) {
                if (selectSubCategory > 0 && selectSubCategory != fileSubCategory)
                    key = 1;
            }
            if (sortMode == SORT_REPLAY) {
                if (type != 'R' && type != 'B')
                    key = 1;
            }
            if (sortMode == SORT_BESTLAP || sortMode == SORT_BESTLAP2) {
                if (type != 'B')
                    key = 1;
            }
            if (sortMode == SORT_SETTING) {
                if (type != 'S')
                    key = 1;
            }
            if (sortMode == SORT_GAME) {
                if (type == 'G')
                    key = 2;
            }
            if (sortMode == SORT_BESTLAP2) {
                if (fileCategory == MCFM_CATEGORY_TOP_SPEED)
                    key = 1;
            }
            order[frontCount].id = line;    /* (id first: loop.c then reduces the key's
                                               address, $fp = &order[0].key) */
            order[frontCount].key = key;
            frontCount++;
        } else {
            backStart--;
            order[backStart].id = line;
            order[backStart].key = (u64)-1;
        }
    }

    sortIndex(0, frontCount - 1);
    line = 0;
    sortIndex(backStart, dirCount - 1);

    /* The runs of equal keys (line == runStart at the top of the loop). */
    if (dirCount > 0) {
        int runStart = 0;
        u64 runKey = order[0].key;
        while (effectiveFilter == 0 || runKey == 0) {
            for (line++; line < dirCount && order[line].key == runKey; line++)
                ;
            if (line - runStart >= 2) {
                for (int k = runStart; k < line; k++) {
                    ::McDirEntry *entry = dirCache.getEntry(order[k].id);
                    if (entry == NULL)
                        return MCFM_STEP_NO_ENTRY;
                    mcfm_set_date_key(order[k], entry->modify);
                }
                sortIndex(runStart, line - 1);
            }
            if (line >= dirCount)
                break;
            runStart = line;
            runKey = order[line].key;
        }
    }
    fileCount = line;
    return MCFM_STEP_BUSY;
}

/* Quicksort of order[lo..hi] by key (unsigned), pivot the middle line;
 * the middle is (lo + hi) >> 1. */
// FUNC 0x00148CC8 gt2k_menu::MemorycardFileManager::sortIndex
void MemorycardFileManager::sortIndex(int lo, int hi)
{
    if (hi < 0 || hi < lo)
        return;

    u64 pivot = order[(lo + hi) >> 1].key;
    int left = lo;
    int right = hi;
    for (;;) {
        while (order[left].key < pivot)
            left++;
        while (pivot < order[right].key)
            right--;
        if (left >= right)
            break;
        MemorycardFileOrder tmp = order[left];
        order[left] = order[right];
        order[right] = tmp;
        left++;
        right--;
    }
    if (lo < left - 1)
        sortIndex(lo, left - 1);
    if (right + 1 < hi)
        sortIndex(right + 1, hi);
}

/* The window around the selection: the information (forced), the sizes of
 * the 3 lines up / 2 down nearest it and the selected line's icon;
 * alternately up and down from select. */
// FUNC 0x00148E40 gt2k_menu::MemorycardFileManager::readScreenInfo
int MemorycardFileManager::readScreenInfo()
{
    for (int dist = 0; dist < screenSize; dist++) {
        int result = readInfo(select - dist, 1);
        if (result != MCFM_STEP_DONE)
            return result;
        if (dist < 3) {
            result = readSize(select - dist);
            if (result != MCFM_STEP_DONE)
                return result;
        }
        if (dist == 0) {
            result = readIcon(select);
            if (result != MCFM_STEP_DONE)
                return result;
        }
        result = readInfo(select + dist + 1, 1);
        if (result != MCFM_STEP_DONE)
            return result;
        if (dist < 2) {
            result = readSize(select + dist + 1);
            if (result != MCFM_STEP_DONE)
                return result;
        }
    }
    return MCFM_STEP_DONE;
}

/* The information beyond the window distance (not forced: only into a free
 * cell or one farther away). */
// FUNC 0x00148F70 gt2k_menu::MemorycardFileManager::readOtherInfo
int MemorycardFileManager::readOtherInfo()
{
    for (int dist = screenSize; dist < fileCount; dist++) {
        int result = readInfo(select - dist, 0);
        if (result != MCFM_STEP_DONE)
            return result;
        result = readInfo(select + dist + 1, 0);
        if (result != MCFM_STEP_DONE)
            return result;
    }
    return MCFM_STEP_DONE;
}

/* One step for the information of line `line': MCFM_STEP_DONE if there is
 * nothing to do (out of range, already read, or not forced and no cell worth
 * recycling), MCFM_STEP_BUSY after a read attempt, MCFM_STEP_NO_ENTRY /
 * MCFM_STEP_NO_CELL.  A file is created for the entry if needed; after a
 * failed read it is replaced by a damaged file at once when the card says
 * the slot or the entry is gone (MCFM_STEP_NO_CELL if that fails), else
 * after 5 more failures.  A good read passes the mode's selection to the
 * file (checkSelectable). */
// FUNC 0x00149020 gt2k_menu::MemorycardFileManager::readInfo
int MemorycardFileManager::readInfo(int line, int force)
{
    if (line < 0 || line >= fileCount)
        return MCFM_STEP_DONE;

    int id = order[line].id;
    MemorycardFile *file = fileList.findById(id);
    if (file != NULL) {
        if (file->infoValid() != 0)
            return MCFM_STEP_DONE;
    } else {
        if (force == 0 && fileList.hasFreeCell() == 0) {
            int worst = getFileDistance(fileList.findMaxScore());
            if (!(getIndexDistance(line) < worst))
                return MCFM_STEP_DONE;
        }
        ::McDirEntry *entry = dirCache.getEntry(id);
        if (entry == NULL)
            return MCFM_STEP_NO_ENTRY;
        MemorycardDir dir;                  /* sp+0x00 */
        dir.id = id;
        dir.entry = *entry;
        file = fileList.add(dir);
        if (file == NULL)
            return MCFM_STEP_NO_CELL;
    }

    if (file->readInfo(port) == 0) {
        int status = mc_slot_status(port);  /* (u32)(status - 1) < 2 */
        if (status == MC_STATUS_NO_SLOT || status == MC_STATUS_NO_ENTRY)
            return (fileList.setDamaged(file) != 0) ? MCFM_STEP_BUSY : MCFM_STEP_NO_CELL;
        if (file->readCount++ >= MCFM_READ_RETRIES)     /* !retry() */
            fileList.setDamaged(file);
        return MCFM_STEP_BUSY;
    }

    switch (sortMode) {
    case SORT_REPLAY:
        file->checkSelectable('R', 0, 0, 0, 0);
        break;
    case SORT_BESTLAP:
    case SORT_BESTLAP2:
        file->checkSelectable('B', selectCourse, 0, selectCategory, selectSubCategory);
        break;
    case SORT_SETTING:
        file->checkSelectable('S', 0, selectCar, 0, 0);
        break;
    default:
        file->checkSelectable(0, 0, 0, 0, 0);
        break;
    }
    return MCFM_STEP_BUSY;
}

/* The sizes of all lines, alternately up and down from select. */
// FUNC 0x00149310 gt2k_menu::MemorycardFileManager::readAllSizes
int MemorycardFileManager::readAllSizes()
{
    for (int dist = 0; dist < fileCount; dist++) {
        int result = readSize(select - dist);
        if (result != MCFM_STEP_DONE)
            return result;
        result = readSize(select + dist + 1);
        if (result != MCFM_STEP_DONE)
            return result;
    }
    return MCFM_STEP_DONE;
}

/* One step for the size of line `line''s loaded file: MCFM_STEP_DONE if
 * there is nothing to do, else MCFM_STEP_BUSY.  After 5 failures the file is
 * replaced by a damaged one and the old object gets sizeKB -2 although
 * setDamaged has released its cell (a store into the free cell, as in the
 * original). */
// FUNC 0x001493B8 gt2k_menu::MemorycardFileManager::readSize
int MemorycardFileManager::readSize(int line)
{
    if (line < 0 || line >= fileCount)
        return MCFM_STEP_DONE;

    MemorycardFile *file = fileList.findById(order[line].id);
    if (file == NULL)
        return MCFM_STEP_DONE;
    if (file->sizeKB >= 0)
        return MCFM_STEP_DONE;
    if (file->sizeKB == MCFM_SIZE_ERROR)    /* isSizeError() */
        return MCFM_STEP_DONE;
    if (file->updateSize(port) != 0)
        return MCFM_STEP_BUSY;
    if (file->readCount++ >= MCFM_READ_RETRIES) {   /* !retry() */
        fileList.setDamaged(file);
        file->sizeKB = MCFM_SIZE_ERROR;     /* setSizeError() */
    }
    return MCFM_STEP_BUSY;
}

/* The icons of the visible lines, alternately up and down from select. */
// FUNC 0x00149488 gt2k_menu::MemorycardFileManager::readScreenIcons
int MemorycardFileManager::readScreenIcons()
{
    for (int dist = 0; dist < screenSize; dist++) {
        int line = select - dist;
        if (line >= screenTop) {
            int result = readIcon(line);
            if (result != MCFM_STEP_DONE)
                return result;
        }
        line = select + dist + 1;
        if (line < screenTop + screenSize) {
            int result = readIcon(line);
            if (result != MCFM_STEP_DONE)
                return result;
        }
    }
    return MCFM_STEP_DONE;
}

/* The icons of all lines, alternately up and down from select. */
// FUNC 0x00149558 gt2k_menu::MemorycardFileManager::readAllIcons
int MemorycardFileManager::readAllIcons()
{
    for (int dist = 0; dist < fileCount; dist++) {
        int result = readIcon(select - dist);
        if (result != MCFM_STEP_DONE)
            return result;
        result = readIcon(select + dist + 1);
        if (result != MCFM_STEP_DONE)
            return result;
    }
    return MCFM_STEP_DONE;
}

/* One step for the icon of line `line''s loaded file: MCFM_STEP_DONE if
 * there is nothing to do, else MCFM_STEP_BUSY.  A failed read counts towards
 * the damaged file (after 5).  If the read did not finish the icon, the
 * farthest file holding icon memory releases it, unless it is nearer the
 * window than this one. */
// FUNC 0x00149600 gt2k_menu::MemorycardFileManager::readIcon
int MemorycardFileManager::readIcon(int line)
{
    if (line < 0 || line >= fileCount)
        return MCFM_STEP_DONE;

    MemorycardFile *file = fileList.findById(order[line].id);
    if (file == NULL)
        return MCFM_STEP_DONE;
    if (file->isPrepareIcon() != 0)
        return MCFM_STEP_DONE;
    if (file->readIcon(port, 0) == 0) {     /* 0: the list icon */
        if (file->readCount++ >= MCFM_READ_RETRIES)     /* !retry() */
            fileList.setDamaged(file);
        return MCFM_STEP_BUSY;
    }
    if (file->isPrepareIcon() != 0)
        return MCFM_STEP_BUSY;

    MemorycardFile *victim = fileList.findMaxScoreWithIcon();
    if (victim == NULL)
        return MCFM_STEP_BUSY;
    if (getFileDistance(victim) < getFileDistance(file))
        return MCFM_STEP_BUSY;
    victim->releaseIcon(false);
    return MCFM_STEP_BUSY;
}

/* The twelve one-line overrides are inline in game/menu/memorycard/T109_memorycard_file_manager.h
 * (Lupo 2 has them in .gnu.linkonce.t sections): egcs emits these copies with
 * the vtable, at the end of the unit in reverse slot order. */

// FUNC 0x00149758 gt2k_menu::MemorycardFileManager::getXIconNum
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (4 icons per row; the base: 8);
 * egcs emits this copy */

// FUNC 0x00149760 gt2k_menu::MemorycardFileManager::getFileCount
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (fileCount); egcs emits this copy */

// FUNC 0x00149770 gt2k_menu::MemorycardFileManager::isThreadActive
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (threadActive); egcs emits this copy */

// FUNC 0x00149780 gt2k_menu::MemorycardFileManager::isFormat
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (format); egcs emits this copy */

// FUNC 0x00149790 gt2k_menu::MemorycardFileManager::isReading
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (reading); egcs emits this copy */

// FUNC 0x001497A0 gt2k_menu::MemorycardFileManager::isError
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (error); egcs emits this copy */

// FUNC 0x001497B0 gt2k_menu::MemorycardFileManager::setScreenSize
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (nextScreenSize = n); egcs emits
 * this copy */

// FUNC 0x001497C0 gt2k_menu::MemorycardFileManager::setScreenTop
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (nextScreenTop = i); egcs emits
 * this copy */

// FUNC 0x001497D0 gt2k_menu::MemorycardFileManager::setSelect
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (nextSelect = i); egcs emits this
 * copy */

// FUNC 0x001497E0 gt2k_menu::MemorycardFileManager::clearChangeCard
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (changeCard = 0); egcs emits this
 * copy */

// FUNC 0x001497F0 gt2k_menu::MemorycardFileManager::clearError
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (restartRequest = 1, error = 0);
 * egcs emits this copy */

// FUNC 0x00149808 gt2k_menu::MemorycardFileManager::getPort
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (port); egcs emits this copy */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (si, base MemorycardFileManagerBase). */
// FUNC 0x00149818 __tfQ29gt2k_menu21MemorycardFileManager
extern "C" void *__tfQ29gt2k_menu21MemorycardFileManager(void)
{
    if (__tiQ29gt2k_menu21MemorycardFileManager[0] == 0) {
        __tfQ29gt2k_menu25MemorycardFileManagerBase();
        __rtti_si(__tiQ29gt2k_menu21MemorycardFileManager,
                  "Q29gt2k_menu21MemorycardFileManager",
                  (const type_info *)__tiQ29gt2k_menu25MemorycardFileManagerBase);
    }
    return __tiQ29gt2k_menu21MemorycardFileManager;
}
#endif

/* MemorycardFileManagerBase: every member is inline in
 * game/menu/memorycard/T109_memorycard_file_manager.h (the class has no key function).  egcs
 * emits the vtable 0x0031C498 and these out-of-line copies of the members, in
 * reverse slot order (T079 emits its own copies 0x0015C848..0x0015C928 and
 * vtable 0x003207D0). */

// FUNC 0x00149868 gt2k_menu::MemorycardFileManagerBase::getXIconNum
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (8); egcs emits this copy */

// FUNC 0x00149870 gt2k_menu::MemorycardFileManagerBase::setEffectiveFilter
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (empty); egcs emits this copy */

// FUNC 0x00149878 gt2k_menu::MemorycardFileManagerBase::setFilter
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (empty); egcs emits this copy */

// FUNC 0x00149880 gt2k_menu::MemorycardFileManagerBase::setSort
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (empty); egcs emits this copy */

// FUNC 0x00149888 gt2k_menu::MemorycardFileManagerBase::releaseIcon
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (empty); egcs emits this copy */

// FUNC 0x00149890 gt2k_menu::MemorycardFileManagerBase::restart
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (1); egcs emits this copy */

// FUNC 0x00149898 gt2k_menu::MemorycardFileManagerBase::pause
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (1); egcs emits this copy */

// FUNC 0x001498A0 gt2k_menu::MemorycardFileManagerBase::isThreadActive
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (0); egcs emits this copy */

// FUNC 0x001498A8 gt2k_menu::MemorycardFileManagerBase::endThread
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (empty); egcs emits this copy */

// FUNC 0x001498B0 gt2k_menu::MemorycardFileManagerBase::startThread
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (empty); egcs emits this copy */

// FUNC 0x001498B8 gt2k_menu::MemorycardFileManagerBase::setSelectSubCategory
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (empty); egcs emits this copy */

// FUNC 0x001498C0 gt2k_menu::MemorycardFileManagerBase::setSelectCategory
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (empty); egcs emits this copy */

// FUNC 0x001498C8 gt2k_menu::MemorycardFileManagerBase::setSelectCar
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (empty); egcs emits this copy */

// FUNC 0x001498D0 gt2k_menu::MemorycardFileManagerBase::setSelectCourse
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (empty); egcs emits this copy */

// FUNC 0x001498D8 gt2k_menu::MemorycardFileManagerBase::setScreenSize
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (empty); egcs emits this copy */

// FUNC 0x001498E0 gt2k_menu::MemorycardFileManagerBase::setScreenTop
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (empty); egcs emits this copy */

// FUNC 0x001498E8 gt2k_menu::MemorycardFileManagerBase::setSelect
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (empty); egcs emits this copy */

// FUNC 0x001498F0 gt2k_menu::MemorycardFileManagerBase::isChangeCard
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (0); egcs emits this copy */

// FUNC 0x001498F8 gt2k_menu::MemorycardFileManagerBase::clearChangeCard
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (empty); egcs emits this copy */

// FUNC 0x00149900 gt2k_menu::MemorycardFileManagerBase::updateFileIdForDelete
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (1); egcs emits this copy */

// FUNC 0x00149908 gt2k_menu::MemorycardFileManagerBase::updateFileId
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (1); egcs emits this copy */

// FUNC 0x00149910 gt2k_menu::MemorycardFileManagerBase::clearError
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (empty); egcs emits this copy */

// FUNC 0x00149918 gt2k_menu::MemorycardFileManagerBase::clear
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (empty); egcs emits this copy */

// FUNC 0x00149920 gt2k_menu::MemorycardFileManagerBase::isReading
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (0); egcs emits this copy */

// FUNC 0x00149928 gt2k_menu::MemorycardFileManagerBase::isError
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (0); egcs emits this copy */

// FUNC 0x00149930 gt2k_menu::MemorycardFileManagerBase::isFormat
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (1); egcs emits this copy */

// FUNC 0x00149938 gt2k_menu::MemorycardFileManagerBase::isConnect
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (1); egcs emits this copy */

// FUNC 0x00149940 gt2k_menu::MemorycardFileManagerBase::getPort
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h (0); egcs emits this copy */

// FUNC 0x00149948 gt2k_menu::MemorycardFileManagerBase::~MemorycardFileManagerBase
/* inline in game/menu/memorycard/T109_memorycard_file_manager.h; egcs emits this copy (it
 * stores the base's vtable; the deleting form calls operator delete) */
/* implicit: operator delete (deleting destructor, the inline destructor in the header) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (also called by
 * __tfQ29gt2k_menu25MemorycardDemoFileManager 0x0015C7F8, T079). */
// FUNC 0x00149978 __tfQ29gt2k_menu25MemorycardFileManagerBase
extern "C" void *__tfQ29gt2k_menu25MemorycardFileManagerBase(void)
{
    if (__tiQ29gt2k_menu25MemorycardFileManagerBase[0] == 0)
        __rtti_user(__tiQ29gt2k_menu25MemorycardFileManagerBase,
                    "Q29gt2k_menu25MemorycardFileManagerBase");
    return __tiQ29gt2k_menu25MemorycardFileManagerBase;
}
#endif
