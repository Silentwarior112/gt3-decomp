/* T030 - the layout object hooks of the arcade-mode menu.  0x0010FF90..0x00113540.
 *
 * Each hook makes the menu object of one object item of an arcade layout
 * (arcade_item_factories_internal.h: how PageLayout::createObject calls them; ctx is the
 * ArcadeControl).  T030_static_init fills the hook table g_00292A18, sorted
 * by type name, that ArcadeControl::setup hands to its PageLayouts.
 *
 * Every `new` below is Object::operator new (T220: the menu heap, size in the
 * comments); its landing pad in the binary only frees the block with
 * Object::operator delete if the constructor throws, then rethrows.
 *
 * The includes are in the order that gives retail's type names (.rodata
 * 0x0030D030, reverse definition order): SmartIndexT<int>, OperatorList,
 * List / OperatorHolder / Cell<OperatorHolder>, the garage lists (wheel list
 * .. Cell<GarageCar>), Operator.  So Operator (T217) was defined first, then
 * the GameStatus (T429), then the menu classes (T216's OperatorHolder /
 * OperatorList), then ArcadeControl (T010's SmartIndexT<int>). */
#include "game/menu/framework/T217_operator.h"             /* Operator, OperatorCode (the wipes) */
#include "game/save/T429_game_status.h"          /* getGameStatus: arcadeLevel, events, arcade */
#include "game/save/T424_arcade_records.h"       /* ArcadeRecords::getPlace */
#include "game/save/T425_event_results.h"        /* EventResults::*LeagueWon */
#include "game/menu/arcade/T030_arcade_item_factories.h"   /* the hooks, g_00292A18, g_00350F48; T229 MenuHook */
#include "game/menu/framework/T229_page_layout.h"          /* PageLayout (findHiddenItem, getTexture), MenuLayoutItem */
#include "game/menu/framework/T220_object.h"               /* Object (operator new / delete: the menu heap) */
#include "game/menu/framework/T216_menu_transitive.h"      /* Transitive::start; OperatorHolder, OperatorList */
#include "game/menu/framework/T213_menu_item.h"            /* Item */
#include "game/menu/framework/T249_mpeg_image.h"           /* MpegImage ("mpeg") */
#include "game/menu/arcade/T034_connect_status.h"       /* ConnectStatus ("connect_status") */
#include "game/menu/framework/T221_callback_item.h"        /* CallbackItem ("my_name", "countdown") */
#include "game/menu/arcade/T010_arcade_control.h"       /* ArcadeControl (the hooks' ctx), g_00350FC8; SmartIndexT<int> */
#include "game/menu/framework/T205_blink.h"                /* create_blink ("blink") */
#include "game/menu/framework/T033_flash_item.h"           /* create_flash_item ("flash_item") */
#include "game/data/T435_product_config.h"       /* product_config: the "SINGLE_RACE" course list */
#include "game/data/T481_car_data_base.h"        /* nameToCode */
#include "engine/system/T473_link_session.h"         /* F_0022D830: own i.LINK slot */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "game/menu/arcade/arcade_item_factories_internal.h"                           /* the unit's constants */

using gt2k_menu::ArcadeControl;
using gt2k_menu::CallbackItem;
using gt2k_menu::ConnectStatus;
using gt2k_menu::Item;
using gt2k_menu::MenuHook;
using gt2k_menu::MenuHookFn;
using gt2k_menu::MenuLayoutItem;
using gt2k_menu::MenuTexture;
using gt2k_menu::MpegImage;
using gt2k_menu::Object;
using gt2k_menu::OperatorCode;
using gt2k_menu::PageLayout;
using gt2k_menu::Transitive;

/* "mpeg": a MpegImage of the item (Object::operator new(0x120); operator
 * delete on the landing pad). */
// FUNC 0x0010FF90 arcade_create_mpeg
Transitive *arcade_create_mpeg(void *ctx, PageLayout *layout, Object *parent, MenuLayoutItem *item,
                               int p8, int pC, int p10, int p14)
{
    return new MpegImage(item, layout); /* implicit: Object::operator delete (if the constructor throws) */
}

/* "mpeg_course": the same, kept as the ArcadeControl's mpegCourse (the
 * course movie of "ar/select_course"; the inline setMpegCourse, T010). */
// FUNC 0x00110008 arcade_create_mpeg_course
Transitive *arcade_create_mpeg_course(void *ctx, PageLayout *layout, Object *parent,
                                      MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    ArcadeControl *control = static_cast<ArcadeControl *>(ctx);
    MpegImage *movie =
        static_cast<MpegImage *>(arcade_create_mpeg(ctx, layout, parent, item, p8, pC, p10, p14));
    control->setMpegCourse(movie);
    return movie;
}

/* "mpeg_class": the same, kept as mpegClass (the class still of
 * "ar/select_class"; the inline setMpegClass). */
// FUNC 0x00110030 arcade_create_mpeg_class
Transitive *arcade_create_mpeg_class(void *ctx, PageLayout *layout, Object *parent,
                                     MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    ArcadeControl *control = static_cast<ArcadeControl *>(ctx);
    MpegImage *movie =
        static_cast<MpegImage *>(arcade_create_mpeg(ctx, layout, parent, item, p8, pC, p10, p14));
    control->setMpegClass(movie);
    return movie;
}

/* The CallbackItem selector of "my_name": this console's own i.LINK slot
 * picks one of "namea" .. "nameh" (-1 when not linked: none is drawn). */
// FUNC 0x00110058 arcade_my_name_index
int arcade_my_name_index(void *ctx)
{
    return F_0022D830();
}

/* "my_name": a CallbackItem of the hidden items "namea" .. "nameh"
 * (Object::operator new(0xD8); operator delete on the landing pad). */
