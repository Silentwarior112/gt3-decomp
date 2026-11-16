/* T474 - Control: the input state of one player.  0x0022DA98..0x0022E470.
 *
 * Every frame the PadManager (T475) calls update() on each attached Control:
 * latch() keeps the previous frame's buttons, the port translates its
 * device into the Control (type, status, GT3 buttons, raw device buttons,
 * 32 analog channels), and emulatePressure() fakes the pressure channels of
 * pads that have none.  The small accessors after getRepeat are the
 * out-of-line copies of Control's inline functions; Lupo 2 has them as
 * linkonce symbols (0x002D2760..0x002D29F8, in the reverse order). */
#include "engine/input/control_internal.h"
#include "engine/input/T474_control.h"
#include "game/enhanced.h"                  /* ENH_INPUT_WHEEL_MENU_STEER */

/* The pedal map flag (isPedalMapEnable / setPedalMapEnable), T474's only
 * .sdata item; initially 1. */
int g_00352CE0 = 1;                         /* 0x00352CE0 */

// FUNC 0x0022DA98 Control::Control
Control::Control()
{
    linked = 0;
    prev   = NULL;
    next   = NULL;
    port   = NULL;
    status = PORT_NONE;
    type   = 0;
    curButtons.mask    = 0;
    curButtons.buttons = 0;
    for (int i = 1; i >= 0; i--)
        curButtons.raw[i] = 0;
    prevButtons = curButtons;               /* the inline latch, without the mask reset */
    repeating   = 0;
    repeatCount = 0;
    for (int ch = CHAN_COUNT - 1; ch >= 0; ch--)
        channels[ch] = 0;
}

/* Index: Control::~Control.  Detaches itself from the PadManager if it is
 * still in its list. */
// FUNC 0x0022DB48 Control::~Control
Control::~Control()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
    /* (egcs: the vptr is stored first; operator delete(this) follows when
     * flags & 1) */
    if (linked != 0)
        detach();
}

/* (? name) The state of a Control with nothing behind it: status 1, no
 * type, no buttons, all channels 0 (the masks and the previous frame stay).
 * PadManager::init applies it to the spare Control g_002EB170. */
// FUNC 0x0022DBA8 Control::clear
void Control::clear()
{
    int i;                                  /* one counter for both loops (u105): its
                                               register then holds status's 1 */
    type   = 0;
    status = PORT_CONNECTED;
    curButtons.buttons = 0;
    for (i = 1; i >= 0; i--)
        curButtons.raw[i] = 0;
    for (i = CHAN_COUNT - 1; i >= 0; i--)
        channels[i] = 0;
}

/* (? name) Copies what a port translates: type, status, the raw buttons,
 * the GT3 buttons and the channels (not the masks nor the previous frame).
 * Caller: SteeringPSPort::translate.  The raw buttons as an exact form,
 * not the original: `int n = 2; while (n--) *d++ = *s++;' (setChannels'
 * loop) is 97.0%, the counter's `li 1' then comes after the source
 * pointer's set-up; `n = 1' and do / while (n--) copy the same two words
 * and put it between the pointers, as retail. */
// FUNC 0x0022DC10 Control::assign
void Control::assign(const Control &src)
{
    status = src.status;
    type   = src.type;
    {
        u32 *d = curButtons.raw;
        int n = 1;
        const u32 *s = src.curButtons.raw;
        do
            *d++ = *s++;
        while (n--);
    }
    curButtons.buttons = src.curButtons.buttons;
    setChannels(src);
}

// FUNC 0x0022DC98 Control::attach
void Control::attach(int slot)
{
    g_002EA7F8.attach(this, slot);
}

// FUNC 0x0022DCC0 Control::detach
void Control::detach()
{
    g_002EA7F8.detach(this);
}

/* Index: Control::virtual_9.  A digital pad and a DualShock report no
 * pressures: their 12 pressure channels become PRESSURE_DOWN (0x80) while
 * the button is down, else 0 (the square's is its bit value 0x80 itself).
 * Left before right, R2 last: the order retail's schedule shows. */
