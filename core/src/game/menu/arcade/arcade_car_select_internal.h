/* arcade_car_select_internal.h - unit u004 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001095A8..0x0010C9E0, second pass.
 *
 * Translation units (link order):
 *   T035  gt2k_menu::SelectBox, the colour chip list of the car select
 *   T023  "ar/select_car", the one-player arcade car select: member
 *         functions of gt2k_menu::ArcadeControl (declared with the class in
 *         include/game/menu/arcade/T010_arcade_control.h) and gt2k_menu::TransitiveEX,
 *         the car dimension panel
 *
 * Every class is defined in a canonical header; this header adds the outside
 * headers the sources need (one section per translation unit, below) and
 * what only this unit uses: the declarations of the operator scripts (T023
 * defines its own), the page's constants, the per-player accessors and
 * helpers of T023, and the type_info storage of the unit's own classes. */
#ifndef U004_H
#define U004_H

#include "types.h"
#include "cxxrt.h"                          /* type_info, __rtti_si */
#include <stddef.h>

/* Each source defines its translation unit's name before including this
 * header and gets only the declarations it uses: the classes a unit sees
 * decide which typeinfo names its .rodata holds (in the reverse order of
 * their definition), and retail T035's has none of T023's (TransitiveEX,
 * the garage lists): with them, SelectBox's name sits 0xF8 bytes off
 * (__tfQ29gt2k_menu9SelectBox's data reference). */
#ifdef U004_T035
/* own translation unit, first: SmartIndexT<int> is instantiated after it
 * (retail's names: SmartIndexT<int>, then SelectBox) */
#include "game/menu/arcade/T035_select_box.h"           /* gt2k_menu::SelectBox */

/* outside the unit */
#include "engine/pgl/G001_flare.h"                /* gfx_blend_func (PGL_BLEND_SEL), gfx_alloc_tail,
                                               pgl_draw_strip, pgl_color */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item */
#include "game/menu/framework/T218_color.h"                /* Color4f */
#include "game/common/T251_smart_index.h"          /* gt2k_menu::SmartIndexT<int> */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_clip_rect, menu_clip_full */
#endif /* U004_T035 */

#ifdef U004_T023
/* First, for the data layout: retail T023's .rodata (0x0030A780) opens with
 * "(%f, %f, %f) %f %f %f\n", the string of SimpleCameraBase::print (an
 * inline member: egcs writes its strings in every unit that reads it); then
 * the classes in the order retail defined them (egcs names their types in
 * the reverse order): Operator and the operator list (T216 -> T217), the
 * garage lists (T429 -> T036, T491), TransitiveEX, SmartIndexT<int> (T010).
 * Retail's names after TransitiveEX's vtable: SmartIndexT<int>,
 * TransitiveEX (0x0030B000, its typeinfo function's data reference), the
 * garage lists (0x0030B020..0x0030B108), OperatorList .. Operator
 * (0x0030B108..0x0030B1DE). */
#include "game/camera/T412_simple_camera_base.h"   /* (data layout only) */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive; T217 Operator, OperatorCode */
#include "game/save/T429_game_status.h"          /* getGameStatus, GameStatus; T427 MenuStatus;
                                               T036 GarageCar */

/* own translation unit */
#include "game/menu/arcade/T035_select_box.h"           /* gt2k_menu::SelectBox */
#include "game/menu/arcade/T023_arcade_car_select.h"    /* gt2k_menu::TransitiveEX, specPanel* */

/* outside the unit */
#include "engine/pgl/G001_flare.h"                /* gfx_blend_func (PGL_BLEND_SEL), gfx_alloc_tail,
                                               pgl_draw_strip, pgl_color, gfx_vsync,
                                               fmt_sprintf */
#include "game/app/T004_game_status_holder.h"   /* getGameStatusHolder (the guest garage) */
#include "game/menu/arcade/T010_arcade_control.h"       /* ArcadeControl, ARCADE_* modes; T247 ControlBase,
                                               T211 PageManager, T232 MenuCarSpec, T251 / T017
                                               indices */
