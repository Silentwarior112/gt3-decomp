/* T150_serialize_replay_data.h - the serialisers of the replay and ghost
 * saves: Serialize::ReplayData, GhostData, TrackData, MaxSpeed (translation
 * unit T150, unit u049), 0x0018D4F8..0x0018DD80.
 *
 * Each is a Serialize::Base (T509) class with one static object (ReplayData
 * 0x00353248, GhostData 0x00353240, TrackData 0x00353238, MaxSpeed
 * 0x00353230) and a registration (ClassInfo 0x002BA7D8 "ReplayData",
 * 0x002BA7E8 "GhostData", 0x002BA7F8 "TrackData", 0x002BA808 "MaxSpeed",
 * with the Factories 0x00351F48 / 58 / 68 / 78; serialize_static_init).
 * They hold no data: the race being saved or loaded does the work, reached
 * through the organization g_00351F50 (serialize_set_organization, set by
 * RaceBase::virtual_8) and its race:
 *   ReplayData  size RaceBase::virtual_10, save virtual_11, description
 *               virtual_12 (kind 2); loading copies 0x41940 bytes to dest;
 *   GhostData   size virtual_16, save virtual_17, description virtual_18
 *               (kind 3); loading virtual_19 + virtual_20;
 *   TrackData   a GhostData that loads like ReplayData (0x41940 bytes);
 *   MaxSpeed    a GhostData whose description does not set the kind.
 * Header version 4, sub-version 0 for all (Serialize::Factory and Header:
 * T509). */
#ifndef GAME_T150_SERIALIZE_REPLAY_DATA_H
#define GAME_T150_SERIALIZE_REPLAY_DATA_H

#include "game/game.h"
#include "engine/io/T509_serialize.h"                /* base Serialize::Base, ClassInfo, Factory, Header */
#include "game/race/core/T125_race_organization.h"        /* RaceOrganization (ReplayData::getRace) */

class RaceBase;                                 /* T129 (pointers only) */
namespace gt2k_game {
struct GameThumbnail;                           /* T108 (pointers only) */
}
class type_info;                                /* T548 */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
extern "C" {
/* (__si_type_info 0x004B2100, base GhostData 0x004B20E0) */
const type_info &__tfQ29Serialize8MaxSpeed(void);      /* 0x0018DC20 */
/* (__si_type_info 0x004B2110, base GhostData 0x004B20E0) */
const type_info &__tfQ29Serialize9TrackData(void);     /* 0x0018DC70 */
/* (__si_type_info 0x004B20E0, base Serialize::Base 0x003534C8) */
const type_info &__tfQ29Serialize9GhostData(void);     /* 0x0018DCC0 */
/* (__si_type_info 0x004B20F0, base Serialize::Base 0x003534C8) */
const type_info &__tfQ29Serialize10ReplayData(void);   /* 0x0018DD30 */
}
#endif

/* The organization the serialisers work on (T150's .sdata; set by
 * serialize_set_organization). */
extern RaceOrganization *g_00351F50;        /* 0x00351F50 gp-0x5AA0 */

namespace Serialize {

/* vtable 0x0032D408 (9 slots; all but 5 its own), typeinfo
 * __tfQ29Serialize10ReplayData 0x0018DD30.  A replay save.  No data: vptr
 * +0x00 (Base's), 4 bytes. */
class ReplayData : public Base {
public:
    ReplayData();                               /* 0x0018D520 */
    virtual const ClassInfo *getClassInfo();    /* [0] +0x0C 0x0018D4F8 "ReplayData" (0x002BA7D8) */
    virtual ~ReplayData();                      /* [1] +0x14 0x0018D558 */
    virtual int  getDataSize();                 /* [2] +0x1C 0x0018D580 the race's virtual_10 */
    virtual int  getVersion();                  /* [3] +0x24 0x0018D5B0 4 */
    virtual int  getSubVersion();               /* [4] +0x2C 0x0018D5B8 0 */
    /* The header read and checked (checkHeader 2: NULL), then 0x41940
     * bytes of data copied to dest; the end of the data. */
    virtual const void *load(const void *data, void *dest);     /* [6] +0x3C 0x0018D5C0 */
    /* The header, then the race's virtual_11 after it; the end of that
     * block. */
    virtual void *save(void *buf);              /* [7] +0x44 0x0018D660 */
    /* type 2 (a replay), then the race's virtual_12. */
    virtual void describe(gt2k_game::GameThumbnail *thumb);   /* [8] +0x4C 0x0018D6B0 */

