/* T138_race_link_battle.h - RaceLinkBattleInformation, RaceLinkBattleInput,
 * RaceLinkBattleNodeInput, RaceLinkBattle (translation unit T138, unit
 * u047), 0x001848A8..0x00186968; with the instances RaceBattleInformationT<8>
 * and RaceBattleT<8> of the T137 templates.
 *
 * RaceLinkBattle runs one console of an i.LINK battle of up to 8 PS2s
 * ("RaceLinkBattle", g_002B7960).  Every node of the session has a
 * RaceLinkBattleNodeInput that reads the input the node published (its
 * GameOptions, GameStatus menu.options[node] (T427 / T428, 0x888 bytes per
 * node; u047: LinkNodeStatus), attached to the pad manager as slots
 * 0xC..0x13).  A console with viewMode 0
 * (g_00352CA8) drives a car with its own RaceLinkBattleInput (slot 0x14);
 * the other view modes (1..9) make it a display console: setupView picks the
 * node it follows and the place of its screen in the screen wall
 * (viewParams, a ScreenLayout for CameraManager::setScreenLayout) from its
 * position among the display nodes.  Every frame (virtual_46 -> updateLink)
 * a state machine follows the link: 0 running (a local pause request or a
 * state sent by the others moves on), 1 the pause menu, 2 waiting for the
 * others to resume, 3 the connection error message; virtual_27 resets the
 * link after 601 frames of waiting (g_0035322C).
 *
 * RaceLinkBattleInformation sets the race up from what the host (node 0)
 * published: the option block, 6 entries, the car of every linked node;
 * entries without a node get cars from RaceInformation::assignCars, and the
 * linked nodes come first in the start order.
 *
 * The dossier class "Recorder" (u047 / u048 / u049) is the nested class
 * AutomobileControlRecord::Recorder (RTTI Q223AutomobileControlRecord8Recorder,
 * T318_control_record.h; u047's copy is declared inside
 * AutomobileControlRecord): it is not redeclared here.
 */
#ifndef GAME_T138_RACE_LINK_BATTLE_H
#define GAME_T138_RACE_LINK_BATTLE_H

#include "game/game.h"
#include <stddef.h>                         /* size_t */
#include "game/race/modes/T137_race_battle_templates.h"    /* bases RaceBattleT<N>, RaceBattleInformationT<N> (not
                                                   T137_race_split_battle.h: retail's T138 has no type
                                                   name of the <2> instances it makes) */
#include "game/race/core/T413_race_input.h"           /* base RaceInput */
#include "game/race/dynamics/T310_dynamics_conductor_battlemp.h"  /* DynamicsConductorBattleMP (held by value) */
#include "game/car/model/T184_car_icon_maker.h"       /* CarIcon (held by value) */
#include "game/menu/records/T191_result_link_battle.h"   /* ResultLinkBattle (held by value) */
#include "game/race/pause/T169_race_pause.h"           /* RacePause (held by value) */
#include "game/race/pause/T170_replay_pause.h"         /* ReplayPause (held by value) */
#include "game/race/display/T154_race_display.h"         /* RaceDisplay (held by value) */
#include "game/camera/T383_camera_manager.h"       /* ScreenLayout (held by value) */

class GSBuffer;                             /* pointers only: forward declarations */
class RaceBase;                             /* T129 */
class RaceEntryBase;                        /* T121 */
class RaceEntryCar;                         /* T121 */
class RaceOrganization;                     /* T125 */
class ControllerConfig;                     /* T426 */
/* What a node published: its GameOptions (T428), GameStatus menu.options[node]
 * (T427: GameStatus +0x307C8 +0x600 + node x 0x888; u047: LinkNodeStatus, its
 * +0x638 the node's ControllerConfig). */
class GameOptions;                          /* T428 */
class type_info;

/* The typeinfo functions, declared before the classes (see T132): for GCC 14
 * only; egcs declares and generates them itself (u047 matching: guarded, all
 * six compile exact). */
