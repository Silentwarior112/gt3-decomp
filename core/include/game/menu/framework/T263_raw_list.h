/* T263_raw_list.h - the list windows of the menu system: gt2k_menu::RawListItem,
 * RawListItemFixed, RawList and its variants (translation unit T263, unit u074).
 *
 * A RawListItem is a RawWindow that can be linked into a list (prev / next,
 * the owning list, lock flags for the layout, a "hi-light" image shown while
 * it is selected but not focused).  A RawList is itself an item and holds a
 * doubly linked list of items with one focus item: the focus moves with the
 * cursor events (3 / 4 up / down for a vertical list, 5 / 6 left / right for
 * a horizontal one, with menu sound 3 "move" or 4 "cannot"), optionally
 * wrapping around (moveLoop) and keeping the index when it moves between two
 * sub-lists (keepSubListIndex).  layout() places the items one after the
 * other (margin, spacing; sub-lists first) and resizes the list to them
 * unless its size is locked; scrollToFocus() scrolls the list inside its
 * scroll window (setScroll) so that the focus item is visible, through the
 * RawAnimScroll of RawWindow::anim.
 *
 * The variants only change flags: H = horizontal (vertical 0); Pos / Size /
 * Fixed lock the position / the size / both when the list is laid out as an
 * item of a parent list (RawListFixed and RawListV change nothing).  Slot 8
 * (the class name) and the registry entries (factory, create, canCreate) of
 * every class here are defined in T272 (u073).  The accessors at
 * 0x001D5A18..0x001D5AE0 and 0x001D5B88..0x001D5BB8 are inline members of
 * the original, emitted out of line (no callers): they are defined inline
 * after their classes, in reverse address order, and the #pragma interface
 * below / #pragma implementation in T263's source make egcs emit the copies
 * there.  getRawList's dynamic_cast is expanded at its uses (layout() and
 * virtual_28).
 * index.txt's Lupo 2 names for them are one function off (see raw_list_internal.h); the
 * names here are the ones that fit the code. */
#ifndef GAME_T263_RAW_LIST_H
#define GAME_T263_RAW_LIST_H

#include "game/game.h"
#include "game/menu/framework/T264_raw_window.h"           /* base gt2k_menu::RawWindow */

namespace gt2k_menu {
struct RawWindowArgs;                       /* T264 (references / pointers only) */
struct MenuLayoutItem;                      /* T229 */
class PageLayout;                           /* T229 */
class MenuControl;                          /* T214 */
class RawAnimation;                         /* T270 */
class RawList;

#ifdef BOOT_OLD_GCC
/* From here on (egcs applies it to the classes defined after it), with
 * #pragma implementation in T263_raw_list.cpp: egcs emits the copies of
 * the inline accessors below and the vtable only there, before __tf. */
#pragma interface
#endif

/* vtable 0x00338D58 (24 slots: [0], [8], [21] its own, the rest RawWindow's).
 * typeinfo __tfQ29gt2k_menu11RawListItem 0x001D5BC0 (si, base RawWindow;
 * __si_type_info 0x004B2750).  Size 0x204 (operator new in T272's create). */
class RawListItem : public RawWindow {
public:
    RawListItem(const RawWindowArgs &args); /* 0x001D3F70 (moveDir left: RawList's constructor sets it) */
    virtual ~RawListItem();                 /* [0] +0x0C 0x001D3FC0 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x001D3610 (T272) "RawListItem" */
    virtual void virtual_21(int focus);     /* [21] +0xB4 0x001D4018 selected and not focused: the
                                               hi-light image; else RawWindow's (index.txt:
                                               RawListItemFixed::virtual_21) */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node, int p0, int p1, int p2, int p3); /* 0x001D3618 (T272) */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001D3658 (T272) */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001D36D0 (T272) 1 */

