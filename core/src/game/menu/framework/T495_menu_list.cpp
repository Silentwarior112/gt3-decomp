/* T495 - gt2k_menu::pCell / pList: the intrusive doubly linked list under
 * every menu list (Cell<T>, List<T>: game/menu/framework/menu_list.h).
 * 0x0023C0C0..0x0023C760.
 *
 * Data: .rodata 0x00343AB0..0x00343B12: the vtables of pList and pCell (the
 * key functions ~pList / ~pCell are here), then their type names.  The
 * inline accessors' copies come out in the original's order with the
 * declaration order of game/menu/framework/T495_menu_list.h (its comments). */
#include "runtime/newlib/G007_stdio.h"                    /* qsort */
#include "game/data/car_database_internal.h"
#include "game/menu/framework/T495_menu_list.h"

namespace gt2k_menu {

/* Unlinks the cell from its neighbours (not from a list's head / tail),
 * clears next and prev. */
// FUNC 0x0023C0C0 gt2k_menu::pCell::~pCell
pCell::~pCell()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* the vptr (+0x8) = 0x00343AC8 first; operator delete if flags & 1 */
    if (next != 0)
        next->prev = prev;
    if (prev != 0)
        prev->next = next;
    next = 0;
    prev = 0;
}

/* Slot 1: this < other (unsigned addresses). */
// FUNC 0x0023C120 gt2k_menu::pCell::isBefore
int pCell::isBefore(pCell *other)
{
    return (u32)this < (u32)other;
}

/* Slot 2: this != other. */
// FUNC 0x0023C128 gt2k_menu::pCell::isNot
int pCell::isNot(pCell *other)
{
    return this != other;
}

/* removeAll(): the cells still linked are deleted. */
// FUNC 0x0023C138 gt2k_menu::pList::~pList
pList::~pList()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* the vptr (+0x8) = 0x00343AB0 first; operator delete if flags & 1 */
    removeAll();
}

/* Appends cell at the tail (nothing for NULL). */
// FUNC 0x0023C188 gt2k_menu::pList::append
pList *pList::append(pCell *cell)
{
    if (cell == 0)
        return this;
    if (tail != 0) {
        tail->next = cell;
        cell->prev = tail;
    } else {
        head = cell;
    }
    tail = cell;
    return this;
}

/* Inserts cell at the head (nothing for NULL). */
// FUNC 0x0023C1B8 gt2k_menu::pList::prepend
pList *pList::prepend(pCell *cell)
{
    if (cell == 0)
        return this;
    if (head != 0) {
        head->prev = cell;
        cell->next = head;
    } else {
        tail = cell;
    }
    head = cell;
    return this;
}

/* Inserts cell before position pos (0 = prepend); past the end it goes
 * after the last cell.  Quirks: no check of an empty list, and the tail is
 * not updated when the cell becomes the last.  Caller: move(). */
// FUNC 0x0023C1E8 gt2k_menu::pList::insert
pList *pList::insert(pCell *cell, int pos)
{
    pCell *before;
    int i;

    if (cell == 0)
        return this;
    if (pos == 0) {
        prepend(cell);
    } else {
        before = head;
        for (i = 1; i < pos && before->next != 0; i++)
            before = before->next;
        if (before->next != 0)
            before->next->prev = cell;
        cell->next = before->next;
        cell->prev = before;
        before->next = cell;
    }
    return this;
}

/* Unlinks cell (head / tail updated), clears its links. */
// FUNC 0x0023C288 gt2k_menu::pList::unlink
pList *pList::unlink(pCell *cell)
{
    if (cell == 0)
        return this;
    if (cell == head)
        head = cell->next;
    if (cell == tail)
        tail = cell->prev;
    if (cell->next != 0)
        cell->next->prev = cell->prev;
    if (cell->prev != 0)
        cell->prev->next = cell->next;
    cell->next = 0;
    cell->prev = 0;
    return this;
}

/* Unlinks every cell (they survive). */
// FUNC 0x0023C2F8 gt2k_menu::pList::unlinkAll
pList *pList::unlinkAll()
{
    unlinkFrom(head);
    return this;
}

/* Unlinks cell and the cells after it, the last first (recursive). */
// FUNC 0x0023C328 gt2k_menu::pList::unlinkFrom
void pList::unlinkFrom(pCell *cell)
{
    if (cell != 0) {
        unlinkFrom(cell->next);
        unlink(cell);
    }
}

/* Unlinks cell and deletes it (its virtual destructor, slot 0, flags 3). */
// FUNC 0x0023C370 gt2k_menu::pList::remove
pList *pList::remove(pCell *cell)
{
    unlink(cell);
    delete cell;
    return this;
}

/* Removes (deletes) every cell. */
// FUNC 0x0023C3C0 gt2k_menu::pList::removeAll
pList *pList::removeAll()
{
    removeFrom(head);
    return this;
}

