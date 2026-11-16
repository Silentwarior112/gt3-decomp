/* T495_menu_list.h - gt2k_menu::pCell / gt2k_menu::pList (translation unit
 * T495, unit u109), 0x0023C0C0..0x0023C760: the intrusive doubly linked list
 * under every menu list.  The typed templates Cell<T> / List<T> over them are
 * in menu_list.h.
 *
 * The constructors and the small accessors are inline in the original: most
 * units expand pCell() / pList() (two zero stores, then the vptr).  T495 has
 * out-of-line copies of them with no callers, at the end of the unit
 * (0x0023C678..0x0023C720, with the typeinfo functions): getTail 0x0023C678,
 * getHead 0x0023C680, pList() 0x0023C688, getPrev 0x0023C6E8, getNext
 * 0x0023C6F0, clearLinks 0x0023C6F8, pCell() 0x0023C708. */
#ifndef GAME_T495_MENU_LIST_H
#define GAME_T495_MENU_LIST_H

#include "game/game.h"

namespace gt2k_menu {

/* A node of a list.  vtable 0x00343AC8 (3 slots).  typeinfo
 * __tfQ29gt2k_menu5pCell 0x0023C728 (__user_type_info 0x003534B8, gp-0x4538).
 * egcs layout: next +0x0, prev +0x4, vptr +0x8.  Size 0xC (the Cell<T> data
 * starts at +0xC: GarageWheel::valid, ...). */
class pCell {
public:
    /* next = prev = 0: prev is stored first, as in the out-of-line copy
     * 0x0023C708 and the expansions (MemorycardFile's constructors, u023) */
    pCell() { next = prev = 0; }
    virtual ~pCell();                           /* [0] +0x0C 0x0023C0C0 unlinks the cell from its neighbours */
    virtual int isBefore(pCell *other);         /* [1] +0x14 0x0023C120 this < other (unsigned addresses) */
    virtual int isNot(pCell *other);            /* [2] +0x1C 0x0023C128 this != other */
    /* The accessors in this order: egcs writes a class's in-class inline
     * members in the reverse of their declaration order, and T495's copies
     * are getPrev 0x0023C6E8, getNext 0x0023C6F0, clearLinks 0x0023C6F8,
     * pCell() 0x0023C708 (its frame info; u109's data pass). */
    void   clearLinks() { prev = 0; next = 0; }   /* this order: T495's copy 0x0023C6F8 stores next first (u109) */
    pCell *getNext() { return next; }
    pCell *getPrev() { return prev; }

    pCell *next;                                /* 0x00 */
    pCell *prev;                                /* 0x04 */
    /*     vptr                                    0x08 */
};

/* A list of cells.  vtable 0x00343AB0 (2 slots).  typeinfo
 * __tfQ29gt2k_menu5pList 0x0023C6A8 (__user_type_info 0x003534C0, gp-0x4530).
 * egcs layout: head +0x0, tail +0x4, vptr +0x8.  Size 0xC. */
class pList {
public:
    pList() : head(0), tail(0) {}
    virtual ~pList();                           /* [0] +0x0C 0x0023C138 removeAll(): the cells are deleted */
    virtual void sort(int (*cmp)(const void *, const void *)); /* [1] +0x14 0x0023C580 qsort of the cell
                                                   pointers with cmp, then appended back */
    pList *append(pCell *cell);                 /* 0x0023C188 at the tail (nothing for NULL) */
    pList *prepend(pCell *cell);                /* 0x0023C1B8 at the head (nothing for NULL) */
    pList *insert(pCell *cell, int pos);        /* 0x0023C1E8 before position pos (0 = prepend) */
    pList *unlink(pCell *cell);                 /* 0x0023C288 head / tail updated, links cleared */
    pList *unlinkAll();                         /* 0x0023C2F8 every cell (they survive) */
    void   unlinkFrom(pCell *cell);             /* 0x0023C328 cell and the ones after it, the last first */
    pList *remove(pCell *cell);                 /* 0x0023C370 unlink + delete */
    pList *removeAll();                         /* 0x0023C3C0 deletes every cell */
    void   removeFrom(pCell *cell);             /* 0x0023C3F0 cell and the ones after it, the last first */
    int    count();                             /* 0x0023C438 */
    int    countOf(pCell *cell);                /* 0x0023C470 times cell is in the list (no callers) */
    int    indexOf(pCell *cell);                /* 0x0023C4A8 -1 if absent */
    pCell *at(int n);                           /* 0x0023C4D8 cell n (0 = head), NULL past the end */
    int    move(pCell *cell, int pos);          /* 0x0023C508 1 if done (no callers) */
    /* In this order: T495's copies are getTail 0x0023C678, getHead
     * 0x0023C680, pList() 0x0023C688 (reverse declaration order). */
    pCell *getHead() { return head; }
    pCell *getTail() { return tail; }

    pCell *head;                                /* 0x00 */
    pCell *tail;                                /* 0x04 */
    /*     vptr                                    0x08 */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::pCell, 0xC);
EGCS_SIZE(gt2k_menu::pList, 0xC);

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu5pList(void);             /* 0x0023C6A8 */
void *__tfQ29gt2k_menu5pCell(void);             /* 0x0023C728 */
extern u32 __tiQ29gt2k_menu5pList[2];           /* 0x003534C0 gp-0x4530 __user_type_info */
extern u32 __tiQ29gt2k_menu5pCell[2];           /* 0x003534B8 gp-0x4538 __user_type_info */
}
#endif

#endif /* GAME_T495_MENU_LIST_H */
