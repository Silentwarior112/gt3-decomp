/* T481_car_data_base.cpp - the car database: CarDataBase, its parameter
 * tables (DataBlock) and string tables (StringsDataBase); unit u106,
 * 0x00231190..0x00231AB0. */
#include "game/data/car_equipments_internal.h"
#include "game/data/T481_car_data_base.h"

/* 0x002EBD88 (.data): the car database, defined here (u106: egcs's
 * _GLOBAL_$I$ / $D$ of it are the retail static constructor / destructor,
 * the markers T481_static_init / fini below). */
CarDataBase g_002EBD88;

/* The 64-bit code of a name: the sum of its bytes (low 16 bits), then each
 * byte added after a rotate left by 7.
 * Matching (91.7%): sum is reused for the rotate's high bits (retail's $s0
 * holds both; it is set before strlen, so it crosses the call), the loops
 * count up (reversed to count-downs) over a pointer.  Left: retail sets each
 * loop's pointer copy (a0 = name) after the loop's entry test (blez), as a
 * loop preheader does, here it is before it and fills the test's delay slot;
 * index forms (name[i], p[i]) keep i and are not reversed, block-scoped /
 * second pointers and the declaration orders tried do not move it. */
// FUNC 0x00231190 nameToCode
u64 nameToCode(const char *name)
{
    u64 sum = 0;
    int len = strlen(name);
    u64 code;
    const u8 *p;
    int i;
    int j;

    p = (const u8 *)name;
    for (i = 0; i < len; i++)
        sum += *p++;
    code = sum & 0xFFFF;
    p = (const u8 *)name;
    for (j = 0; j < len; j++) {
        sum = code >> 57;
        code <<= 7;
        code |= sum;
        code += *p++;
    }
    return code;
}

/* ---- the constructors and destructors ---------------------------------- */

/* DataBlock and StringsDataBase are only overlays on the loaded files: their
 * constructors store the vptr and are never called. */
// FUNC 0x00231220 DataBlock::DataBlock
DataBlock::DataBlock()
{
}

// FUNC 0x00231238 DataBlock::~DataBlock
DataBlock::~DataBlock()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* the vptr; the deleting form (flags & 1) calls operator delete(this) */
}

// FUNC 0x00231268 StringsDataBase::StringsDataBase
StringsDataBase::StringsDataBase()
{
}

// FUNC 0x00231280 StringsDataBase::~StringsDataBase
StringsDataBase::~StringsDataBase()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* the vptr; the deleting form (flags & 1) calls operator delete(this) */
}

/* The vptr, then clear(). */
// FUNC 0x002312B0 CarDataBase::CarDataBase
CarDataBase::CarDataBase()
{
    clear();
}

/* The files are not freed. */
// FUNC 0x002312E8 CarDataBase::~CarDataBase
CarDataBase::~CarDataBase()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* the vptr; the deleting form (flags & 1) calls operator delete(this) */
}

/* memset of the whole object: it also zeroes the vptr the constructor has
 * just stored (the original's quirk; CarEquipments stores its own after). */
// FUNC 0x00231318 CarDataBase::clear
void CarDataBase::clear()
{
    memset((void *)this, 0, 0x10);      /* sizeof(CarDataBase) */
}

// FUNC 0x00231338 CarDataBase::load
void CarDataBase::load()
{
    loadDataBase();
    loadStrings();
    loadUniStrings();
}

// FUNC 0x00231370 CarDataBase::loadStrings
void CarDataBase::loadStrings()
{
    if (this->strings != NULL)
        free(this->strings);
    this->strings = (StringsDataBase *)readFile("database/paramstr_us.db");
}

// FUNC 0x002313B8 CarDataBase::loadUniStrings
void CarDataBase::loadUniStrings()
{
    if (this->uniStrings != NULL)
        free(this->uniStrings);
    this->uniStrings = (StringsDataBase *)readFile("database/paramunistr_us.db");
}

/* The whole file (unpacked) in a 64-byte-aligned buffer of its size rounded
 * up to 64; NULL if it could not be read (the buffer is not freed then).
 * The size is an int before the rounding (retail's mask is addiu -0x40; on
 * the u32 result it would be lui / ori 0xFFFFFFC0). */
// FUNC 0x00231400 CarDataBase::readFile
void *CarDataBase::readFile(const char *path)
{
    int size = file_size_retry(path, 1, &vol_device_object);
    size = (size + 0x3F) & ~0x3F;
    void *buf = memalign(0x40, size);

    if (file_read_blocking(path, buf, size, 1, &vol_device_object) == NULL)
        buf = NULL;
    return buf;
}

/* As readFile, inline; on a failed read db keeps the pointer it had (freed
 * above when non-NULL) and the buffer leaks. */
