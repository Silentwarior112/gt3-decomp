/* T481_car_data_base.h - the car database: CarDataBase, DataBlock, StringsDataBase (translation unit T481, unit u106).
 *
 * CarDataBase holds the three files database/paramdb_us.db (the parameter
 * tables, read into a 64-byte-aligned buffer), database/paramstr_us.db and
 * database/paramunistr_us.db (string tables).  The parameter file is a
 * header (CarDataBaseHeader) with an index of tables; each table is a
 * DataBlock: fixed-size records sorted by their 64-bit code (searched by
 * bisection).  A code is the rotate-left-7 hash of a name (nameToCode).  The
 * record layouts of the tables are in T482_car_equipments.h.  One global
 * instance, g_002EBD88; CarEquipments (T482) and RaceDataBase (T484) derive
 * from CarDataBase and copy its file pointers.
 *
 * Names: the small accessors at the end of the unit are Lupo 2's class-inline
 * methods, emitted in the reverse order (CarDataBase 0x002D2E68..0x002D2FD8,
 * DataBlock 0x002D2D50..0x002D2D80, StringsDataBase 0x002D2DD8..0x002D2E20;
 * the sizes agree); the other names are the first pass's.
 *
 * The class-inline methods are defined in the classes (u106): egcs emits
 * every inline member of a class in the unit of its key function (the
 * destructor, T481), class by class before its typeinfo function and in the
 * reverse of their definition order; so each class defines them here in the
 * reverse of their addresses.  Other units inline them (no caller in
 * retail: DataBlock::search, CarDataBase::getRecord and
 * CarEquipments::findPart use getElement inline).
 */
#ifndef GAME_T481_CAR_DATA_BASE_H
#define GAME_T481_CAR_DATA_BASE_H

#include "game/game.h"

class DataBlock;
class StringsDataBase;
class CarDataBase;

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself (and rejects hand declarations). */
extern "C" {
void *__tf11CarDataBase(void);              /* 0x00231958 */
void *__tf15StringsDataBase(void);          /* 0x002319E8 */
void *__tf9DataBlock(void);                 /* 0x00231A70 */
}
#endif

/* The code of a name: s = the sum of its bytes & 0xFFFF, then for each byte
 * s = rotl64(s, 7) + byte.  Records, cars, courses and races are looked up
 * by it. */
u64 nameToCode(const char *name);                       /* 0x00231190 */

/* ? name.  The head of database/paramdb_us.db: an index of the tables.
 * Table i (a DataBlock) is at (offsets[i] & ~alignMask) + indexSize from the
 * start (CarDataBase::getDataBlock). */
struct CarDataBaseHeader {
    u32 unk_00;                             /* 0x00 (? magic: checkMagic) */
    u32 numOfFile;                          /* 0x04 the number of tables (getNumOfFile) */
    u32 indexSize;                          /* 0x08 (getIndexSize) */
    u32 alignMask;                          /* 0x0C flag bits of the index entries (getAlignMask) */
    u32 offsets[1];                         /* 0x10 [numOfFile] (getIndex) */
};

/* One table of paramdb, an overlay on the loaded file (never constructed
 * there): numElements records of elementSize bytes from +0x10, sorted by
 * the u64 code at their start.  vtable 0x00341CE0 (1 slot).  typeinfo
 * __tf9DataBlock 0x00231A70 (__user_type_info at 0x00353488, gp-0x4568).
 * vptr +0x14 (egcs).  Size 0x18. */
class DataBlock {
public:
    u32 unk_00;                             /* 0x00 (? magic: checkMagic) */
    u16 version;                            /* 0x04 getVersion */
    u16 unk_06;                             /* 0x06 */
    u16 numElements;                        /* 0x08 getNumOfElements */
    u16 elementSize;                        /* 0x0A getElementSize, getBlockSize */
    u32 blockNumber;                        /* 0x0C getBlockNumber */
    u8  elements[4];                        /* 0x10 the records (? declared size) */
    /*  vptr                                   0x14 (egcs) */

    DataBlock();                            /* 0x00231220 */
    virtual ~DataBlock();                   /* [0] +0x0C 0x00231238 */

    int   search(u64 code);                 /* 0x002316E8 bisection: the index, or -1 */
    void  checkMagic() {}                                           /* 0x00231A68 (empty) */
    u16   getVersion() { return version; }                          /* 0x00231A60 */
    u32   getBlockNumber() { return blockNumber; }                  /* 0x00231A58 */
    u16   getBlockSize() { return elementSize; }                    /* 0x00231A50 (also elementSize) */
    u16   getElementSize() { return elementSize; }                  /* 0x00231A48 */
    u16   getNumOfElements() { return numElements; }                /* 0x00231A40 */
    void *getElement(int i)                 /* 0x00231A28 record i */
    {
        u8 *top = elements;                 /* the start in its own register first, as retail */

        return top + elementSize * i;
    }
};
EGCS_SIZE(DataBlock, 0x18);

/* A string table (paramstr_us.db, ? 1 byte a character; paramunistr_us.db),
 * an overlay on the loaded file: offsets[i] -> {u16 size in bytes, the
 * characters}.  vtable 0x00341CD0 (1 slot).  typeinfo __tf15StringsDataBase
 * 0x002319E8 (__user_type_info at 0x00353498, gp-0x4558).  vptr +0x14
 * (egcs).  Size 0x18. */
