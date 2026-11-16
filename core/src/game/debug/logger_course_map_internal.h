/* logger_course_map_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u039,
 * 0x0016E640..0x00171CA8, second pass.
 *
 * Translation unit:
 *   T088  the logger's course map (LoggerCourseMap: relocate, draw, toMap)
 *
 * T087's second part (0x0016E640..0x00171968) was this unit's too; T087 is
 * one file now, src/game/debug/T087_logger_windows.cpp (its constants, helpers and
 * typeinfo storage moved to logger_windows_internal.h).
 *
 * Every class and function is declared in its canonical header
 * (include/game/debug/T088_logger_course_map.h and the headers of the classes it
 * uses); T088 includes those itself (the data pass: retail's T088 saw no
 * polymorphic class, its object has no .rodata).  This header adds the one
 * constant T088 uses.
 */
#ifndef U039_H
#define U039_H

#include "types.h"
#include "engine/pgl/G001_flare.h"                /* PGL_BLEND_SEL, PGL_SEL_* */

/* The blend mode of the course map (gfx_blend_func): colour = (Cs - Cd) *
 * As + Cd (0x44; the logger windows' LOGGER_BLEND_ALPHA, logger_windows_internal.h). */
#define LOGGER_BLEND_ALPHA   PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD)

#endif /* U039_H */
