/* T240 - gt2k_menu::PageHistory (0x001C6FC0..0x001C7220), the page "back"
 * stack of the PageManager (+0x1E4): a ring of 32 page names.  push() writes
 * at `top` and advances it, dropping the oldest name when the ring is full;
 * both indices wrap (clamp 0) over 0..31.
 *
 * Data (retail .rodata 0x00336B18..0x00336B5E): SmartIndexT<int>'s vtable
 * and type name only.  The vtable (its first item) is counted to T239's range
 * by the code references: the anchor below.
 */
// DATA_START 0x00336B18
#include "game/menu/framework/T240_page_history.h"
#ifndef BOOT_OLD_GCC
#include "game/common/T017_smart_index.h"          /* __tiQ29gt2k_menut11SmartIndexT1Zi */
#endif
#include "game/menu/framework/menu_links_internal.h"

namespace gt2k_menu {

/* idx->move(delta) through the vtable (as T235's): the indices call slot 3
 * virtually, the address computed for each call. */
static inline int moveIndex(SmartIndexT<int> *idx, int delta)
{
    return idx->move(delta);
}

enum { HISTORY_SIZE = 32 };                 /* PageHistory::names */

/* The inline SmartIndexT() of both indices, then [0, 32) at 0, wrapping.
 * Caller: PageManager::PageManager (0x001B6250).  Not exact (96.2%): retail
 * stores top.value right after first.end, sched1 here takes first.clamp
 * first (top.value is output-dependent on first.end, the insn just
 * scheduled); same with an int `wrapped' and with clamp = 0 stores. */
// FUNC 0x001C6FC0 gt2k_menu::PageHistory::PageHistory
PageHistory::PageHistory() : first(), top()
{
    first.setRange(0, HISTORY_SIZE);
    first.set(0);
    first.setClamp(false);
    top.setRange(0, HISTORY_SIZE);
    top.set(0);
    top.setClamp(false);
}

/* Callers: 0x001B6AF0, 0x001B6B68 (T211).  The moves are virtual calls,
 * through pointers taken before the call / the test (retail computes them
 * there: &top in $s1 before strcpy, &first in the branch's delay slot). */
// FUNC 0x001C7078 gt2k_menu::PageHistory::push
void PageHistory::push(const char *page)
{
    SmartIndexT<int> *t = &top;
    strcpy(names[top.value], page);
    t->move(1);
    SmartIndexT<int> *f = &first;
    if (top.value == first.value)
        f->move(1);
}

/* Callers: 0x001B6E10, 0x001B7848 (T211). */
// FUNC 0x001C70F8 gt2k_menu::PageHistory::pop
void PageHistory::pop()
{
    SmartIndexT<int> *t = &top;
    if (top.value != first.value)
        t->move(-1);
}

/* The newest name (the one before `top`), NULL if empty.  Caller 0x001B6E10.
 * Not exact (77.9%): retail copies top through $a2 with the loads begin,
 * wrapped, clamp, end (stores value, begin, clamp, end, wrapped); here the
 * clamp load goes first.  Tried: the copy from a pointer taken before the
 * test (*t), newest(top), default construction + assignment (45 insns),
 * an explicit SmartIndexT copy constructor (both forms, shadow header), a
 * pointer for the virtual move; the reference compiler gives the same. */
// FUNC 0x001C7138 gt2k_menu::PageHistory::getTop
const char *PageHistory::getTop()
{
    if (top.value != first.value) {
        SmartIndexT<int> newest = top;      /* a copy (vptr = SmartIndexT<int>'s) */
        moveIndex(&newest, -1);
        return names[newest.value];
    }
    return 0;
}

/* The oldest name, NULL if empty.  Caller 0x001B6918. */
// FUNC 0x001C71C0 gt2k_menu::PageHistory::getFirst
const char *PageHistory::getFirst()
{
    if (top.value != first.value)
        return names[first.value];
    return 0;
}

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function of SmartIndexT<int>: T240's copy,
 * never referenced (see T235's).  Weak so that the copies can be linked. */
// FUNC 0x001C71E0 __tfQ29gt2k_menut11SmartIndexT1Zi
extern "C" __attribute__((weak)) void *__tfQ29gt2k_menut11SmartIndexT1Zi(void)
{
    if (__tiQ29gt2k_menut11SmartIndexT1Zi[0] == 0)
        __rtti_user(__tiQ29gt2k_menut11SmartIndexT1Zi, "Q29gt2k_menut11SmartIndexT1Zi");
    return __tiQ29gt2k_menut11SmartIndexT1Zi;
}
#endif