    /* (? names; u049: serialize_race, serialize_organization,
     * serialize_set_organization_inline) The organization the serialisers
     * work on (g_00351F50) and its race.  Inline: T150's out-of-line
     * copies have no callers and sit right before
     * __tfQ29Serialize10ReplayData (egcs emits a class's inline members
     * with its typeinfo function, in reverse declaration order; defined
     * out of the class they come before all the typeinfo functions).
     * serialize_set_organization (0x0018DAE8) has setOrganization's code. */
    static void setOrganization(RaceOrganization *org) { g_00351F50 = org; } /* 0x0018DD28 */
    static RaceOrganization *getOrganization() { return g_00351F50; }      /* 0x0018DD20 */
    static RaceBase *getRace() { return g_00351F50->race; }               /* 0x0018DD10 */
};
EGCS_SIZE(Serialize::ReplayData, 4);

/* vtable 0x0032D3B8 (9 slots; all but 5 its own; index.txt names slots
 * 2..4, 6, 7 MaxSpeed's and slot 8 TrackData's: they are GhostData's,
 * inherited by both), typeinfo __tfQ29Serialize9GhostData 0x0018DCC0.  A
 * ghost save.  No data: vptr +0x00 (Base's), 4 bytes. */
class GhostData : public Base {
public:
    GhostData();                                /* 0x0018D718 */
    virtual const ClassInfo *getClassInfo();    /* [0] +0x0C 0x0018D6F0 "GhostData" (0x002BA7E8) */
    virtual ~GhostData();                       /* [1] +0x14 0x0018D750 */
    virtual int  getDataSize();                 /* [2] +0x1C 0x0018D778 the race's virtual_16 */
    virtual int  getVersion();                  /* [3] +0x24 0x0018D7A8 4 */
    virtual int  getSubVersion();               /* [4] +0x2C 0x0018D7B0 0 */
    /* The header read and checked (checkHeader 2: NULL), then the race's
     * virtual_19 and virtual_20 on the data; the end of the data. */
    virtual const void *load(const void *data, void *dest);     /* [6] +0x3C 0x0018D7B8 */
    /* The header, then the race's virtual_17 after it. */
    virtual void *save(void *buf);              /* [7] +0x44 0x0018D878 */
    /* type 3 (a ghost), then the race's virtual_18. */
    virtual void describe(gt2k_game::GameThumbnail *thumb);   /* [8] +0x4C 0x0018D8C8 */
};
EGCS_SIZE(Serialize::GhostData, 4);

/* vtable 0x0032D368 (9 slots; 0, 1 and 6 its own), typeinfo
 * __tfQ29Serialize9TrackData 0x0018DC70.  A GhostData that loads like
 * ReplayData.  4 bytes. */
class TrackData : public GhostData {
public:
    TrackData();                                /* 0x0018D930 */
    virtual const ClassInfo *getClassInfo();    /* [0] +0x0C 0x0018D908 "TrackData" (0x002BA7F8) */
    virtual ~TrackData();                       /* [1] +0x14 0x0018D968 */
    /* As ReplayData's: 0x41940 bytes of data copied to dest. */
    virtual const void *load(const void *data, void *dest);     /* [6] +0x3C 0x0018D990 */
};
EGCS_SIZE(Serialize::TrackData, 4);

/* vtable 0x0032D318 (9 slots; 0, 1 and 8 its own), typeinfo
 * __tfQ29Serialize8MaxSpeed 0x0018DC20.  4 bytes. */
class MaxSpeed : public GhostData {
public:
    MaxSpeed();                                 /* 0x0018DA58 */
    virtual const ClassInfo *getClassInfo();    /* [0] +0x0C 0x0018DA30 "MaxSpeed" (0x002BA808) */
    virtual ~MaxSpeed();                        /* [1] +0x14 0x0018DA90 */
    /* The race's virtual_18 only (the type is not set). */
    virtual void describe(gt2k_game::GameThumbnail *thumb);   /* [8] +0x4C 0x0018DAB8 */
};
EGCS_SIZE(Serialize::MaxSpeed, 4);

} /* namespace Serialize */

/* The Factories of the four classes: get returns the static object,
 * release does nothing.  (? names) */
Serialize::Base *replay_data_get(void);             /* 0x0018D508 */
void replay_data_release(Serialize::Base *obj);     /* 0x0018D518 */
Serialize::Base *ghost_data_get(void);              /* 0x0018D700 */
void ghost_data_release(Serialize::Base *obj);      /* 0x0018D710 */
Serialize::Base *track_data_get(void);              /* 0x0018D918 */
void track_data_release(Serialize::Base *obj);      /* 0x0018D928 */
Serialize::Base *max_speed_get(void);               /* 0x0018DA40 */
void max_speed_release(Serialize::Base *obj);       /* 0x0018DA50 */
/* The organization the serialisers work on (g_00351F50).  Caller:
 * RaceBase::virtual_8 (u043).  (u049 F_0018DAE8.) */
void serialize_set_organization(RaceOrganization *org);    /* 0x0018DAE8 */
/* .dtors entry 0x0028F214: the registrations and objects, in reverse. */
void serialize_static_fini(void);                   /* 0x0018DAF0 */
/* .ctors entry 0x0028F124: each static object, then its registration. */
void serialize_static_init(void);                   /* 0x0018DB78 */

/* The class names of the serialisers, T150's .sdata: each name pointer is
 * followed by its class's Factory (0x00351F40 "ReplayData", Factory
 * 0x00351F48; 0x00351F54 "GhostData", 0x00351F58; 0x00351F60 "TrackData",
 * 0x00351F68; 0x00351F70 "MaxSpeed", 0x00351F78).  The memory card code
 * looks the save classes up by them: "ReplayData" / "GhostData" (u026 /
 * u028 / u030), "TrackData" (the logger's load, u030) and "MaxSpeed"
 * (MemorycardControl::begin, u026).  (The plan had put them in T068.) */
extern const char *g_00351F40;              /* 0x00351F40 gp-0x5AB0 -> "ReplayData" */
extern const char *g_00351F54;              /* 0x00351F54 gp-0x5A9C -> "GhostData" */
extern const char *g_00351F60;              /* 0x00351F60 gp-0x5A90 -> "TrackData" */
extern const char *g_00351F70;              /* 0x00351F70 gp-0x5A80 -> "MaxSpeed" */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
/* The __si_type_info nodes {vptr, name, base} the typeinfo functions fill
 * (u049: g_004B20E0 .. g_004B2110). */
extern "C" u32 __tiQ29Serialize9GhostData[3];   /* 0x004B20E0 */
extern "C" u32 __tiQ29Serialize10ReplayData[3]; /* 0x004B20F0 */
extern "C" u32 __tiQ29Serialize8MaxSpeed[3];    /* 0x004B2100 */
extern "C" u32 __tiQ29Serialize9TrackData[3];   /* 0x004B2110 */
#endif

#endif /* GAME_T150_SERIALIZE_REPLAY_DATA_H */
