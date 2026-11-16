/* T168_pause_base.h - PauseBase, the base of the race pause menus
 * (translation unit T168, unit u057), 0x001A2590..0x001A2678.
 *
 * RaceBasic (T132) holds a PauseBase * (+0xC880): START opens it
 * (virtual_1), every paused frame runs virtual_2 and draws virtual_3.
 * virtual_2's result: 0 still paused, 1 stays paused (an input was taken),
 * 2 timed out, 3 continue, else the id of the button chosen (4 exit, 5
 * retry ...).  PauseBase itself is the menu that continues at once; the
 * menus are RacePause (T169), ReplayPause (T170), RacePauseWithRetry (T171,
 * and its LicensePause, T144).  The timer counts the frames since the last
 * input when a timeout is set (RaceArcadeDemo: 30 s).
 */
#ifndef GAME_T168_PAUSE_BASE_H
#define GAME_T168_PAUSE_BASE_H

#include "game/game.h"

class RaceInput;                            /* T413 (pointers only: forward declarations) */
class RaceOrganization;                     /* T125 */
class GSBuffer;                             /* T514 */
class VariableCameraBase;                   /* T397 */
class type_info;                            /* T548 */

/* PauseBase::virtual_2's results.  A decided button returns its id: the
 * menus' button ids are these values (m_continue 3, m_exit / m_ok 4, m_retry
 * 5; the buttons that switch the page or open the replay options return 1).
 * RaceBasic::virtual_30 maps them to RaceBase::virtual_34's end codes (2
 * timeout -> 3, 4 exit -> 2, 5 retry -> 4).  (The sweep moved u045 / u047 /
 * u048 / u049 / u057 / u058's copies here.) */
enum PauseResult {
    PAUSE_RESULT_PAUSED   = 0,              /* still paused */
    PAUSE_RESULT_INPUT    = 1,              /* still paused, an input was taken */
    PAUSE_RESULT_TIMEOUT  = 2,              /* the menu timed out (PauseBase::tick) */
    PAUSE_RESULT_CONTINUE = 3,              /* continue (or cancel on the main page) */
    PAUSE_RESULT_EXIT     = 4,              /* exit, confirmed with ok */
    PAUSE_RESULT_RETRY    = 5               /* retry (RacePauseWithRetry) */
};

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" const type_info &__tf9PauseBase(void);  /* 0x001A2638 */
#endif

/* vtable 0x0032FFA8 (4 slots), typeinfo __tf9PauseBase 0x001A2638 (user).
 * 8 bytes of data, egcs vptr +0x8.  Size 0xC. */
class PauseBase {
public:
    PauseBase();                            /* 0x001A2590 timer = timeout = 0 */
    virtual ~PauseBase();                   /* [0] +0x0C 0x001A25B0 */
    /* Opened (RaceBasic::setPauseMenu, START): nothing here. */
    virtual void virtual_1();               /* [1] +0x14 0x001A2630 */
    /* The update of a paused frame: 3 (continue) here. */
    virtual int  virtual_2(RaceInput *in, RaceOrganization *org);  /* [2] +0x1C 0x001A25F0 */
    /* The draw: nothing here.  camera: view 0's (CameraManager +0x300 +
     * 0x2D4, RaceBasic::virtual_45); no menu reads it (u057 / u058: int). */
    virtual void virtual_3(GSBuffer *buf, RaceOrganization *org,
                           VariableCameraBase *camera);            /* [3] +0x24 0x001A25F8 */
    void setTimeout(int seconds);           /* 0x001A25E0 ? name: timeout = seconds * 60 (u046 F_001A25E0) */
    int  tick();                            /* 0x001A2600 ? name: counts a frame; 1 once past the timeout */
    void resetTimer();                      /* 0x001A2628 ? name */

    int timer;                              /* 0x0 frames since the last input (u046 / u050 field_00) */
    int timeout;                            /* 0x4 frames, 0: none */
    /*  vptr                                   0x8 */
};
EGCS_SIZE(PauseBase, 0xC);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
/* The __user_type_info of PauseBase, the base node of RacePause (T169),
 * ReplayPause (T170) and RacePauseWithRetry (T171).  The plan put it in
 * T167 (u057 / u058: g_003533C0). */
extern "C" u32 __ti9PauseBase[2];           /* 0x003533C0 gp-0x4630 */
#endif

#endif /* GAME_T168_PAUSE_BASE_H */
