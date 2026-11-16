/* T075 - the game load / save screens of the memory card (MemorycardControl::
 * gameMenu, MODE_GAME_SAVE 0x19, MODE_GAME_LOAD 0x1A .. MODE_GAME_LOAD_GTMODE
 * 0x1D) and the autoload at boot.  0x00156A80..0x00158CD0.
 *
 * The "root" window of menu/memorycard.mbl has one item per memory card
 * slot (MemorycardSlotMenu).  Deciding a slot with a PS2 memory card in it
 * opens the "confirm" window: for a load, OK (WorkingMenu) when the card
 * has the game data ("BASCUS-97102GAMEDATA"), else "confirm_text_nofile";
 * for the save, the space is checked first (the "nospace" dialog) and the
 * texts say whether the game data is overwritten or the card formatted.
 * The WorkingMenu then shows the "working" window (message, warning and the
 * progress bar), loads the game (Serialize "GameStatus" into the control's
 * arg) or saves the new file, and shows "working_text_complete".  A load
 * leaves the screen with result 1; after a save the OK item rebuilds the
 * root window.  The port used is remembered in the GameStatus menu state
 * (gameCardPort / gameCardPort1D / gameCardPort1C by mode): the root window
 * opens on that slot.
 *
 * The autoload (MemorycardAutoLoad) runs the same load on slot 1 at boot
 * when a game file is there.
 *
 * Source order (matching): the two static helpers, then the inline code
 * (the "working" window, the game load, WorkingMenu's and
 * MemorycardSlotMenu's members), then the out-of-line functions.  egcs
 * writes a string literal into .rodata when it expands the function that
 * uses it first, so this order gives the binary's string order (the working
 * window's, "GameStatus", the confirm window's, the root window's); the
 * inline members' code is egcs's deferred output at the end of the object,
 * as in the binary.
 *
 * Includes (data pass): the original's .rodata has, after T075's own type
 * names (MemorycardSlotMenu, WorkingMenu), the dialogs' (T076), T073's
 * (List<MemorycardMenuItem>, MemorycardMenuItem, Cell<MemorycardMenuItem>),
 * MemorycardControl, MemorycardFileManagerBase with List / Cell<
 * MemorycardFile>, the GameStatus's garage lists (wheel list first: T075
 * saw the GameStatus), SmartIndexT<int> and the operator holders (T216),
 * Operator last (0x0031F528..0x0031F9D0).  The first five headers complete
 * those classes in the reverse of that order. */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (MemorycardLayerMenu::root);
                                               OperatorHolder, OperatorList */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (current, popupLevel, layers,
                                               exitRequest); SmartIndexT<int> */
#include "game/save/T429_game_status.h"          /* getGameStatus, gt2k_game::GameStatus::menu; the
                                               garage car and wheel lists */
#include "game/menu/memorycard/T109_memorycard_file_manager.h"  /* MemorycardFileManagerBase, List / Cell<MemorycardFile> */
#include "game/menu/memorycard/T075_memorycard_game_slots.h"    /* gt2k_menu::MemorycardSlotMenu, WorkingMenu,
                                               MemorycardSlotMenuRoot, MemorycardAutoLoad; first
                                               T068 (gt2k_menu::MemorycardControl: MODE_*, STATE_*,
                                               isLoadMode, pauseThreads, restartThreads,
                                               getControl1), T073 (the menu classes,
                                               MemorycardMenuResult) and T076 (the dialogs) */
/* the rest */
#include "libc.h"                           /* free */
#include "libmc.h"                          /* sceMcTypePS2 (mc_slot_type) */
#include <stddef.h>
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/memorycard/T108_memorycard_file.h"      /* gt2k_menu::MemorycardFile, MemorycardGT3File,
                                               MemorycardGameFile, gt2k_game::GameThumbnail */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item (setText, virtual_4) */
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl (decide) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (kill) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem, MenuText */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/save/T427_menu_status.h"          /* gt2k_game::MenuStatus (gameCardPort*) */
#include "engine/input/T474_control.h"              /* Control::getButtonsOn, CONTROL_START */
#include "game/menu/framework/T495_menu_list.h"            /* gt2k_menu::pList::append */
#include "engine/io/T501_memorycard_manager.h"   /* mc_slot_status, mc_slot_type, mc_slot_free,
                                               mc_slot_formatted, mc_format, mc_slot_read_size */
#include "engine/io/T509_serialize.h"            /* Serialize::Base, serialize_create / _destroy */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "game/menu/memorycard/memorycard_game_slots_internal.h"                           /* the unit's constants, the RTTI storage (GCC 14) */

using namespace gt2k_menu;

/* The progress callback (MemorycardProgressFunc) the card operations of this
 * unit get: the control's progress bar goes to `progress'.  (T074 has its
 * own copy, 0x00151AD0, with the same name; T075's is file-local.) */
