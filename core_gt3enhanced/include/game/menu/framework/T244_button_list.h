/* T244_button_list.h - gt2k_menu::ButtonList, its List<Button> base and the
 * button record gt2k_menu::ButtonData (translation unit T244, unit u070),
 * 0x001C7FE0..0x001CA550.
 *
 * A ButtonList is a Drawable holding a List<Button>: one Button per record
 * (ButtonData) of a page.  The buttons are laid along one axis (controlType
 * != 0: x and the widths, 0: y and the heights), each centred at its
 * offPoint, `spacing' apart.  The elements named "slider*" make a Slider
 * (T238); "multi*" / "free*" / "concurrent*" make the sub-list buttons
 * (T269 / T268 / T267) when the record is a sub list with entries; the rest
 * go through the virtual createButton.  The cursor (a SmartIndexT<int> over
 * the buttons) skips the buttons whose focus is disabled; the display value
 * (displayX / displayY) eases every frame towards the scroll target that
 * keeps the focused button inside the list's own w / h.  The layout's
 * "focus" / "disable" pictures and the focused button's help layers are
 * drawn with it; decide() calls the decide callback.
 *
 * T244 also has List<Button>'s destructor (0x001CA358, vtable 0x00336C48:
 * 2 slots, [1] pList's sort 0x0023C580) and typeinfo function, and a copy of
 * SmartIndexT<int>'s typeinfo function (0x001CA510, no callers; declared in
 * T017_smart_index.h). */
#ifndef GAME_T244_BUTTON_LIST_H
#define GAME_T244_BUTTON_LIST_H

#include "game/game.h"
#include "game/menu/framework/T259_menu_drawable.h"        /* base gt2k_menu::Drawable */
#include "game/menu/framework/menu_list.h"                  /* base gt2k_menu::List<Button> (the List / Cell templates) */
/* The template only (u071's data pass): the index member instantiates
 * SmartIndexT<int> after the List<Button> base, retail's order in the files
 * that saw it only through ButtonList (T251_smart_index_template.h). */
#include "game/common/T251_smart_index_template.h" /* SmartIndexT<int> (index) */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue (displayX, displayY) */
#include "game/menu/framework/T244_button_data.h"          /* gt2k_menu::ButtonData, BUTTON_* (split from here: a
                                               plain record, so its place does not matter) */

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class Transitive;                           /* T216 */
class Item;                                 /* T213 */
class Button;                               /* T206 */
class PageLayout;                           /* T229 */
class MenuControl;                          /* T214 */
struct MenuLayoutItem;                      /* T229 */
struct SelectableFuncData;                  /* T211: the per-page availability checks */
class ButtonList;

/* T235's name of the layout item (the same typedef as there; the first-pass
 * users of ButtonList, T224 among them, call it so). */
typedef MenuLayoutItem MenuResource;

/* The button record ButtonData and its flags BUTTON_*: T244_button_data.h. */

/* The decide callback of a list: ctx, the decided record, the cursor, the
 * list, decideButton's a2 (PageManager::buttonCallback's `effect': 1 from
 * decide(), 0 from the lists that play their own sound, T092 / T219). */
typedef void (*ButtonDecideFunc)(void *ctx, ButtonData *data, int index,
                                 ButtonList *list, int a4);

/* Bases: Drawable at +0x00, List<Button> at +0xB0 (head, tail, vptr +0xB8).
 * vtables 0x00336BD8 (13 slots: [2] Drawable's 0x001D0478, [3] / [4]
 * 0x001BFF00 / 0x001BFF08) and 0x00336BC0 (the List<Button> part, 2 slots,
 * this-delta -0xB0).  typeinfo __tfQ29gt2k_menu10ButtonList 0x001CA460
 * (class, base list 0x00336CA8: Drawable public +0, List<Button> public
 * +0xB0; __class_type_info 0x004B2650).  Size 0x128. */
