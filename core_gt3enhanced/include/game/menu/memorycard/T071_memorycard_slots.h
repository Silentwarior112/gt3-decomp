/* T071_memorycard_slots.h - gt2k_menu::MemorycardSlot / MemorycardSlotSet,
 * the two card slots of the file screens "file_operation" / "demo_loader"
 * (translation unit T071, unit u028), 0x0014ED40..0x001504D0.
 *
 * Each slot shows a manager's files (after the new file when saving) as a
 * grid of getXIconNum() columns, drawn by the icon views of T069.  One frame
 * is MemorycardSlotSet::update (from MemorycardControl::fileOperation, T068):
 * while an operation menu is open it gets the input and its result is acted
 * on; else the pad moves the cursor (left / right past the edge goes to the
 * other slot), the info panel follows the selected file, and decide opens
 * the operation menu on it (MemorycardOperationMenu, T074) - or, for the
 * demo replays of the disc (port 0x100), loads the replay at once.
 *
 * The constructors and ~MemorycardSlotSet are inline in the original:
 * F_0014AE00 (T068) expands them when it builds a MemorycardControl. */
#ifndef GAME_T071_MEMORYCARD_SLOTS_H
#define GAME_T071_MEMORYCARD_SLOTS_H

#include "game/game.h"

namespace gt2k_menu {
class MemorycardFileManagerBase;            /* T109 (pointers only) */
class MemorycardFile;                       /* T108 */
struct MemorycardIconRenderInfo;            /* T108 */
class MemorycardControl;                    /* T068 */
class MemorycardLayerMenu;                  /* T073 */
class Transitive;                           /* T216 */
class PageLayout;                           /* T229 */
class PageManager;                          /* T211 */

/* Not polymorphic.  Size 0x24 (MemorycardSlotSet::slot stride).  The
 * entries are the manager's files, after the new file when saving
 * (newFile); topRow / cursor index them in a grid of getXIconNum() columns. */
struct MemorycardSlot {
    /* No constructor: MemorycardSlotSet's (inline, F_0014AE00) calls init()
     * for each slot; retail has two unrolled runs of stores, where a
     * constructor makes egcs construct the array in a loop.  manager, unk14
     * and menuOpen are left as they are (? name). */
    void init()
    {
        topRow = 0;
        cursor = 0;
        focus = false;
        paused = false;
        unk1C = false;
        newFile = NULL;
    }
    /* Inline (MemorycardSlotSet::reset): back to the top, not focused, not
     * paused (? name). */
    void reset() { topRow = 0; cursor = 0; focus = false; paused = false; }

    MemorycardFileManagerBase *getManager();    /* 0x0014B608 (defined in T068, at its end) manager */
    void setTopRow(int row);                /* 0x0014ED40 >= 0; the manager's screen top (row *
                                               columns, one less with a new file) */
    void setCursor(int i);                  /* 0x0014EDE8 kept inside the entries; the manager's
                                               selection follows */
    bool isReady();                         /* 0x0014EEA0 connected, no error, and unformatted or
                                               listed (bool: T074's CopyMenu::updateItem stores it
                                               as is into its bool selectable) */
    int  getCount();                        /* 0x0014EF58 the files (none if unformatted) + the new
                                               file; -1 while the list is not read */
    int  isOutside(int col, int row);       /* 0x0014EFC8 past the grid or past the last entry */
    void checkCard();                       /* 0x0014F058 each frame: a slot that is not ready starts
                                               again from the top; failed reads cleared */
    MemorycardFile *getFile(int i);         /* 0x0014F148 entry i: the new file first, then the
                                               manager's files */
    void renderFile(int i, MemorycardIconRenderInfo &info); /* 0x0014F188 entry i's icon (the new file
                                               also gets "icon_new") */
    int  isFileSelectable(int i);           /* 0x0014F208 entry i's selectable flag (0 if none) */
    int  getLastRow();                      /* 0x00150468 the row of the last entry */