// FUNC 0x00156A80 memorycard_set_progress
static void memorycard_set_progress(f32 progress, void *ctx)
{
    MemorycardControl *control = (MemorycardControl *)ctx;
    f32 prev = control->progress;

    control->progress = progress;
    control->progressPrev = prev;
}

/* 1 when the card in `port' has the game data, 0 when not, < 0 on an error:
 * a MemorycardGameFile on the stack (sp+0x00) asks the card.  Callers:
 * MemorycardAutoLoad::checkPort and MemorycardSlotMenu::makeSubMenu. */
// FUNC 0x00156A98 memorycard_game_file_exists
static int memorycard_game_file_exists(int port)
{
    MemorycardGameFile file;

    /* implicit: ~MemorycardGT3File (file's implicit destructor) */
    return file.exists(port);
}

namespace gt2k_menu {

/* ------------------------------------------------------------------------ */
/* Inline in the original (expanded in MemorycardAutoLoad::load and         */
/* WorkingMenu::makeSubMenu, the same code in both): the "working" window   */
/* and the game load.                                                       */
/* ------------------------------------------------------------------------ */

/* The "working" window on the caller's stack (sp+0x00..0x0C; our name for
 * the inline helper the two expansions share).  Two inline levels: retail
 * keeps `this' of the members that do the work in a register holding sp
 * (daddu sN, $sp, $zero; sw ..., 4(sN); after the first branch the reads
 * too, `subMenu = ' / `window = ' included), which egcs gives a member
 * called from another inline member; called on the local object itself,
 * `this' folds to sp (as menu_list.h's ListIterator::step).  The outer
 * members (open, complete) only forward here: an exact form, the
 * original's split and names are unknown (behaviour is the same). */
struct MemorycardWorkingWindow {
    MemorycardLayerMenu *menu;                  /* sp+0x00 */
    Item                *message;               /* sp+0x04 "working_text_message" */
    Item                *warning;               /* sp+0x08 "working_text_warning" */
    Transitive          *progressBar;           /* sp+0x0C "working_progress_bar" of layer "hidden",
                                                   NULL if none */

