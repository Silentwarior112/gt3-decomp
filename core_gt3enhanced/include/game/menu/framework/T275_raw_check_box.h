/* T275_raw_check_box.h - gt2k_menu::RawCheckBox (translation unit T275, unit
 * u073), 0x001D2C88..0x001D2FD0.
 *
 * A list item showing one of four pictures, images[checked][boxFocus].  Decide
 * (while the box and its parents are active) sends event 7 to itself and up
 * the parents; receiving its own event 7 toggles checked.  Its class name
 * (slot 8), layout factory and registry functions are in T272. */
#ifndef GAME_T275_RAW_CHECK_BOX_H
#define GAME_T275_RAW_CHECK_BOX_H

#include "game/game.h"
#include "game/menu/framework/T263_raw_list.h"             /* base gt2k_menu::RawListItem */

/* The window event (RawWindow::virtual_19 / virtual_17, after T264's 1..6
 * decide, cancel, up, down, left, right) a check box sends when decide
 * toggles it; PlayMusicBox (T085) takes it from its check box. */
enum { RAW_EVENT_CHECK_TOGGLE = 7 };

namespace gt2k_menu {
class RawWindow;                            /* T264 (pointers only) */
struct RawWindowArgs;                       /* T264 (references / pointers only) */
struct MenuLayoutItem;                      /* T229 (pointers only) */
class PageLayout;                           /* T229 (pointers only) */
class MenuControl;                          /* T214 (pointers only) */

#ifdef BOOT_OLD_GCC
/* From here on (egcs applies it to the classes defined after it), with
 * #pragma implementation in T275_raw_check_box.cpp: egcs emits the copies of
 * the inline accessors below and the vtable only there, before __tf. */
#pragma interface
#endif

/* vtable 0x00337C80 (27 slots: [1]..[3] / [9]..[15] / [18] / [19] / [22] /
 * [23] RawWindow's, [21] RawListItem's, [4] Item's, [5] Image's, [6] / [7]
 * Transitive's; [24]..[26] its own).  typeinfo __tfQ29gt2k_menu11RawCheckBox
 * 0x001D2F80 (__si_type_info 0x004B2720, base RawListItem).  Size 0x21C
 * (operator new(0x21C) in create). */
class RawCheckBox : public RawListItem {
public:
    RawCheckBox(const RawWindowArgs &args); /* 0x001D2C88 RawListItem(args); the pictures p0 / p1 / p2 /
                                               p3 = on / off / on+focus / off+focus (NULL: "check_on",
                                               "check_off", "check_on_focus", "check_off_focus") */
    virtual ~RawCheckBox();                 /* [0] +0x0C 0x001D2D68 only ~RawListItem */
    virtual const char *virtual_8();        /* [8] +0x4C 0x001D2FD0 (T272) the class name "RawCheckBox" */
    virtual int  virtual_16(MenuControl *pad); /* [16] +0x8C 0x001D2DC0 input: decide (box and parents
                                               active) sends event 7 and returns 0; else 1 */
    virtual int  virtual_17(RawWindow *sender, int event, int arg); /* [17] +0x94 0x001D2E28 its own
                                               event 7 toggles; then RawWindow's */
    virtual void virtual_20(int focus);     /* [20] +0xAC 0x001D2EA8 boxFocus = f, the picture (RawWindow's
                                               not called: RawWindow::focus is not updated) */
    virtual void virtual_24(int checked);   /* [24] +0xCC 0x001D2ED8 checked = c, the picture */
    virtual void virtual_25();              /* [25] +0xD4 0x001D2F08 toggles checked */
    virtual void virtual_26();              /* [26] +0xDC 0x001D2F38 shows images[checked][boxFocus] */

    int isChecked();                        /* 0x001D2F78 (inline in the original; unreferenced copy) */

    /* The layout factory (no callers): the eight words as a RawWindowArgs, then create. */
    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node, int p0, int p1, int p2, int p3); /* 0x001D2FD8 (T272) */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001D3018 (T272) the registry's */
    static int        canCreate(const RawWindowArgs *args);    /* 0x001D3090 (T272) 1 (args unused) */

    /* boxFocus: `focus' before the sweep, which hid RawWindow::focus (+0x164). */
    int         boxFocus;                   /* 0x204 the picture's focus */
    int         checked;                    /* 0x208 */
    const char *images[2][2];               /* 0x20C [checked][boxFocus]: off, off+focus, on, on+focus */
};

/* The inline accessors 0x001D2F78..0x001D2F78: no callers, the users expand them; defined
 * in reverse address order (egcs emits the #pragma implementation copies
 * in reverse definition order). */
inline int RawCheckBox::isChecked() { return checked; }  /* 0x001D2F78 */
EGCS_SIZE(gt2k_menu::RawCheckBox, 0x21C);

} /* namespace gt2k_menu */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu11RawCheckBox(void);  /* 0x001D2F80 */
}
#endif

#endif /* GAME_T275_RAW_CHECK_BOX_H */