// FUNC 0x00110078 arcade_create_my_name
Transitive *arcade_create_my_name(void *ctx, PageLayout *layout, Object *parent,
                                  MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    CallbackItem *names = new CallbackItem(8, arcade_my_name_index, ctx); /* implicit: Object::operator delete (if the constructor throws) */
    names->setItem(layout->findHiddenItem("namea"), layout, 0);
    names->setItem(layout->findHiddenItem("nameb"), layout, 1);
    names->setItem(layout->findHiddenItem("namec"), layout, 2);
    names->setItem(layout->findHiddenItem("named"), layout, 3);
    names->setItem(layout->findHiddenItem("namee"), layout, 4);
    names->setItem(layout->findHiddenItem("namef"), layout, 5);
    names->setItem(layout->findHiddenItem("nameg"), layout, 6);
    names->setItem(layout->findHiddenItem("nameh"), layout, 7);
    return names;
}

/* The selector of "countdown": the seconds left of the "ar/connect"
 * countdown g_00350FC8 (1..539 frames: count_0 .. count_8), else
 * count_none.  Quirk: the countdown starts at 540 frames and 540 / 60 would
 * be 9, but 540 is outside the tested range, so count_9 is never shown. */
// FUNC 0x00110220 arcade_countdown_index
int arcade_countdown_index(void *ctx)
{
    int frames = g_00350FC8;
    if (frames > 0 && frames < 540)         /* (u32)(frames - 1) < 539 */
        return frames / FRAMES_PER_SECOND;
    return COUNTDOWN_NONE;
}

/* "countdown": a CallbackItem of "count_0" .. "count_9" and "count_none"
 * (Object::operator new(0xD8); operator delete on the landing pad). */
// FUNC 0x00110258 arcade_create_countdown
Transitive *arcade_create_countdown(void *ctx, PageLayout *layout, Object *parent,
                                    MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    CallbackItem *digits = new CallbackItem(11, arcade_countdown_index, ctx); /* implicit: Object::operator delete (if the constructor throws) */
    digits->setItem(layout->findHiddenItem("count_0"), layout, 0);
    digits->setItem(layout->findHiddenItem("count_1"), layout, 1);
    digits->setItem(layout->findHiddenItem("count_2"), layout, 2);
    digits->setItem(layout->findHiddenItem("count_3"), layout, 3);
    digits->setItem(layout->findHiddenItem("count_4"), layout, 4);
    digits->setItem(layout->findHiddenItem("count_5"), layout, 5);
    digits->setItem(layout->findHiddenItem("count_6"), layout, 6);
    digits->setItem(layout->findHiddenItem("count_7"), layout, 7);
    digits->setItem(layout->findHiddenItem("count_8"), layout, 8);
    digits->setItem(layout->findHiddenItem("count_9"), layout, 9);
    digits->setItem(layout->findHiddenItem("count_none"), layout, COUNTDOWN_NONE);
    return digits;
}

/* "connect_status": the i.LINK connection panel (Object::operator
 * new(0x128); operator delete on the landing pad). */
// FUNC 0x00110470 arcade_create_connect_status
Transitive *arcade_create_connect_status(void *ctx, PageLayout *layout, Object *parent,
                                         MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    return new ConnectStatus(item, layout); /* implicit: Object::operator delete (if the constructor throws) */
}

/* The two diagonal wipes (T217 operator scripts, codes: 18 opSetColors, 5
 * opWait, 22 opFadeColor2 (corners arg / arg2, rate f1C for f20 s), 4
 * opRestart).  File-scope statics: egcs defers a static const with an
 * initialiser and writes it at the end of the file, after the type names,
 * where retail has them (.rodata 0x0030D210, 0x0030D3C0), in definition
 * order. */
/* diagonal_out: grey 0.5, alpha 0.5; wait 2 s; alpha -1 / s for 0.5 s on
 * corners 0 / 2, then on 1 / 3; wait 4 s; the same +1 / s; wait 2 s;
 * restart (out and back in). */
static const OperatorCode g_0030D210[9] = {     /* 0x0030D210 */
    { 18, { 0 }, 0, { 0 }, 0.5f, 0.5f, 0.5f,  0.5f, 0.0f, { 0 }, 0 },
    {  5, { 0 }, 0, { 0 }, 2.0f, 0.0f, 0.0f,  0.0f, 0.0f, { 0 }, 1 },
    { 22, { 0 }, 2, { 0 }, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, { 0 }, 1 },
    { 22, { 1 }, 3, { 0 }, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, { 0 }, 1 },
    {  5, { 0 }, 0, { 0 }, 4.0f, 0.0f, 0.0f,  0.0f, 0.0f, { 0 }, 1 },
    { 22, { 0 }, 2, { 0 }, 0.0f, 0.0f, 0.0f,  1.0f, 0.5f, { 0 }, 1 },
    { 22, { 1 }, 3, { 0 }, 0.0f, 0.0f, 0.0f,  1.0f, 0.5f, { 0 }, 1 },
    {  5, { 0 }, 0, { 0 }, 2.0f, 0.0f, 0.0f,  0.0f, 0.0f, { 0 }, 1 },
    {  4, { 0 }, 0, { 0 }, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, { 0 }, 0 },
};
/* diagonal_in: the same from alpha 0 (in, then out: the two items take
 * turns). */
static const OperatorCode g_0030D3C0[9] = {     /* 0x0030D3C0 */
    { 18, { 0 }, 0, { 0 }, 0.5f, 0.5f, 0.5f,  0.0f, 0.0f, { 0 }, 0 },
    {  5, { 0 }, 0, { 0 }, 2.0f, 0.0f, 0.0f,  0.0f, 0.0f, { 0 }, 1 },
    { 22, { 0 }, 2, { 0 }, 0.0f, 0.0f, 0.0f,  1.0f, 0.5f, { 0 }, 1 },
    { 22, { 1 }, 3, { 0 }, 0.0f, 0.0f, 0.0f,  1.0f, 0.5f, { 0 }, 1 },
    {  5, { 0 }, 0, { 0 }, 4.0f, 0.0f, 0.0f,  0.0f, 0.0f, { 0 }, 1 },
    { 22, { 0 }, 2, { 0 }, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, { 0 }, 1 },
    { 22, { 1 }, 3, { 0 }, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, { 0 }, 1 },
    {  5, { 0 }, 0, { 0 }, 2.0f, 0.0f, 0.0f,  0.0f, 0.0f, { 0 }, 1 },
    {  4, { 0 }, 0, { 0 }, 0.0f, 0.0f, 0.0f,  0.0f, 0.0f, { 0 }, 0 },
};

