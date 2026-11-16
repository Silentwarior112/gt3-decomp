/* T198 - the race menu buttons.  0x001B02F8..0x001B0678.
 *
 * A button is two Items of a layer: `name' and `name'_sel.  refresh()
 * (vtable slot 1) shows them by selected / visible and dims the normal
 * image to alpha 0.5 when the button is disabled. */
/* The .rodata opens with the two buttons' vtables (0x003318A8), which only
 * other files' code (the inline constructors' expansions) references. */
// DATA_START 0x003318A8
#include "game/menu/race_menu/race_menu_internal.h"
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item */
#include "game/menu/race_menu/T197_race_menu_page.h"       /* race_menu_new_item */
#include "game/menu/race_menu/T198_race_menu_button.h"

using namespace gt2k_menu;

/* The images `name' and `name'_sel under root (both sorted by z), the id,
 * then refresh() (virtual).  No callers. */
// FUNC 0x001B02F8 RaceMenuImageChangeButton::init
void RaceMenuImageChangeButton::init(PageLayout *layout, Transitive *root, const char *name,
                                     int id)
{
    char selName[0x80];

    strcpy(selName, name);
    strcat(selName, "_sel");
    image = race_menu_new_item(layout, root, name, 1);
    imageSel = race_menu_new_item(layout, root, selName, 1);
    this->id = id;
    refresh();
}

/* The normal image while not selected, the "_sel" image while selected
 * (both only when visible); the normal image at alpha 0.5 when disabled. */
// FUNC 0x001B03B0 RaceMenuImageChangeButton::refresh
void RaceMenuImageChangeButton::refresh()
{
    Item *normal;
    int shown;
    int shownSel;

    shown = 0;
    if (selected == 0)
        shown = visible != 0;
    normal = image;
    normal->visible = shown;
    shownSel = 0;
    if (selected != 0)
        shownSel = visible != 0;
    imageSel->visible = shownSel;
    if (enabled != 0)
        normal->alphaRatio = 1.0f;
    else
        normal->alphaRatio = 0.5f;
}

/* As RaceMenuImageChangeButton::init, but the "_sel" image is put one z
 * above the normal one.  Callers: PauseMenu, ReplayPauseMenu,
 * PauseMenuWithRetry constructors. */
// FUNC 0x001B0418 RaceMenuImageBlinkButton::init
void RaceMenuImageBlinkButton::init(PageLayout *layout, Transitive *root, const char *name,
                                    int id)
{
    char selName[0x80];

    strcpy(selName, name);
    strcat(selName, "_sel");
    image = race_menu_new_item(layout, root, name, 1);
    Item *sel = race_menu_new_item(layout, root, selName, 1);
    this->id = id;
    imageSel = sel;
    sel->z = image->z + 1.0f;
    refresh();
}

/* The normal image whenever visible (the raw value is stored), the "_sel"
 * image over it while selected; the blink restarts at 1. */
// FUNC 0x001B04E8 RaceMenuImageBlinkButton::refresh
void RaceMenuImageBlinkButton::refresh()
{
    Item *normal = image;
    int shown;

    normal->visible = visible;
    blink = 1.0f;
    shown = 0;
    if (visible != 0)
        shown = selected != 0;
    imageSel->visible = shown;
    if (enabled != 0)
        normal->alphaRatio = 1.0f;
    else
        normal->alphaRatio = 0.5f;
}

/* While selected the "_sel" image fades from 1 to 0 in 30 frames (1/30 a
 * frame), then starts again. */
// FUNC 0x001B0540 RaceMenuImageBlinkButton::update
void RaceMenuImageBlinkButton::update()
{
    if (selected != 0) {
        blink = blink - 0.033333335f;
        if (blink < 0.0f)
            blink = 1.0f;
        imageSel->alphaRatio = blink;
    }
}

#ifndef BOOT_OLD_GCC
/* Compiler-generated: the __si_type_info __ti24RaceMenuImageBlinkButton,
 * base __ti14RaceMenuButton (gp-0x4700). */
// FUNC 0x001B0590 __tf24RaceMenuImageBlinkButton
extern "C" const type_info &__tf24RaceMenuImageBlinkButton(void)
{
    if (__ti24RaceMenuImageBlinkButton[0] == 0) {
        __tf14RaceMenuButton();
        __rtti_si(__ti24RaceMenuImageBlinkButton, "24RaceMenuImageBlinkButton",
                  (const type_info *)__ti14RaceMenuButton);
    }
    return *(const type_info *)__ti24RaceMenuImageBlinkButton;
}

/* The __si_type_info __ti25RaceMenuImageChangeButton, base
 * __ti14RaceMenuButton. */
// FUNC 0x001B05E0 __tf25RaceMenuImageChangeButton
extern "C" const type_info &__tf25RaceMenuImageChangeButton(void)
{
    if (__ti25RaceMenuImageChangeButton[0] == 0) {
        __tf14RaceMenuButton();
        __rtti_si(__ti25RaceMenuImageChangeButton, "25RaceMenuImageChangeButton",
                  (const type_info *)__ti14RaceMenuButton);
    }
    return *(const type_info *)__ti25RaceMenuImageChangeButton;
}
#endif

// FUNC 0x001B0630 RaceMenuButton::update
/* inline in game/menu/race_menu/T198_race_menu_button.h; egcs emits this copy (empty: slot
 * 0 of RaceMenuImageChangeButton) */

#ifndef BOOT_OLD_GCC
/* The __user_type_info __ti14RaceMenuButton (gp-0x4700): no base. */
// FUNC 0x001B0638 __tf14RaceMenuButton
extern "C" const type_info &__tf14RaceMenuButton(void)
{
    if (__ti14RaceMenuButton[0] == 0)
        __rtti_user(__ti14RaceMenuButton, "14RaceMenuButton");
    return *(const type_info *)__ti14RaceMenuButton;
}
#endif
