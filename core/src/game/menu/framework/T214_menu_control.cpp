/* T214 - gt2k_menu::MenuControl (0x001BA970..0x001BB1D8).
 *
 * One pad's menu input over Control (T474): every frame emulatePressure maps
 * the raw pad bits onto Control's button word (MenuButton), then
 * updateKeys() derives the auto-repeated directions (faster when a direction
 * is pressed hard), the eight "delayed" directions (a direction newly held
 * for 5 frames, diagonals included), decide / cancel and the auto-repeated
 * shoulder buttons.
 */
/* Retail's .rodata: the vtable and MenuControl's type name, nothing else
 * (Control has its key function in T474). */
#include "game/menu/framework/menu_item_internal.h"
#include "engine/input/T472_analog_curve.h"         /* g_003416F8 (the analog stick curve) */
#include "engine/input/T474_control.h"              /* base Control, ControlButton, PADRAW_*, CHAN_* */
#include "game/menu/framework/T214_menu_control.h"

namespace gt2k_menu {

// FUNC 0x001BA970 gt2k_menu::MenuControl::MenuControl
/* Control() (0x0022DA98) first.  l1 .. r2 (+0xD8..+0xE4) are not set. */
MenuControl::MenuControl()
    : analogRVertical(0), up(0), down(0), left(0), right(0),
      decide(0), cancel(0), upPress(0), downPress(0), leftPress(0), rightPress(0),
      lastKey(0), decideButtons(CONTROL_DECIDE), vCount(0), hCount(0), dirMask(0),
      delayedUpLeft(0), delayedUpRight(0), delayedDownLeft(0), delayedDownRight(0),
      delayedUp(0), delayedDown(0), delayedLeft(0), delayedRight(0)
{
    /* implicit: Control (base constructor) */
}

// FUNC 0x001BAA08 gt2k_menu::MenuControl::repeatDir
/* Control::getRepeat of one d-pad direction, repeating every frame while
 * its pressure is PRESSURE_FULL or more (a signed compare of the channel). */
int MenuControl::repeatDir(u32 dir, int delay, int interval)
{
    int pressure = 0;

    if (dir == CONTROL_LEFT)
        pressure = channels[CHAN_PRESS_LEFT];
    else if (dir == CONTROL_RIGHT)
        pressure = channels[CHAN_PRESS_RIGHT];
    else if (dir == CONTROL_UP)
        pressure = channels[CHAN_PRESS_UP];
    else if (dir == CONTROL_DOWN)
        pressure = channels[CHAN_PRESS_DOWN];

    if (pressure >= PRESSURE_FULL)
        interval = 1;
    return getRepeat(dir, delay, interval);
}

// FUNC 0x001BAA70 gt2k_menu::MenuControl::updateKeys
/* The button tests are Control's inline getNow / getOn: retail keeps
 * decideButtons and the L1 | R1 mask in $s0 over the call (the inline's
 * argument), tests CANCEL with andi / sltu, and still calls getButtons for
 * the last test (L2 | R2), whose result nothing uses. */
void MenuControl::updateKeys()
{
    /* frames a direction has been held: vCount < 0 up, > 0 down */
    if (getNow(CONTROL_UP))
        vCount--;
    else if (getNow(CONTROL_DOWN))
        vCount++;
    else
        vCount = 0;
    if (getNow(CONTROL_LEFT))
        hCount--;
    else if (getNow(CONTROL_RIGHT))
        hCount++;
    else
        hCount = 0;

    /* the direction held for DIR_HOLD_FRAMES frames, diagonals first */
    int dir;
    if (vCount <= -DIR_HOLD_FRAMES && hCount <= -DIR_HOLD_FRAMES)
        dir = DIR_UP_LEFT;
    else if (vCount <= -DIR_HOLD_FRAMES && hCount >= DIR_HOLD_FRAMES)
        dir = DIR_UP_RIGHT;
    else if (vCount >= DIR_HOLD_FRAMES && hCount <= -DIR_HOLD_FRAMES)
        dir = DIR_DOWN_LEFT;
    else if (vCount >= DIR_HOLD_FRAMES && hCount >= DIR_HOLD_FRAMES)
        dir = DIR_DOWN_RIGHT;
    else if (vCount <= -DIR_HOLD_FRAMES)
        dir = DIR_UP;
    else if (vCount >= DIR_HOLD_FRAMES)
        dir = DIR_DOWN;
    else if (hCount <= -DIR_HOLD_FRAMES)
        dir = DIR_LEFT;
    else if (hCount >= DIR_HOLD_FRAMES)
        dir = DIR_RIGHT;
    else
        dir = 0;

    /* delayed*: the direction is new (not the last one or a neighbour) */
    int last = dirMask;
    dirMask = dir;
    int entered = dir & ~last;
    delayedUpLeft    = entered & 1;
    delayedUpRight   = (entered >> 1) & 1;
    delayedDownLeft  = (entered >> 2) & 1;
    delayedDownRight = (entered >> 3) & 1;
    delayedUp        = (entered >> 4) & 1;
    delayedDown      = (entered >> 5) & 1;
    delayedLeft      = (entered >> 6) & 1;
    delayedRight     = (u32)entered >> 7;

    /* remember the neighbours too, so sliding to an adjacent direction does
     * not count as a new one */
    if (dir & DIR_UP_LEFT)
        dirMask = dir | DIR_UP | DIR_LEFT;
    else if (dir & DIR_UP_RIGHT)
        dirMask = dir | DIR_UP | DIR_RIGHT;
    else if (dir & DIR_DOWN_LEFT)
        dirMask = dir | DIR_DOWN | DIR_LEFT;
    else if (dir & DIR_DOWN_RIGHT)
        dirMask = dir | DIR_DOWN | DIR_RIGHT;
    else if (dir & DIR_UP)
        dirMask = dir | DIR_UP_LEFT | DIR_UP_RIGHT;
    else if (dir & DIR_DOWN)
        dirMask = dir | DIR_DOWN_LEFT | DIR_DOWN_RIGHT;
    else if (dir & DIR_LEFT)
        dirMask = dir | DIR_UP_LEFT | DIR_DOWN_LEFT;
    else if (dir & DIR_RIGHT)
        dirMask = dir | DIR_UP_RIGHT | DIR_DOWN_RIGHT;

    cancel = 0;
    decide = 0;
    r2 = 0;
    l2 = 0;
    r1 = 0;
    l1 = 0;
    right = 0;
    left = 0;
    down = 0;
    up = 0;
    upPress    = channels[CHAN_PRESS_UP] > PRESSURE_FULL;      /* unsigned */
    downPress  = channels[CHAN_PRESS_DOWN] > PRESSURE_FULL;
    leftPress  = channels[CHAN_PRESS_LEFT] > PRESSURE_FULL;
    rightPress = channels[CHAN_PRESS_RIGHT] > PRESSURE_FULL;

    /* one direction a frame: vertical first */
    up = repeatDir(CONTROL_UP, REPEAT_DELAY, REPEAT_INTERVAL);
    if (up == 0)
        down = repeatDir(CONTROL_DOWN, REPEAT_DELAY, REPEAT_INTERVAL);
    else
        down = 0;
    if (up != 0)
        return;
    if (down != 0)
        return;

    left = repeatDir(CONTROL_LEFT, REPEAT_DELAY, REPEAT_INTERVAL);
    if (left == 0)
        right = repeatDir(CONTROL_RIGHT, REPEAT_DELAY, REPEAT_INTERVAL);
    else
        right = 0;

    /* no buttons while a direction is held */
    if (getNow(CONTROL_UP | CONTROL_DOWN))
        return;
    if (getNow(CONTROL_LEFT | CONTROL_RIGHT))
        return;

    decide = getOn(decideButtons);
    if (decide == 0)
        cancel = getOn(CONTROL_CANCEL);
    else
        cancel = 0;
    if (decide != 0)
        lastKey = CONTROL_DECIDE;
    else if (cancel != 0)
        lastKey = CONTROL_CANCEL;
    else
        lastKey = 0;

    l1 = getRepeat(MENU_BUTTON_L1, REPEAT_DELAY, REPEAT_INTERVAL);
    if (l1 == 0)
        r1 = getRepeat(MENU_BUTTON_R1, REPEAT_DELAY, REPEAT_INTERVAL);
    else
        r1 = 0;

    if (getNow(MENU_BUTTON_L1 | MENU_BUTTON_R1))
        return;
    l2 = getRepeat(MENU_BUTTON_L2, REPEAT_DELAY, REPEAT_INTERVAL);
    if (l2 == 0)
        r2 = getRepeat(MENU_BUTTON_R2, REPEAT_DELAY, REPEAT_INTERVAL);
    else
        r2 = 0;
    if (getNow(MENU_BUTTON_L2 | MENU_BUTTON_R2))
        return;
}

// FUNC 0x001BAF18 gt2k_menu::MenuControl::emulatePressure
/* Per frame: Control's emulatePressure, the raw pad buttons onto the button
 * word (its low byte kept), the right stick through the curve g_003416F8
 * (-1 .. 1, 128 = 0), then updateKeys(). */
void MenuControl::emulatePressure()
{
    Control::emulatePressure();

    u32 buttons = getButtons() & 0xFF;
    u32 raw = curButtons.raw[0];
    if (raw & PADRAW_TRIANGLE) buttons |= MENU_BUTTON_TRIANGLE;
    if (raw & PADRAW_CIRCLE)   buttons |= MENU_BUTTON_CIRCLE;
    if (raw & PADRAW_CROSS)    buttons |= MENU_BUTTON_CROSS;
    if (raw & PADRAW_SQUARE)   buttons |= MENU_BUTTON_SQUARE;
    if (raw & PADRAW_L1)       buttons |= MENU_BUTTON_L1;
    if (raw & PADRAW_L2)       buttons |= MENU_BUTTON_L2;
    if (raw & PADRAW_L3)       buttons |= MENU_BUTTON_L3;
    if (raw & PADRAW_R1)       buttons |= MENU_BUTTON_R1;
    if (raw & PADRAW_R2)       buttons |= MENU_BUTTON_R2;
    if (raw & PADRAW_R3)       buttons |= MENU_BUTTON_R3;
    curButtons.buttons = buttons;

    /* the low byte of the channel (lbu); the entry's address first */
    const u8 *curve = &g_003416F8[(u8)channels[CHAN_RSTICK_Y]];
    analogRVertical = (f32)((int)*curve - 0x80) * (1.0f / 128.0f);
    updateKeys();
}

// FUNC 0x001BB018 gt2k_menu::MenuControl::getPrev
int MenuControl::getPrev(int horizontal) const
{
    if (horizontal == 0)
        return up;
    return left;
}

// FUNC 0x001BB030 gt2k_menu::MenuControl::getNext
int MenuControl::getNext(int horizontal) const
{
    if (horizontal == 0)
        return down;
    return right;
}

// FUNC 0x001BB048 gt2k_menu::MenuControl::anyKey
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB0A0 gt2k_menu::MenuControl::getDelayedRight(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB0A8 gt2k_menu::MenuControl::getDelayedLeft(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB0B0 gt2k_menu::MenuControl::getDelayedDown(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB0B8 gt2k_menu::MenuControl::getDelayedUp(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB0C0 gt2k_menu::MenuControl::getDelayedDownRight(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB0C8 gt2k_menu::MenuControl::getDelayedDownLeft(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB0D0 gt2k_menu::MenuControl::getDelayedUpRight(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB0D8 gt2k_menu::MenuControl::getDelayedUpLeft(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB0E0 gt2k_menu::MenuControl::setDecideButtons(unsigned int)
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB0E8 gt2k_menu::MenuControl::getLastKey(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB0F0 gt2k_menu::MenuControl::getRightPress(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB0F8 gt2k_menu::MenuControl::getLeftPress(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB100 gt2k_menu::MenuControl::getDownPress(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB108 gt2k_menu::MenuControl::getUpPress(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB110 gt2k_menu::MenuControl::getCancel(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB118 gt2k_menu::MenuControl::getDecide(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB120 gt2k_menu::MenuControl::getR2(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB128 gt2k_menu::MenuControl::getL2(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB130 gt2k_menu::MenuControl::getR1(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB138 gt2k_menu::MenuControl::getL1(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB140 gt2k_menu::MenuControl::getRight(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB148 gt2k_menu::MenuControl::getLeft(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB150 gt2k_menu::MenuControl::getDown(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB158 gt2k_menu::MenuControl::getUp(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB160 gt2k_menu::MenuControl::getAnalogRVertical(void) const
/* inline in game/menu/framework/T214_menu_control.h; egcs emits this copy */

// FUNC 0x001BB1B8 gt2k_menu::MenuControl::~MenuControl
/* implicit (game/menu/framework/T214_menu_control.h): egcs emits this copy (vtable slot 0),
 * only Control::~Control (0x0022DB48) with the caller's flags, no vptr
 * store */
/* implicit: ~Control (base destructor) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (base Control). */
// FUNC 0x001BB168 __tfQ29gt2k_menu11MenuControl
extern "C" void *__tfQ29gt2k_menu11MenuControl(void)
{
    if (__tiQ29gt2k_menu11MenuControl[0] == 0) {
        __tf7Control();
        __rtti_si(__tiQ29gt2k_menu11MenuControl, "Q29gt2k_menu11MenuControl",
                  (const type_info *)__ti7Control);
    }
    return __tiQ29gt2k_menu11MenuControl;
}
#endif