// FUNC 0x00231490 CarDataBase::loadDataBase
void CarDataBase::loadDataBase()
{
    const char *path;
    int size;
    void *buf;

    if (this->db != NULL)
        free(this->db);
    path = "database/paramdb_us.db";
    size = file_size_retry(path, 1, &vol_device_object);
    size = (size + 0x3F) & ~0x3F;
    buf = memalign(0x40, size);
    if (file_read_blocking(path, buf, size, 1, &vol_device_object) != NULL) {
        this->db = (CarDataBaseHeader *)buf;
        getVersion();                   /* the result is not used */
    }
}

// FUNC 0x00231548 CarDataBase::getDataBlock
DataBlock *CarDataBase::getDataBlock(int table)
{
    CarDataBaseHeader *header = this->db;

    return (DataBlock *)((u8 *)header
                         + ((header->offsets[table] & ~header->alignMask) + header->indexSize));
}

/* The version of table 0. */
// FUNC 0x00231578 CarDataBase::getVersion
int CarDataBase::getVersion()
{
    return getDataBlock(0)->version;
}

/* The implicit assignment (the 0x10 bytes, the vptr kept), then 1. */
// FUNC 0x00231598 CarDataBase::copy
int CarDataBase::copy(const CarDataBase &src)
{
    *this = src;
    return 1;
}

// FUNC 0x002315D8 CarDataBase::getRecord
void *CarDataBase::getRecord(int table, u64 code)
{
    int index = getDataBlock(table)->search(code);

    if (index == -1)
        return NULL;
    return getDataBlock(table)->getElement(index);
}

// FUNC 0x00231650 CarDataBase::getIndex
int CarDataBase::getIndex(int table, u64 code)
{
    return getDataBlock(table)->search(code);
}

/* The region tests read the country (T516). */
// FUNC 0x00231680 CarDataBase::isJapan
int CarDataBase::isJapan()
{
    return g_00352FD8.value == COUNTRY_JP;
}

// FUNC 0x00231690 CarDataBase::isUS
int CarDataBase::isUS()
{
    return g_00352FD8.value == COUNTRY_US;
}

// FUNC 0x002316A0 CarDataBase::isUK
int CarDataBase::isUK()
{
    return g_00352FD8.value == COUNTRY_BR;
}

/* Neither Japan nor the US (the European countries). */
// FUNC 0x002316B0 CarDataBase::isEurope
int CarDataBase::isEurope()
{
    if (isUS())
        return 0;
    return !isJapan();
}

/* Bisection over the records (sorted by their leading u64 code, compared
 * unsigned); the index, or -1.  With no records it still reads record -1
 * once. */
// FUNC 0x002316E8 DataBlock::search
int DataBlock::search(u64 code)
{
    int hi = this->numElements;
    int lo = -1;

    do {
        int mid = (lo + hi) >> 1;
        u64 key = *(u64 *)getElement(mid);

        if (key == code)
            return mid;
        if (key < code)
            lo = mid;
        else
            hi = mid;
    } while (lo + 1 != hi);
    return -1;
}

/* String i: offsets[i] -> {u16 size, the characters}. */
// FUNC 0x00231740 StringsDataBase::getString
const char *StringsDataBase::getString(int i)
{
    return (const char *)this + this->offsets[i] + 2;
}

// FUNC 0x00231758 StringsDataBase::getStringSize
u16 StringsDataBase::getStringSize(int i)
{
    return *(u16 *)((u8 *)this + this->offsets[i]);
}

/* ---- the static destructor and constructor of g_002EBD88 (egcs's
 * _GLOBAL_$D$ / _GLOBAL_$I$, emitted after the functions) ---------------- */

// FUNC 0x00231770 T481_static_fini (_GLOBAL_$D$g_002EBD88)
/* the static destructor (the .dtors entry 0x0028F258): ~CarDataBase on
 * g_002EBD88 with egcs flags 2 */
/* implicit: ~CarDataBase (the static object's destructor) */

// FUNC 0x00231798 T481_static_init (_GLOBAL_$I$g_002EBD88)
/* the static constructor (the .ctors entry 0x0028F184): CarDataBase::
 * CarDataBase on g_002EBD88 */
/* implicit: CarDataBase (the static object's constructor) */

/* ---- the class-inline methods (Lupo 2): egcs emits every inline member
 * of a class in the unit of its key function (the destructor), each class's
 * before its typeinfo function, in the reverse of their definition order
 * (game/data/T481_car_data_base.h); none has a caller ------------------------- */

// FUNC 0x002317B8 CarDataBase::checkMagic (checkMagic__11CarDataBase)
/* inline in game/data/T481_car_data_base.h */

// FUNC 0x002317C0 CarDataBase::getIndex (getIndex__11CarDataBasei)
/* inline in game/data/T481_car_data_base.h */

// FUNC 0x002317D8 CarDataBase::getAlignMask (getAlignMask__11CarDataBase)
/* inline in game/data/T481_car_data_base.h */

