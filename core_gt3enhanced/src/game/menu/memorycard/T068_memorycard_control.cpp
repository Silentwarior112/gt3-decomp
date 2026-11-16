/* T068 - gt2k_menu::MemorycardControl, the memory card screens
 * ("menu/memorycard.mbl").  0x001499B8..0x0014B610.
 *
 * F_0014AE00 (main, the arcade / GT-mode menus, the race menus, the logger,
 * the setting screen) builds the controller on the stack over a 384 KB menu
 * heap and runs one mode (MemorycardControl::MODE, T068's header).  begin()
 * picks the file managers (the cards in ports 0 and 1, or the disc's demo
 * replays), the list's sort and, for a save, builds the file to save around
 * the serialised data; then one of three screen loops runs: fileOperation
 * (the file list, T071), gameMenu (the slot menu of the game load / save,
 * T075) or autoLoad (at boot, T075); then end().  The rest are accessors the
 * menus of T069..T076 call.
 *
 * Data (u026's data pass): retail's .rodata opens with <new>'s "bad_alloc"
 * (0x0031C670, unreferenced: the anchor below), then the literals and jump
 * tables of the functions, the vtable and the type names of the classes the
 * original saw, in reverse order of definition: MemorycardControl,
 * List<MemorycardMenuItem>, MemorycardMenuItem, Cell<MemorycardMenuItem>,
 * MemorycardFileManagerBase, OperatorList, List<OperatorHolder>,
 * OperatorHolder, Cell<OperatorHolder>, List / Cell<MemorycardFile>,
 * UnitArena<64>, RegurationResult, RecordsMenuArg, the garage lists,
 * SmartIndexT<int>, Operator.  The headers below complete them in that
 * order (none of T076's dialogs: the class header, not
 * T068_memorycard_control.h); the headers after the class add no type
 * name. */
// DATA_START 0x0031C670
#include <new>                              /* "bad_alloc" (its inline what()), first */
#include "types.h"
#include "libc.h"                           /* malloc, free, memalign, strcpy */
#include <stddef.h>

/* the classes whose type names retail's T068 has, in its order */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager; SmartIndexT<int> (T240) */
#include "game/save/T429_game_status.h"          /* (seen, not used: the garage lists) */
#include "game/menu/records/T089_records_menu_args.h"    /* (seen, not used: RecordsMenuArg) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen, not used: RegurationResult) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/menu/memorycard/T108_memorycard_file.h"      /* the file classes, gt2k_game::GameThumbnail, mcInitIconSys;
                                               List / Cell<MemorycardFile> */
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen, not used: OperatorHolder, OperatorList) */
#include "game/menu/memorycard/T109_memorycard_file_manager.h"  /* gt2k_menu::MemorycardFileManagerBase (SORT),
                                               MemorycardFileManager */
#include "game/menu/memorycard/T073_memorycard_menu.h"      /* (seen, not used: MemorycardMenuItem) */
#include "game/menu/memorycard/T071_memorycard_slots.h"     /* gt2k_menu::MemorycardSlot, MemorycardSlotSet */

namespace gt2k_menu {

/* A slot's port: getOtherPort's `getManager()->getPort()'.  An exact form,
 * not the original (? which had an inline member of MemorycardSlot, defined
 * in the class before getManager: T071's MemorycardSlotSet::getCurrentPort
 * has the same `jal getManager' + getPort).  Compiled before getManager's
 * body, the inline function keeps the call (egcs inlines a function only
 * once its body is compiled; the inlined copy here does not try again), as
 * every caller of getManager in T068, T069 and T071 has it. */
static inline int slot_port(MemorycardSlot *slot)
{
    return slot->getManager()->getPort();
}

/* An inline accessor of T071's MemorycardSlot that egcs emitted out of line
 * at the end of T068 (13 callers in T068, T069, T071; they bind to this
 * copy).  `inline' and defined here, before MemorycardControl's class, for
 * egcs: its out-of-line copy then comes after MemorycardControl's
 * destructor and typeinfo function (deferred functions come out in reverse
 * order of when egcs queued them: getManager here, __tf at the class, the
 * destructor at its definition), as retail's 0x0014B608 does; T071's header
 * declares it without `inline' (the other units call it).  An ordinary
 * definition for GCC 14 (an inline one would not be emitted for the other
 * units' calls). */
#ifdef BOOT_OLD_GCC
#define T068_EGCS_INLINE inline
#else
#define T068_EGCS_INLINE
#endif
// FUNC 0x0014B608 gt2k_menu::MemorycardSlot::getManager
T068_EGCS_INLINE MemorycardFileManagerBase *MemorycardSlot::getManager()
{
    return manager;
}
#undef T068_EGCS_INLINE

} /* namespace gt2k_menu */

