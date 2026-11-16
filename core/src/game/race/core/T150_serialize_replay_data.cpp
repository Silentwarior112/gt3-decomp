/* T150 - the serialisers of the replay and ghost saves (u049):
 * Serialize::ReplayData, GhostData, TrackData, MaxSpeed.
 *
 * Each is a Serialize::Base (T509) class with one static object and a
 * registration (Serialize::ClassInfo "ReplayData", "GhostData",
 * "TrackData", "MaxSpeed", built by serialize_static_init).  They hold no
 * data: the race being saved or loaded does the work, reached through
 * g_00351F50 (the organization RaceBase::virtual_8 set) and its race:
 *   ReplayData  size virtual_10, save virtual_11, description virtual_12
 *               (a replay thumbnail); loading copies 0x41940 bytes to dest;
 *   GhostData   size virtual_16, save virtual_17, description virtual_18
 *               (a best lap thumbnail); loading virtual_19 + virtual_20;
 *   TrackData   a GhostData that loads like ReplayData (0x41940 bytes);
 *   MaxSpeed    a GhostData whose description does not set the type.
 * Header version 4, sub-version 0 for all.
 *
 * The race's virtual_12 / virtual_18 fill the replay / best lap part of the
 * GameThumbnail (T108: time +0x08, car +0x10, course +0x18, laps / power
 * +0x20, power / mass +0x24).
 *
 * Data (u049's data pass): retail's .rodata 0x0032D2C8..0x0032D4E8 is
 * "bad_alloc" (<new>, not referenced: an anchor), the four class names
 * (the .sdata pointers' literals), the vtables (MaxSpeed, TrackData,
 * GhostData, ReplayData), then the type names MaxSpeed, TrackData,
 * GhostData, ReplayData, RaceCameraBase, UnitArena<64>: UnitArena<64> (T153,
 * emitted) and RaceCameraBase (T383) defined first; no memory card file
 * list names (T108_game_thumbnail.h, not T108_memorycard_file.h).  Its
 * .sdata starts with g_00351F40, which only other units' code reads: an
 * anchor too.
 */