#include "game/menu/arcade/T020_arcade_helpers.h"       /* arcade_fade_out, F_001051B0 */
#include "game/menu/framework/T024_mini_cursor.h"          /* gt2k_menu::MiniCursor */
#include "game/menu/arcade/T029_arcade_movies.h"        /* MovieParam g_00292930[8] */
#include "game/menu/framework/T032_circle_gauge.h"         /* gt2k_menu::CircleGauge */
#include "game/menu/gtmode/T036_garage.h"               /* GarageCar */
#include "game/menu/framework/T212_menu_car.h"             /* gt2k_menu::Car */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive */
#include "game/menu/framework/T217_operator.h"             /* gt2k_menu::OperatorCode (the scripts below) */
#include "game/menu/framework/T218_color.h"                /* Color4f */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/framework/T253_menu_bg_movie.h"        /* MenuBgMovie g_002CDC10 */
#include "game/effects/T254_blur2d.h"               /* gt2k_menu::Blur2D */
#include "game/menu/framework/T255_cross.h"                /* gt2k_menu::Cross */
#include "game/menu/framework/T258_menu_string.h"          /* gt2k_menu::String */
#include "game/menu/framework/T259_menu_drawable.h"        /* gt2k_menu::Drawable */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_clip_rect, menu_clip_full */
#include "game/car/model/T278_show_room_car.h"        /* ShowRoomCar (MenuCarSpec::car) */
#include "game/data/T435_product_config.h"       /* product_config, ProductConfig */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "game/data/T482_car_equipments.h"       /* CarEquipments, PART_FRONT_TIRE_DIRT */
#include "game/data/T483_car_information.h"      /* CarInformation (GarageCar's base) */
#include "engine/io/T507_async_loader.h"         /* async_loader_create / destroy */
#include "game/data/T518_unit_settings.h"        /* UnitSettings g_002F59D8 */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */

/* enum ArcadeClass (ARCADE_CLASS_*): game/menu/arcade/T010_arcade_control.h. */

/* The `player' argument: 2 is player 2, anything else player 1. */
enum { ARCADE_PLAYER_2 = 2 };

/* The menu sound effects: T233's MENU_SE_*. */

/* The states of runCarSelect (the option rows of the cross bar: scrollToRow
 * 0 the car, 1 transmission, 2 racing / dri, 3 opt1d). */
enum {
    CAR_SELECT_BACK         = 0,            /* leave, result 0 */
    CAR_SELECT_CAR          = 1,            /* selectCarAndColor */
    CAR_SELECT_TRANSMISSION = 2,            /* selectTransmission */
    CAR_SELECT_RACING       = 3,            /* selectRacing */
    CAR_SELECT_OPT_D        = 4,            /* nothing (F_0010C5A0 has no callers) */
    CAR_SELECT_DONE         = 5             /* leave, result 1 */
};

/* Camera modes of the Car view (MenuCarCamera::mode: the showroom record of
 * that mode). */
enum { CAR_CAMERA_ARCADE = 0, CAR_CAMERA_COMPULSORY = 3 };

/* ShowRoomCar::driveName(): the drive train as a multi-character constant. */
enum {
    DRIVE_FF  = 0x4646,                     /* 'FF' */
    DRIVE_FR  = 0x4652,                     /* 'FR' */
    DRIVE_4WD = 0x345744,                   /* '4WD' */
    DRIVE_MR  = 0x4D52,                     /* 'MR' */
    DRIVE_RR  = 0x5252                      /* 'RR' */
};

/* Drawable::anchorType of the option row buttons: T259's ANCHOR_Y, only y
 * follows the anchor (the cross bar). */

/* The operator scripts (arrays of 0x30-byte instructions, T217).  T023
 * defines its own (const, so .rodata: 0x0030A898..0x0030AF70, each one in
 * front of the function that starts it, between the functions' strings, as
 * the retail layout shows).  The fourth, g_0030B1E0, lies past T023's
 * typeinfo names (0x0030B1E0..0x0030B2A0, the end of T023's .rodata): a
 * const with internal linkage, which egcs writes at the end of the file;
 * T023 defines it static, so it has no declaration here. */
