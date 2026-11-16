/* T113 - the qm_* page actions and layout hooks of the short menu, and
 * gt2k_menu::ShortMenuText.  0x0012CB70..0x0012E2C8.
 *
 * T113_static_init fills the two tables ShortmenuControl::begin gives its
 * PageManager: g_0029C6B0, the 21 actions {name, function}
 * (ExecuteFuncData), and g_0029C758, the 45 hooks {name, create, p8, pC,
 * 0, 0} (MenuHook) of both layouts; both are sorted by name.  An action
 * stores its code (QM_*) in the session's ShortMenuArgs (g_003514F0): the
 * race mode acts on it when run_short_menu returns.  A hook builds the
 * object of a layout item: here a ShortMenuText whose text is formatted from
 * the ShortMenuArgs (or the game status); highlighted lines get the colour
 * SHORT_MENU_HIGHLIGHT.  Every hook first takes the item's texture
 * (PageLayout::getTexture), then calls operator new (Object's: the menu
 * heap) and the inline ShortMenuText constructor (Item(item, texture), then
 * the vptr); the landing pads only free the object (Object::operator
 * delete).
 *
 * Includes (data pass): retail's .rodata (0x00314CD0..0x003154E0) is
 * "bad_alloc", the literals, the vtable, then the type names ShortMenuText,
 * ShortmenuControl, ResultTermBar, ResultTitleBar, ReplayPauseMenu,
 * PauseMenu, RaceMenuSelectWindow, RaceMenuWindow, RaceMenuButton,
 * RaceMiniMap .. RaceDisplayBase, RaceCameraBase, UnitArena<64>,
 * SmartIndexT<int>, the OperatorHolder group, RegurationResult, the garage
 * lists, Operator (reverse definition order: the menu items and the page
 * manager before the race classes; no RecordsMenuArg).  Its .sdata opens
 * with T157's "" (the race displays).  "bad_alloc" and the "" are
 * unreferenced: the anchors; g_00351454, after the "", is this file's but
 * only T111's / T103's code reads it: its anchor gives it to T113. */
// DATA_START 0x00314CD0
// DATA_START 0x00351450
// DATA_START 0x00351454
#include <new>                              /* "bad_alloc" (retail's first .rodata item) */
#include "game/save/T429_game_status.h"          /* getGameStatus (T415 MachineTestRecords, T427 MenuStatus) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen, not used: RegurationResult) */
#include "game/menu/framework/T213_menu_item.h"            /* Item, MenuTexture (the OperatorHolder group) */
#include "game/menu/framework/T211_page_manager.h"         /* ExecuteFuncData (SmartIndexT<int>) */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/camera/T383_race_camera_base.h"     /* (seen, not used) */
#include "game/race/display/T154_race_display.h"         /* (seen, not used: the race displays) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (seen, not used) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* (seen, not used) */
#include "game/race/pause/T169_race_pause.h"           /* (seen, not used) */
#include "game/race/pause/T170_replay_pause.h"         /* (seen, not used) */
#include "game/menu/records/T200_result_board.h"         /* (seen, not used) */
#include "game/menu/race_menu/T111_shortmenu_control.h"    /* (seen, not used: ShortmenuControl) */
#include "game/menu/race_menu/T113_shortmenu_items.h"      /* ShortMenuText, the tables */
#include "game/data/messages/T114_msg_shortmenu.h"        /* g_002B1B30, the SHORTMENU texts */
#include "game/race/core/T124_race_information.h"     /* RaceInformation::getRaceDB */
#include "game/menu/framework/T220_object.h"               /* Object (operator new / delete: the menu heap) */
#include "game/menu/framework/T229_page_layout.h"          /* PageLayout::getTexture, MenuLayoutItem, MenuHook */
#include "game/menu/framework/T244_button_data.h"          /* ButtonData */
#include "game/menu/framework/T257_menu_image.h"           /* Image::texSet, fitToTexture */
#include "game/data/T484_race_database.h"        /* RaceDataBase, RaceEvent */
#include "game/data/T489_car_database.h"         /* db_car_name, race_name_split, race_kind_text */
#include "game/common/T494_text_utils.h"           /* format_time */
#include "engine/io/T503_vol_thread.h"           /* file_read_blocking, vol_device_object */
#include "game/data/T518_unit_settings.h"        /* UnitSettings g_002F59D8 */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "engine/pgl/G001_flare.h"                /* tex1_relocate, Tex1Image */
#include "libc.h"                           /* memalign */
#include "game/menu/race_menu/shortmenu_internal.h"                           /* g_003514F0, the qm_* functions */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* __rtti_si, type_info */
#endif

