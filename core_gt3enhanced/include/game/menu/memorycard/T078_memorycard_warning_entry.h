/* T078_memorycard_warning_entry.h - memorycard_warning, the boot-time memory
 * card warning's entry point (translation unit T078, unit u032; the
 * controller class is in T078_memorycard_warning.h).
 *
 * In its own header since u001's data pass (2026-10-17): of the originals,
 * only T078's .rodata has MemorycardWarningControl's type name; its caller
 * main (T000) saw the function without the class (egcs writes the type name
 * of every class it sees whose virtual functions are all inline).
 * T078_memorycard_warning.h includes this header where it declared the
 * function. */
#ifndef GAME_T078_MEMORYCARD_WARNING_ENTRY_H
#define GAME_T078_MEMORYCARD_WARNING_ENTRY_H

#include "game/game.h"

/* The warning screen over a 1 MB menu heap (main, T000).  Always 1. */
int memorycard_warning(void);               /* 0x0015C150 */

#endif /* GAME_T078_MEMORYCARD_WARNING_ENTRY_H */