/* Removes cell and the cells after it, the last first (recursive). */
// FUNC 0x0023C3F0 gt2k_menu::pList::removeFrom
void pList::removeFrom(pCell *cell)
{
    if (cell != 0) {
        removeFrom(cell->next);
        remove(cell);
    }
}

/* Number of cells. */
// FUNC 0x0023C438 gt2k_menu::pList::count
int pList::count()
{
    int n = 0;
    pCell *cell;

    for (cell = head; cell != 0; cell = cell->next)
        n++;
    return n;
}

/* Number of times cell is in the list.  No callers. */
// FUNC 0x0023C470 gt2k_menu::pList::countOf
int pList::countOf(pCell *cell)
{
    int n = 0;
    pCell *c;

    for (c = head; c != 0; c = c->next) {
        if (c == cell)
            n++;
    }
    return n;
}

/* Index of cell, -1 if absent. */
// FUNC 0x0023C4A8 gt2k_menu::pList::indexOf
int pList::indexOf(pCell *cell)
{
    int i = 0;
    pCell *c;

    for (c = head; c != 0; c = c->next, i++) {
        if (c == cell)
            return i;
    }
    return -1;
}

/* Cell n (0 = head), NULL past the end. */
// FUNC 0x0023C4D8 gt2k_menu::pList::at
pCell *pList::at(int n)
{
    pCell *cell = head;
    int i = 0;

    while (cell != 0 && i != n) {
        cell = cell->next;
        i++;
    }
    return cell;
}

/* Moves cell to position pos (0 <= pos < count()); 1 if done.  No
 * callers. */
// FUNC 0x0023C508 gt2k_menu::pList::move
int pList::move(pCell *cell, int pos)
{
    if (pos < 0 || pos >= count())
        return 0;
    unlink(cell);
    insert(cell, pos);
    return 1;
}

/* index.txt: pList::virtual_1 (slot 1).  Sorts the cells: copies them into
 * a stack array (alloca), unlinks them all, qsort's the array of pointers
 * with cmp, appends them back. */
// FUNC 0x0023C580 gt2k_menu::pList::sort
void pList::sort(int (*cmp)(const void *, const void *))
{
    int n = count();
    pCell **cells = (pCell **)__builtin_alloca(n * sizeof(pCell *));
    int i;

    for (i = 0; i < n; i++)
        cells[i] = at(i);
    unlinkAll();
    qsort(cells, n, sizeof(pCell *), cmp);
    for (i = 0; i < n; i++)
        append(cells[i]);
}

// FUNC 0x0023C678 gt2k_menu::pList::getTail
/* inline in game/menu/framework/T495_menu_list.h; egcs emits this copy (unreferenced) */

// FUNC 0x0023C680 gt2k_menu::pList::getHead
/* inline in game/menu/framework/T495_menu_list.h; egcs emits this copy (unreferenced) */

// FUNC 0x0023C688 gt2k_menu::pList::pList
/* inline in game/menu/framework/T495_menu_list.h; egcs emits this copy (head = tail = 0,
 * the vptr 0x00343AB0; unreferenced) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (no bases). */
// FUNC 0x0023C6A8 __tfQ29gt2k_menu5pList
extern "C" void *__tfQ29gt2k_menu5pList(void)
{
    if (__tiQ29gt2k_menu5pList[0] == 0)
        __rtti_user(__tiQ29gt2k_menu5pList, "Q29gt2k_menu5pList");
    return __tiQ29gt2k_menu5pList;
}
#endif

namespace gt2k_menu {

// FUNC 0x0023C6E8 gt2k_menu::pCell::getPrev
/* inline in game/menu/framework/T495_menu_list.h; egcs emits this copy (unreferenced) */

// FUNC 0x0023C6F0 gt2k_menu::pCell::getNext
/* inline in game/menu/framework/T495_menu_list.h; egcs emits this copy (unreferenced) */

// FUNC 0x0023C6F8 gt2k_menu::pCell::clearLinks
/* inline in game/menu/framework/T495_menu_list.h; egcs emits this copy (next = prev = 0;
 * unreferenced) */

// FUNC 0x0023C708 gt2k_menu::pCell::pCell
/* inline in game/menu/framework/T495_menu_list.h; egcs emits this copy (prev = next = 0,
 * the vptr 0x00343AC8; unreferenced) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (no bases). */
// FUNC 0x0023C728 __tfQ29gt2k_menu5pCell
extern "C" void *__tfQ29gt2k_menu5pCell(void)
{
    if (__tiQ29gt2k_menu5pCell[0] == 0)
        __rtti_user(__tiQ29gt2k_menu5pCell, "Q29gt2k_menu5pCell");
    return __tiQ29gt2k_menu5pCell;
}
#endif
