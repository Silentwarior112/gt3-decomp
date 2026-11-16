/* T248 - the menu signal flag.  0x001CBE30..0x001CBE80.
 *
 * One .sbss int (g_0035324C): an operator script sets it through
 * menu_signal_set (the callback of operator 0x1E in T211's fade-out script
 * g_00333AD0, data 0x00333B34), the PageManager (PageManager::fadeOut,
 * T211) polls and clears it, ControlBase::attach clears it.  (The names are
 * not the original ones.)
 *
 * Data: no .rodata in the original; the flag is in the .sbss (0x0035324C),
 * so it was a file static (egcs writes a C++ global without an initialiser
 * as zeros in .sdata). */
#include "game/menu/framework/T248_menu_signal.h"
#include "game/menu/framework/render_base_internal.h"

static int g_0035324C;                      /* 0x0035324C gp-0x47A4 (.sbss) */

/* Callers: PageManager::fadeOut (T211), ControlBase::attach. */
// FUNC 0x001CBE30 menu_signal_clear
void menu_signal_clear(void)
{
    g_0035324C = 0;
}

/* Empty (called by ControlBase::detach). */
// FUNC 0x001CBE38 menu_signal_nop
void menu_signal_nop(void)
{
}

// FUNC 0x001CBE40 menu_signal_set
void menu_signal_set(void)
{
    g_0035324C = 1;
}

// FUNC 0x001CBE50 menu_signal_reset
void menu_signal_reset(void)
{
    g_0035324C = 0;
}

// FUNC 0x001CBE58 menu_signal_get
int menu_signal_get(void)
{
    return g_0035324C;
}

/* 0 (and clears it) once the signal is set, 1 while it is not. */
// FUNC 0x001CBE60 menu_signal_poll
int menu_signal_poll(void)
{
    if (g_0035324C != 0) {
        g_0035324C = 0;
        return 0;
    }
    return 1;
}