// FUNC 0x002317E8 CarDataBase::getIndexSize (getIndexSize__11CarDataBase)
/* inline in game/data/T481_car_data_base.h */

// FUNC 0x002317F8 CarDataBase::getMultiByteStringSize (getMultiByteStringSize__11CarDataBasei)
/* implicit: getStringSize (inline in game/data/T481_car_data_base.h: uniStrings->getStringSize(i)) */

// FUNC 0x00231818 CarDataBase::getMultiByteString (getMultiByteString__11CarDataBasei)
/* implicit: getString (inline in game/data/T481_car_data_base.h: uniStrings->getString(i)) */

// FUNC 0x00231838 CarDataBase::getStringSize (getStringSize__11CarDataBasei)
/* implicit: getStringSize (inline in game/data/T481_car_data_base.h: strings->getStringSize(i)) */

// FUNC 0x00231858 CarDataBase::getString (getString__11CarDataBasei)
/* implicit: getString (inline in game/data/T481_car_data_base.h: strings->getString(i)) */

// FUNC 0x00231878 CarDataBase::getNumOfFile (getNumOfFile__11CarDataBase)
/* inline in game/data/T481_car_data_base.h */

// FUNC 0x00231888 CarDataBase::searchElement (searchElement__11CarDataBaseiPCc)
/* implicit: getDataBlock, nameToCode, search (inline in game/data/T481_car_data_base.h) */

// FUNC 0x002318D0 CarDataBase::searchElement (searchElement__11CarDataBaseiUx)
/* implicit: getDataBlock, search (inline in game/data/T481_car_data_base.h) */

// FUNC 0x00231900 CarDataBase::getElement (getElement__11CarDataBaseii)
/* implicit: getDataBlock (inline in game/data/T481_car_data_base.h; DataBlock::getElement inlined) */

// FUNC 0x00231938 CarDataBase::getNumOfElements (getNumOfElements__11CarDataBasei)
/* implicit: getDataBlock (inline in game/data/T481_car_data_base.h) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function: builds the __user_type_info
 * __ti11CarDataBase (name 0x00341CF0) on the first call. */
// FUNC 0x00231958 __tf11CarDataBase
extern "C" void *__tf11CarDataBase(void)
{
    if (__ti11CarDataBase[0] == 0)
        __rtti_user(__ti11CarDataBase, "11CarDataBase");
    return __ti11CarDataBase;
}
#endif

// FUNC 0x00231998 StringsDataBase::getDataBaseSize (getDataBaseSize__15StringsDataBase)
/* inline in game/data/T481_car_data_base.h */

// FUNC 0x002319A0 StringsDataBase::getNumOfElements (getNumOfElements__15StringsDataBase)
/* inline in game/data/T481_car_data_base.h */

// FUNC 0x002319A8 StringsDataBase::getLength (getLength__15StringsDataBasei)
/* implicit: getStringSize (inline in game/data/T481_car_data_base.h) */

// FUNC 0x002319E0 StringsDataBase::checkMagic (checkMagic__15StringsDataBase)
/* inline in game/data/T481_car_data_base.h */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__ti15StringsDataBase, name
 * 0x00341D00). */
// FUNC 0x002319E8 __tf15StringsDataBase
extern "C" void *__tf15StringsDataBase(void)
{
    if (__ti15StringsDataBase[0] == 0)
        __rtti_user(__ti15StringsDataBase, "15StringsDataBase");
    return __ti15StringsDataBase;
}
#endif

// FUNC 0x00231A28 DataBlock::getElement (getElement__9DataBlocki)
/* inline in game/data/T481_car_data_base.h */

// FUNC 0x00231A40 DataBlock::getNumOfElements (getNumOfElements__9DataBlock)
/* inline in game/data/T481_car_data_base.h */

// FUNC 0x00231A48 DataBlock::getElementSize (getElementSize__9DataBlock)
/* inline in game/data/T481_car_data_base.h */

// FUNC 0x00231A50 DataBlock::getBlockSize (getBlockSize__9DataBlock)
/* inline in game/data/T481_car_data_base.h */

// FUNC 0x00231A58 DataBlock::getBlockNumber (getBlockNumber__9DataBlock)
/* inline in game/data/T481_car_data_base.h */

// FUNC 0x00231A60 DataBlock::getVersion (getVersion__9DataBlock)
/* inline in game/data/T481_car_data_base.h */

// FUNC 0x00231A68 DataBlock::checkMagic (checkMagic__9DataBlock)
/* inline in game/data/T481_car_data_base.h */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__ti9DataBlock, name 0x00341D18). */
// FUNC 0x00231A70 __tf9DataBlock
extern "C" void *__tf9DataBlock(void)
{
    if (__ti9DataBlock[0] == 0)
        __rtti_user(__ti9DataBlock, "9DataBlock");
    return __ti9DataBlock;
}
#endif