    inline MemorycardLayerMenu *create(MemorycardControl &control);
    inline MemorycardLayerMenu *open(MemorycardControl &control);
    inline void show(PageLayout *layout, bool addOk);
    inline void complete(PageLayout *layout, bool addOk);
};

/* The progress bar starts at 0; the window, not cancelable, is centred on
 * the screen. */
inline MemorycardLayerMenu *MemorycardWorkingWindow::create(MemorycardControl &control)
{
    PageLayout *layout = control.pages.getFragmentManager();
    MenuLayoutItem *bar;

    control.progressPrev = 0.0f;
    control.progress = 0.0f;
    menu = new MemorycardLayerMenu(0, 0, layout, control.pages.getLayer(), "working",
                                   "working_window");
    menu->setPosition((f32)MC_CENTRE_X, (f32)MC_CENTRE_Y);
    message = new Item("working_text_message", layout);
    warning = new Item("working_text_warning", layout);
    progressBar = NULL;
    bar = layout->findObject("working_progress_bar", "hidden");
    if (bar != NULL)
        progressBar = layout->createObject(bar, menu->root, NULL);
    menu->attachObject(message);
    menu->attachObject(warning);
    if (progressBar != NULL)
        menu->attachObject(progressBar);
    return menu;
}

inline MemorycardLayerMenu *MemorycardWorkingWindow::open(MemorycardControl &control)
{
    return create(control);
}

/* Done: the texts close (their fade-out) and "working_text_complete"
 * appears; addOk (after a save): 4 frames, the progress bar goes and an OK
 * item ("working_m_ok", {MC_RESULT_SAVE_DONE}) is added. */
inline void MemorycardWorkingWindow::show(PageLayout *layout, bool addOk)
{
    message->virtual_4(NULL);
    warning->virtual_4(NULL);
    menu->attachObject(new Item("working_text_complete", layout));
    if (addOk) {
        Transitive *parent;

        gfx_vsync(MC_SAVE_WAIT);
        if (progressBar != NULL)
            progressBar->kill();
        parent = menu->root;
        menu->append(new MemorycardCancelItem(layout, parent, "working_m_ok", MC_RESULT_SAVE_DONE));
    }
}

inline void MemorycardWorkingWindow::complete(PageLayout *layout, bool addOk)
{
    show(layout, addOk);
}

/* What memorycard_load_game returns. */
enum {
    MC_LOAD_OK      = 0,                        /* done, or nothing to load; the threads run again */
    MC_LOAD_DAMAGED = 1,                        /* short, not restored (checksum), or not read with
                                                   status 0 / MC_STATUS_NO_ENTRY */
    MC_LOAD_VERSION = 2,                        /* checkHeader: another version */
    MC_LOAD_FAILED  = 3                         /* the card failed */
};

/* Loads the game data of the card in `port' into the control's arg (the
 * GameStatus, Serialize class "GameStatus"; only in the modes 0x1A..0x1D)
 * with the reader threads paused.  On an error the threads stay paused.
 * The file object lives at sp+0x10; its implicit destructor
 * (~MemorycardGT3File 0x001447B0, flags 2) runs on every return. */
static inline int memorycard_load_game(MemorycardControl &control, int port)
{
    const char *className;
    void *data;
    Serialize::Base *object;
    const void *restored;

    control.pauseThreads();
    MemorycardGameFile file;                    /* sp+0x10 */
    file.getDir()->id = MC_DIR_ID_ON_CARD;      /* load() reads only a file on the card */
    className = NULL;
    if (control.mode <= MemorycardControl::MODE_GAME_LOAD_GTMODE) {
        if (control.mode >= MemorycardControl::MODE_GAME_LOAD)
            className = "GameStatus";
    }
    if (className != NULL) {
        data = file.load(port, memorycard_set_progress, &control);
        if (data != NULL) {
            /* 0x10: no room for the header */
            if ((u32)mc_slot_read_size(port) < sizeof(Serialize::Header)) {
                free(data);
                return MC_LOAD_DAMAGED;
            }
            object = serialize_create(className);   /* not checked for NULL */
            if (object->checkHeader(data, control.arg) >= 2) {
                serialize_destroy(object);
                free(data);
                return MC_LOAD_VERSION;
            }
            restored = object->restore(data, control.arg);
            serialize_destroy(object);
            free(data);
            if (restored == NULL)
                return MC_LOAD_DAMAGED;
        } else {
            int status = mc_slot_status(port);
            int damaged = 0;

            if (status == MC_STATUS_NO_ENTRY || status == MC_STATUS_OK)
                damaged = 1;
            return damaged ? MC_LOAD_DAMAGED : MC_LOAD_FAILED;
        }
    }
    control.restartThreads();
    return MC_LOAD_OK;
}

/* ------------------------------------------------------------------------ */
/* WorkingMenu                                                              */
/* ------------------------------------------------------------------------ */

/* The virtual methods of WorkingMenu and MemorycardSlotMenu are inline in
 * the original: egcs emits them as its deferred output, with the typeinfo
 * function and the implicit destructor (no key method; no copy of the
 * inline constructor either), each class's in reverse order of definition
 * (checked). */

/* OK of the confirm window: the "working" window becomes the sub menu.
 * Load screens: the game is loaded from the card (an error: {0xB, the read
 * error dialog}); done is set.  Save screen (0x19): the control's
 * thumbnail (comment emptied) goes into the new file (a MemorycardGT3File),
 * which is saved with the serialised data, formatting the card first when
 * it is not (an error: {0xB, the write error dialog}, the threads stay
 * paused; no GT3 file: nothing saved).  Then sound 7 and
 * "working_text_complete"; after a save, 4 frames, the progress bar goes
 * and an OK item ("working_m_ok", MemorycardCancelItem {MC_RESULT_SAVE_DONE}:
 * the root is rebuilt) is added; after a load, 60 frames.  The port is
 * remembered.  {0, MC_ARG_SUB_MENU_OPENED}. */
// FUNC 0x001583D8 gt2k_menu::WorkingMenu::makeSubMenu
inline MemorycardMenuResult WorkingMenu::makeSubMenu(MemorycardControl &control)
{
    /* Expanded here, as in MemorycardAutoLoad::load: MemorycardWorkingWindow
     * (new MemorycardLayerMenu, setPosition, findObject, createObject,
     * complete) and memorycard_load_game (MemorycardGameFile, load,
     * mc_slot_status, mc_slot_read_size, free, serialize_create,
     * checkHeader, restore, serialize_destroy, ~MemorycardGT3File). */
    /* implicit: operator new, operator delete (MemorycardMenuItem's and
     * Object's: the new expressions and their landing pads),
     * MemorycardMenuItemItem (MemorycardCancelItem's inline constructor),
     * MemorycardLayerMenu, setPosition (the inline MemorycardWorkingWindow),
     * MemorycardGameFile, mc_slot_status, mc_slot_read_size, free,
     * serialize_create, restore, serialize_destroy (the inline
     * memorycard_load_game) */
    /* implicit: findObject, createObject, attachObject, Item, gfx_vsync, kill,
     * append (the inline MemorycardWorkingWindow), MemorycardGT3File::load
     * (file.load in the inline memorycard_load_game) */
    MemorycardWorkingWindow win;                /* sp+0x00 */
    int mode;

    subMenu = win.open(control);
    mode = control.mode;
    if (control.isLoadMode()) {
        switch (memorycard_load_game(control, port)) {
        case MC_LOAD_DAMAGED:
            return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                new MemorycardReadErrorDialog(control, MC_CENTRE_X, MC_CENTRE_Y, 1, 0));
        case MC_LOAD_VERSION:
            return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                new MemorycardReadErrorDialog(control, MC_CENTRE_X, MC_CENTRE_Y, 0, 1));
        case MC_LOAD_FAILED:
            return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                new MemorycardReadErrorDialog(control, MC_CENTRE_X, MC_CENTRE_Y, 0, 0));
        case MC_LOAD_OK:
        default:
            done = true;
            break;
        }
    } else if (mode == MemorycardControl::MODE_GAME_SAVE) {
        int savePort = port;
        bool wasFormatted = formatted;
        /* The original casts from MemorycardFile * (control.newFile's type;
         * __dynamic_cast gets __tfQ29gt2k_menu14MemorycardFile as the
         * source type). */
        MemorycardFile *file;
        MemorycardGT3File *gt3;
        int failed;

        control.pauseThreads();
        file = control.newFile;
        gt3 = dynamic_cast<MemorycardGT3File *>(file);
        if (gt3 != NULL && file != NULL) {
            gt3->setThumbnail(control.thumbnail);
            gt3->getThumbnail()->comment[0] = '\0';
            if (file->save(savePort, control.data, control.dataSize, !wasFormatted,
                           memorycard_set_progress, &control) != 0) {
                control.restartThreads();
                failed = 0;
            } else {
                failed = 4;                     /* (only tested != 0) */
            }
        } else {
            control.restartThreads();
            failed = 0;
        }
        if (failed != 0)
            return MemorycardMenuResult(MC_RESULT_NEXT_MENU,
                new MemorycardWriteErrorDialog(control, MC_CENTRE_X, MC_CENTRE_Y));
    }

    menu_play_se(MENU_SE_CONFIRM);
    win.complete(control.pages.getFragmentManager(), !done);
    if (done)
        gfx_vsync(MC_LOAD_WAIT);

    switch (control.mode) {
    case MemorycardControl::MODE_GAME_SAVE:
    case MemorycardControl::MODE_GAME_LOAD:
    case MemorycardControl::MODE_AUTOLOAD:
        getGameStatus()->getMenuStatus()->gameCardPort = port;
        break;
    case MemorycardControl::MODE_GAME_LOAD_GUEST:
        getGameStatus()->getMenuStatus()->gameCardPort1C = port;
        break;
    case MemorycardControl::MODE_GAME_LOAD_GTMODE:
        getGameStatus()->getMenuStatus()->gameCardPort1D = port;
        break;
    }
    return MemorycardMenuResult(MC_RESULT_NONE, MC_ARG_SUB_MENU_OPENED);
}

