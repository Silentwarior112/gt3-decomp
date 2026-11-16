/* T155_race_pod2p_display.h - the enhanced engine: RaceSplitDisplayBase and
 * RacePOD2PDisplay, Lupo 2's split screen displays (ref/enhanced/survey/
 * hud.md C22), defined in T155_race_split_display.cpp.
 *
 * Lupo 2 splits its RaceSplitDisplay into an abstract RaceSplitDisplayBase
 * (every RaceDisplayBase slot forwarded to both screens' displays, or to the
 * target one, through a virtual getDisplay(i)) and the RaceSplitDisplay of
 * two RaceDisplays; the Pod's two-player race has its own RacePOD2PDisplay
 * on the base, with two RacePODDisplays (T141).  GT3's RaceSplitDisplay
 * (T155_race_split_display.h) keeps its own class and forwarding: its
 * behaviour is the same as Lupo 2's refactored one, so only the Pod's
 * display uses the base here.  Nothing of GT3 builds these: RacePOD2P (the
 * race area's, Lupo 2 L2 0x0018D6A0) constructs the Pod's display.
 *
 * A header of its own (not T155_race_split_display.h, which retail's race
 * modes read without the time trial's display classes). */
#ifndef GAME_T155_RACE_POD2P_DISPLAY_H
#define GAME_T155_RACE_POD2P_DISPLAY_H

#include "game/game.h"
#include "game/race/display/T154_race_display.h"             /* base RaceDisplayBase, RaceDisplay */
#include "game/race/display/T141_race_time_trial_display.h"  /* RacePODDisplay (held by value) */

class RaceOrganization;                     /* T125_race_organization.h (pointers / references only) */
class GSBuffer;                             /* T514_gs_buffer.h */

/* Lupo 2's "20RaceSplitDisplayBase" (vtable L2 0x0032F1A8; constructor
 * L2 0x00193720, destructor L2 0x00193738, getCurrentDisplay L2 0x002C6820 /
 * 0x002C6850): the RaceDisplayBase slots of a split screen display.  The
 * set-up, the update, the draw, the inner panel switch, the organization
 * and the panel switches go to both displays, the queries and the car
 * numbers to the target one, drawTarget draws the target one (as GT3's
 * RaceSplitDisplay).  The displays come from the derived class.  No data
 * (Lupo 2 keeps the target index here, at +4; the derived classes keep it
 * here, so GT3's RaceSplitDisplay could take this base without moving its
 * members).  Size 4. */
class RaceSplitDisplayBase : public RaceDisplayBase {
public:
    RaceSplitDisplayBase();                 /* L2 0x00193720 */
    virtual void virtual_0(RaceOrganization *org, int raceMode, int replay);  /* [ 0] both */
    virtual void virtual_1();               /* [ 1] both */
    virtual void virtual_2(f32 dt);         /* [ 2] both */
    virtual void virtual_3(GSBuffer &buf);  /* [ 3] both */
    virtual void innerPanel(bool on);       /* [ 4] both */
    virtual bool innerPanel() const;        /* [ 5] the target's */
    virtual void setOrganization(RaceOrganization &org);   /* [ 6] both */
    virtual void setPlayerCarNumber(int n); /* [ 7] the target's */
    virtual void setTargetCarNumber(int n); /* [ 8] the target's */
    virtual void virtual_9(int mount);      /* [ 9] the target's */
    virtual void setTopLeftPanel(bool on);  /* [10] both */
    virtual void setTopRightPanel(bool on); /* [11] both */
    virtual void setBottomLeftPanel(bool on);   /* [12] both */
    virtual void setBottomRightPanel(bool on);  /* [13] both */
    virtual void setCenterPanel(bool on);   /* [14] both */
    virtual void *getMirrorObject();        /* [15] the target's */
    virtual ~RaceSplitDisplayBase();        /* [16] L2 0x00193738 */
    virtual void drawTarget(GSBuffer &buf); /* [17] the target display's virtual_3 */
    /* The displays of the two screens (Lupo 2 slots 18 / 19) and the
     * target one (Lupo 2's getCurrentDisplay, here virtual: the target index
     * is the derived class's). */
    virtual RaceDisplay *getDisplay(int i) = 0;                     /* [18] */
    virtual RaceDisplay *getTargetDisplay() = 0;                    /* [19] */
    virtual const RaceDisplay *getTargetDisplay() const = 0;        /* [20] */
};
EGCS_SIZE(RaceSplitDisplayBase, 4);

/* Lupo 2's "16RacePOD2PDisplay" (vtable L2 0x003305B0; constructor
 * L2 0x001B41D0, destructor L2 0x001B4318, virtual_0 L2 0x001B4498,
 * virtual_2 L2 0x001B43F8, getDisplay L2 0x002C98F0 / 0x002C9908; built by
 * RacePOD2P's constructor L2 0x0018D6A0): the Pod race's split screen, two
 * RacePODDisplays whose disqualification messages hold (their stoppers);
 * once both players' messages hold (both are disqualified), the race ends
 * (event 1, once).  The race area sets the target display before use (as
 * GT3's RaceSplitBattle does with RaceSplitDisplay). */
class RacePOD2PDisplay : public RaceSplitDisplayBase {
public:
    /* The two displays (screens 1, then 0), no target, not ended. */
    RacePOD2PDisplay();                     /* L2 0x001B41D0 */
    /* Both displays' set-up, their messages' stoppers on, not ended. */
    virtual void virtual_0(RaceOrganization *org, int raceMode, int replay);  /* [ 0] L2 0x001B4498 */
    /* Both displays' update, then the end of the race when both hold. */
    virtual void virtual_2(f32 dt);         /* [ 2] L2 0x001B43F8 */
    virtual ~RacePOD2PDisplay();            /* [16] L2 0x001B4318 */
    virtual RaceDisplay *getDisplay(int i);                         /* [18] L2 0x002C98F0 */
    virtual RaceDisplay *getTargetDisplay();                        /* [19] */
    virtual const RaceDisplay *getTargetDisplay() const;            /* [20] */

    void setTargetDisplay(int i) { target = i; }
    RacePODDisplay *getPODDisplay(int i) { return &displays[i]; }

    /*  vptr                                   0x00000 */
    RacePODDisplay displays[2];             /* 0x00004 one per screen (Lupo 2 +8, stride 0xA908) */
    int            target;                  /* 0x1523C the display the queries go to, -1 */
    int            ended;                   /* 0x15240 1: the end of the race is posted (Lupo 2
                                               +0x15218) */
};
EGCS_SIZE(RacePOD2PDisplay, 0x15244);

#endif /* GAME_T155_RACE_POD2P_DISPLAY_H */
