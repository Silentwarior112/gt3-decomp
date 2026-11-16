/* T488_setting_serialize.h - SettingSerialize, the Serialize::Base of a car
 * setting (translation unit T488, unit u108), 0x00239930..0x00239ED0: a
 * CarEquipments and a course code; registered as "SettingSerialize"
 * (ClassInfo 0x002F03F8, factory 0x00352EE0 {setting_serialize_get,
 * setting_serialize_release}; the object is the pointer g_00352EE8).  Header version 1, sub-version 0x827, data
 * 0x1D0 bytes (CarSaveEquipments).
 *
 * The virtuals take Serialize::Base's names (T509).  Lupo 2 names:
 * setCourseCode, getCourseCode, getOtherEquipements, getEquipements. */
#ifndef GAME_T488_SETTING_SERIALIZE_H
#define GAME_T488_SETTING_SERIALIZE_H

#include "game/game.h"
#include "engine/io/T509_serialize.h"                /* base Serialize::Base */
#include "game/data/T482_car_equipments.h"           /* CarEquipments (held by value) */

class SettingSerialize;

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo function itself (and rejects a hand declaration). */
extern "C" {
void *__tf16SettingSerialize(void);         /* 0x00239E80 (__si_type_info 0x004B2A70, base Serialize::Base) */
}
#endif

/* vtable 0x00342568 (9 slots; [5] is Base::checkHeader 0x00242DA0).  typeinfo
 * __tf16SettingSerialize 0x00239E80.  Base Serialize::Base at +0x00 (vptr
 * +0x00).  Size 0x1F8 (the stack slot of load_demo_setting, u015, is 0x200).
 * The constructor and the destructor are inline in the original: u015 / u032
 * expand them; T488's copies (0x00239E18, 0x00239DC8: the one the vtable
 * holds) have no callers. */
class SettingSerialize : public Serialize::Base {
public:
    /* Base(), the vptr, the CarEquipments (courseCode is not set). */
    SettingSerialize() {}
    virtual const Serialize::ClassInfo *getClassInfo(); /* [0] +0x0C 0x00239930 0x002F03F8 */
    virtual ~SettingSerialize() {}              /* [1] +0x14 0x00239DC8 */
    virtual int  getDataSize();                 /* [2] +0x1C 0x00239940 0x1D0 */
    virtual int  getVersion();                  /* [3] +0x24 0x00239948 1 */
    virtual int  getSubVersion();               /* [4] +0x2C 0x00239950 0x827 */
    virtual const void *load(const void *data, void *dest);    /* [6] +0x3C 0x00239958 readHeader, then
                                                   eq.deserialize; returns the start of the data */
    virtual void *save(void *buf);              /* [7] +0x44 0x002399A0 writeHeader, then eq.serialize;
                                                   returns the start of the data */
    virtual void describe(gt2k_game::GameThumbnail *thumb);  /* [8] +0x4C 0x002399E0 kind 1, the
                                                   car and course codes and the power (ps) of the
                                                   object 0x00352EE8 */
    int  applySettings(CarEquipments *car);     /* 0x00239A78 the saved settings onto car: -1 if another
                                                   car, else the number of part groups that differ */
    /* The accessors are inline, defined in T488_setting_serialize.cpp (no
     * caller anywhere: retail's copies are T488's, written with the
     * end-of-file group because the key function is there). */
    inline void setCourseCode(u64 code);        /* 0x00239D90 */
    inline u64  getCourseCode();                /* 0x00239D98 */
    inline void getOtherEquipements(CarEquipments &src);    /* 0x00239DA0 eq = src (CarEquipments' assignment) */
    inline CarEquipments *getEquipements();     /* 0x00239DC0 &eq */

    /*            Serialize::Base                  0x000 (vptr 0x000) */
    CarEquipments eq;                           /* 0x008 */
    u64           courseCode;                   /* 0x1F0 */
};
EGCS_SIZE(SettingSerialize, 0x1F8);

/* The factory of "SettingSerialize" (0x00352EE0). */
Serialize::Base *setting_serialize_get(void);   /* 0x00239A68 the object pointer 0x00352EE8 */
void setting_serialize_release(Serialize::Base *obj);   /* 0x00239A70 nothing */

/* The object the memory card screens save from / load into for the car
 * setting (setting_menu, T080, points it at its local SettingSerialize
 * before F_0014AE00 modes 0x17 / 0x16; describe and the factory read it).
 * T488 .sdata (after the factory 0x00352EE0). */
extern SettingSerialize *g_00352EE8;        /* 0x00352EE8 gp-0x4B08 */

/* The static destructor / constructor of the registration g_002F03F8
 * (.dtors entry 0x0028F264, .ctors entry 0x0028F190: ClassInfo
 * ("SettingSerialize", 0x00352EE0)) are egcs's _GLOBAL_$D$ / _GLOBAL_$I$ of
 * T488 (0x00239D38 / 0x00239D60; the object is defined there). */

#endif /* GAME_T488_SETTING_SERIALIZE_H */
