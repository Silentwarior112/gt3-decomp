/* T144_race_license.h - RaceLicense (the licence test, "RACE_MODE_LICENSE"),
 * RaceLicenseBGM and LicensePause (translation unit T144, unit u048),
 * 0x00189AE0..0x0018A0E8.
 *
 * RaceLicense is a RaceSolitaire registered in the race class registry as
 * "RaceLicense" (RaceClassInfo 0x002B93D0 {"RaceLicense", 0x3B440,
 * race_license_create, race_license_construct}, node 0x00351ED8 gp-0x5B18;
 * race_license_static_init, .ctors 0x0028F118).  Its members: a
 * LicensePause menu (a race), a ReplayPause menu (a replay), its own BGM
 * (RaceLicenseBGM, RaceBase::bgm), the RaceLicenseDisplay (T147, which
 * judges the test with LicenseExaminationMessages, T146; it gets the pad
 * input) and the DynamicsConductorLicense (RaceBase::dc).  No result
 * sequence.
 *
 * The virtual methods keep the slot names of the bases (RaceBase T129,
 * RacePS2Base / RaceBGMPS2 T130, RaceSolitaire T139, RacePauseWithRetry
 * T171). */
#ifndef GAME_T144_RACE_LICENSE_H
#define GAME_T144_RACE_LICENSE_H

#include "game/game.h"
#include <stddef.h>                             /* size_t */
#include "game/race/modes/T139_race_solitaire.h"           /* base RaceSolitaire */
#include "game/race/core/T130_race_ps2_base.h"            /* base RaceBGMPS2 */
#include "game/race/pause/T171_race_pause_with_retry.h"    /* base RacePauseWithRetry */
#include "game/race/pause/T170_replay_pause.h"             /* ReplayPause (held by value) */
#include "game/race/display/T147_race_license_display.h"     /* RaceLicenseDisplay (held by value) */
#include "game/race/dynamics/T312_dc_license.h"               /* DynamicsConductorLicense (held by value) */

class RaceBase;                                 /* T129 */
class type_info;                                /* T548 */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
extern "C" {
/* (__si_type_info 0x004B2050, base RaceSolitaire 0x004B1FE0) */
const type_info &__tf11RaceLicense(void);       /* 0x00189FD8 */
/* (__si_type_info 0x004B2060, base RaceBGMPS2 0x004B1EA0) */
const type_info &__tf14RaceLicenseBGM(void);    /* 0x0018A028 */
/* (__si_type_info 0x004B1760, base RacePauseWithRetry 0x004B2380) */
const type_info &__tf12LicensePause(void);      /* 0x0018A078 */
}
#endif

/* (Defined before RaceLicenseBGM: retail's type names are 11RaceLicense,
 * 14RaceLicenseBGM, 12LicensePause - egcs writes them in reverse
 * definition order; u048 matching.)
 * vtable 0x0032C1D8 (4 slots; 0 its own), typeinfo __tf12LicensePause
 * 0x0018A078.  The pause menu of a licence test: a RacePauseWithRetry of
 * its own type.  No data of its own: vptr +0x08 (PauseBase's); 0x2D0 bytes
 * (RaceLicense+0x21800).
 * No user-declared member: the constructor is inline (RaceLicense's
 * constructor stores the vptr after RacePauseWithRetry's), and the
 * destructor is the implicit virtual one ([0] +0x0C 0x0018A0C8, T144's
 * copy: ~RacePauseWithRetry with the caller's flags and no vptr store, as
 * RaceLicense's destructor has it expanded). */
class LicensePause : public RacePauseWithRetry {
public:
};
EGCS_SIZE(LicensePause, 0x2D0);

/* vtable 0x0032C190 (8 slots; 0 and 4 its own), typeinfo
 * __tf14RaceLicenseBGM 0x0018A028.  No data of its own: vptr +0x04
 * (RaceBGMBase's); 0x9B8 bytes (RaceLicense+0x22010). */
class RaceLicenseBGM : public RaceBGMPS2 {
public:
    RaceLicenseBGM();                           /* 0x00189AE0 */
    /* (deleting form: RaceBGMPS2::operator delete(this, 0x9B8)) */
    virtual ~RaceLicenseBGM();                  /* [0] +0x0C 0x00189B18 */
    /* Play: code 0x14 (the race's BGM event) stops the music (stop)
     * instead; every other code goes to RaceBGMPS2. */
    virtual void play(int code, int arg);       /* [4] +0x2C 0x00189B70 */
};
EGCS_SIZE(RaceLicenseBGM, 0x9B8);