/* "diagonal_in": an Item of the item running the diagonal wipe g_0030D3C0
 * (Object::operator new(0x100); operator delete on the landing pad).  The
 * texture is looked up before the allocation. */
// FUNC 0x001104E8 arcade_create_diagonal_in
Transitive *arcade_create_diagonal_in(void *ctx, PageLayout *layout, Object *parent,
                                      MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    MenuTexture *texture = layout->getTexture(item);
    Item *wipe = new Item(item, texture); /* implicit: Object::operator delete (if the constructor throws) */
    wipe->start(g_0030D3C0);
    return wipe;
}

/* "diagonal_out": the same with the opposite wipe g_0030D210. */
// FUNC 0x00110588 arcade_create_diagonal_out
Transitive *arcade_create_diagonal_out(void *ctx, PageLayout *layout, Object *parent,
                                       MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    MenuTexture *texture = layout->getTexture(item);
    Item *wipe = new Item(item, texture);   /* Object::operator new(0x100), operator delete */ /* implicit: Object::operator delete (if the constructor throws) */
    wipe->start(g_0030D210);
    return wipe;
}

/* "end_button": once the arcade menu offers the ending (one of the three GT
 * mode leagues won, T425, or every arcade course level passed), an Item of
 * the item flashing (g_0030BFF8; Object::operator new(0x100), operator
 * delete on the landing pad); else nothing. */
// FUNC 0x00110628 arcade_create_end_button
Transitive *arcade_create_end_button(void *ctx, PageLayout *layout, Object *parent,
                                     MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    int open = 0;
    if (getGameStatus()->getEventResults()->beginnerLeagueWon() != 0 ||
        getGameStatus()->getEventResults()->amateurLeagueWon() != 0 ||
        getGameStatus()->getEventResults()->professionalLeagueWon() != 0 ||
        getGameStatus()->getArcadeProgressLevel() >= ARCADE_LEVELS)
        open = 1;

    if (open != 0) {
        /* The flash, .rodata 0x0030BFF8 (the same as T205's blink script):
         * opFadeColors alpha +10 / s for 0.1 s, alpha -2.5 / s for 0.4 s,
         * opJump -2.  A function-local static: egcs writes it into .rodata
         * when it compiles this function, where retail has it. */
        static const OperatorCode flash[] = {
            { 20, { 0 }, 0, { 0 }, 0.0f, 0.0f, 0.0f, 10.0f, 0.1f, { 0 }, 1 },
            { 20, { 0 }, 0, { 0 }, 0.0f, 0.0f, 0.0f, -2.5f, 0.4f, { 0 }, 1 },
            {  1, { -2 }, 0, { 0 }, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 0 },
        };
        Item *button = new Item(item, layout); /* implicit: Object::operator delete (if the constructor throws) */
        button->start(flash);
        return button;
    }
    return NULL;
}

/* "end_message": the same condition, a still Item (Object::operator
 * new(0x100), operator delete on the landing pad). */
// FUNC 0x00110730 arcade_create_end_message
Transitive *arcade_create_end_message(void *ctx, PageLayout *layout, Object *parent,
                                      MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    int open = 0;
    if (getGameStatus()->getEventResults()->beginnerLeagueWon() != 0 ||
        getGameStatus()->getEventResults()->amateurLeagueWon() != 0 ||
        getGameStatus()->getEventResults()->professionalLeagueWon() != 0 ||
        getGameStatus()->getArcadeProgressLevel() >= ARCADE_LEVELS)
        open = 1;

    Item *message = NULL;
    if (open != 0)
        message = new Item(item, layout); /* implicit: Object::operator delete (if the constructor throws) */
    return message;
}

/* "course_logo_NN" (course = NN - 1, an index of the "SINGLE_RACE" list):
 * the hidden picture "<course>_mini" placed on the item, only when the
 * course is open (its level <= GameStatus::arcadeLevel).  Object::operator
 * new(0x100), operator delete on the landing pad. */
// FUNC 0x00110820 arcade_create_course_logo
Transitive *arcade_create_course_logo(void *ctx, PageLayout *layout, Object *parent,
                                      MenuLayoutItem *item, int course, int pC, int p10, int p14)
{
    char picture[64];
    const char *name = product_config()->courseName(course, ARCADE_COURSE_LIST);
    int level = product_config()->courseLevel(course, ARCADE_COURSE_LIST);

    if (getGameStatus()->getArcadeProgressLevel() >= level) {
        sprintf(picture, "%s_mini", name);
        return new Item(picture, layout, item); /* implicit: Object::operator delete (if the constructor throws) */
    }
    return NULL;
}

/* "course_name_NN": the same with the picture "<course>_doc"
 * (Object::operator new(0x100), operator delete on the landing pad). */
// FUNC 0x00110910 arcade_create_course_name
Transitive *arcade_create_course_name(void *ctx, PageLayout *layout, Object *parent,
                                      MenuLayoutItem *item, int course, int pC, int p10, int p14)
{
    char picture[64];
    const char *name = product_config()->courseName(course, ARCADE_COURSE_LIST);
    int level = product_config()->courseLevel(course, ARCADE_COURSE_LIST);

    if (getGameStatus()->getArcadeProgressLevel() >= level) {
        sprintf(picture, "%s_doc", name);
        return new Item(picture, layout, item); /* implicit: Object::operator delete (if the constructor throws) */
    }
    return NULL;
}

/* "mark_<difficulty>_NN" (course = NN - 1, difficulty ARCADE_DIFFICULTY_*):
 * the clear mark "clear_easy" / "clear_normal" / "clear_hard" placed on the
 * item when the course is won at that difficulty or a harder one.  Quirk:
 * there is no case for ARCADE_DIFFICULTY_PRO, so the 34 "mark_pro_NN"
 * entries never make a mark.  Object::operator new(0x100), operator delete
 * on the landing pad. */
