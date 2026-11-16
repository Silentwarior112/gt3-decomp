/* T121_race_entry.h - RaceEntryInformation, RaceEntryCar, RaceEntryBase, the
 * voices of a car's sound (translation unit T121, unit u042),
 * 0x00177108..0x00179468.
 *
 * A RaceEntryCar is one car of a race: its car data (RaceEntryInformation =
 * CarEquipments + the id of the input that drives it), its lap times and
 * records, its model (RaceCarModel, T180), its sound (CarSound, T176), the
 * input RaceBase::virtual_26 gave it, a few per-entry options, the driver
 * name and the date of the record.  RaceEntryBase is the list of the entries
 * (RaceOrganization +0x18): it links them by distance to a point (the draw
 * order) and by the cars' rank byte.  Every entry and list is allocated from
 * the race pool g_002B3BF0 (T123).
 *
 * The RaceEntryCar model / sound wrappers forward to RaceCarModel (T180,
 * u060's names) and CarSound (T176, u059's names); their arguments pass
 * through in the registers (the callers in T130 / T131 set them), so they
 * are declared with them although the first pass wrote them without.
 *
 * Also emitted here: CarSound's compiler-generated copy assignment
 * (operator=, 0x00179230, for copyFrom's `sound = src->sound').
 *
 * Not here although the plan put them in T121: CarSoundEngine and
 * CarSoundSlot (0x168 / 0x1C bytes, CarSound's members: a CarSoundPan, then
 * a GtSoundVoice + EngineSound / a GtSoundHandle).  They hold EngineSound
 * (T176) by value and CarSound (T176) holds them by value, while
 * RaceEntryCar here holds CarSound by value: with them in T121, T121 and
 * T176 would have to include each other.  They belong to T176 with CarSound
 * (see ref/pass2/groups/h23/notes.md).
 */
#ifndef GAME_T121_RACE_ENTRY_H
#define GAME_T121_RACE_ENTRY_H

#include "game/game.h"
#include <stddef.h>                         /* size_t */
#include "game/data/T482_car_equipments.h"       /* base CarEquipments */
#include "game/race/core/T126_race_lap_time.h"        /* RaceLapTime (held by value) */
#include "game/race/core/T127_race_records.h"         /* TimeRecord (held by value) */
#include "game/race/core/T128_race_lap_record.h"      /* LapRecord (held by value) */
#include "game/car/model/T180_race_car_model.h"       /* RaceCarModel (held by value) */
#include "game/sound/T176_car_sound.h"            /* CarSound (held by value) */
#include "game/car/physics/T280_car_condition.h"        /* CarCondition (held by value) */
#include "game/car/model/T456_car_model_file.h"       /* CarModelFile (getModelInfo: model.data->info) */

class type_info;
class RaceCarState;                         /* T302 (pointers only: forward declarations) */
class RaceInput;                            /* T413 */
class CarModel;                             /* T446 */
class CameraManager;                        /* T383 */
class ParticleManager;                      /* T165 */
class FlareClass;                           /* T159 */

/* The typeinfo functions (vtable entry 0), declared before the classes, for
 * GCC 14 only: egcs generates them itself (u042 matching: an earlier
 * declaration is the one egcs uses, and it then emits no __tf function;
 * without it T121 emits the three, and no other unit's code changes). */
#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
const type_info &__tf13RaceEntryBase(void);         /* 0x00178DD8 (__user_type_info 0x00353368) */
const type_info &__tf12RaceEntryCar(void);          /* 0x00179188 (__user_type_info 0x00353360) */
/* the __class_type_info at 0x004B1E70 (one base, CarEquipments: the base
 * list 0x00351DD8) */
const type_info &__tf20RaceEntryInformation(void);  /* 0x001791D8 */
}
#endif

/* ------------------------------------------------------------------------ */
/* The voices of a car's sound                                              */
/* ------------------------------------------------------------------------ */

/* One voice of a CarSoundFrame: where it sounds (relative to the listener),
 * how loud, its pitch.  Size 0x18 (u042, u059). */
struct CarSoundVoice {
    f32 pos[4];                             /* 0x00 relative to the listener */
    f32 volume;                             /* 0x10 */
    f32 pitch;                              /* 0x14 (the Doppler factor and the voice's own) */
};
EGCS_SIZE(CarSoundVoice, 0x18);

