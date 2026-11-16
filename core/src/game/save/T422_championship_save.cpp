/* T422 (0x00215EF8..0x00216290): the suspended championship of the save data
 * (GameStatus+0x2EF28): the page to go back to, the race codes, the session
 * and the 8 entrants' cars and points. */
#include "game/save/prize_records_internal.h"
#include "game/save/T422_championship_save.h"

/* The bytes of the object that are saved as they are (active .. sessions);
 * the entries follow, each the car's save block and the points. */
#define CHAMPIONSHIP_SAVE_HEAD 0x8C

/* Copies size bytes and returns the end of the save data: the size is a
 * parameter, so egcs calls memcpy (as retail) instead of expanding it. */
static inline const u8 *load_bytes(void *dst, const u8 *src, u32 size)
{
    memcpy(dst, src, size);
    return src + size;
}

static inline u8 *save_bytes(u8 *dst, const void *src, u32 size)
{
    memcpy(dst, src, size);
    return dst + size;
}

// FUNC 0x00215EF8 ChampionshipSave::ChampionshipSave
ChampionshipSave::ChampionshipSave()
{
    /* implicit: CarEquipments, ~CarEquipments (compiler-generated: bases, members, deleting form) */
    /* The 8 entries' CarEquipments::CarEquipments run first (the exception
     * cleanup calls CarEquipments::~CarEquipments on the ones already
     * built). */
    clear();
}

// FUNC 0x00216010 ChampionshipSave::clear
void ChampionshipSave::clear()
{
    active = 0;
    page[0] = 0;
    globalRaceCode[0] = 0;
    currentRaceCode[0] = 0;
    session = 0;
    sessions = 0;
    for (int i = 7; i >= 0; i--)
        entries[i].points = 0;
}

// FUNC 0x00216058 ChampionshipSave::setPage
void ChampionshipSave::setPage(const char *s)
{
    strcpy(page, s);
}

// FUNC 0x00216078 ChampionshipSave::setGlobalRaceCode
void ChampionshipSave::setGlobalRaceCode(const char *s)
{
    strcpy(globalRaceCode, s);
}

// FUNC 0x00216098 ChampionshipSave::setCurrentRaceCode
void ChampionshipSave::setCurrentRaceCode(const char *s)
{
    strcpy(currentRaceCode, s);
}

// FUNC 0x002160B8 ChampionshipSave::setSession
void ChampionshipSave::setSession(int v)
{
    session = v;
}

// FUNC 0x002160C0 ChampionshipSave::setSessions
void ChampionshipSave::setSessions(int v)
{
    sessions = v;
}

// FUNC 0x002160C8 ChampionshipSave::setCar
void ChampionshipSave::setCar(const CarEquipments *car, int i)
{
    entries[i].car = *car;                      /* implicit: CarEquipments::operator= (0x00231B90) */
}

// FUNC 0x002160F0 ChampionshipSave::setPoints
void ChampionshipSave::setPoints(int points, int i)
{
    entries[i].points = points;
}

// FUNC 0x00216108 ChampionshipSave::getPage
char *ChampionshipSave::getPage()
{
    return page;
}

// FUNC 0x00216110 ChampionshipSave::getGlobalRaceCode
char *ChampionshipSave::getGlobalRaceCode()
{
    return globalRaceCode;
}

// FUNC 0x00216118 ChampionshipSave::getCurrentRaceCode
char *ChampionshipSave::getCurrentRaceCode()
{
    return currentRaceCode;
}

// FUNC 0x00216120 ChampionshipSave::getSession
int ChampionshipSave::getSession()
{
    return session;
}

// FUNC 0x00216128 ChampionshipSave::getSessions
int ChampionshipSave::getSessions()
{
    return sessions;
}

// FUNC 0x00216130 ChampionshipSave::getCar
CarEquipments *ChampionshipSave::getCar(int i)
{
    return &entries[i].car;
}

// FUNC 0x00216148 ChampionshipSave::getPoints
int ChampionshipSave::getPoints(int i)
{
    return entries[i].points;
}

// FUNC 0x00216160 ChampionshipSave::getSize
int ChampionshipSave::getSize()
{
    return 0xF2C;                               /* 0x8C + 8 * (0x1D0 + 4) */
}

// FUNC 0x00216168 ChampionshipSave::load
const u8 *ChampionshipSave::load(const u8 *src, int arg)
{
    /* implicit: memcpy (load_bytes) */
    src = load_bytes((void *)this, src, CHAMPIONSHIP_SAVE_HEAD);
    for (int i = 0; i < 8; i++) {
        /* the points' address taken before the call: retail keeps it in its
         * own register (loop.c does not derive it from the car's) */
        int *points = &entries[i].points;
        src = entries[i].car.deserialize(src);
        src = load_bytes(points, src, sizeof(int));
    }
    return src;
}

// FUNC 0x002161F8 ChampionshipSave::save
u8 *ChampionshipSave::save(u8 *dst)
{
    /* implicit: memcpy (save_bytes) */
    dst = save_bytes(dst, (void *)this, CHAMPIONSHIP_SAVE_HEAD);
    for (int i = 0; i < 8; i++) {
        int *points = &entries[i].points;
        dst = entries[i].car.serialize(dst);
        dst = save_bytes(dst, points, sizeof(int));
    }
    return dst;
}