    void setSelected(int sel);              /* 0x001D4068 virtual_21(focus) if it changed */
    void setHiLightPropertyName(const char *name);  /* 0x001D5B88 */
    RawListItem *getNextItem();             /* 0x001D5B90 (index.txt: getPrevItem) */
    RawListItem *getPrevItem();             /* 0x001D5B98 */
    void lockSize(bool lock);               /* 0x001D5BA0 */
    void lockPosition(bool lock);           /* 0x001D5BA8 */
    bool isLockSize();                      /* 0x001D5BB0 (index.txt: isLockPosition) */
    bool isLockPosition();                  /* 0x001D5BB8 (index.txt: getNextItem) */

    RawList     *list;                      /* 0x1E4 the list holding the item */
    RawListItem *prevItem;                  /* 0x1E8 */
    RawListItem *nextItem;                  /* 0x1EC */
    /* both bool, 4 bytes under egcs (RawList::layout's !isLockSize() loads
     * straight into its xori's register); GCC 14 keeps them int (its bool is
     * 1 byte; the values are 0 / 1), so the layout stays the same there */
#ifdef BOOT_OLD_GCC
    bool         positionLocked;            /* 0x1F0 the list's layout leaves the position alone */
    bool         sizeLocked;                /* 0x1F4 a list does not resize to its items */
#else
    int          positionLocked;            /* 0x1F0 */
    int          sizeLocked;                /* 0x1F4 */
#endif
    int          moveDir;                   /* 0x1F8 (RawList) -1 / 0 / 1: the direction of the last focus move */
    const char  *hiLightPropertyName;       /* 0x1FC the image shown when selected but not focused */
    int          selected;                  /* 0x200 */
};
EGCS_SIZE(gt2k_menu::RawListItem, 0x204);

/* The inline accessors 0x001D5B88..0x001D5BB8 (see the top), in reverse
 * address order. */
inline bool RawListItem::isLockPosition() { return positionLocked; }  /* 0x001D5BB8 */
inline bool RawListItem::isLockSize() { return sizeLocked; }  /* 0x001D5BB0 */
inline void RawListItem::lockPosition(bool lock) { positionLocked = lock; }  /* 0x001D5BA8 */
inline void RawListItem::lockSize(bool lock) { sizeLocked = lock; }  /* 0x001D5BA0 */
inline RawListItem *RawListItem::getPrevItem() { return prevItem; }  /* 0x001D5B98 */
inline RawListItem *RawListItem::getNextItem() { return nextItem; }  /* 0x001D5B90 */
inline void RawListItem::setHiLightPropertyName(const char *name) { hiLightPropertyName = name; }  /* 0x001D5B88 */

/* vtable 0x00338C90 (24 slots).  typeinfo __tfQ29gt2k_menu16RawListItemFixed
 * 0x001D5B38 (si, base RawListItem; 0x004B27F0).  Position and size locked.
 * Size 0x204 (operator new in T272). */
class RawListItemFixed : public RawListItem {
public:
    RawListItemFixed(const RawWindowArgs &args);   /* 0x001D40A8 */
    virtual ~RawListItemFixed();            /* [0] +0x0C 0x001D40E8 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x001D36D8 (T272) "RawListItemFixed" */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node, int p0, int p1, int p2, int p3); /* 0x001D36E0 (T272) */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001D3720 (T272) */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001D3798 (T272) */
};
EGCS_SIZE(gt2k_menu::RawListItemFixed, 0x204);

/* vtable 0x00338B80 (33 slots: [0], [8], [9], [14], [17], [20], [22]
 * overridden, [24]..[32] new; index.txt names the implementations
 * RawListVSize::virtual_N, they are RawList's own).  typeinfo
 * __tfQ29gt2k_menu7RawList 0x001D5AE8 (si, base RawListItem; 0x004B27B0).
 * Size 0x248 (operator new in T272). */
