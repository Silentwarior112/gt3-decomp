/* T325_misc.h - g_002D28B0, the dynamics tuning bytes (translation unit T325,
 * no code: .data 0x002D28B0..0x002D296C, its .eh_frame; T325_dyn_params.cpp).
 *
 * DynParams, their layout, is declared in T283_condition_model.h (h17), with
 * each byte's users: the slipstream (T329), the car parameter builder (T284),
 * the engine's power limiter (T291), the tyre heat (T295), the drivetrain
 * (T296), the wear curve (T327), the surface tables (T328), ConditionModel
 * (T283), the assists (T340, T356).
 *
 * The other units' views: u077 DynConstTable; u079 / u081 index it as u8[]
 * (g_002D28B0[0xAD], g_002D28B0 + kind: the members oversteerAngleLo..,
 * slipFadeSpeed[kind] ..); u078 reads +0x08 as its own global g_002D28B8. */
#ifndef GAME_T325_MISC_H
#define GAME_T325_MISC_H

#include "game/game.h"
#include "game/car/physics/T283_condition_model.h"      /* DynParams (the global's type, by value) */

class DynParams;                            /* defined in T283_condition_model.h */

/* T325's .data runs 0xBC bytes to its .eh_frame (4-aligned at 0x002D296C):
 * DynParams is 0xBC bytes (0xB8 used, then 4 zero bytes no code reads). */
extern DynParams g_002D28B0;                /* 0x002D28B0 */

/* u078's g_002D28B8 (the power limiter's softness, %: CarEngine.limitSoftness
 * = it / 100) is a member: units drop their own extern declarations. */
#define g_002D28B8 (g_002D28B0.powerLimitSoftness)  /* 0x002D28B8 */

#endif /* GAME_T325_MISC_H */