#include "game/menu/memorycard/T068_memorycard_control_class.h" /* own: gt2k_menu::MemorycardControl,
                                               MemorycardLoadInfo, F_0014AE00 */

/* the rest (no type names) */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/memorycard/T069_memorycard_items.h"     /* g_002A4E20 (the page's MenuHook table), g_00351808 */
#include "game/menu/memorycard/T070_memorycard_actions.h"   /* g_00351810 (the page's ExecuteFuncData table), g_00351818 */
#include "game/menu/memorycard/T075_memorycard_slot_menu_root.h"    /* gt2k_menu::MemorycardSlotMenuRoot,
                                               MemorycardAutoLoad */
#include "game/menu/memorycard/T077_memorycard_icon.h"      /* icon_heap_assign / _unassign, icon_slots_alloc / _free */
#include "game/app/T079_demo_file_manager.h"    /* gt2k_menu::MemorycardDemoFileManager */
#include "game/menu/race_menu/T102_race_menu_helpers.h"    /* MemCardRaceInfo (a race save's arg) */
#include "game/race/core/T150_serialize_replay_data.h"    /* g_00351F40 "ReplayData", g_00351F54 "GhostData",
                                               g_00351F70 "MaxSpeed" */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_push, menu_heap_pop */
#include "game/menu/framework/T227_text_input.h"           /* run_text_input */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout::setHooks */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase, TRANSITION_FADEOUT */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase (attach, detach, waitFrame) */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_draw_reset, g_0035274C, g_00352750 */
#include "game/data/T489_car_database.h"         /* RACE_KIND_MACHINE_VMAX (a MemCardRaceInfo's licence) */
#include "engine/io/T509_serialize.h"            /* Serialize::Base, serialize_create / _destroy */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#include "game/menu/memorycard/memorycard_control_internal.h"                           /* the unit's constants, the RTTI storage (GCC 14) */

using namespace gt2k_menu;

