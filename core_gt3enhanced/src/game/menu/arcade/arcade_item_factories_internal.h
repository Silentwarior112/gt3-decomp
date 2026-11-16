/* arcade_item_factories_internal.h - unit u006 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x0010FF90..0x00113540, second pass.
 *
 * Translation unit:
 *   T030  the layout object hooks of the arcade-mode menu and their table
 *         g_00292A18 (234 MenuHooks, sorted by name), which
 *         gt2k_menu::ArcadeControl::setup (T010) gives to its PageLayouts
 *         (include/game/menu/arcade/T030_arcade_item_factories.h).
 *
 * How the table is used (T229): PageLayout::createObject looks the type of
 * an object item up (findHook -> find_hook, a binary search) and calls
 *     hook->create(layout->hookCtx, layout, parent, item,
 *                  hook->p8, hook->pC, hook->p10, hook->p14)
 * ($a4..$a7 = the four words, 0x001C2D98); hookCtx is the ArcadeControl.  A
 * NULL result means "no object for this item".
 *
 * Every class is defined in a canonical header; T030 includes the ones it
 * needs itself, in the order that gives retail's type names.  This header
 * has the unit's constants only.
 */
#ifndef U006_H
#define U006_H

/* ======================================================================== */
/* Named constants                                                          */
/* ======================================================================== */

/* The course list of the arcade single race (T435 ProductConfig). */
#define ARCADE_COURSE_LIST "SINGLE_RACE"

/* The arcade race codes whose records (T424 ArcadeRecords) tell whether a
 * course is cleared: the second letter is the difficulty ('E' easy, 'N'
 * normal, 'H' hard). */
#define ARCADE_RACE_EASY   "AECT0001"
#define ARCADE_RACE_NORMAL "ANCT0001"
#define ARCADE_RACE_HARD   "AHCT0001"
#define PLACE_WON          0                /* ArcadeRecords::getPlace: first place (T418) */

/* The difficulty word (pC of "mark_*" items, p10 of "prize_*" items). */
enum {
    ARCADE_DIFFICULTY_EASY   = 0,
    ARCADE_DIFFICULTY_NORMAL = 1,
    ARCADE_DIFFICULTY_HARD   = 2,
    ARCADE_DIFFICULTY_PRO    = 3            /* only "mark_pro_NN": no case, never an item */
};

/* The course levels of the arcade (ProductConfig::courseLevel 0..5).
 * GameStatus::arcadeLevel (getArcadeProgressLevel) is the number of levels
 * passed (GameStatus::updateArcadeProgress, u098): ARCADE_LEVELS once every
 * course has been won at some difficulty. */
#define ARCADE_LEVELS 6

/* The "countdown" selector: frames a second, the index of "count_none". */
enum { FRAMES_PER_SECOND = 60, COUNTDOWN_NONE = 10 };

#endif /* U006_H */
