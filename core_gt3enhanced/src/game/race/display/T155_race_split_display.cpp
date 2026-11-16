#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/game/race/display/T155_race_split_display.cpp"
#pragma implementation "src/game/race/display/T155_race_split_display.cpp"
#pragma implementation "T155_race_split_display.h"
#endif
/* T155_race_split_display.cpp - RaceSplitDisplay (0x00195B88..0x00196490).
 * See include/game/race/display/T155_race_split_display.h.
 *
 * The display of a split screen race (RaceSplitBattle, RaceSolitaire): two
 * RaceDisplays, one per screen.  The set-up, update, draw, inner panel
 * switch, organization and panel switches go to both; the queries and the
 * car numbers go to the "target" display (-1 after construction: the race
 * classes set it before use).  drawTarget draws only the target one.  Every
 * call on the displays is a virtual call in the original (through the
 * displays' vptr, slot offsets in the comments), on the display that the
 * inline getDisplay(i) / getTargetDisplay() give.
 *
 * #pragma implementation (egcs only, first lines): the pair of
 * T155_race_split_display.h's #pragma interface, so the out-of-line copies
 * of the inline accessors are emitted here, at the end (the #line names this
 * file: the SCE driver compiles a temporary .ii, see T530).
 *
 * Data: .rodata 0x0032E620 (the vtable, "16RaceSplitDisplay", then the race
 * display header's type names) and the frame info are T155's own, both
 * referenced.  Its .sdata is one item nothing references: the "" of T157's
 * inline RaceMessageDisplay::clear (read with the race display; -G8), the
 * 8-byte zero slot 0x00352020 between T154's last string "@finish" and
 * T157's own "" 0x00352028 (T156 in between sees no display part and has no
 * .sdata): anchored with the DATA_START below.
 *
 * <new> first: retail's T155 .rodata opens with "bad_alloc" (0x0032E610,
 * nothing references it; <new>'s inline what(), DATA.md) and 6 zero bytes
 * before the vtable (u051's data pass found it; ref/data/u052/pending.md).
 */
// DATA_START 0x0032E610
// DATA_START 0x00352020
#include <new>
#include "game/race/display/race_split_display_internal.h"
#include "game/race/display/T155_race_split_display.h"

// FUNC 0x00195B88 RaceSplitDisplay::RaceSplitDisplay
//   The vptr, the two displays (RaceDisplay::RaceDisplay 0x001909E0; if
//   one throws, the ones built are destroyed again, last first, through
//   their virtual destructor), target -1, then each display's screen
//   number (the inline setDisplayNumber: 1, then 0).  Callers:
//   RaceSplitBattle::RaceSplitBattle, RaceSolitaire::RaceSolitaire.
RaceSplitDisplay::RaceSplitDisplay()
{
    /* implicit: RaceDisplay (constructor of each display) */
    target = -1;
    for (int i = 1; i >= 0; i--)
        displays[i].setDisplayNumber(i);
}

// FUNC 0x00195CC0 RaceSplitDisplay::~RaceSplitDisplay
//   The vptr; the displays destroyed last first through their virtual
//   destructor (slot 21, flags 0); operator delete when the flags ask for
//   it.  Callers: RaceSplitBattle / RaceSolitaire (their constructors'
//   landing pads and their virtual_0, the destructors).
RaceSplitDisplay::~RaceSplitDisplay()
{
    /* implicit: operator delete (deleting destructor) */
}

// FUNC 0x00195D70 RaceSplitDisplay::setOrganization
//   (index: virtual_6.)  Both (slot 6).
void RaceSplitDisplay::setOrganization(RaceOrganization &org)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->setOrganization(org);
}

// FUNC 0x00195DD8 RaceSplitDisplay::virtual_0
//   Both (slot 0, the set-up for a race).  Caller: RaceSolitaire::virtual_8.
void RaceSplitDisplay::virtual_0(RaceOrganization *org, int raceMode, int replay)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->virtual_0(org, raceMode, replay);
}