// FUNC 0x00110A00 arcade_create_clear_mark
Transitive *arcade_create_clear_mark(void *ctx, PageLayout *layout, Object *parent,
                                     MenuLayoutItem *item, int course, int difficulty,
                                     int p10, int p14)
{
    const char *picture = NULL;
    int cleared = 0;
    u64 code = nameToCode(product_config()->courseName(course, ARCADE_COURSE_LIST));
    ArcadeRecords *records = getGameStatus()->getArcadeRecords();

    switch (difficulty) {
    case ARCADE_DIFFICULTY_EASY:
        picture = "clear_easy";
        /* (the binary shares the normal / hard tests with the next case) */
        if (records->getPlace(ARCADE_RACE_EASY, code) == PLACE_WON ||
            records->getPlace(ARCADE_RACE_NORMAL, code) == PLACE_WON ||
            records->getPlace(ARCADE_RACE_HARD, code) == PLACE_WON)
            cleared = 1;
        break;
    case ARCADE_DIFFICULTY_NORMAL:
        picture = "clear_normal";
        if (records->getPlace(ARCADE_RACE_NORMAL, code) == PLACE_WON ||
            records->getPlace(ARCADE_RACE_HARD, code) == PLACE_WON)
            cleared = 1;
        break;
    case ARCADE_DIFFICULTY_HARD:
        picture = "clear_hard";
        if (records->getPlace(ARCADE_RACE_HARD, code) == PLACE_WON)
            cleared = 1;
        break;
    }

    if (cleared != 0)
        return new Item(picture, layout, item); /* implicit: Object::operator delete (if the constructor throws) */
    return NULL;
}

/* "prize_<difficulty>_<letter>" (the courses [first, end) of a group: a
 * 0..5, b 6..11, c 12..17, d 18..23, e 24..28, f 29..33; difficulty easy /
 * normal / hard): the prize picture "prize_item_<difficulty>" placed on the
 * item.  Won (every course of the group won at that difficulty or a harder
 * one, or an empty group): it flashes (g_0030C130); else it is dimmed (grey
 * 0x80, alpha 0x20).  Another difficulty gives a dim Item of the name NULL
 * (no picture).  Object::operator new(0x100) in both branches, operator
 * delete on their landing pads.  The loops step the parameter itself (the
 * first course): with a local copy, GCSE's copy propagation puts the first
 * loop's entry test on the parameter and the code differs. */
// FUNC 0x00110BB8 arcade_create_prize_item
Transitive *arcade_create_prize_item(void *ctx, PageLayout *layout, Object *parent,
                                     MenuLayoutItem *item, int course, int end, int difficulty,
                                     int p14)
{
    const char *picture = NULL;
    ArcadeRecords *records = getGameStatus()->getArcadeRecords();
    int won = 0;
    u64 code;

    switch (difficulty) {
    case ARCADE_DIFFICULTY_EASY:
        picture = "prize_item_easy";
        for (; course < end; course++) {
            code = nameToCode(product_config()->courseName(course, ARCADE_COURSE_LIST));
            if (records->getPlace(ARCADE_RACE_EASY, code) != PLACE_WON &&
                records->getPlace(ARCADE_RACE_NORMAL, code) != PLACE_WON &&
                records->getPlace(ARCADE_RACE_HARD, code) != PLACE_WON)
                break;
        }
        won = (course >= end);
        break;
    case ARCADE_DIFFICULTY_NORMAL:
        picture = "prize_item_normal";
        for (; course < end; course++) {
            code = nameToCode(product_config()->courseName(course, ARCADE_COURSE_LIST));
            if (records->getPlace(ARCADE_RACE_NORMAL, code) != PLACE_WON &&
                records->getPlace(ARCADE_RACE_HARD, code) != PLACE_WON)
                break;
        }
        won = (course >= end);
        break;
    case ARCADE_DIFFICULTY_HARD:
        picture = "prize_item_hard";
        for (; course < end; course++) {
            code = nameToCode(product_config()->courseName(course, ARCADE_COURSE_LIST));
            if (records->getPlace(ARCADE_RACE_HARD, code) != PLACE_WON)
                break;
        }
        won = (course >= end);
        break;
    }

    Item *prize;
    if (won != 0) {
        /* The flash of a won prize, 0x0030C130: the end button's twice as
         * fast (alpha +20 / s for 0.05 s, -5 / s for 0.2 s, opJump -2). */
        static const OperatorCode flash[] = {
            { 20, { 0 }, 0, { 0 }, 0.0f, 0.0f, 0.0f, 20.0f, 0.05f, { 0 }, 1 },
            { 20, { 0 }, 0, { 0 }, 0.0f, 0.0f, 0.0f, -5.0f, 0.2f, { 0 }, 1 },
            {  1, { -2 }, 0, { 0 }, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, { 0 }, 0 },
        };
        prize = new Item(picture, layout, item); /* implicit: Object::operator delete (if the constructor throws) */
        prize->start(flash);
        return prize;
    }

    prize = new Item(picture, layout, item); /* implicit: Object::operator delete (if the constructor throws) */
    /* The inline Drawable::setColor(u32) (T259; Item::setColor is the text
     * colour): each corner through a Color4f temporary.  The binary has r,
     * g, b folded (0x80 / 255, an .sdata 0x3F008081 per corner) and converts
     * the alpha 0x20 at run time as an unsigned byte. */
    prize->Drawable::setColor(0x20808080);
    return prize;
}

/* The table's entry count, .sdata 0x00350F48: a constant word (234), read
 * by ArcadeControl::setup (T010).  Defined here, after the hooks, where the
 * original defined it (its place in .sdata, between the hooks' short strings
 * and the table's, shows it). */
const int g_00350F48 = sizeof(g_00292A18) / sizeof(g_00292A18[0]);

/* The hook table, .data 0x00292A18..0x00294008: zeros in the object, filled
 * at boot by T030_static_init.  A C++ global without an initialiser: egcs
 * writes it as zeros at the end of the file's .data, in front of the frame
 * info, where retail has it (the original's MenuHook constructor calls left
 * it zero in .data as well, see T030_static_init). */
MenuHook g_00292A18[234];                   /* 0x00292A18 */

/* One table entry: the inline constructor of the original's MenuHook (see
 * T030_static_init). */
static inline void set_entry(MenuHook *e, const char *name, MenuHookFn *create,
                             int p8, int pC, int p10, int p14)
{
    e->name   = name;
    e->create = create;
    e->p8     = p8;
    e->pC     = pC;
    e->p10    = p10;
    e->p14    = p14;
}