// FUNC 0x0022DCE8 Control::emulatePressure
void Control::emulatePressure()
{
    int psType = type & PS_PAD_TYPE_MASK;
    if (psType == PS_PAD_DIGITAL || psType == PS_PAD_DUALSHOCK) {
        u32 raw = curButtons.raw[0];
        channels[CHAN_PRESS_LEFT]     = (raw & SCE_PADLleft)  ? PRESSURE_DOWN : 0;
        channels[CHAN_PRESS_RIGHT]    = (raw & SCE_PADLright) ? PRESSURE_DOWN : 0;
        channels[CHAN_PRESS_UP]       = (raw & SCE_PADLup)    ? PRESSURE_DOWN : 0;
        channels[CHAN_PRESS_DOWN]     = (raw & SCE_PADLdown)  ? PRESSURE_DOWN : 0;
        channels[CHAN_PRESS_TRIANGLE] = (raw & SCE_PADRup)    ? PRESSURE_DOWN : 0;
        channels[CHAN_PRESS_CIRCLE]   = (raw & SCE_PADRright) ? PRESSURE_DOWN : 0;
        channels[CHAN_PRESS_CROSS]    = (raw & SCE_PADRdown)  ? PRESSURE_DOWN : 0;
        channels[CHAN_PRESS_SQUARE]   = raw & SCE_PADRleft;              /* 0x80 or 0 */
        channels[CHAN_PRESS_L1]       = (raw & SCE_PADL1)     ? PRESSURE_DOWN : 0;
        channels[CHAN_PRESS_R1]       = (raw & SCE_PADR1)     ? PRESSURE_DOWN : 0;
        channels[CHAN_PRESS_L2]       = (raw & SCE_PADL2)     ? PRESSURE_DOWN : 0;
        channels[CHAN_PRESS_R2]       = (raw & SCE_PADR2)     ? PRESSURE_DOWN : 0;
    }
}

/* Index: Control::virtual_8.  The previous frame = this one; every button
 * counts again (invalidButtons clears the mask for one frame). */
// FUNC 0x0022DE10 Control::latch
void Control::latch()
{
    prevButtons     = curButtons;
    curButtons.mask = 0xFFFFFFFF;
}

/* Pushes itself at the head of the list (after a latch). */
// FUNC 0x0022DE40 Control::link
void Control::link(Control **head)
{
    latch();
    prev = NULL;
    next = *head;
    if (*head != NULL)
        (*head)->prev = this;
    *head  = this;
    linked = 1;
}

// FUNC 0x0022DEA8 Control::unlink
void Control::unlink(Control **head)
{
    if (next != NULL)
        next->prev = prev;
    if (prev != NULL)
        prev->next = next;
    else
        *head = next;
    linked = 0;
}

/* Index: Control::virtual_1.  Called by PadManager::update every frame. */
// FUNC 0x0022DED8 Control::update
void Control::update()
{
    latch();
    if (port != NULL)
        port->translate(*this);
    emulatePressure();
}

/* Index: Control::virtual_2.  Called by PadManager::updateLink (i.LINK). */
// FUNC 0x0022DF40 Control::updateSync
void Control::updateSync()
{
    latch();
    if (port != NULL)
        port->translateSync(*this);
    emulatePressure();
}

/* Index: Control::virtual_4. */
// FUNC 0x0022DFA8 Control::getButtons
u32 Control::getButtons() const
{
    return curButtons.buttons;
}

/* Index: Control::virtual_5.  Buttons down now and up in the previous
 * frame, among the ones that count in both. */
// FUNC 0x0022DFB0 Control::getButtonsOn
u32 Control::getButtonsOn() const
{
    u32 on = curButtons.buttons & prevButtons.mask;     /* the halves in their own variables, */
    u32 off = ~prevButtons.buttons & curButtons.mask;   /* as rawButtonDown (one expression: */
    return on & off;                                    /* fold regroups the operands) */
}

