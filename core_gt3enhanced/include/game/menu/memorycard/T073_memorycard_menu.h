/* T073_memorycard_menu.h - the menu classes of the memory card screens:
 * gt2k_menu::MemorycardMenuItem, MemorycardMenuItemItem,
 * MemorycardLayerMenuItem, MemorycardMenu, MemorycardLayerMenu and the
 * result MemorycardMenuResult (translation unit T073, unit u028),
 * 0x001507F8..0x00151AD0.
 *
 * The operation menus and dialogs of T074..T076 derive from these.  A
 * MemorycardLayerMenu is the objects of one layer of the page's layout with
 * a list of items; each frame the open menu gets select() (the cursor moves
 * between the selectable items; decide on a MemorycardLayerMenuItem makes
 * its sub menu, which then gets the input until it is cancelled) and
 * update() (the items' images, the menu's fade in / out).  Everything is
 * allocated on the menu heap (MemorycardMenuItem's operator new).
 *
 * select() returns a MemorycardMenuResult {code, arg} in $v0.  Codes: 0
 * nothing, 1 the input was used (the menu stays), 2 cancelled; the OK items
 * of T074's working windows give 4 (comment changed), 5 (saved: result 1),
 * 7 (deleted / copied), LoadWorkingMenu 6 (loaded: result 1, leave); 3 /
 * 0xA a confirm window's Cancel, 0xC a dialog's OK (T076), 8 the card went
 * away, 9 comment edit (arg 1 runs the name entry), 0xB the next menu (arg:
 * the menu); {1, 0xB} when a sub menu was cancelled, {0, 0xA} the default
 * makeSubMenu.  T071 (MemorycardSlotSet::update) puts the saved comment
 * back on every code >= 2 but 4, 5, 6, 7 and 9.
 *
 * The slot names (select, isSelectable, update, render, makeSubMenu,
 * updateItem) are the Lupo 2 names of the vtable slots; index.txt's
 * MemorycardLayerMenu::virtual_3 / 4 / 6 (0x00150A88 / 0x00150CA8 /
 * 0x00150D58) are MemorycardMenu's, inherited.  T073 also emits the
 * template members ~List<MemorycardMenuItem> 0x00151720 (vtable
 * 0x0031E048) and ~Cell<MemorycardMenuItem> 0x00151748 (vtable 0x0031E0A0)
 * of menu_list.h.
 *
 * Inline members.  The end of T073 (0x00151770..0x00151AD0) is egcs's
 * deferred output: per class, its inline members, then its typeinfo
 * function (and the implicit destructor): LayerMenuItem updateItem /
 * makeSubMenu, LayerMenu getRoot / setAutoHide, MenuItemItem setBlink /
 * setDim / isSelectable, Menu isActive, MenuItem render / update.  So these
 * ten are defined in the classes below (the unreferenced accessors were
 * inline helpers); MemorycardMenu::isSelectable and MemorycardMenuItemItem::
 * select, among the unit's ordinary functions, are not.  egcs emits the
 * groups in reverse order of the class definitions and each class's
 * inline members in reverse order of declaration: the order below (classes
 * MenuItem, Menu, MenuItemItem, LayerMenu, LayerMenuItem; setDim before
 * setBlink, setAutoHide before getRoot) is the one that gives the binary's
 * (checked with egcs -O2). */
#ifndef GAME_T073_MEMORYCARD_MENU_H
#define GAME_T073_MEMORYCARD_MENU_H

#include "game/game.h"
#include "game/menu/framework/menu_list.h"                 /* bases gt2k_menu::Cell<T>, gt2k_menu::List<T> */
#include <stddef.h>                         /* size_t */

