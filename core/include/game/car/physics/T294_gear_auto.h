/* T294_gear_auto.h - the automatic gear ratios (translation unit T294, unit
 * u079), 0x001E28A8..0x001E2CC8.
 *
 * The "auto" gear setting of the setting screens (u035) and of the save
 * (u108): the ratios x 1000 of a level (the top speed in km/h, 150..400)
 * from the steps of 0x002D43C8 (5, 6 or 7 gears), and the adjustable range
 * of every gear (gear_ranges, 0x001E2B30, inside u079). */
#ifndef GAME_T294_GEAR_AUTO_H
#define GAME_T294_GEAR_AUTO_H

#include "game/game.h"

struct CarSpec;                             /* T482_car_equipments.h (pointers only) */

/* The ratios into ratios[0..gearCount]: the top gear reaches
 * CarSpec.autoGearRpm at `level', each lower gear is a step longer (the
 * steps of the gear count, interpolated by the level); the reverse is (3 x
 * 1st + 2nd) / 4.  clearAuto != 0 also clears CarSpec.autoGear. */
void gear_auto_ratios(struct CarSpec *spec, int level, s16 *ratios, int clearAuto); /* 0x001E28A8 */
/* The adjustable range lo[i]..hi[i] of every gear at auto level `level'
 * with the final gear `final' (put into the spec meanwhile). */
void gear_auto_ranges(struct CarSpec *spec, int level, s16 final, s16 *lo, s16 *hi); /* 0x001E2C58 */

#endif /* GAME_T294_GEAR_AUTO_H */
