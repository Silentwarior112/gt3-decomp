/* T199_race_menu_layer.h - RaceMenuLayer, RaceMenuWindow and
 * RaceMenuSelectWindow (translation unit T199, unit u062),
 * 0x001B0678..0x001B0F28.
 *
 * A layer owns one object built from a layout element (`root'), hung under
 * its RootObject `obj'; the images of the layer are Items attached under
 * root (T197's helpers).  A window adds a frame image whose size / position
 * stand for the window's; a select window moves a cursor over buttons
 * (RaceMenuButton, T198) with the pad (Control, T474).  Users: the pause
 * menus (T169..T171), the replay option (T196), the result bars (T200).
 *
 * RaceMenuWindow is here, not in T169 where the plan put it (h28): the chain
 * RaceMenuLayer <- RaceMenuWindow <- RaceMenuSelectWindow <- PauseMenu
 * (T169) would make the two headers include each other.  Its methods are
 * T199's; its constructor and destructor are inline (T169 / T170 / T171 /
 * T200 each emit a vtable and a weak copy of the destructor; T169's link
 * first).  RaceMenuSelectWindow is inline but for its two methods (its
 * typeinfo function is emitted in T169..T171).  The typeinfo functions and
 * nodes of both are declared in T169 (T169's copies won the link);
 * __ti13RaceMenuLayer is declared here.
 *
 * The small members at the end of T199 (0x001B0E80..0x001B0EA0: isVisible
 * .. the constructor, then __tf13RaceMenuLayer) are out-of-line copies of
 * inline functions: the constructor is expanded in every derived
 * constructor (T169, T170, T196, T200). */
#ifndef GAME_T199_RACE_MENU_LAYER_H
#define GAME_T199_RACE_MENU_LAYER_H