/* vtable 0x0032BFA8 (60 slots; 0, 8, 24 and 32 its own), typeinfo
 * __tf11RaceLicense 0x00189FD8.  vptr +0x00 (GameObject's); 0x3B440 bytes
 * (the race classes are 64-byte aligned: T132). */
class RaceLicense : public RaceSolitaire {
public:
    /* T144's copies of RaceBase's race-pool allocation (0x0017BA18 /
     * 0x0017BA40): g_002B3BF0, 16-byte aligned; the size is ignored. */
    static void *operator new(size_t size);     /* 0x00189BB8 */
    static void operator delete(void *p, size_t size);  /* 0x00189BE0 */

    /* Also called directly by run_license_test (F_00132030). */
    RaceLicense();                              /* 0x00189CA0 */
    /* (deleting form: operator delete(this, 0x3B440)) */
    virtual ~RaceLicense();                     /* [ 0] +0x00C 0x00189E50 */
    /* Race start: RaceSolitaire's, then the pause menu (a race) or the
     * replay pause menu given the organization (a replay). */
    virtual void virtual_8(int replay);         /* [ 8] +0x04C 0x00189F00 */
    virtual const char *virtual_24();           /* [24] +0x0CC 0x00189C08 "RaceLicense" */
    /* Posts the start event 3 only (RaceBase's then also starts the race
     * music: event 0x14, code 0x14). */
    virtual void start();                       /* [32] +0x10C 0x00189F78 */

    LicensePause             pause;             /* 0x21800 the pause menu of a race */
    ReplayPause              replayPause;       /* 0x21AD0 (mode 0) */
    RaceLicenseBGM           licenseBgm;        /* 0x22010 RaceBase::bgm */
    RaceLicenseDisplay       display;           /* 0x229C8 RaceBasic's race display; input = &input0 */
    DynamicsConductorLicense dcLicense;         /* 0x2D064 RaceBase::dc */
    /*  0x3B410 .. 0x3B43F: alignment */
};
/* The enhanced engine: its display 4 and its conductor 0x1E0 longer: the
 * members after display sit 4 later than shown; RaceSolitaire 0x200 longer
 * (the ghost car, race_logic.md M1): every member 0x200 later (GT3
 * 0x3B440). */
/* (Wave 1: the hud's RaceDisplay (0xA8E8, GT3 0xA47C) and the embedded race classes' growth; the size
 * from tools/sizeprobe.py, the member offsets above stay
 * GT3's.) */
EGCS_SIZE(RaceLicense, 0x3C580);

/* RaceClassInfo::create of "RaceLicense": new RaceLicense. */
RaceBase *race_license_create(void);                        /* 0x00189C18 */
/* RaceClassInfo::construct: a RaceLicense in buf (NULL if buf is NULL);
 * arg is not used. */
RaceBase *race_license_construct(void *buf, int arg);       /* 0x00189C78 */
/* .ctors entry 0x0028F118: registers "RaceLicense". */
void      race_license_static_init(void);                   /* 0x00189FB0 */

/* T144's registry data (RaceClassInfo / RaceClassNode: T129). */
extern RaceClassInfo g_002B93D0;                /* 0x002B93D0 {"RaceLicense", 0x3B440, race_license_create, race_license_construct} */
extern RaceClassNode g_00351ED8;                /* 0x00351ED8 gp-0x5B18 RaceLicense's node in the registry */

#ifndef BOOT_OLD_GCC                            /* egcs generates them itself */
/* The __si_type_info {vptr, name, base} the three __tf functions fill (u048
 * g_004B2050 / g_004B2060 / g_004B1760). */
extern "C" {
extern u32 __ti11RaceLicense[3];                /* 0x004B2050 */
extern u32 __ti14RaceLicenseBGM[3];             /* 0x004B2060 */
extern u32 __ti12LicensePause[3];               /* 0x004B1760 */
}
#endif

#endif /* GAME_T144_RACE_LICENSE_H */
