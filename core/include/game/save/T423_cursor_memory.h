/* T423_cursor_memory.h - CursorMemory, a page name and the button index
 * last used on it (translation unit T423, unit u096), 0x00216290..0x00216308.
 * MenuStatus+0x594 (T427). */
#ifndef GAME_T423_CURSOR_MEMORY_H
#define GAME_T423_CURSOR_MEMORY_H

#include "game/game.h"

/* No vtable.  Size 0x44 (embedded in MenuStatus at 0x594, next member 0x5D8). */
class CursorMemory {
public:
    CursorMemory();                             /* 0x00216290 */
    void set(const char *name, int index);      /* 0x002162A0 */
    int  get(const char *name);                 /* 0x002162D8 index if page == name, else 0 */

    char page[0x40];                            /* 0x00 */
    int  index;                                 /* 0x40 */
};
EGCS_SIZE(CursorMemory, 0x44);

#endif /* GAME_T423_CURSOR_MEMORY_H */