// FUNC 0x00195E60 RaceSplitDisplay::virtual_1
//   Both (slot 1).
void RaceSplitDisplay::virtual_1()
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->virtual_1();
}

// FUNC 0x00195EB8 RaceSplitDisplay::virtual_2
//   Both (slot 2, the per-frame update).  Callers:
//   RaceSplitBattle::updateDisplay, RaceSolitaire::virtual_28.  dt: the
//   enhanced engine's time step (hud.md C19), passed on.
void RaceSplitDisplay::virtual_2(f32 dt)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->virtual_2(dt);
}

// FUNC 0x00195F10 RaceSplitDisplay::virtual_3
//   Both draw into buf (slot 3).
void RaceSplitDisplay::virtual_3(GSBuffer &buf)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->virtual_3(buf);
}

// FUNC 0x00195F78 RaceSplitDisplay::drawTarget
//   (index: virtual_17.)  The target display draws into buf (slot 3).
//   Callers: RaceSplitBattle::virtual_45, RaceSolitaire::virtual_45.
void RaceSplitDisplay::drawTarget(GSBuffer &buf)
{
    getTargetDisplay()->virtual_3(buf);
}

// FUNC 0x00195FC0 RaceSplitDisplay::innerPanel
//   (index: virtual_4; the setter.)  Both (slot 4).
void RaceSplitDisplay::innerPanel(bool on)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->innerPanel(on);
}

// FUNC 0x00196028 RaceSplitDisplay::innerPanel
//   (index: virtual_5; the const getter.)  The target's (slot 5).
bool RaceSplitDisplay::innerPanel() const
{
    return getTargetDisplay()->innerPanel();
}

// FUNC 0x00196070 RaceSplitDisplay::setTargetCarNumber
//   (index: virtual_8.)  The target's (slot 8).  Callers:
//   RaceSplitBattle::updateDisplay, RaceSolitaire::virtual_28.
void RaceSplitDisplay::setTargetCarNumber(int n)
{
    getTargetDisplay()->setTargetCarNumber(n);
}

// FUNC 0x001960B8 RaceSplitDisplay::setPlayerCarNumber
//   (index: virtual_7.)  The target's (slot 7).  Callers:
//   RaceSplitBattle::updateDisplay, RaceSolitaire::virtual_28.
void RaceSplitDisplay::setPlayerCarNumber(int n)
{
    getTargetDisplay()->setPlayerCarNumber(n);
}

// FUNC 0x00196100 RaceSplitDisplay::virtual_9
//   The target's (slot 9; the view camera's mount, u046).  Callers:
//   RaceSplitBattle::updateDisplay, RaceSolitaire::virtual_28.
void RaceSplitDisplay::virtual_9(int mount)
{
    getTargetDisplay()->virtual_9(mount);
}

// FUNC 0x00196148 RaceSplitDisplay::getMirrorObject
//   (index: virtual_15.)  The target's (slot 15).
void *RaceSplitDisplay::getMirrorObject()
{
    return getTargetDisplay()->getMirrorObject();
}

// FUNC 0x00196190 RaceSplitDisplay::setTopLeftPanel
//   (index: virtual_10.)  Both (slot 10).
void RaceSplitDisplay::setTopLeftPanel(bool on)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->setTopLeftPanel(on);
}

// FUNC 0x001961F8 RaceSplitDisplay::setTopRightPanel
//   (index: virtual_11.)  Both (slot 11).
void RaceSplitDisplay::setTopRightPanel(bool on)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->setTopRightPanel(on);
}

// FUNC 0x00196260 RaceSplitDisplay::setBottomLeftPanel
//   (index: virtual_12.)  Both (slot 12).
void RaceSplitDisplay::setBottomLeftPanel(bool on)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->setBottomLeftPanel(on);
}

// FUNC 0x001962C8 RaceSplitDisplay::setBottomRightPanel
//   (index: virtual_13.)  Both (slot 13).
void RaceSplitDisplay::setBottomRightPanel(bool on)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->setBottomRightPanel(on);
}

