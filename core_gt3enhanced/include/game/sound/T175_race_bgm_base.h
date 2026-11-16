/* T175_race_bgm_base.h - RaceBGMBase, the race BGM interface (translation
 * unit T175, unit u058), 0x001A6318..0x001A6488.
 *
 * Every method is empty; RaceBGMPS2 (T130, RacePS2Base +0xBCF0) overrides
 * them all under these names (h23).  RaceBase +0xC points at the race's
 * BGM: the static dummy g_003521D0 (built by T175_static_init) until
 * RacePS2Base installs its RaceBGMPS2; RaceOrganization::getBGM returns it.
 * RaceBase::virtual_8 stores the organization in org.  The plan had put
 * g_003521D0 in T125 and the __user_type_info __ti11RaceBGMBase in T130;
 * both are declared here, at the end.
 */
#ifndef GAME_T175_RACE_BGM_BASE_H
#define GAME_T175_RACE_BGM_BASE_H

#include "game/game.h"
#include <stddef.h>                         /* size_t */

class RaceOrganization;                     /* T125 (pointers only: forward declarations) */
class type_info;                            /* T548 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" const type_info &__tf11RaceBGMBase(void);   /* 0x001A6448 (user; base of __tf10RaceBGMPS2, u044) */
#endif

#ifdef BOOT_OLD_GCC
/* With #pragma implementation in T175_race_bgm_base.cpp: egcs emits the
 * out-of-line copies of the inline accessors (0x001A6438 / 0x001A6440), the
 * vtable and __tf11RaceBGMBase only there, at the end of the object after
 * the static constructor / destructor (the listing's order). */
#pragma interface
#endif

/* vtable 0x00330890 (8 slots), typeinfo __tf11RaceBGMBase 0x001A6448
 * (user).  4 bytes of data, egcs vptr +0x4.  Size 8 (the deleting
 * destructor: operator delete(this, 8)). */
class RaceBGMBase {
public:
    /* The class's copies of RaceBase's race-pool operators (g_002B3BF0,
     * 16-aligned; T123).  new has no callers. */
    static void *operator new(size_t size);             /* 0x001A6318 */
    static void  operator delete(void *p, size_t size); /* 0x001A6340 */

    /* Only the vptr; org is not set.  Callers: RaceBGMPS2's constructor,
     * T175_static_init. */
    RaceBGMBase();                          /* 0x001A6368 */
    virtual ~RaceBGMBase();                 /* [0] +0x0C 0x001A6380 (index: virtual_0) */
    /* The slot names (u119 / u044 / u058; virtual_1 .. virtual_7 before the
     * sweep): RaceBase::virtual_27 maps the BGM event's codes -2 / -3 / -1
     * / else to pause / resume / stop / play.  2 / 3 are getMarkerTime /
     * getMarkerHit, not the proposed markerTime / markerHit: RaceBGMPS2's
     * arrays have those names. */
    virtual void update();                  /* [1] +0x14 0x001A63B0 */
    virtual f32  getMarkerTime(int ch);     /* [2] +0x1C 0x001A63B8 seconds to channel ch's next marker: FLT_MAX */
    virtual int  getMarkerHit(int ch);      /* [3] +0x24 0x001A63C8 channel ch passed a marker this frame: 0 */
    virtual void play(int code, int arg);   /* [4] +0x2C 0x001A63D0 */
    virtual void pause();                   /* [5] +0x34 0x001A63D8 */
    virtual void resume();                  /* [6] +0x3C 0x001A63E0 */
    virtual void stop();                    /* [7] +0x44 0x001A63E8 */
    /* Inline accessors of org; T175's out-of-line copies (#pragma
     * implementation) have no callers.  egcs emits a class's inline copies in
     * reverse definition order: the setter is defined first. */
    void F_001A6440(RaceOrganization *o) { org = o; }   /* 0x001A6440 org = o */
    RaceOrganization *F_001A6438() { return org; }      /* 0x001A6438 return org */

    RaceOrganization *org;                  /* 0x0 */
    /*                vptr                     0x4 */
};
EGCS_SIZE(RaceBGMBase, 0x8);

/* The dummy BGM: RaceBase::bgm until RacePS2Base installs its RaceBGMPS2,
 * and what getBGM returns without a race.  Defined in T175 (.sdata, the
 * FLT_MAX getMarkerTime returns follows it at 0x003521D8); its static
 * constructor / destructor (_GLOBAL_$I$ / $D$ 0x001A6418 / 0x001A63F0,
 * .ctors / .dtors entries 0x0028F13C / 0x0028F21C) build and destroy it.
 * u043 typed it RaceInput, u044 / u046 Control (h10: it is neither). */
extern RaceBGMBase g_003521D0;              /* 0x003521D0 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
/* The __user_type_info {vptr, name} __tf11RaceBGMBase fills; the base node
 * of __ti10RaceBGMPS2 (T130). */
extern "C" u32 __ti11RaceBGMBase[2];        /* 0x003533C8 gp-0x4628 */
#endif

#endif /* GAME_T175_RACE_BGM_BASE_H */
