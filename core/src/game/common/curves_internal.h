/* curves_internal.h - unit u075 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001D5D60..0x001D93C0, second pass.
 *
 * Translation units (link order):
 *   T264  gt2k_menu::RawWindow, the base window of the layout-driven menu
 *         windows (include/game/menu/framework/T264_raw_window.h)
 *   T265  the RawWindow class registry (T265_raw_window_register.h)
 *   T276  the dynamic environment map render (T276_envmap_render.h)
 *   T277  DynamicEnvmapCamera, the camera of that render
 *         (T277_dynamic_envmap_camera.h)
 *   T319  angle helpers (T319_angle_math.h)
 *   T320  the set-up of a Curve (T320_record_set.h, which defines Curve)
 *   T321..T323  Curve lookups (T321_curve_lookup.h .. T323_curve_pair.h)
 *   T354  ResponseCurve (T354_response_curve.h)
 *
 * Every class is defined in its canonical header (include/game/).  Each
 * source includes the canonical headers it needs itself, in the order that
 * gives the original's type names (data pass, ref/data/u075/notes.md); this
 * header only declares what the unit's own data is: the T264 clip
 * rectangles, the T265 list head and the typeinfo storage of
 * DynamicEnvmapCamera (GCC 14).  T276's embedded model is a file-local
 * table of T276.
 */
#ifndef U075_H
#define U075_H

#include "types.h"

/* ======================================================================== */
/* T264 / T265: the clip rectangles, the registry list                      */
/* ======================================================================== */

/* T264 .data (defined there): the whole virtual screen and the current clip
 * rectangle, each {x0, y0, x1, y1} (both 0, 0, 640, 448 in the file).
 * RawWindow::virtual_2 resets the clip rectangle to the screen,
 * clipToWindow narrows it. */
extern f32 g_002D1018[4];                   /* 0x002D1018 */
extern f32 g_002D1028[4];                   /* 0x002D1028 */

namespace gt2k_menu {

class RawWindowRegister;                    /* T265 */

/* T265 .sdata (defined there): the head of the registrar list
 * (RawWindowRegister's constructor pushes; findRegistered walks it). */
extern RawWindowRegister *g_003528F8;       /* 0x003528F8 gp-0x50F8 */

/* RAW_EVENT_* (the RawWindow events): game/menu/framework/T264_raw_window.h. */

} /* namespace gt2k_menu */

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions and nodes itself; this is for GCC 14
 * only.  RawWindow's node is declared in T264, Item's in T213, CameraBase's
 * in T410. */
extern "C" u32 __ti19DynamicEnvmapCamera[3];    /* 0x004B2820 its __si_type_info, filled by
                                                   __tf19DynamicEnvmapCamera */
#endif /* !BOOT_OLD_GCC */

#endif /* U075_H */
