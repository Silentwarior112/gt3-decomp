/* T423 (0x00216290..0x00216308): CursorMemory, a menu page name and the
 * button index the cursor was last on there (MenuStatus+0x594, T427). */
#include "game/save/prize_records_internal.h"
#include "game/save/T423_cursor_memory.h"

// FUNC 0x00216290 CursorMemory::CursorMemory
CursorMemory::CursorMemory()
{
    index = 0;
    page[0] = 0;
}

// FUNC 0x002162A0 CursorMemory::set
void CursorMemory::set(const char *name, int buttonIndex)
{
    strcpy(page, name);
    index = buttonIndex;
}

// FUNC 0x002162D8 CursorMemory::get
int CursorMemory::get(const char *name)
{
    if (str_equal(page, name))
        return index;
    return 0;
}
