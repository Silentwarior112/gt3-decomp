/* T076_memorycard_dialogs.h - the dialogs of the memory card screens and
 * their cancel item (translation unit T076, unit u031), 0x00158CD0..0x00159EE8.
 *
 * MemorycardErrorDialog is a MemorycardLayerMenu (T073) on the layer `name'
 * of menu/memorycard.mbl with a single item "<name>_m_ok" (a
 * MemorycardCancelItem {0xC}); its kinds add their texts:
 *   MemorycardReadErrorDialog     "readerror" (+ "file damage" or "version
 *                                 error")
 *   MemorycardWriteErrorDialog    "writeerror"
 *   MemorycardNoSpaceDialog       "nospace" (+ "nospace_text_need" with the
 *                                 KB missing)
 *   MemorycardWriteProtectDialog  "writeprotect" (no caller)
 * MemorycardFormatDialog ("format", no caller) asks whether to format the
 * card in a slot; its OK (FormatWorkingMenu) formats it under the
 * "formatworking" window.  MemorycardCancelItem returns its action code when
 * decided; T074 and T075 use it for every Cancel / OK.
 *
 * Every destructor of T076 is implicit (checked with egcs -O2: the code and
 * the order below come out as in the binary).  The implicit destructor of a
 * class with two vptrs (MemorycardMenu's subclasses) stores them, then
 * ~MemorycardLayerMenu (flags 0) and operator delete when flags & 1: so
 * MemorycardErrorDialog's and MemorycardFormatDialog's store their own, and
 * the four kinds' expand ~MemorycardErrorDialog (they store
 * MemorycardErrorDialog's vptrs, not their own).  MemorycardCancelItem's and
 * FormatWorkingMenu's store no vptr.
 *
 * egcs's deferred output (the end of T076, from 0x00159548): per class
 * (FormatWorkingMenu, MemorycardFormatDialog, the four kinds,
 * MemorycardErrorDialog, MemorycardCancelItem: the reverse of the order in
 * which T076's code first uses their vtables), its inline members, its
 * typeinfo function and its implicit destructor.  FormatWorkingMenu::
 * makeSubMenu is there, so it is inline (T076's source defines it `inline';
 * the class has no key method, and egcs emits no copy of its inline
 * constructor), while MemorycardCancelItem::select is out of line (so
 * T076 has the class's inline members: the unreferenced constructor copy
 * 0x00159D78).  After them: T076's copies of MemorycardMenuItem::render
 * 0x00159E68 (inline in T073; for MemorycardCancelItem's vtable [6]), of
 * Cell<MemorycardMenuItem>'s inline destructor 0x00159E70 and of its
 * typeinfo function 0x00159E98 (menu_list.h); all three unreferenced, the
 * vtables use T073's. */
#ifndef GAME_T076_MEMORYCARD_DIALOGS_H
#define GAME_T076_MEMORYCARD_DIALOGS_H

#include "game/game.h"
#include "game/menu/memorycard/T073_memorycard_menu.h"      /* bases gt2k_menu::MemorycardMenuItemItem,
                                               MemorycardLayerMenuItem, MemorycardLayerMenu;
                                               MemorycardMenuResult */

namespace gt2k_menu {
class MemorycardControl;                    /* T068 (references only) */
class Object;                               /* T220 (pointers only) */
class PageLayout;                           /* T229 */

/* The Cancel / OK item of the windows: decided, sound 1 and {action, 0};
 * else {0, 0}.  vtable 0x0031FE18 (7 slots: [1] / [2] pCell's, [4]
 * MemorycardMenuItemItem::isSelectable 0x00151860, [5]
 * MemorycardMenuItemItem::update 0x00150FA8, [6] MemorycardMenuItem::render
 * 0x001519C8).  typeinfo __tfQ29gt2k_menu20MemorycardCancelItem 0x00159DC0
 * (si, base MemorycardMenuItemItem; storage 0x004B1BC0).  Size 0x24
 * (operator new in T074 / T075 / T076).  The destructor stores only
 * Cell<MemorycardMenuItem>'s vptr (the implicit ~MemorycardMenuItemItem /
 * ~MemorycardMenuItem expand Cell's inline destructor), then ~pCell. */
class MemorycardCancelItem : public MemorycardMenuItemItem {
public:
    /* Inline (every maker expands it; 0x00159D78 is the unreferenced
     * out-of-line copy). */
    MemorycardCancelItem(PageLayout *layout, Object *parent, const char *name, int action) /* 0x00159D78 */
        : MemorycardMenuItemItem(layout, parent, name), action(action) {}
    /* [0] +0x0C 0x00159E10 ~MemorycardCancelItem: implicit (no vptr store) */
    virtual MemorycardMenuResult select(MemorycardControl &control); /* [3] +0x24 0x00158CD0 */

