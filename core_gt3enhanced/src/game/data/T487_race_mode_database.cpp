/* T487 - RaceModeDatabase: database/racemode.db (the race modes: music and
 * sound per play / replay / demo, up to 32 detail codes) and
 * database/racedetail.db (the details: type, position, size, alignment);
 * both are DataBlock tables (T481).  The static object g_002F00C0.
 * 0x002393D8..0x00239930.
 *
 * Names: the record accessors (getAlignY .. getNumRaceModes) are Lupo 2's;
 * the others describe what the listing does. */
#include "game/data/race_database_internal.h"
#include "game/data/T487_race_mode_database.h"

/* 0x002F00C0 (.data): the race mode database, defined here: egcs's
 * _GLOBAL_$I$ / $D$ of it are the retail static constructor / destructor
 * (the markers T487_static_init / fini below). */
RaceModeDatabase g_002F00C0;

/* Caller: T487_static_init. */
// FUNC 0x002393D8 RaceModeDatabase::RaceModeDatabase
RaceModeDatabase::RaceModeDatabase()
{
    modes   = NULL;
    details = NULL;
    mode    = NULL;
    detail  = NULL;
}

/* The buffers are not freed (release() does that). */
// FUNC 0x00239400 RaceModeDatabase::~RaceModeDatabase
RaceModeDatabase::~RaceModeDatabase()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* the vptr; egcs: operator delete if flags & 1 */
}

/* Reads `path' into buf; buf, or NULL on a failure.  Caller: load. */
// FUNC 0x00239430 RaceModeDatabase::loadFile
void *RaceModeDatabase::loadFile(const char *path, void *buf, int size)
{
    if (file_read_blocking(path, buf, size, 1, &vol_device_object) == NULL)
        return NULL;
    return buf;
}

/* Frees and reloads both files (0x2000- and 0x3000-byte 64-aligned
 * buffers); 1 if both were read.  A buffer whose read failed is not freed.
 * Caller: game_init. */
// FUNC 0x00239478 RaceModeDatabase::load
int RaceModeDatabase::load()
{
    void *buf;

    if (modes != NULL)
        free(modes);
    buf = memalign(0x40, 0x2000);
    modes = (DataBlock *)loadFile("database/racemode.db", buf, 0x2000);
    if (modes == NULL)
        return 0;
    if (details != NULL)
        free(details);
    buf = memalign(0x40, 0x3000);
    details = (DataBlock *)loadFile("database/racedetail.db", buf, 0x3000);
    return details != NULL;
}

/* Frees both buffers (the pointers are kept).  No caller. */
// FUNC 0x00239520 RaceModeDatabase::release
void RaceModeDatabase::release()
{
    if (modes != NULL)
        free(modes);
    if (details != NULL)
        free(details);
}

/* selectMode of the code of a name.  No caller. */
// FUNC 0x00239568 RaceModeDatabase::selectMode(const char *)
int RaceModeDatabase::selectMode(const char *name)
{
    return selectMode(nameToCode(name));
}

/* Selects mode `code'; 0 if absent (unchanged then). */
// FUNC 0x002395A0 RaceModeDatabase::selectMode
int RaceModeDatabase::selectMode(u64 code)
{
    int index;

    index = modes->search(code);
    if (index == -1)
        return 0;
    mode = (RaceModeRecord *)modes->getElement(index);
    return 1;
}

/* Selects the first mode whose id is `id' (a linear search); 0 if none.
 * The record pointer is built in two steps (`p = elements; p += size * i'),
 * as in getModeId / selectDetail and T484's regulated cars: retail keeps the
 * start and the offset in separate registers.  Caller:
 * RaceDisplay::setLayout. */
// FUNC 0x002395F8 RaceModeDatabase::selectModeById
int RaceModeDatabase::selectModeById(int id)
{
    u8  *p;
    int  i, n;

    n = modes->numElements;
    for (i = 0; i < n; i++) {
        p = modes->elements;
        p += modes->elementSize * i;
        if (((RaceModeRecord *)p)->id == id) {
            mode = (RaceModeRecord *)p;
            return 1;
        }
    }
    return 0;
}

/* The id of the mode of a name, -1 if absent.  Callers:
 * RaceBasic::setRaceMode, RaceSolitaire::virtual_8, RaceDisplay::virtual_0,
 * RaceDisplay::updateDisplayMode. */
// FUNC 0x00239650 RaceModeDatabase::getModeId
int RaceModeDatabase::getModeId(const char *name)
{
    int index;
    u8 *p;

    index = modes->search(nameToCode(name));
    if (index == -1)
        return -1;
    p  = modes->elements;
    p += modes->elementSize * index;
    return ((RaceModeRecord *)p)->id;
}

/* The details of the selected mode before the first "(Null)" (at most 32).
 * Callers: RaceDisplay::setLayout, selectDetail. */
// FUNC 0x002396A8 RaceModeDatabase::getNumDetails
int RaceModeDatabase::getNumDetails()
{
    u64 *list = mode->details;          /* before the call, as retail */
    u64  none = nameToCode("(Null)");
    int  n;

    for (n = 0; n < 0x20; n++) {
        if (list[n] == none)
            break;
    }
    return n;
}

/* Selects detail i of the selected mode; 0 if i is not below the count or
 * the detail is absent (two early returns: retail shares one `return 0'
 * block between them).  Caller: RaceDisplay::setLayout. */