/* An engine voice: a CarSoundVoice and the engine parameter.  Size 0x1C. */
struct CarSoundVoiceEx {
    f32 pos[4];                             /* 0x00 */
    f32 volume;                             /* 0x10 */
    f32 pitch;                              /* 0x14 */
    f32 param;                              /* 0x18 (the engine rpm, u059) */
};
EGCS_SIZE(CarSoundVoiceEx, 0x1C);

/* The voices of one car for one frame: what RaceEntryCar::updateSound builds
 * on its stack and hands CarSound::update (0x001A6C38, T176).  u042 / u059
 * call it CarSoundParams, a name T288 has for another class (h17).  Size
 * 0x37C (u042 / u059). */
struct CarSoundFrame {
    int             onboard;                /* 0x000 the current view looks from this car */
    f32             pos[4];                 /* 0x004 the car, relative to the listener */
    f32             scale;                  /* 0x014 1.0 */
    /* [0], [1] the engine (param: the rpm), [2] the wind (car_sound_wind;
     * param: its pitch).  u042: engine[2] and the voice at +0x50. */
    CarSoundVoiceEx engine[3];              /* 0x018 */
    /* [0] the gear whine, [1] the turbo, [2] the road, [3] silent (volume
     * 0), then seven per wheel: [4 + 7 wheel + k] (the T287 evaluators).
     * The effects CarSound plays for them (u059, T469's table): [0..3]
     * "turbine2", "turbine", "in_car", "body_noiz"; per wheel "off_skill",
     * "on_skill", "off_roadnoiz", "rumble", "off_skill", "on_road",
     * "rain_noiz". */
    CarSoundVoice   voice[32];              /* 0x06C */
    /* The volumes of the sounds 13, 15, 16, 17 (u059): [0] "b_off" (? the
     * blow-off, DynCarState blowOffLevel), [1 + kind] the loudest crash of
     * the frame ("clash", "clash2", "clash3"). */
    f32             oneShot[4];             /* 0x36C */
};
EGCS_SIZE(CarSoundFrame, 0x37C);

/* ------------------------------------------------------------------------ */
/* RaceEntryInformation                                                     */
/* ------------------------------------------------------------------------ */

/* The date of a record (RaceEntryCar +0x2138): one 64-bit word of bit fields
 * (the code builds it with masks and copies it with ld / sd; u037 reads its
 * bytes 4, 5 and the halfword 6: EntryDate).  setDate(NULL) takes the CD
 * clock (BCD, converted).  ? name (u042).  The memory card's date,
 * gt2k_menu::McDateTime (T108: resv, sec, min, hour, day, month, year), has
 * the same 8-byte layout: logger_run / logger_main (u018) cast one to the
 * other. */
struct RaceDate {
    u64 zero   : 8;                         /* bits 0..7, always 0 */
    u64 second : 8;                         /* bits 8..15 */
    u64 minute : 8;                         /* bits 16..23 */
    u64 hour   : 8;                         /* bits 24..31 */
    u64 day    : 8;                         /* bits 32..39 */
    u64 month  : 8;                         /* bits 40..47 */
    u64 year   : 16;                        /* bits 48..63, e.g. 2001 */
};
EGCS_SIZE(RaceDate, 0x8);

/* The car of an entry and the input that drives it.  Base CarEquipments at
 * +0 (its CarDataBase base's vptr +0x1DC is the object's).  vtable 0x003273A0
 * (1 slot, the secondary one in the CarDataBase base: this_delta -0x1D0).
 * typeinfo __tf20RaceEntryInformation 0x001791D8.  Size 0x1F0 (0x1EC bytes
 * of data, 8-byte aligned by CarSaveEquipments' u64; RaceEntryCar +0x20, then
 * +0x210). */
class RaceEntryInformation : public CarEquipments {
public:
    RaceEntryInformation();                 /* 0x00177108 inputId = 0 */
    virtual ~RaceEntryInformation();        /* [0] +0x0C 0x00177140 */
    /* Inline (no callers; T121 has the key method, so egcs emits the copies
     * there, after __tf12RaceEntryCar, in reverse order of definition).
     * RaceEntryCar::copyFrom's setInputID(1) keeps &info in a register as
     * retail does (u042). */
    void setInputID(int id) { inputId = id; }   /* 0x001791D0 */
    int  getInputID() { return inputId; }       /* 0x001791C8 (Lupo 2 name) */

