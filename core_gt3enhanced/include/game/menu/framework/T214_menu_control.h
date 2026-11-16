/* T214_menu_control.h - gt2k_menu::MenuControl (translation unit T214, unit
 * u065), 0x001BA970..0x001BB1D8.
 *
 * One pad's menu input over Control (T474).  Every frame its slot 9 maps the
 * raw pad bits onto Control's button mask (0x200 triangle, 0x400 circle,
 * 0x800 cross, 0x1000 square, 0x2000 L1, 0x4000 L2, 0x8000 L3, 0x10000 R1,
 * 0x20000 R2, 0x40000 R3 over Control's low byte), reads the right stick and
 * runs updateKeys(): the auto-repeated directions (faster when a direction
 * is pressed hard), the eight "delayed" directions (a direction newly held
 * for 5 frames, diagonals included), decide / cancel and the auto-repeated
 * shoulder buttons.  The page code reads the result through the get*
 * accessors (Lupo 2 names) or the members directly.
 */
#ifndef GAME_T214_MENU_CONTROL_H
#define GAME_T214_MENU_CONTROL_H

#include "game/game.h"
#include "engine/input/T474_control.h"              /* base Control */

namespace gt2k_menu {

/* The bits emulatePressure adds to Control's button word (getButtons /
 * getButtonsOn / getRepeat masks) over its low byte (ControlButton, T474),
 * one per raw PS pad button. */
enum MenuButton {
    MENU_BUTTON_TRIANGLE = 0x00200,
    MENU_BUTTON_CIRCLE   = 0x00400,
    MENU_BUTTON_CROSS    = 0x00800,
    MENU_BUTTON_SQUARE   = 0x01000,
    MENU_BUTTON_L1       = 0x02000,
    MENU_BUTTON_L2       = 0x04000,
    MENU_BUTTON_L3       = 0x08000,
    MENU_BUTTON_R1       = 0x10000,
    MENU_BUTTON_R2       = 0x20000,
    MENU_BUTTON_R3       = 0x40000
};

/* vtable 0x00333FA0 (10 slots: [0] and [9] its own, [1..8] Control's; vptr
 * +0xC0, Control's).  typeinfo __tfQ29gt2k_menu11MenuControl 0x001BB168
 * (__si_type_info 0x004B24F0, base Control).  Size 0x134 (the page
 * controls' members start at +0x134). */
class MenuControl : public Control {
public:
    MenuControl();                          /* 0x001BA970 decideButtons 0x40, the rest 0 (l1..r2 not set) */
    /*  [0] +0x0C 0x001BB1B8 the implicit destructor: only Control::~Control, no vptr store */
    /* Control's emulatePressure(), then the menu buttons, the right stick
     * (analogRVertical) and updateKeys(): the menu's per-frame input. */
    virtual void emulatePressure();         /* [9] +0x54 0x001BAF18 */

