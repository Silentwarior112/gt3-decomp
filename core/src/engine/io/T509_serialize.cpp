/* T509 - Serialize: Base and the registry of serialisable classes (u111).
 * 0x00242B38..0x002430E0.
 *
 * Each serialisable class (ReplayData, GhostData, TrackData, MaxSpeed,
 * GameStatus, SettingSerialize) registers a ClassInfo {name, factory} at
 * static-construction time; serialize_create(name) gets the object through
 * the factory.  A save is Header (0x10 bytes) + the name of Base's
 * registration ("Base") padded to headerSize (64-aligned), then the data,
 * byte-inverted, with a GZIP::crc32 of the inverted bytes in the header.
 */
#include "engine/io/file_loader_internal.h"
#include "engine/io/T509_serialize.h"
#include "runtime/cxx/T548_tinfo.h"            /* __rtti_user (the typeinfo function at the end) */

/* checkHeader's results */
enum { HEADER_OK = 0, HEADER_SUBVERSION_DIFFERS = 1, HEADER_VERSION_DIFFERS = 2 };

/* ------------------------------------------------------------------------ */
/* The registry                                                             */
/* ------------------------------------------------------------------------ */

// FUNC 0x00242B38 serialize_find_class
//   ? name.  The registration named name, NULL if none.
Serialize::ClassInfo *serialize_find_class(const char *name)
{
    Serialize::ClassInfo *info;

    for (info = g_00352FC0; info != NULL; info = info->next)
        if (strcmp(info->name, name) == 0)
            break;
    return info;
}

// FUNC 0x00242B88 Serialize::ClassInfo::ClassInfo
//   Links it at the head of the registry.  Callers: the static constructors
//   of T150 (0x0018DB78), T429 (0x0021A4B0), T488 (0x00239D60) and this
//   unit.
Serialize::ClassInfo::ClassInfo(const char *name, const Factory *f)
{
    this->name = name;
    next = g_00352FC0;
    factory = f;
    g_00352FC0 = this;
}

// FUNC 0x00242BA8 Serialize::ClassInfo::~ClassInfo
//   Nothing (not unlinked); the deleting form calls operator delete.
Serialize::ClassInfo::~ClassInfo()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
}

/* ------------------------------------------------------------------------ */
/* Base                                                                     */
/* ------------------------------------------------------------------------ */

// FUNC 0x00242BD0 Serialize::Base::getClassInfo
//   Its registration, g_002F3F18 ("Base").
const Serialize::ClassInfo *Serialize::Base::getClassInfo()
{
    return &g_002F3F18;
}

// FUNC 0x00242BE0 base_factory_get
//   Factory::get of "Base" (g_00352FC8): no object (abstract).
Serialize::Base *base_factory_get(void)
{
    return NULL;
}

// FUNC 0x00242BE8 base_factory_release
//   Factory::release of "Base" (g_00352FC8 + 4): nothing.
void base_factory_release(Serialize::Base *obj)
{
}

// FUNC 0x00242BF0 Serialize::Base::Base
//   The vptr (0x00343BE8).  Callers: the constructors of the derived
//   classes.
Serialize::Base::Base()
{
}

// FUNC 0x00242C08 Serialize::Base::~Base
//   The vptr; the deleting form calls operator delete.
Serialize::Base::~Base()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x00242C38 Serialize::Base::saveSize
//   headerSize() + getDataSize(), rounded up to 64.  Callers: 0x00149A00,
//   0x0015BB78.
int Serialize::Base::saveSize()
{
    int hdrSize = headerSize();

    return (hdrSize + getDataSize() + 0x3F) & ~0x3F;
}

// FUNC 0x00242C90 Serialize::Base::headerSize
//   (strlen(Base's registered name) + 0x50) & ~0x3F: the 0x10-byte header
//   and the name with its NUL, rounded up to 64 (0x40 for "Base").  Always
//   Base's name (g_002F3F18), whatever the object.
int Serialize::Base::headerSize()
{
    return ((int)strlen(g_002F3F18.name) + 0x50) & ~0x3F;
}

// FUNC 0x00242CC0 Serialize::Base::writeHeader
//   Zeroes headerSize() bytes of buf, fills the header (size, getDataSize,
//   getSubVersion, getVersion) and the name; returns where the data goes.
//   Callers: the save methods (slot 7) of the derived classes.
u8 *Serialize::Base::writeHeader(void *buf)
{
    Header *hdr = (Header *)buf;
    int hdrSize = headerSize();

    memset(buf, 0, hdrSize);
    hdr->headerSize = hdrSize;
    hdr->dataSize = getDataSize();
    hdr->subVersion = (u16)getSubVersion();
    hdr->version = (u16)getVersion();
    strcpy((char *)(hdr + 1), g_002F3F18.name);
    return (u8 *)buf + hdr->headerSize;
}

// FUNC 0x00242D70 Serialize::Base::readHeader
//   Copies the 16-byte header (unaligned) into *hdr; returns where the data
//   starts.  Callers: the load methods (slot 6) and checkHeader.
const u8 *Serialize::Base::readHeader(Header *hdr, const void *data)
{
    *hdr = *(const Header *)data;
    return (const u8 *)data + ((const Header *)data)->headerSize;
}