#ifndef BOOT_OLD_GCC
extern "C" {
/* the __si_type_info __tit22RaceBattleInformationT1i8 at 0x004B1250 (base
 * __ti15RaceInformation gp-0x4678) */
const type_info &__tft22RaceBattleInformationT1i8(void);    /* 0x00186918 */
/* the __si_type_info at 0x004B1FA0 (base 0x004B1250) */
const type_info &__tf25RaceLinkBattleInformation(void);     /* 0x001868C8 */
/* the __si_type_info at 0x004B1FB0 (base __ti9RaceInput 0x004B2990) */
const type_info &__tf19RaceLinkBattleInput(void);           /* 0x00186878 */
/* the __si_type_info at 0x004B1F90 (base __ti9RaceInput) */
const type_info &__tf23RaceLinkBattleNodeInput(void);       /* 0x00186800 */
/* the __si_type_info __tit11RaceBattleT1i8 at 0x004B1340 (base __ti9RaceBasic
 * 0x004B1ED0) */
const type_info &__tft11RaceBattleT1i8(void);               /* 0x001867A8 */
/* the __si_type_info at 0x004B1F80 (base 0x004B1340) */
const type_info &__tf14RaceLinkBattle(void);                /* 0x00186758 */
}
#endif

/* The instances <8> (T137 has the templates) are instantiated by the classes
 * derived from them: their size checks follow those classes.  egcs emits
 * template instances' vtables and type names in reverse order of
 * instantiation; retail's order (RaceLinkBattle, RaceBattleT<8>, the node
 * input, the input, the information, RaceBattleInformationT<8>) needs
 * RaceBattleT<8> instantiated by RaceLinkBattle, not earlier. */

/* vtable 0x0032A868 (15 slots: [0] [2] [3] [4] [10] [11] its own).
 * typeinfo __tf25RaceLinkBattleInformation 0x001868C8 (si, base
 * RaceBattleInformationT<8>).  vptr +0x54.  Size 0x1088 (a stack object of
 * F_00100DB8). */
class RaceLinkBattleInformation : public RaceBattleInformationT<8> {
public:
    /* 6 entries; the option block (RaceInformation +0x28) from node 0's
     * status.  Caller: F_00100DB8. */
    RaceLinkBattleInformation();                        /* 0x001848A8 */
    virtual ~RaceLinkBattleInformation();               /* [ 0] +0x0C 0x00184950 */
    virtual u64  getCourseCode();                       /* [ 2] +0x1C 0x00184C28 courseCode, else the race DB's */
    virtual void setCourseCode(u64 code);               /* [ 3] +0x24 0x00184C50 */
    virtual RaceEntryBase *createEntries();             /* [ 4] +0x2C 0x00184978 new RaceEntryPS2 */
    /* A car whose node is linked gets its entry's car and colour, inputId
     * 0x100 + node and the values the node published; the others are
     * queued (addMissingEntry) for assignCars. */
    virtual void setupEntry(RaceEntryCar *car);         /* [10] +0x5C 0x001849D0 */
    /* assignCars for the missing entries; the linked nodes first in the
     * start order. */
    virtual void setupOrganization(RaceOrganization *org);  /* [11] +0x64 0x00184AD8 */

    u64 courseCode;                         /* 0x1080 0: the race database's */
};
EGCS_SIZE(RaceBattleInformationT<8>, 0x1080);
EGCS_SIZE(RaceLinkBattleInformation, 0x1088);

/* vtable 0x0032A808 (11 slots: [0] [2] [10] its own, [9] RaceInput's).
 * typeinfo __tf19RaceLinkBattleInput 0x00186878 (si, base RaceInput).  vptr
 * +0xC0 (Control's).  Size 0x14C (RaceLinkBattle +0x1AC6C .. +0x1ADB8): the
 * local player's input. */
class RaceLinkBattleInput : public RaceInput {
public:
    RaceLinkBattleInput();                  /* 0x00184C90 */
    virtual ~RaceLinkBattleInput();         /* [ 0] +0x0C 0x00184CC8 */
    virtual void updateSync();              /* [ 2] +0x1C 0x00184CF0 empty (Control's is not run) */
    virtual ControllerConfig *getControllerConfig(); /* [10] +0x5C 0x00184CF8 the settings this node published (status +0x638) */
    /* Inline (u047 matching): the copy egcs emits with the vtable has no
     * callers and sits with the unit's other inline copies; the constructor
     * and setupView expand it. */
    void setStatus(GameOptions *s) { status = s; }  /* 0x00186870 */

