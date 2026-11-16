/* race_display_object_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u053,
 * 0x00196490..0x00196910: translation unit T156.
 *
 *   T156  RaceDisplayObjectBase: the base class of every element of the race
 *         display (meters, speed / gear / lap / rank displays, the mini map,
 *         ...), plus a few helpers of the race display (the 2D render
 *         state, the wide screen x shift of an alignment, alignment
 *         wrappers) and RaceSplitDisplayBase's setTargetDisplay.
 *
 * Every shared class and function this unit uses is declared in the
 * canonical headers below.  This header keeps only what is local to the
 * unit: RaceSplitDisplayBase (only its setter survives, no other unit has
 * the class) and named constants of the race display's alignment.
 *
 * Offsets in comments are the ORIGINAL ones (egcs / GCC 2.9x ee layout).
 */
#ifndef U053_H
#define U053_H

#include "types.h"
#include "cxxrt.h"                          /* type_info, __rtti_user */

#include "game/race/display/T156_race_display_object.h"  /* own: RaceDisplayObjectBase, the race_display_* helpers,
                                               __tf21RaceDisplayObjectBase / __ti21RaceDisplayObjectBase */
#include "engine/pgl/G001_flare.h"                /* pgl_color, gfx_enable / gfx_disable, gfx_blend_func,
                                               gfx_tex_param, pgl_ortho, PGL_* */
#include "engine/text/T496_font.h"                 /* Font::setAlign / setSize, the system font g_002F1968 */
#include "engine/io/T506_image_pack.h"           /* image_pack_find */
#include "game/race/display/T158_race_display_draw.h"    /* race_align_pos, race_align_posf */
#include "engine/gfx/T512_render_context.h"       /* g_002F424C (RenderContext::wide) */

/* The alignment values RACE_ALIGN_* / RACE_WIDE_SHIFT: T158_race_display_draw.h. */

/* ------------------------------------------------------------------------ */
/* RaceSplitDisplayBase                                                     */
/* ------------------------------------------------------------------------ */
/* The Lupo 2 name of the one function (setTargetDisplay__20RaceSplitDisplayBasei,
 * no callers).  ? layout: not polymorphic as far as the RTTI shows (no
 * vtable, no typeinfo); only the field +4 is known.  Not T155's
 * RaceSplitDisplay (whose target display is at +0x148FC and has its own
 * setter, 0x001963F0). */
class RaceSplitDisplayBase {
public:
    void setTargetDisplay(int display);     /* 0x00196750 */

    u8  pad_00[4];                          /* 0x00 ? */
    int targetDisplay;                      /* 0x04 the display the queries go to (? as RaceSplitDisplay::target) */
};

#endif /* U053_H */