// FUNC 0x00239718 RaceModeDatabase::selectDetail
int RaceModeDatabase::selectDetail(int i)
{
    int  index;
    u64 *p;

    if (i >= getNumDetails())
        return 0;
    p  = mode->details;
    p += i;
    index = details->search(*p);
    if (index == -1)
        return 0;
    detail = (RaceDetailRecord *)details->getElement(index);
    return 1;
}

/* ---- the static destructor and constructor of g_002F00C0 (egcs's
 * _GLOBAL_$D$ / _GLOBAL_$I$; retail has them here, between selectDetail
 * and the accessors) ------------------------------------------------------ */

// FUNC 0x00239798 T487_static_fini (_GLOBAL_$D$g_002F00C0)
/* the static destructor (the .dtors entry 0x0028F260): ~RaceModeDatabase on
 * g_002F00C0 with egcs flags 2 */
/* implicit: ~RaceModeDatabase (the static object's destructor) */

// FUNC 0x002397C0 T487_static_init (_GLOBAL_$I$g_002F00C0)
/* the static constructor (the .ctors entry 0x0028F18C): RaceModeDatabase::
 * RaceModeDatabase on g_002F00C0 */
/* implicit: RaceModeDatabase (the static object's constructor) */

/* ---- the accessors: inline members (Lupo 2's names; nothing calls them,
 * the callers read the records themselves).  egcs writes them because the
 * class's key function (~RaceModeDatabase) is in this file, after the static
 * destructor / constructor, with the end-of-file group (before
 * __tf16RaceModeDatabase); out-of-class inline members come out in the
 * reverse of their definition order, so getNumRaceModes is defined first
 * and getAlignY last (the frame info's FDE order, 0x002F00D4).  Lupo 2 has
 * the same functions in this definition order (its typeinfo function, then
 * getNumRaceModes .. getAlignY: docs/symbols/lupo2_classes.txt) --------- */

/* racemode.db's header. */
// FUNC 0x002398E0 RaceModeDatabase::getNumRaceModes
inline u16 RaceModeDatabase::getNumRaceModes()
{
    return modes->numElements;
}

// FUNC 0x002398D0 RaceModeDatabase::getVersion
inline u16 RaceModeDatabase::getVersion()
{
    return modes->version;
}

/* The selected mode's fields. */
// FUNC 0x002398C0 RaceModeDatabase::getMusicPlay
inline u8 RaceModeDatabase::getMusicPlay()
{
    return mode->musicPlay;
}

// FUNC 0x002398B0 RaceModeDatabase::getSoundPlay
inline u8 RaceModeDatabase::getSoundPlay()
{
    return mode->soundPlay;
}

// FUNC 0x002398A0 RaceModeDatabase::getMusicReplay
inline u8 RaceModeDatabase::getMusicReplay()
{
    return mode->musicReplay;
}

// FUNC 0x00239890 RaceModeDatabase::getSoundReplay
inline u8 RaceModeDatabase::getSoundReplay()
{
    return mode->soundReplay;
}

// FUNC 0x00239880 RaceModeDatabase::getMusicDemo
inline u8 RaceModeDatabase::getMusicDemo()
{
    return mode->musicDemo;
}

// FUNC 0x00239870 RaceModeDatabase::getSoundDemo
inline u8 RaceModeDatabase::getSoundDemo()
{
    return mode->soundDemo;
}

/* The selected detail's fields. */
// FUNC 0x00239860 RaceModeDatabase::getType
inline u16 RaceModeDatabase::getType()
{
    return detail->type;
}

// FUNC 0x00239850 RaceModeDatabase::getSubType
inline u16 RaceModeDatabase::getSubType()
{
    return detail->subType;
}

// FUNC 0x00239840 RaceModeDatabase::getMode
inline s16 RaceModeDatabase::getMode()
{
    return detail->mode;
}

// FUNC 0x00239830 RaceModeDatabase::getWidth
inline s16 RaceModeDatabase::getWidth()
{
    return detail->width;
}

// FUNC 0x00239820 RaceModeDatabase::getHeight
inline s16 RaceModeDatabase::getHeight()
{
    return detail->height;
}

// FUNC 0x00239810 RaceModeDatabase::getX
inline s16 RaceModeDatabase::getX()
{
    return detail->x;
}

// FUNC 0x00239800 RaceModeDatabase::getY
inline s16 RaceModeDatabase::getY()
{
    return detail->y;
}

// FUNC 0x002397F0 RaceModeDatabase::getAlignX
inline u16 RaceModeDatabase::getAlignX()
{
    return detail->alignX;
}

// FUNC 0x002397E0 RaceModeDatabase::getAlignY
inline u16 RaceModeDatabase::getAlignY()
{
    return detail->alignY;
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated: builds the __user_type_info __ti16RaceModeDatabase
 * (gp-0x4540) on the first call; vtable 0x00342528 entry 0. */
// FUNC 0x002398F0 __tf16RaceModeDatabase
extern "C" void *__tf16RaceModeDatabase(void)
{
    if (__ti16RaceModeDatabase[0] == 0)
        __rtti_user(__ti16RaceModeDatabase, "16RaceModeDatabase");
    return __ti16RaceModeDatabase;
}
#endif
