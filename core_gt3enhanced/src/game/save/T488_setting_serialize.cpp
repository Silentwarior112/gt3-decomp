/* T488 - SettingSerialize: the Serialize::Base of a car setting (a
 * CarEquipments at +0x8 and a course code at +0x1F0), its registration
 * "SettingSerialize" (g_002F03F8, factory g_00352EE0: the object is
 * g_00352EE8, which setting_menu points at its local SettingSerialize), and
 * applySettings (a saved setting onto a car).  Header version 1, sub-version
 * 0x827, data 0x1D0 bytes.  0x00239930..0x00239ED0.
 *
 * Names: the virtuals are Serialize::Base's (T509); setCourseCode ..
 * getEquipements are Lupo 2's; the others describe what the listing does. */
#include "game/data/race_database_internal.h"
#include "game/menu/memorycard/T108_game_thumbnail.h"          /* gt2k_game::GameThumbnail (describe) */
#include "game/car/model/T278_show_room_car.h"            /* ShowRoomCar (describe's stack record) */
#include "game/save/T488_setting_serialize.h"

/* 0x00352EE0 (.sdata): the factory of "SettingSerialize" (extern in race_database_internal.h:
 * a namespace-scope const has internal linkage otherwise). */
const Serialize::Factory g_00352EE0 = { setting_serialize_get, setting_serialize_release };   /* 0x00352EE0 */

/* 0x002F03F8 (.data): the registration "SettingSerialize", defined here:
 * egcs's _GLOBAL_$I$ / $D$ of it are the retail static constructor /
 * destructor (the markers T488_static_init / fini below; egcs names them,
 * and the frame info's _GLOBAL_$F$, after g_00352EE0, the first object the
 * file defines). */
Serialize::ClassInfo g_002F03F8("SettingSerialize", &g_00352EE0);

/* 0x00352EE8 (.sdata, after the factory): the object the memory card
 * screens save from / load into (setting_menu points it at its local
 * SettingSerialize); zero in retail's .sdata: a C++ global without an
 * initialiser, which egcs writes as zeros at the end of the file's data. */
SettingSerialize *g_00352EE8;                   /* 0x00352EE8 */

/* Its registration. */
// FUNC 0x00239930 SettingSerialize::getClassInfo
const Serialize::ClassInfo *SettingSerialize::getClassInfo()
{
    return &g_002F03F8;
}

/* The size of the data: the CarEquipments save block. */
// FUNC 0x00239940 SettingSerialize::getDataSize
int SettingSerialize::getDataSize()
{
    return 0x1D0;               /* sizeof(CarSaveEquipments) */
}

// FUNC 0x00239948 SettingSerialize::getVersion
int SettingSerialize::getVersion()
{
    return 1;
}

// FUNC 0x00239950 SettingSerialize::getSubVersion
int SettingSerialize::getSubVersion()
{
    return 0x827;
}

/* Loads: the header is read (readHeader), then the car's save block from
 * the data after it.  Returns the start of the data, not its end (Base's
 * contract is the end; dest is not used, and the header is not checked
 * here: checkHeader is the caller's). */
// FUNC 0x00239958 SettingSerialize::load
const void *SettingSerialize::load(const void *data, void *dest)
{
    Serialize::Header header;           /* sp+0x00 */
    const u8         *body;

    body = readHeader(&header, data);
    eq.deserialize(body);
    return body;
}

/* Saves: the header into buf, then the car's save block after it; returns
 * the start of the block (not its end). */
// FUNC 0x002399A0 SettingSerialize::save
void *SettingSerialize::save(void *buf)
{
    u8 *body;

    body = writeHeader(buf);
    eq.serialize(body);
    return body;
}

/* The thumbnail of a car setting save: type 1, the car and the course of
 * the object g_00352EE8 (not this), and the car's power (PS) from a
 * ShowRoomCar built on the stack with a new condition. */
// FUNC 0x002399E0 SettingSerialize::describe
void SettingSerialize::describe(gt2k_game::GameThumbnail *thumb)
{
    ShowRoomCar car;                    /* sp+0x0000 */
    int         rpm;                    /* sp+0x1370 */

    thumb->type           = THUMBNAIL_SETTING;
    thumb->setting.car    = g_00352EE8->eq.carCode;
    thumb->setting.course = g_00352EE8->courseCode;
    {
        CarCondition condition;         /* sp+0x1350 */

        car.SetupShowRoomCar(&g_00352EE8->eq, &condition);
        thumb->setting.power = (f32)car.power(&rpm, POWER_UNIT_PS, 0);
    }
}