// FUNC 0x00242DA0 Serialize::Base::checkHeader
//   (slot 5 of every derived class) 2 if the version differs from
//   getVersion(), else 1 if the sub-version differs from getSubVersion(),
//   else 0.  dest is not read (the callers pass the dest of their load /
//   restore in $a2).
int Serialize::Base::checkHeader(const void *data, void *dest)
{
    Header hdr;                                 /* sp+0x00 */

    readHeader(&hdr, data);
    if (hdr.version != getVersion())
        return HEADER_VERSION_DIFFERS;
    if (hdr.subVersion != getSubVersion())
        return HEADER_SUBVERSION_DIFFERS;
    return HEADER_OK;
}

// FUNC 0x00242E18 Serialize::Base::restore
//   unseal (crc check + inversion) the data, then load (slot 6) it into
//   dest; NULL if the checksum is wrong.  Callers: the memory-card load
//   menus (0x0014F440, 0x00154708, 0x00157198, 0x001583D8).
const void *Serialize::Base::restore(const void *data, void *dest)
{
    if (!unseal((void *)data))
        return NULL;
    return load(data, dest);
}

// FUNC 0x00242E80 Serialize::Base::serialize
//   save (slot 7) into buf, then seal it; returns what save returned.
//   Caller: 0x00149A00.
void *Serialize::Base::serialize(void *buf)
{
    void *end = save(buf);

    seal(buf);
    return end;
}

// FUNC 0x00242EE0 Serialize::Base::unseal
//   If the header's crc is GZIP::crc32 of the dataSize bytes after the
//   header, inverts them back (~) and returns 1; else 0 (left as is).
int Serialize::Base::unseal(void *data)
{
    Header *hdr = (Header *)data;
    u8 *start = (u8 *)data + hdr->headerSize;

    if (hdr->crc != GZIP::crc32(start, hdr->dataSize))
        return 0;

    u8 *p = start;
    u8 *end = p + hdr->dataSize;

    for (; p != end; p++)
        *p = ~*p;
    return 1;
}

// FUNC 0x00242F60 Serialize::Base::seal
//   Inverts (~) the dataSize bytes after the header, then stores their
//   GZIP::crc32 in the header.
void Serialize::Base::seal(void *buf)
{
    Header *hdr = (Header *)buf;
    u8 *start = (u8 *)buf + hdr->headerSize;
    u8 *end = start + hdr->dataSize;
    u8 *p;

    for (p = start; p != end; p++)
        *p = ~*p;
    hdr->crc = GZIP::crc32(start, hdr->dataSize);
}

/* ------------------------------------------------------------------------ */
/* Objects by name                                                          */
/* ------------------------------------------------------------------------ */

// FUNC 0x00242FC0 serialize_create
//   The object of the class registered as className (its factory's get),
//   NULL if there is no such class.
Serialize::Base *serialize_create(const char *className)
{
    Serialize::ClassInfo *info = serialize_find_class(className);
    Serialize::Base *obj = NULL;

    if (info != NULL)
        obj = info->factory->get();
    return obj;
}

// FUNC 0x00243000 serialize_destroy
//   Hands obj back to its class's factory (release).
void serialize_destroy(Serialize::Base *obj)
{
    obj->getClassInfo()->factory->release(obj);
}

/* The registry and Base's factory (.sdata, initialised: written here, the
 * registry first; "Base" follows them, written with the static
 * initialiser). */
Serialize::ClassInfo *g_00352FC0 = NULL;        /* 0x00352FC0 gp-0x4A30 */
Serialize::Factory g_00352FC8 = { base_factory_get, base_factory_release };    /* 0x00352FC8 gp-0x4A28 */

/* Base's registration (.data 0x002F3F18, at the end of T509's data: egcs
 * puts a public object that has a constructor in .data). */
Serialize::ClassInfo g_002F3F18("Base", &g_00352FC8);

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_002F3F18 (.dtors entry 0x0028F27C, .ctors entry 0x0028F1AC), emitted at
 * the end of the unit, the destructor first: ~ClassInfo (flags 2) /
 * ClassInfo("Base", &g_00352FC8) on it. */
// FUNC 0x00243048 T509_static_dtor (_GLOBAL_$D$serialize_find_class__FPCc)
/* implicit: ~ClassInfo (the static destructor of g_002F3F18) */

// FUNC 0x00243070 T509_static_init (_GLOBAL_$I$serialize_find_class__FPCc)
/* implicit: ClassInfo (the static initialiser of g_002F3F18) */

#ifndef BOOT_OLD_GCC
// FUNC 0x002430A0 __tfQ29Serialize4Base
//   Compiler-generated typeinfo function: builds the __user_type_info
//   __tiQ29Serialize4Base (gp-0x4528) on the first call (__rtti_user: no
//   bases).
extern "C" const type_info &__tfQ29Serialize4Base(void)
{
    void *node = __tiQ29Serialize4Base;

    if (__tiQ29Serialize4Base[0] == 0)
        __rtti_user(node, "Q29Serialize4Base");
    return *(const type_info *)node;
}
#endif
