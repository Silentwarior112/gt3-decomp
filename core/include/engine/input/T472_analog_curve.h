/* T472_analog_curve.h - the analog stick curve g_003416F8 (data of the
 * data-only translation unit T472, between T471 and T473).
 *
 * Split from T087_logger_windows.h (which includes it where the declaration
 * was): the curve's readers outside the logger, T214's MenuControl (u065)
 * among them, did not see the logger windows in the original (retail's
 * T214 has only MenuControl's type name; T087_logger_windows.h brings the
 * operator list classes' names into every includer).  A plain declaration:
 * the old includers see the same.
 */
#ifndef GAME_T472_ANALOG_CURVE_H
#define GAME_T472_ANALOG_CURVE_H

#include "types.h"

/* The analog stick curve (0x80 centre; 0 for 0..15, ...): LoggerGraphRoot
 * (T087, u038), u065 (T214's analogRVertical), u095, u105.  Not T474's data (i2):
 * it lies after T471's typeinfo name (0x003416E0), in the data-only T472,
 * before T473's 0x003417F8; no T471..T474 code reads it.
 * Not const: RaceInput::readAnalogSteer (u095) loads it before its CarInput
 * stores, as a plain (aliasing) u8 load; u039 / u065 / u105 score the same
 * either way.  Defined (const, retail's .rodata) by src/engine/input/T472_analog_curve.cpp,
 * which does not include this header. */
extern u8 g_003416F8[256];                  /* 0x003416F8 */

#endif /* GAME_T472_ANALOG_CURVE_H */