class StringsDataBase {
public:
    u32 unk_00;                             /* 0x00 (? magic: checkMagic) */
    int numOfElements;                      /* 0x04 getNumOfElements */
    u16 charSize;                           /* 0x08 (? bytes per character: getLength) */
    u16 unk_0A;                             /* 0x0A */
    u32 dataBaseSize;                       /* 0x0C getDataBaseSize */
    u32 offsets[1];                         /* 0x10 [numOfElements] */
    /*  vptr                                   0x14 (egcs) */

    StringsDataBase();                      /* 0x00231268 */
    virtual ~StringsDataBase();             /* [0] +0x0C 0x00231280 */

    const char *getString(int i);           /* 0x00231740 the characters (after the size) */
    u16  getStringSize(int i);              /* 0x00231758 bytes */
    void checkMagic() {}                                              /* 0x002319E0 (empty) */
    int  getLength(int i)                   /* 0x002319A8 characters */
    {
        int size = getStringSize(i);        /* an int: retail divides signed */

        return size / charSize;
    }
    int  getNumOfElements() { return numOfElements; }                 /* 0x002319A0 */
    u32  getDataBaseSize() { return dataBaseSize; }                   /* 0x00231998 */
};
EGCS_SIZE(StringsDataBase, 0x18);

/* vtable 0x00341CC0 (1 slot).  typeinfo __tf11CarDataBase 0x00231958
 * (__user_type_info __ti11CarDataBase 0x00353490).  vptr +0xC (egcs).  Size
 * 0x10 (copy / clear move 0x10 bytes; CarEquipments' base at +0x1D0 ends at
 * +0x1E0).  The region tests read the country g_00352FD8 (gp-0x4A18,
 * T516). */
class CarDataBase {
public:
    CarDataBaseHeader *db;                  /* 0x00 database/paramdb_us.db */
    StringsDataBase   *strings;             /* 0x04 database/paramstr_us.db (getString) */
    StringsDataBase   *uniStrings;          /* 0x08 database/paramunistr_us.db (getMultiByteString) */
    /*  vptr                                   0x0C (egcs) */

    CarDataBase();                          /* 0x002312B0 clear() */
    virtual ~CarDataBase();                 /* [0] +0x0C 0x002312E8 */

    void  clear();                          /* 0x00231318 memset of the 0x10 bytes (the vptr too) */
    void  load();                           /* 0x00231338 the three files */
    void  loadStrings();                    /* 0x00231370 */
    void  loadUniStrings();                 /* 0x002313B8 */
    void *readFile(const char *path);       /* 0x00231400 the whole file, 64-byte aligned; NULL if not read */
    void  loadDataBase();                   /* 0x00231490 */
    DataBlock *getDataBlock(int table);     /* 0x00231548 */
    int   getVersion();                     /* 0x00231578 */
    int   copy(const CarDataBase &src);     /* 0x00231598 the 0x10 bytes, the vptr kept; 1 */
    void *getRecord(int table, u64 code);   /* 0x002315D8 NULL if none */
    int   getIndex(int table, u64 code);    /* 0x00231650 the record's index, -1 if none */
    int   isJapan();                        /* 0x00231680 COUNTRY_JP (prices x 100) */
    int   isUS();                           /* 0x00231690 COUNTRY_US */
    int   isUK();                           /* 0x002316A0 COUNTRY_BR (the UK) */
    int   isEurope();                       /* 0x002316B0 neither JP nor US (Europe) */
    u16   getNumOfElements(int table) { return getDataBlock(table)->numElements; }         /* 0x00231938 */
    void *getElement(int table, int i) { return getDataBlock(table)->getElement(i); }     /* 0x00231900 */
    int   searchElement(int table, u64 code) { return getDataBlock(table)->search(code); }  /* 0x002318D0 */
    int   searchElement(int table, const char *name)        /* 0x00231888 the index of nameToCode(name) */
    {
        DataBlock *block = getDataBlock(table);

        return block->search(nameToCode(name));
    }
    u32   getNumOfFile() { return db->numOfFile; }          /* 0x00231878 */
    const char *getString(int i) { return strings->getString(i); }                 /* 0x00231858 */
    u16   getStringSize(int i) { return strings->getStringSize(i); }               /* 0x00231838 */
    const char *getMultiByteString(int i) { return uniStrings->getString(i); }     /* 0x00231818 */
    u16   getMultiByteStringSize(int i) { return uniStrings->getStringSize(i); }    /* 0x002317F8 */
    u32   getIndexSize() { return db->indexSize; }          /* 0x002317E8 */
    u32   getAlignMask() { return db->alignMask; }          /* 0x002317D8 */
    u32   getIndex(int table) { return db->offsets[table]; } /* 0x002317C0 the index entry of a table */
    void  checkMagic() {}                                   /* 0x002317B8 (empty) */
};
EGCS_SIZE(CarDataBase, 0x10);

extern CarDataBase g_002EBD88;              /* 0x002EBD88 the car database */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
extern u32 __ti11CarDataBase[2];            /* 0x00353490 gp-0x4560 (__user_type_info) */
}
#endif

/* The static destructor / constructor of g_002EBD88 (.dtors entry
 * 0x0028F258, .ctors entry 0x0028F184) are egcs's _GLOBAL_$D$ / _GLOBAL_$I$
 * of T481 (0x00231770 / 0x00231798). */

#endif /* GAME_T481_CAR_DATA_BASE_H */