/* The factory of "SettingSerialize" (g_00352EE0): get = the object pointer
 * g_00352EE8. */
// FUNC 0x00239A68 setting_serialize_get
Serialize::Base *setting_serialize_get(void)
{
    return g_00352EE8;
}

/* The factory's release: nothing (the object is setting_menu's). */
// FUNC 0x00239A70 setting_serialize_release
void setting_serialize_release(Serialize::Base *obj)
{
}

/* Copies the saved setting onto `car', part group by part group: -1 if it
 * is another car; else the settings of each group whose part is the same in
 * both, and returns the number of groups that differ (transmission, drive
 * train, brake controller, racing modification, turbine kit, suspension,
 * LSD, ASCC, TCSC).  Caller: setting_menu.
 * Not exact (98.6%): four of GCSE's hoisted compare loads (car->drivetrain,
 * car->brakeController, car->racingModify, eq.racingModify) get t3..t6 in
 * another order; the reference compiler 991111-01 gives exact (its pseudo
 * numbering differs), so the source is retail's.  `if (same) copy; else
 * differ++;' gives retail's bnel with differ++ in the delay slot. */
// FUNC 0x00239A78 SettingSerialize::applySettings
int SettingSerialize::applySettings(CarEquipments *car)
{
    int differ;
    int i;

    differ = 0;
    if (car->carCode != eq.carCode)
        return -1;
    if (car->gear == eq.gear) {
        for (i = 0; i < 8; i++)
            car->gearRatio[i] = eq.gearRatio[i];
        car->finalGearRatio     = eq.finalGearRatio;
        car->maxSpeed           = eq.maxSpeed;
        car->lastFinalGearRatio = eq.lastFinalGearRatio;
    } else {
        differ = 1;
    }
    if (car->drivetrain == eq.drivetrain) {
        car->param4WD = eq.param4WD;
    } else {
        differ++;
    }
    if (car->brakeController == eq.brakeController) {
        car->frontABS = eq.frontABS;
        car->rearABS  = eq.rearABS;
    } else {
        differ++;
    }
    if (car->racingModify == eq.racingModify) {
        car->clF = eq.clF;
        car->clR = eq.clR;
    } else {
        differ++;
    }
    if (car->turbine == eq.turbine) {
        car->boost1    = eq.boost1;
        car->peekRPM1  = eq.peekRPM1;
        car->response1 = eq.response1;
        car->boost2    = eq.boost2;
        car->peekRPM2  = eq.peekRPM2;
        car->response2 = eq.response2;
    } else {
        differ++;
    }
    if (car->suspension == eq.suspension) {
        car->frontCamber     = eq.frontCamber;
        car->rearCamber      = eq.rearCamber;
        car->frontRideheight = eq.frontRideheight;
        car->rearRideheight  = eq.rearRideheight;
        car->frontToe        = eq.frontToe;
        car->rearToe         = eq.rearToe;
        car->frontSpringrate = eq.frontSpringrate;
        car->rearSpringrate  = eq.rearSpringrate;
        car->unk_164         = eq.unk_164;
        car->unk_168         = eq.unk_168;
        car->frontDamperF1B  = eq.frontDamperF1B;
        car->frontDamperF2B  = eq.frontDamperF2B;
        car->frontDamperF1R  = eq.frontDamperF1R;
        car->frontDamperF2R  = eq.frontDamperF2R;
        car->rearDamperF1B   = eq.rearDamperF1B;
        car->rearDamperF2B   = eq.rearDamperF2B;
        car->rearDamperF1R   = eq.rearDamperF1R;
        car->rearDamperF2R   = eq.rearDamperF2R;
        car->frontStabilizer = eq.frontStabilizer;
        car->rearStabilizer  = eq.rearStabilizer;
    } else {
        differ++;
    }
    if (car->lsd == eq.lsd) {
        car->frontLSDparam  = eq.frontLSDparam;
        car->rearLSDparam   = eq.rearLSDparam;
        car->frontLSDparam2 = eq.frontLSDparam2;
        car->rearLSDparam2  = eq.rearLSDparam2;
        car->frontLSDparam3 = eq.frontLSDparam3;
        car->rearLSDparam3  = eq.rearLSDparam3;
    } else {
        differ++;
    }
    if (car->ascc == eq.ascc) {
        car->settingASCC  = eq.settingASCC;
        car->asccParam[0] = eq.asccParam[0];
        car->asccParam[1] = eq.asccParam[1];
        car->asccParam[2] = eq.asccParam[2];
    } else {
        differ++;
    }
    if (car->tcsc == eq.tcsc) {
        car->settingTCSC = eq.settingTCSC;
    } else {
        differ++;
    }
    return differ;
}