/* Once the game is loaded: leave the screen with result 1 ({6, 0}). */
// FUNC 0x001583A0 gt2k_menu::WorkingMenu::select
inline MemorycardMenuResult WorkingMenu::select(MemorycardControl &control)
{
    if (done)
        return MemorycardMenuResult(MC_RESULT_LEAVE, 0);
    return MemorycardLayerMenuItem::select(control);
}

/* ------------------------------------------------------------------------ */
/* MemorycardSlotMenu                                                       */
/* ------------------------------------------------------------------------ */

// FUNC 0x00158300 gt2k_menu::MemorycardSlotMenu::isSelectable
inline bool MemorycardSlotMenu::isSelectable(MemorycardControl &control)
{
    return selectable;
}

/* Each frame: selectable while a PS2 memory card is in the slot (dimmed
 * when not). */
// FUNC 0x001582B0 gt2k_menu::MemorycardSlotMenu::updateItem
inline void MemorycardSlotMenu::updateItem(MemorycardControl &control, bool sel)
{
    connected = mc_slot_type(port) == sceMcTypePS2;
    formatted = mc_slot_formatted(port);
    selectable = connected;
    dim = !connected;
}

/* The card was taken out while the confirm window is open: the window is
 * deleted ({1, MC_ARG_SUB_MENU_CLOSED}).  Else MemorycardLayerMenuItem's
 * select. */
// FUNC 0x00158238 gt2k_menu::MemorycardSlotMenu::select
inline MemorycardMenuResult MemorycardSlotMenu::select(MemorycardControl &control)
{
    if (selectable || subMenu == NULL)
        return MemorycardLayerMenuItem::select(control);
    delete subMenu;
    subMenu = NULL;
    return MemorycardMenuResult(MC_RESULT_USED, MC_ARG_SUB_MENU_CLOSED);
}