using namespace gt2k_menu;

/* ------------------------------------------------------------------------ */
/* Actions (ExecuteFuncData): the names are the table's                     */
/* ------------------------------------------------------------------------ */

// FUNC 0x0012CB70 qm_replay
void qm_replay(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_REPLAY;
}

// FUNC 0x0012CB80 qm_exit
void qm_exit(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_EXIT;
}

// FUNC 0x0012CB90 qm_try_again
void qm_try_again(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_TRY_AGAIN;
}

// FUNC 0x0012CBA0 qm_setting
void qm_setting(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_SETTING;
}

/* Also the action of "qm_start". */
// FUNC 0x0012CBB0 qm_go_race
void qm_go_race(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_GO_RACE;
}

// FUNC 0x0012CBC0 qm_free_run
void qm_free_run(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_FREE_RUN;
}

// FUNC 0x0012CBD0 qm_option
void qm_option(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_OPTION;
}

// FUNC 0x0012CBE0 qm_save_replay
void qm_save_replay(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_SAVE_REPLAY;
}

// FUNC 0x0012CBF0 qm_save_ghost
void qm_save_ghost(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_SAVE_GHOST;
}

// FUNC 0x0012CC00 qm_load_ghost
void qm_load_ghost(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_LOAD_GHOST;
}

// FUNC 0x0012CC10 qm_logger
void qm_logger(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_LOGGER;
}

// FUNC 0x0012CC20 qm_load_setting
void qm_load_setting(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_LOAD_SETTING;
}

// FUNC 0x0012CC30 qm_save_setting
void qm_save_setting(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_SAVE_SETTING;
}

// FUNC 0x0012CC40 qm_save_game
void qm_save_game(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_SAVE_GAME;
}

// FUNC 0x0012CC50 qm_next_session
void qm_next_session(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_NEXT_SESSION;
}

// FUNC 0x0012CC60 qm_records
void qm_records(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_RECORDS;
}

/* The answers of a yes / no page (result unchanged). */
// FUNC 0x0012CC70 qm_yes
void qm_yes(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->yes = 1;
}

// FUNC 0x0012CC80 qm_no
void qm_no(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->yes = 0;
}

// FUNC 0x0012CC90 qm_demo
void qm_demo(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
    g_003514F0->result = QM_DEMO;
}

// FUNC 0x0012CCA0 qm_dummy
void qm_dummy(void *ctx, ButtonData *rec, int index, PageLayout *layout)
{
}

/* The entry count of the action table (T111 / T103 read it).  T113 .sdata
 * right after the unit's first literal ("", an inline's) and before the
 * hooks' strings ("---"): defined here, after the actions. */
int g_00351454 = sizeof(g_0029C6B0) / sizeof(g_0029C6B0[0]);   /* 0x00351454 */

/* ------------------------------------------------------------------------ */
/* Hooks (MenuHookFn): the names are the table's                            */
/* ------------------------------------------------------------------------ */

/* "qm_best_time": ShortMenuArgs::bestTime as a time. */
// FUNC 0x0012CCA8 qm_best_time
Transitive *qm_best_time(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                         int p8, int pC, int p10, int p14)
{
    MenuTexture *texture = layout->getTexture(item);
    ShortMenuText *text = new ShortMenuText(item, texture);     /* Item(item, texture) */ /* implicit: Object::operator delete (if the constructor throws) */
    /* implicit: Item (inline ShortMenuText constructor) */
    format_time(text->buf, g_003514F0->bestTime);
    text->setText(text->buf);
    return text;
}