/* Index: Control::virtual_6.  Buttons up now and down in the previous frame. */
// FUNC 0x0022DFD8 Control::getButtonsOff
u32 Control::getButtonsOff() const
{
    u32 off = ~curButtons.buttons & prevButtons.mask;   /* as rawButtonUp */
    u32 on = prevButtons.buttons & curButtons.mask;
    return off & on;
}

/* Index: Control::virtual_7. */
// FUNC 0x0022E000 Control::getAnalog
u32 Control::getAnalog(int ch) const
{
    return channels[ch];
}

/* Index: Control::virtual_3.  No null check of the port. */
// FUNC 0x0022E010 Control::isPortReady
int Control::isPortReady()
{
    return port->isReady();
}

/* (? name) The port's slot 3, no null check (caller: RaceInput::sendVibration). */
// FUNC 0x0022E040 Control::setActuator
void Control::setActuator(int ch, int value)
{
    port->setActuator(ch, value);
}

/* (? name) The port's slot 4, no null check. */
// FUNC 0x0022E070 Control::stopActuators
void Control::stopActuators()
{
    port->stopActuators();
}

/* (? name) Key repeat of the buttons in mask: 1 on the frame they go down,
 * then once more than `first' frames have passed while held, then every
 * time more than `interval' further frames have passed.  The button reads
 * are virtual calls.
 * The enhanced engine (backend.md B3, ENH_INPUT_WHEEL_MENU_STEER; Lupo 2 L2
 * 0x0023D658): the count runs in eighths of a frame, 8 a frame; for left /
 * right a wheel turned beyond its dead zone adds its port's level 1..8
 * instead (slot 6) and skips the first delay: turned a little the cursor
 * moves slowly, turned fully at the pad's speed.  A pad counts as before.
 * (Lupo 2 does not test the port for NULL; here a missing port counts 8.) */
// FUNC 0x0022E0A0 Control::getRepeat
int Control::getRepeat(u32 mask, int first, int interval)
{
    if ((getButtons() & mask) == 0)
        return 0;
    if (getButtonsOn() & mask) {
        repeating   = 0;
        repeatCount = 0;
        return 1;
    }
#if ENH_INPUT_WHEEL_MENU_STEER
    {
        int level = 0;

        if ((mask & (CONTROL_LEFT | CONTROL_RIGHT)) && port != NULL)
            level = port->getSteerLevel();
        if (level != 0) {
            repeating = 1;
            repeatCount += level;
        } else {
            repeatCount += 8;
        }
    }
    int held = repeatCount >> 3;
#else
    int held = ++repeatCount;
#endif
    if (repeating == 0) {
        if (first < held) {
            repeatCount = 0;
            repeating   = 1;
            return 1;
        }
        return 0;
    }
    if (interval < held) {
        repeatCount = 0;
        return 1;
    }
    return 0;
}

/* The accessors 0x0022E178..0x0022E420 are inline in engine/input/T474_control.h
 * (none has a caller; the ports and assign() expand them).  egcs emits these
 * copies with the vtable, in the reverse order of their declarations (the
 * header declares them from isRepeating up to getOff).  Lupo 2 has them as
 * linkonce symbols (0x002D2760..0x002D29F8, in the reverse order). */

// FUNC 0x0022E178 Control::getOff
/* inline (u061 matching): (getButtonsOff() & mask) != 0 */

// FUNC 0x0022E1B8 Control::getOn
/* inline (ReplayOption::update, T196, expands it): (getButtonsOn() & mask)
 * != 0 */

// FUNC 0x0022E1F8 Control::getNow
/* inline: (getButtons() & mask) != 0 */

/* index.txt: GTSOUNDINSTRUMENT::GetAddr(void) (lupo2m, a match of the code
 * only); the Lupo 2 order makes it isPedalMapEnable. */
