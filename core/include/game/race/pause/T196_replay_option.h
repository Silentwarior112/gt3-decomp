/* T196_replay_option.h - ReplayOption, the option window of the replay pause
 * menu (translation unit T196, unit u061), 0x001AEA98..0x001AFA60.
 *
 * ReplayPause (T170) holds it at +0xC.  Its page: RaceMenuPage::load
 * ("replay_panel1", "replay_option") in 1P, ("replay_panel2",
 * "replay_option_2p") in 2P (a split battle replay).  Rows, top to bottom (2P: the screen row first): screen
 * (single / split: GameOptions split), display mode (traditional,
 * avantgarde, broadcast, crs: viewMode, applied through the camera manager,
 * T394), camera target (manual, top, auto: target[viewMode], only the ones
 * the mode allows), display items 0..2 and 3..6 (easymeter, carname, time /
 * map, steering, tire, accel: the bits of items, one column shared by both
 * rows), sound (BGM / SE; the cursor kept on SE for display modes with
 * their own BGM).  Each image is coloured off / on / disabled with the
 * page's colours (select_off / select_on are loaded, not used); the cursor
 * is a blinking additive quad over the current image.
 * The results are written into the game options (T428 / T429). */
#ifndef GAME_T196_REPLAY_OPTION_H
#define GAME_T196_REPLAY_OPTION_H

#include "game/game.h"
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* base RaceMenuLayer */
#include "game/menu/race_menu/T197_race_menu_page.h"       /* RaceMenuPage, RaceMenuColor (held by value) */

namespace gt2k_menu { class Item; }         /* T213_menu_item.h (pointers only) */
class RaceOrganization;                     /* T125_race_organization.h */
class CameraManager;                        /* T383_camera_manager.h */
class RaceInput;                            /* T413_race_input.h */
class GSBuffer;                             /* T514_gs_buffer.h */
class type_info;                            /* T548 */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
/* The __si_type_info at 0x004B23D0, base __ti13RaceMenuLayer. */
extern "C" const type_info &__tf12ReplayOption(void);   /* 0x001AFA10 */
#endif

/* vtable 0x00331480 (1 slot), typeinfo __tf12ReplayOption 0x001AFA10 (si,
 * base RaceMenuLayer).  vptr +0x2C (RaceMenuLayer's).  Size 0x2BC
 * (ReplayPause +0xC .. +0x2C8). */
class ReplayOption : public RaceMenuLayer {
public:
    /* The row of each kind (1P / 2P) and their number. */
    int  rowViewMode();                     /* 0x001AEA98 mode != 0 */
    int  rowTarget();                       /* 0x001AEAA8 1 / 2 */
    int  rowItems1();                       /* 0x001AEAC0 2 / 3 */
    int  rowItems2();                       /* 0x001AEAD8 3 / 4 */
    int  rowSound();                        /* 0x001AEAF0 4 / 5 */
    int  rowScreen();                       /* 0x001AEB08 0 (2P only) */
    int  rowCount();                        /* 0x001AEB10 5 / 6 */

    /* mode: 0 a 1P replay, 1 a 2P (split battle) replay.  The fields
     * cleared (blink, cursorImage and org are not), then load(). */
    ReplayOption(int mode);                 /* 0x001AEB28 */
    /* Everything under obj released; then ~RaceMenuPage, ~RaceMenuLayer. */
    virtual ~ReplayOption();                /* [0] +0x0C 0x001AEC40 */

    /* The page, its "defaultLayer", loadColors and loadImages.  1 when
     * loaded. */
    int  load();                            /* 0x001AEC98 */
    /* colors[] from the images color_off .. color_select_on (vertex 0). */
    void loadColors();                      /* 0x001AED20 */
    /* images[0..15] (g_002C4690's names), in 2P [17] screen_single and [18]
     * screen_v_split / screen_h_split. */
    void loadImages();                      /* 0x001AED90 */
    /* Every image's colour and the cursor image from the game options and
     * the columns. */
    void refresh();                         /* 0x001AEE78 */
    /* colors[2] when disabled, else colors[1] (on) or colors[0] (off). */
    void setColor(int image, int disabled, int on);    /* 0x001AF140 */
    /* Opened: the columns from the game options (viewMode, its target,
     * split), cursor 0, blink 0, active, refresh(). */
    void open();                            /* 0x001AF180 */
    void close();                           /* 0x001AF210 active = 0 */
    /* One frame of input while open (the cursor, the toggles, written into
     * the game options and the camera).  Returns 0 when closed. */
    int  update(RaceInput *in);             /* 0x001AF218 */
    /* The layer and the cursor inside a menu draw, while open. */
    void draw(GSBuffer *buf);               /* 0x001AF8A0 */
    void resetBlink();                      /* 0x001AF8F0 blink = 0.5 */
    void tickBlink();                       /* 0x001AF900 0.5 down to 0 in 30 frames, then again */
    void drawCursor();                      /* 0x001AF938 the quad over cursorImage */
    /* The race and its camera manager (ReplayPause::F_001A3248). */
    void setRace(RaceOrganization *org, CameraManager *cm);    /* 0x001AF9F8 */
    int  isOpen();                          /* 0x001AFA08 active */

    int               mode;                 /* 0x030 0: 1P replay, 1: 2P (split battle) replay */
    RaceMenuPage      page;                 /* 0x034 */
    int               active;               /* 0x1F0 open (u058 open) */
    int               cursor;               /* 0x1F4 the row */
    int               viewMode;             /* 0x1F8 the display mode row's column */
    int               target;               /* 0x1FC the target row's column */
    int               items1;               /* 0x200 the items 0..2 row's column */
    int               items2;               /* 0x204 the items 3..6 row's column */
    int               sound;                /* 0x208 0 BGM, 1 SE */
    int               screen;               /* 0x20C 0 single, 1 split */
    f32               blink;                /* 0x210 the alpha of the cursor */
    RaceMenuColor     colors[5];            /* 0x214 off, on, disable, select_off, select_on */
    /* [0..15] g_002C4690's names, [16] unused, [17], [18] the screen row's */
    gt2k_menu::Item  *images[19];           /* 0x264 */
    gt2k_menu::Item  *cursorImage;          /* 0x2B0 */
    RaceOrganization *org;                  /* 0x2B4 */
    CameraManager    *camera;               /* 0x2B8 */
};
EGCS_SIZE(ReplayOption, 0x2BC);

#endif /* GAME_T196_REPLAY_OPTION_H */
