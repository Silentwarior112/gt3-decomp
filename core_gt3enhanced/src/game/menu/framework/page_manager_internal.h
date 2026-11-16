/* page_manager_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u064,
 * 0x001B6250..0x001B9318, second pass.
 *
 * Translation units:
 *   T211  gt2k_menu::PageManager ("../../page_manager.cxx"): the page set
 *         of a menu screen (include/game/menu/framework/T211_page_manager.h).
 *   T212  gt2k_menu::Car: the 3D car of the menu screens
 *         (include/game/menu/framework/T212_menu_car.h).
 *
 * Every class is defined in its canonical header (include/game/); this header
 * adds the outside declarations the sources need and what only this unit
 * uses: the car buffers (T212 data, defined there), the Car typeinfo
 * storage, and named constants for the page / button flags.  (The fade
 * scripts are file-static tables of T211.)
 */
#ifndef U064_H
#define U064_H

#include "types.h"
#include "libc.h"                           /* strcpy (core 0x0027EB84) */
#include "cxxrt.h"                          /* type_info, __rtti_si, __rtti_user */
#include "eekernel.h"                       /* EI() */
#include "pdistd/allocator.h"               /* intr_suspend (core 0x00269B00), pdistd::Allocator */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync, gfx_enable, gfx_blend_func */
#include "game/common/T494_text_utils.h"           /* str_equal, zero_memory */

/* Each source defines its translation unit's name before including this
 * header and gets only the declarations it uses: the classes a unit sees
 * decide which typeinfo names (of template instances) its .rodata holds,
 * and retail T212's has none of T211's (List<Button>, SmartIndex<int>):
 * with them, Car's name sits 0x50 bytes off (__tfQ29gt2k_menu3Car). */
