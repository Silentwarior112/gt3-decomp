/* T170_replay_pause.h - ReplayPause, the pause menu of a replay, and its
 * window ReplayPauseMenu (translation unit T170, unit u058),
 * 0x001A2EC8..0x001A3580.
 *
 * ReplayPauseMenu is the page "pause_replay" of the menu file pause2.pmb:
 * continue (id 3), exit (4), replay (1).  "replay" opens the replay option
 * window (ReplayOption, T196: the camera choice), which then takes the
 * input until it closes.  virtual_2: 1 while the option window is open,
 * else a decided button's id (replay's 1 keeps the pause), 3 on cancel, 1
 * while the window takes input, 2 on the timeout, else 0.
 *
 * T170 also has unreferenced copies of the inline RaceMenuWindow
 * destructor (0x001A3508) and of __tf20RaceMenuSelectWindow / __tf14
 * RaceMenuWindow (0x001A34B8 / 0x001A3530): T169's are the ones used.
 */
#ifndef GAME_T170_REPLAY_PAUSE_H
#define GAME_T170_REPLAY_PAUSE_H

#include "game/game.h"
#include "game/race/pause/T168_pause_base.h"           /* base PauseBase */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* base RaceMenuSelectWindow */
#include "game/race/pause/T196_replay_option.h"        /* ReplayOption (held by value) */
#include "game/menu/race_menu/T197_race_menu_page.h"       /* RaceMenuPage (held by value) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* RaceMenuImageBlinkButton (held by value) */

class RaceInput;                            /* T413 (pointers only: forward declarations) */
class RaceOrganization;                     /* T125 */
class GSBuffer;                             /* T514 */
class VariableCameraBase;                   /* T397 */
class RaceMenuButton;                       /* T198 */
class type_info;                            /* T548 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
/* the __si_type_info at 0x004B2370 (base __ti9PauseBase) */
const type_info &__tf11ReplayPause(void);           /* 0x001A33C8 */
/* the __si_type_info at 0x004B12E0 (base __ti20RaceMenuSelectWindow) */
const type_info &__tf15ReplayPauseMenu(void);       /* 0x001A3418 */
}
#endif

/* vtable 0x00330320 (1 slot), typeinfo __tf15ReplayPauseMenu 0x001A3418
 * (si, base RaceMenuSelectWindow).  vptr +0x2C (RaceMenuLayer's).  Size
 * 0x278 (ReplayPause +0x2C8 .. +0x540). */
class ReplayPauseMenu : public RaceMenuSelectWindow {
public:
    /* The bases and members inline (only RaceMenuLayer's vtable is stored
     * before this one); the page, its "defaultLayer", the buttons
     * m_continue (3), m_exit (4), m_replay (1), the items = all three. */
    ReplayPauseMenu();                      /* 0x001A2EC8 */
    /* [0] +0x0C 0x001A3468 ~ReplayPauseMenu: implicit (the page with egcs
     * flags 2, ~RaceMenuWindow inline, ~RaceMenuLayer).  Retail expands it
     * inline in ReplayPause's destructor and constructor cleanup, and the
     * T170 copy stores no vtable of its own: egcs synthesized it (a
     * declared destructor, even empty and inline, stores its vtable). */

    RaceMenuPage             page;          /* 0x050 page "pause_replay" of pause2.pmb */
    RaceMenuImageBlinkButton buttons[3];    /* 0x20C continue (3), exit (4), replay (1) */
    RaceMenuButton          *list[3];       /* 0x26C the items */
};
EGCS_SIZE(ReplayPauseMenu, 0x278);

/* vtable 0x003302F8 (4 slots), typeinfo __tf11ReplayPause 0x001A33C8 (si,
 * base PauseBase).  vptr +0x8 (PauseBase's).  Size 0x540 (held by value by
 * the race modes: RaceArcade +0x25C94 .. dcSingle +0x261D4). */
class ReplayPause : public PauseBase {
public:
    /* PauseBase(), option(mode), the menu, then ReplayPause::virtual_1
     * (direct).  mode: ReplayOption's (0 single player, 1 split battle). */
    ReplayPause(int mode);                  /* 0x001A30C0 */
    /* The menu inline, ~ReplayOption (flags 2), ~PauseBase. */
    virtual ~ReplayPause();                 /* [0] +0x0C 0x001A31A8 (index: virtual_0) */
    /* Opened: the cursor on continue, the timer reset. */
    virtual void virtual_1();               /* [1] +0x14 0x001A3228 */
    virtual int  virtual_2(RaceInput *in, RaceOrganization *org);  /* [2] +0x1C 0x001A3298 */
    /* The menu (draw(buf, 1)) or, while it is open, the option window. */
    virtual void virtual_3(GSBuffer *buf, RaceOrganization *org,
                           VariableCameraBase *camera);            /* [3] +0x24 0x001A3380 */
    /* option.org = org, option.camera = org->race->virtual_6() (the camera
     * manager).  Callers: the race modes' virtual_8 in a replay. */
    void F_001A3248(RaceOrganization *org); /* 0x001A3248 */

    ReplayOption    option;                 /* 0x00C the replay option window */
    ReplayPauseMenu menu;                   /* 0x2C8 */
};
EGCS_SIZE(ReplayPause, 0x540);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
extern u32 __ti11ReplayPause[3];            /* 0x004B2370 __si_type_info (u058 g_004B2370) */
extern u32 __ti15ReplayPauseMenu[3];        /* 0x004B12E0 __si_type_info (u058 g_004B12E0) */
}
#endif

#endif /* GAME_T170_REPLAY_PAUSE_H */