namespace gt2k_menu {
class MemorycardControl;                    /* T068 (references only) */
class Object;                               /* T220 (pointers only) */
class Transitive;                           /* T216 */
class Item;                                 /* T213 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
class MemorycardLayerMenu;

/* MemorycardMenuResult::code: the select protocol of the memory card menus
 * (? names: the units' own, u028..u031 agreed on most).  A code below
 * MC_RESULT_CANCELLED keeps the menu open; T071 (MemorycardSlotSet::update)
 * and T075 delete it on any other code and act on some (T071 puts the saved
 * comment back on every code >= 2 but 4, 5, 6, 7 and 9). */
enum {
    MC_RESULT_NONE           = 0,           /* not handled: the caller goes on (checkDecide, ...) */
    MC_RESULT_USED           = 1,           /* the input was used; the menu stays */
    MC_RESULT_CANCELLED      = 2,           /* cancel on a cancelable menu */
    MC_RESULT_COMMENT_CANCEL = 3,           /* Cancel of the comment change's window */
    MC_RESULT_COMMENT_DONE   = 4,           /* CommentChangeWorkingMenu's OK: the new comment stays */
    MC_RESULT_SAVE_DONE      = 5,           /* SaveWorkingMenu's OK (T075: after a save): result 1 */
    MC_RESULT_LEAVE          = 6,           /* a load is done: result 1, leave the screen */
    MC_RESULT_FILE_DONE      = 7,           /* Delete / CopyWorkingMenu's OK: the list changed */
    MC_RESULT_CARD_GONE      = 8,           /* the slot has no usable PS2 memory card any more */
    MC_RESULT_NAME_ENTRY     = 9,           /* arg 1: run the name entry for the save's comment */
    MC_RESULT_CANCEL         = 0xA,         /* Cancel of a confirm window (T071: the comment restored) */
    MC_RESULT_NEXT_MENU      = 0xB,         /* menu: the menu that replaces this one (a dialog) */
    MC_RESULT_CLOSE          = 0xC          /* a dialog's OK (T076; the format dialog's Cancel too) */
};
/* MemorycardMenuResult::arg of the results with no menu: {MC_RESULT_NONE,
 * MC_ARG_SUB_MENU_OPENED} from makeSubMenu (the default one's too),
 * {MC_RESULT_USED, MC_ARG_SUB_MENU_CLOSED} when a MemorycardLayerMenuItem's
 * sub menu was cancelled (T071 then restores the comment). */
enum {
    MC_ARG_SUB_MENU_OPENED = 0xA,
    MC_ARG_SUB_MENU_CLOSED = 0xB
};

/* What select() and its helpers return (8 bytes, in $v0; ? name).  8-byte
 * aligned with an inline constructor: the functions build it in a stack
 * temporary (sw code; sw arg) and return it with one ld $v0, which egcs
 * does only for an 8-aligned struct whose address is taken (a 4-aligned one
 * is returned through shifts and masks; a constructor-less 8-aligned one is
 * folded into a constant).  The default constructor leaves it unset;
 * arg's default 0 is a convenience (every T073 return passes both). */
struct MemorycardMenuResult {
    MemorycardMenuResult() {}
    MemorycardMenuResult(int c, int a = 0) : code(c), arg(a) {}
    /* {MC_RESULT_NEXT_MENU, menu} (u031: T075 / T076 build it after the new
     * dialog: sw menu; sw code; ld, as this constructor gives). */
    MemorycardMenuResult(int c, MemorycardLayerMenu *m) : code(c), menu(m) {}
    /* (? names) Inline accessors: MemorycardSlotSet::update (u028,
     * 0x0014F440) reads the result through them; their `this' keeps the
     * local in memory (sd $v0 after the call, then lw of code and of arg),
     * where direct field reads take code out of $v0 (dsll32 / dsra32). */
    int getCode() const { return code; }
    int getArg() const { return arg; }
    MemorycardLayerMenu *getMenu() const { return menu; }

    int code;                               /* 0x0 (u029 / u030: action) */
    union {
        int                  arg;           /* 0x4 */
        MemorycardLayerMenu *menu;          /* 0x4 code 0xB: the next menu (u030: dialog) */
    };
} __attribute__((aligned(8)));

/* vtable 0x0031E060 (7 slots: [1] / [2] pCell's, [3] / [4]
 * __pure_virtual).  typeinfo __tfQ29gt2k_menu18MemorycardMenuItem
 * 0x001519D8 (si, base Cell<MemorycardMenuItem>; storage 0x004B19A0).
 * vptr +0x08 (pCell's); size 0xC.  Abstract. */
class MemorycardMenuItem : public Cell<MemorycardMenuItem> {
public:
    static void *operator new(size_t size);     /* 0x001507F8 the current menu heap (T204), 16-aligned;
                                                   on failure the heap's debug dump */
    static void *operator new[](size_t size);   /* 0x00150850 (no callers) */
    static void  operator delete(void *p);      /* 0x00150870 */
    static void  operator delete[](void *p);    /* 0x001508A0 (no callers) */