    GameOptions *status;                    /* 0x148 this node's published options (u047 LinkNodeStatus) */
};
EGCS_SIZE(RaceLinkBattleInput, 0x14C);

/* vtable 0x0032A7A8 (11 slots: [0] [10] its own).  typeinfo
 * __tf23RaceLinkBattleNodeInput 0x00186800 (si, base RaceInput).  vptr
 * +0xC0.  Size 0x14C (RaceLinkBattle's array: stride 0x14C).  Its
 * constructor is inline (expanded in RaceLinkBattle's, 8 times).  Its
 * destructor is the implicit one: the copy emitted with the vtable
 * ([0] +0x0C 0x00186850) stores no vptr, it only calls ~RaceInput. */
class RaceLinkBattleNodeInput : public RaceInput {
public:
    virtual ControllerConfig *getControllerConfig(); /* [10] +0x5C 0x00184D08 the settings the node published (status +0x638) */
    /* Inline (as RaceLinkBattleInput's; the constructor expands it). */
    void setStatus(GameOptions *s) { status = s; }  /* 0x001867F8 */

    GameOptions *status;                    /* 0x148 the node's published options (u047 LinkNodeStatus) */
};
EGCS_SIZE(RaceLinkBattleNodeInput, 0x14C);

/* vtable 0x0032A418 (56 slots: [0] [5] [8] [24] [25] [26] [27] [28] [30]
 * [33] [35] [43] [46] its own, [45] RaceBasic's).  typeinfo
 * __tf14RaceLinkBattle 0x00186758 (si, base RaceBattleT<8>).  vptr +0.  Size
 * 0x35000 (race_link_battle_create; g_002B7960). */
class RaceLinkBattle : public RaceBattleT<8> {
public:
    static void *operator new(size_t size);                 /* 0x00184D18 the race pool */
    static void  operator delete(void *p, size_t size);     /* 0x00184D40 */
    /* The members (replayPause in mode 0), setRaceMode("RACE_MODE_SINGLE"),
     * dc = &dcBattle, nodeId / viewMode from the session; a driving console
     * uses input (pad slot 0x14), a display console is set up by setupView;
     * node input i gets node i's status, pad slot 0xC + i and record slot
     * i; display becomes the race display. */
    RaceLinkBattle();                                       /* 0x00184E00 */
    virtual ~RaceLinkBattle();                              /* [ 0] +0x0C  0x00185208 the inputs leave the pad manager */
    virtual int  virtual_5();                               /* [ 5] +0x34  0x00185398 1 */
    /* The argument is not used: a driving console runs the race, or (with
     * g_00352CB0) as a replay; a display console with viewCar's view and
     * the camera panels of viewParams. */
    virtual void virtual_8(int replay);                     /* [ 8] +0x4C  0x001853A0 */
    virtual const char *virtual_24();                       /* [24] +0xCC  0x00184D68 "RaceLinkBattle" */
    virtual RaceInput *virtual_25();                        /* [25] +0xD4  0x001857C8 activeInput */
    /* 0x100 + n: node n's input while it is there, else the inert input
     * g_002DFB58 (T413). */
    virtual RaceInput *virtual_26(int id);                  /* [26] +0xDC  0x001857E0 */
    /* The link reset after 601 frames of waiting; returns linkActive. */
    virtual int  virtual_27();                              /* [27] +0xE4  0x00185848 */
    /* RacePS2Base's (RaceBasic's is skipped), the race display, the result,
     * the pause request (CONTROL_START) of a driving console. */
    virtual void virtual_28(float dt);                      /* [28] +0xEC  0x00185920 */
    /* Inline (u047 matching): the copy sits with the inline copies after the
     * static initialiser, before __tf14RaceLinkBattle; RaceBasic's frame while
     * paused is not run (the pause menu runs in updateLink). */
    virtual void virtual_30() {}                            /* [30] +0xFC  0x00186750 */
    virtual void virtual_33();                              /* [33] +0x114 0x00185758 tells the others, RacePS2Base's */
    virtual void virtual_35();                              /* [35] +0x124 0x00185788 RaceBasic's, the session ends */
    virtual void virtual_43();                              /* [43] +0x164 0x001856F0 the icons, given to the result */
    virtual void virtual_46(GSBuffer *buf);                 /* [46] +0x17C 0x00185F78 RacePS2Base's, then updateLink */