namespace gt2k_menu {

// FUNC 0x001499B8 gt2k_menu::MemorycardControl::load
void MemorycardControl::load()
{
    pages.loadFile("menu/memorycard.mbl");
}

// FUNC 0x001499E0 gt2k_menu::MemorycardControl::unload
void MemorycardControl::unload()
{
    pages.unloadFile();
}

/* Sets the screens up for `mode': the managers of both slots (the demo
 * replays twice in MODE_DEMO_REPLAY_LOAD: one slot then), the page's hooks,
 * the list's selection (course, car, licence as the category, test + 1 as
 * the sub-category; no test without a licence), its sort, and for the saves
 * the file to save: the object of the class named is serialised into `data'
 * and the file gets the thumbnail (the object's description, plus the
 * licence name and test of the MemCardRaceInfo in arg), the date and its
 * size.  The new file is given to the slots (entry 0 of their lists),
 * except for the game save (the slot menu of T075 saves it). */
// FUNC 0x00149A00 gt2k_menu::MemorycardControl::begin
void MemorycardControl::begin(int mode, u64 race, u64 car, int licence, int test, void *arg)
{
    MemorycardGT3File *file;
    const char *className;

    attach();                               /* ControlBase::attach: the pads */
    waitFrame();
    if (mode == MODE_DEMO_REPLAY_LOAD) {
        demo.buildList();
        manager[0] = &demo;
        manager[1] = &demo;
    } else {
        manager[0] = &files[0];
        manager[1] = &files[1];
    }
    slots.setManagers(manager[0], manager[1]);
    mcInitIconSys();
    pages.initialize(&render.root, NULL);
    pages.setHooks(g_002A4E20, g_00351808, this);
    pages.setExecuteFuncData(g_00351810, g_00351818, this);
    g_00352750 = 0;                         /* no screen clear (T261) */
    g_0035274C = 0;

    if (licence < 0)
        licence = 0;
    test = test + 1;
    if (test < 0 || licence <= 0)
        test = 0;
    file = NULL;
    className = NULL;
    this->arg = arg;
    this->car = car;
    this->licence = licence;
    this->test = test;
    this->race = race;
    manager[0]->setSelectCourse(this->race);
    manager[1]->setSelectCourse(this->race);
    manager[0]->setSelectCar(this->car);
    manager[1]->setSelectCar(this->car);
    manager[0]->setSelectCategory(this->licence);
    manager[1]->setSelectCategory(this->licence);
    manager[0]->setSelectSubCategory(this->test);
    manager[1]->setSelectSubCategory(this->test);

    /* The new files: operator new(0x298) and the constructor (egcs calls
     * operator delete on the landing pad if the constructor throws). */
    /* implicit: operator new, operator delete (the new-expressions below) */
    switch (mode) {
    case MODE_REPLAY_SAVE:
        setSort(MemorycardFileManagerBase::SORT_GAME);
        setEffectiveFilter(false);
        file = new MemorycardReplayFile;
        className = g_00351F40;             /* "ReplayData" */
        break;
    case MODE_GHOST_SAVE:
    case MODE_GHOST_SAVE2:
        setSort(MemorycardFileManagerBase::SORT_GAME);
        setEffectiveFilter(false);
        file = new MemorycardBestlapReplayFile;
        className = g_00351F54;             /* "GhostData" */
        if (this->arg != NULL && ((MemCardRaceInfo *)this->arg)->licence == RACE_KIND_MACHINE_VMAX)
            className = g_00351F70;         /* "MaxSpeed" */
        break;
    case MODE_SETTING_SAVE:
        setSort(MemorycardFileManagerBase::SORT_GAME);
        setEffectiveFilter(false);
        file = new MemorycardSettingFile;
        className = "SettingSerialize";
        break;
    case MODE_GAME_SAVE:
        setSort(MemorycardFileManagerBase::SORT_GAME);
        setEffectiveFilter(false);
        file = new MemorycardGameFile;
        className = "GameStatus";
        break;
    case MODE_DEMO_REPLAY_LOAD:
    case MODE_REPLAY_LOAD:
        setSort(MemorycardFileManagerBase::SORT_REPLAY);
        setEffectiveFilter(true);
        break;
    case MODE_GHOST_LOAD:
        setSort(MemorycardFileManagerBase::SORT_BESTLAP);
        setEffectiveFilter(true);
        break;
    case MODE_LOGGER_LOAD:
        setSort(MemorycardFileManagerBase::SORT_BESTLAP2);
        setEffectiveFilter(true);
        break;
    case MODE_SETTING_LOAD:
        setSort(MemorycardFileManagerBase::SORT_SETTING);
        setEffectiveFilter(true);
        break;
    case MODE_GAME_LOAD:
    case MODE_AUTOLOAD:
    case MODE_GAME_LOAD_GUEST:
    case MODE_GAME_LOAD_GTMODE:
        setSort(MemorycardFileManagerBase::SORT_GAME);
        setEffectiveFilter(true);
        break;
    case MODE_FILE_UTILITY:
        setSort(MemorycardFileManagerBase::SORT_GAME);
        break;
    }

    if (file != NULL) {
        if (className != NULL) {
            Serialize::Base *object;
            MemCardRaceInfo *info;
            int size;
            void *buf;

            newFile = file;
            object = serialize_create(className);   /* not checked for NULL */
            object->describe(&thumbnail);
            size = object->saveSize();
            dataSize = size;
            buf = malloc(size);
            data = buf;
            object->serialize(buf);
            serialize_destroy(object);
            info = (MemCardRaceInfo *)this->arg;
            if (info != NULL) {
                int subCategory = info->test + 1;

                if (subCategory <= 0)
                    strcpy(thumbnail.licenceName, info->licenceName);
                else
                    sprintf(thumbnail.licenceName, "%s%d", info->licenceName, subCategory);
                /* info->licence read twice: PRE gives retail's two
                 * `subCategory = 0' blocks (a local is if-converted). */
                if (subCategory < 0 || info->licence <= 0)
                    subCategory = 0;
                thumbnail.category = info->licence;
                thumbnail.subCategory = subCategory;
            } else {
                thumbnail.licenceName[0] = '\0';
                thumbnail.category = 0;
                thumbnail.subCategory = 0;
            }
            thumbnail.comment[0] = '\0';
            file->setThumbnail(thumbnail);
            file->dir.setTimeNow();
            file->calcSize(dataSize);
        } else {
            delete file;                    /* (no such case: every new file has a name) */
        }
    }
    if (mode != MODE_GAME_SAVE)
        slots.setNewFile(newFile);
    render.attach();
    render.setTransition(true, TRANSITION_FADEOUT);
}

// FUNC 0x0014A088 gt2k_menu::MemorycardControl::end
void MemorycardControl::end()
{
    pages.finalize();
    gfx_vsync(2);
    render.detach();
    detach();                               /* ControlBase::detach */
}

/* The name entry's abort test (run_text_input's abortFn, arg = the
 * controller): nonzero once the card of the current slot is gone. */
// FUNC 0x0014A0C8 name_entry_abort
static int name_entry_abort(void *arg)
{
    MemorycardSlot *slot = ((MemorycardControl *)arg)->getCurrentSlot();

    /* xori 1: isConnect() returns bool (T109). */
    if (slot != NULL)
        return !slot->manager->isConnect();
    return 0;
}

/* The file screens (page "file_operation", "demo_loader" for the demo
 * replays): the slots start again from the top and both managers' reader
 * threads run while the menus of T071 work.  The menus set `state':
 * STATE_NAME_ENTRY runs the name entry for `comment' and comes back,
 * STATE_LEAVE leaves.  Afterwards the threads end and the files' icons and
 * the icon heaps are released. */
// FUNC 0x0014A108 gt2k_menu::MemorycardControl::fileOperation
void MemorycardControl::fileOperation(int mode)
{
    int done;

    this->mode = mode;
    state = STATE_FILE_SCREEN;
    assignIconHeaps();
    done = 0;
    slots.reset();                          /* implicit: releaseInfo (the inline MemorycardSlotSet::reset) */
    manager[0]->startThread();
    manager[1]->startThread();
    do {
        switch (state) {
        case STATE_FILE_SCREEN:
            if (mode != MODE_DEMO_REPLAY_LOAD)
                pages.setPage("file_operation", NULL);
            else
                pages.setPage("demo_loader", NULL);
            pages.startPage(NULL);
            slots.begin();
            while (pages.exited == 0) {
                slots.update();
                pages.update();
                waitFrame();
            }
            pages.closePage();
            break;
        case STATE_NAME_ENTRY:
            run_text_input(&pages, "name_entry", comment, comment, MEMORYCARD_COMMENT_LEN,
                           name_entry_abort, this);
            state = STATE_FILE_SCREEN;
            break;
        case STATE_LEAVE:
            done = 1;
            break;
        }
    } while (done == 0);
    manager[0]->endThread();
    manager[1]->endThread();
    gfx_vsync(2);
    manager[0]->releaseIcon(true);
    manager[1]->releaseIcon(true);
    releaseIconHeaps();
}

/* The game load / save (MODE_GAME_SAVE, the game loads): page "gamesave",
 * "gameload_g" (the guest / GT-mode loads) or "gameload", with the slot
 * menu of T075.  The menus set `state': STATE_FILE_SCREEN runs the file
 * utility and comes back, STATE_LEAVE leaves. */
// FUNC 0x0014A3C0 gt2k_menu::MemorycardControl::gameMenu
void MemorycardControl::gameMenu(int mode)
{
    int done = 0;

    state = STATE_GAME_MENU;
    do {
        switch (state) {
        case STATE_GAME_MENU: {
            int save;

            this->mode = mode;
            switch (mode) {
            case MODE_GAME_SAVE:
                pages.setPage("gamesave", NULL);
                save = 1;
                break;
            case MODE_GAME_LOAD:
            case MODE_AUTOLOAD:
            default:
                pages.setPage("gameload", NULL);
                save = 0;
                break;
            case MODE_GAME_LOAD_GUEST:
            case MODE_GAME_LOAD_GTMODE:
                pages.setPage("gameload_g", NULL);
                save = 0;
                break;
            }
            pages.startPage(NULL);
            slotMenu.open(this, save);
            while (pages.exited == 0) {
                slotMenu.update(this);
                pages.update();
                waitFrame();
            }
            pages.closePage();
            break;
        }
        case STATE_FILE_SCREEN:
            fileOperation(MODE_FILE_UTILITY);
            state = STATE_GAME_MENU;
            break;
        case STATE_LEAVE:
            done = 1;
            break;
        }
    } while (done == 0);
}

/* The autoload at boot (MODE_AUTOLOAD): nothing when MemorycardAutoLoad::
 * check finds nothing to load, else the page "autoload" until it is left.
 * Unlike the other loops this one does not call PageManager::closePage. */
// FUNC 0x0014A590 gt2k_menu::MemorycardControl::autoLoad
void MemorycardControl::autoLoad(int mode)
{
    int found;

    this->mode = mode;
    MemorycardAutoLoad loader;              /* sp+0x00 */
    found = loader.check(this);
    if (found == 0)
        return;
    if (mode == MODE_AUTOLOAD)
        pages.setPage("autoload", NULL);
    pages.startPage(NULL);
    loader.start(this, found);
    while (pages.exited == 0) {
        loader.update(this);
        pages.update();
        waitFrame();
    }
}

/* The selected file of the current slot (NULL if none). */
// FUNC 0x0014A6B8 gt2k_menu::MemorycardControl::getSelectedFile
MemorycardFile *MemorycardControl::getSelectedFile()
{
    return slots.getSelectedFile();
}

// FUNC 0x0014A6E0 gt2k_menu::MemorycardControl::getCurrentSlot
MemorycardSlot *MemorycardControl::getCurrentSlot()
{
    return slots.getCurrentSlot();
}

/* The slot that is not the current one (NULL without a current slot). */
// FUNC 0x0014A708 gt2k_menu::MemorycardControl::getOtherSlot
MemorycardSlot *MemorycardControl::getOtherSlot()
{
    MemorycardSlot *current = getCurrentSlot();
    MemorycardSlotSet *set;

    if (current == NULL)
        return NULL;
    /* Returns, not `other = slot[0]; if (...) other = slot[1];' (which egcs
     * turns into a movn); the base pointer gives retail's &slots + 8 / + 0x2C. */
    set = &slots;
    if (&set->slot[0] == current)
        return &set->slot[1];
    return &set->slot[0];
}

/* The current slot's port, -1 if none. */
// FUNC 0x0014A750 gt2k_menu::MemorycardControl::getCurrentPort
int MemorycardControl::getCurrentPort()
{
    return slots.getCurrentPort();
}

// FUNC 0x0014A778 gt2k_menu::MemorycardControl::getOtherPort
int MemorycardControl::getOtherPort()
{
    MemorycardSlot *other = getOtherSlot();

    if (other == NULL)
        return -1;
    return slot_port(other);                /* implicit: getManager (the inline slot_port) */
}

// FUNC 0x0014A7C0 gt2k_menu::MemorycardControl::getCurrentManager
MemorycardFileManagerBase *MemorycardControl::getCurrentManager()
{
    MemorycardSlot *current = getCurrentSlot();

    if (current == NULL)
        return NULL;
    return current->manager;
}

/* The manager that is not the current slot's: manager[1] when the current
 * one is manager[0], else manager[0] (NULL without a current slot). */
// FUNC 0x0014A7E8 gt2k_menu::MemorycardControl::getOtherManager
MemorycardFileManagerBase *MemorycardControl::getOtherManager()
{
    MemorycardFileManagerBase *current = getCurrentManager();
    MemorycardFileManagerBase *other;

    if (current == NULL)
        return NULL;
    other = manager[0];
    if (other == current)
        other = manager[1];
    return other;
}

/* The saves that make a new file: replay, ghost, setting, game (not
 * MODE_GHOST_SAVE2, which begin() also gives a new file; it has no
 * caller). */
// FUNC 0x0014A830 gt2k_menu::MemorycardControl::isSaveMode
int MemorycardControl::isSaveMode()
{
    switch (mode) {
    case MODE_REPLAY_SAVE:
    case MODE_GHOST_SAVE:
    case MODE_SETTING_SAVE:
    case MODE_GAME_SAVE:
        return 1;
    default:
        return 0;
    }
}

// FUNC 0x0014A888 gt2k_menu::MemorycardControl::isLoadMode
int MemorycardControl::isLoadMode()
{
    switch (mode) {
    case MODE_DEMO_REPLAY_LOAD:
    case MODE_REPLAY_LOAD:
    case MODE_GHOST_LOAD:
    case MODE_LOGGER_LOAD:
    case MODE_SETTING_LOAD:
    case MODE_GAME_LOAD:
    case MODE_AUTOLOAD:
    case MODE_GAME_LOAD_GUEST:
    case MODE_GAME_LOAD_GTMODE:
        return 1;
    default:
        return 0;
    }
}

// FUNC 0x0014A8D0 gt2k_menu::MemorycardControl::setSort
void MemorycardControl::setSort(int sort)
{
    manager[0]->setSort((MemorycardFileManagerBase::SORT)sort);
    manager[1]->setSort((MemorycardFileManagerBase::SORT)sort);
}

/* 1 for the loads (only the matching files are listed), 0 for the saves
 * (begin). */
// FUNC 0x0014A938 gt2k_menu::MemorycardControl::setEffectiveFilter
void MemorycardControl::setEffectiveFilter(bool on)
{
    manager[0]->setEffectiveFilter(on);
    manager[1]->setEffectiveFilter(on);
}

/* Pauses the reader threads that run, waiting a frame at a time until
 * each says it is paused. */
// FUNC 0x0014A9A0 gt2k_menu::MemorycardControl::pauseThreads
void MemorycardControl::pauseThreads()
{
    int active0 = manager[0]->isThreadActive();
    int active1 = manager[1]->isThreadActive();

    if (active0)
        manager[0]->pause();
    if (active1)
        manager[1]->pause();
    while (active0 && manager[0]->pause() == 0)
        gfx_vsync(1);
    while (active1 && manager[1]->pause() == 0)
        gfx_vsync(1);
    paused = true;
}

/* Restarts them the same way (no first request without a wait, unlike
 * pauseThreads). */
// FUNC 0x0014AAD0 gt2k_menu::MemorycardControl::restartThreads
void MemorycardControl::restartThreads()
{
    int active0 = manager[0]->isThreadActive();
    int active1 = manager[1]->isThreadActive();

    while (active0 && manager[0]->restart() == 0)
        gfx_vsync(1);
    while (active1 && manager[1]->restart() == 0)
        gfx_vsync(1);
    paused = false;
}

/* The two icon heaps of T077 over the halves of the work buffer (each half
 * a multiple of 64 bytes), once, if there is a buffer; then T077's two
 * fixed icon slots from them. */
// FUNC 0x0014ABC8 gt2k_menu::MemorycardControl::assignIconHeaps
void MemorycardControl::assignIconHeaps()
{
    if (iconHeaps == 0 && workBuf != NULL && workSize != 0) {
        int half = (workSize / 2) & ~0x3F;
        int heap, offset;

        for (heap = 0, offset = 0; heap < 2; heap++) {
            icon_heap_assign(heap, (u8 *)workBuf + offset, half);
            offset += half;
        }
        icon_slots_alloc();
        iconHeaps = 1;
    }
}

// FUNC 0x0014AC90 gt2k_menu::MemorycardControl::releaseIconHeaps
void MemorycardControl::releaseIconHeaps()
{
    if (iconHeaps != 0) {
        icon_slots_free();
        icon_heap_unassign(1);
        icon_heap_unassign(0);
        iconHeaps = 0;
    }
}

/* One mode: MODE_AUTOLOAD the autoload, the game save / loads the slot
 * menu, anything else the file screens.  Returns `result', which the menus
 * set. */
// FUNC 0x0014ACE0 gt2k_menu::MemorycardControl::run
int MemorycardControl::run(int mode, u64 race, u64 car, int licence, int test, void *arg,
                           MemorycardLoadInfo *loadInfo)
{
    load();
    begin(mode, race, car, licence, test, arg);
    this->loadInfo = loadInfo;
    switch (mode) {
    case MODE_AUTOLOAD:
        autoLoad(mode);
        break;
    case MODE_GAME_SAVE:
    case MODE_GAME_LOAD:
    case MODE_GAME_LOAD_GUEST:
    case MODE_GAME_LOAD_GTMODE:
        gameMenu(mode);
        break;
    default:
        fileOperation(mode);
        break;
    }
    end();
    unload();
    return result;
}

/* Inline: only F_0014AE00 builds one.  ControlBase, the vptr, then the
 * members in order: RenderBase, PageManager(this), the demo manager's inline
 * constructor, the two MemorycardFileManagers, the slots' inline constructor
 * (their init()s), the slot menu; then the body.  iconHeaps last: the store
 * that frees the base register goes first (sched1), as retail's
 * 0x774 does; the others keep this order. */
inline MemorycardControl::MemorycardControl(void *buf, int size)
    : render(), pages(this), demo(), slots(this), slotMenu()
{
    workBuf = buf;
    workSize = size;
    files[0].setPort(0);
    files[1].setPort(1);
    newFile = NULL;
    data = NULL;
    arg = NULL;
    result = 0;
    paused = false;
    iconPos[1] = 0;
    iconPos[0] = 0;
    iconPos[3] = 0;
    iconPos[2] = 0;
    iconPos[5] = 0;
    iconPos[4] = 0;
    comment[0] = '\0';
    iconHeaps = 0;
}

/* Inline (F_0014AE00 expands it); this is the copy for the vtable. */
// FUNC 0x0014B468 gt2k_menu::MemorycardControl::~MemorycardControl
inline MemorycardControl::~MemorycardControl()
{
    if (data != NULL)
        free(data);
    /* (delete tests for NULL itself; an `if' around it adds a second use of
     * the label, CSE1 stops there and F_0014AE00's array loop loses the
     * reuse of &files[0].) */
    delete newFile;
    /* Then the members: the slot menu (~MemorycardSlotMenuRoot, flags 2),
     * the slots (~MemorycardSlotSet: its releaseInfo), files[1] and files[0]
     * (through their vtables, flags 0), the demo manager (flags 2), render's
     * implicit destructor (~RootObject, GameObject::~GameObject, flags 2);
     * then ~ControlBase(this, flags), which does the operator delete. */
    /* implicit: releaseInfo (the inline ~MemorycardSlotSet of slots) */
}

} /* namespace gt2k_menu */