/* "qm_rank_speed1".."5" (p8 = 0..4): the speed of rank line p8 in the
 * user's speed unit, "%4.1f %s" (the line's speed is in m/s: convSpeed).
 * The converted speed is a float local, widened to double (__extendsfdf2)
 * as the argument, before the unit name is fetched (retail keeps the format
 * in $s0 across both calls). */
// FUNC 0x0012CD60 qm_rank_speed
Transitive *qm_rank_speed(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                          int p8, int pC, int p10, int p14)
{
    MenuTexture *texture = layout->getTexture(item);
    ShortMenuText *text = new ShortMenuText(item, texture);     /* Item(item, texture) */ /* implicit: Object::operator delete (if the constructor throws) */
    /* implicit: Item (inline ShortMenuText constructor) */
    ShortMenuRankEntry *line = &g_003514F0->rank[p8];
    float speed = g_002F59D8.convSpeed(line->speed);
    /* implicit: fptodp (float widened to double) */
    sprintf(text->buf, "%4.1f %s", speed, g_002F59D8.speedName());
    text->setText(text->buf);
    return text;
}

/* "qm_rank_time1".."5" (p8 = 0..4): the time of rank line p8. */
// FUNC 0x0012CE68 qm_rank_time
Transitive *qm_rank_time(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                         int p8, int pC, int p10, int p14)
{
    MenuTexture *texture = layout->getTexture(item);
    ShortMenuText *text = new ShortMenuText(item, texture);     /* Item(item, texture) */ /* implicit: Object::operator delete (if the constructor throws) */
    /* implicit: Item (inline ShortMenuText constructor) */
    ShortMenuRankEntry *line = &g_003514F0->rank[p8];
    format_time(text->buf, line->time);
    text->setText(text->buf);
    if (line->highlight != 0)
        text->textColor = SHORT_MENU_HIGHLIGHT;
    return text;
}

/* "qm_rank_name1".."5" (p8 = 0..4): the name of rank line p8,
 * "Polyphony Digital" when it has none. */
// FUNC 0x0012CF50 qm_rank_name
Transitive *qm_rank_name(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                         int p8, int pC, int p10, int p14)
{
    MenuTexture *texture = layout->getTexture(item);
    ShortMenuText *text = new ShortMenuText(item, texture);     /* Item(item, texture) */ /* implicit: Object::operator delete (if the constructor throws) */
    /* implicit: Item (inline ShortMenuText constructor) */
    ShortMenuRankEntry *line = &g_003514F0->rank[p8];
    if (line->name == NULL)
        text->setText("Polyphony Digital");
    else
        text->setText(line->name);
    if (line->highlight != 0)
        text->textColor = SHORT_MENU_HIGHLIGHT;
    return text;
}

/* "qm_list_car_name1".."6" (p8 = 0..5): the car name of result line p8
 * (formatted on the stack), "---" for an empty line (never highlighted).
 * An if / else with one return (an early return makes jump2 merge the two
 * returns, which retail does not). */
// FUNC 0x0012D040 qm_list_car_name
Transitive *qm_list_car_name(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                             int p8, int pC, int p10, int p14)
{
    char name[0x80];                        /* sp+0x00 */

    MenuTexture *texture = layout->getTexture(item);
    ShortMenuText *text = new ShortMenuText(item, texture);     /* Item(item, texture) */ /* implicit: Object::operator delete (if the constructor throws) */
    /* implicit: Item (inline ShortMenuText constructor) */
    ShortMenuResultEntry *line = &g_003514F0->list[p8];
    if (line->car == 0) {
        text->setText("---");
    } else {
        sprintf(name, "%s", db_car_name(line->car));
        text->setText(name);
        if (line->highlight != 0)
            text->textColor = SHORT_MENU_HIGHLIGHT;
    }
    return text;
}

/* "qm_mt_1000_time_rank01".."05" (p8 = 0, pC = 0..4): place pC of machine
 * test record table p8 (the table the names give: the 0-1000 m times). */
