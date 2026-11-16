/* T425 (0x00216E58..0x00217538): the GT mode event results of the save data,
 * GameStatus+0x23510: the best place (BestPlace, T418: 0 = won, 8 = never
 * raced) of each of the 382 events of the product database's event table
 * (sorted names, found with ProductConfig::eventIndex), and the "series
 * complete" checks used by the ending (T016), the arcade item factories
 * (T030) and the prize cars (T419).
 *
 * Retail's .rodata (0x003402E0..0x003403DA, unreferenced: type names only)
 * is the GameStatus's garage type names, then RegurationResult's: the
 * original saw RegurationResult before the GameStatus, as the other retail
 * files that use the race / car databases (T069, T124, T419 have the same
 * names in the same order).  Of T434, T425 and T426 (whose .rodata lie
 * between T424's and T427's) only T425 uses those databases (RaceDataBase,
 * race_name_split); T434 and T426 have no .rodata. */
// DATA_START 0x003402E0
#include "game/save/game_options_internal.h"
#include "game/data/T435_product_config.h"   /* product_config, the event table */
#include "game/data/T481_car_data_base.h"    /* nameToCode */
#include "game/data/T484_race_database.h"    /* RaceDataBase, RaceEvent (difficulty = the league) */
#include "game/data/T489_car_database.h"     /* race_name_split */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (seen, not used: its type name) */
#include "game/save/T429_game_status.h"      /* (seen, not used: the garage's type names) */
#include "game/save/T425_event_results.h"

/* ---- EventResult --------------------------------------------------------- */

// FUNC 0x00216E58 EventResult::EventResult
EventResult::EventResult()
{
    /* only the BestPlace member constructor (inline: place = 8) */
}

// FUNC 0x00216E68 EventResult::clear
void EventResult::clear()
{
    place.clear();
}

// FUNC 0x00216E78 EventResult::getSize
int EventResult::getSize()
{
    return BestPlace::getSize();
}

// FUNC 0x00216E98 EventResult::load
const u8 *EventResult::load(const u8 *src, int arg)
{
    return place.load(src, arg);
}

// FUNC 0x00216EB8 EventResult::save
u8 *EventResult::save(u8 *dst)
{
    return place.save(dst);
}

/* ---- EventResults -------------------------------------------------------- */

// FUNC 0x00216ED8 EventResults::EventResults
EventResults::EventResults()
{
    /* implicit: EventResult (compiler-generated: bases, members, deleting form) */
    /* the 382 element constructors (EventResult::EventResult, a count-down
     * loop), then clear() */
    clear();
}

// FUNC 0x00216F40 EventResults::clear
void EventResults::clear()
{
    int i;

    for (i = 0; i < 382; i++)
        e[i].clear();
}

/* The number of events won (of the database's event count). */
// FUNC 0x00216F80 EventResults::wins
int EventResults::wins()
{
    int events = product_config()->eventCount();
    int won = 0;
    int i;

    for (i = 0; i < events; i++) {
        if (e[i].place.get() == PLACE_WON)
            won++;
    }
    return won;
}

// FUNC 0x00216FE0 EventResults::count
int EventResults::count()
{
    return product_config()->eventCount();
}

// FUNC 0x00217008 EventResults::find
EventResult *EventResults::find(const char *event)
{
    return &e[product_config()->eventIndex(event)];
}

/* 1 if p is a new best place of `event'. */
// FUNC 0x00217050 EventResults::updatePlace
int EventResults::updatePlace(const char *event, int p)
{
    return find(event)->place.update(p);
}

// FUNC 0x00217080 EventResults::getPlace
int EventResults::getPlace(const char *event)
{
    return find(event)->place.get();
}

/* 1 if every event of league `league' has been won.  Only the event codes
 * ending in "00xx" or in "00" are looked at.  The league is RaceEvent::
 * difficulty of the code itself ("..00xx", race_name_split's single races)
 * or of the code with its last character incremented ("..00" -> "..01":
 * the first race of the series; ? the "..00" code has no race of its own).
 * select()'s result is not checked.  The result checked is the one of the
 * event code itself.  (Retail lays out the "..00" block first: the "..00xx"
 * test is written negated, its block in the else.) */
// FUNC 0x002170A0 EventResults::leagueWon
int EventResults::leagueWon(int league)
{
    int result = 1;
    int i = 0;
    int events = product_config()->eventCount();

    for (; i < events; i++) {
        const char *name = product_config()->eventName(i);
        int len = strlen(name);

        if (name[len - 4] != '0' || name[len - 3] != '0') {
            if (name[len - 2] == '0' && name[len - 1] == '0') {
                char code[0x40];                    /* sp+0x00 */
                strcpy(code, name);
                code[len - 1]++;
                RaceDataBase race;                  /* sp+0x40 */
                race.select(nameToCode(code));
                if (race.event->difficulty == league && find(name)->place.get() != PLACE_WON) {
                    result = 0;
                    break;
                }
            }
        } else {
            char code[0x40];                    /* sp+0x00 */
            strcpy(code, name);
            RaceDataBase race;                  /* sp+0x40 */
            race.select(nameToCode(code));
            if (race.event->difficulty == league && find(name)->place.get() != PLACE_WON) {
                result = 0;
                break;
            }
        }
    }
    return result;
}

// FUNC 0x002172C8 EventResults::beginnerLeagueWon
int EventResults::beginnerLeagueWon()
{
    return leagueWon(LEAGUE_BEGINNER);
}

// FUNC 0x002172E8 EventResults::amateurLeagueWon
int EventResults::amateurLeagueWon()
{
    return leagueWon(LEAGUE_AMATEUR);
}

// FUNC 0x00217308 EventResults::professionalLeagueWon
int EventResults::professionalLeagueWon()
{
    return leagueWon(LEAGUE_PROFESSIONAL);
}

/* 1 if every event whose code race_name_split classes as `kind' has been
 * won. */
// FUNC 0x00217328 EventResults::kindWon
int EventResults::kindWon(int kind)
{
    int result = 1;
    int i = 0;
    int events = product_config()->eventCount();

    for (; i < events; i++) {
        const char *name = product_config()->eventName(i);
        int eventKind, eventIndex;              /* sp+0x00, sp+0x04 */

        race_name_split(name, &eventKind, &eventIndex);
        if (eventKind == kind && find(name)->place.get() != PLACE_WON) {
            result = 0;
            break;
        }
    }
    return result;
}

/* Every "D.." (dirt: rally) event won (T419: the prize of reason 16). */
// FUNC 0x002173F0 EventResults::dirtEventsWon
int EventResults::dirtEventsWon()
{
    return kindWon(RACE_KIND_GT_DIRT);
}

/* Every "E.." (endurance) event won (T419: the prize of reason 17). */
// FUNC 0x00217410 EventResults::enduranceEventsWon
int EventResults::enduranceEventsWon()
{
    return kindWon(RACE_KIND_GT_ENDURANCE);
}

// FUNC 0x00217430 EventResults::allWon
int EventResults::allWon()
{
    return wins() == count();
}

// FUNC 0x00217470 EventResults::getSize
int EventResults::getSize()
{
    return EventResult::getSize() * 382;
}

/* arg is not passed on: every result gets 0. */
// FUNC 0x00217498 EventResults::load
const u8 *EventResults::load(const u8 *src, int arg)
{
    int i;

    for (i = 0; i < 382; i++)
        src = e[i].load(src, 0);
    return src;
}

// FUNC 0x002174E8 EventResults::save
u8 *EventResults::save(u8 *dst)
{
    int i;

    for (i = 0; i < 382; i++)
        dst = e[i].save(dst);
    return dst;
}