    /* RaceBase::virtual_26's argument: 0x100 + player, 1 the second (ghost)
     * car (u015 / u017: unk1E8). */
    int inputId;                            /* 0x1E8 */
};
EGCS_SIZE(RaceEntryInformation, 0x1F0);

/* ------------------------------------------------------------------------ */
/* RaceEntryCar                                                             */
/* ------------------------------------------------------------------------ */

/* Returns its argument: an out-of-line inline conversion (6 callers; the
 * result is a RaceCarModel's CarModel, u045 / u061: CarDrawParam).  Inline
 * in the original (T121, T129, T131, T180 and T184 each have a weak copy
 * after their other deferred functions, T180's before UnitArena<64>'s
 * typeinfo function, and every call goes to T121's): the
 * callers are inline functions compiled before its body, so the calls
 * stay (setModelLod here).  Declared here for setModelLod's body; T121
 * defines it (inline under egcs, after the classes: its copy comes out
 * last, as retail's 0x00179460). */
CarModel *F_00179460(CarModel *m);          /* 0x00179460 */

/* vtable 0x00327390 (1 slot).  typeinfo __tf12RaceEntryCar 0x00179188.  vptr
 * +0x2140 (after the data).  Size 0x2148 (the deleting destructor:
 * operator delete(this, 0x2148)). */
class RaceEntryCar {
public:
    static void *operator new(size_t size);             /* 0x00177168 the race pool, 16-byte aligned */
    static void  operator delete(void *p, size_t size); /* 0x00177190 */
    /* The members, then: number -1, ghostLapTime RACE_TIME_INVALID,
     * assistTcs / modelEnabled / assistAsm 1, driverName g_00352388
     * ("SESSION BEST"), setDate(NULL).  rankPrev / rankNext are left. */
    RaceEntryCar();                         /* 0x001771B8 */
    virtual ~RaceEntryCar();                /* [0] +0x0C 0x00177350 unloadModel() */