    MemorycardFileManagerBase *manager;     /* 0x00 */
    int             topRow;                 /* 0x04 the first row on screen */
    int             cursor;                 /* 0x08 the selected entry */
    /* focus, paused and unk1C are bool (4 bytes in egcs): F_0014AE00's
     * slot initialisation (u026) is scheduled as stores of another alias set
     * than the ints topRow / cursor / count; paused is copied from
     * MemorycardControl::paused, also bool (T071's update: lw / sw, no
     * conversion). */
    bool            focus;                  /* 0x0C the current slot: bright, with the cursor (u027: active) */
    bool            paused;                 /* 0x10 MemorycardControl::paused, each frame */
    int             unk14;                  /* 0x14 */
    int             menuOpen;               /* 0x18 the operation menu is open (no cursor frame) */
    bool            unk1C;                  /* 0x1C cleared while no card is connected */
    MemorycardFile *newFile;                /* 0x20 the file to save: entry 0 if set */
};

/* Not polymorphic.  Size 0x6C (MemorycardControl::slots, +0x30628). */
struct MemorycardSlotSet {
    /* Inline (F_0014AE00), as is the destructor.  (The rest assigned in the
     * body, after the slots: an initialiser list orders the stores
     * otherwise.) */
    MemorycardSlotSet(MemorycardControl *c)
        : owner(c), control(c)
    {
        slot[0].init();
        slot[1].init();
        info = NULL;
        current = -1;
        menu = NULL;
        reopen = 0;
        count = 2;
    }
    ~MemorycardSlotSet() { releaseInfo(); }
    /* Inline (T068's MemorycardControl::begin): the slots' managers; one
     * slot when both are the same (the demo replays) (? name). */
    void setManagers(MemorycardFileManagerBase *m0, MemorycardFileManagerBase *m1)
    {
        slot[0].manager = m0;
        slot[1].manager = m1;
        if (m0 == m1)
            count = 1;
    }
    /* Inline (T068's MemorycardControl::fileOperation): the info panel
     * released, no current slot, both slots from the top (? name). */
    void reset()
    {
        releaseInfo();
        current = -1;
        slot[0].reset();
        slot[1].reset();
    }
    /* Inline (T069's memcard_create_slot_title / _icon_view, p8 the slot):
     * &slot[0] or &slot[1] from `this' in a register, a movz (? name). */
    MemorycardSlot *getSlot(int i) { return i == 0 ? &slot[0] : &slot[1]; }

    void setNewFile(MemorycardFile *file);  /* 0x0014F230 entry 0 of both slots */
    void setCurrent(int i);                 /* 0x0014F240 the slots' focus, current = i */
    MemorycardFile *getSelectedFile();      /* 0x0014F260 the file under the current slot's cursor;
                                               NULL if none */
    MemorycardSlot *getCurrentSlot();       /* 0x0014F298 NULL if none */
    int  getCurrentPort();                  /* 0x0014F2B8 its manager's getPort(); -1 if none */
    PageLayout  *getLayout();               /* 0x0014F300 the page manager's current layout */
    /* getPageManager and getPages are the same code, two ordinary
     * accessors (getLayout calls the first, updateInfo the second; ? which
     * original name is which). */
    PageManager *getPageManager();          /* 0x0014F320 &owner->pages */
    PageManager *getPages();                /* 0x0014F330 &owner->pages */
    void getSlotRect(f32 *x, f32 *y, f32 *w, f32 *h);   /* 0x0014F340 the current slot's element
                                               ("icon_view_slot1/2", g_00351820) as a centre (y up
                                               from 448) and size; 0, 0, 1, 1 without one */
    void update();                          /* 0x0014F440 one frame of the file screens */
    void resetInfo();                       /* 0x0014FD30 the info panel again after an operation */
    void updateInfo();                      /* 0x0014FD98 the info panel of the selected file (its
                                               getInfoName layer) */
    void releaseInfo();                     /* 0x0014FF00 */
    void openMenu(int reopen);              /* 0x0014FF38 the operation menu on the selected file */
    void begin();                           /* 0x001502C8 the start of the page loop (reopens the
                                               menu after the name entry) */
    void backupComment();                   /* 0x00150390 the selected GT3 save's comment to
                                               g_0049C0B8 */
    void restoreComment();                  /* 0x001503F8 and back */
    /* (0x00150460, the matching pass's getOwner, is F_00150460 below.) */

    MemorycardControl   *owner;             /* 0x00 */
    MemorycardControl   *control;           /* 0x04 (= owner) its pad input */
    MemorycardSlot       slot[2];           /* 0x08, 0x2C */
    Transitive          *info;              /* 0x50 the info panel of the selected file */
    int                  current;           /* 0x54 the current slot; -1: none */
    MemorycardLayerMenu *menu;              /* 0x58 the open menu (a MemorycardOperationMenu) */
    int                  reopen;            /* 0x5C 1: reopen the menu after the name entry */
    f32                  menuX;             /* 0x60 where the menu was opened */
    f32                  menuY;             /* 0x64 */
    int                  count;             /* 0x68 slots in use: 2, 1 when both are the same manager */
};

/* No callers: slot->manager, the same code as MemorycardSlot::getManager
 * (? T071's out-of-line copy of it, u028's data pass: see T071's source). */
MemorycardFileManagerBase *F_00150460(MemorycardSlot *slot);   /* 0x00150460 */

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::MemorycardSlot, 0x24);
EGCS_SIZE(gt2k_menu::MemorycardSlotSet, 0x6C);

#endif /* GAME_T071_MEMORYCARD_SLOTS_H */
