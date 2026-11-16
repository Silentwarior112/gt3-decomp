/* T216_menu_transitive.h - gt2k_menu::Transitive, gt2k_menu::OperatorHolder
 * (translation unit T216, unit u065), 0x001BB430..0x001BBC58.
 *
 * A Transitive is a Drawable (T259) animated by operator scripts (T217):
 * start(script) wraps a new Operator in an OperatorHolder, runs its first
 * step at once and appends it to the object's List<OperatorHolder>;
 * virtual_1 (the per-frame update) steps every operator once while `active'
 * and stops as soon as one of them released the object.  Base of Rectangle
 * (T256) -> Image (T257) -> Item (T213), of Car (T212) and of most page
 * objects.
 *
 * Also emitted here (with their vtables): List<OperatorHolder>::~List
 * 0x001BBA08 and Cell<OperatorHolder>::~Cell 0x001BBAE8, the instances of
 * the template destructors (menu_list.h); the typeinfo
 * functions of Transitive, OperatorHolder, List / Cell<OperatorHolder> and
 * Operator (T217's user type).
 */
#ifndef GAME_T216_MENU_TRANSITIVE_H
#define GAME_T216_MENU_TRANSITIVE_H

#include "game/game.h"
#include "game/menu/framework/T259_menu_drawable.h"        /* base gt2k_menu::Drawable */
#include "game/menu/framework/menu_list.h"                  /* gt2k_menu::Cell<T>, List<T> (base, member) */
#include "game/menu/framework/T217_operator.h"             /* base gt2k_menu::Operator, OperatorCode */
#include <stddef.h>                         /* size_t */

namespace gt2k_menu {

/* A running operator script of a Transitive: an Operator at +0x00 and a
 * Cell<OperatorHolder> at +0x28 (the __rtti_class base list 0x00334380:
 * {Operator, public +0}, {Cell<OperatorHolder>, public +0x28}).  The only
 * vtable 0x003342A8 (3 slots, this-delta -0x28) is stored in the Cell's vptr
 * (+0x30).  typeinfo __tfQ29gt2k_menu14OperatorHolder 0x001BBB10
 * (__class_type_info 0x004B1300).  Size 0x38 (operator new in
 * Transitive::start). */
class OperatorHolder : public Operator, public Cell<OperatorHolder> {
public:
    /* Inline (expanded in Transitive::start, with Operator()). */
    OperatorHolder(int id_) : id(id_) {}
    static void *operator new(size_t size); /* 0x001BB430 menu heap, 16-aligned; Allocator::debug() on failure */
    static void  operator delete(void *p);  /* 0x001BB4A8 */
    /*  [0] +0x0C the implicit destructor 0x001BBB70 (u065 matching: emitted
     *  with the vtable in T216, it calls ~Cell<OperatorHolder> out of line;
     *  a user-declared one inlines it),
     *  [1] +0x14 pCell::isBefore 0x0023C120, [2] +0x1C pCell::isNot 0x0023C128 */