    void unloadModel();                     /* 0x001773D0 model.release(); no sound, no pending model */
    void updateGhostLapTime();              /* 0x00177400 ghostLapTime = state->bestLap (+0x16C4) */
    /* model.colorIndex = the car's colour in the colour table g_00352D98
     * (T485), 0 when it is not there. */
    void updateColorIndex();                /* 0x00177430 */
    /* Takes over another entry: car data (inputId then 1), lap times, the
     * options, its model (now, or by applyPendingModel when deferModel), a
     * copy of its sound (constructed in place).  RaceOrganization::
     * setupGhost. */
    void copyFrom(RaceEntryCar *src, int deferModel);   /* 0x00177488 */
    int  applyPendingModel();               /* 0x00177660 1 when a deferred model was taken over */
    void loadModel(int kind);               /* 0x001776A8 the model file of kind (car_model_path, T446) */
    void loadTires();                       /* 0x001776F8 model.loadTire(the tyre parts, F_00236600) */
    void loadWheel();                       /* 0x00177758 model.loadWheel(the wheel) unless the default one */
    void loadSound();                       /* 0x001777B0 sound.reserveSpu(); the engine sound once */
    void setupTires();                      /* 0x00177830 model.setupTires() */
    /* The draw wrappers: the model's function while modelEnabled. */
    void setReflectionPlane(f32 t);         /* 0x00177850 */
    void F_00177878();                      /* 0x00177878 model.F_001AA568 (empty; not tested) */
    /* The body's shadow in the ground frame (once per CarRenderCtx light
     * pass, T181; u042 F_00177898). */
    void drawShadow(f32 t);                 /* 0x00177898 model.drawShadow */
    void F_001778C0(f32 t);                 /* 0x001778C0 model.F_001AA680 */
    void F_001778E8(f32 t);                 /* 0x001778E8 model.F_001AA6D0 */
    void drawBody(f32 t);                   /* 0x00177910 model.drawBody(t, 1) */
    /* lod: the CarModel LOD (u060).  lod last: RacePS2Base::drawCars (u045)
     * loads it last, in the cross-jumped tail of its two calls (egcs loads
     * argument registers in parameter order); the same registers either way. */
    void drawTires(int blur, f32 scale, f32 t, int lod);   /* 0x00177938 */
    void drawTireCylinders(f32 unused, f32 t);  /* 0x00177960 model.drawTireCylinders (u042 drawWheels) */
    void F_00177988(f32 t);                 /* 0x00177988 model.F_001AAD70 (empty) */
    void F_001779B0(f32 t);                 /* 0x001779B0 model.F_001AAD78 (no callers) */
    void emitSparks(ParticleManager *pm, f32 t);        /* 0x001779D8 */
    void addLamps(FlareClass *flare, f32 t);            /* 0x00177A00 */
    void drawLampReflections(FlareClass *flare, f32 t); /* 0x00177A28 */
    /* Once per frame and car (RacePS2Base::virtual_29; noCrashSound:
     * replayMode != 0): builds the car's CarSoundFrame at its position with
     * the gains of the listening mode (F_001DE050) and a Doppler factor, and
     * hands it to sound.update().  noCrashSound != 0 mutes the crash
     * one-shots (u042 noSkid: it took them for skids). */
    void updateSound(CameraManager *cm, int noCrashSound);  /* 0x00177A50 */
    /* lapRecord.section[section] = time; the difference to the session best. */
    int  recordSectionTime(int section, u32 time);      /* 0x001786B0 */
    /* lapRecord.total = total; the difference to the session best, which
     * takes the lap when update and it is better. */
    int  recordLapTime(u32 total, int update);          /* 0x001786E8 */
    void resetLapTime(u32 time);            /* 0x00178750 lapTime.reset(0), add(time) (no callers) */
    void setDriverName(const char *name);   /* 0x00178798 strcpy */
    void setDate(const RaceDate *date);     /* 0x001787B8 NULL: now */
    /* The accessors: inline (no callers anywhere: every unit expands them;
     * T121 has the key function, so egcs writes all of them there, after
     * __tf13RaceEntryBase, in the reverse order of these bodies:
     * setCondition 0x00178E18 first, getSortPrev 0x00179180 last). */
    RaceEntryCar *getSortPrev() { return sortPrev; }    /* 0x00179180 (index.txt: RaceResultBase::getCarNumber, a code match) */
    RaceEntryCar *getSortNext() { return sortNext; }    /* 0x00179178 */
    RaceEntryCar *getRankPrev() { return rankPrev; }    /* 0x00179170 */
    RaceEntryCar *getRankNext() { return rankNext; }    /* 0x00179168 */
    void setNumber(int n) { number = n; }               /* 0x00179160 */
    int  getNumber() { return number; }                 /* 0x00179158 */
    void setOrder(int v) { order = v; }                 /* 0x00179150 */
    int  getOrder() { return order; }                   /* 0x00179148 */
    void setColorIndex(int v) { model.colorIndex = v; } /* 0x00179140 */
    int  getColorIndex() { return model.colorIndex; }   /* 0x00179138 */
    void setTransmission(int v) { transmission = v; }   /* 0x00179130 */
    int  getTransmission() { return transmission; }     /* 0x00179128 */
    void setTyre(int v) { tyre = v; }                   /* 0x00179120 (u042 setField20E8) */
    int  getTyre() { return tyre; }                     /* 0x00179118 (u042 getField20E8) */
    void setSteeringAssist(int v) { steeringAssist = v; }   /* 0x00179110 */
    int  getSteeringAssist() { return steeringAssist; } /* 0x00179108 */
    void setAssistAsm(int v) { assistAsm = v; }         /* 0x00179100 */
    int  getAssistAsm() { return assistAsm; }           /* 0x001790F8 */
    void setAssistTcs(int v) { assistTcs = v != 0; }    /* 0x001790E8 */
    int  getAssistTcs() { return assistTcs; }           /* 0x001790E0 */
    void setVibration(int v) { vibration = v; }         /* 0x001790D8 */
    int  getVibration() { return vibration; }           /* 0x001790D0 */
    void setPoints(int v) { points = v; }               /* 0x001790C8 */
    int  getPoints() { return points; }                 /* 0x001790C0 */
    /* The model keeps its own copy of the state pointer. */
    void setState(RaceCarState *s) { state = s; model.state = s; }     /* 0x001790B0 */
    RaceCarState *getState() { return state; }          /* 0x001790A8 */
    bool IsValidAutomobile() { return state != 0; }     /* 0x00179098 */
    int  loadCar(u64 code) { return info.setCar(code); }    /* 0x00179078 */
    RaceEntryInformation *getEquipments() { return &info; } /* 0x00179070 */
    u64  getCarCode() { return info.getModelCode(); }   /* 0x00179050 */
    const char *getCarName() { return info.getName1(); }    /* 0x00179030 */
    const char *getCarName2() { return info.getName2(); }   /* 0x00179010 */
    RaceLapTime *getLapTime() { return &lapTime; }      /* 0x00179008 (Lupo 2 names from here to setGhostLapTime) */
    TimeRecord *getTimeRecord() { return &timeRecord; } /* 0x00179000 */
    u32  getSessionBestSectionTime(int i) const { return sessionBest.section[i]; }  /* 0x00178FF0 */
    u32  getSessionBestLapTime() const { return sessionBest.total; }    /* 0x00178FE8 */
    u32  getLastSectionTime(int i) const { return lapRecord.section[i]; }   /* 0x00178FD8 */
    u32  getLastLapTime() const { return lapRecord.total; } /* 0x00178FD0 */
    LapRecord *getLapRecord() { return &lapRecord; }    /* 0x00178FC8 */
    LapRecord *getSessionBestLapRecord() { return &sessionBest; }   /* 0x00178FC0 */
    u32  getGhostLapTime() { return ghostLapTime; }     /* 0x00178FB8 */
    void setGhostLapTime(u32 t) { ghostLapTime = t; }   /* 0x00178FB0 */
    RaceCarModel *getModel() { return &model; }         /* 0x00178FA8 */
    /* The car file's CarModelInfo (RaceCarModel::getInfo's code; no NULL
     * check; no callers; u042 F_00178F98, int). */
    CarModelInfo *getModelInfo() { return model.data->info; }   /* 0x00178F98 */
    CarModel *getCarModel() { return &model.carModel; } /* 0x00178F90 the model's CarModel (model + 4) */
    int  getModelEnabled() { return modelEnabled; }     /* 0x00178F88 */
    void setModelEnabled(int on) { modelEnabled = on; } /* 0x00178F80 */
    CarSound *getSound() { return &sound; }             /* 0x00178F78 */
    void startSound() { sound.start(); }                /* 0x00178F58 */
    void stopSound() { sound.stop(); }                  /* 0x00178F38 */
    void setInput(RaceInput *in) { input = in; }        /* 0x00178F30 */
    RaceInput *getInput() { return input; }             /* 0x00178F28 */
    /* A fixed LOD for the body (-1: by distance), the CarModel's
     * callback.lod, through F_00179460: its body comes after this one, so
     * the call stays (retail calls 0x00179460 here). */
    void setModelLod(int lod) { F_00179460(model.getCarModel())->callback.lod = lod; }  /* 0x00178EF0 */
    void updateBodyState() { model.updateBodyState(); } /* 0x00178ED0 (u042 updateLamps) */
    char *getDriverName() { return driverName; }        /* 0x00178EC8 */
    RaceDate *getDate() { return &date; }               /* 0x00178EC0 */
    CarCondition *getCondition() { return &condition; } /* 0x00178EB8 */
    void setCondition2(const CarCondition &c) { condition = c; }    /* 0x00178E70 the same code as setCondition */
    f32  getTopSpeed() { return topSpeed; }             /* 0x00178E68 */
    void setTopSpeed(f32 v) { topSpeed = v; }           /* 0x00178E60 */
    void setCondition(const CarCondition &c) { condition = c; }     /* 0x00178E18 (no callers) */

