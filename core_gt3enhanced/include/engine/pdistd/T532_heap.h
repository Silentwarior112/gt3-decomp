/* T532_heap.h - the global heap: the C malloc family over one pdistd::Allocator
 * (translation unit T532, unit u125).
 *
 * The heap object (pdistd::Allocator, 0x18 bytes, best fit) is at 0x004A9EC0
 * (.bss) and manages [align16(&_end) .. 0x01F00000), zero-filled and assigned
 * on the first malloc / realloc / memalign / calloc (heap_init_once
 * 0x0026C228, static).  These functions override newlib's: Sony libraries call
 * them as plain libc symbols (also declared, with bootstrap addresses, in
 * include/libc.h).  Unreferenced here: heap_max_free_size 0x0026C2B8,
 * heap_total_free_size 0x0026C2D8, heap_dump 0x0026C2F8,
 * heap_free_block_count 0x0026C318, calloc 0x0026C338.
 */
#ifndef GAME_T532_HEAP_H
#define GAME_T532_HEAP_H

#include "game/game.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void  free(void *ptr);                          /* 0x0026C380 Allocator::free; no heap_init_once */
void *malloc(size_t size);                      /* 0x0026C3A8 allocate(size, 16) */
void *realloc(void *ptr, size_t size);          /* 0x0026C3E0 reallocate(ptr, size, 16) */
void *memalign(size_t align, size_t size);      /* 0x0026C428 allocate(size, max(align, 16)) */
/* Unreferenced in GT3; the enhanced engine logs them at each race start
 * (T154 race_display_log_new_images: the heap left once a race is loaded) */
unsigned int heap_max_free_size(void);          /* 0x0026C2B8 Allocator::getMaxFreeSize */
unsigned int heap_total_free_size(void);        /* 0x0026C2D8 Allocator::getTotalFreeSize */

/* The static destructor / constructor of the global Allocator (.dtors /
 * .ctors, 0x0026C470 ~Allocator(2) / 0x0026C498 Allocator(0)) are
 * compiler-generated from heap_object's definition in T532 (nothing calls
 * them by name). */

#ifdef __cplusplus
}
#endif

#endif /* GAME_T532_HEAP_H */
