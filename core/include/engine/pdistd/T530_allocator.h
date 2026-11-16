/* T530_allocator.h - pdistd::Allocator (allocator.cxx; translation unit T530, unit u125).
 *
 * The class is the same machine code as the bootstrap's and is declared once, in
 * the shared include/pdistd/allocator.h (its address comments are the
 * bootstrap's).  Core addresses (src/engine/pdistd/T530_allocator.cpp):
 *   Allocator(int firstFit)                          0x0026A7C8
 *   Allocator(void *base, u32 size, int firstFit)    0x0026A7F0  (unreferenced)
 *   ~Allocator()                    [0] +0x0C        0x0026A828
 *   alignRepStart 0x0026A868, rebuildHeadRep 0x0026A928, insertRepAt 0x0026A958,
 *   unlinkRep 0x0026A988, pushFreeRep 0x0026A9A8, removeFreeRep 0x0026A9C8
 *   void *assign(void *base, u32 size)               0x0026A9F0
 *   void *unassign(void)                             0x0026AA50
 *   void *allocate(u32 size, u32 align)              0x0026AA68  (heap_alloc_core)
 *   void *reallocate(void *ptr, u32 size, u32 align) 0x0026ABC0
 *   void  free(void *ptr)                            0x0026AD88  (heap_free_core)
 *   u32   getMaxFreeSize(void)                       0x0026AEC0
 *   u32   getTotalFreeSize(void)                     0x0026AF28
 *   void  debug(void)                                0x0026AF90
 *   int   countFreeReps(void)                        0x0026B088
 *   inline repValid / reset, out-of-line copies      0x0026B128 / 0x0026B170
 * vtable _vt$Q26pdistd9Allocator 0x00346098 (1 slot), vptr +0x14 (egcs).
 * typeinfo __tfQ26pdistd9Allocator 0x0026B188, RTTI name 0x003460A8,
 * __user_type_info 0x003534F0.  Size 0x18 (the global heap object 0x004A9EC0,
 * T532).
 *
 */
#ifndef GAME_T530_ALLOCATOR_H
#define GAME_T530_ALLOCATOR_H

#include "game/game.h"
#include "pdistd/allocator.h"                   /* class pdistd::Allocator */

#ifdef __cplusplus
EGCS_SIZE(pdistd::Allocator, 0x18);
EGCS_SIZE(pdistd::Allocator::Rep, 0x10);

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" void *__tfQ26pdistd9Allocator(void);         /* 0x0026B188 */
#endif
#endif /* __cplusplus */

/* The .data after T530's own (g_mpeg_callbacks 0x002FF140, the voice bank
 * 0x002FF168, the sequencer 0x002FF958, the SPU2 state 0x00302698) belongs to
 * the frameless C objects of G002 and is declared in G002_mpeg_frameq.h. */

#endif /* GAME_T530_ALLOCATOR_H */
