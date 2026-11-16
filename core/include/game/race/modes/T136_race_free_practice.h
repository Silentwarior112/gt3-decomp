/* T136_race_free_practice.h - RaceFreePractice (translation unit T136, unit
 * u046), 0x00182E58..0x00183240.
 *
 * The free practice race mode ("RACE_MODE_FREE_PRACTICE"): RaceSinglePlayer
 * with the 7 car icons, the race / replay pause menus and its own dynamics
 * (DynamicsConductorFreePractice, which RaceBase::dc points to).  Unlike
 * RaceArcade it has no result sequence: virtual_8 only installs a pause
 * menu, and the icons go to the race display alone.  Registered as
 * "RaceFreePractice" (g_002B70D8).
 */
#ifndef GAME_T136_RACE_FREE_PRACTICE_H
#define GAME_T136_RACE_FREE_PRACTICE_H

#include "game/game.h"
#include <stddef.h>                         /* size_t */
#include "game/race/modes/T133_race_single_player.h"   /* base RaceSinglePlayer */
#include "game/car/model/T184_car_icon_maker.h"       /* CarIcon (held by value) */
#include "game/race/pause/T169_race_pause.h"           /* RacePause (held by value) */
#include "game/race/pause/T170_replay_pause.h"         /* ReplayPause (held by value) */
#include "game/race/dynamics/T315_dc_free_practice.h"     /* DynamicsConductorFreePractice (held by value) */

class RaceBase;                             /* pointers only */
class type_info;

/* The typeinfo function, declared before the class (see T132): the
 * __si_type_info at 0x004B1F50 (base __ti16RaceSinglePlayer 0x004B1EF0).
 * GCC 14 build only: egcs generates it itself (u046 matching). */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf16RaceFreePractice(void);  /* 0x001831F0 */
}
#endif

/* vtable 0x00329708 (56 slots: [0] [8] [24] [43] its own).  typeinfo
 * __tf16RaceFreePractice 0x001831F0 (si, base RaceSinglePlayer).  vptr +0.
 * Size 0x33C40 (RaceFreePractice_create; the deleting destructor): the
 * conductor ends at 0x33C34, the rest is the alignment of the race
 * classes (T132). */
class RaceFreePractice : public RaceSinglePlayer {
public:
    /* (No class placement new: RaceFreePractice_construct uses <new>'s
     * ::new, and an inline member would be emitted with the vtable, which
     * retail's T136 has not; u046's data pass.) */
    static void *operator new(size_t size);                 /* 0x00182E58 the race pool */
    static void  operator delete(void *p, size_t size);     /* 0x00182E80 */
    /* The members (replayPause in mode 0), setRaceMode
     * ("RACE_MODE_FREE_PRACTICE"), dc = &dcFree. */
    RaceFreePractice();                                     /* 0x00182F40 */
    virtual ~RaceFreePractice();                            /* [ 0] +0x0C  0x00183078 */
    /* RaceSinglePlayer's, then the race pause menu, or in a replay (mode 1)
     * the replay pause menu (told the organization first). */
    virtual void virtual_8(int replay);                     /* [ 8] +0x4C  0x00183100 */
    virtual const char *virtual_24();                       /* [24] +0xCC  0x00182EA8 "RaceFreePractice" */
    /* The car icons (F_001AE3F8, T184), given to the race display. */
    virtual void virtual_43();                              /* [43] +0x164 0x00183178 */

    CarIcon     icons[7];                   /* 0x16EC0 one per car */
    RacePause   pause;                      /* 0x2509C */
    ReplayPause replayPause;                /* 0x25348 (mode 0) */
    DynamicsConductorFreePractice dcFree;   /* 0x25888 RaceBase::dc points here (never destroyed) */
    /*  0x33C34 .. 0x33C3F: alignment */
};
EGCS_SIZE(RaceFreePractice, 0x33C40);

/* RaceClassInfo "RaceFreePractice" (g_002B70D8: size 0x33C40, these two). */
RaceBase *RaceFreePractice_create(void);                    /* 0x00182EB8 */
RaceBase *RaceFreePractice_construct(void *buf, int arg);   /* 0x00182F18 a RaceFreePractice in buf (NULL: none) */
/* .ctors entry 0x0028F100: race_class_register(&g_00351E58, &g_002B70D8). */
void      race_free_practice_static_ctor(void);             /* 0x001831C8 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" u32 __ti16RaceFreePractice[3];   /* 0x004B1F50 the __si_type_info (u046 g_004B1F50) */
#endif

#endif /* GAME_T136_RACE_FREE_PRACTICE_H */