// FUNC 0x0012D158 qm_mt_1000_time_rank
Transitive *qm_mt_1000_time_rank(void *ctx, PageLayout *layout, Object *parent,
                                 MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    MenuTexture *texture = layout->getTexture(item);
    ShortMenuText *text = new ShortMenuText(item, texture);     /* Item(item, texture) */ /* implicit: Object::operator delete (if the constructor throws) */
    /* implicit: Item (inline ShortMenuText constructor) */
    format_time(text->buf, getGameStatus()->getMachineTestRecords()->getTime(p8, pC));
    text->setText(text->buf);
    return text;
}

/* "qm_list_time1".."6" (p8 = 0..5): the time of result line p8. */
// FUNC 0x0012D240 qm_list_time
Transitive *qm_list_time(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                         int p8, int pC, int p10, int p14)
{
    MenuTexture *texture = layout->getTexture(item);
    ShortMenuText *text = new ShortMenuText(item, texture);     /* Item(item, texture) */ /* implicit: Object::operator delete (if the constructor throws) */
    /* implicit: Item (inline ShortMenuText constructor) */
    ShortMenuResultEntry *line = &g_003514F0->list[p8];
    format_time(text->buf, line->time);
    text->setText(text->buf);
    if (line->highlight != 0)
        text->textColor = SHORT_MENU_HIGHLIGHT;
    return text;
}

/* "qm_pt1".."6" (p8 = 0..5): the points of result line p8, "%d pt" or (2
 * and more) "%d pts". */
// FUNC 0x0012D328 qm_pt
Transitive *qm_pt(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                  int p8, int pC, int p10, int p14)
{
    MenuTexture *texture = layout->getTexture(item);
    ShortMenuText *text = new ShortMenuText(item, texture);     /* Item(item, texture) */ /* implicit: Object::operator delete (if the constructor throws) */
    /* implicit: Item (inline ShortMenuText constructor) */
    ShortMenuResultEntry *line = &g_003514F0->list[p8];
    if (line->points >= 2)
        sprintf(text->buf, "%d %s", line->points, g_002B1B30[SHORTMENU_TEXT_POINT_PLURAL]);
    else
        sprintf(text->buf, "%d %s", line->points, g_002B1B30[SHORTMENU_TEXT_POINT_SINGLE]);
    text->setText(text->buf);
    if (line->highlight != 0)
        text->textColor = SHORT_MENU_HIGHLIGHT;
    return text;
}

/* "qm_laps": ShortMenuArgs::laps, "%d Lap" or (2 and more) "%d Laps". */
// FUNC 0x0012D450 qm_laps
Transitive *qm_laps(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                    int p8, int pC, int p10, int p14)
{
    MenuTexture *texture = layout->getTexture(item);
    ShortMenuText *text = new ShortMenuText(item, texture);     /* Item(item, texture) */ /* implicit: Object::operator delete (if the constructor throws) */
    /* implicit: Item (inline ShortMenuText constructor) */
    int laps = g_003514F0->laps;
    if (laps >= 2)
        sprintf(text->buf, "%d %s", laps, g_002B1B30[SHORTMENU_TEXT_LAPS_PLURAL]);
    else
        sprintf(text->buf, "%d %s", laps, g_002B1B30[SHORTMENU_TEXT_LAPS_SINGLE]);
    text->setText(text->buf);
    return text;
}

/* "qm_license_name" (p8 = 0): the licence and the number of the current
 * test (the menu status's event, race_name_split: e.g. "License B-" and 1),
 * "%s%d" with the number from 1 (the licence text fetched first: the format
 * is loaded after that call).  "qm_license_time_gold" / "_silver" /
 * "_bronz" (p8 = 1 / 2 / 3): the medal time of the test from its race
 * record (RaceEvent::time[p8 - 1]: min * 60000 + sec * 1000 + ms, written
 * out three times through db->event, no pointer to the time: retail
 * reloads db->event per case and uses the fields' offsets from it;
 * RaceDataBase's getGoldTime .. are not called).  The race DB
 * (RaceInformation slot 14) is fetched for every p8.  Another p8 would set
 * the text from the uninitialised buffer. */
