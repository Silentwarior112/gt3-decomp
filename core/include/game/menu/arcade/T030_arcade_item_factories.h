/* T030_arcade_item_factories.h - the layout object hooks of the arcade menu
 * (translation unit T030, unit u006), 0x0010FF90..0x00113540.
 *
 * g_00292A18 maps the object types of the arcade layouts ("mpeg",
 * "my_name", "countdown", "connect_status", "course_logo_NN", "mark_*_NN",
 * "prize_*_*", ..., sorted for the binary search) to their create
 * functions; ArcadeControl::setup (T010) gives it to both layouts of its
 * PageManager (PageLayout::setHooks, ctx the ArcadeControl).
 * PageLayout::createObject (T229) calls create(ctx, layout, parent, item,
 * p8, pC, p10, p14) with the entry's four words; NULL: no object.
 *
 * ItemFactoryEntry (u006, u021) is T229's MenuHook, and an ItemFactory is a
 * MenuHookFn; the first pass's LayoutLoader is PageLayout, LayoutNode is
 * MenuLayoutItem and arg2 is the parent. */
#ifndef GAME_T030_ARCADE_ITEM_FACTORIES_H
#define GAME_T030_ARCADE_ITEM_FACTORIES_H

#include "game/game.h"
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuHook (g_00292A18), MenuHookFn */

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class Transitive;                           /* T216 */
}

typedef gt2k_menu::MenuHook ItemFactoryEntry;  /* 0x18 bytes: name, create, p8, pC, p10, p14 */

/* "mpeg": a MpegImage (T249) of the item. */
gt2k_menu::Transitive *arcade_create_mpeg(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x0010FF90 */
/* "mpeg_course" / "mpeg_class": the same, kept in ArcadeControl::mpegCourse
 * / mpegClass. */
gt2k_menu::Transitive *arcade_create_mpeg_course(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00110008 */
gt2k_menu::Transitive *arcade_create_mpeg_class(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00110030 */
/* The selector of "my_name": the console's own i.LINK node (T473
 * F_0022D830; -1 when not linked: nothing shown). */
int arcade_my_name_index(void *ctx);        /* 0x00110058 */
/* "my_name": a CallbackItem (T221) of the hidden items "namea" .. "nameh". */
gt2k_menu::Transitive *arcade_create_my_name(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00110078 */
/* The selector of "countdown": the seconds left of g_00350FC8 (1..539
 * frames: 0..8), else 10 ("count_none"). */
int arcade_countdown_index(void *ctx);      /* 0x00110220 */
/* "countdown": a CallbackItem of "count_0" .. "count_9", "count_none". */
gt2k_menu::Transitive *arcade_create_countdown(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00110258 */
/* "connect_status": a ConnectStatus (T034). */
gt2k_menu::Transitive *arcade_create_connect_status(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00110470 */
/* "diagonal_in" / "diagonal_out": an Item running the diagonal wipe
 * g_0030D3C0 / g_0030D210 (T030 .rodata: in then out / out then in,
 * looping; the two items take turns). */
gt2k_menu::Transitive *arcade_create_diagonal_in(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x001104E8 */
gt2k_menu::Transitive *arcade_create_diagonal_out(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00110588 */
/* "end_button" (flashing, g_0030BFF8) / "end_message": only once the
 * ending is open (EventResults::beginnerLeagueWon / amateurLeagueWon /
 * professionalLeagueWon, T425, or GameStatus::arcadeLevel >= 6: every arcade
 * course level passed). */
gt2k_menu::Transitive *arcade_create_end_button(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00110628 */
gt2k_menu::Transitive *arcade_create_end_message(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00110730 */
/* "course_logo_NN" / "course_name_NN" (course = NN - 1 in the T435
 * "SINGLE_RACE" list): the picture "<course>_mini" / "<course>_doc", only
 * for an open course (ProductConfig::courseLevel <= GameStatus::arcadeLevel). */
gt2k_menu::Transitive *arcade_create_course_logo(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int course, int pC, int p10, int p14);  /* 0x00110820 */
gt2k_menu::Transitive *arcade_create_course_name(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int course, int pC, int p10, int p14);  /* 0x00110910 */
/* "mark_<difficulty>_NN" (difficulty 0 easy, 1 normal, 2 hard, 3 pro: no
 * case, never an item): "clear_easy" / ... when the course is cleared at
 * that difficulty or a harder one (T424 records "AECT0001" / "ANCT0001" /
 * "AHCT0001"). */
gt2k_menu::Transitive *arcade_create_clear_mark(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int course, int difficulty, int p10, int p14);  /* 0x00110A00 */
/* "prize_<difficulty>_<letter>" (the courses [first, end) of a group):
 * "prize_item_<difficulty>", flashing (g_0030C130) once every course of the
 * group is cleared, else dimmed (Drawable::setColor(0x20808080)). */
gt2k_menu::Transitive *arcade_create_prize_item(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int first, int end, int difficulty, int p14);   /* 0x00110BB8 */

/* T030 .data 0x00292A18..0x00294008, filled by T030_static_init (234
 * entries, sorted by name; "blink" is T205's create_blink, "flash_item"
 * T033's create_flash_item). */
extern gt2k_menu::MenuHook g_00292A18[234];    /* 0x00292A18 */
extern const int g_00350F48;                /* 0x00350F48 gp-0x6AA8 its entries (234; const: see T016_ending_menu.h) */
void T030_static_init(void);                /* 0x00111098 (.ctors 0x0028F044) */

#endif /* GAME_T030_ARCADE_ITEM_FACTORIES_H */