/* The hooks of other translation units (MenuHookFns). */
#define HOOK_BLINK      create_blink                        /* T205 */
#define HOOK_FLASH_ITEM create_flash_item                   /* T033 */

/* One element as egcs's build_vec_init expands it: the constructor called
 * on a copy of the cursor (`this'), then the cursor and the count of the
 * elements left stepped.  (No do-while wrapper: its loop notes would change
 * the code.) */
#define HOOK(name, create, p8, pC, p10, p14)                                  \
    { MenuHook *e = base; set_entry(e, name, create, p8, pC, p10, p14); }   \
    base++;                                                                   \
    left--

/* Static initialiser of the hook table g_00292A18 (.ctors entry 0x0028F044):
 * a compiler-generated _GLOBAL_$I$ function in the original.  The names are
 * sorted (findHook binary-searches them); entry i is at 0x00292A18 + 0x18 * i.
 *
 * The original defined the table here, with MenuHook objects built by a
 * constructor, e.g. MenuHook("blink", create_blink, 0, 0, 0, 0): egcs builds
 * such a table at run time, in this function (cp/init.c build_vec_init), and
 * leaves .data zero.  Written that way (a MenuHook with an inline constructor
 * and the table definition) the function compiles to exactly these
 * instructions.  The decomp defines the table without initialisers (above:
 * the same zeros in .data; MenuHook stays a plain struct for T229 and T010)
 * and writes the function out: an exact form, not the original.  It spells
 * out what build_vec_init expands, including the count of the elements left
 * (`left', dead: flow deletes it) and the copy of the cursor per element;
 * without either, the 234 stores come out with other registers (45-53%
 * instead of exact).  A plain `p->name = ...; p++' walk
 * computes the same. */