// FUNC 0x0012D540 qm_license
Transitive *qm_license(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                       int p8, int pC, int p10, int p14)
{
    char label[0x80];                       /* sp+0x00 */
    int kind;                               /* sp+0x80 the race kind (RACE_KIND_LICENSE_B ..) */
    int test;                               /* sp+0x84 the test, from 0 */

    MenuTexture *texture = layout->getTexture(item);
    ShortMenuText *text = new ShortMenuText(item, texture);     /* Item(item, texture) */ /* implicit: Object::operator delete (if the constructor throws) */
    /* implicit: Item (inline ShortMenuText constructor) */
    RaceDataBase *db = g_003514F0->raceInfo->getRaceDB();
    switch (p8) {
    case 0: {
        race_name_split(getGameStatus()->getMenuStatus()->event, &kind, &test);
        const char *name = race_kind_text(kind);
        sprintf(label, "%s%d", name, test + 1);
        break;
    }
    case 1:
        format_time(label, db->event->time[LICENSE_MEDAL_GOLD].min * 60000 +
                           db->event->time[LICENSE_MEDAL_GOLD].sec * 1000 +
                           db->event->time[LICENSE_MEDAL_GOLD].ms);
        break;
    case 2:
        format_time(label, db->event->time[LICENSE_MEDAL_SILVER].min * 60000 +
                           db->event->time[LICENSE_MEDAL_SILVER].sec * 1000 +
                           db->event->time[LICENSE_MEDAL_SILVER].ms);
        break;
    case 3:
        format_time(label, db->event->time[LICENSE_MEDAL_BRONZE].min * 60000 +
                           db->event->time[LICENSE_MEDAL_BRONZE].sec * 1000 +
                           db->event->time[LICENSE_MEDAL_BRONZE].ms);
        break;
    }
    text->setText(label);
    return text;
}

/* "qm_session_num": session + 1 between the two SESSION texts (EUC-JP
 * "No." N "race"). */
// FUNC 0x0012D720 qm_session_num
Transitive *qm_session_num(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                           int p8, int pC, int p10, int p14)
{
    MenuTexture *texture = layout->getTexture(item);
    ShortMenuText *text = new ShortMenuText(item, texture);     /* Item(item, texture) */ /* implicit: Object::operator delete (if the constructor throws) */
    /* implicit: Item (inline ShortMenuText constructor) */
    sprintf(text->buf, "%s%d%s", g_002B1B30[SHORTMENU_TEXT_SESSION_BEFORE],
            g_003514F0->session + 1, g_002B1B30[SHORTMENU_TEXT_SESSION_AFTER]);
    text->setText(text->buf);
    return text;
}

/* In no table (no references): a test hook ("kjm") that shows the TEX1
 * picture "database/kjmtest.img" (0x4000 bytes, memalign'd) as a plain
 * Item (Item(item, NULL), operator new(0x100)): the picture relocated, the
 * Image's texSet, fitToTexture.  The buffer is not freed when the file
 * cannot be read (then NULL). */
// FUNC 0x0012D7F0 kjm_test_image
Transitive *kjm_test_image(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                           int p8, int pC, int p10, int p14)
{
    Tex1Image *picture = (Tex1Image *)memalign(0x40, 0x4000);
    if (file_read_blocking("database/kjmtest.img", picture, 0x4000, 1, &vol_device_object) == NULL)
        return NULL;
    tex1_relocate(picture);
    Item *image = new Item(item, (MenuTexture *)NULL); /* implicit: Object::operator delete (if the constructor throws) */
    image->texSet = picture;
    image->fitToTexture();
    return image;
}

/* An empty function (no references). */
// FUNC 0x0012D8B8 F_0012D8B8
void F_0012D8B8(void)
{
}

/* ------------------------------------------------------------------------ */
/* Static initialiser (.ctors entry 0x0028F0DC)                             */
/* ------------------------------------------------------------------------ */
/* The entry count of the hook table (T111 / T103 read it).  T113 .sdata
 * after the hooks' strings ("%s%d%s") and before the initialiser's
 * ("qm_demo"): defined here, after the hooks. */
int g_00351480 = sizeof(g_0029C758) / sizeof(g_0029C758[0]);   /* 0x00351480 */

