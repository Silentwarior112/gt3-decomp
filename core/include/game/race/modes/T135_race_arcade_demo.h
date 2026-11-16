/* T135_race_arcade_demo.h - RaceArcadeDemoInformation, RaceArcadeDemo
 * (translation unit T135, unit u046), 0x00182850..0x00182E58.
 *
 * RaceArcadeDemo is an arcade race run as a demonstration: the attract demo
 * or, with a "shop car", the test drive of a dealer's car (the
 * information's setShopCar / setShopCarPos, Lupo 2 names).  The shop car,
 * when set, is loaded into the car setupEntry is given (colour id 0 of the
 * car) and keeps slot 0; the opponents fill the rest, and the car is moved
 * to grid position shopCarPos (1-based; 0 = the last).  The race has no time
 * limit by default (getTimeLimitOr(0)) and no laps (getNumberOfLaps 0).
 *
 * The race itself: the pause menu closes itself after 30 s, event 7 ends
 * the race with code 5 (virtual_34), and an idle timer of 1800 frames (30 s)
 * posts event 1 (the end) unless a button other than left / right
 * (CONTROL_LEFT / CONTROL_RIGHT, T474) is held (or in a replay).  The race display is created
 * without its inner panel.  Registered as "RaceArcadeDemo" (g_002B6DD0).
 */
#ifndef GAME_T135_RACE_ARCADE_DEMO_H
#define GAME_T135_RACE_ARCADE_DEMO_H

#include "game/game.h"
#include <stddef.h>                         /* size_t */
#include "game/race/modes/T134_race_arcade.h"          /* bases RaceArcade, RaceArcadeInformation */
#include "game/race/pause/T169_race_pause.h"           /* RacePause (held by value) */
#include "game/race/pause/T170_replay_pause.h"         /* ReplayPause (held by value) */

class RaceBase;                             /* pointers only */
class RaceEntryCar;                         /* T121 */
class RaceOrganization;                     /* T125 */
class type_info;

/* The typeinfo functions, declared before the classes (see T132): GCC 14
 * build only.  egcs generates them itself (and their type names), unless an
 * earlier extern "C" declaration hides them (u046 matching). */
#ifndef BOOT_OLD_GCC
extern "C" {
/* the __si_type_info at 0x004B1F30 (base __ti21RaceArcadeInformation
 * 0x004B1F10) */
const type_info &__tf25RaceArcadeDemoInformation(void);     /* 0x00182E08 */
/* the __si_type_info at 0x004B1F40 (base __ti10RaceArcade 0x004B1F00) */
const type_info &__tf14RaceArcadeDemo(void);                /* 0x00182DA8 */
}
#endif

/* vtable 0x003293D8 (15 slots: [0] [6] [10] [11] its own).  typeinfo
 * __tf25RaceArcadeDemoInformation 0x00182E08 (si, base
 * RaceArcadeInformation).  vptr +0x54.  Size 0x268 (u64 aligned). */
class RaceArcadeDemoInformation : public RaceArcadeInformation {
public:
    RaceArcadeDemoInformation();                        /* 0x00182850 no shop car */
    virtual ~RaceArcadeDemoInformation();               /* [ 0] +0x0C 0x00182890 */
    virtual int  getTimeLimit();                        /* [ 6] +0x3C 0x001828B8 getTimeLimitOr(0): none by default */
    /* With a shop car: the car's equipments load it, with its colour id 0
     * (the colour table g_00352D98).  No entry number check, no inputId. */
    virtual void setupEntry(RaceEntryCar *car);         /* [10] +0x5C 0x001828D8 */
    /* The opponents from slot 1 (with a shop car) or 0; slot 0 then goes to
     * grid position shopCarPos - 1, or to the last place. */
    virtual void setupOrganization(RaceOrganization *org);  /* [11] +0x64 0x00182930 */

