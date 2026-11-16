/* T247 - gt2k_menu::ControlBase.  0x001CB9E8..0x001CBE30.
 *
 * The base of every menu controller: two MenuControls attached to the pad
 * manager's slots 20 / 21, an idle-frame counter with an optional timeout
 * (waitFrame, once per frame), the screen saver after 108000 idle frames
 * (30 minutes) and a debug pause on the second pad.
 *
 * Data (retail .rodata 0x00336F48..0x00336F80): ControlBase's vtable and type
 * name only (MenuControl, Control, RenderBase: key functions elsewhere). */
#include "game/menu/framework/T247_control_base.h"
#include "game/menu/framework/T246_render_base.h"          /* RenderBase (pause, setScreenSaver) */
#include "game/menu/framework/T248_menu_signal.h"          /* menu_signal_clear / menu_signal_nop */
#include "game/menu/framework/T214_menu_control.h"         /* gt2k_menu::MenuControl, MENU_BUTTON_* */
#include "engine/input/T474_control.h"              /* Control, CONTROL_LEFT */
#include "engine/input/T475_pad_manager.h"          /* PadManager g_002EA7F8 */
#include "engine/pgl/G001_flare.h"                /* gfx_vsync */
#include "game/menu/framework/render_base_internal.h"

namespace gt2k_menu {

// FUNC 0x001CB9E8 gt2k_menu::ControlBase::ControlBase
ControlBase::ControlBase()
{
    /* implicit: MenuControl (member constructors), ~Control (exception cleanup) */
    /* egcs stores the vptr before constructing the members: control1 and
     * control2 by MenuControl() (the landing pad runs Control::~Control) */
    unk268      = 0;
    frameCount  = 0;
    timedOut    = 0;
    timeout     = 0;
    unk278      = 0;
    unk27C      = 0;
    unk280      = 0;
    pageManager = NULL;
}

// FUNC 0x001CBA70 gt2k_menu::ControlBase::~ControlBase
ControlBase::~ControlBase()
{
    /* The vptr, then control2 and control1: their implicit MenuControl
     * destructors are expanded to Control::~Control (flags 2); then
     * operator delete with flags & 1. */
    /* implicit: ~Control (member destructors), operator delete (deleting destructor) */
}

/* Attaches the two pads to the pad manager and clears the menu signal. */
// FUNC 0x001CBAD8 gt2k_menu::ControlBase::attach
void ControlBase::attach()
{
    g_002EA7F8.attach(&control1, PAD_SLOT_MENU1);
    g_002EA7F8.attach(&control2, PAD_SLOT_MENU2);
    menu_signal_clear();
}

// FUNC 0x001CBB30 gt2k_menu::ControlBase::detach
void ControlBase::detach()
{
    menu_signal_nop();
    g_002EA7F8.detach(&control2);
    g_002EA7F8.detach(&control1);
}

/* Debug pause: with R2, L2 and left held on the second pad and triangle
 * newly pressed, pauses the render object's tree and waits (vsync by vsync)
 * for the same combination again.  (Control's inline getNow / getOn: each
 * button is a separate virtual call of getButtons / getButtonsOn.)  The
 * wait is a while (true) loop with a break: egcs leaves its test at the top
 * as retail (while (!combo) and for (;;) rotate it to the bottom); loop.c
 * hoists &control2 and 0x20000 into $s0 / $s2. */
// FUNC 0x001CBB80 gt2k_menu::ControlBase::debugPause
void ControlBase::debugPause(RenderBase *render)
{
    if (control2.getNow(MENU_BUTTON_R2) && control2.getNow(MENU_BUTTON_L2) &&
        control2.getNow(CONTROL_LEFT) && control2.getOn(MENU_BUTTON_TRIANGLE)) {
        render->pause = 1;
        gfx_vsync(1);
        while (true) {
            if (control2.getNow(MENU_BUTTON_R2) && control2.getNow(MENU_BUTTON_L2) &&
                control2.getNow(CONTROL_LEFT) && control2.getOn(MENU_BUTTON_TRIANGLE))
                break;
            gfx_vsync(1);
        }
        gfx_vsync(1);
        render->pause = 0;
    }
}

/* RenderBase::setScreenSaver (inline): on after 108000 idle frames (its
 * flag: the count is not 0), else off with the flag set. */
// FUNC 0x001CBCF8 gt2k_menu::ControlBase::updateScreenSaver
void ControlBase::updateScreenSaver(RenderBase *render)
{
    if (frameCount > SCREEN_SAVER_FRAMES)
        render->setScreenSaver(true, frameCount);
    else
        render->setScreenSaver(false, 1);
}

/* Slot 1 (index.txt: gt2k_menu::ControlBase::virtual_1).  Any held button
 * of the first pad restarts the idle count; waits one vsync, counts the
 * frame and flags the timeout. */
// FUNC 0x001CBD30 gt2k_menu::ControlBase::waitFrame
void ControlBase::waitFrame()
{
    Control *pad = &control1;

    if (pad->getButtons() != 0)
        frameCount = 0;
    gfx_vsync(1);
    frameCount = frameCount + 1;
    if (timeout > 0 && timeout < frameCount)
        timedOut = 1;
}

// FUNC 0x001CBD98 gt2k_menu::ControlBase::getPageManager
/* inline in game/menu/framework/T247_control_base.h; egcs emits this copy */

// FUNC 0x001CBDA0 gt2k_menu::ControlBase::F_001CBDA0
/* inline in game/menu/framework/T247_control_base.h; egcs emits this copy */

// FUNC 0x001CBDA8 gt2k_menu::ControlBase::countFrame
/* inline in game/menu/framework/T247_control_base.h; egcs emits this copy */

// FUNC 0x001CBDB8 gt2k_menu::ControlBase::getFrameCount
/* inline in game/menu/framework/T247_control_base.h; egcs emits this copy */

// FUNC 0x001CBDC0 gt2k_menu::ControlBase::resetFrameCount
/* inline in game/menu/framework/T247_control_base.h; egcs emits this copy */

// FUNC 0x001CBDD0 gt2k_menu::ControlBase::setTimeout
/* inline in game/menu/framework/T247_control_base.h; egcs emits this copy */

// FUNC 0x001CBDE0 gt2k_menu::ControlBase::getControl2
/* inline in game/menu/framework/T247_control_base.h; egcs emits this copy */

// FUNC 0x001CBDE8 gt2k_menu::ControlBase::getControl1
/* inline in game/menu/framework/T247_control_base.h; egcs emits this copy */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__user_type_info, no bases). */
// FUNC 0x001CBDF0 __tfQ29gt2k_menu11ControlBase
extern "C" void *__tfQ29gt2k_menu11ControlBase(void)
{
    if (__tiQ29gt2k_menu11ControlBase[0] == 0)
        __rtti_user(__tiQ29gt2k_menu11ControlBase, "Q29gt2k_menu11ControlBase");
    return __tiQ29gt2k_menu11ControlBase;
}
#endif