    /* Control::getRepeat(dir, delay, interval), with interval 1 while the
     * direction's pressure is 250 or more. */
    int  repeatDir(u32 dir, int delay, int interval);  /* 0x001BAA08 */
    void updateKeys();                      /* 0x001BAA70 */
    int  getPrev(int horizontal) const;     /* 0x001BB018 horizontal ? left : up */
    int  getNext(int horizontal) const;     /* 0x001BB030 horizontal ? right : down */
    /* The accessors are inline in the original: no callers, and
     * 0x001BB048..0x001BB160 are egcs's out-of-line copies before the
     * typeinfo function; the pages read the members (PageManager::update
     * expands anyKey, FreeButton / MultiButton getDecide, u064 / u068).
     * Declared in the members' order, anyKey last: egcs writes a class's
     * in-class inline members in reverse declaration order, and retail's
     * T214 has anyKey first, getAnalogRVertical last (its frame info). */
    f32 getAnalogRVertical() const { return analogRVertical; } /* 0x001BB160 */
    int getUp() const { return up; }                /* 0x001BB158 */
    int getDown() const { return down; }            /* 0x001BB150 */
    int getLeft() const { return left; }            /* 0x001BB148 */
    int getRight() const { return right; }          /* 0x001BB140 */
    int getL1() const { return l1; }                /* 0x001BB138 */
    int getR1() const { return r1; }                /* 0x001BB130 */
    int getL2() const { return l2; }                /* 0x001BB128 */
    int getR2() const { return r2; }                /* 0x001BB120 */
    int getDecide() const { return decide; }        /* 0x001BB118 */
    int getCancel() const { return cancel; }        /* 0x001BB110 */
    int getUpPress() const { return upPress; }      /* 0x001BB108 */
    int getDownPress() const { return downPress; }  /* 0x001BB100 */
    int getLeftPress() const { return leftPress; }  /* 0x001BB0F8 */
    int getRightPress() const { return rightPress; } /* 0x001BB0F0 */
    int getLastKey() const { return lastKey; }      /* 0x001BB0E8 */
    void setDecideButtons(unsigned int mask) { decideButtons = mask; } /* 0x001BB0E0 */
    int getDelayedUpLeft() const { return delayedUpLeft; } /* 0x001BB0D8 */
    int getDelayedUpRight() const { return delayedUpRight; } /* 0x001BB0D0 */
    int getDelayedDownLeft() const { return delayedDownLeft; } /* 0x001BB0C8 */
    int getDelayedDownRight() const { return delayedDownRight; } /* 0x001BB0C0 */
    int getDelayedUp() const { return delayedUp; }  /* 0x001BB0B8 */
    int getDelayedDown() const { return delayedDown; } /* 0x001BB0B0 */
    int getDelayedLeft() const { return delayedLeft; } /* 0x001BB0A8 */
    int getDelayedRight() const { return delayedRight; } /* 0x001BB0A0 */
    int anyKey() const                              /* 0x001BB048 (? name) any direction, decide or cancel */
    {
        int any = 0;
        if (up != 0 || down != 0 || left != 0 || right != 0 || decide != 0 || cancel != 0)
            any = 1;
        return any;
    }

    f32 analogRVertical;                    /* 0xC4 -1 .. 1 (the curve g_003416F8 of the low byte of
                                               channels[1], the right stick's vertical axis) */
    int up;                                 /* 0xC8 auto-repeated directions */
    int down;                               /* 0xCC */
    int left;                               /* 0xD0 */
    int right;                              /* 0xD4 */
    int l1;                                 /* 0xD8 auto-repeated shoulder buttons */
    int r1;                                 /* 0xDC (0 when l1 is set) */
    int l2;                                 /* 0xE0 (not updated while L1 / R1 is held) */
    int r2;                                 /* 0xE4 (0 when l2 is set) */
    int decide;                             /* 0xE8 a decideButtons button pressed this frame */
    int cancel;                             /* 0xEC 0x80 pressed this frame (not with decide) */
    int upPress;                            /* 0xF0 pressure >= 251 */
    int downPress;                          /* 0xF4 */
    int leftPress;                          /* 0xF8 */
    int rightPress;                         /* 0xFC */
    int lastKey;                            /* 0x100 0x40 decide, 0x80 cancel, 0 */
    u32 decideButtons;                      /* 0x104 (0x40) */
    int vCount;                             /* 0x108 frames held: < 0 up, > 0 down */
    int hCount;                             /* 0x10C < 0 left, > 0 right */
    int dirMask;                            /* 0x110 the last direction (with its neighbours) */
    int delayedUpLeft;                      /* 0x114 a newly entered direction, after 5 frames */
    int delayedUpRight;                     /* 0x118 */
    int delayedDownLeft;                    /* 0x11C */
    int delayedDownRight;                   /* 0x120 */
    int delayedUp;                          /* 0x124 */
    int delayedDown;                        /* 0x128 */
    int delayedLeft;                        /* 0x12C */
    int delayedRight;                       /* 0x130 */
};

} /* namespace gt2k_menu */

EGCS_SIZE(gt2k_menu::MenuControl, 0x134);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" void *__tfQ29gt2k_menu11MenuControl(void);   /* 0x001BB168 */
#endif

#endif /* GAME_T214_MENU_CONTROL_H */