/* ---- the static destructor and constructor of g_002F03F8 (egcs's
 * _GLOBAL_$D$ / _GLOBAL_$I$; retail has them here, between applySettings
 * and the accessors) ------------------------------------------------------ */

// FUNC 0x00239D38 T488_static_fini (_GLOBAL_$D$g_00352EE0)
/* the static destructor (the .dtors entry 0x0028F264): ~ClassInfo of the
 * registration with egcs flags 2 */
/* implicit: ~ClassInfo (the static object's destructor) */

// FUNC 0x00239D60 T488_static_init (_GLOBAL_$I$g_00352EE0)
/* the static constructor (the .ctors entry 0x0028F190): the registration
 * "SettingSerialize" with its factory g_00352EE0 */
/* implicit: ClassInfo (the constructor of g_002F03F8) */

/* ---- the accessors: inline members (Lupo 2's names; nothing calls them).
 * egcs writes them because the class's key function (getClassInfo) is in
 * this file, after the static destructor / constructor, first in the
 * end-of-file group (then the class's in-class inline destructor and
 * constructor and __tf16SettingSerialize); out-of-class inline members come
 * out in the reverse of their definition order, so getEquipements is
 * defined first and setCourseCode last (the frame info's FDE order,
 * 0x002F0404).  Lupo 2 has them in this order (after its typeinfo
 * function, constructor and destructor: docs/symbols/lupo2_classes.txt) -- */

// FUNC 0x00239DC0 SettingSerialize::getEquipements
inline CarEquipments *SettingSerialize::getEquipements()
{
    return &eq;
}

/* Copies `src' into the held CarEquipments (its assignment). */
// FUNC 0x00239DA0 SettingSerialize::getOtherEquipements
inline void SettingSerialize::getOtherEquipements(CarEquipments &src)
{
    eq = src;                                   /* implicit: CarEquipments::operator= */
}

// FUNC 0x00239D98 SettingSerialize::getCourseCode
inline u64 SettingSerialize::getCourseCode()
{
    return courseCode;
}

// FUNC 0x00239D90 SettingSerialize::setCourseCode
inline void SettingSerialize::setCourseCode(u64 code)
{
    courseCode = code;
}

// FUNC 0x00239DC8 SettingSerialize::~SettingSerialize
/* implicit: inline in game/save/T488_setting_serialize.h; egcs emits this copy (the one
 * the vtable holds; no callers): the vptr, CarEquipments::~CarEquipments
 * of eq (flags 2), then Serialize::Base::~Base with the caller's flags */

// FUNC 0x00239E18 SettingSerialize::SettingSerialize
/* implicit: Base, ~Base, CarEquipments; inline in game/save/T488_setting_serialize.h; egcs emits this copy (no
 * callers: u015 / u032 expand it): Serialize::Base::Base, the vptr, then
 * CarEquipments::CarEquipments of eq (on an exception ~Base); courseCode is
 * not set */

#ifndef BOOT_OLD_GCC
/* Compiler-generated: builds the __si_type_info __ti16SettingSerialize
 * (base Serialize::Base, __tiQ29Serialize4Base gp-0x4528) on the first
 * call; vtable 0x00342568 entry 0. */
// FUNC 0x00239E80 __tf16SettingSerialize
extern "C" void *__tf16SettingSerialize(void)
{
    if (__ti16SettingSerialize[0] == 0) {
        __tfQ29Serialize4Base();
        __rtti_si(__ti16SettingSerialize, "16SettingSerialize",
                  (const type_info *)__tiQ29Serialize4Base);
    }
    return __ti16SettingSerialize;
}
#endif