class RawList : public RawListItem {
public:
    RawList(const RawWindowArgs &args);     /* 0x001D4140 vertical, keepSubListIndex, focusControl;
                                               childrenFocusChange 0 */
    virtual ~RawList();                     /* [0] +0x0C 0x001D41C8 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x001D3160 (T272) "RawList" */
    virtual void virtual_9();               /* [9] +0x54 0x001D4220 runs the focus animation */
    virtual void virtual_14(MenuControl *ctl);  /* [14] +0x7C 0x001D4408 RawWindow's, then virtual_24(ctl) */
    virtual int  virtual_17(RawWindow *sender, int event, int arg);   /* [17] +0x94 0x001D42B8 its own
                                               cursor events move the focus (handled: 0); the rest RawWindow's */
    virtual void virtual_20(int focus);     /* [20] +0xAC 0x001D4450 the focus to the children that are not
                                               items, then to the focus item; the focus animation follows it */
    virtual void virtual_22(RawWindow *child);  /* [22] +0xBC 0x001D4248 a RawListItem child is appended
                                               (no relayout) */
    virtual void virtual_24(MenuControl *ctl);  /* [24] +0xCC 0x001D45C8 the focus item gets ctl (NULL if
                                               focusControl is 0) */
    virtual void virtual_25(int on);        /* [25] +0xD4 0x001D4608 focusControl = on; virtual_24(menuControl) */
    virtual int  virtual_26();              /* [26] +0xDC 0x001D4638 the focus to the previous item (moveLoop:
                                               wraps); 0 if it did not move */
    virtual int  virtual_27();              /* [27] +0xE4 0x001D46A0 the same to the next item */
    virtual int  virtual_28(RawListItem *item); /* [28] +0xEC 0x001D4708 moves the focus (and the menu
                                               control) to item; sub-lists: virtual_30; 1 */
    virtual void virtual_29(RawListItem *item); /* [29] +0xF4 0x001D4840 focusItem = item */
    virtual void virtual_30(RawList *from, RawList *to);   /* [30] +0xFC 0x001D4B30 to->virtual_31(from's
                                               focus index) */
    virtual void virtual_31(int index);     /* [31] +0x104 0x001D4B88 the focus (and the menu control) to item
                                               index; no scroll */
    virtual void virtual_32(int index);     /* [32] +0x10C 0x001D4C30 the focus to item index (virtual_28,
                                               moveDir), then scrollToFocus */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node, int p0, int p1, int p2, int p3); /* 0x001D3168 (T272) */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001D31A8 (T272) */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001D3220 (T272) */

    void scrollToFocus();                   /* 0x001D4848 aims anim (a RawAnimScroll) so that the focus item
                                               is visible in the scroll window */
    int  getIndex(RawListItem *item);       /* 0x001D4CC0 the index of item (NULL: the focus item), -1 none */
    RawListItem *getItem(int index);        /* 0x001D4CF8 item index (< 0: the focus item), NULL past the end */
    void setVertical(int vertical, int relayout);  /* 0x001D4D40 */
    void addItem(RawListItem *item, int relayout); /* 0x001D4D68 insertItem at count */
    void insertItem(RawListItem *item, int index, int relayout);   /* 0x001D4D88 links item in before item
                                               index (interrupts off), its position relative to the list */
    void removeItem(RawListItem *item, int relayout);  /* 0x001D4EB0 unlinks item; the focus goes to its next
                                               (else previous) item if it had it */
    void layout();                          /* 0x001D4F80 lays the items out, then resizes the list to them
                                               unless sizeLocked */

    static RawList *getRawList(RawWindow *w);   /* 0x001D5A18 NULL for NULL */
    void setScrollNextOffset(f32 v);        /* 0x001D5A70 */
    void setScrollNextMargin(f32 v);        /* 0x001D5A78 */
    void setScrollPrevOffset(f32 v);        /* 0x001D5A80 */
    void setScrollPrevMargin(f32 v);        /* 0x001D5A88 */
    void setScrollCenter(bool on);          /* 0x001D5A90 */
    void setScroll(RawWindow *w);           /* 0x001D5A98 */
    void setFocusAnimation(RawAnimation *anim);    /* 0x001D5AA0 */
    int  getItemCount();                    /* 0x001D5AA8 (index.txt: getFocusItem) */
    int  getIndex(RawListItem &item);       /* 0x001D5AB0 getIndex(&item) */
    void setKeepSubListIndex(bool on);      /* 0x001D5AD0 (index.txt: setMoveLoop) */
    void setMoveLoop(bool on);              /* 0x001D5AD8 */
    RawListItem *getFocusItem();            /* 0x001D5AE0 */

    RawAnimation *focusAnimation;           /* 0x204 follows the focus item (its slot 5: the target window) */
    RawListItem  *firstItem;                /* 0x208 */
    RawListItem  *lastItem;                 /* 0x20C */
    RawListItem  *focusItem;                /* 0x210 */
    int           count;                    /* 0x214 */
    f32           margin;                   /* 0x218 the position of the first item along the axis */
    f32           spacing;                  /* 0x21C the gap between items */
    RawWindow    *scroll;                   /* 0x220 the visible part (the list moves inside it) */
    int           scrollCenter;             /* 0x224 the focus item is centred in it */
    f32           scrollPrevMargin;         /* 0x228 */
    f32           scrollPrevOffset;         /* 0x22C */
    f32           scrollNextMargin;         /* 0x230 */
    f32           scrollNextOffset;         /* 0x234 */
    int           vertical;                 /* 0x238 1 (default), 0 RawListH* */
    int           moveLoop;                 /* 0x23C the focus wraps around (virtual_26 / virtual_27) */
    int           keepSubListIndex;         /* 0x240 1: moving between two sub-lists keeps the index (virtual_28) */
    int           focusControl;             /* 0x244 1: the focus item gets the menu control (virtual_24) */
};
EGCS_SIZE(gt2k_menu::RawList, 0x248);