/* Decided: nothing ({0, 0}) unless a PS2 memory card is in the slot; else
 * the "confirm" window centred on the screen (cursor on Cancel when there
 * is an OK):
 *   loads (0x1A..0x1D)  makeLoadConfirm: the game data on the card:
 *                       "confirm_text_default", OK (WorkingMenu) / Cancel;
 *                       none (or the card is not formatted):
 *                       "confirm_text_nofile" and "confirm_m_cancel2";
 *                       asking failed: the read error dialog instead
 *   save (0x19)         makeSaveConfirm: with the game data on the card
 *                       already: the new file's size (control.dataSize
 *                       rounded up to KB) minus the old one's must fit in
 *                       the free space less 2 KB, else the "nospace"
 *                       dialog; then "same file exist" / "overwrite" (the
 *                       file marked as on the card).  Without it: a
 *                       formatted card needs the file's size free (else
 *                       "nospace"; a card error gives the read error
 *                       dialog), "confirm_text_default"; an unformatted
 *                       one "format_save" / "approve" (a new file either
 *                       way).  OK (WorkingMenu) / Cancel.  No new file:
 *                       only OK / Cancel.
 * Returns {0, MC_ARG_SUB_MENU_OPENED}.  The two cases are inline helpers
 * (? names) defined before makeSubMenu: their strings come before
 * "confirm_window" in .rodata, and their cleanups (the new expressions'
 * landing pads) at the end of each case, as in the binary; so are the
 * dialogs that replace the window (showReadError / showNoSpace: their
 * landing pads right after each).  Still open: retail allocates the pages
 * pointer (and the save case's KB variable) to $s2 and the new file to
 * $s3, egcs here the other way round (global-alloc priorities 0.25 /
 * 0.273; see ref/matching/u031/notes.md). */
// FUNC 0x00157938 gt2k_menu::MemorycardSlotMenu::makeSubMenu
inline void MemorycardSlotMenu::showReadError(MemorycardControl &control, int damaged)
{
    delete subMenu;
    subMenu = new MemorycardReadErrorDialog(control, MC_CENTRE_X, MC_CENTRE_Y, damaged, 0);
}

inline void MemorycardSlotMenu::showNoSpace(MemorycardControl &control, int needKB)
{
    delete subMenu;
    subMenu = new MemorycardNoSpaceDialog(control, MC_CENTRE_X, MC_CENTRE_Y, needKB);
}

inline void MemorycardSlotMenu::makeLoadConfirm(MemorycardControl &control)
{
    Transitive *parent = subMenu->root;
    PageLayout *layout = control.pages.getFragmentManager();
    int exists = 0;

    if (formatted)
        exists = memorycard_game_file_exists(port);
    if (exists < 0) {
        showReadError(control, 0);
    } else if (exists == 1) {
        subMenu->attachObject(new Item("confirm_text_default", layout));
        subMenu->append(new WorkingMenu(layout, parent, "confirm_m_ok", port, true));
        subMenu->append(new MemorycardCancelItem(layout, parent, "confirm_m_cancel",
                                                 MC_RESULT_CANCEL));
        subMenu->selectLast();
    } else {
        subMenu->attachObject(new Item("confirm_text_nofile", layout));
        subMenu->append(new MemorycardCancelItem(layout, parent, "confirm_m_cancel2",
                                                 MC_RESULT_CANCEL));
    }
}

inline void MemorycardSlotMenu::makeSaveConfirm(MemorycardControl &control)
{
    MemorycardFile *newFile = control.newFile;
    PageLayout *layout = control.pages.getFragmentManager();
    Transitive *parent;

    if (newFile != NULL) {
        int oldKB = -1;

        if (formatted)
            oldKB = newFile->getMainFileKB(port);
        if (oldKB < 0) {
            if (formatted) {
                int status = mc_slot_status(port);

                if (status != MC_STATUS_NO_ENTRY) {
                    showReadError(control, status == MC_STATUS_OK);
                    return;
                }
                oldKB = newFile->sizeKB;
                if (mc_slot_free(port) - MC_FREE_MARGIN_KB < oldKB) {
                    showNoSpace(control, oldKB);
                    return;
                }
            }
            newFile->dir.id = MC_DIR_ID_NEW;            /* saveFiles makes the directory */
            if (formatted) {
                subMenu->attachObject(new Item("confirm_text_default", layout));
            } else {
                subMenu->attachObject(new Item("confirm_text_format_save", layout));
                subMenu->attachObject(new Item("confirm_text_approve", layout));
            }
        } else {
            /* the game data is on the card: overwrite it */
            int needKB = (control.dataSize + 0x3FF) / 0x400 - oldKB;

            if (mc_slot_free(port) - MC_FREE_MARGIN_KB < needKB && needKB > 0) {
                showNoSpace(control, needKB);
                return;
            }
            newFile->dir.id = MC_DIR_ID_ON_CARD;        /* saveFiles rewrites the files only */
            subMenu->attachObject(new Item("confirm_text_same_file_exist", layout));
            subMenu->attachObject(new Item("confirm_text_overwrite", layout));
        }
    }
    parent = subMenu->root;
    subMenu->append(new WorkingMenu(layout, parent, "confirm_m_ok", port, formatted));
    subMenu->append(new MemorycardCancelItem(layout, parent, "confirm_m_cancel", MC_RESULT_CANCEL));
    subMenu->selectLast();
}