// FUNC 0x00111098 T030_static_init
void T030_static_init(void)
{
    MenuHook *base = g_00292A18;
    int left = 233;                         /* build_vec_init's iterator: elements - 1 */

    HOOK("blink",           HOOK_BLINK,                     0,  0, 0, 0);
    HOOK("connect_status",  arcade_create_connect_status,   0,  0, 0, 0);
    HOOK("countdown",       arcade_create_countdown,        0,  0, 0, 0);
    HOOK("course_logo_01",  arcade_create_course_logo,      0,  0, 0, 0);
    HOOK("course_logo_02",  arcade_create_course_logo,      1,  0, 0, 0);
    HOOK("course_logo_03",  arcade_create_course_logo,      2,  0, 0, 0);
    HOOK("course_logo_04",  arcade_create_course_logo,      3,  0, 0, 0);
    HOOK("course_logo_05",  arcade_create_course_logo,      4,  0, 0, 0);
    HOOK("course_logo_06",  arcade_create_course_logo,      5,  0, 0, 0);
    HOOK("course_logo_07",  arcade_create_course_logo,      6,  0, 0, 0);
    HOOK("course_logo_08",  arcade_create_course_logo,      7,  0, 0, 0);
    HOOK("course_logo_09",  arcade_create_course_logo,      8,  0, 0, 0);
    HOOK("course_logo_10",  arcade_create_course_logo,      9,  0, 0, 0);
    HOOK("course_logo_11",  arcade_create_course_logo,     10,  0, 0, 0);
    HOOK("course_logo_12",  arcade_create_course_logo,     11,  0, 0, 0);
    HOOK("course_logo_13",  arcade_create_course_logo,     12,  0, 0, 0);
    HOOK("course_logo_14",  arcade_create_course_logo,     13,  0, 0, 0);
    HOOK("course_logo_15",  arcade_create_course_logo,     14,  0, 0, 0);
    HOOK("course_logo_16",  arcade_create_course_logo,     15,  0, 0, 0);
    HOOK("course_logo_17",  arcade_create_course_logo,     16,  0, 0, 0);
    HOOK("course_logo_18",  arcade_create_course_logo,     17,  0, 0, 0);
    HOOK("course_logo_19",  arcade_create_course_logo,     18,  0, 0, 0);
    HOOK("course_logo_20",  arcade_create_course_logo,     19,  0, 0, 0);
    HOOK("course_logo_21",  arcade_create_course_logo,     20,  0, 0, 0);
    HOOK("course_logo_22",  arcade_create_course_logo,     21,  0, 0, 0);
    HOOK("course_logo_23",  arcade_create_course_logo,     22,  0, 0, 0);
    HOOK("course_logo_24",  arcade_create_course_logo,     23,  0, 0, 0);
    HOOK("course_logo_25",  arcade_create_course_logo,     24,  0, 0, 0);
    HOOK("course_logo_26",  arcade_create_course_logo,     25,  0, 0, 0);
    HOOK("course_logo_27",  arcade_create_course_logo,     26,  0, 0, 0);
    HOOK("course_logo_28",  arcade_create_course_logo,     27,  0, 0, 0);
    HOOK("course_logo_29",  arcade_create_course_logo,     28,  0, 0, 0);
    HOOK("course_logo_30",  arcade_create_course_logo,     29,  0, 0, 0);
    HOOK("course_logo_31",  arcade_create_course_logo,     30,  0, 0, 0);
    HOOK("course_logo_32",  arcade_create_course_logo,     31,  0, 0, 0);
    HOOK("course_logo_33",  arcade_create_course_logo,     32,  0, 0, 0);
    HOOK("course_logo_34",  arcade_create_course_logo,     33,  0, 0, 0);
    HOOK("course_name_01",  arcade_create_course_name,      0,  0, 0, 0);
    HOOK("course_name_02",  arcade_create_course_name,      1,  0, 0, 0);
    HOOK("course_name_03",  arcade_create_course_name,      2,  0, 0, 0);
    HOOK("course_name_04",  arcade_create_course_name,      3,  0, 0, 0);
    HOOK("course_name_05",  arcade_create_course_name,      4,  0, 0, 0);
    HOOK("course_name_06",  arcade_create_course_name,      5,  0, 0, 0);
    HOOK("course_name_07",  arcade_create_course_name,      6,  0, 0, 0);
    HOOK("course_name_08",  arcade_create_course_name,      7,  0, 0, 0);
    HOOK("course_name_09",  arcade_create_course_name,      8,  0, 0, 0);
    HOOK("course_name_10",  arcade_create_course_name,      9,  0, 0, 0);
    HOOK("course_name_11",  arcade_create_course_name,     10,  0, 0, 0);
    HOOK("course_name_12",  arcade_create_course_name,     11,  0, 0, 0);
    HOOK("course_name_13",  arcade_create_course_name,     12,  0, 0, 0);
    HOOK("course_name_14",  arcade_create_course_name,     13,  0, 0, 0);
    HOOK("course_name_15",  arcade_create_course_name,     14,  0, 0, 0);
    HOOK("course_name_16",  arcade_create_course_name,     15,  0, 0, 0);
    HOOK("course_name_17",  arcade_create_course_name,     16,  0, 0, 0);
    HOOK("course_name_18",  arcade_create_course_name,     17,  0, 0, 0);
    HOOK("course_name_19",  arcade_create_course_name,     18,  0, 0, 0);
    HOOK("course_name_20",  arcade_create_course_name,     19,  0, 0, 0);
    HOOK("course_name_21",  arcade_create_course_name,     20,  0, 0, 0);
    HOOK("course_name_22",  arcade_create_course_name,     21,  0, 0, 0);
    HOOK("course_name_23",  arcade_create_course_name,     22,  0, 0, 0);
    HOOK("course_name_24",  arcade_create_course_name,     23,  0, 0, 0);
    HOOK("course_name_25",  arcade_create_course_name,     24,  0, 0, 0);
    HOOK("course_name_26",  arcade_create_course_name,     25,  0, 0, 0);
    HOOK("course_name_27",  arcade_create_course_name,     26,  0, 0, 0);
    HOOK("course_name_28",  arcade_create_course_name,     27,  0, 0, 0);
    HOOK("course_name_29",  arcade_create_course_name,     28,  0, 0, 0);
    HOOK("course_name_30",  arcade_create_course_name,     29,  0, 0, 0);
    HOOK("course_name_31",  arcade_create_course_name,     30,  0, 0, 0);
    HOOK("course_name_32",  arcade_create_course_name,     31,  0, 0, 0);
    HOOK("course_name_33",  arcade_create_course_name,     32,  0, 0, 0);
    HOOK("course_name_34",  arcade_create_course_name,     33,  0, 0, 0);
    HOOK("diagonal_in",     arcade_create_diagonal_in,      0,  0, 0, 0);
    HOOK("diagonal_out",    arcade_create_diagonal_out,     0,  0, 0, 0);
    HOOK("end_button",      arcade_create_end_button,       0,  0, 0, 0);
    HOOK("end_message",     arcade_create_end_message,      0,  0, 0, 0);
    HOOK("flash_item",      HOOK_FLASH_ITEM,                0,  0, 0, 0);
    HOOK("mark_easy_01",    arcade_create_clear_mark,       0,  0, 0, 0);
    HOOK("mark_easy_02",    arcade_create_clear_mark,       1,  0, 0, 0);
    HOOK("mark_easy_03",    arcade_create_clear_mark,       2,  0, 0, 0);
    HOOK("mark_easy_04",    arcade_create_clear_mark,       3,  0, 0, 0);
    HOOK("mark_easy_05",    arcade_create_clear_mark,       4,  0, 0, 0);
    HOOK("mark_easy_06",    arcade_create_clear_mark,       5,  0, 0, 0);
    HOOK("mark_easy_07",    arcade_create_clear_mark,       6,  0, 0, 0);
    HOOK("mark_easy_08",    arcade_create_clear_mark,       7,  0, 0, 0);
    HOOK("mark_easy_09",    arcade_create_clear_mark,       8,  0, 0, 0);
    HOOK("mark_easy_10",    arcade_create_clear_mark,       9,  0, 0, 0);
    HOOK("mark_easy_11",    arcade_create_clear_mark,      10,  0, 0, 0);
    HOOK("mark_easy_12",    arcade_create_clear_mark,      11,  0, 0, 0);
    HOOK("mark_easy_13",    arcade_create_clear_mark,      12,  0, 0, 0);
    HOOK("mark_easy_14",    arcade_create_clear_mark,      13,  0, 0, 0);
    HOOK("mark_easy_15",    arcade_create_clear_mark,      14,  0, 0, 0);
    HOOK("mark_easy_16",    arcade_create_clear_mark,      15,  0, 0, 0);
    HOOK("mark_easy_17",    arcade_create_clear_mark,      16,  0, 0, 0);
    HOOK("mark_easy_18",    arcade_create_clear_mark,      17,  0, 0, 0);
    HOOK("mark_easy_19",    arcade_create_clear_mark,      18,  0, 0, 0);
    HOOK("mark_easy_20",    arcade_create_clear_mark,      19,  0, 0, 0);
    HOOK("mark_easy_21",    arcade_create_clear_mark,      20,  0, 0, 0);
    HOOK("mark_easy_22",    arcade_create_clear_mark,      21,  0, 0, 0);
    HOOK("mark_easy_23",    arcade_create_clear_mark,      22,  0, 0, 0);
    HOOK("mark_easy_24",    arcade_create_clear_mark,      23,  0, 0, 0);
    HOOK("mark_easy_25",    arcade_create_clear_mark,      24,  0, 0, 0);
    HOOK("mark_easy_26",    arcade_create_clear_mark,      25,  0, 0, 0);
    HOOK("mark_easy_27",    arcade_create_clear_mark,      26,  0, 0, 0);
    HOOK("mark_easy_28",    arcade_create_clear_mark,      27,  0, 0, 0);
    HOOK("mark_easy_29",    arcade_create_clear_mark,      28,  0, 0, 0);
    HOOK("mark_easy_30",    arcade_create_clear_mark,      29,  0, 0, 0);
    HOOK("mark_easy_31",    arcade_create_clear_mark,      30,  0, 0, 0);
    HOOK("mark_easy_32",    arcade_create_clear_mark,      31,  0, 0, 0);
    HOOK("mark_easy_33",    arcade_create_clear_mark,      32,  0, 0, 0);
    HOOK("mark_easy_34",    arcade_create_clear_mark,      33,  0, 0, 0);
    HOOK("mark_hard_01",    arcade_create_clear_mark,       0,  2, 0, 0);
    HOOK("mark_hard_02",    arcade_create_clear_mark,       1,  2, 0, 0);
    HOOK("mark_hard_03",    arcade_create_clear_mark,       2,  2, 0, 0);
    HOOK("mark_hard_04",    arcade_create_clear_mark,       3,  2, 0, 0);
    HOOK("mark_hard_05",    arcade_create_clear_mark,       4,  2, 0, 0);
    HOOK("mark_hard_06",    arcade_create_clear_mark,       5,  2, 0, 0);
    HOOK("mark_hard_07",    arcade_create_clear_mark,       6,  2, 0, 0);
    HOOK("mark_hard_08",    arcade_create_clear_mark,       7,  2, 0, 0);
    HOOK("mark_hard_09",    arcade_create_clear_mark,       8,  2, 0, 0);
    HOOK("mark_hard_10",    arcade_create_clear_mark,       9,  2, 0, 0);
    HOOK("mark_hard_11",    arcade_create_clear_mark,      10,  2, 0, 0);
    HOOK("mark_hard_12",    arcade_create_clear_mark,      11,  2, 0, 0);
    HOOK("mark_hard_13",    arcade_create_clear_mark,      12,  2, 0, 0);
    HOOK("mark_hard_14",    arcade_create_clear_mark,      13,  2, 0, 0);
    HOOK("mark_hard_15",    arcade_create_clear_mark,      14,  2, 0, 0);
    HOOK("mark_hard_16",    arcade_create_clear_mark,      15,  2, 0, 0);
    HOOK("mark_hard_17",    arcade_create_clear_mark,      16,  2, 0, 0);
    HOOK("mark_hard_18",    arcade_create_clear_mark,      17,  2, 0, 0);
    HOOK("mark_hard_19",    arcade_create_clear_mark,      18,  2, 0, 0);
    HOOK("mark_hard_20",    arcade_create_clear_mark,      19,  2, 0, 0);
    HOOK("mark_hard_21",    arcade_create_clear_mark,      20,  2, 0, 0);
    HOOK("mark_hard_22",    arcade_create_clear_mark,      21,  2, 0, 0);
    HOOK("mark_hard_23",    arcade_create_clear_mark,      22,  2, 0, 0);
    HOOK("mark_hard_24",    arcade_create_clear_mark,      23,  2, 0, 0);
    HOOK("mark_hard_25",    arcade_create_clear_mark,      24,  2, 0, 0);
    HOOK("mark_hard_26",    arcade_create_clear_mark,      25,  2, 0, 0);
    HOOK("mark_hard_27",    arcade_create_clear_mark,      26,  2, 0, 0);
    HOOK("mark_hard_28",    arcade_create_clear_mark,      27,  2, 0, 0);
    HOOK("mark_hard_29",    arcade_create_clear_mark,      28,  2, 0, 0);
    HOOK("mark_hard_30",    arcade_create_clear_mark,      29,  2, 0, 0);
    HOOK("mark_hard_31",    arcade_create_clear_mark,      30,  2, 0, 0);
    HOOK("mark_hard_32",    arcade_create_clear_mark,      31,  2, 0, 0);
    HOOK("mark_hard_33",    arcade_create_clear_mark,      32,  2, 0, 0);
    HOOK("mark_hard_34",    arcade_create_clear_mark,      33,  2, 0, 0);
    HOOK("mark_normal_01",  arcade_create_clear_mark,       0,  1, 0, 0);
    HOOK("mark_normal_02",  arcade_create_clear_mark,       1,  1, 0, 0);
    HOOK("mark_normal_03",  arcade_create_clear_mark,       2,  1, 0, 0);
    HOOK("mark_normal_04",  arcade_create_clear_mark,       3,  1, 0, 0);
    HOOK("mark_normal_05",  arcade_create_clear_mark,       4,  1, 0, 0);
    HOOK("mark_normal_06",  arcade_create_clear_mark,       5,  1, 0, 0);
    HOOK("mark_normal_07",  arcade_create_clear_mark,       6,  1, 0, 0);
    HOOK("mark_normal_08",  arcade_create_clear_mark,       7,  1, 0, 0);
    HOOK("mark_normal_09",  arcade_create_clear_mark,       8,  1, 0, 0);
    HOOK("mark_normal_10",  arcade_create_clear_mark,       9,  1, 0, 0);
    HOOK("mark_normal_11",  arcade_create_clear_mark,      10,  1, 0, 0);
    HOOK("mark_normal_12",  arcade_create_clear_mark,      11,  1, 0, 0);
    HOOK("mark_normal_13",  arcade_create_clear_mark,      12,  1, 0, 0);
    HOOK("mark_normal_14",  arcade_create_clear_mark,      13,  1, 0, 0);
    HOOK("mark_normal_15",  arcade_create_clear_mark,      14,  1, 0, 0);
    HOOK("mark_normal_16",  arcade_create_clear_mark,      15,  1, 0, 0);
    HOOK("mark_normal_17",  arcade_create_clear_mark,      16,  1, 0, 0);
    HOOK("mark_normal_18",  arcade_create_clear_mark,      17,  1, 0, 0);
    HOOK("mark_normal_19",  arcade_create_clear_mark,      18,  1, 0, 0);
    HOOK("mark_normal_20",  arcade_create_clear_mark,      19,  1, 0, 0);
    HOOK("mark_normal_21",  arcade_create_clear_mark,      20,  1, 0, 0);
    HOOK("mark_normal_22",  arcade_create_clear_mark,      21,  1, 0, 0);
    HOOK("mark_normal_23",  arcade_create_clear_mark,      22,  1, 0, 0);
    HOOK("mark_normal_24",  arcade_create_clear_mark,      23,  1, 0, 0);
    HOOK("mark_normal_25",  arcade_create_clear_mark,      24,  1, 0, 0);
    HOOK("mark_normal_26",  arcade_create_clear_mark,      25,  1, 0, 0);
    HOOK("mark_normal_27",  arcade_create_clear_mark,      26,  1, 0, 0);
    HOOK("mark_normal_28",  arcade_create_clear_mark,      27,  1, 0, 0);
    HOOK("mark_normal_29",  arcade_create_clear_mark,      28,  1, 0, 0);
    HOOK("mark_normal_30",  arcade_create_clear_mark,      29,  1, 0, 0);
    HOOK("mark_normal_31",  arcade_create_clear_mark,      30,  1, 0, 0);
    HOOK("mark_normal_32",  arcade_create_clear_mark,      31,  1, 0, 0);
    HOOK("mark_normal_33",  arcade_create_clear_mark,      32,  1, 0, 0);
    HOOK("mark_normal_34",  arcade_create_clear_mark,      33,  1, 0, 0);
    HOOK("mark_pro_01",     arcade_create_clear_mark,       0,  3, 0, 0);
    HOOK("mark_pro_02",     arcade_create_clear_mark,       1,  3, 0, 0);
    HOOK("mark_pro_03",     arcade_create_clear_mark,       2,  3, 0, 0);
    HOOK("mark_pro_04",     arcade_create_clear_mark,       3,  3, 0, 0);
    HOOK("mark_pro_05",     arcade_create_clear_mark,       4,  3, 0, 0);
    HOOK("mark_pro_06",     arcade_create_clear_mark,       5,  3, 0, 0);
    HOOK("mark_pro_07",     arcade_create_clear_mark,       6,  3, 0, 0);
    HOOK("mark_pro_08",     arcade_create_clear_mark,       7,  3, 0, 0);
    HOOK("mark_pro_09",     arcade_create_clear_mark,       8,  3, 0, 0);
    HOOK("mark_pro_10",     arcade_create_clear_mark,       9,  3, 0, 0);
    HOOK("mark_pro_11",     arcade_create_clear_mark,      10,  3, 0, 0);
    HOOK("mark_pro_12",     arcade_create_clear_mark,      11,  3, 0, 0);
    HOOK("mark_pro_13",     arcade_create_clear_mark,      12,  3, 0, 0);
    HOOK("mark_pro_14",     arcade_create_clear_mark,      13,  3, 0, 0);
    HOOK("mark_pro_15",     arcade_create_clear_mark,      14,  3, 0, 0);
    HOOK("mark_pro_16",     arcade_create_clear_mark,      15,  3, 0, 0);
    HOOK("mark_pro_17",     arcade_create_clear_mark,      16,  3, 0, 0);
    HOOK("mark_pro_18",     arcade_create_clear_mark,      17,  3, 0, 0);
    HOOK("mark_pro_19",     arcade_create_clear_mark,      18,  3, 0, 0);
    HOOK("mark_pro_20",     arcade_create_clear_mark,      19,  3, 0, 0);
    HOOK("mark_pro_21",     arcade_create_clear_mark,      20,  3, 0, 0);
    HOOK("mark_pro_22",     arcade_create_clear_mark,      21,  3, 0, 0);
    HOOK("mark_pro_23",     arcade_create_clear_mark,      22,  3, 0, 0);
    HOOK("mark_pro_24",     arcade_create_clear_mark,      23,  3, 0, 0);
    HOOK("mark_pro_25",     arcade_create_clear_mark,      24,  3, 0, 0);
    HOOK("mark_pro_26",     arcade_create_clear_mark,      25,  3, 0, 0);
    HOOK("mark_pro_27",     arcade_create_clear_mark,      26,  3, 0, 0);
    HOOK("mark_pro_28",     arcade_create_clear_mark,      27,  3, 0, 0);
    HOOK("mark_pro_29",     arcade_create_clear_mark,      28,  3, 0, 0);
    HOOK("mark_pro_30",     arcade_create_clear_mark,      29,  3, 0, 0);
    HOOK("mark_pro_31",     arcade_create_clear_mark,      30,  3, 0, 0);
    HOOK("mark_pro_32",     arcade_create_clear_mark,      31,  3, 0, 0);
    HOOK("mark_pro_33",     arcade_create_clear_mark,      32,  3, 0, 0);
    HOOK("mark_pro_34",     arcade_create_clear_mark,      33,  3, 0, 0);
    HOOK("mpeg",            arcade_create_mpeg,             0,  0, 0, 0);
    HOOK("mpeg_class",      arcade_create_mpeg_class,       0,  0, 0, 0);
    HOOK("mpeg_course",     arcade_create_mpeg_course,      0,  0, 0, 0);
    HOOK("my_name",         arcade_create_my_name,          0,  0, 0, 0);
    HOOK("prize_easy_a",    arcade_create_prize_item,       0,  6, 0, 0);
    HOOK("prize_easy_b",    arcade_create_prize_item,       6, 12, 0, 0);
    HOOK("prize_easy_c",    arcade_create_prize_item,      12, 18, 0, 0);
    HOOK("prize_easy_d",    arcade_create_prize_item,      18, 24, 0, 0);
    HOOK("prize_easy_e",    arcade_create_prize_item,      24, 29, 0, 0);
    HOOK("prize_easy_f",    arcade_create_prize_item,      29, 34, 0, 0);
    HOOK("prize_hard_a",    arcade_create_prize_item,       0,  6, 2, 0);
    HOOK("prize_hard_b",    arcade_create_prize_item,       6, 12, 2, 0);
    HOOK("prize_hard_c",    arcade_create_prize_item,      12, 18, 2, 0);
    HOOK("prize_hard_d",    arcade_create_prize_item,      18, 24, 2, 0);
    HOOK("prize_hard_e",    arcade_create_prize_item,      24, 29, 2, 0);
    HOOK("prize_hard_f",    arcade_create_prize_item,      29, 34, 2, 0);
    HOOK("prize_normal_a",  arcade_create_prize_item,       0,  6, 1, 0);
    HOOK("prize_normal_b",  arcade_create_prize_item,       6, 12, 1, 0);
    HOOK("prize_normal_c",  arcade_create_prize_item,      12, 18, 1, 0);
    HOOK("prize_normal_d",  arcade_create_prize_item,      18, 24, 1, 0);
    HOOK("prize_normal_e",  arcade_create_prize_item,      24, 29, 1, 0);
    HOOK("prize_normal_f",  arcade_create_prize_item,      29, 34, 1, 0);
}