/* The two tables (T113 .data, before the frame info; zeros in the image:
 * T113_static_init fills them).  No initialiser: egcs writes them as zeros
 * in .data at the end of the file's output, in the order of their first
 * declaration (T113_shortmenu_tables.h). */
ExecuteFuncData g_0029C6B0[21];             /* 0x0029C6B0 */
MenuHook        g_0029C758[45];             /* 0x0029C758 */

/* One table entry: the inline constructors of the original's
 * ExecuteFuncData / MenuHook (see T113_static_init); the fields in member
 * order. */
static inline void set_action(ExecuteFuncData *e, const char *name,
                              void (*func)(void *ctx, ButtonData *rec, int index,
                                           PageLayout *layout))
{
    e->name = name;
    e->func = func;
}

static inline void set_hook(MenuHook *e, const char *name, MenuHookFn *create,
                            int p8, int pC, int p10, int p14)
{
    e->name   = name;
    e->create = create;
    e->p8     = p8;
    e->pC     = pC;
    e->p10    = p10;
    e->p14    = p14;
}

/* One element as egcs's build_vec_init expands it (u006 / u012 / u013's
 * form): the constructor on a copy of the cursor, then the cursor and the
 * count of the elements left stepped. */
#define ACTION(name, func)                                                    \
    { ExecuteFuncData *e = actions; set_action(e, name, func); }              \
    actions++;                                                                \
    actionsLeft--
#define HOOK(name, create, p8, pC)                                            \
    { MenuHook *e = hooks; set_hook(e, name, create, p8, pC, 0, 0); }         \
    hooks++;                                                                  \
    hooksLeft--

/* Static initialiser of the two tables, both sorted by name (.ctors entry
 * 0x0028F0DC).  Compiler-generated in the original (_GLOBAL_$I$...): the
 * tables were defined in this unit with ExecuteFuncData / MenuHook objects
 * built by inline constructors, which egcs builds at run time (cp/init.c
 * build_vec_init: a walking cursor per table; the scheduler interleaves the
 * stores of both, as retail; .data stays zero).  The decomp defines the
 * tables without initialiser (zeros, above) and writes the function out the
 * way build_vec_init expands it (a cursor, a copy of it per
 * element, the dead count of the elements left): an exact form, not the
 * original; it stores the same 21 + 45 entries.  A hook entry is {name,
 * create, p8, pC, 0, 0}. */