    void setActiveInput(RaceInput *in);     /* 0x001857B0 */
    void drawConnectionError();             /* 0x00185AF0 the message g_002C43E8[3] (u047 g_002C43F4) */
    void drawWaitingForResponse();          /* 0x00185BA0 the message g_002C43E8[4] (u047 g_002C43F8; unreferenced) */
    void updateLink(GSBuffer &buf);         /* 0x00185C58 the link state machine, one frame */
    void setupView();                       /* 0x00185FB8 the followed node and the place in the screen wall */

    DynamicsConductorBattleMP dcBattle;     /* 0x0C8C0 RaceBase::dc points here */
    RaceLinkBattleInput       input;        /* 0x1AC6C the local player's (pad slot 0x14) */
    RaceLinkBattleNodeInput   nodeInputs[8];/* 0x1ADB8 one per i.LINK node */
    RaceInput                *activeInput;  /* 0x1B818 what virtual_25 returns */
    CarIcon                   icons[7];     /* 0x1B81C (u047: carDisplays, T184Object) */
    ResultLinkBattle          result;       /* 0x299F8 */
    RacePause                 racePause;    /* 0x2A34C */
    ReplayPause               replayPause;  /* 0x2A5F8 */
    RaceDisplay               display;      /* 0x2AB38 */
    int viewMode;                           /* 0x34FB4 g_00352CA8: 0 this console drives a car, 1..9 the view layouts */
    int nodeId;                             /* 0x34FB8 own i.LINK node */
    int viewCar;                            /* 0x34FBC the node / car this console shows */
    /* this screen's place in the screen wall (setScreenLayout; u047: int[5]) */
    ScreenLayout viewParams;                /* 0x34FC0 */
    /* 0 running, 1 our pause menu, 2 waiting for the others (the command
     * exchange), 3 connection error */
    int state;                              /* 0x34FD4 */
    int linkResult;                         /* 0x34FD8 the last session state (F_0022CD28; 1 also: our pause) */
    int linkActive;                         /* 0x34FDC what virtual_27 returns */
    int pauseRequest;                       /* 0x34FE0 the local player asked for the pause */
    /*  0x34FE4 .. 0x34FFF: alignment */
};
EGCS_SIZE(RaceBattleT<8>, 0xC8C0);
/* The enhanced engine: its conductor 0x1E0 and its display 4 longer: the
 * members after dcBattle sit 0x1E0 later than shown, after display 0x1E4
 * (GT3 0x35000). */
/* (Wave 1: the hud's RaceDisplay (0xA8E8, GT3 0xA47C) and the embedded race classes' growth; the size
 * from tools/sizeprobe.py, the member offsets above stay
 * GT3's.) */
EGCS_SIZE(RaceLinkBattle, 0x35680);

/* RaceClassInfo "RaceLinkBattle" (g_002B7960: size 0x35000, these two). */
RaceBase *race_link_battle_create(void);                    /* 0x00184D78 */
RaceBase *race_link_battle_construct(void *buf, int arg);   /* 0x00184DD8 a RaceLinkBattle in buf (NULL: none) */
/* .ctors entry 0x0028F108: race_class_register(&g_00351E78, &g_002B7960). */
void      race_link_battle_static_init(void);               /* 0x00186728 */

/* (__ti24RaceSolitaireInformation / __ti13RaceSolitaire, which the plan put
 * here, are declared in T139_race_solitaire.h.) */

#endif /* GAME_T138_RACE_LINK_BATTLE_H */