inline MemorycardMenuResult MemorycardSlotMenu::makeSubMenu(MemorycardControl &control)
{
    int mode;

    connected = mc_slot_type(port) == sceMcTypePS2;
    formatted = mc_slot_formatted(port);
    if (!connected)
        return MemorycardMenuResult(MC_RESULT_NONE, 0);
    /* implicit: operator new, operator delete (MemorycardMenuItem's and
     * Object's: the new expressions and their landing pads),
     * MemorycardLayerMenuItem, MemorycardMenuItemItem (WorkingMenu's and
     * MemorycardCancelItem's inline constructors) */

    subMenu = new MemorycardLayerMenu(0, 1, control.pages.getFragmentManager(),
                                      control.pages.getLayer(), "confirm", "confirm_window");
    subMenu->setPosition((f32)MC_CENTRE_X, (f32)MC_CENTRE_Y);

    mode = control.mode;
    /* (if tests, not a switch: retail compares 0x1A, 0x1C, 0x1D, 0x1B in turn) */
    if (mode == MemorycardControl::MODE_GAME_LOAD || mode == MemorycardControl::MODE_GAME_LOAD_GUEST
        || mode == MemorycardControl::MODE_GAME_LOAD_GTMODE
        || mode == MemorycardControl::MODE_AUTOLOAD)
        makeLoadConfirm(control);
    else if (mode == MemorycardControl::MODE_GAME_SAVE)
        makeSaveConfirm(control);
    return MemorycardMenuResult(MC_RESULT_NONE, MC_ARG_SUB_MENU_OPENED);
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00158308 __tfQ29gt2k_menu18MemorycardSlotMenu
extern "C" void *__tfQ29gt2k_menu18MemorycardSlotMenu(void)
{
    if (__tiQ29gt2k_menu18MemorycardSlotMenu[0] == 0) {
        __tfQ29gt2k_menu23MemorycardLayerMenuItem();
        __rtti_si(__tiQ29gt2k_menu18MemorycardSlotMenu, "Q29gt2k_menu18MemorycardSlotMenu",
                  (const type_info *)__tiQ29gt2k_menu23MemorycardLayerMenuItem);
    }
    return __tiQ29gt2k_menu18MemorycardSlotMenu;
}
#endif

// FUNC 0x00158358 gt2k_menu::MemorycardSlotMenu::~MemorycardSlotMenu
/* implicit (game/menu/memorycard/T075_memorycard_game_slots.h); egcs emits this copy for the
 * vtable: ~MemorycardLayerMenuItem (0x00151488, flags 0), then operator
 * delete (0x00150870) when flags & 1. */
/* implicit: ~MemorycardLayerMenuItem, operator delete (compiler-generated
 * destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x00158C38 __tfQ29gt2k_menu11WorkingMenu
extern "C" void *__tfQ29gt2k_menu11WorkingMenu(void)
{
    if (__tiQ29gt2k_menu11WorkingMenu[0] == 0) {
        __tfQ29gt2k_menu23MemorycardLayerMenuItem();
        __rtti_si(__tiQ29gt2k_menu11WorkingMenu, "Q29gt2k_menu11WorkingMenu",
                  (const type_info *)__tiQ29gt2k_menu23MemorycardLayerMenuItem);
    }
    return __tiQ29gt2k_menu11WorkingMenu;
}
#endif

// FUNC 0x00158C88 gt2k_menu::WorkingMenu::~WorkingMenu
/* implicit (game/menu/memorycard/T075_memorycard_game_slots.h); egcs emits this copy for the
 * vtable: ~MemorycardLayerMenuItem (0x00151488, flags 0), then operator
 * delete (0x00150870) when flags & 1. */
/* implicit: ~MemorycardLayerMenuItem, operator delete (compiler-generated
 * destructor) */

namespace gt2k_menu {

// FUNC 0x00156B08 gt2k_menu::MemorycardSlotMenuRoot::MemorycardSlotMenuRoot
MemorycardSlotMenuRoot::MemorycardSlotMenuRoot()
{
    root = NULL;
    save = 0;
    insertText = NULL;
    selectText = NULL;
    slot1 = NULL;
    slot2 = NULL;
}

/* Deletes the window (virtual destructor, flags 3); egcs: operator delete
 * (0x0027C0C0) of this when flags & 1. */
// FUNC 0x00156B28 gt2k_menu::MemorycardSlotMenuRoot::~MemorycardSlotMenuRoot
MemorycardSlotMenuRoot::~MemorycardSlotMenuRoot()
{
    /* implicit: operator delete (the deleting destructor, flags & 1) */
    delete root;
}

/* Builds the "root" window (if none is open): the two slots, the texts
 * "insert a card" / "select a card" (hidden until update()), and the cursor
 * on slot 2 when that was the port used last.  The window does not fade
 * out while a confirm window is open (autoHide 0). */
// FUNC 0x00156B88 gt2k_menu::MemorycardSlotMenuRoot::open
void MemorycardSlotMenuRoot::open(MemorycardControl *c, int save)
{
    PageLayout *layout;
    Transitive *parent;

    this->save = save;
    if (root != NULL)
        return;

    layout = c->pages.getFragmentManager();
    /* implicit: operator new, operator delete (MemorycardMenuItem's and
     * Object's: the new expressions and their landing pads),
     * MemorycardLayerMenuItem (MemorycardSlotMenu's inline constructor) */
    root = new MemorycardLayerMenu(0, 1, layout, c->pages.getLayer(), "root", "root_window");
    parent = root->root;
    root->autoHide = 0;

    slot1 = new MemorycardSlotMenu(layout, parent, "root_m_slot1", MC_PORT_SLOT1);
    slot2 = new MemorycardSlotMenu(layout, parent, "root_m_slot2", MC_PORT_SLOT2);
    root->append(slot1);
    root->append(slot2);

    insertText = new Item("root_text_insert_card", layout);
    insertText->visible = 0;
    root->attachObject(insertText);
    selectText = new Item("root_text_select_card", layout);
    selectText->visible = 0;
    root->attachObject(selectText);

    switch (c->mode) {
    case MemorycardControl::MODE_GAME_SAVE:
    case MemorycardControl::MODE_GAME_LOAD:
    case MemorycardControl::MODE_AUTOLOAD:
        if (getGameStatus()->getMenuStatus()->gameCardPort != MC_PORT_SLOT1)
            root->selectLast();
        break;
    case MemorycardControl::MODE_GAME_LOAD_GUEST:
        if (getGameStatus()->getMenuStatus()->gameCardPort1C != MC_PORT_SLOT1)
            root->selectLast();
        break;
    case MemorycardControl::MODE_GAME_LOAD_GTMODE:
        if (getGameStatus()->getMenuStatus()->gameCardPort1D != MC_PORT_SLOT1)
            root->selectLast();
        break;
    }
}

/* One frame of the window.  Its result {code, arg}:
 *   < 2               the window stays (update)
 *   MC_RESULT_SAVE_DONE    result 1, the window is rebuilt (OK after a save)
 *   CANCEL, CARD_GONE, CLOSE   the window is rebuilt
 *   MC_RESULT_NEXT_MENU  arg replaces the window (a dialog); the reader
 *                     threads run again
 *   MC_RESULT_LEAVE   result 1, then as any other code: leave the screen
 *                     (STATE_LEAVE).
 * Then the texts, and in the save screen the start button leaves for the
 * file utility (STATE_FILE_SCREEN). */
// FUNC 0x00156E98 gt2k_menu::MemorycardSlotMenuRoot::update
void MemorycardSlotMenuRoot::update(MemorycardControl *c)
{
    MemorycardMenuResult r;
    int code;

    if (root == NULL)
        return;

    r = root->select(*c);                       /* sp+0x00 */
    code = r.getCode();
    if ((u32)code >= 2) {
        delete root;
        root = NULL;
        if (code == MC_RESULT_SAVE_DONE) {
            c->result = 1;
            open(c, save);
            return;
        }
        if (code == MC_RESULT_CANCEL || code == MC_RESULT_CARD_GONE || code == MC_RESULT_CLOSE) {
            open(c, save);
            return;
        }
        if (code == MC_RESULT_LEAVE) {
            c->result = 1;
        } else if (code == MC_RESULT_NEXT_MENU) {
            root = r.getMenu();
            c->restartThreads();
            return;
        }
        c->state = MemorycardControl::STATE_LEAVE;
        c->pages.exitRequest = 1;
    } else {
        root->update(*c, true);
    }

    if (!c->paused && root != NULL) {
        if (!slot1->selectable && !slot2->selectable) {
            selectText->visible = 0;
            insertText->visible = 1;
        } else {
            selectText->visible = 1;
            insertText->visible = 0;
        }
    } else {
        selectText->visible = 0;
        insertText->visible = 0;
    }

    if ((c->getControl1()->getButtonsOn() & CONTROL_START) && save != 0 && !c->paused) {
        delete root;
        root = NULL;
        c->pages.exitRequest = 1;
        c->state = MemorycardControl::STATE_FILE_SCREEN;
    }
}

// FUNC 0x001570C8 gt2k_menu::MemorycardAutoLoad::MemorycardAutoLoad
MemorycardAutoLoad::MemorycardAutoLoad()
{
    menu = NULL;
}

/* Deletes the dialog (virtual destructor, flags 3); egcs: operator delete
 * (0x0027C0C0) of this when flags & 1. */
// FUNC 0x001570D8 gt2k_menu::MemorycardAutoLoad::~MemorycardAutoLoad
MemorycardAutoLoad::~MemorycardAutoLoad()
{
    /* implicit: operator delete (the deleting destructor, flags & 1) */
    delete menu;
}

/* 1 when the card in `port' is a formatted PS2 memory card with the game
 * data, 0 when not, -1 when asking failed.  (c is not used.) */
// FUNC 0x00157138 gt2k_menu::MemorycardAutoLoad::checkPort
int MemorycardAutoLoad::checkPort(MemorycardControl *c, int port)
{
    int exists;

    if (mc_slot_type(port) != sceMcTypePS2)
        return 0;
    if (mc_slot_formatted(port) == 0)
        return 0;
    exists = memorycard_game_file_exists(port);
    if (exists < 0)
        return -1;
    return exists == 1;
}

/* Loads the game from `port' under the "working" window.  Done: sound 7,
 * the texts give way to "working_text_complete", result 1, the port is
 * remembered, 90 frames, and the window is deleted (menu stays NULL: the
 * page is left).  Else the threads run again and the read error dialog
 * replaces the window.  Returns 1. */
// FUNC 0x00157198 gt2k_menu::MemorycardAutoLoad::load
int MemorycardAutoLoad::load(MemorycardControl *c, int port)
{
    /* Expanded here: MemorycardWorkingWindow (new MemorycardLayerMenu,
     * setPosition, two Items, findObject, createObject, attachObject,
     * complete) and memorycard_load_game (pauseThreads, MemorycardGameFile,
     * load, mc_slot_status, mc_slot_read_size, free, serialize_create,
     * checkHeader, restore, serialize_destroy, restartThreads,
     * ~MemorycardGT3File). */
    /* implicit: operator new, operator delete (MemorycardMenuItem's and
     * Object's: the new expressions and their landing pads),
     * MemorycardGameFile, mc_slot_status, mc_slot_read_size, free,
     * serialize_create, restore, serialize_destroy (the inline
     * memorycard_load_game) */
    /* implicit: setPosition, findObject, createObject, attachObject, Item (the
     * inline MemorycardWorkingWindow), pauseThreads (the inline
     * memorycard_load_game) */
    MemorycardWorkingWindow win;                /* sp+0x00 */
    MemorycardLayerMenu *window;

    window = win.open(*c);
    switch (memorycard_load_game(*c, port)) {
    case MC_LOAD_DAMAGED:
        c->restartThreads();
        delete window;
        menu = new MemorycardReadErrorDialog(*c, MC_CENTRE_X, MC_CENTRE_Y, 1, 0);
        break;
    case MC_LOAD_VERSION:
        c->restartThreads();
        delete window;
        menu = new MemorycardReadErrorDialog(*c, MC_CENTRE_X, MC_CENTRE_Y, 0, 1);
        break;
    case MC_LOAD_FAILED:
        c->restartThreads();
        delete window;
        menu = new MemorycardReadErrorDialog(*c, MC_CENTRE_X, MC_CENTRE_Y, 0, 0);
        break;
    default:
        menu_play_se(MENU_SE_CONFIRM);
        win.complete(c->pages.getFragmentManager(), false);
        c->result = 1;
        getGameStatus()->getMenuStatus()->gameCardPort = port;
        gfx_vsync(MC_AUTOLOAD_WAIT);
        delete window;
        break;
    }
    return 1;
}

/* 0 when slot 1 has nothing to load. */
// FUNC 0x001577B0 gt2k_menu::MemorycardAutoLoad::check
int MemorycardAutoLoad::check(MemorycardControl *c)
{
    return checkPort(c, MC_PORT_SLOT1);
}

/* r: check()'s result.  -1: the read error dialog, else the load from
 * slot 1; nothing left to show leaves the page. */
// FUNC 0x001577D0 gt2k_menu::MemorycardAutoLoad::start
void MemorycardAutoLoad::start(MemorycardControl *c, int r)
{
    /* implicit: operator new, operator delete (MemorycardMenuItem's and
     * Object's: the new expressions and their landing pads) */
    if (r < 0)
        menu = new MemorycardReadErrorDialog(*c, MC_CENTRE_X, MC_CENTRE_Y, 0, 0);
    else
        load(c, MC_PORT_SLOT1);
    if (menu == NULL)
        c->pages.exitRequest = 1;
}

/* One frame of the dialog; once it is left: leave the screen (STATE_LEAVE). */
// FUNC 0x00157880 gt2k_menu::MemorycardAutoLoad::update
void MemorycardAutoLoad::update(MemorycardControl *c)
{
    MemorycardMenuResult r;

    if (menu == NULL)
        return;
    r = menu->select(*c);
    if ((u32)r.code >= 2) {
        delete menu;
        c->state = MemorycardControl::STATE_LEAVE;
        c->pages.exitRequest = 1;
        menu = NULL;
    } else {
        menu->update(*c, true);
    }
}

} /* namespace gt2k_menu */
