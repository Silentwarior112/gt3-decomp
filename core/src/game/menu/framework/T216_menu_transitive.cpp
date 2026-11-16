#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/game/menu/framework/T216_menu_transitive.cpp"
#pragma implementation "src/game/menu/framework/T216_menu_transitive.cpp"
#pragma implementation "T216_menu_transitive.h"
#endif
/* T216 - gt2k_menu::Transitive (0x001BB430..0x001BBC58).
 *
 * A Drawable animated by operator scripts (T217): start(script) wraps a new
 * Operator in an OperatorHolder (an Operator plus a Cell<OperatorHolder>
 * at +0x28), runs its first step at once and appends it to the object's
 * List<OperatorHolder>; virtual_1 steps every operator once per frame and
 * stops as soon as one of them killed the object.  Also egcs's copies of
 * the List / Cell<OperatorHolder> destructors and the typeinfo functions.
 */
/* Retail's type names (T216 .rodata, reverse definition order): Transitive,
 * OperatorList, List<OperatorHolder>, OperatorHolder (its base list),
 * Cell<OperatorHolder>, Operator: T216_menu_transitive.h's classes. */
#include "game/menu/framework/menu_item_internal.h"
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_current */
#include "game/menu/framework/T216_menu_transitive.h"

namespace gt2k_menu {

/* T216 .rodata (defined below, before the vtables): the default close
 * script of Transitive::virtual_4: opWait (5) 0.15 s, opRelease (6). */
extern const OperatorCode g_003341E8[2];    /* 0x003341E8 */

/* The holder of operator `id' in list, NULL if none.  No callers (? an
 * inline helper of the list emitted out of line). */
OperatorHolder *F_001BB758(List<OperatorHolder> *list, int id);    /* 0x001BB758 */

// FUNC 0x001BB430 gt2k_menu::OperatorHolder::operator new
void *OperatorHolder::operator new(size_t size)
{
    void *p = menu_heap_current()->allocate(size, 0x10);
    if (p == NULL)
        menu_heap_current()->debug();   /* dumps the heap; NULL is returned anyway */
    return p;
}

// FUNC 0x001BB488 F_001BB488
void *F_001BB488(size_t size)
{
    return Operator::operator new(size);
}

// FUNC 0x001BB4A8 gt2k_menu::OperatorHolder::operator delete
void OperatorHolder::operator delete(void *p)
{
    menu_heap_current()->free(p);
}

// FUNC 0x001BB4D8 F_001BB4D8
void F_001BB4D8(void *p)
{
    Operator::operator delete(p);
}

// FUNC 0x001BB4F8 gt2k_menu::Transitive::Transitive
/* Object() (0x001BF390), the inline Drawable() (T259: centre 0, size
 * 100 x 100, colours 0.502, alphaRatio 1, visible 1, the rest 0), then the
 * empty operator list (the inline List<OperatorHolder>()). */
Transitive::Transitive()
    : nextId(0), active(1)
{
    /* implicit: Object (base constructor) */
}

// FUNC 0x001BB5F8 gt2k_menu::Transitive::~Transitive
/* Then ~List<OperatorHolder> (inline: its vptr, pList::~pList 0x0023C138)
 * and ~Object (0x001BFEC0) directly: Drawable's destructor is implicit.  The
 * deleting form ends with Object::operator delete (0x001BF458). */
Transitive::~Transitive()
{
    clearOperators();
    /* implicit: ~pList (inline ~List of the operators), ~Object (base destructor), operator delete (deleting destructor) */
}

// FUNC 0x001BB670 gt2k_menu::Transitive::start
int Transitive::start(const OperatorCode *script)
{
    if (script == NULL)
        return -1;

    int id = nextId++;
    /* OperatorHolder::operator new(0x38), then the inline Operator(),
     * Cell<OperatorHolder>() and OperatorHolder(id) (no NULL test) */
    OperatorHolder *holder = new OperatorHolder(id);
    holder->init(script, id);
    holder->run(this);                  /* the first step at once */
    operators.append(holder);           /* the NULL-tested upcast to the Cell */
    return id;
}

// FUNC 0x001BB758 F_001BB758
OperatorHolder *F_001BB758(List<OperatorHolder> *list, int id)
{
    ListIterator<OperatorHolder> it(list);
    OperatorHolder *holder;

    while ((holder = it.next()) != NULL) {
        if (holder->id == id)
            return holder;
    }
    return NULL;
}

// FUNC 0x001BB7B8 gt2k_menu::Transitive::getOperator
OperatorHolder *Transitive::getOperator(int n)
{
    return static_cast<OperatorHolder *>(static_cast<Cell<OperatorHolder> *>(operators.at(n)));
}

// FUNC 0x001BB7E0 gt2k_menu::Transitive::rewindOperators
void Transitive::rewindOperators()
{
    ListIterator<OperatorHolder> it(&operators);
    OperatorHolder *holder;

    while ((holder = it.next()) != NULL)
        holder->pc = 0;
}

// FUNC 0x001BB830 gt2k_menu::Transitive::stopOperator
/* The operator `id' (OperatorHolder::id) is unlinked and deleted
 * (pList::remove).  Called by opRemove (T217). */
void Transitive::stopOperator(int id)
{
    ListIterator<OperatorHolder> it(&operators);
    OperatorHolder *holder;

    while ((holder = it.next()) != NULL) {
        if (holder->id == id) {
            operators.remove(static_cast<Cell<OperatorHolder> *>(holder));
            return;
        }
    }
}

// FUNC 0x001BB8A8 gt2k_menu::Transitive::clearOperators
void Transitive::clearOperators()
{
    int interrupts = intr_suspend();
    ListIterator<OperatorHolder> it(&operators);
    OperatorHolder *holder;

    while ((holder = it.next()) != NULL)
        operators.remove(static_cast<Cell<OperatorHolder> *>(holder));
    if (interrupts)
        EI();
}

// FUNC 0x001BB940 gt2k_menu::Transitive::virtual_1
/* Update: one step of every operator while active; stops (and goes to
 * sleep) once an operator killed the object (opRelease).  `break', not
 * `return': with a jump to the loop's end in the body, egcs does not move
 * the exit test to the bottom (retail tests at the top). */
void Transitive::virtual_1()
{
    if (active == 0)
        return;

    ListIterator<OperatorHolder> it(&operators);
    OperatorHolder *holder;
    while ((holder = it.next()) != NULL) {
        holder->run(this);
        if (condition != CONDITION_ALIVE) {
            active = 0;
            break;
        }
    }
}

/* The default close script (T216 .rodata, before the vtables): opWait (5)
 * 0.15 s, then opRelease (6): the object is killed. */
const OperatorCode g_003341E8[2] = {        /* 0x003341E8 */
    { 5 /* opWait */, 0, 0, { 0, 0, 0, 0 }, 0.15f, 0.0f, 0.0f, 0.0f, 0.0f,
      { 0, 0, 0, 0, 0, 0, 0, 0 }, 1 },
    { 6 /* opRelease */, 0, 0, { 0, 0, 0, 0 }, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
      { 0, 0, 0, 0, 0, 0, 0, 0 }, 1 },
};

// FUNC 0x001BB9C8 gt2k_menu::Transitive::virtual_4
/* Disappear: start(script), by default g_003341E8 (0.15 s, then kill()). */
void Transitive::virtual_4(const OperatorCode *script)
{
    if (script == NULL)
        script = g_003341E8;
    start(script);
}

// FUNC 0x001BB9F0 gt2k_menu::Transitive::virtual_5
void Transitive::virtual_5()
{
}

// FUNC 0x001BB9F8 gt2k_menu::Transitive::virtual_6
void Transitive::virtual_6()
{
}

// FUNC 0x001BBA00 gt2k_menu::Transitive::virtual_7
void Transitive::virtual_7()
{
}

// FUNC 0x001BBA08 gt2k_menu::List<gt2k_menu::OperatorHolder>::~List
/* inline in game/menu/framework/menu_list.h; egcs emits this copy (vtable 0x00334290 slot
 * 0): the List<OperatorHolder> vptr, then pList::~pList (0x0023C138) with
 * the flags */
/* implicit: ~pList (base destructor) */

// FUNC 0x001BBA30 gt2k_menu::Transitive::sleep(void)
/* inline in game/menu/framework/T216_menu_transitive.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001BBA38 gt2k_menu::Transitive::wakeup(void)
/* inline in game/menu/framework/T216_menu_transitive.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001BBAE8 gt2k_menu::Cell<gt2k_menu::OperatorHolder>::~Cell
/* inline in game/menu/framework/menu_list.h; egcs emits this copy (vtable 0x003342C8 slot
 * 0): the Cell<OperatorHolder> vptr, then pCell::~pCell (0x0023C0C0) with
 * the flags */
/* implicit: ~pCell (base destructor) */

// FUNC 0x001BBB70 gt2k_menu::OperatorHolder::~OperatorHolder
/* implicit (game/menu/framework/T216_menu_transitive.h): egcs emits this copy with the
 * vtable.  The vptr (+0x30), then ~Cell<OperatorHolder> (0x001BBAE8, on
 * +0x28, flags 0) as a call: the template's destructor is not instantiated
 * yet when the implicit one is generated (a user-declared destructor
 * inlines it); the deleting form ends with OperatorHolder::operator delete
 * (0x001BB4A8).  Operator has no destructor. */
/* implicit: ~Cell (base destructor), operator delete (deleting destructor) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo functions. */

// FUNC 0x001BBA48 __tfQ29gt2k_menu10Transitive
extern "C" void *__tfQ29gt2k_menu10Transitive(void)
{
    if (__tiQ29gt2k_menu10Transitive[0] == 0) {
        __tfQ29gt2k_menu8Drawable();
        __rtti_si(__tiQ29gt2k_menu10Transitive, "Q29gt2k_menu10Transitive",
                  (const type_info *)__tiQ29gt2k_menu8Drawable);
    }
    return __tiQ29gt2k_menu10Transitive;
}

// FUNC 0x001BBA98 __tfQ29gt2k_menut4List1ZQ29gt2k_menu14OperatorHolder
extern "C" void *__tfQ29gt2k_menut4List1ZQ29gt2k_menu14OperatorHolder(void)
{
    if (__tiQ29gt2k_menut4List1ZQ29gt2k_menu14OperatorHolder[0] == 0) {
        __tfQ29gt2k_menu5pList();
        __rtti_si(__tiQ29gt2k_menut4List1ZQ29gt2k_menu14OperatorHolder,
                  "Q29gt2k_menut4List1ZQ29gt2k_menu14OperatorHolder",
                  (const type_info *)__tiQ29gt2k_menu5pList);
    }
    return __tiQ29gt2k_menut4List1ZQ29gt2k_menu14OperatorHolder;
}

// FUNC 0x001BBB10 __tfQ29gt2k_menu14OperatorHolder
/* Two bases: __rtti_class with the base list g_00334380. */
extern "C" void *__tfQ29gt2k_menu14OperatorHolder(void)
{
    if (__tiQ29gt2k_menu14OperatorHolder[0] == 0) {
        __tfQ29gt2k_menut4Cell1ZQ29gt2k_menu14OperatorHolder();
        __tfQ29gt2k_menu8Operator();
        __rtti_class(__tiQ29gt2k_menu14OperatorHolder, "Q29gt2k_menu14OperatorHolder",
                     g_00334380, 2);
    }
    return __tiQ29gt2k_menu14OperatorHolder;
}

// FUNC 0x001BBBC8 __tfQ29gt2k_menut4Cell1ZQ29gt2k_menu14OperatorHolder
extern "C" void *__tfQ29gt2k_menut4Cell1ZQ29gt2k_menu14OperatorHolder(void)
{
    if (__tiQ29gt2k_menut4Cell1ZQ29gt2k_menu14OperatorHolder[0] == 0) {
        __tfQ29gt2k_menu5pCell();
        __rtti_si(__tiQ29gt2k_menut4Cell1ZQ29gt2k_menu14OperatorHolder,
                  "Q29gt2k_menut4Cell1ZQ29gt2k_menu14OperatorHolder",
                  (const type_info *)__tiQ29gt2k_menu5pCell);
    }
    return __tiQ29gt2k_menut4Cell1ZQ29gt2k_menu14OperatorHolder;
}

// FUNC 0x001BBC18 __tfQ29gt2k_menu8Operator
/* Operator (T217) has no vtable: a __user_type_info, built here. */
extern "C" void *__tfQ29gt2k_menu8Operator(void)
{
    if (__tiQ29gt2k_menu8Operator[0] == 0)
        __rtti_user(__tiQ29gt2k_menu8Operator, "Q29gt2k_menu8Operator");
    return __tiQ29gt2k_menu8Operator;
}
#endif