    /*  MemorycardMenuItemItem                 0x00 */
    int action;                             /* 0x20 the result code (0xA Cancel, 0xC OK, 5, ...) */
};
EGCS_SIZE(gt2k_menu::MemorycardCancelItem, 0x24);

/* vtables 0x0031FDD8 (primary, 7 slots: [1] / [2] pCell's, [3] / [4] / [5] /
 * [6] MemorycardLayerMenu's 0x00150A88 / 0x00150CA8 / 0x001511C8 /
 * 0x00150D58) and 0x0031FDC0 (the List<MemorycardMenuItem> part, 2 slots,
 * this-delta -0xC: [1] pList's sort 0x0023C580).  typeinfo
 * __tfQ29gt2k_menu21MemorycardErrorDialog 0x00159CC0 (class, base list
 * g_003518C0 {MemorycardLayerMenu, public +0}; storage 0x004B1A20).  Size
 * 0x3C. */
class MemorycardErrorDialog : public MemorycardLayerMenu {
public:
    /* The window of layer name (anchor window), moved with up / down, not
     * cancelable, centred on (x, y), with the item "<name>_m_ok" {0xC}. */
    MemorycardErrorDialog(MemorycardControl &control, int x, int y, const char *name,
                          const char *window); /* 0x00158D28 */
    /* [0] +0x0C 0x00159D18 ~MemorycardErrorDialog: implicit (stores the vptrs
     * 0x0031FDC0 / 0x0031FDD8; the four kinds' implicit destructors expand
     * it) */
};
EGCS_SIZE(gt2k_menu::MemorycardErrorDialog, 0x3C);

/* "readerror".  vtables 0x0031FD80 / 0x0031FD68 (as MemorycardErrorDialog's).
 * typeinfo __tfQ29gt2k_menu25MemorycardReadErrorDialog 0x00159C08 (class,
 * base list g_003518C8 {MemorycardErrorDialog, public +0}; storage
 * 0x004B1A50).  Size 0x3C (operator new in T074 / T075). */
class MemorycardReadErrorDialog : public MemorycardErrorDialog {
public:
    /* damaged: + "readerror_text_file_damage", else versionError: +
     * "readerror_text_version_error". */
    MemorycardReadErrorDialog(MemorycardControl &control, int x, int y, int damaged,
                              int versionError); /* 0x00158EA0 */
    /* [0] +0x0C 0x00159C60 ~MemorycardReadErrorDialog: implicit (expands ~MemorycardErrorDialog) */
};
EGCS_SIZE(gt2k_menu::MemorycardReadErrorDialog, 0x3C);

/* "writeerror".  vtables 0x0031FD28 / 0x0031FD10.  typeinfo
 * __tfQ29gt2k_menu26MemorycardWriteErrorDialog 0x00159B50 (class, base list
 * g_003518C8; storage 0x004B1A40).  Size 0x3C. */
class MemorycardWriteErrorDialog : public MemorycardErrorDialog {
public:
    MemorycardWriteErrorDialog(MemorycardControl &control, int x, int y); /* 0x00159020 */
    /* [0] +0x0C 0x00159BA8 ~MemorycardWriteErrorDialog: implicit (expands ~MemorycardErrorDialog) */
};
EGCS_SIZE(gt2k_menu::MemorycardWriteErrorDialog, 0x3C);

/* "nospace": the card lacks needKB KB.  vtables 0x0031FCD0 / 0x0031FCB8.
 * typeinfo __tfQ29gt2k_menu23MemorycardNoSpaceDialog 0x00159A98 (class, base
 * list g_003518C8; storage 0x004B1A10).  Size 0xBC (operator new in T074 /
 * T075). */
class MemorycardNoSpaceDialog : public MemorycardErrorDialog {
public:
    /* needKB != 0: the element "nospace_text_need" becomes an Item whose
     * text is the element's text (a format) with needKB. */
    MemorycardNoSpaceDialog(MemorycardControl &control, int x, int y, int needKB); /* 0x00159070 */
    /* [0] +0x0C 0x00159AF0 ~MemorycardNoSpaceDialog: implicit (expands ~MemorycardErrorDialog) */

