/* menu_list.h - the typed list templates gt2k_menu::Cell<T> / gt2k_menu::List<T>
 * over the intrusive list of T495 (pCell / pList, T495_menu_list.h), and
 * their inline iterator gt2k_menu::ListIterator<T>.
 *
 * Not a translation unit of its own: the templates are inline in the
 * original (in a header every menu unit includes).  egcs emits the instances
 * it needs in each translation unit that uses them, as out-of-line copies
 * with their vtables and typeinfo functions; for example Cell<Button>::~Cell
 * 0x001B47C0 (T206), List<Button>::~List 0x001CA358 (T244),
 * List<OperatorHolder>::~List 0x001BBA08 and Cell<OperatorHolder>::~Cell
 * 0x001BBAE8 (T216), Cell<GarageCar>::~Cell 0x00125A10 (T036, more copies in
 * T080, T096, T427, T429).  Each destructor stores the instance's vtable in
 * the vptr (+0x08) and calls ~pCell / ~pList with the flags.
 *
 * A typed cell is a base of its element class (Button : Item, Cell<Button>
 * at +0x100), so a list link (pCell *) points at that base, not at the
 * element: convert with static_cast<T *>(static_cast<Cell<T> *>(cell)).
 *
 * Kept out of T027_level_button_list.h (where the first pass put them) so
 * that T244 (ButtonList : List<Button>) and T027 (LevelButtonList :
 * ButtonList) do not include each other. */
#ifndef GAME_MENU_LIST_H
#define GAME_MENU_LIST_H

#include "game/game.h"
#include "game/menu/framework/T495_menu_list.h"            /* bases gt2k_menu::pCell, gt2k_menu::pList */

namespace gt2k_menu {

/* vtable per instance (3 slots: [1] / [2] pCell's isBefore 0x0023C120 /
 * isNot 0x0023C128); typeinfo __tfQ29gt2k_menut4Cell1Z<T> (si, base
 * pCell).  egcs layout: next +0x0, prev +0x4, vptr +0x8; size 0xC (the data
 * of a class derived from Cell<T> alone starts at +0xC). */
template <class T>
class Cell : public pCell {
public:
    Cell() {}                               /* inline: next = prev = 0, the vptr */
    virtual ~Cell() {}                      /* [0] +0x0C inline: the vptr, then ~pCell */
};

/* vtable per instance (2 slots: [1] pList's sort 0x0023C580); typeinfo
 * __tfQ29gt2k_menut4List1Z<T> (si, base pList).  egcs layout: head +0x0,
 * tail +0x4, vptr +0x8; size 0xC. */
template <class T>
class List : public pList {
public:
    List() {}                               /* inline: head = tail = 0, the vptr */
    virtual ~List() {}                      /* [0] +0x0C inline: the vptr, then ~pList (which deletes
                                               the cells still linked) */
    /* (? name) The typed append: pList::append(item) (0x0023C188).  Inline,
     * so its `this' (the list part of the object, e.g. +0x0C in a
     * MemorycardMenu) is substituted where the body uses it: retail
     * evaluates the argument first (`subMenu->append(new Item(...))' runs
     * operator new and the constructor, then loads subMenu, adds 0xC and
     * calls append), where a direct call to pList::append converts `this'
     * before the argument (u030 matching: T074's makers; u029's, T075's and
     * T076's have the same shape). */
    void append(T *item) { pList::append(item); }
};

/* The walk over a List<T> every unit expands inline: {the next cell, the
 * list} on the stack (8 bytes; list is stored, never read); next() returns
 * the element of the current cell (NULL at the end) and steps on.  (The
 * plan's header for it is T042_event_hider.h, which includes this one.)
 *
 * next() returns the converted cell alone: egcs's null-checked static_cast
 * (cell - offset, then movz / movn on the cell) is retail's shape in every
 * user (ButtonList::totalWidth 0x001C9818, setCursorAt 0x001C9550,
 * EventHider::virtual_2 0x00119258, ...); an explicit `c != 0 ? ... : 0'
 * around it gave a second branch (u008 matching, ref/matching/u008/notes.md:
 * 25 functions of u034 / u040 / u063 / u065 / u066 / u070 closer, one worse).
 * The reference constructor takes a list whose conversion needs no null
 * check (EventHider walks *list: retail adds +0xB0 without movz). */
template <class T>
struct ListIterator {
    pCell *cell;                            /* 0x0 */
    pList *list;                            /* 0x4 */

    /* The list first, then its head (assignments, not member initialisers,
     * which egcs expands in declaration order): sched1 keeps the two stores
     * in that order where they tie, retail's `sw list, 4($sp)' before `sw
     * cell, 0($sp)' (u070 matching: ButtonList::setDecideFunc 0x001C9288,
     * widthBeforeFocus 0x001C9938, heightBeforeFocus 0x001C9A48 and
     * MapButtonList 0x001B4E10 exact; no function of the 14 files that walk
     * a list worse, ref/matching/u070/notes.md). */
    ListIterator(List<T> *l) { list = l; cell = l->head; }
    ListIterator(List<T> &l) { list = &l; cell = l.head; }
    /* (? name) next()'s step, an inline member of its own (defined first, so
     * that next() expands it): its `this' is a fresh pseudo, so the store of
     * the next cell goes through a register holding the iterator's address
     * (retail: `daddu sN, $sp, $zero', `sw ..., 0(sN)') while the read of
     * cell folds to $sp; written in next() itself both fold to $sp.  When
     * that register is not worth a callee-saved one, reload puts $sp back
     * (ButtonList::hideButtons 0x001C9DC0).  u024 matching: 23 functions of
     * u024 / u025 / u028 / u065 / u070 / u098 exact
     * (ref/matching/u024/notes.md). */
    void step() { cell = (cell != 0) ? cell->next : 0; }
    T *next()
    {
        pCell *c = cell;
        step();
        return static_cast<T *>(static_cast<Cell<T> *>(c));
    }
};

} /* namespace gt2k_menu */

#endif /* GAME_MENU_LIST_H */