/* The inline accessors 0x001D5A70..0x001D5AE0: no callers, the users expand them; defined
 * in reverse address order (egcs emits the #pragma implementation copies
 * in reverse definition order). */
inline RawListItem *RawList::getFocusItem() { return focusItem; }  /* 0x001D5AE0 */
inline void RawList::setMoveLoop(bool on) { moveLoop = on; }  /* 0x001D5AD8 */
inline void RawList::setKeepSubListIndex(bool on) { keepSubListIndex = on; }  /* 0x001D5AD0 */
inline int RawList::getIndex(RawListItem &item) { return getIndex(&item); }  /* 0x001D5AB0 */
inline int RawList::getItemCount() { return count; }  /* 0x001D5AA8 */
inline void RawList::setFocusAnimation(RawAnimation *anim) { focusAnimation = anim; }  /* 0x001D5AA0 */
inline void RawList::setScroll(RawWindow *w) { scroll = w; }  /* 0x001D5A98 */
inline void RawList::setScrollCenter(bool on) { scrollCenter = on; }  /* 0x001D5A90 */
inline void RawList::setScrollPrevMargin(f32 v) { scrollPrevMargin = v; }  /* 0x001D5A88 */
inline void RawList::setScrollPrevOffset(f32 v) { scrollPrevOffset = v; }  /* 0x001D5A80 */
inline void RawList::setScrollNextMargin(f32 v) { scrollNextMargin = v; }  /* 0x001D5A78 */
inline void RawList::setScrollNextOffset(f32 v) { scrollNextOffset = v; }  /* 0x001D5A70 */
inline RawList *RawList::getRawList(RawWindow *w) { return dynamic_cast<RawList *>(w); }  /* 0x001D5A18 */

/* The variants: vtables of 33 slots ([0] and [8] their own, the rest
 * RawList's), size 0x248 (operator new in T272), si typeinfo with the base
 * shown. */