// DATA_START 0x0032D2C8
// DATA_START 0x00351F40
#include "types.h"
#include "libc.h"                           /* memcpy */
#include <new>                              /* "bad_alloc" (retail's first .rodata string) */
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info, __rtti_si (egcs's <new> and cxxrt.h conflict) */
#endif
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name, the last one */
#include "game/race/core/T153_race_course.h"
#include "game/camera/T383_race_camera_base.h"     /* (RaceCameraBase: its name before T150's own) */
#include "game/race/core/T150_serialize_replay_data.h"
#include "game/menu/memorycard/T108_game_thumbnail.h"       /* gt2k_game::GameThumbnail */
#include "game/race/core/T129_race_base.h"            /* RaceBase (the race's save / load virtuals) */
#include "game/race/display/race_license_internal.h"

/* The serialisers' Factories and registrations (defined below; extern
 * first: a namespace-scope const has internal linkage otherwise). */
extern const Serialize::Factory g_00351F48; /* 0x00351F48 gp-0x5AA8 {replay_data_get, replay_data_release} */
extern const Serialize::Factory g_00351F58; /* 0x00351F58 gp-0x5A98 {ghost_data_get, ghost_data_release} */
extern const Serialize::Factory g_00351F68; /* 0x00351F68 gp-0x5A88 {track_data_get, track_data_release} */
extern const Serialize::Factory g_00351F78; /* 0x00351F78 gp-0x5A78 {max_speed_get, max_speed_release} */
extern Serialize::ClassInfo g_002BA7D8;     /* 0x002BA7D8 "ReplayData" */
extern Serialize::ClassInfo g_002BA7E8;     /* 0x002BA7E8 "GhostData" */
extern Serialize::ClassInfo g_002BA7F8;     /* 0x002BA7F8 "TrackData" */
extern Serialize::ClassInfo g_002BA808;     /* 0x002BA808 "MaxSpeed" */

/* ------------------------------------------------------------------------ */
/* The static data, in the original order: .sdata each class's name (read   */
/* by the memory card code) and Factory, and the organization; .data the    */
/* registrations (constructed by egcs's static initialiser, which also     */
/* builds the objects); .sbss the objects: file statics (a public object    */
/* would be written in place, zero-filled in .sdata; retail has them in     */
/* .sbss in reverse order, as egcs writes uninitialised statics at the end */
/* of the unit)                                                             */
/* ------------------------------------------------------------------------ */

const char *g_00351F40 = "ReplayData";                                      /* 0x00351F40 */
const Serialize::Factory g_00351F48 = { replay_data_get, replay_data_release };   /* 0x00351F48 */
static Serialize::ReplayData g_00353248;                                           /* 0x00353248 */
Serialize::ClassInfo g_002BA7D8("ReplayData", &g_00351F48);                 /* 0x002BA7D8 */
RaceOrganization *g_00351F50 = NULL;                                        /* 0x00351F50 */

const char *g_00351F54 = "GhostData";                                       /* 0x00351F54 */
const Serialize::Factory g_00351F58 = { ghost_data_get, ghost_data_release };     /* 0x00351F58 */
static Serialize::GhostData g_00353240;                                            /* 0x00353240 */
Serialize::ClassInfo g_002BA7E8("GhostData", &g_00351F58);                  /* 0x002BA7E8 */

const char *g_00351F60 = "TrackData";                                       /* 0x00351F60 */
const Serialize::Factory g_00351F68 = { track_data_get, track_data_release };     /* 0x00351F68 */
static Serialize::TrackData g_00353238;                                            /* 0x00353238 */
Serialize::ClassInfo g_002BA7F8("TrackData", &g_00351F68);                  /* 0x002BA7F8 */

const char *g_00351F70 = "MaxSpeed";                                        /* 0x00351F70 */
const Serialize::Factory g_00351F78 = { max_speed_get, max_speed_release };       /* 0x00351F78 */
static Serialize::MaxSpeed g_00353230;                                             /* 0x00353230 */
Serialize::ClassInfo g_002BA808("MaxSpeed", &g_00351F78);                   /* 0x002BA808 */


/* ------------------------------------------------------------------------ */
/* ReplayData                                                               */
/* ------------------------------------------------------------------------ */

// FUNC 0x0018D4F8 Serialize::ReplayData::getClassInfo
//   (index.txt: virtual_0) Its registration.
const Serialize::ClassInfo *Serialize::ReplayData::getClassInfo()
{
    return &g_002BA7D8;
}

// FUNC 0x0018D508 replay_data_get
//   ? name.  Factory::get of "ReplayData" (g_00351F48): the static object.
Serialize::Base *replay_data_get(void)
{
    return &g_00353248;
}

// FUNC 0x0018D518 replay_data_release
//   ? name.  Factory::release of "ReplayData" (g_00351F48+4): nothing.
void replay_data_release(Serialize::Base *obj)
{
}

// FUNC 0x0018D520 Serialize::ReplayData::ReplayData
//   Base (0x00242BF0), then the vptr.  Caller: serialize_static_init.
Serialize::ReplayData::ReplayData()
{
    /* implicit: Base (base constructor) */
}

// FUNC 0x0018D558 Serialize::ReplayData::~ReplayData
//   (index.txt: virtual_1) The vptr, then ~Base with the caller's flags
//   (the deleting form is there).
Serialize::ReplayData::~ReplayData()
{
    /* implicit: ~Base (the base destructor, with the caller's flags) */
}

// FUNC 0x0018D580 Serialize::ReplayData::getDataSize
//   (index.txt: virtual_2) The race's replay data size (virtual_10).
int Serialize::ReplayData::getDataSize()
{
    return g_00351F50->race->virtual_10();
}

// FUNC 0x0018D5B0 Serialize::ReplayData::getVersion
//   (index.txt: virtual_3)
int Serialize::ReplayData::getVersion()
{
    return 4;
}

// FUNC 0x0018D5B8 Serialize::ReplayData::getSubVersion
//   (index.txt: virtual_4)
int Serialize::ReplayData::getSubVersion()
{
    return 0;
}

// FUNC 0x0018D5C0 Serialize::ReplayData::load
//   (index.txt: virtual_6) The header is read, checked (checkHeader,
//   through the vtable; 2, a version error, is refused: NULL), then 0x41940
//   bytes of the data are copied to dest; the end of the data is returned.
const void *Serialize::ReplayData::load(const void *data, void *dest)
{
    Header header;                          /* sp+0x00 */
    const void *top = data;                 /* retail: data's register is reused for the body */
    data = readHeader(&header, data);

    if (checkHeader(top, dest) == 2)
        return NULL;
    memcpy(dest, data, REPLAY_DATA_LOAD_SIZE);
    return (const u8 *)data + header.dataSize;
}

// FUNC 0x0018D660 Serialize::ReplayData::save
//   (index.txt: virtual_7) The header into buf, then the race writes its
//   replay data (virtual_11) after it; returns the end of that block (its
//   first word is its size).
void *Serialize::ReplayData::save(void *buf)
{
    u8 *body = writeHeader(buf);
    g_00351F50->race->virtual_11((RaceInfoBlock *)body);
    return body + *(u32 *)body;
}

// FUNC 0x0018D6B0 Serialize::ReplayData::describe
//   (index.txt: virtual_8) The thumbnail of the save: type 2 (a replay),
//   the rest by the race (virtual_12).
void Serialize::ReplayData::describe(gt2k_game::GameThumbnail *thumb)
{
    thumb->type = MC_THUMBNAIL_REPLAY;
    g_00351F50->race->virtual_12(thumb);
}

/* ------------------------------------------------------------------------ */
/* GhostData                                                                */
/* ------------------------------------------------------------------------ */

// FUNC 0x0018D6F0 Serialize::GhostData::getClassInfo
//   (index.txt: virtual_0) Its registration.
const Serialize::ClassInfo *Serialize::GhostData::getClassInfo()
{
    return &g_002BA7E8;
}

// FUNC 0x0018D700 ghost_data_get
//   ? name.  Factory::get of "GhostData" (g_00351F58).
Serialize::Base *ghost_data_get(void)
{
    return &g_00353240;
}

// FUNC 0x0018D710 ghost_data_release
//   ? name.  Factory::release of "GhostData": nothing.
void ghost_data_release(Serialize::Base *obj)
{
}

// FUNC 0x0018D718 Serialize::GhostData::GhostData
//   Base, then the vptr.  Callers: TrackData's, MaxSpeed's constructors,
//   serialize_static_init.
Serialize::GhostData::GhostData()
{
    /* implicit: Base (base constructor) */
}

// FUNC 0x0018D750 Serialize::GhostData::~GhostData
//   (index.txt: virtual_1) The vptr, then ~Base with the caller's flags.
//   Callers: TrackData's, MaxSpeed's destructors, serialize_static_fini.
Serialize::GhostData::~GhostData()
{
    /* implicit: ~Base (the base destructor, with the caller's flags) */
}

// FUNC 0x0018D778 Serialize::GhostData::getDataSize
//   (index.txt: Serialize::MaxSpeed::virtual_2; slot 2 of all three ghost
//   classes) The race's ghost data size (virtual_16).
int Serialize::GhostData::getDataSize()
{
    return g_00351F50->race->virtual_16();
}

// FUNC 0x0018D7A8 Serialize::GhostData::getVersion
//   (index.txt: Serialize::MaxSpeed::virtual_3)
int Serialize::GhostData::getVersion()
{
    return 4;
}

// FUNC 0x0018D7B0 Serialize::GhostData::getSubVersion
//   (index.txt: Serialize::MaxSpeed::virtual_4)
int Serialize::GhostData::getSubVersion()
{
    return 0;
}

// FUNC 0x0018D7B8 Serialize::GhostData::load
//   (index.txt: Serialize::MaxSpeed::virtual_6; slot 6 of GhostData and
//   MaxSpeed) The header is read and checked (2 refused: NULL), then the
//   race reads the ghost (virtual_19, virtual_20); the end of the data is
//   returned.  dest is not used (but passed to checkHeader in $a2).
const void *Serialize::GhostData::load(const void *data, void *dest)
{
    Header header;                          /* sp+0x00 */
    const void *top = data;                 /* retail: data's register is reused for the body */
    data = readHeader(&header, data);

    if (checkHeader(top, dest) == 2)
        return NULL;
    RaceBase *race = g_00351F50->race;
    race->virtual_19((RaceInfoBlock *)data);
    race->virtual_20((RaceInfoBlock *)data);
    return (const u8 *)data + header.dataSize;
}

// FUNC 0x0018D878 Serialize::GhostData::save
//   (index.txt: Serialize::MaxSpeed::virtual_7; slot 7 of all three)
//   The header, then the race's ghost data (virtual_17); returns the end of
//   that block.
void *Serialize::GhostData::save(void *buf)
{
    u8 *body = writeHeader(buf);
    g_00351F50->race->virtual_17((RaceInfoBlock *)body);
    return body + *(u32 *)body;
}

// FUNC 0x0018D8C8 Serialize::GhostData::describe
//   (index.txt: Serialize::TrackData::virtual_8; slot 8 of GhostData and
//   TrackData) The thumbnail: type 3 (a best lap), the rest by the race
//   (virtual_18).
void Serialize::GhostData::describe(gt2k_game::GameThumbnail *thumb)
{
    thumb->type = MC_THUMBNAIL_BESTLAP;
    g_00351F50->race->virtual_18(thumb);
}

/* ------------------------------------------------------------------------ */
/* TrackData                                                                */
/* ------------------------------------------------------------------------ */

// FUNC 0x0018D908 Serialize::TrackData::getClassInfo
//   (index.txt: virtual_0) Its registration.
const Serialize::ClassInfo *Serialize::TrackData::getClassInfo()
{
    return &g_002BA7F8;
}

// FUNC 0x0018D918 track_data_get
//   ? name.  Factory::get of "TrackData" (g_00351F68).
Serialize::Base *track_data_get(void)
{
    return &g_00353238;
}

// FUNC 0x0018D928 track_data_release
//   ? name.  Factory::release of "TrackData": nothing.
void track_data_release(Serialize::Base *obj)
{
}

// FUNC 0x0018D930 Serialize::TrackData::TrackData
//   GhostData, then the vptr.
Serialize::TrackData::TrackData()
{
    /* implicit: GhostData (base constructor) */
}

// FUNC 0x0018D968 Serialize::TrackData::~TrackData
//   (index.txt: virtual_1) The vptr, then ~GhostData with the caller's
//   flags.
Serialize::TrackData::~TrackData()
{
    /* implicit: ~GhostData (the base destructor, with the caller's flags) */
}

// FUNC 0x0018D990 Serialize::TrackData::load
//   (index.txt: virtual_6) As ReplayData's: header, check (2 refused:
//   NULL), 0x41940 bytes to dest; the end of the data.
const void *Serialize::TrackData::load(const void *data, void *dest)
{
    Header header;                          /* sp+0x00 */
    const void *top = data;                 /* retail: data's register is reused for the body */
    data = readHeader(&header, data);

    if (checkHeader(top, dest) == 2)
        return NULL;
    memcpy(dest, data, REPLAY_DATA_LOAD_SIZE);
    return (const u8 *)data + header.dataSize;
}

/* ------------------------------------------------------------------------ */
/* MaxSpeed                                                                 */
/* ------------------------------------------------------------------------ */

// FUNC 0x0018DA30 Serialize::MaxSpeed::getClassInfo
//   (index.txt: virtual_0) Its registration.
const Serialize::ClassInfo *Serialize::MaxSpeed::getClassInfo()
{
    return &g_002BA808;
}

// FUNC 0x0018DA40 max_speed_get
//   ? name.  Factory::get of "MaxSpeed" (g_00351F78).
Serialize::Base *max_speed_get(void)
{
    return &g_00353230;
}

// FUNC 0x0018DA50 max_speed_release
//   ? name.  Factory::release of "MaxSpeed": nothing.
void max_speed_release(Serialize::Base *obj)
{
}

// FUNC 0x0018DA58 Serialize::MaxSpeed::MaxSpeed
//   GhostData, then the vptr.
Serialize::MaxSpeed::MaxSpeed()
{
    /* implicit: GhostData (base constructor) */
}

// FUNC 0x0018DA90 Serialize::MaxSpeed::~MaxSpeed
//   (index.txt: virtual_1) The vptr, then ~GhostData with the caller's
//   flags.
Serialize::MaxSpeed::~MaxSpeed()
{
    /* implicit: ~GhostData (the base destructor, with the caller's flags) */
}

// FUNC 0x0018DAB8 Serialize::MaxSpeed::describe
//   (index.txt: virtual_8) The thumbnail by the race (virtual_18), without
//   setting the type.
void Serialize::MaxSpeed::describe(gt2k_game::GameThumbnail *thumb)
{
    g_00351F50->race->virtual_18(thumb);
}

/* ------------------------------------------------------------------------ */
/* The organization, the static objects                                     */
/* ------------------------------------------------------------------------ */

// FUNC 0x0018DAE8 serialize_set_organization
//   The organization the serialisers
//   work on.  Caller: RaceBase::virtual_8.
void serialize_set_organization(RaceOrganization *org)
{
    g_00351F50 = org;
}

// FUNC 0x0018DAF0 serialize_static_fini (_GLOBAL_$D$g_00351F40)
//   egcs's static destructor (.dtors entry 0x0028F214): the registrations
//   and objects in the reverse order of their construction (each
//   destructor with flags 2).
/* implicit: ~ClassInfo, ~MaxSpeed, ~TrackData, ~GhostData, ~ReplayData */

// FUNC 0x0018DB78 serialize_static_init (_GLOBAL_$I$g_00351F40)
//   egcs's static initialiser (.ctors entry 0x0028F124): each static
//   object, then its registration (name and factory).
/* implicit: ReplayData, GhostData, TrackData, MaxSpeed, ClassInfo (the constructors) */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
// FUNC 0x0018DC20 __tfQ29Serialize8MaxSpeed
//   Builds the __si_type_info at 0x004B2100 (base: GhostData, 0x004B20E0)
//   on the first call.
extern "C" const type_info &__tfQ29Serialize8MaxSpeed(void)
{
    if (__tiQ29Serialize8MaxSpeed[0] == 0) {
        __tfQ29Serialize9GhostData();
        __rtti_si(__tiQ29Serialize8MaxSpeed, "Q29Serialize8MaxSpeed",
                  (const type_info *)__tiQ29Serialize9GhostData);
    }
    return *(const type_info *)__tiQ29Serialize8MaxSpeed;
}

// FUNC 0x0018DC70 __tfQ29Serialize9TrackData
//   Builds the __si_type_info at 0x004B2110 (base: GhostData, 0x004B20E0)
//   on the first call.
extern "C" const type_info &__tfQ29Serialize9TrackData(void)
{
    if (__tiQ29Serialize9TrackData[0] == 0) {
        __tfQ29Serialize9GhostData();
        __rtti_si(__tiQ29Serialize9TrackData, "Q29Serialize9TrackData",
                  (const type_info *)__tiQ29Serialize9GhostData);
    }
    return *(const type_info *)__tiQ29Serialize9TrackData;
}

// FUNC 0x0018DCC0 __tfQ29Serialize9GhostData
//   Builds the __si_type_info at 0x004B20E0 (base: Serialize::Base,
//   gp-0x4528) on the first call.
extern "C" const type_info &__tfQ29Serialize9GhostData(void)
{
    if (__tiQ29Serialize9GhostData[0] == 0) {
        __tfQ29Serialize4Base();
        __rtti_si(__tiQ29Serialize9GhostData, "Q29Serialize9GhostData",
                  (const type_info *)__tiQ29Serialize4Base);
    }
    return *(const type_info *)__tiQ29Serialize9GhostData;
}
#endif

// FUNC 0x0018DD10 Serialize::ReplayData::getRace
/* inline in game/race/core/T150_serialize_replay_data.h; egcs emits this copy (no callers; u049
 * serialize_race): g_00351F50->race */

// FUNC 0x0018DD20 Serialize::ReplayData::getOrganization
/* inline in game/race/core/T150_serialize_replay_data.h; egcs emits this copy (no callers; u049
 * serialize_organization): g_00351F50 */

// FUNC 0x0018DD28 Serialize::ReplayData::setOrganization
/* inline in game/race/core/T150_serialize_replay_data.h; egcs emits this copy (no callers; u049
 * serialize_set_organization_inline): g_00351F50 = org */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x0018DD30 __tfQ29Serialize10ReplayData
//   Builds the __si_type_info at 0x004B20F0 (base: Serialize::Base,
//   gp-0x4528) on the first call.
extern "C" const type_info &__tfQ29Serialize10ReplayData(void)
{
    if (__tiQ29Serialize10ReplayData[0] == 0) {
        __tfQ29Serialize4Base();
        __rtti_si(__tiQ29Serialize10ReplayData, "Q29Serialize10ReplayData",
                  (const type_info *)__tiQ29Serialize4Base);
    }
    return *(const type_info *)__tiQ29Serialize10ReplayData;
}
#endif