namespace gt2k_menu {

/* The instruction codes the scripts use: T217's handlers (Operator::op*,
 * their index in the table g_002C8850). */
enum {
    OPC_REMOVE      = 3,                    /* opRemove: the script ends, the object stays */
    OPC_RESTART     = 4,                    /* opRestart: from the first instruction */
    OPC_WAIT        = 5,                    /* opWait: f10 seconds */
    OPC_RELEASE     = 6,                    /* opRelease: the object is killed */
    OPC_SET_RECT    = 7,                    /* opSetRect: x, y, w, h = f10..f1C */
    OPC_MOVE        = 9,                    /* opMove: x / y += f10 / f14 per second for f18 s */
    OPC_GROW        = 13,                   /* opGrow: w / h += f10 / f14 per second for f18 s */
    OPC_SET_COLORS  = 18,                   /* opSetColors: r, g, b, a = f10..f1C */
    OPC_FADE_COLORS = 20,                   /* opFadeColors: += f10..f1C per second for f20 s */
    OPC_FADE_ALPHA  = 25,                   /* opFadeAlpha: alphaRatio += f10 per second for f14 s */
    OPC_SET_8C      = 28,                   /* opSet8C: Drawable f[0..3] = f10..f1C */
    OPC_ADD_8C      = 29,                   /* opAdd8C: f[0..3] += f10..f1C per second for f20 s */
    OPC_CALL        = 30                    /* opCall: arg (a function of the Transitive) */
};

/* One instruction: code, arg, the operands f10..f20, result (1 ends the
 * frame); arg2 and the unknown bytes are 0 in every T023 script.  An opCall
 * function goes in as (int)fn: the union's first member is the int, and
 * both compilers fold the address into a static initializer. */
#define U004_OP(code, arg, f10, f14, f18, f1C, f20, result) \
    { (code), (arg), 0, { 0, 0, 0, 0 }, (f10), (f14), (f18), (f1C), (f20), \
      { 0, 0, 0, 0, 0, 0, 0, 0 }, (result) }

/* A dimension text: colours (0, 0, 0, 0), alpha in at 1 / s for 0.5 s, 3 s,
 * out again, then kill() (opRelease). */
extern const OperatorCode g_0030A898[];     /* 0x0030A898 */
/* A dimension text: at (100, 250), 21 x 17, rising 40 / s for 8 s. */
extern const OperatorCode g_0030A988[];     /* 0x0030A988 */
/* The spec panel, forever: 1 s, specPanelShowLength, 1 s, ...Height, 1 s,
 * ...Width, 1 s, ...WheelBase, 0.5 s (opCall at 0x0030AA88 / AAE8 / AB48 /
 * ABA8). */
extern const OperatorCode g_0030AA58[];     /* 0x0030AA58 */
/* The Car view: colours (1, 1, 1, 0), alpha in at 1 / s for 1 s. */
extern const OperatorCode g_0030ACA0[];     /* 0x0030ACA0 */
/* The mini cars: alpha out at 2 / s for 0.5 s (hideMiniCars) / in
 * (showMiniCars). */
extern const OperatorCode g_0030AD90[];     /* 0x0030AD90 */
extern const OperatorCode g_0030ADF0[];     /* 0x0030ADF0 */
/* The blur: from size 0 at the origin, w / h grow by 4 / s for 0.5 s
 * (blurIn); shrink again, then size 0 at the origin (blurOut). */
extern const OperatorCode g_0030AE50[];     /* 0x0030AE50 */
extern const OperatorCode g_0030AEE0[];     /* 0x0030AEE0 */
} /* namespace gt2k_menu */

/* A GarageCar's CarInformation base (+0x10) without the null test of a
 * pointer conversion: the listing adds 0x10 to GameStatus::getCar's result
 * directly.  Through a reference variable: egcs tests a static_cast of the
 * dereferenced car for null (movn), a reference bound to it not. */
static inline CarInformation *garageCarInfo(GarageCar *car)
{
    CarInformation &info = *car;

    return &info;
}

namespace gt2k_menu {

/* Player `player''s copy of a two-player member of ArcadeControl: player
 * 2's own one, anything else player 1's (arcade_car_select_2p_internal.h's accessors, for the same
 * reason: retail selects the member's address with movn and loads through
 * it, as the inline call's return value gives; a plain ?: of the two
 * members in the function compiles to other code).  Not members, so that
 * T010's object stays as it is. */
#define U004_PLAYER_MEMBER(T, fn, m1, m2) \
    static inline T *fn(ArcadeControl *ac, int player) \
    { return (player != ARCADE_PLAYER_2) ? &ac->m1 : &ac->m2; }

U004_PLAYER_MEMBER(SmartIndexT<int>, classOf, class1, class2)
U004_PLAYER_MEMBER(SmartCarIndex, carOf, car1, car2)
U004_PLAYER_MEMBER(SmartIndexT<int>, colorOf, color1, color2)
U004_PLAYER_MEMBER(MenuCarSpec, specOf, car1Spec, car2Spec)

#undef U004_PLAYER_MEMBER

/* index->move(delta) through the vtable (u003's T022 helper; ? an inline of
 * the original): the inline's pointer argument is a fresh pseudo, which
 * loop.c hoists in front of the select loops (retail computes &racing1 /
 * &transmission1 / &opt1d / &car1 / &color1 after scrollToRow's argument,
 * into the jal's delay slot). */
static inline int moveIndex(SmartIndexT<int> *idx, int delta)
{
    return idx->move(delta);
}

} /* namespace gt2k_menu */
#endif /* U004_T023 */

/* The type_info storage of the unit's own classes (__si_type_info, 3 words;
 * filled by their typeinfo functions).  egcs generates both itself. */
#ifndef BOOT_OLD_GCC
extern "C" {
extern u32 __tiQ29gt2k_menu9SelectBox[3];   /* 0x004B14A0 */
extern u32 __tiQ29gt2k_menu12TransitiveEX[3]; /* 0x004B1490 */
}
#endif /* !BOOT_OLD_GCC */

#endif /* U004_H */
