/* gtmode_buttons_internal.h - unit u008 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x001168B8..0x00119778, second pass: GT-mode menu pieces.
 *
 * Translation units (link order), each with its canonical header:
 *   T047  gt2k_menu::TopButtonList / TopButton: the top menu's list, whose
 *         buttons fade with their distance from the list's centre line
 *         (include/game/menu/gtmode/T047_top_button_list.h)
 *   T040  gt2k_menu::OilChangeCylinder: the oil change animation (dirty oil
 *         drains, turns clean, refills) (T040_oil_change_cylinder.h)
 *   T048  the FocusFuncData table g_00295380: page -> the cursor it starts on
 *         (T048_page_cursor_hooks.h)
 *   T041  gt2k_menu::EventStatusObject (the row of frames and result marks
 *         of a series of events) and gt2k_menu::EventStatusButtonObject (the
 *         result mark and course picture over one event button)
 *         (T041_event_status.h)
 *   T049  the SelectableFuncData table g_002959E8: per-button availability
 *         checks of two pages (T049_page_button_checks.h)
 *   T042  gt2k_menu::EventHider: a cover drawn over the closed events of a
 *         list (T042_event_hider.h)
 *
 * Every class is defined in a canonical header.  Each source includes the
 * headers it uses itself, its own canonical header last, then this one (the
 * matching pass, ref/matching/u008/notes.md): egcs writes the type names of
 * the classes and template instances a unit sees into its .rodata in reverse
 * definition order, so the own classes' names come first, right after the
 * vtables, as in retail.  This header adds no class: the unit's constants,
 * and outside egcs the type_info storage of the unit's classes.
 */
#ifndef U008_H
#define U008_H

#include "types.h"
#include <stddef.h>

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* MenuStatus::raceMode (T427) of a championship: the mode go_race runs
 * (T037 action_set_race_code_champ; u050's MENU_RACE_MODE_SERIES, set by
 * RaceChampionshipInformation::resume).  4 is a single race, 3 a spot race. */
enum { RACE_MODE_CHAMPIONSHIP = 2 };

/* EventResults::getPlace (T425, BestPlace T418): 0 won .. 5, 8 never raced
 * (no mark).  The marks are "mark_<place + 1>" (EventStatusObject::marks). */
enum { EVENT_PLACE_NONE = 8 };

/* EventStatusObject: the event numbers are two digits ("<base>NN"), at most
 * 0x100 are tried; without arrows the frames are FRAME_GAP apart. */
enum { EVENT_NUMBER_LIMIT = 0x100 };
#define EVENT_FRAME_GAP 8.0f

/* OilChangeCylinder::state (T040). */
enum {
    OIL_IDLE         = 0,                   /* waits for start() */
    OIL_WAIT_DRAIN   = 1,                   /* 60 frames, then the sound and the drain */
    OIL_DRAINING     = 2,                   /* level -0.01 a frame down to 0 */
    OIL_WAIT_REFILL  = 3,                   /* 60 frames, then clean (dirt 0), the sound */
    OIL_REFILLING    = 4,                   /* level +0.01 a frame up to 1, then setFreeze(0) */
    OIL_DONE         = 5                    /* back to idle, started cleared */
};
enum { OIL_WAIT_FRAMES = 60 };
#define OIL_LEVEL_STEP 0.01f
/* The sound effect of the drain and of the refill: T233's MENU_SE_OIL. */

/* EventStatusButtonObject / EventHider: the grey of the vertex colours on
 * the focused button and on the others (Drawable::setColor, RGBA8 with r in
 * the low byte). */
#define MENU_GREY_FOCUSED   0x80808080u
#define MENU_GREY_UNFOCUSED 0x80404040u

/* ======================================================================== */
/* Type_info storage of the unit's classes (.bss), filled by their __tf     */
/* functions; the base lists of the two __class_type_infos (.sdata)        */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC                        /* egcs generates the __tf functions itself */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si, __rtti_class, __class_type_info::base_info */
extern "C" {
extern u32 __tiQ29gt2k_menu13TopButtonList[4];          /* 0x004B1530 __class_type_info */
extern u32 __tiQ29gt2k_menu9TopButton[4];               /* 0x004B1540 __class_type_info */
extern u32 __tiQ29gt2k_menu17OilChangeCylinder[3];      /* 0x004B1550 __si_type_info */
extern u32 __tiQ29gt2k_menu17EventStatusObject[3];      /* 0x004B1560 __si_type_info */
extern u32 __tiQ29gt2k_menu23EventStatusButtonObject[3];/* 0x004B1570 __si_type_info */
extern u32 __tiQ29gt2k_menu10EventHider[3];             /* 0x004B1580 __si_type_info */
}
/* {base, public, offset 0}: TopButtonList's base ButtonList
 * (__tiQ29gt2k_menu10ButtonList 0x004B2650), TopButton's base Button
 * (__tiQ29gt2k_menu6Button 0x004B2450). */
extern const __class_type_info::base_info g_00350FD0[1];   /* 0x00350FD0 */
extern const __class_type_info::base_info g_00350FD8[1];   /* 0x00350FD8 */
#endif /* !BOOT_OLD_GCC */

#endif /* U008_H */