// FUNC 0x00196330 RaceSplitDisplay::setCenterPanel
//   (index: virtual_14.)  Both (slot 14).
void RaceSplitDisplay::setCenterPanel(bool on)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->setCenterPanel(on);
}

// FUNC 0x00196398 RaceSplitDisplay::getTargetDisplay (getTargetDisplay__16RaceSplitDisplay)
/* inline in game/race/display/T155_race_split_display.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001963B8 RaceSplitDisplay::getTargetDisplay (getTargetDisplay__C16RaceSplitDisplay)
/* inline in game/race/display/T155_race_split_display.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001963D8 RaceSplitDisplay::getDisplay
/* inline in game/race/display/T155_race_split_display.h; egcs emits this copy (#pragma implementation) */

// FUNC 0x001963F0 RaceSplitDisplay::setTargetDisplay
/* inline in game/race/display/T155_race_split_display.h; egcs emits this copy (#pragma implementation) */

#ifndef BOOT_OLD_GCC
// FUNC 0x00196400 __tf16RaceSplitDisplay
//   __si_type_info at 0x004B21E0, base __ti15RaceDisplayBase (built by
//   T154's copy 0x00195B48).
extern "C" const type_info &__tf16RaceSplitDisplay(void)
{
    if (__ti16RaceSplitDisplay[0] == 0) {
        __tf15RaceDisplayBase();
        __rtti_si(__ti16RaceSplitDisplay, "16RaceSplitDisplay", (const type_info *)__ti15RaceDisplayBase);
    }
    return *(const type_info *)__ti16RaceSplitDisplay;
}

// FUNC 0x00196450 __tf15RaceDisplayBase
//   T155's copy of the RaceDisplayBase typeinfo function (the same code as
//   0x00195B48, its own copy of the name string, 0x0032E780); never
//   called.  Weak: T154's copy links too.
extern "C" __attribute__((weak)) const type_info &__tf15RaceDisplayBase(void)
{
    if (__ti15RaceDisplayBase[0] == 0)
        __rtti_user(__ti15RaceDisplayBase, "15RaceDisplayBase");
    return *(const type_info *)__ti15RaceDisplayBase;
}
#endif

/* ======================================================================== */
/* The enhanced engine: RaceSplitDisplayBase, RacePOD2PDisplay              */
/* ======================================================================== */

/* Lupo 2's split screen displays (hud.md C22; T155_race_pod2p_display.h).
 * Every call on a display is a virtual call through getDisplay(i) /
 * getTargetDisplay(), as Lupo 2's base makes them. */
#include "game/race/display/T155_race_pod2p_display.h"
#include "game/race/core/T125_race_organization.h"    /* RaceOrganization::eventQueue */
#include "game/race/core/T122_race_event_queue.h"     /* RaceEventQueue::post, RACE_EVENT_END */
#include "engine/pdistd/T533_printf.h"               /* printf (the log line) */

// RaceSplitDisplayBase::RaceSplitDisplayBase (the enhanced engine)
//   Lupo 2 L2 0x00193720.  No data.
RaceSplitDisplayBase::RaceSplitDisplayBase()
{
}

// RaceSplitDisplayBase::~RaceSplitDisplayBase (the enhanced engine)
//   Lupo 2 L2 0x00193738.
RaceSplitDisplayBase::~RaceSplitDisplayBase()
{
}

// RaceSplitDisplayBase::virtual_0 .. getMirrorObject, drawTarget (the
// enhanced engine)
//   Lupo 2 L2 0x00193770..0x00193F50 (the twins of RaceSplitDisplay's,
//   above): both displays or the target one, through getDisplay(i) /
//   getTargetDisplay().
void RaceSplitDisplayBase::setOrganization(RaceOrganization &org)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->setOrganization(org);
}

void RaceSplitDisplayBase::virtual_0(RaceOrganization *org, int raceMode, int replay)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->virtual_0(org, raceMode, replay);
}

void RaceSplitDisplayBase::virtual_1()
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->virtual_1();
}

void RaceSplitDisplayBase::virtual_2(f32 dt)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->virtual_2(dt);
}

void RaceSplitDisplayBase::virtual_3(GSBuffer &buf)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->virtual_3(buf);
}

