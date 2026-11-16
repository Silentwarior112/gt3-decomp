/* T265 - the RawWindow class registry (0x001D8178..0x001D8260).
 *
 * Every window unit's static constructor builds one RawWindowRegister per
 * class; the constructor pushes it on the list g_003528F8.  A layout names
 * the class of a window; createRegistered finds it by name and calls its
 * create function.
 */
#include "game/menu/framework/T265_raw_window_register.h"    /* this translation unit */
#include "game/common/curves_internal.h"
#include "game/menu/framework/T264_raw_window.h"           /* (no use: RawWindow, Item's Operator classes) */
#include "runtime/newlib/G007_stdio.h"                /* strcmp */

/* .rodata: only the type names of the Operator classes (OperatorList ..
 * Operator, T213's) after T264's, 0x003393F8..0x003394D0; no code
 * references them. */
// DATA_START 0x003393F8

namespace gt2k_menu {

/* .sdata: the list head (gp-0x50F8), zero in the file */
RawWindowRegister *g_003528F8 = NULL;       /* 0x003528F8 */

// FUNC 0x001D8178 gt2k_menu::findRegistered
RawWindowRegister *findRegistered(const char *name)
{
    RawWindowRegister *reg;

    for (reg = g_003528F8; reg != NULL; reg = reg->next) {
        if (strcmp(name, reg->cls->name) == 0)
            return reg;
    }
    return NULL;
}

/* NULL if no class of that name is registered */
// FUNC 0x001D81D8 gt2k_menu::createRegistered
RawWindow *createRegistered(const char *cls, const RawWindowArgs *args)
{
    RawWindowRegister *reg = findRegistered(cls);

    if (reg != NULL)
        return reg->cls->create(args);
    return NULL;
}

// FUNC 0x001D8220 gt2k_menu::RawWindowRegister::RawWindowRegister
RawWindowRegister::RawWindowRegister(const RawWindowClass *c)
{
    cls = c;
    next = g_003528F8;
    g_003528F8 = this;
}

// FUNC 0x001D8238 gt2k_menu::RawWindowRegister::~RawWindowRegister
/* implicit: operator delete; does not unlink; the deleting form (flags & 1) calls the global operator
 * delete (0x0027C0C0) */
RawWindowRegister::~RawWindowRegister()
{
}

} /* namespace gt2k_menu */