    /* [0] +0x0C 0x00151A28 ~MemorycardMenuItem: implicit (no vptr store) */
    /*  [1] +0x14 / [2] +0x1C pCell's virtual_1 / virtual_2 */
    virtual MemorycardMenuResult select(MemorycardControl &ctl) = 0;    /* [3] +0x24 */
    virtual bool isSelectable(MemorycardControl &ctl) = 0;             /* [4] +0x2C */
    virtual void update(MemorycardControl &ctl, bool sel) {}           /* [5] +0x34 0x001519D0 */
    virtual void render(MemorycardControl &ctl, bool sel) {}           /* [6] +0x3C 0x001519C8 */
};

/* vtables 0x0031E008 (primary, 7 slots) and 0x0031DFF0 (List at +0x0C,
 * this-delta -12, 2 slots: [1] pList's sort).  typeinfo
 * __tfQ29gt2k_menu14MemorycardMenu 0x00151918 (__class_type_info, storage
 * 0x004B1A90; bases g_0031E158: MemorycardMenuItem at 0, List at 0xC).
 * Size 0x28.  A list of items with a cursor; the items are not deleted
 * with it. */
class MemorycardMenu : public MemorycardMenuItem, public List<MemorycardMenuItem> {
public:
    MemorycardMenu(int vertical, int cancelable);   /* 0x001508C0 active 1, no current item */
    virtual ~MemorycardMenu();                  /* [0] +0x0C 0x00150920 */
    /* The current item gets the input first (code != 0: returned, the menu
     * not active); else cancel gives {2, 0} if cancelable, the directions
     * move to the previous / next selectable item (sounds 3 / 4); {1, 0}. */
    virtual MemorycardMenuResult select(MemorycardControl &ctl);        /* [3] +0x24 0x00150A88 */
    virtual bool isSelectable(MemorycardControl &ctl);                 /* [4] +0x2C 0x00150CA8 true */
    virtual void update(MemorycardControl &ctl, bool sel);              /* [5] +0x34 0x00150CB0 every item
                                                   (sel only for the current one) */
    virtual void render(MemorycardControl &ctl, bool sel);              /* [6] +0x3C 0x00150D58 the same */
    MemorycardMenuItem *nextSelectable(MemorycardControl &ctl, MemorycardMenuItem *item);  /* 0x001509A8
                                                   NULL if none */
    MemorycardMenuItem *prevSelectable(MemorycardControl &ctl, MemorycardMenuItem *item);  /* 0x00150A10 */
    void selectLast();                          /* 0x00150A78 the cursor on the last item */
    int  isActive() { return active; }          /* 0x00151910 (no callers) */

