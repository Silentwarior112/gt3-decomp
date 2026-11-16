/* T286_level_colour.h - a level -> RGB colour ramp (translation unit T286,
 * unit u077), 0x001DE318..0x001DE3B8.
 *
 * C++ translation unit without classes: the function has C++ linkage (its
 * caller, RaceDisplay::virtual_2, u051, is C++); the header also compiles as
 * C. */
#ifndef GAME_T286_LEVEL_COLOUR_H
#define GAME_T286_LEVEL_COLOUR_H

#include "game/game.h"

/* 64..: red, green falling; 32..63: red, green falling faster; 0..31:
 * green, red rising; -63..-1: green falling, blue rising; below: black. */
void level_to_rgb(int v, u8 *r, u8 *g, u8 *b);   /* 0x001DE318 (? a meter's colour) */

#endif /* GAME_T286_LEVEL_COLOUR_H */