    f32            distance;                /* 0x0000 to the point of RaceEntryBase::sortByDistance */
    RaceEntryCar  *sortPrev;                /* 0x0004 the distance order (u045: the draw list, far to near) */
    RaceEntryCar  *sortNext;                /* 0x0008 */
    RaceEntryCar  *rankPrev;                /* 0x000C the rank order (RaceEntryBase::sortByRank) */
    RaceEntryCar  *rankNext;                /* 0x0010 */
    int            number;                  /* 0x0014 the index in RaceEntryBase (-1 until added; 0 the player) */
    /* The car of the DynamicsConductor (u045 / u061 CarState, u051 / u052
     * RaceCarDynamics; u043 / u044 dyn18). */
    RaceCarState  *state;                   /* 0x0018 */
    /*  0x001C: alignment (info is 8-byte aligned) */
    RaceEntryInformation info;              /* 0x0020 */
    RaceLapTime    lapTime;                 /* 0x0210 (u082 lapTimes) */
    LapRecord      sessionBest;             /* 0x11C8 the car's best lap and sections (u043 bestRecord) */
    LapRecord      lapRecord;               /* 0x11DC the current / last lap (u043 lastRecord) */
    TimeRecord     timeRecord;              /* 0x11F0 the best lap time (T303 updates it; u082 bestLap) */
    u32            ghostLapTime;            /* 0x11F4 RACE_TIME_INVALID until set (u018 / u037 / u043 field_11F4) */
    RaceCarModel   model;                   /* 0x11F8 (its state +0x18A4: u043 / u044 dyn18A4) */
    RaceCarModel  *pendingModel;            /* 0x18B8 the model copyFrom deferred */
    int            modelEnabled;            /* 0x18BC the draw wrappers run (u045 visible; u043 / u044 field_18BC) */
    int            soundLoaded;             /* 0x18C0 */
    CarSound       sound;                   /* 0x18C4 */
    /* RaceBase::virtual_26(info.inputId) (u085; u042 / u043 CarDynamics
     * *dynamics: it is RaceInput, T413 / h17). */
    RaceInput     *input;                   /* 0x20DC */
    int            order;                   /* 0x20E0 the grid / result position, 0-based */
    /* 1: manual (GameOptions::gearAuto ^ 1, T053 / T428; u051: the
     * tachometers' transmission). */
    int            transmission;            /* 0x20E4 */
    /* RaceEntry::tyre (T000; the menus' tyre choice), copied by the
     * information classes' setupEntry (T133, T137, T138). */
    int            tyre;                    /* 0x20E8 (u042 field_20E8) */
    /* GameOptions' steering assist / ASM / TCS (T428 steeringAssist1p /
     * assistAsm / assistTcs; the information classes' setupEntry and
     * RaceOrganization::setEntry* copy them).  steeringAssist: 0..4
     * (car_set_steer_assist), assistAsm 0 clears CarSpec.ascFlags (no brake
     * assist / ABS aids), assistTcs 0 clears the throttle cut (u077). */
    int            steeringAssist;          /* 0x20EC (u042 steeringAssist) */
    int            assistAsm;               /* 0x20F0 1 (u042 assistAsm) */
    int            assistTcs;               /* 0x20F4 0 / 1 (u042 assistTcs) */
    /* The input's vibration: RaceOrganization::attachInputs copies it to
     * RaceInput::vibration (+0x138) outside replays. */
    int            vibration;               /* 0x20F8 */
    int            points;                  /* 0x20FC championship points (u015 / u017 / u041) */
    f32            topSpeed;                /* 0x2100 (T303 sets it; u082) */
    /* Copied to RaceInfoSlot +0x1D0 (u043 / u085 name2104, u049 RaceCarName). */
    CarCondition   condition;               /* 0x2104 */
    char           driverName[0x14];        /* 0x2124 "SESSION BEST" by default (u037 name) */
    RaceDate       date;                    /* 0x2138 (u037 EntryDate) */
    /*  vptr                                   0x2140 */
};
EGCS_SIZE(RaceEntryCar, 0x2148);