class ButtonList : public Drawable, public List<Button> {
public:
    ButtonList(int controlType, int loop, PageLayout *layout); /* 0x001C8B90 no buttons yet (the
                                               derived lists build them); spacing 4, relative 1 */
    ButtonList(int controlType, int loop, PageLayout *layout, f32 x, f32 y, f32 w, f32 h,
               f32 spacing, ButtonData **items, int count, int relative, int index,
               MenuLayoutItem *rects);      /* 0x001C88D0 builds the buttons (rects: one element per
                                               record, else the layout's), the cursor at index.
                                               The floats right after layout (as Button's
                                               offPoint): the callers move them first (u009's
                                               ride_sure, u063's T209 and u064's createButtonList
                                               exact only so; u070 matching) */
    virtual ~ButtonList();                  /* [0] +0x0C 0x001C8DA8 unlinks the buttons, deletes the
                                               focus / disable pictures */
    virtual void virtual_1();               /* [1] +0x14 0x001C9CA8 update: the pictures, the display
                                               eases towards scrollTarget() */
    virtual Button *createButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                                 int index); /* [5] +0x34 0x001C84A8 a plain Button, attached.
                                               offPoint before index: the calls set $f12 before
                                               $a3 (u070 / u009 matching) */
    virtual void createButtons(ButtonData **items, int count,
                               MenuLayoutItem *rects); /* [6] +0x3C 0x001C8570 */
    virtual f32  scrollTarget();            /* [7] +0x44 0x001C9B58 */
    virtual Button *focusButton();          /* [8] +0x4C 0x001C9760 the button at the cursor; NULL
                                               while the focus is disabled */
    virtual void refresh();                 /* [9] +0x54 0x001C93D0 refreshFocus, the display on the
                                               target at once */
    virtual int  getCursor();               /* [10] +0x5C 0x001C9810 index.value */
    virtual void decide();                  /* [11] +0x64 0x001CA0F8 decideButton(focused record, 1) */
    virtual void doControl(const MenuControl &ctl); /* [12] +0x6C 0x001C9E38 the pad input */

    void    resetIndex();                   /* 0x001C7FE0 the cursor over the buttons (clamped unless
                                               loop), at 0 */
    Button *createSlider(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                         int index);     /* 0x001C8040 a Slider (T238) with the layout's slider
                                               function of the element */
    Button *createMultiButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                              int index); /* 0x001C8148 a MultiButton (T269) */
    Button *createFreeButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                             int index); /* 0x001C8290 a FreeButton (T268) */
    Button *createConcurrentButton(MenuLayoutItem *elem, ButtonData *data, f32 offPoint,
                                   int index); /* 0x001C8398 a ConcurrentButton (T267), also
                                               `concurrent' */
    void    createFocusItems();             /* 0x001C87E0 the layout's "focus" / "disable" pictures */
    void    showHelpLayer(Object *parent, const char *layer);  /* 0x001C8EB8 the focused button's help
                                               layer in a new Transitive under parent */
    void    showHelpLayer2(Object *parent, const char *layer); /* 0x001C9030 the same, helpLayer2 */
    void    setButtonChecks(const SelectableFuncData *e, void *ctx); /* 0x001C91B0 the buttons' checks
                                               (the last one repeats) */
    void    setDecideFunc(void *ctx, ButtonDecideFunc func);  /* 0x001C9288 also to every button
                                               (Button::virtual_10) */
    void    refreshFocus();                 /* 0x001C9320 */
    void    enableFocus();                  /* 0x001C9430 */
    void    disableFocus();                 /* 0x001C9450 */
    void    focusLink(const char *link);    /* 0x001C9470 the cursor on the first button leading to
                                               link */
    void    setCursor(int index);           /* 0x001C9520 */
    void    setCursorAt(f32 pos);           /* 0x001C9550 the button under pos along the list */
    f32     focusPosition();                /* 0x001C9640 the focused button's centre along the list */
    f32     scrollOffset();                 /* 0x001C9700 the display value from the leading edge */
    ButtonData *focusData();                /* 0x001C97D8 the focused button's record, NULL if none */
    f32     totalWidth();                   /* 0x001C9818 the widths plus the spacing between */
    f32     totalHeight();                  /* 0x001C98A8 */
    f32     widthBeforeFocus();             /* 0x001C9938 (0 while the focus is disabled) */
    f32     widthAfterFocus();              /* 0x001C99B8 */
    f32     heightBeforeFocus();            /* 0x001C9A48 */
    f32     heightAfterFocus();             /* 0x001C9AC8 */
    void    hideButtons(int hide);          /* 0x001C9DC0 */
    int     movePrev();                     /* 0x001C9F48 to the previous focusable button */
    int     moveNext();                     /* 0x001CA020 */
    void    decideButton(ButtonData *data, int a2); /* 0x001CA150 the focused button decided; unless
                                               data is a sub list: decideFunc (a2 is its `effect')
                                               if available, else the buzzer */
    void    sortButtons(int (*cmp)(const void *, const void *)); /* 0x001CA1F8 sorts and lays them out
                                               again (lists that scroll only) */
    /* The accessors are inline in the original: no callers, and
     * 0x001CA380..0x001CA458 are egcs's out-of-line copies before the
     * typeinfo function (FreeButton / MultiButton expand isEnabledFocus,
     * u068; PageManager setActive, u064).  Declared in the reverse of their
     * address order: egcs writes a class's inline members in reverse
     * declaration order (T244's frame info, 0x002CC488, has them
     * getDisableItem .. getControlType; u070 data pass). */
    int getControlType() const { return controlType; } /* 0x001CA458 */
    f32 getTotalSize()                              /* 0x001CA420 totalHeight() or totalWidth() */
    {
        if (controlType == 0)
            return totalHeight();
        return totalWidth();
    }
    f32 getSize()                                   /* 0x001CA400 h (controlType 0) or w */
    {
        if (controlType == 0)
            return h;
        return w;
    }
    void setActive(bool on) { active = on; }        /* 0x001CA3F8 */
    int getActive() const { return active; }        /* 0x001CA3F0 */
    int getValue() const { return index.value; }    /* 0x001CA3E8 index.value */
    SmartIndexT<int> *getIndex() { return &index; } /* 0x001CA3E0 (not const: movePrev / moveNext
                                               move the cursor through it, u070 matching) */
    f32 getDisplayX() const { return displayX.value; } /* 0x001CA3D8 */
    f32 getDisplayY() const { return displayY.value; } /* 0x001CA3D0 */
    f32 getInterpolateDiff() const { return interpolateDiff; } /* 0x001CA3C8 */
    int isEnabledFocus() const { return focusDisabled ^ 1; } /* 0x001CA3B8 */
    Button *getFocusButton() { return focusButton(); } /* 0x001CA390 focusButton() */
    Item *getFocusItem() { return focusItem; }      /* 0x001CA388 */
    Item *getDisableItem() { return disableItem; }  /* 0x001CA380 */

    /*               List<Button>              0xB0 (head, tail, vptr) */
    int              controlType;           /* 0xBC 0: vertical (heights), else horizontal (widths) */
    int              loop;                  /* 0xC0 the cursor wraps around (index.clamp = !loop) */
    PageLayout      *layout;                /* 0xC4 given to every new button */
    Transitive      *helpLayer;             /* 0xC8 showHelpLayer */
    Transitive      *helpLayer2;            /* 0xCC showHelpLayer2 */
    SmartIndexT<int> index;                 /* 0xD0 the cursor */
    int              focusDisabled;         /* 0xE8 */
    int              active;                /* 0xEC 1 */
    Button          *concurrent;            /* 0xF0 the ConcurrentButton: gets the input before the
                                               focused one */
    f32              spacing;               /* 0xF4 4.0 */
    int              relative;              /* 0xF8 given to every new button; 0: no scrolling */
    MotionValue      displayX;              /* 0xFC the scroll display (horizontal lists) */
    MotionValue      displayY;              /* 0x108 (vertical lists) */
    f32              interpolateDiff;       /* 0x114 |target - display| after the last update */
    ButtonDecideFunc decideFunc;            /* 0x118 */
    void            *decideCtx;             /* 0x11C */
    Item            *focusItem;             /* 0x120 the layout's "focus" picture */
    Item            *disableItem;           /* 0x124 the layout's "disable" picture */
};
EGCS_SIZE(gt2k_menu::ButtonList, 0x128);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu10ButtonList(void);                /* 0x001CA460 */
void *__tfQ29gt2k_menut4List1ZQ29gt2k_menu6Button(void); /* 0x001CA4C0 (si, base pList; __si_type_info
                                                            0x004B14E0) */
/* Their type_info storage (.bss), filled by them: __class_type_info = 4
 * words, __si_type_info = 3. */
extern u32 __tiQ29gt2k_menu10ButtonList[4];              /* 0x004B2650 */
extern u32 __tiQ29gt2k_menut4List1ZQ29gt2k_menu6Button[3]; /* 0x004B14E0 */
}
#endif

#endif /* GAME_T244_BUTTON_LIST_H */
