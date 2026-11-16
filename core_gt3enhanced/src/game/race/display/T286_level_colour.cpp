/* T286 (0x001DE318..0x001DE3B8): a level -> RGB colour ramp (? a meter's
 * colour; called from RaceDisplay::virtual_2, T154). */
#include "game/race/display/T286_level_colour.h"
#include "game/car/physics/car_param_internal.h"

// FUNC 0x001DE318 level_to_rgb
/* 64..: red, green falling; 32..63: red, green falling faster; 0..31: green,
 * red rising; -63..-1: green falling, blue rising; below: black. */
void level_to_rgb(int v, u8 *r, u8 *g, u8 *b)
{
    if (v >= 0x40) {
        *r = 0xFC;
        *g = (0x7F - v) * 2;
        *b = 0;
        return;
    }
    if (v >= 0x20) {
        *r = 0xFC;
        *g = (0x5F - v) * 4;
        *b = 0;
        return;
    }
    if (v >= 0) {
        *r = v * 8;
        *g = 0xFC;
        *b = 0;
        return;
    }
    if (v >= -0x3F) {
        int blue = -v * 4;

        *r = 0;
        *g = 0xFC - blue;       /* computed as -4 - blue: the same byte */
        *b = blue;
        return;
    }
    *b = 0;
    *g = 0;
    *r = 0;
}