void RaceSplitDisplayBase::drawTarget(GSBuffer &buf)
{
    getTargetDisplay()->virtual_3(buf);
}

void RaceSplitDisplayBase::innerPanel(bool on)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->innerPanel(on);
}

bool RaceSplitDisplayBase::innerPanel() const
{
    return getTargetDisplay()->innerPanel();
}

void RaceSplitDisplayBase::setTargetCarNumber(int n)
{
    getTargetDisplay()->setTargetCarNumber(n);
}

void RaceSplitDisplayBase::setPlayerCarNumber(int n)
{
    getTargetDisplay()->setPlayerCarNumber(n);
}

void RaceSplitDisplayBase::virtual_9(int mount)
{
    getTargetDisplay()->virtual_9(mount);
}

void *RaceSplitDisplayBase::getMirrorObject()
{
    return getTargetDisplay()->getMirrorObject();
}

void RaceSplitDisplayBase::setTopLeftPanel(bool on)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->setTopLeftPanel(on);
}

void RaceSplitDisplayBase::setTopRightPanel(bool on)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->setTopRightPanel(on);
}

void RaceSplitDisplayBase::setBottomLeftPanel(bool on)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->setBottomLeftPanel(on);
}

void RaceSplitDisplayBase::setBottomRightPanel(bool on)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->setBottomRightPanel(on);
}

void RaceSplitDisplayBase::setCenterPanel(bool on)
{
    for (int i = 0; i < 2; i++)
        getDisplay(i)->setCenterPanel(on);
}

// RacePOD2PDisplay::RacePOD2PDisplay (the enhanced engine)
//   Lupo 2 L2 0x001B41D0: the two RacePODDisplays, not ended, each
//   display's screen number (1, then 0); no target yet (the race sets it).
RacePOD2PDisplay::RacePOD2PDisplay()
{
    /* implicit: RaceSplitDisplayBase, RacePODDisplay (constructors) */
    target = -1;
    ended = 0;
    for (int i = 1; i >= 0; i--)
        displays[i].setDisplayNumber(i);
}

// RacePOD2PDisplay::~RacePOD2PDisplay (the enhanced engine)
//   Lupo 2 L2 0x001B4318: the displays, last first.
RacePOD2PDisplay::~RacePOD2PDisplay()
{
}

// RacePOD2PDisplay::getDisplay / getTargetDisplay (the enhanced engine)
//   Lupo 2 L2 0x002C98F0 / 0x002C9908.
RaceDisplay *RacePOD2PDisplay::getDisplay(int i)
{
    return &displays[i];
}

RaceDisplay *RacePOD2PDisplay::getTargetDisplay()
{
    return &displays[target];
}

const RaceDisplay *RacePOD2PDisplay::getTargetDisplay() const
{
    return &displays[target];
}

// RacePOD2PDisplay::virtual_0 (the enhanced engine)
//   Lupo 2 L2 0x001B4498 (hud.md C22): both displays' set-up, then their
//   disqualification messages hold (stoppers); the race not ended.
void RacePOD2PDisplay::virtual_0(RaceOrganization *org, int raceMode, int replay)
{
    RaceSplitDisplayBase::virtual_0(org, raceMode, replay);
    for (int i = 1; i >= 0; i--)
        displays[i].setStopper();
    ended = 0;
}

// RacePOD2PDisplay::virtual_2 (the enhanced engine)
//   Lupo 2 L2 0x001B43F8 (hud.md C22): both displays' update; once both
//   players' disqualification messages hold, the end of the race (event 1)
//   is posted, once.
void RacePOD2PDisplay::virtual_2(f32 dt)
{
    RaceSplitDisplayBase::virtual_2(dt);
    if (ended != 0)
        return;
    for (int i = 0; i < 2; i++) {
        if (!displays[i].isStopped())
            return;
    }
    displays[0].getOrganization()->eventQueue.post(RACE_EVENT_END, 0, 0);
    ended = 1;
    printf("hud: both Pods disqualified: the race ends\n");
}