/* vtable 0x00338A70.  __tfQ29gt2k_menu12RawListFixed 0x001D59C8 (base
 * RawList; 0x004B27D0).  The constructor sets nothing. */
class RawListFixed : public RawList {
public:
    RawListFixed(const RawWindowArgs &args);   /* 0x001D5190 */
    virtual ~RawListFixed();                /* [0] +0x0C 0x001D51C8 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x001D3228 (T272) "RawListFixed" */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node, int p0, int p1, int p2, int p3); /* 0x001D3230 (T272) */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001D3270 (T272) */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001D32E8 (T272) */
};
EGCS_SIZE(gt2k_menu::RawListFixed, 0x248);

/* vtable 0x00338960.  __tfQ29gt2k_menu8RawListH 0x001D5978 (base RawList;
 * 0x004B2760).  Horizontal. */
class RawListH : public RawList {
public:
    RawListH(const RawWindowArgs &args);    /* 0x001D5220 vertical = 0 */
    virtual ~RawListH();                    /* [0] +0x0C 0x001D5258 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x001D32F0 (T272) "RawListH" */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node, int p0, int p1, int p2, int p3); /* 0x001D32F8 (T272) */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001D3338 (T272) */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001D33B0 (T272) */
};
EGCS_SIZE(gt2k_menu::RawListH, 0x248);

/* vtable 0x00338850.  __tfQ29gt2k_menu13RawListHFixed 0x001D5928 (base
 * RawListH; 0x004B27C0).  Size and position locked. */
class RawListHFixed : public RawListH {
public:
    RawListHFixed(const RawWindowArgs &args);  /* 0x001D52B0 */
    virtual ~RawListHFixed();               /* [0] +0x0C 0x001D52F0 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x001D33B8 (T272) "RawListHFixed" */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node, int p0, int p1, int p2, int p3); /* 0x001D33C0 (T272) */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001D3400 (T272) */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001D3478 (T272) */
};
EGCS_SIZE(gt2k_menu::RawListHFixed, 0x248);

/* vtable 0x00338740.  __tfQ29gt2k_menu11RawListHPos 0x001D58D8 (base
 * RawListH; 0x004B2770).  Position locked. */
class RawListHPos : public RawListH {
public:
    RawListHPos(const RawWindowArgs &args); /* 0x001D5348 */
    virtual ~RawListHPos();                 /* [0] +0x0C 0x001D5388 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x001D3480 (T272) "RawListHPos" */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node, int p0, int p1, int p2, int p3); /* 0x001D3488 (T272) */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001D34C8 (T272) */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001D3540 (T272) */
};
EGCS_SIZE(gt2k_menu::RawListHPos, 0x248);

/* vtable 0x00338630.  __tfQ29gt2k_menu12RawListHSize 0x001D5888 (base
 * RawListH; 0x004B2790).  Size locked. */
class RawListHSize : public RawListH {
public:
    RawListHSize(const RawWindowArgs &args);   /* 0x001D53E0 */
    virtual ~RawListHSize();                /* [0] +0x0C 0x001D5420 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x001D3548 (T272) "RawListHSize" */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node, int p0, int p1, int p2, int p3); /* 0x001D3550 (T272) */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001D3590 (T272) */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001D3608 (T272) */
};
EGCS_SIZE(gt2k_menu::RawListHSize, 0x248);

/* vtable 0x00338520.  __tfQ29gt2k_menu8RawListV 0x001D5838 (base RawList;
 * 0x004B2780).  Vertical (RawList's default; the constructor sets nothing). */
class RawListV : public RawList {
public:
    RawListV(const RawWindowArgs &args);    /* 0x001D5478 */
    virtual ~RawListV();                    /* [0] +0x0C 0x001D54B0 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x001D37A0 (T272) "RawListV" */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node, int p0, int p1, int p2, int p3); /* 0x001D37A8 (T272) */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001D37E8 (T272) */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001D3860 (T272) */
};
EGCS_SIZE(gt2k_menu::RawListV, 0x248);