/* ------------------------------------------------------------------------ */
/* RaceEntryBase                                                            */
/* ------------------------------------------------------------------------ */

/* vtable 0x00327360 (5 slots).  typeinfo __tf13RaceEntryBase 0x00178DD8.
 * vptr +0x1C (after the data).  Size 0x20 (operator delete(this, 0x20)).
 * Base of RaceEntryPS2 (T130).
 *
 * Inline in the original: getRankEnd, getRankTop, getSortEnd, getSortTop and
 * getCount (defined inline below, last first; T121 emits the copies, before
 * __tf13RaceEntryBase).  The Lupo 2 map's RaceEntryBase names at T125's
 * 0x0017B3B0..0x0017B3F0 are code matches: those are RaceOrganization's
 * accessors (u043; T125 defines them so). */
class RaceEntryBase {
public:
    static void *operator new(size_t size);             /* 0x00178938 the race pool (RaceEntryPS2 uses its own) */
    static void  operator delete(void *p, size_t size); /* 0x00178960 */
    RaceEntryBase();                        /* 0x00178988 the rank list is left */
    virtual ~RaceEntryBase();               /* [0] +0x0C 0x001789B0 releaseEntries() */
    virtual RaceEntryCar *createEntry();    /* [1] +0x14 0x00178B10 new RaceEntryCar */
    /* Whether car may be added (here always 1).  It takes the car: addEntry
     * and RaceOrganization::canAddEntry (Lupo 2 inquiry(RaceEntryCar &))
     * pass it through in $a1. */
    virtual int  canAddEntry(RaceEntryCar *car);    /* [2] +0x1C 0x00178B08 1 */
    /* Appends car and gives it its number; -1 when canAddEntry(car) refuses. */
    virtual int  addEntry(RaceEntryCar *car);   /* [3] +0x24 0x00178A90 */
    /* Empty here; RaceSolitaireEntry (T139): the one-car view of the list
     * on / off.  RaceOrganization::setLoggerMode passes its mode (u042:
     * cleanup(); index.txt: RaceResultBase::cleanup, a code match). */
    virtual void virtual_4(int on);         /* [4] +0x2C 0x00178B68 */