    /*  MemorycardErrorDialog                  0x00 */
    char text[0x80];                        /* 0x3C "nospace_text_need" formatted with needKB */
};
EGCS_SIZE(gt2k_menu::MemorycardNoSpaceDialog, 0xBC);

/* "writeprotect" (no caller of the constructor).  vtables 0x0031FC78 /
 * 0x0031FC60.  typeinfo __tfQ29gt2k_menu28MemorycardWriteProtectDialog
 * 0x001599E0 (class, base list g_003518C8; storage 0x004B1A30).  Size 0x3C. */
class MemorycardWriteProtectDialog : public MemorycardErrorDialog {
public:
    MemorycardWriteProtectDialog(MemorycardControl &control, int x, int y); /* 0x001591D0 */
    /* [0] +0x0C 0x00159A38 ~MemorycardWriteProtectDialog: implicit (expands ~MemorycardErrorDialog) */
};
EGCS_SIZE(gt2k_menu::MemorycardWriteProtectDialog, 0x3C);

/* "format": OK (FormatWorkingMenu) formats the card in port, Cancel {0xC}
 * (no caller of the constructor).  vtables 0x0031FC20 (primary, 7 slots:
 * [4] / [5] / [6] MemorycardLayerMenu's) and 0x0031FC08 (the
 * List<MemorycardMenuItem> part, this-delta -0xC).  typeinfo
 * __tfQ29gt2k_menu22MemorycardFormatDialog 0x00159928 (class, base list
 * g_003518C0 {MemorycardLayerMenu}; storage 0x004B1BA0).  Size 0x40. */
class MemorycardFormatDialog : public MemorycardLayerMenu {
public:
    /* Cancelable, the cursor on Cancel; "format_text_slot1" for port 0,
     * else "format_text_slot2"; centred on (x, y). */
    MemorycardFormatDialog(MemorycardControl &control, int x, int y, int port); /* 0x00159220 */
    /* [0] +0x0C 0x00159980 ~MemorycardFormatDialog: implicit (stores the vptrs
     * 0x0031FC08 / 0x0031FC20) */
    /* {8, 0} once the slot has no PS2 memory card, else MemorycardMenu's. */
    virtual MemorycardMenuResult select(MemorycardControl &control); /* [3] +0x24 0x001594E8 */

    /*  MemorycardLayerMenu                    0x00 */
    int port;                               /* 0x3C */
};
EGCS_SIZE(gt2k_menu::MemorycardFormatDialog, 0x40);

/* OK of the format dialog.  vtable 0x0031FBB0 (10 slots: [3]
 * MemorycardLayerMenuItem::select, [4] MemorycardMenuItemItem::isSelectable,
 * [5] / [6] / [8] / [9] MemorycardLayerMenuItem's).  typeinfo
 * __tfQ29gt2k_menu17FormatWorkingMenu 0x00159890 (si, base
 * MemorycardLayerMenuItem; storage 0x004B1BB0).  Size 0x30 (operator new in
 * the MemorycardFormatDialog constructor). */
class FormatWorkingMenu : public MemorycardLayerMenuItem {
public:
    /* Inline (MemorycardFormatDialog's constructor expands it). */
    FormatWorkingMenu(PageLayout *layout, Object *parent, const char *name, int x, int y, int port)
        : MemorycardLayerMenuItem(layout, parent, name), x(x), y(y), port(port) {}
    /* [0] +0x0C 0x001598E0 ~FormatWorkingMenu: implicit (no vptr store) */
    /* The "formatworking" window at (x, y); the reader threads pause and the
     * card is formatted (failed: {0xB, the write error dialog}); done:
     * sound 7, "formatworking_text_complete" and an OK item {0xC}, the
     * threads run again; {0, 0xA}.  Inline in the original (egcs's deferred
     * output): T076's source defines it `inline', so only T076 may build a
     * FormatWorkingMenu. */
    virtual MemorycardMenuResult makeSubMenu(MemorycardControl &control); /* [7] +0x44 0x00159548 */

    /*  MemorycardLayerMenuItem                0x00 */
    int x;                                  /* 0x24 where the window goes */
    int y;                                  /* 0x28 */
    int port;                               /* 0x2C */
};
EGCS_SIZE(gt2k_menu::FormatWorkingMenu, 0x30);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu17FormatWorkingMenu(void);            /* 0x00159890 */
void *__tfQ29gt2k_menu22MemorycardFormatDialog(void);       /* 0x00159928 */
void *__tfQ29gt2k_menu28MemorycardWriteProtectDialog(void); /* 0x001599E0 */
void *__tfQ29gt2k_menu23MemorycardNoSpaceDialog(void);      /* 0x00159A98 */
void *__tfQ29gt2k_menu26MemorycardWriteErrorDialog(void);   /* 0x00159B50 */
void *__tfQ29gt2k_menu25MemorycardReadErrorDialog(void);    /* 0x00159C08 */
void *__tfQ29gt2k_menu21MemorycardErrorDialog(void);        /* 0x00159CC0 */
void *__tfQ29gt2k_menu20MemorycardCancelItem(void);         /* 0x00159DC0 */
}
#endif

#endif /* GAME_T076_MEMORYCARD_DIALOGS_H */
