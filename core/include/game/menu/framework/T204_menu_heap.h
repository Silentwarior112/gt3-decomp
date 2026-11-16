/* T204_menu_heap.h - the menu heap stack (translation unit T204, unit u063),
 * 0x001B3918..0x001B3B58.
 *
 * Four pdistd::Allocator objects; a menu pushes one over a memory block when
 * it starts (menu_heap_push) and pops it when it ends.  Everything the menus
 * allocate comes from the current one: gt2k_menu::Object::operator new
 * (T220), the operators (T216 / T217), the Item textures and texts (T213),
 * the car buffers (T212).  The heaps themselves (g_0049C7E0) are a static
 * array of T204.
 */
#ifndef GAME_T204_MENU_HEAP_H
#define GAME_T204_MENU_HEAP_H

#include "game/game.h"
#include "engine/pdistd/T530_allocator.h"            /* pdistd::Allocator, held by value (the array) */
#include <stddef.h>                         /* size_t */

#ifdef __cplusplus                          /* a C++ translation unit (C++ linkage) */

extern int g_00352510;                      /* 0x00352510 gp-0x54E0 index of the current one, -1: none */

pdistd::Allocator *menu_heap_current(void); /* 0x001B3918 &g_0049C7E0[g_00352510] */
/* The next heap, emptied (the inline Allocator::reset), assigned [mem, mem +
 * size). */
void menu_heap_push(void *mem, size_t size);    /* 0x001B3938 */
/* Unassigns the current heap, steps back, empties the heap it popped. */
void menu_heap_pop(void);                   /* 0x001B3998 */
int  menu_heap_depth(void);                 /* 0x001B3A00 g_00352510 */
int  menu_heap_active(void);                /* 0x001B3A08 1 while a heap is pushed (no callers) */

/* egcs's _GLOBAL_$D$ / $I$ functions of T204 (no names in C): the .dtors
 * entry 0x0028F230 destroys the four heaps (last first, through the virtual
 * destructor), the .ctors entry 0x0028F154 builds them (0x001B3A30 /
 * 0x001B3A98). */

#endif /* __cplusplus */

#endif /* GAME_T204_MENU_HEAP_H */