#ifdef U064_T211
/* outside the unit */
#include "game/menu/framework/T206_button.h"               /* gt2k_menu::Button (the decide effects, slots 15..17);
                                               (seen: T216's operator classes, Cell<Button>) */
/* ButtonList's header before T240_page_history.h (which reads
 * T251_smart_index.h): its List<Button> base, then its index member
 * instantiate SmartIndexT<int> (retail's names: SmartIndexT<int> 0x003338F8,
 * then List<Button> 0x00333918). */
#include "game/menu/framework/T244_button_list.h"          /* gt2k_menu::ButtonList, ButtonData */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (the layers) */
#include "game/menu/framework/T217_operator.h"             /* gt2k_menu::OperatorCode (the fade scripts) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object / RootObject */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout, MenuLayoutItem */
#include "game/menu/framework/T233_menu_sound.h"           /* MenuSound::selectMusic, menu_play_se */
#include "game/menu/framework/T240_page_history.h"         /* gt2k_menu::PageHistory */
#include "game/menu/framework/T241_link_manager.h"         /* gt2k_menu::LinkManager, LinkPage */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase (control1: the MenuControl) */
#include "game/menu/framework/T248_menu_signal.h"          /* menu_signal_clear / _poll (fadeOut), menu_signal_set
                                               (the fade-out script's opCall) */
#include "game/menu/framework/T252_timer.h"                /* gt2k_menu::Timer */
#include "game/menu/framework/T256_menu_rectangle.h"       /* gt2k_menu::Rectangle (the fade rectangle) */
#include "game/menu/framework/T259_menu_drawable.h"        /* gt2k_menu::Drawable */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_message, g_00352748 / 4C / 50 */
#include "game/save/T423_cursor_memory.h"        /* CursorMemory */

/* own translation unit, last: egcs emits the typeinfo names of the classes
 * (template instances) a unit sees in the reverse order of their
 * definition, and retail T211's .rodata has PageManager's first (0x003338D8,
 * right after its vtable), then the others. */
#include "game/menu/framework/T211_page_manager.h"
#endif /* U064_T211 */

#ifdef U064_T212
/* own translation unit */
#include "game/menu/framework/T212_menu_car.h"

/* outside the unit */
#include "game/menu/framework/T204_menu_heap.h"            /* menu_heap_current (the car buffers) */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (Car's base) */
#include "game/menu/gtmode/T232_menu_car_spec.h"        /* MenuCarSpec (getCode, the tyre sizes) */
#include "game/car/model/T245_car_render.h"           /* CarRender */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue */
#include "game/camera/T410_camera_base.h"          /* CameraBase */
#include "game/camera/T411_menu_car_camera.h"      /* MenuCarCamera */
#include "game/car/model/T446_car_model.h"            /* CarModel, car_model_path */
#include "game/car/model/T447_tire_model_base.h"      /* wheel_file_path / _relocate, WheelFile2 */
#include "game/car/model/T450_menu_tire_model.h"      /* MenuTireModel */
#include "game/car/model/T456_car_model_file.h"       /* CarModelFile, carmodelfile_relocate / _work_area */
#include "engine/input/T475_pad_manager.h"          /* g_002EB170 (the spare Control) */
#include "engine/io/T507_async_loader.h"         /* AsyncLoader, async_loader_get */
#include "engine/io/T508_loader_thread.h"        /* loader_defer */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer (CarRender's buffers) */
#endif /* U064_T212 */

#ifdef U064_T211
/* ======================================================================== */
/* T211 constants (only used here)                                          */
/* ======================================================================== */

/* LinkPage::flags (T241). */
enum {
    PAGE_CONTROL_TYPE  = 0x0001,            /* the list's controlType (LinkManager::getFlag0) */
    PAGE_LOOP          = 0x0002,            /* the list's loop (getFlag1) */
    PAGE_FADE_IN       = 0x0004,            /* startPage: a fade-in rectangle (g_00333A40) */
    PAGE_FADE_OUT      = 0x0008,            /* closePage: a fade-out rectangle (g_00333AD0), waited
                                               for */
    PAGE_CLEAR_MASK    = 0x0030,            /* the per-frame screen clear (g_00352750, colour
                                               g_0035274C, T261): */
    PAGE_CLEAR_NONE    = 0x0000,            /*   off */
    PAGE_CLEAR_NOW     = 0x0010,            /*   on from the start */
    PAGE_CLEAR_LATER   = 0x0020,            /*   on after 30 frames (the fade Timer) */
    PAGE_EFFECT_MASK   = 0x00C0,            /* playDecideEffect: 0 the sound only, 0x40 / 0x80 / 0xC0
                                               Button::virtual_15 / 16 / 17 */
    PAGE_CANCEL_SHIFT  = 8                  /* bit 8: the cancel button goes back (update) */
};

/* ButtonData::flags & BUTTON_ACTION_MASK: T244's BUTTON_JUMP .. BUTTON_SUB_LIST. */

/* MenuControl::lastKey (T214): the button of the last decide / cancel. */
enum { MENU_KEY_DECIDE = 0x40, MENU_KEY_CANCEL = 0x80 };

/* The fade scripts g_00333A40 (in) / g_00333AD0 (out) are file-static
 * tables of T211 (internal linkage: written at the end of its .rodata, after
 * the type names), not declared here. */
#endif /* U064_T211 */

#ifdef U064_T212
/* ======================================================================== */
/* T212 data (only used here)                                               */
/* ======================================================================== */

/* The model buffers of the menu cars, one per player (0xA8000 bytes on the
 * menu heap, made by the first Car of the player, freed by a deferred call
 * when its last one goes), their reference counts, and the "loading" flags
 * (one Car loads at a time).  T212 .sdata, defined there (zeros: these
 * externs are their first declarations, which give egcs their order and
 * the $gp addressing). */
extern void *g_00352550[2];                 /* 0x00352550 gp-0x54A0 */
extern int   g_00352558[2];                 /* 0x00352558 gp-0x5498 */
extern int   g_00352560[2];                 /* 0x00352560 gp-0x5490 */

/* car_model_path's kind (T446: 0 day, 1 night, 2 menu, 3 eve). */
enum { CAR_MODEL_MENU = 2 };                /* "menu/cars/<name>" */

#define MENU_CAR_BUFFER_SIZE  0xA8000       /* the largest model file (car_model_load) */
#define MENU_TIRE_SEGMENTS    30            /* MenuTireModel::build: one TirePacket of 30 segments */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" u32 __tiQ29gt2k_menu3Car[3];     /* 0x004B24D0 its __si_type_info (base Transitive) */
#endif
#endif /* U064_T212 */

#endif /* U064_H */