    /*    MemorycardMenuItem                       0x00 .. 0x0C (vptr +0x08) */
    /*    List<MemorycardMenuItem>                 0x0C .. 0x18 (head, tail, vptr +0x14) */
    int                 vertical;               /* 0x18 up / down move the cursor (else left / right) */
    int                 cancelable;             /* 0x1C cancel returns {2, 0} */
    MemorycardMenuItem *current;                /* 0x20 the cursor's item (u029 / u030: cursor) */
    int                 active;                 /* 0x24 0 while the current item used the input */
};

/* vtable 0x0031DFB0 (7 slots: [6] MemorycardMenuItem's).  typeinfo
 * __tfQ29gt2k_menu22MemorycardMenuItemItem 0x00151868 (si, base
 * MemorycardMenuItem; storage 0x004B1A60).  Size 0x20.  Two images of the
 * layout, name and name + "_sel", under parent: the first when not
 * selected (always, while blinking), the second when selected (its alpha
 * blinking). */
class MemorycardMenuItemItem : public MemorycardMenuItem {
public:
    /* Both images hidden until the first update, placed relative to parent
     * (anchor type 3); the "_sel" one a step in front. */
    MemorycardMenuItemItem(PageLayout *layout, Object *parent, const char *name);  /* 0x00150E00 */
    /* [0] +0x0C 0x001518B8 ~MemorycardMenuItemItem: implicit (no vptr store;
     * the images belong to the object tree) */
    virtual MemorycardMenuResult select(MemorycardControl &ctl);        /* [3] +0x24 0x00150F90 {0, 0} */
    virtual bool isSelectable(MemorycardControl &ctl) { return true; } /* [4] +0x2C 0x00151860 */
    virtual void update(MemorycardControl &ctl, bool sel);              /* [5] +0x34 0x00150FA8 the
                                                   images' visibility and alpha (0.5; 0.1 dimmed) */
    void setDim(int on) { dim = on; }           /* 0x00151858 (no callers) */
    void setBlink(int on) { blink = on; }       /* 0x00151850 (no callers) */

    /*    Cell<MemorycardMenuItem>                 0x00 .. 0x0C (vptr +0x08) */
    Item *images[2];                            /* 0x0C name, 0x10 name + "_sel" (u029: normal, selected) */
    int   dim;                                  /* 0x14 alpha 0.1 instead of 0.5 (not selectable) */
    f32   blinkTime;                            /* 0x18 1 .. 0, -1/30 a frame while selected */
    int   blink;                                /* 0x1C */
};

/* vtables 0x0031DF70 (primary, 7 slots: [3] / [4] / [6] MemorycardMenu's)
 * and 0x0031DF58 (2 slots).  typeinfo __tfQ29gt2k_menu19MemorycardLayerMenu
 * 0x001517F8 (__class_type_info, storage 0x004B1A70; base list g_00351840).
 * Size 0x3C.  The objects of layer `name' of the layout under parent,
 * placed by the element `anchorName' of that layer; fades out while not
 * active (autoHide), in while active. */
class MemorycardLayerMenu : public MemorycardMenu {
public:
    MemorycardLayerMenu(int vertical, int cancelable, PageLayout *layout, Object *parent,
                        const char *name, const char *anchorName);     /* 0x00151050 */
    virtual ~MemorycardLayerMenu();             /* [0] +0x0C 0x00151158 root->disappearAll(NULL): the
                                                   layer's objects run their default close scripts
                                                   (which release them) */
    virtual void update(MemorycardControl &ctl, bool sel);              /* [5] +0x34 0x001511C8 the items,
                                                   then the fade */
    void setShown(int show, int animate);       /* 0x001512A0 the fade of g_0031DD80 (animate) or the
                                                   alpha at once */
    void setPosition(f32 x, f32 y);             /* 0x00151340 the anchor's centre at (x, y), y up from
                                                   448 */
    void setPriority(f32 z);                    /* 0x001513B0 root's depth */
    f32  getWidth();                            /* 0x001513C0 the anchor's; 1 without one */
    f32  getHeight();                           /* 0x001513E0 */
    void attachObject(Transitive *obj);         /* 0x00151400 under root, placed relative to it */
    void setAutoHide(int on) { autoHide = on; } /* 0x001517F0 (no callers) */
    Transitive *getRoot() { return root; }      /* 0x001517E8 (no callers) */