/* The memory card screens.  Frame 0x30870: the controller at sp+0 (0x307A0
 * bytes), the arguments and the heap pointer above it; the menu heap is
 * MEMORYCARD_HEAP_SIZE bytes of memalign(0x40). */
// FUNC 0x0014AE00 F_0014AE00
int F_0014AE00(int mode, u64 race, u64 car, void *arg, void *buf, int size, int licence, int test,
               MemorycardLoadInfo *loadInfo)
{
    void *heap;
    int result;

    menu_draw_reset();
    heap = memalign(0x40, MEMORYCARD_HEAP_SIZE);
    menu_heap_push(heap, MEMORYCARD_HEAP_SIZE);
    {
        /* The inline constructor, expanded: ControlBase::ControlBase, the
         * vptr, RenderBase::RenderBase, PageManager::PageManager(this), the
         * MemorycardDemoFileManager's inline constructor, the two
         * MemorycardFileManager::MemorycardFileManager, the MemorycardSlotSet
         * inline constructor (the two MemorycardSlot::init),
         * MemorycardSlotMenuRoot::MemorycardSlotMenuRoot, then the body
         * (setPort(0), setPort(1), ...). */
        /* implicit: ControlBase, RenderBase, PageManager, MemorycardFileManager, MemorycardSlotMenuRoot, setPort (the inline MemorycardControl constructor), ~MemorycardSlotMenuRoot, releaseInfo, ~MemorycardDemoFileManager, ~RootObject, ~GameObject, ~ControlBase (the inline destructor, and landing pads) */
        MemorycardControl control(buf, size);   /* sp+0x00000 */

        result = control.run(mode, race, car, licence, test, arg, loadInfo);
        gfx_vsync(1);
        /* The inline destructor, expanded (flags 2 for the members and the
         * base): free(data), delete newFile, ~MemorycardSlotMenuRoot,
         * ~MemorycardSlotSet (its releaseInfo), files[1] and files[0]
         * (~MemorycardFileManager through the vtable, flags 0),
         * ~MemorycardDemoFileManager, RenderBase's implicit destructor
         * (RootObject::~RootObject, GameObject::~GameObject), then
         * ControlBase::~ControlBase. */
    }
    menu_heap_pop();
    free(heap);
    return result;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0014B5B8 __tfQ29gt2k_menu17MemorycardControl
extern "C" void *__tfQ29gt2k_menu17MemorycardControl(void)
{
    if (__tiQ29gt2k_menu17MemorycardControl[0] == 0) {
        __tfQ29gt2k_menu11ControlBase();
        __rtti_si(__tiQ29gt2k_menu17MemorycardControl, "Q29gt2k_menu17MemorycardControl",
                  (const type_info *)__tiQ29gt2k_menu11ControlBase);
    }
    return __tiQ29gt2k_menu17MemorycardControl;
}
#endif

/* (MemorycardSlot::getManager, 0x0014B608, egcs's last function here: defined
 * before MemorycardControl's class, see there.) */
