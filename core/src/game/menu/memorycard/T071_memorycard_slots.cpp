/* T071 - gt2k_menu::MemorycardSlot / MemorycardSlotSet, the two card slots
 * of the file screens ("file_operation", "demo_loader").
 * 0x0014ED40..0x001504D0.
 *
 * Each slot shows a manager's files (after the file to save, when saving)
 * as a grid of getXIconNum() columns (the icon views of T069 draw it).  One
 * frame (MemorycardSlotSet::update, called by MemorycardControl::
 * fileOperation): while an operation menu is open it gets the input and its
 * result is acted on; else the pad moves the cursor (left / right past the
 * edge goes to the other slot), the info panel follows the selected file,
 * and decide opens the operation menu on it (MemorycardOperationMenu,
 * T074) - or, for the demo replays of the disc, loads the replay at once.
 *
 * MemorycardSlot::init() and the MemorycardSlotSet constructor / destructor
 * are inline in game/menu/memorycard/T071_memorycard_slots.h (F_0014AE00, T068, expands
 * them); MemorycardSlot::getManager is defined in T068.
 *
 * Includes (data pass): retail's T071 .rodata (0x0031D708..0x0031DAB8) is
 * g_00351820's two element names, then the type names of the dialogs
 * (T076), MemorycardControl, GarageCarList with List / GarageCar / Cell<
 * GarageCar>, T073's List / MemorycardMenuItem / Cell<MemorycardMenuItem>,
 * MemorycardFileManagerBase with List / Cell<MemorycardFile>, the operator
 * holders, SmartIndexT<int>, and Operator last: the first seven headers
 * complete those classes in the reverse of that order (MemorycardControl
 * through its class header, which does not bring T075's screens: the
 * dialogs come after it). */
#include "game/common/T251_smart_index.h"          /* (SmartIndexT<int> first) */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive; OperatorHolder, OperatorList */
#include "game/menu/memorycard/T109_memorycard_file_manager.h"  /* gt2k_menu::MemorycardFileManagerBase */
#include "game/menu/memorycard/T073_memorycard_menu.h"      /* gt2k_menu::MemorycardLayerMenu, MemorycardMenuResult */
#include "game/save/T429_garage_car_list.h"      /* (no use: retail's T071 has GarageCarList's names) */
#include "game/menu/memorycard/T068_memorycard_control_class.h" /* gt2k_menu::MemorycardControl (getCurrentPort,
                                               isLoadMode, restartThreads; ControlBase's pad
                                               input control1), MemorycardSlot::getManager */
#include "game/menu/memorycard/T076_memorycard_dialogs.h"   /* (no use: retail's T071 has the dialogs' names) */
/* the rest */
#include "game/menu/memorycard/T071_memorycard_slots.h"
#include "engine/pgl/G001_flare.h"                /* vu0_rand */
#include "game/menu/memorycard/T072_memorycard_icon_sprites.h"  /* gt2k_menu::MemorycardIconSprites */
#include "game/menu/memorycard/T074_memorycard_operation_menu.h"    /* gt2k_menu::MemorycardOperationMenu */
#include "game/menu/memorycard/T108_memorycard_file.h"      /* MemorycardFile and its kinds, ::McDirEntry,
                                               MemorycardIconRenderInfo */
#include "game/race/core/T150_serialize_replay_data.h"    /* g_00351F40 "ReplayData" */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_current; pdistd::Allocator */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item */
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (attachTo, disappearAll, forEach,
                                               kill) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T259_menu_drawable.h"        /* gt2k_menu::Drawable (anchor, anchorType, visible) */
#include "engine/io/T501_memorycard_manager.h"   /* mc_get_dir, mc_slot_dir_count */
#include "engine/io/T509_serialize.h"            /* Serialize::Base::restore, serialize_create /
                                               _destroy */
#include "game/menu/memorycard/memorycard_menu_internal.h"

// DATA_START 0x0031D708

/* The elements of the two slots' icon grids (.sdata; the names open T071's
 * .rodata). */
const char *g_00351820[2] = { "icon_view_slot1", "icon_view_slot2" };  /* 0x00351820 */

