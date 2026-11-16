/* T347 - the surface type of a ground contact (RunwayContact, T153). */
/* Retail T347's .rodata (0x0033C870, nothing references it: DATA_START):
 * "bad_alloc", "t9UnitArena1Ui64" (the race headers, as T296).  Its .data:
 * the off-course table g_002D53D8, then the frame info. */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/car/physics/T347_ground_surface.h"
#include "game/car/physics/suspension_internal.h"
// DATA_START 0x0033C870

/* By surface code (RunwayContact.attr): 1 for the codes off the course
 * (2, 3, 4 and 6), else 0 (suspension_internal.h). */
int g_002D53D8[16] = { 0, 0, 1, 1, 1, 0, 1 };  /* 0x002D53D8 */

// FUNC 0x001E8A48 ground_surface_type
/* The surface code (attr); with attr2 bit 1 the codes 0 / 7 become 12,
 * 1 -> 13, 9 -> 14. */
u8 ground_surface_type(GroundQuery *q)
{
    if (!(q->attr2 & ATTR2_REMAP))
        return q->attr;

    u8 code = q->attr;
    switch (code) {
    case 0:
    case 7:
        code = 12;
        break;
    case 1:
        code = 13;
        break;
    case 9:
        code = 14;
        break;
    }
    return code;
}

// FUNC 0x001E8AC0 ground_is_off_course
/* g_002D53D8 of the surface code: 1 for the off-course codes 2, 3, 4, 6. */
int ground_is_off_course(GroundQuery *q)
{
    return g_002D53D8[q->attr];
}
