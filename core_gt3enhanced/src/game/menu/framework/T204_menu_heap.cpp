/* T204 - the menu heap stack.  0x001B3918..0x001B3B58.
 *
 * Four pdistd::Allocator objects (g_0049C7E0, 0x18 bytes each); the menus
 * push one over a memory block when they start (menu_heap_push) and pop it
 * when they end.  gt2k_menu::Object::operator new allocates from the
 * current one. */
#include "game/menu/framework/T204_menu_heap.h"
#include "game/menu/framework/menu_buttons_internal.h"

/* Data (retail): .sdata 0x00352510 (g_00352510, -1), .bss 0x0049C7E0 (the
 * heaps); no .rodata (the original saw no class egcs writes a type name
 * for: pdistd::Allocator's key function is T530's). */

/* The heaps (.bss 0x0049C7E0, 0x18 bytes each), only used here.  Built
 * by egcs's static initialiser (Allocator(): best fit), destroyed by its
 * static destructor.  `static`: egcs puts a public object that has a
 * constructor in .data (T039's g_0029AD40), a static one in .bss, where
 * retail has this array; no other unit refers to it. */
static pdistd::Allocator g_0049C7E0[4];     /* 0x0049C7E0 */

/* Index of the current heap in g_0049C7E0, -1 while none is pushed
 * (.sdata, gp-0x54E0).  Initialised data before the first function: egcs
 * names the static initialiser / destructor and the frame info after it. */
int g_00352510 = -1;                        /* 0x00352510 */

// FUNC 0x001B3918 menu_heap_current
pdistd::Allocator *menu_heap_current(void)
{
    return &g_0049C7E0[g_00352510];
}

/* Next heap, emptied (the inline Allocator::reset), assigned the block. */
// FUNC 0x001B3938 menu_heap_push
void menu_heap_push(void *mem, size_t size)
{
    g_00352510++;
    g_0049C7E0[g_00352510].reset();
    g_0049C7E0[g_00352510].assign(mem, size);
}

/* Unassigns the current heap, steps back, then empties the heap it popped. */
// FUNC 0x001B3998 menu_heap_pop
void menu_heap_pop(void)
{
    g_0049C7E0[g_00352510].unassign();
    int popped = g_00352510;
    g_00352510 = popped - 1;
    g_0049C7E0[popped].reset();
}

// FUNC 0x001B3A00 menu_heap_depth
int menu_heap_depth(void)
{
    return g_00352510;
}

/* 1 while a heap is pushed (depth >= 0: the sign bit of ~depth).  No
 * callers. */
// FUNC 0x001B3A08 menu_heap_active
int menu_heap_active(void)
{
    return (u32)~menu_heap_depth() >> 31;
}

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_0049C7E0 (.dtors entry 0x0028F230, .ctors entry 0x0028F154), emitted at
 * the end of the unit, the destructor first.  The destructor runs the four
 * virtual destructors (flags 0), last first, behind a never-taken NULL
 * test of the array; the initialiser builds Allocator(0) on each, its
 * landing pad destroys the ones already built. */
// FUNC 0x001B3A30 T204_static_dtor (_GLOBAL_$D$g_00352510)
/* implicit: ~Allocator (the static destructor of g_0049C7E0) */

// FUNC 0x001B3A98 T204_static_init (_GLOBAL_$I$g_00352510)
/* implicit: Allocator (the static initialiser of g_0049C7E0) */