/* The comment of the GT3 save the operation menu works on (backupComment /
 * restoreComment), a file static: retail's .bss.  (? size: 0x28 bytes up
 * to T092's g_0049C0E0, the next .bss object; 0x24 as MemorycardControl's
 * comment, whose 8-aligned end is 0x0049C0E0.  The thumbnail's comment it
 * holds is 0x11.) */
static char g_0049C0B8[0x24];               /* 0x0049C0B8 */

namespace gt2k_menu {

/* ======================================================================== */
/* MemorycardSlot                                                           */
/* ======================================================================== */

/* Scrolls the grid: the manager reads the files from the first entry on
 * screen (one less when the file to save is entry 0). */
// FUNC 0x0014ED40 gt2k_menu::MemorycardSlot::setTopRow
void MemorycardSlot::setTopRow(int row)
{
    if (row < 0)
        row = 0;
    topRow = row;
    row *= getManager()->getXIconNum();     /* row now the first entry */
    if (newFile != NULL)
        manager->setScreenTop(row < 1 ? 0 : row - 1);
    else
        manager->setScreenTop(row);
}

/* The cursor, kept inside the entries; the manager's selection is the file
 * under it. */
// FUNC 0x0014EDE8 gt2k_menu::MemorycardSlot::setCursor
void MemorycardSlot::setCursor(int i)
{
    if (i < 0)
        i = 0;
    if (getCount() - 1 < i)
        i = getCount() - 1;
    cursor = i;
    /* (an if with two calls: retail branches where a ?: is if-converted) */
    if (newFile != NULL) {
        if (i > 0)
            manager->setSelect(i - 1);
        else
            manager->setSelect(0);
    } else
        manager->setSelect(i);
}

/* A card is connected without an error and is either not formatted or
 * listed (getFileCount() >= 0). */
// FUNC 0x0014EEA0 gt2k_menu::MemorycardSlot::isReady
bool MemorycardSlot::isReady()
{
    bool ready = false;

    /* one || test: an else-if with its own `ready = true' becomes a
     * store-flag, where retail branches (bltz) */
    if (getManager()->isConnect() && !manager->isError()) {
        if (!getManager()->isFormat() || manager->getFileCount() >= 0)
            ready = true;
    }
    return ready;
}

/* The entries: the files (none on an unformatted card), plus the file to
 * save; -1 while the list is not read. */
// FUNC 0x0014EF58 gt2k_menu::MemorycardSlot::getCount
int MemorycardSlot::getCount()
{
    int files;

    if (getManager()->isFormat())
        files = manager->getFileCount();
    else
        files = 0;
    if (files < 0)
        return -1;
    return files + (newFile != NULL);
}

/* Column col of row row is past the grid or past the last entry. */
// FUNC 0x0014EFC8 gt2k_menu::MemorycardSlot::isOutside
int MemorycardSlot::isOutside(int col, int row)
{
    int i;

    /* the early return (not an if around the rest): its `li v0,1' fills the
     * branch's delay slot without annulling, as in retail */
    if (col >= getManager()->getXIconNum())
        return 1;
    i = row * getManager()->getXIconNum() + col;
    return i >= getCount();
}

/* Each frame: unk1C goes when the card goes; a slot that is not ready
 * starts again from the top, and (unless the reader threads are paused) a
 * failed read, or one the card was pulled out of, is cleared. */
// FUNC 0x0014F058 gt2k_menu::MemorycardSlot::checkCard
void MemorycardSlot::checkCard()
{
    if (!getManager()->isConnect())
        unk1C = 0;
    if (!isReady()) {
        focus = 0;
        topRow = 0;
        cursor = 0;
        if (paused == 0) {
            if ((manager->isReading() && !manager->isConnect()) || manager->isError()) {
                manager->clear();
                manager->clearError();
            }
            focus = 0;
        }
    }
}

/* Entry i: the file to save first when there is one. */
// FUNC 0x0014F148 gt2k_menu::MemorycardSlot::getFile
MemorycardFile *MemorycardSlot::getFile(int i)
{
    if (newFile != NULL) {
        if (i == 0)
            return newFile;
        i--;
    }
    return manager->getFile(i);
}

/* Draws entry i (T069's icon view); the file to save also gets "icon_new"
 * (MemorycardIconSprites::draw). */
// FUNC 0x0014F188 gt2k_menu::MemorycardSlot::renderFile
void MemorycardSlot::renderFile(int i, MemorycardIconRenderInfo &info)
{
    MemorycardFile *file = getFile(i);

    if (file != NULL) {
        file->render(info);
        if (i == 0 && newFile != NULL)
            info.sprites->draw(MC_SPRITE_NEW);
    }
}

// FUNC 0x0014F208 gt2k_menu::MemorycardSlot::isFileSelectable
int MemorycardSlot::isFileSelectable(int i)
{
    MemorycardFile *file = getFile(i);

    if (file == NULL)
        return 0;
    return file->selectable;
}

/* ======================================================================== */
/* MemorycardSlotSet                                                        */
/* ======================================================================== */

/* The file to save (MemorycardControl::begin): entry 0 of both slots. */
// FUNC 0x0014F230 gt2k_menu::MemorycardSlotSet::setNewFile
void MemorycardSlotSet::setNewFile(MemorycardFile *file)
{
    slot[0].newFile = file;
    slot[1].newFile = file;
}

// FUNC 0x0014F240 gt2k_menu::MemorycardSlotSet::setCurrent
void MemorycardSlotSet::setCurrent(int i)
{
    current = i;
    slot[0].focus = (i == 0);
    slot[1].focus = (i == 1);
}

/* The file under the current slot's cursor. */
// FUNC 0x0014F260 gt2k_menu::MemorycardSlotSet::getSelectedFile
MemorycardFile *MemorycardSlotSet::getSelectedFile()
{
    MemorycardSlot *s;

    if (current < 0)
        return NULL;
    s = current == 0 ? &slot[0] : &slot[1];
    return s->getFile(s->cursor);
}

// FUNC 0x0014F298 gt2k_menu::MemorycardSlotSet::getCurrentSlot
MemorycardSlot *MemorycardSlotSet::getCurrentSlot()
{
    if (current < 0)
        return NULL;
    return current == 0 ? &slot[0] : &slot[1];
}

// FUNC 0x0014F2B8 gt2k_menu::MemorycardSlotSet::getCurrentPort
int MemorycardSlotSet::getCurrentPort()
{
    MemorycardSlot *s = getCurrentSlot();

    if (s != NULL)
        return s->getManager()->getPort();
    return -1;
}

// FUNC 0x0014F300 gt2k_menu::MemorycardSlotSet::getLayout
PageLayout *MemorycardSlotSet::getLayout()
{
    return getPageManager()->current;
}

/* getPageManager and getPages are the same code: two ordinary accessors,
 * one called by getLayout, the other by updateInfo (? which original name
 * is which). */
// FUNC 0x0014F320 gt2k_menu::MemorycardSlotSet::getPageManager
PageManager *MemorycardSlotSet::getPageManager()
{
    return &owner->pages;
}

// FUNC 0x0014F330 gt2k_menu::MemorycardSlotSet::getPages
PageManager *MemorycardSlotSet::getPages()
{
    return &owner->pages;
}

/* The current slot's element ("icon_view_slot1" / "icon_view_slot2") as a
 * centre (y up from the bottom of the screen) and size; 0, 0, 1, 1 without
 * one. */
// FUNC 0x0014F340 gt2k_menu::MemorycardSlotSet::getSlotRect
void MemorycardSlotSet::getSlotRect(f32 *x, f32 *y, f32 *w, f32 *h)
{
    MenuLayoutItem *view;

    /* chained: the constants go through $f0 / $f1 (swc1), not `sw $zero' */
    *x = *y = 0.0f;
    *w = *h = 1.0f;
    if (current >= 0) {
        view = getLayout()->findObject(g_00351820[current], NULL);  /* the current
                                               layer */
        if (view != NULL) {
            *x = view->x;
            *y = view->y;
            *w = view->w;
            *h = view->h;
            *x += *w * 0.5f;
            *y = MC_SCREEN_HEIGHT - (*y + *h * 0.5f);
        }
    }
}

/* One frame of the file screens.  The column arithmetic calls
 * getManager()->getXIconNum() at every use, as the original does, and
 * masks with columns - 1 (a power of two: MemorycardFileManager's 4). */
// FUNC 0x0014F440 gt2k_menu::MemorycardSlotSet::update
void MemorycardSlotSet::update()
{
    MemorycardSlot *slot0 = &slot[0];
    MemorycardSlot *slot1 = &slot[1];
    MemorycardSlot *s;
    MemorycardFile *file;
    MemorycardMenuResult result;
    int code;
    int oldCurrent, oldCol, oldRow, row, col, newRow, column, i, usable, changed;
    int screenRow;

    vu0_rand();                             /* a random number each frame, unused here */
    slot0->paused = owner->paused;
    slot1->paused = owner->paused;
    slot0->checkCard();
    slot1->checkCard();
    if (reopen != 0)
        return;

    /* ---- an operation menu is open: it gets the input */
    if (menu != NULL) {
        MemorycardSlot *sel = getCurrentSlot();

        if (sel != NULL)
            sel->menuOpen = 1;
        result = menu->select(*owner);
        code = result.getCode();
        if (code == MC_RESULT_USED) {
            if (result.getArg() == MC_ARG_SUB_MENU_CLOSED)
                restoreComment();
        }
        if ((u32)code >= MC_RESULT_CANCELLED) {
            delete menu;
            menu = NULL;
            /* (an if chain: retail tests the codes one by one, in this
             * order, where a switch gives a jump table) */
            if (code == MC_RESULT_NAME_ENTRY) {     /* the comment: arg 1 runs the name entry */
                reopen = result.getArg();
                if (result.getArg() == 1) {
                    MemorycardSlot *cur = getCurrentSlot();

                    if (cur != NULL) {
                        owner->pages.exitRequest = 1;
                        owner->state = MemorycardControl::STATE_NAME_ENTRY;
                        releaseInfo();
                        cur->manager->clearChangeCard();
                    }
                }
            } else if (code == MC_RESULT_LEAVE) {
                owner->pages.exitRequest = 1;
                owner->state = MemorycardControl::STATE_LEAVE;
                releaseInfo();
                owner->result = 1;
            } else if (code == MC_RESULT_SAVE_DONE) {
                owner->result = 1;
            } else if (code == MC_RESULT_NEXT_MENU) {   /* a dialog replaces the menu */
                slot[0].manager->clear();
                slot[1].manager->clear();
                slot[0].manager->clearError();
                slot[1].manager->clearError();
                menu = result.getMenu();
                owner->restartThreads();
                restoreComment();
            } else if (code != MC_RESULT_COMMENT_DONE && code != MC_RESULT_FILE_DONE) {
                /* cancelled (2, 3, 0xA), the card gone (8), ...: the
                 * comment as it was (a changed comment, 4, or a changed
                 * list, 7, is kept) */
                restoreComment();
            }
        } else {
            menu->update(*owner, true);
        }
        return;
    }

    /* ---- the file list (the pad read through MenuControl's inline
     * accessors: direct reads of up / down and of left / right are merged
     * by fold into 64-bit loads) */
    if (control->control1.getCancel() != 0) {
        owner->pages.exitRequest = 1;
        owner->state = MemorycardControl::STATE_LEAVE;
        releaseInfo();
        return;
    }
    oldCurrent = current;
    s = NULL;
    /* usable: the slot is ready and has an entry.  Retail's form is the
     * nested if setting usable to 1 (not a ?: or a comparison): where a
     * register holds 1 (the loop, current == 1) it stays a movn, elsewhere
     * it becomes slt. */
    if (current >= 0) {
        s = current == 0 ? &slot[0] : &slot[1];
        usable = 0;
        if (s->focus != 0) {
            if (s->isReady()) {
                if (s->getCount() >= 1)
                    usable = 1;
            }
        }
        if (!usable) {
            releaseInfo();
            setCurrent(-1);
        }
    }
    if (current < 0) {
        /* the first slot with entries becomes the current one */
        releaseInfo();
        for (i = 0; i < count; i++) {
            s = i == 0 ? &slot[0] : &slot[1];
            usable = 0;
            if (s->isReady()) {
                if (s->getCount() >= 1)
                    usable = 1;
            }
            if (usable) {
                setCurrent(i);
                break;
            }
        }
        if (current < 0)
            return;
    }

    /* up / down: a row, if there is one */
    oldRow = s->cursor / s->getManager()->getXIconNum();
    if (control->control1.getUp() != 0)
        newRow = oldRow - 1;
    else if (control->control1.getDown() != 0)
        newRow = oldRow + 1;
    else
        newRow = oldRow;
    if (newRow >= 0 && s->getLastRow() >= newRow) {
        column = s->cursor & (s->getManager()->getXIconNum() - 1);
        s->setCursor(column + newRow * s->getManager()->getXIconNum());
    }
    row = s->cursor / s->getManager()->getXIconNum();
    col = s->cursor & (s->getManager()->getXIconNum() - 1);
    oldCol = col;

    /* left / right: past the edge of the grid to the other slot (the same
     * row on screen; column 3 of slot 0 - a literal 3, not getXIconNum() - 1 -
     * or column 0 of slot 1), else kept inside the grid.  The two moves are
     * written out in full: retail shares their tails (cross-jumping). */
    if (control->control1.getLeft() != 0)
        col--;
    else if (control->control1.getRight() != 0)
        col++;
    if (col < 0) {
        if (current == 1) {
            usable = 0;
            if (slot[0].isReady()) {
                if (slot[0].getCount() >= 1)
                    usable = 1;
            }
            col = 0;
            if (usable && count >= 2) {
                screenRow = row - s->topRow;
                s = &slot[0];
                col = 3;
                setCurrent(0);
                newRow = s->topRow + screenRow;
                column = s->cursor & (s->getManager()->getXIconNum() - 1);
                s->setCursor(column + newRow * s->getManager()->getXIconNum());
            }
        } else
            col = 0;
    } else if (s->isOutside(col, row)) {
        if (current == 0) {
            usable = 0;
            if (slot[1].isReady()) {
                if (slot[1].getCount() >= 1)
                    usable = 1;
            }
            if (usable) {
                col = 0;
                if (count >= 2) {
                    screenRow = row - s->topRow;
                    s = &slot[1];
                    setCurrent(1);
                    newRow = s->topRow + screenRow;
                    column = s->cursor & (s->getManager()->getXIconNum() - 1);
                    s->setCursor(column + newRow * s->getManager()->getXIconNum());
                } else
                    col = s->getManager()->getXIconNum() - 1;
            } else
                col = s->getManager()->getXIconNum() - 1;
        } else
            col = s->getManager()->getXIconNum() - 1;
    }

    changed = 0;
    newRow = s->cursor / s->getManager()->getXIconNum();
    s->setCursor(newRow * s->getManager()->getXIconNum() + col);
    col = s->cursor & (s->getManager()->getXIconNum() - 1);
    /* (the row compared is the one before a move to the other slot) */
    if (oldCol != col || oldRow != row || oldCurrent != current)
        changed = 1;
    if (changed || info == NULL)
        updateInfo();
    s->menuOpen = 0;
    if (control->control1.getUp() != 0 || control->control1.getDown() != 0
        || control->control1.getRight() != 0 || control->control1.getLeft() != 0) {
        if (changed)
            menu_play_se(MENU_SE_CURSOR);
        else
            menu_play_se(MENU_SE_REFUSED);
    }

    /* decide */
    file = s->getFile(s->cursor);
    if (control->control1.getDecide() == 0 || file == NULL || file->dir.id == MC_DIR_ID_NOT_READING)
        return;
    if (!file->infoValid())
        return;
    if (dynamic_cast<MemorycardDamageFile *>(file) != NULL && owner->isLoadMode()) {
        menu_play_se(MENU_SE_REFUSED);        /* a damaged file cannot be loaded */
        return;
    }
    menu_play_se(MENU_SE_CONFIRM);
    if (owner->getCurrentPort() == MC_PORT_DEMO_VOLUME) {
        /* a demo replay of the disc: read and restored at once */
        void *data = file->loadFromVolume(NULL, NULL);
        Serialize::Base *replay = serialize_create(g_00351F40);    /* "ReplayData" */
        const void *done = replay->restore(data, owner->arg);

        serialize_destroy(replay);
        free(data);
        if (done != NULL) {
            owner->result = 1;
            owner->state = MemorycardControl::STATE_LEAVE;
            owner->pages.exitRequest = 1;
        }
    } else {
        openMenu(0);
    }
}

/* After an operation (T074's SaveWorkingMenu, CommentChangeWorkingMenu,
 * DeleteWorkingMenu): the info panel again; with nothing selected and no
 * current slot, slot 1's cursor goes to its last entry first. */
// FUNC 0x0014FD30 gt2k_menu::MemorycardSlotSet::resetInfo
void MemorycardSlotSet::resetInfo()
{
    releaseInfo();
    if (getSelectedFile() == NULL && current < 0)
        slot[1].setCursor(slot[1].getCount() - 1);
    updateInfo();
}

/* The info panel of the selected file: the layer the file names, built
 * from a copy of the page's layout (PageLayout::buildLayer) and put under
 * the root object of the current popup level; the previous panel is
 * released (Object::kill). */
// FUNC 0x0014FD98 gt2k_menu::MemorycardSlotSet::updateInfo
void MemorycardSlotSet::updateInfo()
{
    MemorycardFile *file = getSelectedFile();
    Transitive *panel;
    const char *layer;

    if (file == NULL)
        return;
    panel = NULL;
    layer = file->getInfoName();
    if (layer != NULL) {
        PageLayout layout = *getLayout();   /* (a copy on the stack, as the code makes) */
        PageManager *pm;

        panel = layout.buildLayer(layer, 0);
        pm = getPages();
        panel->attachTo(pm->layers[pm->popupLevel]);
    }
    if (info != NULL)
        info->kill();
    info = panel;
}

// FUNC 0x0014FF00 gt2k_menu::MemorycardSlotSet::releaseInfo
void MemorycardSlotSet::releaseInfo()
{
    if (info != NULL)
        info->kill();
    info = NULL;
}

/* Opens the operation menu on the selected file (once its info is read):
 * type 3 for the file to save, 2 for a file that is not a GT3 save or is
 * game data, 0 for a file that passes the screen's filter outside the file
 * utility, else 1 (see T074_memorycard_operation_menu.h).  reopen 0: the
 * comment is kept (backupComment) and the menu goes at iconPos[0..1], moved
 * inside the current slot's rectangle; reopen 1: it goes back where it was. */
// FUNC 0x0014FF38 gt2k_menu::MemorycardSlotSet::openMenu
void MemorycardSlotSet::openMenu(int reopen)
{
    MemorycardFile *file = getSelectedFile();
    MemorycardOperationMenu *m;

    /* The menu is allocated by MemorycardMenuItem::operator new (the menu
     * heap); if its constructor throws, the landing pad gives the block back
     * (MemorycardMenuItem::operator delete). */
    if (file == NULL || !file->infoValid())
        return;
    if (reopen == 0)
        backupComment();
    /* implicit: operator new, operator delete (MemorycardMenuItem's, for the
     * new expressions below and their landing pads) */
    if (file->isNewFile()) {                /* (its store-flag gives retail's nor; bnel) */
        m = new MemorycardOperationMenu(*owner, 3, reopen);
    } else {
        MemorycardGT3File *gt3 = dynamic_cast<MemorycardGT3File *>(file);
        int mode = owner->mode;

        if (gt3 == NULL || dynamic_cast<MemorycardGameFile *>(file) != NULL)
            m = new MemorycardOperationMenu(*owner, 2, reopen);
        else if (file->selectable != 0 && mode != MemorycardControl::MODE_FILE_UTILITY)
            m = new MemorycardOperationMenu(*owner, 0, reopen);
        else
            m = new MemorycardOperationMenu(*owner, 1, reopen);
    }
    menu = m;
    if (m == NULL)
        return;
    if (reopen != 0) {
        m->setPosition(menuX, menuY);
        return;
    }
    {
        f32 x = owner->iconPos[0];
        f32 y = owner->iconPos[1];
        f32 halfW = m->getWidth() * 0.5f;
        f32 halfH = m->getHeight() * 0.5f;
        f32 slotX, slotY, slotHalfW, slotHalfH;

        getSlotRect(&slotX, &slotY, &slotHalfW, &slotHalfH);
        slotHalfW *= 0.5f;
        slotHalfH *= 0.5f;
        if (x - halfW < slotX - slotHalfW)
            x = (halfW + slotX) - slotHalfW;
        if (y - halfH < slotY - slotHalfH)
            y = (halfH + slotY) - slotHalfH;
        if (x + halfW > slotX + slotHalfW)
            x = (-halfW + slotX) + slotHalfW;
        if (y + halfH > slotY + slotHalfH)
            y = (-halfH + slotY) + slotHalfH;
        menu->setPosition(x, y);
        menuX = x;
        menuY = y;
        /* where the confirm windows' icon goes (T074) */
        {
            f32 ix = x + m->iconDX;
            f32 iy = y + m->iconDY;

            owner->iconPos[3] = iy;
            owner->iconPos[2] = ix;
        }
    }
}

/* The start of the page loop: after the name entry (reopen 1, the card not
 * changed) the operation menu comes back where it was, shown at once. */
// FUNC 0x001502C8 gt2k_menu::MemorycardSlotSet::begin
void MemorycardSlotSet::begin()
{
    MemorycardSlot *s;

    releaseInfo();
    s = getCurrentSlot();
    if (s != NULL && s->isReady() && !s->manager->isChangeCard() && reopen == 1) {
        owner->iconPos[0] = owner->iconPos[2];
        owner->iconPos[1] = owner->iconPos[3];
        openMenu(1);
        menu->setShown(1, 0);
        updateInfo();
    }
    reopen = 0;
}

/* The selected GT3 save's comment to g_0049C0B8 ... */
// FUNC 0x00150390 gt2k_menu::MemorycardSlotSet::backupComment
void MemorycardSlotSet::backupComment()
{
    MemorycardGT3File *gt3 = dynamic_cast<MemorycardGT3File *>(getSelectedFile());

    if (gt3 != NULL)
        strcpy(g_0049C0B8, gt3->thumbnail.comment);
}

/* ... and back (the name entry, or a cancelled operation, changed it). */
// FUNC 0x001503F8 gt2k_menu::MemorycardSlotSet::restoreComment
void MemorycardSlotSet::restoreComment()
{
    MemorycardGT3File *gt3 = dynamic_cast<MemorycardGT3File *>(getSelectedFile());

    if (gt3 != NULL)
        strcpy(gt3->thumbnail.comment, g_0049C0B8);
}

/* Returns the word at +0.  No caller, no reference: the same machine code as
 * MemorycardSlot::getManager (0x0014B608, T068's copy, at the end of that
 * TU).  (? T071's own out-of-line copy of getManager, an inline accessor:
 * the only TUs that call getManager, T068, T069 and T071, each have one
 * such function at their end (T069's F_0014DAE0), and every call, T071's
 * included, is bound to T068's copy, as the linker does with weak
 * definitions.  u026's data pass: egcs keeps a call to an inline function
 * in an inline function compiled before the callee's body, and then writes
 * the callee's copy (weak) with the deferred functions; retail's
 * MemorycardSlot probably had in-class helpers such as
 * getManager()->getXIconNum() defined before getManager.  The matching pass
 * called this MemorycardSlotSet::getOwner (MemorycardSlotSet::owner is at
 * +0 too).)  Here an ordinary function in retail's place, between
 * restoreComment and getLastRow: the same code, the same frame info. */
// FUNC 0x00150460 gt2k_menu::F_00150460
MemorycardFileManagerBase *F_00150460(MemorycardSlot *slot)
{
    return slot->manager;
}

/* The row of the last entry. */
// FUNC 0x00150468 gt2k_menu::MemorycardSlot::getLastRow
int MemorycardSlot::getLastRow()
{
    int entries = getCount();

    return (entries - 1) / getManager()->getXIconNum();
}

} /* namespace gt2k_menu */
