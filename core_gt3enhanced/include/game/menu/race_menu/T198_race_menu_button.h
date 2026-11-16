/* T198_race_menu_button.h - the race menu buttons (translation unit T198,
 * unit u062), 0x001B02F8..0x001B0678.
 *
 * A button is two Items of a layer (T213): `name' and `name'_sel.  refresh()
 * (slot 1) shows them by selected / visible and dims the normal image to
 * alpha 0.5 when the button is disabled.  RaceMenuImageChangeButton swaps
 * the two images; RaceMenuImageBlinkButton keeps the normal one and blinks
 * the "_sel" image over it while selected (the pause windows, T169..T171).
 * RaceMenuSelectWindow (T199) moves the cursor over RaceMenuButton *s.
 *
 * The layout is egcs's: RaceMenuButton's own data are the four words, its
 * vptr +0x10 (the derived classes' code loads it from there), and the
 * images are the derived classes' members from +0x14 (egcs puts the vptr
 * after every data member of the class that introduces it, so they cannot
 * be RaceMenuButton's: u057 / u058 / u062 declared them there).
 *
 * RaceMenuButton::update (0x001B0630) and __tf14RaceMenuButton are emitted
 * at the end of T198 with the typeinfo functions: the update is inline. */
#ifndef GAME_T198_RACE_MENU_BUTTON_H
#define GAME_T198_RACE_MENU_BUTTON_H

#include "game/game.h"
/* Seen by every original that sees RaceMenuButton, defined before it
 * (retail's type names: RaceMenuButton, then OperatorList, List / Cell
 * <OperatorHolder>, OperatorHolder; Operator last), u042 data pass. */
#include "game/menu/framework/T216_menu_transitive.h"     /* (seen: the operator list classes) */

namespace gt2k_menu {
class Item;                                 /* T213_menu_item.h (pointers only) */
class Transitive;                           /* T216_menu_transitive.h */
class PageLayout;                           /* T229_page_layout.h */
}
class type_info;                            /* T548 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
/* The __user_type_info at 0x003532F0 (gp-0x4700), no base. */
const type_info &__tf14RaceMenuButton(void);            /* 0x001B0638 */
/* The __si_type_info at 0x004B23F0, base __ti14RaceMenuButton. */
const type_info &__tf24RaceMenuImageBlinkButton(void);  /* 0x001B0590 */
/* The __si_type_info at 0x004B23E0, base __ti14RaceMenuButton. */
const type_info &__tf25RaceMenuImageChangeButton(void); /* 0x001B05E0 */
}
#endif

/* typeinfo __tf14RaceMenuButton 0x001B0638 (user); no vtable of its own
 * survives (abstract, its constructor always inlined).  0x10 bytes of data,
 * egcs vptr +0x10: size 0x14. */
class RaceMenuButton {
public:
    RaceMenuButton() : selected(0), id(0), visible(1), enabled(1) {}
    /* The per-frame update; nothing to animate (RaceMenuImageChangeButton
     * keeps it).  Out-of-line copy 0x001B0630. */
    virtual void update() {}                /* [0] +0x0C 0x001B0630 */
    /* Shows the images by selected / visible / enabled. */
    virtual void refresh() = 0;             /* [1] +0x14 */
    /* visible = v, then refresh() (virtual).  PauseMenu::showMain /
     * showConfirm (T169): retail stores through each button's address and
     * calls refresh() through its vtable. */
    void setVisible(int v) { visible = v; refresh(); }
    /* (? name) selected = s, then refresh() (virtual): RaceMenuSelectWindow::
     * input (T199) moves the selection with it (u062 matching: the old and
     * new button each in a fresh pseudo, the vptr load after the store). */
    void setSelected(int s) { selected = s; refresh(); }

    int selected;                           /* 0x00 */
    int id;                                 /* 0x04 the result of the button */
    int visible;                            /* 0x08 */
    int enabled;                            /* 0x0C (u057 f0C) 0: the normal image at alpha 0.5 */
    /*  vptr                                   0x10 */
};
EGCS_SIZE(RaceMenuButton, 0x14);

/* vtable 0x003318C0 (2 slots: [0] RaceMenuButton::update), typeinfo
 * __tf25RaceMenuImageChangeButton 0x001B05E0 (si).  Nothing constructs it
 * (init has no callers); its size is not known (blink, +0x1C in
 * RaceMenuImageBlinkButton, is never used here). */
class RaceMenuImageChangeButton : public RaceMenuButton {
public:
    /* The normal image while not selected, the "_sel" image while
     * selected (both only when visible). */
    virtual void refresh();                 /* [1] +0x14 0x001B03B0 */
    /* The images `name' and `name'_sel under root, the id, then refresh(). */
    void init(gt2k_menu::PageLayout *layout, gt2k_menu::Transitive *root, const char *name,
              int id);                      /* 0x001B02F8 */

    gt2k_menu::Item *image;                 /* 0x14 */
    gt2k_menu::Item *imageSel;              /* 0x18 the "_sel" image */
};

/* vtable 0x003318A8 (2 slots), typeinfo __tf24RaceMenuImageBlinkButton
 * 0x001B0590 (si).  Size 0x20 (the pause windows' arrays). */
class RaceMenuImageBlinkButton : public RaceMenuButton {
public:
    /* While selected the "_sel" image fades from 1 to 0 in 30 frames, then
     * again. */
    virtual void update();                  /* [0] +0x0C 0x001B0540 */
    /* The normal image whenever visible, the "_sel" image over it while
     * selected; the blink restarts at 1. */
    virtual void refresh();                 /* [1] +0x14 0x001B04E8 */
    /* As RaceMenuImageChangeButton::init, the "_sel" image one priority
     * above the normal one (u057 / u058: RaceMenuButton::F_001B0418). */
    void init(gt2k_menu::PageLayout *layout, gt2k_menu::Transitive *root, const char *name,
              int id);                      /* 0x001B0418 */

    gt2k_menu::Item *image;                 /* 0x14 */
    gt2k_menu::Item *imageSel;              /* 0x18 the "_sel" image */
    f32              blink;                 /* 0x1C its alpha while selected */
};
EGCS_SIZE(RaceMenuImageBlinkButton, 0x20);

#endif /* GAME_T198_RACE_MENU_BUTTON_H */