    PageLayout     *layout;                     /* 0x28 */
    Transitive     *root;                       /* 0x2C the layer's objects, parent of the items (u029: layer) */
    MenuLayoutItem *anchor;                     /* 0x30 anchorName's element, NULL: none (u029: window) */
    int             shown;                      /* 0x34 */
    int             autoHide;                   /* 0x38 fade out while not active (1) */
};

/* vtable 0x0031DF00 (10 slots: [4] MemorycardMenuItemItem's).  typeinfo
 * __tfQ29gt2k_menu23MemorycardLayerMenuItem 0x00151798 (si, base
 * MemorycardMenuItemItem; storage 0x004B1A80).  Size 0x24.  An item that
 * opens a sub menu (makeSubMenu, the derived classes of T074 / T075 / T076)
 * when decided; the sub menu then gets the input. */
class MemorycardLayerMenuItem : public MemorycardMenuItemItem {
public:
    MemorycardLayerMenuItem(PageLayout *layout, Object *parent, const char *name);   /* 0x00151450 */
    virtual ~MemorycardLayerMenuItem();         /* [0] +0x0C 0x00151488 deletes subMenu */
    /* Without a sub menu: checkDecide.  With one: its result, except that a
     * cancelled sub menu (code 2) is deleted and gives {1, 0xB}. */
    virtual MemorycardMenuResult select(MemorycardControl &ctl);        /* [3] +0x24 0x00151510 */
    virtual void update(MemorycardControl &ctl, bool sel);              /* [5] +0x34 0x00151650 the sub
                                                   menu (no blinking meanwhile), updateItem, the images */
    virtual void render(MemorycardControl &ctl, bool sel);              /* [6] +0x3C 0x001516E8 the sub
                                                   menu's */
    virtual MemorycardMenuResult makeSubMenu(MemorycardControl &ctl)    /* [7] +0x44 0x00151778 */
    {
        return MemorycardMenuResult(0, 0xA);    /* nothing opened */
    }
    virtual MemorycardMenuResult checkDecide(MemorycardControl &ctl);   /* [8] +0x4C 0x001515C8 decide on
                                                   a selectable item: sound 7, makeSubMenu (index.txt:
                                                   virtual_8) */
    virtual void updateItem(MemorycardControl &ctl, bool sel) {}       /* [9] +0x54 0x00151770 */

    /* u028 typed it MemorycardMenuItem *; every makeSubMenu (T074..T076)
     * stores a MemorycardLayerMenu and uses it as one. */
    MemorycardLayerMenu *subMenu;               /* 0x20 NULL: none */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::MemorycardMenuResult, 0x8);
EGCS_SIZE(gt2k_menu::MemorycardMenuItem, 0xC);
EGCS_SIZE(gt2k_menu::MemorycardMenuItemItem, 0x20);
EGCS_SIZE(gt2k_menu::MemorycardLayerMenuItem, 0x24);
EGCS_SIZE(gt2k_menu::MemorycardMenu, 0x28);
EGCS_SIZE(gt2k_menu::MemorycardLayerMenu, 0x3C);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu23MemorycardLayerMenuItem(void);                  /* 0x00151798 */
void *__tfQ29gt2k_menu19MemorycardLayerMenu(void);                      /* 0x001517F8 */
void *__tfQ29gt2k_menu22MemorycardMenuItemItem(void);                   /* 0x00151868 */
void *__tfQ29gt2k_menu14MemorycardMenu(void);                           /* 0x00151918 */
void *__tfQ29gt2k_menut4List1ZQ29gt2k_menu18MemorycardMenuItem(void);   /* 0x00151978 */
void *__tfQ29gt2k_menu18MemorycardMenuItem(void);                       /* 0x001519D8 */
void *__tfQ29gt2k_menut4Cell1ZQ29gt2k_menu18MemorycardMenuItem(void);   /* 0x00151A80 */
/* The type_info storage these fill that other translation units use (the
 * __tf functions of T074 / T075 / T076's subclasses; the plan put them in
 * T070): __si_type_info (3 words) / __class_type_info (4 words). */
extern u32 __tiQ29gt2k_menut4Cell1ZQ29gt2k_menu18MemorycardMenuItem[3];   /* 0x004B1980 */
extern u32 __tiQ29gt2k_menu22MemorycardMenuItemItem[3];                   /* 0x004B1A60 */
extern u32 __tiQ29gt2k_menu19MemorycardLayerMenu[4];                      /* 0x004B1A70 */
extern u32 __tiQ29gt2k_menu23MemorycardLayerMenuItem[3];                  /* 0x004B1A80 */
}
#endif

#endif /* GAME_T073_MEMORYCARD_MENU_H */
