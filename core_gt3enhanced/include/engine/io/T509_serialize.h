/* T509_serialize.h - Serialize: Base and the registry of serialisable classes
 * (translation unit T509, unit u111), 0x00242B38..0x002430E0.
 *
 * Each serialisable class (ReplayData, GhostData, TrackData, MaxSpeed in T150,
 * GameStatus T429, SettingSerialize T488) registers a ClassInfo {name,
 * factory} at static-construction time; serialize_create(name) gets the
 * object through the factory.  A save is the Header (0x10 bytes: header size,
 * data size, sub-version, version, crc) and the name of Base's registration
 * ("Base") padded to headerSize (64-aligned), then the data, byte-inverted,
 * with a GZIP::crc32 of the inverted bytes in the header. */
#ifndef GAME_T509_SERIALIZE_H
#define GAME_T509_SERIALIZE_H

#include "game/game.h"

namespace gt2k_game {
struct GameThumbnail;                           /* T108 (pointers only) */
}

namespace Serialize {
class Base;

/* (? name) How the registry reaches the object of a class (ClassInfo::
 * factory).  Not polymorphic, 8 bytes (Base's is 0x00352FC8; T150's are
 * 0x00351F48 / 58 / 68 / 78). */
struct Factory {
    Base *(*get)(void);                         /* 0x0 the object (NULL: none) */
    void  (*release)(Base *obj);                /* 0x4 gives it back (nothing for the static ones) */
};

/* (? name) The header in front of serialised data (Base::writeHeader /
 * readHeader).  Not polymorphic, 0x10 bytes. */
struct Header {
    u32 headerSize;                             /* 0x0 the data starts here */
    u32 dataSize;                               /* 0x4 getDataSize() */
    u16 subVersion;                             /* 0x8 getSubVersion() */
    u16 version;                                /* 0xA getVersion() */
    u32 crc;                                    /* 0xC GZIP::crc32 of the (inverted) data */
};

/* A registered serialisable class; the constructor links it at the head of
 * the registry (0x00352FC0, gp-0x4A30); it is never unlinked.  Not
 * polymorphic.  The members end at 0xC (the static ones are 0x10 apart:
 * egcs aligns data objects to 8). */
class ClassInfo {
public:
    ClassInfo(const char *name, const Factory *f);  /* 0x00242B88 */
    ~ClassInfo();                               /* 0x00242BA8 nothing */

    ClassInfo     *next;                        /* 0x0 */
    const char    *name;                        /* 0x4 the class's name ("Base", "ReplayData", ...) */
    const Factory *factory;                     /* 0x8 how to get / give back its object */
};

/* The base of the serialisable classes.  vtable 0x00343BE8 (9 slots),
 * typeinfo __tfQ29Serialize4Base 0x002430A0 (__user_type_info 0x003534C8,
 * gp-0x4528).  No data: vptr +0x00, size 4 (the derived classes' data starts
 * at +0x04). */
class Base {
public:
    Base();                                     /* 0x00242BF0 */
    virtual const ClassInfo *getClassInfo();    /* [0] +0x0C 0x00242BD0 its registration (Base: 0x002F3F18 "Base") */
    virtual ~Base();                            /* [1] +0x14 0x00242C08 */
    virtual int  getDataSize() = 0;             /* [2] +0x1C the bytes of the data */
    virtual int  getVersion() = 0;              /* [3] +0x24 */
    virtual int  getSubVersion() = 0;           /* [4] +0x2C */
    /* 0 the versions match, 1 the sub-version differs, 2 the version differs;
     * dest is not read (every caller passes the dest of its load / restore) */
    virtual int  checkHeader(const void *data, void *dest); /* [5] +0x34 0x00242DA0 */
    virtual const void *load(const void *data, void *dest) = 0;    /* [6] +0x3C the end of the data */
    virtual void *save(void *buf) = 0;          /* [7] +0x44 the end of what it wrote */
    /* fills the thumbnail (the description) of a save of it: word 0 the
     * kind (ReplayData: 2) */
    virtual void describe(gt2k_game::GameThumbnail *thumb) = 0;  /* [8] +0x4C */

    int   saveSize();                           /* 0x00242C38 headerSize() + getDataSize(), rounded up to 64 */
    int   headerSize();                         /* 0x00242C90 (strlen("Base") + 0x50) & ~0x3F, whatever the object
                                                   (a member: writeHeader keeps this in $a0 for the call) */
    u8   *writeHeader(void *buf);               /* 0x00242CC0 zeroes and fills the header; where the data goes */
    const u8 *readHeader(Header *hdr, const void *data);   /* 0x00242D70 copies the header; where the data starts */
    const void *restore(const void *data, void *dest);     /* 0x00242E18 unseal + load; NULL: bad checksum */
    void *serialize(void *buf);                 /* 0x00242E80 save + seal; what save returned */
    int   unseal(void *data);                   /* 0x00242EE0 crc ok: the data inverted back, 1; else 0 */
    void  seal(void *buf);                      /* 0x00242F60 inverts the data, stores its crc */
};
} /* namespace Serialize */
EGCS_SIZE(Serialize::Factory, 8);
EGCS_SIZE(Serialize::Header, 0x10);
EGCS_SIZE(Serialize::Base, 0x4);

/* Base's factory (0x00352FC8 {base_factory_get, base_factory_release}):
 * Base is abstract, there is no object. */
Serialize::Base *base_factory_get(void);        /* 0x00242BE0 NULL */
void base_factory_release(Serialize::Base *obj);    /* 0x00242BE8 nothing */
/* The object of the class registered as className (its factory's get),
 * NULL if there is no such class. */
Serialize::Base *serialize_create(const char *className);   /* 0x00242FC0 */
void serialize_destroy(Serialize::Base *obj);   /* 0x00243000 obj->getClassInfo()->factory->release(obj) */
/* 0x00243048 (.dtors 0x0028F27C) / 0x00243070 (.ctors 0x0028F1AC): egcs's
 * static destructor and initialiser of Base's registration g_002F3F18
 * (~ClassInfo / ClassInfo("Base", &g_00352FC8)), compiler-generated
 * (T509 defines the object). */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
class type_info;
extern "C" const type_info &__tfQ29Serialize4Base(void);    /* 0x002430A0 */
/* Base's __user_type_info {vptr, name}, built by __tfQ29Serialize4Base.
 * (u049 / u108 / u111: g_003534C8) */
extern "C" u32 __tiQ29Serialize4Base[2];        /* 0x003534C8 gp-0x4528 */
#endif

#endif /* GAME_T509_SERIALIZE_H */