    /* Lupo 2 names; unreferenced.  Inline: T135 (the key function's file)
     * emits the copies at its end, in the reverse of this order, before
     * __tf25RaceArcadeDemoInformation (retail's frame info, u046's data
     * pass). */
    void setShopCar(u64 car) { shopCar = car; }         /* 0x00182E00 (a 64-bit store) */
    void setShopCarPos(int pos) { shopCarPos = pos; }   /* 0x00182DF8 */

    u64 shopCar;                            /* 0x258 0: none (u001: unk258) */
    int shopCarPos;                         /* 0x260 1-based grid position, 0: the last */
};
EGCS_SIZE(RaceArcadeDemoInformation, 0x268);

/* vtable 0x00329210 (56 slots: [0] [8] [24] [27] [28] [40] its own).
 * typeinfo __tf14RaceArcadeDemo 0x00182DA8 (si, base RaceArcade).  vptr +0.
 * Size 0x34D80 (RaceArcadeDemo_create; the deleting destructor). */
class RaceArcadeDemo : public RaceArcade {
public:
    /* (No class placement new: RaceArcadeDemo_construct uses <new>'s ::new,
     * and an inline member would be emitted with the vtable, which retail's
     * T135 has not; u046's data pass.) */
    static void *operator new(size_t size);                 /* 0x001829C0 the race pool */
    static void  operator delete(void *p, size_t size);     /* 0x001829E8 */
    /* RaceArcade, its own two menus (demoReplayPause in mode 0), then the
     * race display's inner panel off. */
    RaceArcadeDemo();                                       /* 0x00182AA8 */
    virtual ~RaceArcadeDemo();                              /* [ 0] +0x0C  0x00182B98 */
    /* RaceArcade's (it installs RaceArcade's menus, not this class's), the
     * pause menu's 30 s timeout, idleTimer = 1800. */
    virtual void virtual_8(int replay);                     /* [ 8] +0x4C  0x00182C20 */
    virtual const char *virtual_24();                       /* [24] +0xCC  0x00182A10 "RaceArcadeDemo" */
    /* Event 7 ends the race with code 5; then RacePS2Base's. */
    virtual int  virtual_27();                              /* [27] +0xE4  0x00182C68 */
    /* RaceSinglePlayer's, then the idle timer. */
    virtual void virtual_28(float dt);                      /* [28] +0xEC  0x00182CC8 */
    /* Inline: T135 emits the copy at its end, before __tf14RaceArcadeDemo
     * (retail's frame info). */
    virtual int  getNumberOfLaps() { return 0; }            /* [40] +0x14C 0x00182DA0 0 (Lupo 2 name) */

    int         idleTimer;                  /* 0x34580 frames left without a button (1800 = 30 s) */
    RacePause   demoPause;                  /* 0x34584 (? unused: virtual_8 keeps RaceArcade's menus) */
    ReplayPause demoReplayPause;            /* 0x34830 (? unused, as demoPause; mode 0) */
    /*  0x34D70 .. 0x34D7F: alignment */
};
EGCS_SIZE(RaceArcadeDemo, 0x34D80);

/* RaceClassInfo "RaceArcadeDemo" (g_002B6DD0: size 0x34D80, these two). */
RaceBase *RaceArcadeDemo_create(void);                      /* 0x00182A20 */
RaceBase *RaceArcadeDemo_construct(void *buf, int arg);     /* 0x00182A80 a RaceArcadeDemo in buf (NULL: none) */
/* .ctors entry 0x0028F0FC: race_class_register(&g_00351E48, &g_002B6DD0). */
void      race_arcade_demo_static_ctor(void);               /* 0x00182D78 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
/* The __si_type_info {vptr, name, base} the two __tf functions fill. */
extern "C" {
extern u32 __ti25RaceArcadeDemoInformation[3];  /* 0x004B1F30 (u046 g_004B1F30) */
extern u32 __ti14RaceArcadeDemo[3];             /* 0x004B1F40 (u046 g_004B1F40) */
}
#endif

#endif /* GAME_T135_RACE_ARCADE_DEMO_H */