// FUNC 0x0012D8C0 T113_static_init
void T113_static_init(void)
{
    ExecuteFuncData *actions = g_0029C6B0;
    int actionsLeft = 20;                   /* build_vec_init's iterator: elements - 1 (dead) */

    ACTION("qm_demo", qm_demo);
    ACTION("qm_dummy", qm_dummy);
    ACTION("qm_exit", qm_exit);
    ACTION("qm_free_run", qm_free_run);
    ACTION("qm_go_race", qm_go_race);
    ACTION("qm_load_ghost", qm_load_ghost);
    ACTION("qm_load_setting", qm_load_setting);
    ACTION("qm_logger", qm_logger);
    ACTION("qm_next_session", qm_next_session);
    ACTION("qm_no", qm_no);
    ACTION("qm_option", qm_option);
    ACTION("qm_records", qm_records);
    ACTION("qm_replay", qm_replay);
    ACTION("qm_save_game", qm_save_game);
    ACTION("qm_save_ghost", qm_save_ghost);
    ACTION("qm_save_replay", qm_save_replay);
    ACTION("qm_save_setting", qm_save_setting);
    ACTION("qm_setting", qm_setting);
    ACTION("qm_start", qm_go_race);
    ACTION("qm_try_again", qm_try_again);
    ACTION("qm_yes", qm_yes);

    MenuHook *hooks = g_0029C758;
    int hooksLeft = 44;                     /* the same (dead) */

    HOOK("qm_best_time", qm_best_time, 0, 0);
    HOOK("qm_laps", qm_laps, 0, 0);
    HOOK("qm_license_name", qm_license, 0, 0);
    HOOK("qm_license_time_bronz", qm_license, 3, 0);
    HOOK("qm_license_time_gold", qm_license, 1, 0);
    HOOK("qm_license_time_silver", qm_license, 2, 0);
    HOOK("qm_list_car_name1", qm_list_car_name, 0, 0);
    HOOK("qm_list_car_name2", qm_list_car_name, 1, 0);
    HOOK("qm_list_car_name3", qm_list_car_name, 2, 0);
    HOOK("qm_list_car_name4", qm_list_car_name, 3, 0);
    HOOK("qm_list_car_name5", qm_list_car_name, 4, 0);
    HOOK("qm_list_car_name6", qm_list_car_name, 5, 0);
    HOOK("qm_list_time1", qm_list_time, 0, 0);
    HOOK("qm_list_time2", qm_list_time, 1, 0);
    HOOK("qm_list_time3", qm_list_time, 2, 0);
    HOOK("qm_list_time4", qm_list_time, 3, 0);
    HOOK("qm_list_time5", qm_list_time, 4, 0);
    HOOK("qm_list_time6", qm_list_time, 5, 0);
    HOOK("qm_mt_1000_time_rank01", qm_mt_1000_time_rank, 0, 0);
    HOOK("qm_mt_1000_time_rank02", qm_mt_1000_time_rank, 0, 1);
    HOOK("qm_mt_1000_time_rank03", qm_mt_1000_time_rank, 0, 2);
    HOOK("qm_mt_1000_time_rank04", qm_mt_1000_time_rank, 0, 3);
    HOOK("qm_mt_1000_time_rank05", qm_mt_1000_time_rank, 0, 4);
    HOOK("qm_pt1", qm_pt, 0, 0);
    HOOK("qm_pt2", qm_pt, 1, 0);
    HOOK("qm_pt3", qm_pt, 2, 0);
    HOOK("qm_pt4", qm_pt, 3, 0);
    HOOK("qm_pt5", qm_pt, 4, 0);
    HOOK("qm_pt6", qm_pt, 5, 0);
    HOOK("qm_rank_name1", qm_rank_name, 0, 0);
    HOOK("qm_rank_name2", qm_rank_name, 1, 0);
    HOOK("qm_rank_name3", qm_rank_name, 2, 0);
    HOOK("qm_rank_name4", qm_rank_name, 3, 0);
    HOOK("qm_rank_name5", qm_rank_name, 4, 0);
    HOOK("qm_rank_speed1", qm_rank_speed, 0, 0);
    HOOK("qm_rank_speed2", qm_rank_speed, 1, 0);
    HOOK("qm_rank_speed3", qm_rank_speed, 2, 0);
    HOOK("qm_rank_speed4", qm_rank_speed, 3, 0);
    HOOK("qm_rank_speed5", qm_rank_speed, 4, 0);
    HOOK("qm_rank_time1", qm_rank_time, 0, 0);
    HOOK("qm_rank_time2", qm_rank_time, 1, 0);
    HOOK("qm_rank_time3", qm_rank_time, 2, 0);
    HOOK("qm_rank_time4", qm_rank_time, 3, 0);
    HOOK("qm_rank_time5", qm_rank_time, 4, 0);
    HOOK("qm_session_num", qm_session_num, 0, 0);
}

#undef ACTION
#undef HOOK

/* ------------------------------------------------------------------------ */
/* gt2k_menu::ShortMenuText                                                 */
/* ------------------------------------------------------------------------ */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (si, base Item). */
// FUNC 0x0012E230 __tfQ29gt2k_menu13ShortMenuText
extern "C" void *__tfQ29gt2k_menu13ShortMenuText(void)
{
    if (__tiQ29gt2k_menu13ShortMenuText[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu13ShortMenuText, "Q29gt2k_menu13ShortMenuText",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu13ShortMenuText;
}
#endif

// FUNC 0x0012E280 gt2k_menu::ShortMenuText::~ShortMenuText
/* implicit in game/menu/race_menu/T113_shortmenu_items.h; egcs emits this copy (slot
 * [0]): Item::~Item (flags 0), then Object::operator delete when bit 0 of
 * the flags is set (buf needs no destruction); no vptr store. */
/* implicit: ~Item, operator delete (base destructor, deleting destructor) */
