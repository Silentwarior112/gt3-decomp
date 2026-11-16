#if defined(__GNUC__) && __GNUC__ < 3   /* egcs only: see "#pragma implementation" below */
#line 3 "src/game/menu/framework/T275_raw_check_box.cpp"
#pragma implementation "src/game/menu/framework/T275_raw_check_box.cpp"
#pragma implementation "T275_raw_check_box.h"
#endif
/* T275 - gt2k_menu::RawCheckBox.  0x001D2C88..0x001D2FD0.
 * A list item with four pictures, images[checked][boxFocus].  Decide (while
 * the box and its parents are shown) sends RAW_EVENT_CHECK_TOGGLE; the box's
 * own event toggles it, and the parents see it on its way up (PlayMusicBox,
 * T085, turns its song on / off with it).  Its slot 8 / factory are in
 * T272. */
#include "game/menu/framework/menu_draw_state_internal.h"

namespace gt2k_menu {

/* p0 / p1 / p2 / p3: the pictures on / off / on+focus / off+focus (the
 * layout hook table's words), read in that order; the stores are in the
 * original's order, the two flags last (120 orders scored: only the
 * parameters read p0 / p1 / p2 / p3 or p1 / p0 / p2 / p3 with the flags
 * after the pictures are exact). */
// FUNC 0x001D2C88 gt2k_menu::RawCheckBox::RawCheckBox
RawCheckBox::RawCheckBox(const RawWindowArgs &args)
    : RawListItem(args)
{
    const char *on       = (const char *)args.p0;
    const char *off      = (const char *)args.p1;
    const char *onFocus  = (const char *)args.p2;
    const char *offFocus = (const char *)args.p3;

    images[CHECK_OFF][0] = off != NULL ? off : "check_off";
    images[CHECK_ON][0]  = on != NULL ? on : "check_on";
    images[CHECK_OFF][1] = offFocus != NULL ? offFocus : "check_off_focus";
    images[CHECK_ON][1]  = onFocus != NULL ? onFocus : "check_on_focus";
    boxFocus = 0;
    checked = CHECK_OFF;
    virtual_26();                           /* direct call (in the constructor) */
    /* EH: ~RawListItem() if it throws */
}

// FUNC 0x001D2D68 gt2k_menu::RawCheckBox::~RawCheckBox
RawCheckBox::~RawCheckBox()
{
    /* implicit: only ~RawListItem() (flags 0), then operator delete if flags & 1 */
}

/* Input: decide sends RAW_EVENT_CHECK_TOGGLE (arg 0) and is handled (0);
 * otherwise, or if the box (or a parent) is not shown, 1. */
// FUNC 0x001D2DC0 gt2k_menu::RawCheckBox::virtual_16
int RawCheckBox::virtual_16(MenuControl *ctl)
{
    if (isShown()) {
        if (ctl->decide != 0) {
            virtual_19(RAW_EVENT_CHECK_TOGGLE, 0);
            return 0;
        }
        return 1;
    }
    return 1;
}

/* The box's own toggle event toggles it; every event then goes on to
 * RawWindow::virtual_17 (direct call; its result is returned).  The code
 * makes the virtual call through `sender' (== this there). */
// FUNC 0x001D2E28 gt2k_menu::RawCheckBox::virtual_17
int RawCheckBox::virtual_17(RawWindow *sender, int event, int arg)
{
    if (this == sender && event == RAW_EVENT_CHECK_TOGGLE)
        virtual_25();
    return RawWindow::virtual_17(sender, event, arg);
}

/* Overrides RawWindow::virtual_20 completely (RawWindow's own focus +0x164
 * is not updated, nor the children's). */
// FUNC 0x001D2EA8 gt2k_menu::RawCheckBox::virtual_20
void RawCheckBox::virtual_20(int on)
{
    boxFocus = on;
    virtual_26();
}

// FUNC 0x001D2ED8 gt2k_menu::RawCheckBox::virtual_24
void RawCheckBox::virtual_24(int on)
{
    checked = on;
    virtual_26();
}

// FUNC 0x001D2F08 gt2k_menu::RawCheckBox::virtual_25
void RawCheckBox::virtual_25()
{
    virtual_24(checked ^ 1);
}

/* The picture: the image of the layout node images[checked][boxFocus], at the
 * window's current place and size. */
// FUNC 0x001D2F38 gt2k_menu::RawCheckBox::virtual_26
void RawCheckBox::virtual_26()
{
    changeImage(images[checked][boxFocus], 0, 0);
}

/* No callers: an inline member emitted out of line. */
// FUNC 0x001D2F78 gt2k_menu::RawCheckBox::isChecked
/* inline in game/menu/framework/T275_raw_check_box.h; egcs emits this copy (#pragma implementation) */

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x001D2F80 __tfQ29gt2k_menu11RawCheckBox
extern "C" void *__tfQ29gt2k_menu11RawCheckBox(void)
{
    if (__tiQ29gt2k_menu11RawCheckBox[0] == 0) {
        __tfQ29gt2k_menu11RawListItem();
        __rtti_si(__tiQ29gt2k_menu11RawCheckBox, "Q29gt2k_menu11RawCheckBox",
                  (const type_info *)__tiQ29gt2k_menu11RawListItem);
    }
    return __tiQ29gt2k_menu11RawCheckBox;
}
#endif
