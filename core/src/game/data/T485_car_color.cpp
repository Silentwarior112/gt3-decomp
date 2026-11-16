/* T485 - CarColorDataBase (? name: no RTTI, no symbol): database/carcolor.db
 * (the colour lists of the cars, the colour table) and database/carcolor.sdb
 * (its strings), loaded into one 0x32000-byte buffer; the static object
 * g_00352D98.  0x00239008..0x002393D8.
 *
 * Names: the method names describe what the listing does. */
#include "game/data/race_database_internal.h"
#include "game/data/T485_car_color.h"

/* 0x00352D98 (.sdata): the colour database, defined here: egcs's
 * _GLOBAL_$I$ / $D$ of it are the retail static constructor / destructor
 * (the markers T485_static_init / fini below). */
CarColorDataBase g_00352D98;

// FUNC 0x00239008 CarColorDataBase::CarColorDataBase
CarColorDataBase::CarColorDataBase()
{
    db      = NULL;
    strings = NULL;
}

// FUNC 0x00239018 CarColorDataBase::~CarColorDataBase
CarColorDataBase::~CarColorDataBase()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* nothing; egcs: operator delete if flags & 1 */
}

/* Frees the old buffer, then loads both files into a new 0x32000-byte
 * buffer (64-aligned; not freed if a read fails).  Caller: game_init. */
// FUNC 0x00239040 CarColorDataBase::load
int CarColorDataBase::load()
{
    if (db != NULL)
        free(db);
    return loadFiles(memalign(0x40, 0x32000), 0x32000);
}

/* carcolor.db at buf, carcolor.sdb after it (at its size rounded up to 64);
 * 1 if both were read.  On a failure the buffer is kept (db may already
 * point to it). */
// FUNC 0x00239098 CarColorDataBase::loadFiles
int CarColorDataBase::loadFiles(void *buf, int size)
{
    int offset;
    u8 *sdb;

    if (file_read_blocking("database/carcolor.db", buf, size, 1, &vol_device_object) == NULL)
        return 0;
    db     = (CarColorHeader *)buf;
    offset = (db->size + 0x3F) & ~0x3F;
    sdb    = (u8 *)buf + offset;
    if (file_read_blocking("database/carcolor.sdb", sdb, size - offset, 1, &vol_device_object) == NULL)
        return 0;
    strings = (StringsDataBase *)sdb;
    return 1;
}

/* Binary search of car `code' in the car entries, which follow the header
 * (an empty table is not checked); NULL if absent.  `db' is read in the
 * loop (retail: the hoisted copy of it, the address recomputed from it each
 * time).  Callers: getNumColors, getColor, getColorIndex. */
// FUNC 0x00239140 CarColorDataBase::findCar
CarColorEntry *CarColorDataBase::findCar(u64 code)
{
    CarColorEntry *car;
    int            lo, hi, mid;

    lo   = -1;
    hi   = db->numCars;
    do {
        mid = (lo + hi) >> 1;
        car = &((CarColorEntry *)(db + 1))[mid];
        if (car->carCode == code)
            return car;
        if (car->carCode < code)
            lo = mid;
        else
            hi = mid;
    } while (lo + 1 != hi);
    return NULL;
}

/* Binary search of colour `code' in the colour table (a count word, then
 * the records); NULL if absent.  Callers: color_candidate_set,
 * MenuCarSpec::getColors, getColorName, GarageCar::getColor. */
// FUNC 0x002391A0 CarColorDataBase::findColor
CarColorRecord *CarColorDataBase::findColor(u32 code)
{
    int            *table;
    CarColorRecord *colors;
    CarColorRecord *color;
    int             lo, hi, mid;

    table  = (int *)((u8 *)db + db->colorTable);
    lo     = -1;
    hi     = table[0];
    colors = (CarColorRecord *)(table + 1);
    do {
        mid   = (lo + hi) >> 1;
        color = &colors[mid];
        if (color->code == code)
            return color;
        if (color->code < code)
            lo = mid;
        else
            hi = mid;
    } while (lo + 1 != hi);
    return NULL;
}

/* The number of colours of car `car', 0 if unknown.  Callers:
 * color_table_build, MenuCarSpec::getColorCount. */
// FUNC 0x00239200 CarColorDataBase::getNumColors
int CarColorDataBase::getNumColors(u64 car)
{
    CarColorEntry *entry;

    entry = findCar(car);
    if (entry == NULL)
        return 0;
    return entry->numColors;
}

/* Colour code i of car `car'; -1 if the car is unknown or i is not below
 * its colour count (signed).  u32 (a colour code): retail builds the -1 as
 * lui / ori.  9 callers (setColorIndex, ...). */
// FUNC 0x00239228 CarColorDataBase::getColor
u32 CarColorDataBase::getColor(u64 car, int i)
{
    CarColorEntry *entry;
    const u32     *list;

    entry = findCar(car);
    if (entry == NULL || i >= entry->numColors)
        return -1;
    list = (const u32 *)((u8 *)db + db->colorLists + (entry->offset >> 2) * 4);
    return list[i];
}

/* The index of colour code `color' among car `car''s colours (an unsigned
 * count), -1 if absent.  Callers: RaceEntryCar::updateColorIndex,
 * setColorCode. */
// FUNC 0x002392A8 CarColorDataBase::getColorIndex
int CarColorDataBase::getColorIndex(u64 car, int color)
{
    CarColorEntry *entry;
    const int     *list;
    u32            i;

    entry = findCar(car);
    if (entry != NULL) {
        list = (const int *)((u8 *)db + db->colorLists + (entry->offset >> 2) * 4);
        for (i = 0; i < (u32)entry->numColors; i++) {
            if (list[i] == color)
                return i;
        }
    }
    return -1;
}

/* The name of colour `color' (carcolor.sdb): nameIntl for the countries US
 * .. SP (1..6), nameJp for JP and any other value.  The colour is not
 * checked for NULL.  A switch: retail tests == 0, < 0 and < 7 (egcs's case
 * tree), an if would fold to one range test.  Caller:
 * MenuCarSpec::getColorName. */
// FUNC 0x00239338 CarColorDataBase::getColorName
const char *CarColorDataBase::getColorName(u32 color)
{
    CarColorRecord *rec;
    const char     *name;

    rec = findColor(color);
    switch (g_00352FD8.value) {
    case COUNTRY_JP:
    default:
        name = strings->getString(rec->nameJp);
        break;
    case COUNTRY_US:
    case COUNTRY_BR:
    case COUNTRY_FR:
    case COUNTRY_GR:
    case COUNTRY_IT:
    case COUNTRY_SP:
        name = strings->getString(rec->nameIntl);
        break;
    }
    return name;
}

/* ---- the static destructor and constructor of g_00352D98 (egcs's
 * _GLOBAL_$D$ / _GLOBAL_$I$, emitted after the functions) ---------------- */

// FUNC 0x00239398 T485_static_fini (_GLOBAL_$D$g_00352D98)
/* the static destructor (the .dtors entry 0x0028F25C): ~CarColorDataBase on
 * g_00352D98 with egcs flags 2 */
/* implicit: ~CarColorDataBase (the static object's destructor) */

// FUNC 0x002393B8 T485_static_init (_GLOBAL_$I$g_00352D98)
/* the static constructor (the .ctors entry 0x0028F188): CarColorDataBase::
 * CarColorDataBase on g_00352D98 */
/* implicit: CarColorDataBase (the static object's constructor) */