/* vtable 0x00338410.  __tfQ29gt2k_menu13RawListVFixed 0x001D57E8 (base
 * RawListV; 0x004B27A0).  Size and position locked. */
class RawListVFixed : public RawListV {
public:
    RawListVFixed(const RawWindowArgs &args);  /* 0x001D5508 */
    virtual ~RawListVFixed();               /* [0] +0x0C 0x001D5548 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x001D3868 (T272) "RawListVFixed" */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node, int p0, int p1, int p2, int p3); /* 0x001D3870 (T272) */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001D38B0 (T272) */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001D3928 (T272) */
};
EGCS_SIZE(gt2k_menu::RawListVFixed, 0x248);

/* vtable 0x00338300.  __tfQ29gt2k_menu11RawListVPos 0x001D5798 (base
 * RawListV; 0x004B2740).  Position locked. */
class RawListVPos : public RawListV {
public:
    RawListVPos(const RawWindowArgs &args); /* 0x001D55A0 */
    virtual ~RawListVPos();                 /* [0] +0x0C 0x001D55E0 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x001D3930 (T272) "RawListVPos" */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node, int p0, int p1, int p2, int p3); /* 0x001D3938 (T272) */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001D3978 (T272) */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001D39F0 (T272) */
};
EGCS_SIZE(gt2k_menu::RawListVPos, 0x248);

/* vtable 0x003381F0.  __tfQ29gt2k_menu12RawListVSize 0x001D5748 (base
 * RawListV; 0x004B27E0).  Size locked. */
class RawListVSize : public RawListV {
public:
    RawListVSize(const RawWindowArgs &args);   /* 0x001D5638 */
    virtual ~RawListVSize();                /* [0] +0x0C 0x001D5678 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x001D39F8 (T272) "RawListVSize" */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node, int p0, int p1, int p2, int p3); /* 0x001D3A00 (T272) */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001D3A40 (T272) */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001D3AB8 (T272) */
};
EGCS_SIZE(gt2k_menu::RawListVSize, 0x248);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself (and rejects hand declarations). */
extern "C" {
void *__tfQ29gt2k_menu12RawListVSize(void);     /* 0x001D5748 */
void *__tfQ29gt2k_menu11RawListVPos(void);      /* 0x001D5798 */
void *__tfQ29gt2k_menu13RawListVFixed(void);    /* 0x001D57E8 */
void *__tfQ29gt2k_menu8RawListV(void);          /* 0x001D5838 */
void *__tfQ29gt2k_menu12RawListHSize(void);     /* 0x001D5888 */
void *__tfQ29gt2k_menu11RawListHPos(void);      /* 0x001D58D8 */
void *__tfQ29gt2k_menu13RawListHFixed(void);    /* 0x001D5928 */
void *__tfQ29gt2k_menu8RawListH(void);          /* 0x001D5978 */
void *__tfQ29gt2k_menu12RawListFixed(void);     /* 0x001D59C8 */
void *__tfQ29gt2k_menu7RawList(void);           /* 0x001D5AE8 */
void *__tfQ29gt2k_menu16RawListItemFixed(void); /* 0x001D5B38 */
void *__tfQ29gt2k_menu11RawListItem(void);      /* 0x001D5BC0 */
/* Their __si_type_info nodes (3 words, .bss) that other units use. */
extern u32 __tiQ29gt2k_menu7RawList[3];         /* 0x004B27B0 */
extern u32 __tiQ29gt2k_menu16RawListItemFixed[3];   /* 0x004B27F0 */
extern u32 __tiQ29gt2k_menu11RawListItem[3];    /* 0x004B2750 */
}
#endif

#endif /* GAME_T263_RAW_LIST_H */
