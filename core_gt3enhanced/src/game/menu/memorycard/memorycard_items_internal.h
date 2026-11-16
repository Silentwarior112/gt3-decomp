/* memorycard_items_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u027,
 * 0x0014B610..0x0014E880, second pass.
 *
 * Translation unit T069: the items of the memory card screens
 * ("menu/memorycard.mbl", run by gt2k_menu::MemorycardControl, T068).  Its
 * static initialiser fills the 51-entry hook table g_002A4E20 that
 * MemorycardControl::begin hands to the page layouts:
 *   - the texts of the file information pages (create_callback_text of
 *     T239, p8 = a MemcardTextFn): size, dates, comment, title lines, copy
 *     protection of any file ("o_pt_*"); money / days / cars / licence /
 *     achievement of a game save ("g_pt_*"); time, laps, car, course, power
 *     of a replay ("r_pt_*"), of a best lap / ghost ("b_pt_*", plus the
 *     mass) and of a setting file ("s_pt_*"); the free space of both cards;
 *   - the factories of the five item classes owned here: SlotTitle,
 *     TitleView, ProgressBar, SingleIconView, IconView (defined in T069's
 *     source, as in the original: only T069 saw them).
 *
 * Every other class is defined in its canonical header (include/game/);
 * this header adds what only this unit uses: constants, T069's .data
 * tables, the unit's own functions that no other unit names, the
 * declaration of T239's page hook (no canonical header has it) and the
 * typeinfo storage of the five classes.  T069's source includes the
 * headers whose classes the original saw first, in the original's order
 * (their type names), then this header.
 */
#ifndef U027_H
#define U027_H

#include "types.h"

/* outside the unit (T069's own header last, below) */
#include "engine/pgl/G001_flare.h"                /* gfx_blend_func, PGL_BLEND_SEL, PGL_SEL_* */
#include "engine/media/G002_mpeg_frameq.h"          /* strlen (0x00269A68); libmc.h: sceMcTypePS2,
                                               sceMcFileAttrDupProhibit */
#include "runtime/newlib/G007_stdio.h"                /* strcpy, strcat, strncpy */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "game/menu/memorycard/T068_memorycard_control_class.h" /* gt2k_menu::MemorycardControl (getSelectedFile,
                                               getCurrentSlot, mode, slots, pages, iconPos,
                                               progressPrev / progress), MODE: the class
                                               without T075's screens (no T076 dialogs) */
#include "game/menu/memorycard/T071_memorycard_slots.h"     /* gt2k_menu::MemorycardSlot (getManager, setTopRow,
                                               getCount, getFile, renderFile, isFileSelectable) */
#include "game/menu/memorycard/T072_memorycard_icon_sprites.h"  /* gt2k_menu::MemorycardIconSprites (draw) */
#include "game/data/messages/T095_msg_memorycard.h"       /* g_002B1C68, the MEMORYCARDTEXT texts */
#include "game/menu/memorycard/T108_memorycard_file.h"      /* gt2k_menu::MemorycardFile and the GT3 save kinds,
                                               gt2k_game::GameThumbnail, MemorycardIconRenderInfo */
