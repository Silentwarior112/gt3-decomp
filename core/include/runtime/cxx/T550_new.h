/* T550_new.h - <new> of the GCC 2.9x-ee C++ runtime: bad_alloc, new_handler,
 * set_new_handler and the placement forms (translation unit T550, egcs
 * libgcc2 new.cc with #pragma implementation "new", unit u136),
 * 0x0027C010..0x0027C0BF.  The global allocation functions have their own
 * translation units: T544 operator delete, T545 operator new, T546 operator
 * delete[], T547 operator new[].
 *
 * The runtime classes are in the GLOBAL namespace (mangled "9bad_alloc":
 * -fhonor-std was off), so they do not clash with GCC 14's std:: ones.
 *
 * egcs puts a class's typeinfo function (__tf<mangled>) in vtable entry 0
 * (vptr+4); typeid(*obj) and the throw in operator new call through it.
 * GCC 14 has no such entry, so for the GCC 14 build only the exception
 * classes declare __tf() as their first virtual (a stand-in with no address
 * in the binary; it returns __tf<mangled>()).  egcs does not see it, so its
 * vtables keep the binary's slots (T543's exception must do the same). */
#ifndef GAME_T550_NEW_H
#define GAME_T550_NEW_H

#include "game/game.h"
#include <stddef.h>
#include "runtime/cxx/T543_exception.h"            /* base exception */

class type_info;                            /* T548 */

/* Typeinfo function of bad_alloc (vtable entry 0).  GCC 14 build only:
 * egcs generates it (see T548_tinfo.h). */
#ifndef BOOT_OLD_GCC
extern "C" const type_info &__tf9bad_alloc(void);       /* 0x0027C040 */
#endif

#if defined(BOOT_OLD_GCC) && defined(__NEW__)
/* egcs's own <new> came first (u136, as each original member of the
 * runtime): bad_alloc (inline what(), implicit destructor), new_handler,
 * set_new_handler and the inline placement forms are its declarations, the
 * same members as below. */
#else
/* vtable _vt$9bad_alloc 0x003483D8 (2 slots): entry 0 (vptr+4) __tf9bad_alloc
 * 0x0027C040.  typeinfo __ti9bad_alloc 0x004B2B40 (.bss, a __si_type_info,
 * base exception).  Size 4, vptr at +0x00: operator new throws an
 * __eh_alloc(4) object. */
class bad_alloc : public exception {
public:
    bad_alloc() {}                              /* inline (egcs <new>): vptr = 0x003483D8 */
#ifndef BOOT_OLD_GCC
    virtual const type_info &__tf() const;      /* GCC 14 stand-in for entry 0 = __tf9bad_alloc */
#endif
    virtual ~bad_alloc();                       /* [0] +0x0C 0x0027C090 */
    virtual const char *what() const throw();   /* [1] +0x14 0x0027C030 returns "bad_alloc" */
};
EGCS_SIZE(bad_alloc, 4);

typedef void (*new_handler)(void);

new_handler set_new_handler(new_handler handler);       /* 0x0027C010 (no callers) */

/* Default placement forms: inline in egcs <new>; these are new.cc's
 * out-of-line copies (no callers).  The two bodies are identical; egcs
 * emits <new>'s inline functions in reverse definition order, so the array
 * form comes first (checked: ee-gcc 2.9-ee-991111b on new.cc emits
 * __vn__FUiPv, then __nw__FUiPv). */
void *operator new(size_t size, void *place) throw();   /* 0x0027C028 */
void *operator new[](size_t size, void *place) throw(); /* 0x0027C020 */
#endif /* BOOT_OLD_GCC && __NEW__ */

/* Polled by operator new (T545) when malloc fails.  .data, initially 0;
 * nothing in the game installs a handler. */
extern new_handler __new_handler;                       /* 0x0030684C */

#endif /* GAME_T550_NEW_H */