    void releaseEntries();                  /* 0x00178A00 deletes every entry; the array stays */
    /* Each car's distance to (x, y, z) (the state's position), the cars
     * linked nearest first (sortTop .. sortEnd; equal ones keep their order). */
    void sortByDistance(f32 x, f32 y, f32 z);   /* 0x00178B70 */
    /* The cars linked by their state's signed rank byte (+0x1465), smallest
     * first (rankTop .. rankEnd). */
    void sortByRank();                      /* 0x00178CE8 */
    RaceEntryCar *getEntry(int i);          /* 0x00178D98 */
    /* (in the reverse of retail's order: egcs writes the copies last body
     * first, getRankEnd 0x00178DB0 .. getCount 0x00178DD0) */
    u32  getCount() { return count; }       /* 0x00178DD0 */
    RaceEntryCar *getSortTop() { return sortTop; }  /* 0x00178DC8 */
    RaceEntryCar *getSortEnd() { return sortEnd; }  /* 0x00178DC0 */
    RaceEntryCar *getRankTop() { return rankTop; }  /* 0x00178DB8 */
    RaceEntryCar *getRankEnd() { return rankEnd; }  /* 0x00178DB0 */

    /* int: the signed compares (u043 0x0017A370 slt, 0x0017AB48 slti of a
     * switch, 0x0017BD24 blez; u089 / u092..u094); the unsigned ones are
     * unsigned loop indices (i < count converts it). */
    int            count;                   /* 0x00 */
    int            capacity;                /* 0x04 RaceEntryPS2: 7 (u047 / u048 / u049; u042 field_04) */
    RaceEntryCar **cars;                    /* 0x08 */
    RaceEntryCar  *sortTop;                 /* 0x0C the nearest car (sortByDistance) */
    RaceEntryCar  *sortEnd;                 /* 0x10 the farthest (u045 drawList: drawn first) */
    RaceEntryCar  *rankTop;                 /* 0x14 the leader (sortByRank) */
    RaceEntryCar  *rankEnd;                 /* 0x18 the last */
    /*  vptr                                   0x1C */
};
EGCS_SIZE(RaceEntryBase, 0x20);

/* ------------------------------------------------------------------------ */
/* Data                                                                     */
/* ------------------------------------------------------------------------ */

/* -> "SESSION BEST" (0x00331248): RaceEntryCar's default driver name, the
 * text table of GT2KMSGRACEMSG (T187 defines it: one entry, 8-aligned). */
extern const char *g_00352388[1];           /* 0x00352388 gp-0x5668 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
extern u32 __ti13RaceEntryBase[2];          /* 0x00353368 gp-0x4688 (u042 / u044: g_00353368) */
extern u32 __ti12RaceEntryCar[2];           /* 0x00353360 gp-0x4690 (u042: g_00353360) */
extern u32 __ti20RaceEntryInformation[4];   /* 0x004B1E70 (u042: g_004B1E70) */
/* (__ti15RaceInformation, which the plan put here, is declared with its
 * class in T124_race_information.h.) */
}
#endif

#endif /* GAME_T121_RACE_ENTRY_H */