#include "game/menu/memorycard/T109_memorycard_file_manager.h"  /* gt2k_menu::MemorycardFileManagerBase (isConnect,
                                               setScreenSize, getXIconNum) */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item (setResource, setText), MenuTexture */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (the hooks' result) */
#include "game/menu/framework/T218_color.h"                /* Color4f */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator new / delete, attachTo) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout (getTexture), MenuLayoutItem,
                                               MenuHook / MenuHookFn */
#include "game/menu/framework/T231_text_box.h"             /* gt2k_menu::create_text_box ("text_box") */
#include "game/menu/framework/T239_callback_text.h"        /* gt2k_menu::CallbackTextFn */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase (create_callback_text's owner) */
#include "game/menu/framework/T260_motion_value.h"         /* MotionValue (set, approach) */
#include "game/menu/framework/T261_menu_draw_state.h"      /* menu_clip_row, menu_clip_full */
#include "game/data/T484_race_database.h"        /* RaceDataBase::getCourseName */
#include "game/data/T489_car_database.h"         /* db_car_name, db_car_name2 */
#include "game/common/T494_text_utils.h"           /* format_number, format_time, format_speed, format_date */
#include "engine/io/T501_memorycard_manager.h"   /* mc_slot_type, mc_slot_free, mc_slot_formatted */
#include "game/data/T518_unit_settings.h"        /* UnitSettings g_002F59D8 (convPower, powerName) */

/* own translation unit: the hook table (the five classes are defined in
 * T069's source, after every header: egcs writes T069's type names in the
 * reverse order of the class definitions, SlotTitle's first, at 0x0031CFE0,
 * as retail) */
#include "game/menu/memorycard/T069_memorycard_items.h"

#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si (GCC 14 only) */
#endif

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* The MEMORYCARDTEXT texts of g_002B1C68 (T095) this unit shows; the names
 * are their keys (g_002B1C28). */
enum {
    MCTEXT_NO_MEMORYCARD = 0,               /* "NO MEMORY CARD(PS2)" */
    MCTEXT_UNFORMATTED   = 1,               /* "Unformatted" */
    MCTEXT_READING       = 2,               /* "Reading..." */
    MCTEXT_BROKEN        = 3,               /* "Broken" */
    MCTEXT_CP_YES        = 4,               /* "yes" (copy protected) */
    MCTEXT_CP_NO         = 5,               /* "no" */
    MCTEXT_KB_FREE       = 6,               /* "KB Free" */
    MCTEXT_LAP           = 7,               /* "lap" */
    MCTEXT_LAPS          = 8                /* "laps" */
};

/* MemorycardFile::sizeKB while there is no size (T108). */
enum {
    MC_SIZE_READING = -1,                   /* not computed yet */
    MC_SIZE_BROKEN  = -2,                   /* the directory could not be listed */
    MC_SIZE_INVALID = -3                    /* no size (a dummy file) */
};

/* The card ids of T501 (port * 4 + slot) of the two card slots. */
enum { MC_ID_SLOT1 = 0, MC_ID_SLOT2 = 4 };

/* GameThumbnail::category of the top speed test ("MaxSpeed" ghosts,
 * T068): bestlap.speed holds the speed instead of a lap time. */
enum { MC_CATEGORY_MAX_SPEED = 0x11 };

/* The licence letters of g_002A4E08: thumbnail.game.licence < 6. */
enum { MC_LICENCE_COUNT = 6 };

/* MemorycardIconSprites (T072) built-in icons this unit draws. */
enum {
    MC_ICON_BLANK  = 12,                    /* "icon_blank": a place beyond the end of the list */
    MC_ICON_CURSOR = 13                     /* "icon_cursor": the cursor frame */
};

/* drawIcon's count for the cursor: never reached (the cursor's entry is
 * always drawn as a list entry). */
enum { ICON_COUNT_ANY = 0x40000000 };

/* SingleIconView::kind (the factory's p8): which "*_icon_face" it is. */
enum {
    ICON_FACE_OP      = 0,                  /* "op_icon_face": the operation menu's page */
    ICON_FACE_CONFIRM = 1,                  /* "confirm_icon_face" */
    ICON_FACE_WORKING = 2                   /* "working_icon_face" (a clone of the file) */
};

/* MemorycardControl::iconPos: where the selected file's icon is (T068). */
enum {
    ICONPOS_CURSOR_X  = 0,                  /* the list cursor's icon (IconView::drawIcon) */
    ICONPOS_CURSOR_Y  = 1,
    ICONPOS_OP_X      = 2,                  /* the "op" place (the operation menu's icon) */
    ICONPOS_OP_Y      = 3,
    ICONPOS_CONFIRM_X = 4,                  /* the "confirm" / "working" place */
    ICONPOS_CONFIRM_Y = 5
};

/* Drawable::anchorType: T259's ANCHOR_XY (x and y relative to the anchor). */

/* gfx_blend_func selectors (G001) around the cursor frame. */
enum {
    MC_BLEND_ADD   = PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_AS, PGL_SEL_CD),  /* 0x48 Cs * As + Cd */
    MC_BLEND_ALPHA = PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD)     /* 0x44 (Cs - Cd) * As + Cd */
};

/* ======================================================================== */
/* T069 .data                                                               */
/* ======================================================================== */
/* Both defined in T069_memorycard_items.cpp, where the original defined them
 * (their strings' places in .rodata / .sdata show it).
 * The licence letters: "B", "A", "IB", "IA", "S", "D" (thumbnail.game.
 * licence, the highest licence passed). */
extern const char *g_002A4E08[MC_LICENCE_COUNT];    /* 0x002A4E08 */
/* The title images of the modes 0x10..0x1A ("title_save_replay",
 * "title_load_replay", "title_save_ghost", "title_load_ghost" (twice),
 * "title_save_setting", "title_load_setting", "title_file_operation",
 * "title_save_game", "title_load_game"); MODE_DEMO_REPLAY_LOAD uses
 * MODE_REPLAY_LOAD's. */
extern const char *g_002A4DD8[11];          /* 0x002A4DD8 */

/* ======================================================================== */
/* This unit                                                                */
/* ======================================================================== */
/* p8 of create_callback_text: writes the text into buf (size 0x80; these
 * callbacks do not check it).  The owner create_callback_text passes is the
 * MemorycardControl (ControlBase at +0). */
typedef void MemcardTextFn(gt2k_menu::MemorycardControl *ctl, char *buf, int size);

/* The texts (MemcardTextFn; memcard_text_replay_course is in the canonical
 * header) */