// FUNC 0x0022E238 Control::isPedalMapEnable
/* inline: return g_00352CE0 */

/* A member that sets the global flag (this is not used). */
// FUNC 0x0022E240 Control::setPedalMapEnable
/* inline: g_00352CE0 = on */

// FUNC 0x0022E248 Control::invalidButtons
/* inline: curButtons.mask = 0 */

// FUNC 0x0022E250 Control::rawAnalogValue
/* inline: channels[ch] */

// FUNC 0x0022E260 Control::rawPrevButtons
/* inline: prevButtons.raw[i] */

// FUNC 0x0022E270 Control::rawButtonUp
/* inline (u047 matching): (~cur.raw[i] & prev.mask) & (prev.raw[i] &
 * cur.mask), the halves in their own variables (one expression: 81.2%) */

// FUNC 0x0022E2A0 Control::rawButtonDown
/* inline (RaceSolitaire::virtual_28, T139, expands it): (cur.raw[i] &
 * prev.mask) & (~prev.raw[i] & cur.mask), the halves in their own variables
 * (one expression: 72.9%) */

// FUNC 0x0022E2D0 Control::rawButtons
/* inline: curButtons.raw[i] */

/* lbu of the low byte, shifted: bits 5-7. */
// FUNC 0x0022E2E0 Control::rawUSBType
/* inline: (type >> 5) & 7 */

// FUNC 0x0022E2F0 Control::rawPSType
/* inline: type & 0x1F */

/* index.txt: _UnitArenaBase::getNumFree(void) const (lupo2m, code match). */
// FUNC 0x0022E300 Control::rawType
/* inline: type */

/* index.txt: GTSOUNDINSTRUMENT::getSpuAddr(void) (lupo2m, code match). */
// FUNC 0x0022E308 Control::rawStatus
/* inline: status */

// FUNC 0x0022E310 Control::setChannels
/* inline: the 32 channels, `while (n--) *d++ = *s++;' (the destination
 * pointer first) */

// FUNC 0x0022E348 Control::setChannel
/* inline: channels[ch] = value */

// FUNC 0x0022E358 Control::setButtons
/* inline: curButtons.buttons = buttons */

// FUNC 0x0022E360 Control::setRawButtons
/* inline: curButtons.raw[i] = buttons */

/* Replaces the whole word (the PS type bits are cleared). */
// FUNC 0x0022E370 Control::setUSBType
/* inline: type = (t & 7) << 5 */

/* Replaces the whole word (the USB type bits are cleared). */
// FUNC 0x0022E380 Control::setPSType
/* inline: type = t & 0x1F */

// FUNC 0x0022E390 Control::setType
/* inline: type = t */

/* index.txt: GTSOUNDINSTRUMENT::setSpuAddr(unsigned int) (lupo2m, code match). */
// FUNC 0x0022E398 Control::setStatus
/* inline: status = s */

// FUNC 0x0022E3A0 Control::portTranslateSync
/* inline: if (port != NULL) port->translateSync(*this) */

// FUNC 0x0022E3D8 Control::portTranslate
/* inline: if (port != NULL) port->translate(*this) */

// FUNC 0x0022E410 Control::setPort
/* inline: port = &p */

/* index.txt: GTSOUNDINSTRUMENT::GetHeaderMax(void) const (lupo2m, code match). */
// FUNC 0x0022E418 Control::getNext
/* inline: next */

// FUNC 0x0022E420 Control::isRepeating
/* inline: repeating != 0 */

#ifndef BOOT_OLD_GCC
/* __user_type_info __ti7Control (0x00353478, gp-0x4578), name "7Control"
 * (0x00341960).  Callers: __tf9RaceInput, __tfQ29gt2k_menu11MenuControl. */
// FUNC 0x0022E430 __tf7Control
extern "C" void *__tf7Control(void)
{
    if (__ti7Control[0] == 0)
        __rtti_user(__ti7Control, "7Control");
    return __ti7Control;
}
#endif
