/* T485_car_color.h - CarColorDataBase (? name), the colours of the cars
 * (translation unit T485, unit u108), 0x00239008..0x002393D8:
 * database/carcolor.db (the colour lists of the cars and the colour table)
 * and database/carcolor.sdb (its strings), read into one 0x32000-byte
 * buffer.  One static object, g_00352D98 (gp-0x4C58), built / destroyed by
 * T485's static constructor / destructor.
 *
 * Other units call the methods as free functions of the object (u109
 * F_00239228 = getColor, F_002391A0 = findColor, ...). */
#ifndef GAME_T485_CAR_COLOR_H
#define GAME_T485_CAR_COLOR_H

#include "game/game.h"

class StringsDataBase;                          /* T481_car_data_base.h (pointers only) */

/* A colour of the colour table of database/carcolor.db (sorted by code;
 * findColor returns one).  u108 calls it ColorInfo.  The names are offsets
 * of carcolor.sdb strings (getColorName).  0x10 bytes.  The plan put it in
 * T193 (u041, the random colours of a race). */
struct CarColorRecord {
    u32 code;                                   /* 0x0 the colour code (u041: id), the search key */
    u32 nameIntl;                               /* 0x4 the name for countries 1..6, US..SP (u108: name4) */
    u32 nameJp;                                 /* 0x8 the name for country 0, JP, and others (u108: name8) */
    u32 rgb;                                    /* 0xC 0x00BBGGRR (u068 / u109: rgba; u108: unk0C) */
};
EGCS_SIZE(CarColorRecord, 0x10);

/* database/carcolor.db, partial: the header. */
struct CarColorHeader {
    u8  unk_00[0x8];
    int numCars;                                /* 0x08 */
    u32 colorLists;                             /* 0x0C offset of the cars' colour lists */
    u32 colorTable;                             /* 0x10 offset of the colour table (a count word, then
                                                   CarColorRecord[count] sorted by code) */
    int size;                                   /* 0x14 the file size (int: loadFiles rounds it up with
                                                   an int mask, addiu -0x40) */
    /* CarColorEntry cars[numCars] at 0x18, sorted by code */
};

/* A car of carcolor.db, 0x10 bytes. */
struct CarColorEntry {
    u64 carCode;                                /* 0x0 */
    int numColors;                              /* 0x8 */
    u32 offset;                                 /* 0xC of its list (colour codes) in the colour lists */
};
EGCS_SIZE(CarColorEntry, 0x10);

/* No RTTI, no virtuals.  Size 8 (the .sdata object g_00352D98). */
class CarColorDataBase {
public:
    CarColorDataBase();                         /* 0x00239008 */
    ~CarColorDataBase();                        /* 0x00239018 nothing */
    int  load();                                /* 0x00239040 frees the old buffer, loads both files */
    int  loadFiles(void *buf, int size);        /* 0x00239098 1 if both files were read */
    CarColorEntry  *findCar(u64 code);          /* 0x00239140 binary search, NULL if absent */
    CarColorRecord *findColor(u32 code);        /* 0x002391A0 binary search, NULL if absent */
    int  getNumColors(u64 car);                 /* 0x00239200 0 if the car is unknown */
    u32  getColor(u64 car, int i);              /* 0x00239228 colour code i of the car, -1 if none (u32:
                                                   retail builds the -1 as lui / ori, an unsigned constant) */
    int  getColorIndex(u64 car, int color);     /* 0x002392A8 index of colour code `color`, -1 if none */
    const char *getColorName(u32 color);        /* 0x00239338 carcolor.sdb name (+0x4 for languages
                                                   1..6, else +0x8) */

    CarColorHeader  *db;                        /* 0x0 database/carcolor.db */
    /* a string table (getColorName calls StringsDataBase::getString on it) */
    StringsDataBase *strings;                   /* 0x4 database/carcolor.sdb */
};
EGCS_SIZE(CarColorDataBase, 0x8);

/* The car colour database (built / destroyed by T485's static constructor /
 * destructor, loaded by game_init, T000). */
extern CarColorDataBase g_00352D98;             /* 0x00352D98 gp-0x4C58 */

/* The static destructor / constructor of g_00352D98 (.dtors entry
 * 0x0028F25C, .ctors entry 0x0028F188) are egcs's _GLOBAL_$D$ / _GLOBAL_$I$
 * of T485 (0x00239398 / 0x002393B8; the object is defined there). */

#endif /* GAME_T485_CAR_COLOR_H */