MemcardTextFn memcard_text_size;            /* 0x0014B610 "*_pt_size" */
void memcard_format_date(char *buf, const gt2k_menu::McDateTime *date); /* 0x0014B6D8 */
MemcardTextFn memcard_text_date;            /* 0x0014B708 "*_pt_date", "o_pt_update" */
MemcardTextFn memcard_text_comment;         /* 0x0014B748 "*_pt_comment" */
MemcardTextFn memcard_text_mode;            /* 0x0014B7F8 "*_pt_mode" */
MemcardTextFn memcard_text_create_date;     /* 0x0014B8A8 "o_pt_create" */
MemcardTextFn memcard_text_copy_protect;    /* 0x0014B8E8 "o_pt_copy_protect" */
MemcardTextFn memcard_text_title1;          /* 0x0014B958 "o_pt_title1" */
MemcardTextFn memcard_text_title2;          /* 0x0014B9E0 "o_pt_title2" */
MemcardTextFn memcard_text_game_licence;    /* 0x0014BA80 "g_pt_license" */
MemcardTextFn memcard_text_game_days;       /* 0x0014BAE0 "g_pt_day" */
MemcardTextFn memcard_text_game_money;      /* 0x0014BB18 "g_pt_money" */
MemcardTextFn memcard_text_game_cars;       /* 0x0014BB50 "g_pt_cars" */
MemcardTextFn memcard_text_game_achievement; /* 0x0014BB88 "g_pt_g_achievement" */
MemcardTextFn memcard_text_replay_time;     /* 0x0014BBF8 "r_pt_time" */
MemcardTextFn memcard_text_replay_laps;     /* 0x0014BC30 "r_pt_laps" */
MemcardTextFn memcard_text_replay_car;      /* 0x0014BCA0 "r_pt_car" */
MemcardTextFn memcard_text_replay_power;    /* 0x0014BDA0 "r_pt_power" */
MemcardTextFn memcard_text_bestlap_time;    /* 0x0014BE20 "b_pt_time" */
MemcardTextFn memcard_text_bestlap_car;     /* 0x0014BE78 "b_pt_car" */
MemcardTextFn memcard_text_bestlap_course;  /* 0x0014BEE8 "b_pt_course" */
MemcardTextFn memcard_text_bestlap_power;   /* 0x0014BF78 "b_pt_power" */
MemcardTextFn memcard_text_bestlap_mass;    /* 0x0014BFF8 "b_pt_mass" */
MemcardTextFn memcard_text_setting_car;     /* 0x0014C040 "s_pt_car" */
MemcardTextFn memcard_text_setting_course;  /* 0x0014C0B0 "s_pt_course" */
MemcardTextFn memcard_text_setting_power;   /* 0x0014C140 "s_pt_power" */
void memcard_text_free_space(int id, gt2k_menu::MemorycardControl *ctl, char *buf,
                             int size);     /* 0x0014C1C0 */
MemcardTextFn memcard_text_free_slot1;      /* 0x0014C260 "pt_free_slot1" */
MemcardTextFn memcard_text_free_slot2;      /* 0x0014C290 "pt_free_slot2" */
MemcardTextFn memcard_text_message;         /* 0x0014C2C0 "pt_message" (empty) */
/* The selected file's thumbnail when it is a save of that kind with its
 * information read; else NULL, and buf is emptied. */
gt2k_game::GameThumbnail *memcard_game_thumbnail(gt2k_menu::MemorycardControl *ctl,
                                                       char *buf);    /* 0x0014C2C8 */
gt2k_game::GameThumbnail *memcard_replay_thumbnail(gt2k_menu::MemorycardControl *ctl,
                                                         char *buf);  /* 0x0014C368 */
gt2k_game::GameThumbnail *memcard_bestlap_thumbnail(gt2k_menu::MemorycardControl *ctl,
                                                          char *buf); /* 0x0014C408 */
gt2k_game::GameThumbnail *memcard_setting_thumbnail(gt2k_menu::MemorycardControl *ctl,
                                                          char *buf); /* 0x0014C4A8 */
/* The factories (MenuHookFn; ctx is the MemorycardControl) */
gt2k_menu::MenuHookFn memcard_create_slot_title;    /* 0x0014CE88 "title_bar_slot1/2" (p8: the slot) */
gt2k_menu::MenuHookFn memcard_create_title_view;    /* 0x0014CF90 "title_base" */
gt2k_menu::MenuHookFn memcard_create_progress_bar;  /* 0x0014D168 "progress_bar" */
gt2k_menu::MenuHookFn memcard_create_icon_face;     /* 0x0014D2F0 "*_icon_face" (p8: the kind) */
gt2k_menu::MenuHookFn memcard_create_icon_view;     /* 0x0014D638 "icon_view_slot1/2" (p8: the slot) */

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself; these are for GCC 14 only.
 * The type_info storage (__si_type_info: 3 words). */
extern "C" {
extern u32 __tiQ29gt2k_menu9TitleView[3];           /* 0x004B19C0 */
extern u32 __tiQ29gt2k_menu14SingleIconView[3];     /* 0x004B19D0 */
extern u32 __tiQ29gt2k_menu9SlotTitle[3];           /* 0x004B19E0 */
extern u32 __tiQ29gt2k_menu11ProgressBar[3];        /* 0x004B19F0 */
extern u32 __tiQ29gt2k_menu8IconView[3];            /* 0x004B1A00 */
}
#endif /* !BOOT_OLD_GCC */

#endif /* U027_H */
