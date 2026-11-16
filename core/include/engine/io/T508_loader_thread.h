/* T508_loader_thread.h - the loader thread (? name) (translation unit T508,
 * unit u111), 0x00242650..0x00242B38.
 *
 * A thread (priority 0x10, a 0x1000-byte static stack) started and stopped
 * around the menus.  Its command word: 1 loads one file (unpacked, from
 * vol_device_object; T507's AsyncLoader drives it), 2 makes it exit; while
 * idle it waits a vsync and runs the calls deferred into its 16-entry ring
 * (0x002F3C30). */
#ifndef GAME_T508_LOADER_THREAD_H
#define GAME_T508_LOADER_THREAD_H

#include "game/game.h"

/* A deferred call: func(arg, arg2) on the loader thread. */
typedef void (*LoaderFn)(void *arg, int arg2);

void loader_init(void);                         /* 0x00242650 no path, no command, the ring cleared (game init) */
void loader_thread_start(void);                 /* 0x00242750 CreateThread + StartThread */
void loader_thread_stop(void);                  /* 0x002427C0 cancels a load, sends the exit command and waits */
/* Queues func(arg, arg2) for the thread's next idle step (interrupts off;
 * no overflow check). */
void loader_defer(LoaderFn func, void *arg, int arg2);  /* 0x002428F8 */
/* 0x00242B00 (.ctors 0x0028F1A8): egcs's static initialiser of the ring
 * g_002F3C30 (its 16 entries zeroed), compiler-generated (T508 defines the
 * ring). */

#endif /* GAME_T508_LOADER_THREAD_H */
