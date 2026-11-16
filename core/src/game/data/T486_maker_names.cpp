/* T486 - the maker names: g_002EFF20, the car makers as the game's file and
 * logo names spell them ("acura" .. "volkswagen", 46, alphabetical;
 * CarEquipments::getMakerName, indexed by CarRecord.maker), and g_002EFFD8,
 * the same list with the tuners ("mines", "mugen", "nismo", "spoon", "toms",
 * "trd": 52; CarEquipments::getBrandName, indexed by CarRecord.brand).
 *
 * A translation unit without code, linked between T485 and T487: retail's
 * object is its .data (the two tables, 0x002EFF20..0x002F00A8) and its frame
 * info (collect2's table entry 0x002F00A8: the CIE and the 0 terminator, no
 * FDE), its .rodata (0x00342428..0x003424F3: the names of more than 8 bytes)
 * and its .sdata (0x00352DA0..0x00352ED4: the names of 8 bytes or less; the
 * file is compiled -G8, ref/compile_flags.json).  egcs writes a .data
 * table's literals just before it, in table order, split by size, and a
 * repeated literal once: the second table adds only the six tuners' names.
 * The extern declarations are in car_equipments_internal.h (T482's getMakerName /
 * getBrandName).  Only the tables point at the names: the anchors below
 * open the file's .rodata and .sdata. */
// DATA_TU frame 0x002F00A8
// DATA_START 0x00342428
// DATA_START 0x00352DA0
#include "types.h"

const char *g_002EFF20[46] = {                  /* 0x002EFF20 */
    "acura",            /* 0x00 */
    "alfaromeo",        /* 0x01 */
    "astonmartin",      /* 0x02 */
    "audi",             /* 0x03 */
    "bmw",              /* 0x04 */
    "chevrolet",        /* 0x05 */
    "chrysler",         /* 0x06 */
    "citroen",          /* 0x07 */
    "daihatsu",         /* 0x08 */
    "dodge",            /* 0x09 */
    "fiat",             /* 0x0A */
    "ford",             /* 0x0B */
    "gillet",           /* 0x0C */
    "honda",            /* 0x0D */
    "infiniti",         /* 0x0E */
    "jaguar",           /* 0x0F */
    "lancia",           /* 0x10 */
    "lexus",            /* 0x11 */
    "lister",           /* 0x12 */
    "lotus",            /* 0x13 */
    "mazda",            /* 0x14 */
    "mercedes",         /* 0x15 */
    "mercury",          /* 0x16 */
    "mg_mini",          /* 0x17 */
    "mitsubishi",       /* 0x18 */
    "nissan",           /* 0x19 */
    "opel",             /* 0x1A */
    "pagani",           /* 0x1B */
    "panoz",            /* 0x1C */
    "peugeot",          /* 0x1D */
    "plymoth",          /* 0x1E */
    "polyphony",        /* 0x1F */
    "renault",          /* 0x20 */
    "rover",            /* 0x21 */
    "ruf",              /* 0x22 */
    "shelby",           /* 0x23 */
    "subaru",           /* 0x24 */
    "suzuki",           /* 0x25 */
    "tickford",         /* 0x26 */
    "tommykaira",       /* 0x27 */
    "toyota",           /* 0x28 */
    "tvr",              /* 0x29 */
    "vauxhall",         /* 0x2A */
    "vector",           /* 0x2B */
    "venturi",          /* 0x2C */
    "volkswagen",       /* 0x2D */
};

const char *g_002EFFD8[52] = {                  /* 0x002EFFD8 */
    "acura",            /* 0x00 */
    "alfaromeo",        /* 0x01 */
    "astonmartin",      /* 0x02 */
    "audi",             /* 0x03 */
    "bmw",              /* 0x04 */
    "chevrolet",        /* 0x05 */
    "chrysler",         /* 0x06 */
    "citroen",          /* 0x07 */
    "daihatsu",         /* 0x08 */
    "dodge",            /* 0x09 */
    "fiat",             /* 0x0A */
    "ford",             /* 0x0B */
    "gillet",           /* 0x0C */
    "honda",            /* 0x0D */
    "infiniti",         /* 0x0E */
    "jaguar",           /* 0x0F */
    "lancia",           /* 0x10 */
    "lexus",            /* 0x11 */
    "lister",           /* 0x12 */
    "lotus",            /* 0x13 */
    "mazda",            /* 0x14 */
    "mercedes",         /* 0x15 */
    "mercury",          /* 0x16 */
    "mg_mini",          /* 0x17 */
    "mines",            /* 0x18 */
    "mitsubishi",       /* 0x19 */
    "mugen",            /* 0x1A */
    "nismo",            /* 0x1B */
    "nissan",           /* 0x1C */
    "opel",             /* 0x1D */
    "pagani",           /* 0x1E */
    "panoz",            /* 0x1F */
    "peugeot",          /* 0x20 */
    "plymoth",          /* 0x21 */
    "polyphony",        /* 0x22 */
    "renault",          /* 0x23 */
    "rover",            /* 0x24 */
    "ruf",              /* 0x25 */
    "shelby",           /* 0x26 */
    "spoon",            /* 0x27 */
    "subaru",           /* 0x28 */
    "suzuki",           /* 0x29 */
    "tickford",         /* 0x2A */
    "tommykaira",       /* 0x2B */
    "toms",             /* 0x2C */
    "toyota",           /* 0x2D */
    "trd",              /* 0x2E */
    "tvr",              /* 0x2F */
    "vauxhall",         /* 0x30 */
    "vector",           /* 0x31 */
    "venturi",          /* 0x32 */
    "volkswagen",       /* 0x33 */
};
