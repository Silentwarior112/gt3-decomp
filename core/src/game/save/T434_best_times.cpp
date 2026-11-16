/* T434 (0x00216BB8..0x00216E58): 44 best times of the save data,
 * GameStatus+0x23B08 (? name and meaning: one per entry of the product
 * database's table at +0x1C, found by name with ProductConfig::index1C).  The
 * by-name update / get have no callers; GameStatus only constructs, clears,
 * loads and saves the table.
 *
 * Retail's T434 has no .rodata (no literal, no type name): the original saw
 * no class with a type name. */
#include "game/save/game_options_internal.h"
#include "game/race/core/T128_race_lap_record.h"  /* RACE_TIME_INVALID (0x157529FF: "no time") */
#include "game/data/T435_product_config.h"   /* product_config (index1C) */
#include "game/save/T434_best_times.h"

/* ---- BestTime ------------------------------------------------------------ */

// FUNC 0x00216BB8 BestTime::BestTime
BestTime::BestTime()
{
    clear();
}

// FUNC 0x00216BE0 BestTime::clear
void BestTime::clear()
{
    time = RACE_TIME_INVALID;
}

/* Unsigned compare. */
// FUNC 0x00216BF0 BestTime::update
void BestTime::update(u32 t)
{
    if (time == RACE_TIME_INVALID || t < time)
        time = t;
}

// FUNC 0x00216C18 BestTime::get
u32 BestTime::get()
{
    return time;
}

// FUNC 0x00216C20 BestTime::getSize
int BestTime::getSize()
{
    return 4;
}

// FUNC 0x00216C28 BestTime::load
const u8 *BestTime::load(const u8 *src, int arg)
{
    time = *(const u32 *)src;
    return src + 4;
}

// FUNC 0x00216C38 BestTime::save
u8 *BestTime::save(u8 *dst)
{
    *(u32 *)dst = time;
    return dst + 4;
}

/* ---- CourseBestTimes ----------------------------------------------------- */

// FUNC 0x00216C48 CourseBestTimes::CourseBestTimes
CourseBestTimes::CourseBestTimes()
{
    /* implicit: BestTime (compiler-generated: bases, members, deleting form) */
    /* the 44 element constructors (BestTime::BestTime, a count-down loop),
     * then clear() */
    clear();
}

// FUNC 0x00216CB0 CourseBestTimes::clear
void CourseBestTimes::clear()
{
    int i;

    for (i = 0; i < 44; i++)
        t[i].clear();
}

// FUNC 0x00216CF0 CourseBestTimes::find
BestTime *CourseBestTimes::find(const char *name)
{
    return &t[product_config()->index1C(name)];
}

/* (No callers.) */
// FUNC 0x00216D38 CourseBestTimes::update
void CourseBestTimes::update(const char *name, u32 time)
{
    find(name)->update(time);
}

/* (No callers.) */
// FUNC 0x00216D68 CourseBestTimes::get
u32 CourseBestTimes::get(const char *name)
{
    return find(name)->get();
}

// FUNC 0x00216D90 CourseBestTimes::getSize
int CourseBestTimes::getSize()
{
    return BestTime::getSize() * 44;
}

/* arg is not passed on: every time gets 0. */
// FUNC 0x00216DB8 CourseBestTimes::load
const u8 *CourseBestTimes::load(const u8 *src, int arg)
{
    int i;

    for (i = 0; i < 44; i++)
        src = t[i].load(src, 0);
    return src;
}

// FUNC 0x00216E08 CourseBestTimes::save
u8 *CourseBestTimes::save(u8 *dst)
{
    int i;

    for (i = 0; i < 44; i++)
        dst = t[i].save(dst);
    return dst;
}
