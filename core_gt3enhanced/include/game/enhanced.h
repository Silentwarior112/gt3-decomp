/* enhanced.h - the switches of the enhanced engine (ref/ENHANCED.md,
 * ref/enhanced/PLAN.md rule 3).
 *
 * Every port that changes how GT3's own content behaves (a retune, a bug
 * fix, an AI change, a new HUD look) is guarded by one switch, one per
 * survey entry, named ENH_<AREA>_<WHAT>:
 *
 *     1  Lupo 2's behaviour (the default)
 *     0  GT3's behaviour
 *
 *     #if ENH_PHYSICS_PITCH_INERTIA
 *         spec->pitchInertia = ...;           (Lupo 2 (L2 0x001DAE0C))
 *     #endif
 *
 * Features that only act on data GT3 does not have (a gearbox type, the
 * Dualnote, the Pod, a lamp blink pattern) have no switch: with GT3's data
 * they do nothing.
 *
 * One file per area (include/game/enhanced/<area>.h), each owned by its
 * area's agent; the survey entry of each switch is in its comment
 * (ref/enhanced/survey/<area>.md).  The shared fields, events and signatures
 * the areas build on: ref/enhanced/INTERFACES.md. */
#ifndef GAME_ENHANCED_H
#define GAME_ENHANCED_H

#include "game/enhanced/physics.h"          /* car/physics, race/dynamics, T482 */
#include "game/enhanced/rendering.h"        /* car/model, effects, course */
#include "game/enhanced/hud.h"              /* race/display */
#include "game/enhanced/race.h"             /* race/core, modes, ai, pause, engine/input */
#include "game/enhanced/menus.h"            /* menu, app, common, save, data */
#include "game/enhanced/cameras_sound.h"    /* camera, sound */

#endif /* GAME_ENHANCED_H */