#include "game/game.h"
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::RootObject (held by value) */
/* RaceMenuButton (and the operator list classes through it) before the
 * layers: every original that sees the windows has their names in this
 * order (RaceMenuSelectWindow, RaceMenuWindow, RaceMenuButton,
 * OperatorList ..: reverse definition order), u042 data pass. */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* RaceMenuButton (RaceMenuSelectWindow's buttons) */

namespace gt2k_menu {
class Transitive;                           /* T216_menu_transitive.h (pointers only) */
class Item;                                 /* T213_menu_item.h */
class PageLayout;                           /* T229_page_layout.h */
}
class GSBuffer;                             /* T514_gs_buffer.h */
class Control;                              /* T474_control.h */
class RaceMenuButton;                       /* T198_race_menu_button.h */
class type_info;                            /* T548 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
/* The __user_type_info at 0x003533E0 (gp-0x4610), no base. */
extern "C" const type_info &__tf13RaceMenuLayer(void);  /* 0x001B0EE8 */
#endif

/* vtable 0x00331A18 (1 slot), typeinfo __tf13RaceMenuLayer 0x001B0EE8
 * (user).  0x2C bytes of data, egcs vptr +0x2C: size 0x30. */
class RaceMenuLayer {
public:
    /* Out-of-line copy 0x001B0EA0. */
    RaceMenuLayer() : root(0), visible(1) {}
    /* release(), clear(), then ~RootObject of obj. */
    virtual ~RaceMenuLayer();               /* [0] +0x0C 0x001B0678 */

    /* root = the object of layer `name' (race_menu_build_layer), appended
     * to obj's children. */
    void load(gt2k_menu::PageLayout *layout, const char *name);     /* 0x001B06E0 */
    /* A new Item of element `name' under root (NULL without a root). */
    gt2k_menu::Item *newItem(gt2k_menu::PageLayout *layout, const char *name,
                             int sorted);   /* 0x001B0720 */
    /* item attached under root when there is one; returns item. */
    gt2k_menu::Transitive *attach(gt2k_menu::Transitive *item, int sorted);   /* 0x001B0758 */
    /* o->virtual_4(NULL) when o is not NULL: o disappears with its default
     * close script (? a disappear helper; no callers). */
    void disappear(gt2k_menu::Object *o);   /* 0x001B0780 o->virtual_4(NULL) (u062 F_001B0780) */
    /* item->visible = on when item is not NULL. */
    void showItem(gt2k_menu::Transitive *item, int on);             /* 0x001B07B8 */
    /* The tree drawn (no projection set-up) when root and visible: sorted
     * by priority or in list order. */
    void drawTree(int sorted);              /* 0x001B07C8 */
    /* drawTree inside a menu draw of buf (race_menu_begin_draw / end). */
    void draw(GSBuffer *buf, int sorted);   /* 0x001B0818 */
    void update();                          /* 0x001B0890 one frame of the tree */
    void clear();                           /* 0x001B08B0 everything under obj deleted (deleteChildren), then a frame */
    void release();                         /* 0x001B08E0 root killed (NULL), then a frame (which steps it) */
    /* root->disappearAll(script) (T220): virtual_4(script) on root and every
     * node below it, so each Transitive / Item starts the script (the result
     * bars' fade in / out). */
    void startScript(const gt2k_menu::OperatorCode *script);   /* 0x001B0920 */
    void resetScripts();                    /* 0x001B09A8 every Transitive under root loses its operators */
    /* No callers (out-of-line copies 0x001B0E80 .. 0x001B0E98, which egcs
     * emits in reverse declaration order: isVisible first). */
    gt2k_menu::Transitive *getRoot() const { return root; }
    gt2k_menu::Transitive *getRoot() { return root; }
    void setVisible(int on) { visible = on; }
    int  isVisible() { return visible; }

    gt2k_menu::Transitive *root;            /* 0x00 (u057 / u058 / u061 void *) */
    gt2k_menu::RootObject  obj;             /* 0x04 */
    int                    visible;         /* 0x28 */
    /*                     vptr                0x2C */
};
EGCS_SIZE(RaceMenuLayer, 0x30);

/* vtable: one per user TU (0x00330140 T169, 0x00330330 T170, 0x00330550
 * T171, 0x00332040 T200; 1 slot), typeinfo __tf14RaceMenuWindow (si, base
 * RaceMenuLayer; T169's copy 0x001A2E78).  Size 0x34. */
class RaceMenuWindow : public RaceMenuLayer {
public:
    RaceMenuWindow() : frame(0) {}
    /* Copies 0x001A2E50 (T169, the one linked), 0x001B2750 (T200), ... */
    virtual ~RaceMenuWindow() {}            /* [0] +0x0C 0x001A2E50 */

    /* The layer `name', and its frame image `frameName' when not NULL. */
    void load(gt2k_menu::PageLayout *layout, const char *name,
              const char *frameName);       /* 0x001B09E0 */
    void loadFrame(gt2k_menu::PageLayout *layout, const char *name);  /* 0x001B0A30 frame = newItem(.., 1) */
    void setFrame(gt2k_menu::Transitive *f);/* 0x001B0A60 */
    /* The frame's size / position (x, y: plus the root's), -1 without a
     * frame or a root. */
    f32  frameWidth();                      /* 0x001B0A68 (no callers) */
    f32  frameHeight();                     /* 0x001B0A98 */
    f32  frameX();                          /* 0x001B0AC8 (no callers) */
    f32  frameY();                          /* 0x001B0B00 */
    /* Puts root at rest (MotionValue::set) so that the frame is at x / y. */
    void setFrameX(f32 x);                  /* 0x001B0B38 (no callers) */
    void setFrameY(f32 y);                  /* 0x001B0B70 */

    gt2k_menu::Transitive *frame;           /* 0x30 (u057 / u058 int f30) */
};
EGCS_SIZE(RaceMenuWindow, 0x34);

/* typeinfo __tf20RaceMenuSelectWindow (si, base RaceMenuWindow; T169's
 * 0x001A2E00); no vtable of its own survives.  Size 0x50 (PauseMenu's data
 * from +0x50). */
class RaceMenuSelectWindow : public RaceMenuWindow {
public:
    RaceMenuSelectWindow()
        : items(0), count(0), cursor(0), decided(0), cancelled(0), anyInput(0), sound(1) {}

    /* One frame: the layer's update(), decided / cancelled (pressed 0x40 /
     * 0x80; their sounds when `sound'), anyInput, the cursor moved by one
     * with auto-repeat (forward only onto an enabled item; both items
     * refreshed), then refreshItems(). */
    void input(Control *in);                /* 0x001B0BA8 */
    /* Each item selected when it is the cursor's (refreshed when that
     * changes), then its update(). */
    void refreshItems();                    /* 0x001B0DD0 */
    /* The results of input() (RacePause::virtual_2, T169: retail reads them
     * through the window's address, the inline-member form). */
    int isDecided() { return decided; }
    int isCancelled() { return cancelled; }
    int hasInput() { return anyInput; }
    RaceMenuButton *current() { return items[cursor]; }    /* the item under the cursor */

    RaceMenuButton **items;                 /* 0x34 [count] */
    int              count;                 /* 0x38 */
    int              cursor;                /* 0x3C */
    int              decided;               /* 0x40 input bit 0x40 */
    int              cancelled;             /* 0x44 input bit 0x80 */
    int              anyInput;              /* 0x48 (u057 / u058 f48) */
    int              sound;                 /* 0x4C plays the cursor sounds (u057 / u058 f4C) */
};
EGCS_SIZE(RaceMenuSelectWindow, 0x50);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
/* RaceMenuLayer's __user_type_info {vptr, name}, filled by
 * __tf13RaceMenuLayer; the base node of __tf14RaceMenuWindow (u057 / u058 /
 * u061 / u062: g_003533E0; the plan put it in T167, h28 in T169). */
extern "C" u32 __ti13RaceMenuLayer[2];      /* 0x003533E0 gp-0x4610 */
#endif

#endif /* GAME_T199_RACE_MENU_LAYER_H */
