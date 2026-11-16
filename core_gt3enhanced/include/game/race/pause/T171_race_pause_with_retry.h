/* T171_race_pause_with_retry.h - RacePauseWithRetry, the pause menu with a
 * retry button, and its window PauseMenuWithRetry (translation unit T171,
 * unit u058), 0x001A3580..0x001A3EA8.
 *
 * PauseMenuWithRetry is the page "pause_retry" of the menu file pause3.pmb:
 * continue (id 3), exit (1), retry (5), and after exit the message
 * exit_message with ok (4) / cancel (1).  The race modes with a retry hold a
 * RacePauseWithRetry (RaceTimeTrial, RaceMachineTest; RaceLicense its
 * LicensePause, T144).  virtual_2 works as RacePause's (T169); an event
 * 0x13 of the organization (taken) disables retry: DynamicsConductor::
 * checkLapRecord (T308) posts it for a new lap record.
 *
 * T171 also has unreferenced copies of the inline RaceMenuWindow
 * destructor (0x001A3E30) and of __tf20RaceMenuSelectWindow / __tf14
 * RaceMenuWindow (0x001A3DE0 / 0x001A3E58): T169's are the ones used.
 */
#ifndef GAME_T171_RACE_PAUSE_WITH_RETRY_H
#define GAME_T171_RACE_PAUSE_WITH_RETRY_H

#include "game/game.h"
#include "game/race/pause/T168_pause_base.h"           /* base PauseBase */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* base RaceMenuSelectWindow */
#include "game/menu/race_menu/T197_race_menu_page.h"       /* RaceMenuPage (held by value) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* RaceMenuImageBlinkButton (held by value) */

class RaceInput;                            /* T413 (pointers only: forward declarations) */
class RaceOrganization;                     /* T125 */
class GSBuffer;                             /* T514 */
class VariableCameraBase;                   /* T397 */
class RaceMenuButton;                       /* T198 */
namespace gt2k_menu { class Item; }         /* T213 */
class type_info;                            /* T548 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
/* the __si_type_info at 0x004B2380 (base __ti9PauseBase); also the base of
 * __tf12LicensePause (T144) */
const type_info &__tf18RacePauseWithRetry(void);    /* 0x001A3CF0 */
/* the __si_type_info at 0x004B1750 (base __ti20RaceMenuSelectWindow) */
const type_info &__tf18PauseMenuWithRetry(void);    /* 0x001A3D40 */
}
#endif

/* vtable 0x00330540 (1 slot), typeinfo __tf18PauseMenuWithRetry 0x001A3D40
 * (si, base RaceMenuSelectWindow).  vptr +0x2C (RaceMenuLayer's).  Size
 * 0x2C0 (RacePauseWithRetry +0xC .. +0x2CC). */
class PauseMenuWithRetry : public RaceMenuSelectWindow {
public:
    /* The bases and members inline (`confirm' is not set here); the page,
     * its "defaultLayer", the buttons m_continue (3), m_exit (1), m_retry
     * (5), m_ok (4), m_cancel (1), the item exit_message; then showMain. */
    PauseMenuWithRetry();                   /* 0x001A3580 */
    /* [0] +0x0C 0x001A3D90 ~PauseMenuWithRetry: implicit (the page with
     * egcs flags 2, ~RaceMenuWindow inline, ~RaceMenuLayer), as
     * ReplayPauseMenu's (T170_replay_pause.h): inline in
     * RacePauseWithRetry's destructor and constructor cleanup, no vtable
     * store of its own in the T171 copy. */
    /* continue / exit / retry shown, the others hidden; items = them,
     * cursor 0. */
    void showMain();                        /* 0x001A37A0 */
    /* ok / cancel and exit_message shown; cursor on cancel. */
    void showConfirm();                     /* 0x001A38A8 */
    void toggle();                          /* 0x001A39A0 showMain or showConfirm */
    /* The retry button's `enabled' (RaceMenuButton +0x0C), refreshed
     * (virtual_1). */
    void setRetryEnabled(int on);           /* 0x001A39D8 */

    int                      confirm;       /* 0x050 the confirmation page is shown */
    RaceMenuPage             page;          /* 0x054 page "pause_retry" of pause3.pmb */
    RaceMenuImageBlinkButton buttons[5];    /* 0x210 continue (3), exit (1), retry (5), ok (4), cancel (1) */
    RaceMenuButton          *list[3];       /* 0x2B0 the items of the page shown */
    gt2k_menu::Item         *exitMessage;   /* 0x2BC RaceMenuLayer::newItem "exit_message" (u058 RaceMenuImage *) */
};
EGCS_SIZE(PauseMenuWithRetry, 0x2C0);

/* vtable 0x00330518 (4 slots), typeinfo __tf18RacePauseWithRetry 0x001A3CF0
 * (si, base PauseBase).  vptr +0x8 (PauseBase's).  Size 0x2D0 (RaceTimeTrial,
 * RaceMachineTest +0x2FA08; the base of LicensePause, T144). */
class RacePauseWithRetry : public PauseBase {
public:
    /* PauseBase(), the menu, then RacePauseWithRetry::virtual_1 (direct). */
    RacePauseWithRetry();                   /* 0x001A3A08 */
    /* Also called by LicensePause's destructor (T144). */
    virtual ~RacePauseWithRetry();          /* [0] +0x0C 0x001A3AB8 (index: virtual_0) */
    /* Opened: retry enabled, the main page, the timer reset, shown = 0. */
    virtual void virtual_1();               /* [1] +0x14 0x001A3B30 */
    virtual int  virtual_2(RaceInput *in, RaceOrganization *org);  /* [2] +0x1C 0x001A3B80 */
    /* menu.draw(buf, 1) (RaceMenuLayer). */
    virtual void virtual_3(GSBuffer *buf, RaceOrganization *org,
                           VariableCameraBase *camera);            /* [3] +0x24 0x001A3CD0 */

    PauseMenuWithRetry menu;                /* 0x00C */
    int                shown;               /* 0x2CC the main page was shown since it opened */
};
EGCS_SIZE(RacePauseWithRetry, 0x2D0);

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
extern u32 __ti18RacePauseWithRetry[3];     /* 0x004B2380 __si_type_info (u058 g_004B2380) */
extern u32 __ti18PauseMenuWithRetry[3];     /* 0x004B1750 __si_type_info (u058 g_004B1750) */
}
#endif

#endif /* GAME_T171_RACE_PAUSE_WITH_RETRY_H */
