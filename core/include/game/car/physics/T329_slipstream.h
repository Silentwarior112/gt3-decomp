/* T329_slipstream.h - the slipstream: a blend factor per car that scales its
 * drag and lift (translation unit T329, unit u076), 0x001DB020..0x001DB220.
 *
 * The blend (DynCarState.slipBlend) rises while slipTarget is non-zero (at
 * slipTarget x g_0035292C per second) and falls otherwise (g_00352930); the
 * drag and lift forces of the frame are scaled from 1 down to the factors
 * g_00352924 / g_00352928 at full blend. */
#ifndef GAME_T329_SLIPSTREAM_H
#define GAME_T329_SLIPSTREAM_H

#include "game/game.h"

class RaceCarState;                         /* T302_car_state.h (pointers only) */

/* T329's .sdata, set by slipstream_init from g_002D28B0 (DynParams).  In the
 * comments: the initial value in the file, then the value slipstream_init sets
 * from the file's DynParams (12, 6, 12, 5, 20). */
extern f32 g_00352920;                      /* 0x00352920 airDensity / 10 (0.8; 1.2; no reader in this unit) */
extern f32 g_00352924;                      /* 0x00352924 slipDrag / airDensity: the drag factor in full slipstream
                                               (0.875; 0.5) */
extern f32 g_00352928;                      /* 0x00352928 slipLift / airDensity: the lift factor (0.875; 1.0) */
extern f32 g_0035292C;                      /* 0x0035292C 10 / slipRise, 1 if 0: the blend's rise rate per second
                                               x slipTarget (1.0; 2.0) */
extern f32 g_00352930;                      /* 0x00352930 10 / slipFall, 0.25 if 0: the blend's fall rate per
                                               second (0.25; 0.5) */

void slipstream_init(void);                      /* 0x001DB020 the factors (from surface_tables_init, T328) */
/* drag = -sign(v) x dragCoef x v^2 x lerp(1, g_00352924, blend), lift[i] =
 * liftCoef[i] x v^2 x lerp(1, g_00352928, blend), v = DynCarState.speedX;
 * to DynCarState +0x4B8 / +0x4BC. */
void car_slipstream_update(RaceCarState *car);         /* 0x001DB118 */

#endif /* GAME_T329_SLIPSTREAM_H */
