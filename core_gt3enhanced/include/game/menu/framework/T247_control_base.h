/* T247_control_base.h - gt2k_menu::ControlBase (translation unit T247, unit
 * u071), 0x001CB9E8..0x001CBE30.
 *
 * The base of every menu controller: two MenuControls attached to the pad
 * manager's slots 0x14 / 0x15 (attach / detach), an idle-frame counter with
 * an optional timeout (waitFrame, once a frame: the controllers' loops end
 * once timedOut is set), the screen saver after 108000 idle frames (30
 * minutes) and a debug pause on the second pad.  The controllers that have
 * a PageManager (OptionControl, GtmodeControl, LoggerControl, ...) point
 * pageManager at it. */
#ifndef GAME_T247_CONTROL_BASE_H
#define GAME_T247_CONTROL_BASE_H

#include "game/game.h"
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl (control1, control2) */

namespace gt2k_menu {
class PageManager;                          /* T211 (pointers only) */
class RenderBase;                           /* T246 */

/* vtable 0x00336F48 (2 slots).  typeinfo __tfQ29gt2k_menu11ControlBase
 * 0x001CBDF0 (__user_type_info 0x003533F8 gp-0x45F8, no bases).  0x288 bytes
 * of data, vptr +0x288; size 0x28C. */
class ControlBase {
public:
    ControlBase();                          /* 0x001CB9E8 */
    virtual ~ControlBase();                 /* [0] +0x0C 0x001CBA70 */
    virtual void waitFrame();               /* [1] +0x14 0x001CBD30 a held button of pad 1 restarts
                                               the idle count; one vsync; frameCount++; timedOut once
                                               past timeout */

    void attach();                          /* 0x001CBAD8 the pads to slots 0x14 / 0x15 (PadManager's
                                               steering[0] / [1]), the menu signal cleared */
    void detach();                          /* 0x001CBB30 */
    void debugPause(RenderBase *render);    /* 0x001CBB80 pad 2: R2 + L2 + left held and triangle
                                               pressed (0x20000 + 0x4000 + 4, 0x200) pause the render
                                               until the same again */
    void updateScreenSaver(RenderBase *render); /* 0x001CBCF8 the expanded RenderBase::setScreenSaver:
                                               on after 108000 idle frames */
    /* The accessors are inline in the original: no callers, and
     * 0x001CBD98..0x001CBDE8 are egcs's out-of-line copies before the
     * typeinfo function; the controllers expand resetFrameCount / setTimeout
     * (T010 runDemo, runILink, runAttract, u007).  Declared in reverse
     * address order: egcs writes a class's in-class inline members out in
     * reverse declaration order (u071's data pass: T247's frame info,
     * 0x002CD498, has getPageManager's FDE first).  setTimeout's body comes
     * before resetFrameCount's then, so it cannot call it inline (egcs
     * inlines only bodies already saved): it clears the two fields itself. */
    MenuControl *getControl1() { return &control1; } /* 0x001CBDE8 */
    MenuControl *getControl2() { return &control2; } /* 0x001CBDE0 */
    void setTimeout(int frames)                     /* 0x001CBDD0 also clears timedOut and the count */
    {
        timeout = frames;
        frameCount = 0;
        timedOut = 0;
    }
    void resetFrameCount()                          /* 0x001CBDC0 also clears timedOut */
    {
        /* this order: sched2 puts timedOut's store first (u071) */
        frameCount = 0;
        timedOut = 0;
    }
    int getFrameCount() { return frameCount; }      /* 0x001CBDB8 */
    void countFrame() { frameCount = frameCount + 1; } /* 0x001CBDA8 */
    int F_001CBDA0() { return unk280; }             /* 0x001CBDA0 unk280 */
    PageManager *getPageManager() { return pageManager; } /* 0x001CBD98 (F_001CBD98) */

    MenuControl  control1;                  /* 0x000 pad 1 (pad manager slot 0x14) */
    MenuControl  control2;                  /* 0x134 pad 2 (slot 0x15) */
    int          unk268;                    /* 0x268 */
    int          frameCount;                /* 0x26C frames since a button of pad 1 was last held */
    int          timedOut;                  /* 0x270 set once frameCount > timeout (and by the
                                               controllers to end their loop) */
    int          timeout;                   /* 0x274 frames; 0: never */
    int          unk278;                    /* 0x278 */
    int          unk27C;                    /* 0x27C */
    int          unk280;                    /* 0x280 */
    PageManager *pageManager;               /* 0x284 the controller's page manager (NULL: none) */
    /*           vptr                          0x288 */
};
EGCS_SIZE(gt2k_menu::ControlBase, 0x28C);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu11ControlBase(void);  /* 0x001CBDF0 */
extern u32 __tiQ29gt2k_menu11ControlBase[2];    /* 0x003533F8 gp-0x45F8 its __user_type_info */
}
#endif

#endif /* GAME_T247_CONTROL_BASE_H */