    /*  Cell<OperatorHolder>                   0x28 (next, prev, vptr +0x30) */
    int id;                                 /* 0x34 start()'s id (stopOperator); init() puts it in Operator::id too */
};

/* A list of operators: no code of it survives, only its typeinfo name
 * "Q29gt2k_menu12OperatorList", which egcs writes into the .rodata of every
 * unit that sees this header, between Transitive's (or the unit's own) names
 * and List<OperatorHolder>'s (retail T213 0x00333EC8, T216 0x00334308).  (?
 * Its members: Transitive::operators is a plain List<OperatorHolder>, whose
 * vtable Transitive() stores.)  Defined before `#pragma interface', as
 * OperatorHolder: T216 emits no vtable for it. */
class OperatorList : public List<OperatorHolder> {
};

#ifdef BOOT_OLD_GCC
/* From here on (egcs applies it to the classes defined after it), with
 * #pragma implementation in T216_menu_transitive.cpp: egcs emits the copies of
 * the inline accessors below and the vtable only there, before __tf. */
#pragma interface
#endif

/* vtable 0x00334248 (8 slots).  typeinfo __tfQ29gt2k_menu10Transitive
 * 0x001BBA48 (__si_type_info 0x004B2500, base Drawable).  Size 0xC4 (the data
 * of Rectangle and Car start at +0xC4). */
class Transitive : public Drawable {
public:
    /* Object(), the Drawable part (coordinates 0, size 100 x 100, colours
     * 0.502), then alphaRatio 1, visible 1, active 1, nextId 0. */
    Transitive();                           /* 0x001BB4F8 */
    virtual ~Transitive();                  /* [0] +0x0C 0x001BB5F8 clearOperators() */
    virtual void virtual_1();               /* [1] +0x14 0x001BB940 one step of every operator while active */
    /*  [2] +0x1C Drawable::virtual_2 0x001D0478, [3] +0x24 Object::virtual_3 0x001BFF00 */
    /* Close: start(script), the default close script g_003341E8 if NULL. */
    virtual void virtual_4(const OperatorCode *script); /* [4] +0x2C 0x001BB9C8 */
    virtual void virtual_5();               /* [5] +0x34 0x001BB9F0 empty (operator code 12) */
    virtual void virtual_6();               /* [6] +0x3C 0x001BB9F8 empty (operator code 32) */
    virtual void virtual_7();               /* [7] +0x44 0x001BBA00 empty (operator code 33) */

    /* Queues script (first step at once); its id, -1 if script is NULL. */
    int  start(const OperatorCode *script); /* 0x001BB670 */
    OperatorHolder *getOperator(int n);     /* 0x001BB7B8 operator number n, NULL past the end */
    void rewindOperators();                 /* 0x001BB7E0 pc = 0 for every operator */
    void stopOperator(int id);              /* 0x001BB830 removes the operator `id' */
    void clearOperators();                  /* 0x001BB8A8 removes every operator (interrupts off) */
    void sleep();                           /* 0x001BBA30 active = 0 */
    void wakeup();                          /* 0x001BBA38 active = 1 */

    List<OperatorHolder> operators;         /* 0xB0 (head, tail, vptr +0xB8) */
    int nextId;                             /* 0xBC id of the next start() */
    int active;                             /* 0xC0 sleep() 0 / wakeup() 1; 0 once an operator released the object */
};

/* The inline accessors 0x001BBA30..0x001BBA38: no callers, the users expand them; defined
 * in reverse address order (egcs emits the #pragma implementation copies
 * in reverse definition order). */
inline void Transitive::wakeup() { active = 1; }  /* 0x001BBA38 */
inline void Transitive::sleep() { active = 0; }  /* 0x001BBA30 */

/* Only used here (or not at all): F_001BB758 0x001BB758 (the holder of an
 * id in a list, no callers: ? an inline helper emitted out of line),
 * F_001BB488 0x001BB488 / F_001BB4D8 0x001BB4D8 (no callers: Operator's
 * operator new / delete forwarded), the default close script g_003341E8
 * (T216 .rodata: opWait 0.15 s, opRelease). */

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::OperatorHolder, 0x38);
EGCS_SIZE(gt2k_menu::Transitive, 0xC4);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu10Transitive(void);                           /* 0x001BBA48 */
extern u32 __tiQ29gt2k_menu10Transitive[3];                         /* 0x004B2500 its __si_type_info */
void *__tfQ29gt2k_menut4List1ZQ29gt2k_menu14OperatorHolder(void);   /* 0x001BBA98 */
void *__tfQ29gt2k_menu14OperatorHolder(void);                       /* 0x001BBB10 */
void *__tfQ29gt2k_menut4Cell1ZQ29gt2k_menu14OperatorHolder(void);   /* 0x001BBBC8 */
}
#endif

#endif /* GAME_T216_MENU_TRANSITIVE_H */
