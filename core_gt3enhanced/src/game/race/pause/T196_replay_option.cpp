/* T196 - ReplayOption: the option window of the replay pause menu
 * (ReplayPause +0xC).  0x001AEA98..0x001AFA60.
 *
 * Page "replay_panel1" / "replay_option" (1P) or "replay_panel2" /
 * "replay_option_2p" (2P).  Rows, top to bottom (2P: the screen row first):
 *   screen      single / split (2P only; GameOptions replaySplit)
 *   display     traditional, avantgarde, broadcast, crs (GameOptions viewMode,
 *               applied through the camera manager, F_0020EBA0)
 *   target      manual, top, auto (GameOptions target[viewMode]; only the
 *               targets the display mode allows, ReplayViewInfo targets)
 *   items 0..2  easymeter, carname, time  \  toggles of GameOptions
 *   items 3..6  map, steering, tire, accel /  displayFlags, one column shared
 *   sound       BGM / SE toggles (the cursor kept on SE for display modes
 *               with their own BGM, which choosing them turns on)
 * In a split-screen 2P replay the display and target rows are skipped and
 * carname is disabled.  Each image is coloured off / on / disabled with the
 * page's colours, the cursor is a blinking additive quad over the current
 * image. */
/* Retail's .rodata (0x00331288..0x003316D8): "bad_alloc" (<new>'s inline
 * what(), written when the header is read), the image names of the .data
 * tables (only the tables point at them: DATA_START), the page names, the
 * vtable, then the type names in the reverse order of the classes'
 * definitions: ReplayOption, RaceMenuSelectWindow, RaceMenuWindow,
 * RaceMenuButton, RaceCameraBase, UnitArena<64>, OperatorList, List /
 * Cell<OperatorHolder>, OperatorHolder, the garage lists, Operator (the
 * non-polymorphic base, last). */
// DATA_START 0x00331288
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include <new>
#include "game/save/T429_game_status.h"          /* gt2k_game::GameStatus, getGameStatus; (seen) the garage */
#include "game/menu/framework/T216_menu_transitive.h"      /* (seen) the operator list classes */
#include "game/race/core/T153_race_course.h"          /* (seen, not used) */
#include "game/camera/T383_race_camera_base.h"     /* (seen, not used) */
#include "game/menu/race_menu/T198_race_menu_button.h"     /* (seen, not used) */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* base RaceMenuLayer */
#include "game/race/pause/T196_replay_option.h"
#include "engine/pgl/G001_flare.h"                /* gfx_blend_func, pgl_*, vu0_scale, PGL_* */
#include "game/app/T053_gtmode_races.h"         /* ::GameOptions (T428) */
#include "game/camera/T383_camera_manager.h"       /* CameraManager::kinds */
#include "game/camera/T394_replay_view_mode.h"     /* ReplayViewInfo, F_0020EA78 / F_0020EB38 / F_0020EBA0 */
#include "game/race/core/T413_race_input.h"           /* RaceInput */
#include "engine/input/T474_control.h"              /* Control::getButtonsOn / getRepeat, CONTROL_* */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/menu/race_menu/T197_race_menu_page.h"       /* RaceMenuPage, RaceMenuColor, race_menu_* */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item (a Drawable: race_menu_set_colors) */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::RootObject */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#include "game/effects/replay_effects_internal.h"                           /* REPLAY_*, BLEND_ADD, the tables below */

/* The image names (.data 0x002C4690 .. 0x002C46F4), defined before the
 * functions as in the original: their strings come right after
 * "bad_alloc", in front of the page names. */
const char *g_002C4690[REPLAY_IMG_NAMED] = {   /* 0x002C4690 */
    "mode_traditional",                 /* 0x00331298 */
    "mode_avantgarde",                  /* 0x003312B0 */
    "mode_broadcast",                   /* 0x003312C0 */
    "mode_crs",                         /* 0x003312D0 */
    "item_easymeter",                   /* 0x003312E0 */
    "item_carname",                     /* 0x003312F0 */
    "item_time",                        /* 0x00331300 */
    "item_map",                         /* 0x00331310 */
    "item_steering",                    /* 0x00331320 */
    "item_tire",                        /* 0x00331330 */
    "item_accel",                       /* 0x00331340 */
    "target_manual",                    /* 0x00331350 */
    "target_top",                       /* 0x00331360 */
    "target_auto",                      /* 0x00331370 */
    "sound_bgm",                        /* 0x00331380 */
    "sound_se",                         /* 0x00331390 */
};
const char *g_002C46D0[3] = {           /* 0x002C46D0 */
    "screen_single",                    /* 0x003313A0 */
    "screen_v_split",                   /* 0x003313B0 */
    "screen_h_split",                   /* 0x003313C0 */
};
const char *g_002C46E0[REPLAY_COLORS] = {   /* 0x002C46E0 */
    "color_off",                        /* 0x003313D0 */
    "color_on",                         /* 0x003313E0 */
    "color_disable",                    /* 0x003313F0 */
    "color_select_off",                 /* 0x00331400 */
    "color_select_on",                  /* 0x00331418 */
};
/* The cursor quad, a strip of 4 (x, y, z).  16-byte aligned in the original:
 * it is at 0x002C4700, 12 zero bytes after g_002C46E0, and T196's .data
 * starts at 0x002C4690, 8 zero bytes after T187's. */
f32 g_002C4700[4][3] __attribute__((aligned(16))) = {   /* 0x002C4700 */
    { -1.0f,  1.0f, 0.0f },
    {  1.0f,  1.0f, 0.0f },
    { -1.0f, -1.0f, 0.0f },
    {  1.0f, -1.0f, 0.0f },
};

// FUNC 0x001AEA98 ReplayOption::rowViewMode
int ReplayOption::rowViewMode()
{
    return mode != REPLAY_OPTION_1P;
}

// FUNC 0x001AEAA8 ReplayOption::rowTarget
int ReplayOption::rowTarget()
{
    return (mode != REPLAY_OPTION_1P) ? 2 : 1;
}

// FUNC 0x001AEAC0 ReplayOption::rowItems1
int ReplayOption::rowItems1()
{
    return (mode != REPLAY_OPTION_1P) ? 3 : 2;
}

// FUNC 0x001AEAD8 ReplayOption::rowItems2
int ReplayOption::rowItems2()
{
    return (mode != REPLAY_OPTION_1P) ? 4 : 3;
}

// FUNC 0x001AEAF0 ReplayOption::rowSound
int ReplayOption::rowSound()
{
    return (mode != REPLAY_OPTION_1P) ? 5 : 4;
}

/* Only meaningful in 2P (in 1P row 0 is the display mode's). */
// FUNC 0x001AEB08 ReplayOption::rowScreen
int ReplayOption::rowScreen()
{
    return 0;
}

// FUNC 0x001AEB10 ReplayOption::rowCount
int ReplayOption::rowCount()
{
    return (mode != REPLAY_OPTION_1P) ? 6 : 5;
}

// FUNC 0x001AEB28 ReplayOption::ReplayOption
/* Caller: ReplayPause's constructor (0x001A30C0).  RaceMenuLayer() (its
 * obj: gt2k_menu::RootObject::RootObject) and the five RaceMenuColor()
 * (0.502) are inline; blink, images, cursorImage and org are not set.  On
 * an exception: ~RaceMenuPage(page, 0), RaceMenuLayer::~RaceMenuLayer(this,
 * 0). */
ReplayOption::ReplayOption(int mode)
    : RaceMenuLayer(), mode(mode), page(), active(0), cursor(0), viewMode(0), target(0),
      items1(0), items2(0), sound(0), screen(0), camera(NULL)
{
    /* implicit: RootObject (in RaceMenuLayer's inline constructor), RaceMenuPage (member constructor), ~RaceMenuPage (exception cleanup) */
    load();
}

// FUNC 0x001AEC40 ReplayOption::~ReplayOption
/* Slot 0 (index.txt: ReplayOption::virtual_0).  Callers: ReplayPause's
 * constructor (exception path) and destructor.  The object tree released,
 * then the members' and the base's destructors: RaceMenuPage::~RaceMenuPage
 * (page, 2), RaceMenuLayer::~RaceMenuLayer(this, flags). */
ReplayOption::~ReplayOption()
{
    obj.deleteChildren();
    /* implicit: ~RaceMenuPage (member destructor), ~RaceMenuLayer (base destructor) */
}

// FUNC 0x001AEC98 ReplayOption::load
int ReplayOption::load()
{
    if (mode == REPLAY_OPTION_1P) {
        if (page.load("replay_panel1", "replay_option") == 0)
            return 0;
    } else {
        if (page.load("replay_panel2", "replay_option_2p") == 0)
            return 0;
    }
    RaceMenuLayer::load(&page.layout, "defaultLayer");
    loadColors();
    loadImages();
    return 1;
}

/* colors[] from the images color_off .. color_select_on (their colour 0). */
// FUNC 0x001AED20 ReplayOption::loadColors
void ReplayOption::loadColors()
{
    for (int i = 0; i < REPLAY_COLORS; i++)
        race_menu_get_color(&colors[i], &page.layout, g_002C46E0[i], 0);
}

/* images[0..15], and in 2P [17] screen_single and [18] screen_v_split (wide
 * screen: the halves side by side) or screen_h_split.  [16] stays unset. */
// FUNC 0x001AED90 ReplayOption::loadImages
void ReplayOption::loadImages()
{
    gt2k_menu::PageLayout *layout = &page.layout;

    for (int i = 0; i < REPLAY_IMG_NAMED; i++)
        images[i] = newItem(layout, g_002C4690[i], 1);
    if (mode != REPLAY_OPTION_1P) {
        ::GameOptions *opt = &getGameStatus()->options;
        images[REPLAY_IMG_SCREEN_SINGLE] = newItem(layout, g_002C46D0[0], 1);
        images[REPLAY_IMG_SCREEN_SPLIT] = newItem(layout, g_002C46D0[(opt->wide != 0) ? 1 : 2], 1);
    }
}

/* The colours of every image from the game options, and the cursor image
 * of the current row. */
// FUNC 0x001AEE78 ReplayOption::refresh
void ReplayOption::refresh()
{
    int split = 0;
    ::GameOptions *opt = &getGameStatus()->options;
    int curMode = opt->viewMode;
    int curTarget = opt->target[curMode];
    int shown = opt->displayFlags;
    int bgmOn = opt->replayBgm;
    int seOn = opt->replaySe;                           /* sp+0x00 */
    if (mode != REPLAY_OPTION_1P)
        split = opt->replaySplit;
    ReplayViewInfo *info = F_0020EA78(camera, curMode);

    if (cursor == rowViewMode())
        cursorImage = images[REPLAY_IMG_MODE + viewMode];
    for (int i = 0; i < REPLAY_VIEW_MODES; i++)
        setColor(REPLAY_IMG_MODE + i, split, curMode == i);   /* disabled = split as it is */

    int bit = 1;
    for (int i = 0; i < REPLAY_ITEMS1 + REPLAY_ITEMS2; i++, bit <<= 1) {
        int img = REPLAY_IMG_ITEM + i;
        int disabled = 0;
        if (split != 0 && i == REPLAY_ITEM_CARNAME)
            disabled = 1;
        setColor(img, disabled, (shown & bit) != 0);
    }

    if (cursor == rowItems1())
        cursorImage = images[REPLAY_IMG_ITEM + items1];
    if (cursor == rowItems2())
        cursorImage = images[REPLAY_IMG_ITEM2 + items2];
    if (cursor == rowTarget())
        cursorImage = images[REPLAY_IMG_TARGET + target];
    for (int i = 0; i < REPLAY_TARGETS; i++) {
        int img = REPLAY_IMG_TARGET + i;
        int disabled = 0;
        if (((info->targets >> i) & 1) == 0 || split != 0)
            disabled = 1;
        setColor(img, disabled, curTarget == i);
    }

    if (cursor == rowSound())
        cursorImage = images[REPLAY_IMG_SOUND + sound];
    setColor(REPLAY_IMG_SOUND + 0, 0, bgmOn);
    setColor(REPLAY_IMG_SOUND + 1, 0, seOn);

    if (mode != REPLAY_OPTION_1P) {
        if (cursor == rowScreen())
            cursorImage = images[(screen != 0) ? REPLAY_IMG_SCREEN_SPLIT : REPLAY_IMG_SCREEN_SINGLE];
        setColor(REPLAY_IMG_SCREEN_SINGLE, 0, split == 0);
        setColor(REPLAY_IMG_SCREEN_SPLIT, 0, split);
    }
}

/* colors[REPLAY_COLOR_DISABLE] when disabled, else on / off.  (The
 * select_off / select_on colours are loaded but not used.) */
// FUNC 0x001AF140 ReplayOption::setColor
void ReplayOption::setColor(int image, int disabled, int on)
{
    const RaceMenuColor *c;
    const RaceMenuColor *off = &colors[REPLAY_COLOR_OFF];  /* retail computes it first (t0) */

    if (disabled != 0)
        c = &colors[REPLAY_COLOR_DISABLE];
    else
        c = (on != 0) ? &colors[REPLAY_COLOR_ON] : off;
    race_menu_set_colors(c, images[image]);
}

/* Caller: ReplayPause::virtual_2.  The columns from the game options. */
// FUNC 0x001AF180 ReplayOption::open
void ReplayOption::open()
{
    ::GameOptions *opt = &getGameStatus()->options;

    viewMode = opt->viewMode;
    if (viewMode >= REPLAY_VIEW_MODES)
        viewMode = 0;
    target = opt->target[viewMode];
    if (mode != REPLAY_OPTION_1P)
        screen = opt->replaySplit != 0;
    else
        screen = 0;
    cursor = 0;
    blink = 0.0f;
    active = 1;
    refresh();
}

/* Caller: update (cancel). */
// FUNC 0x001AF210 ReplayOption::close
void ReplayOption::close()
{
    active = 0;
}

/* Caller: ReplayPause::virtual_2.  One frame of input: 0 when closed (or
 * closed now by cancel), else 1.  Up / down move between the rows, left /
 * right between the columns, decide applies the column (and plays
 * MENU_SE_CONFIRM); a move plays MENU_SE_CURSOR and restarts the blink. */
// FUNC 0x001AF218 ReplayOption::update
int ReplayOption::update(RaceInput *in)
{
    if (active == 0)
        return 0;

    int changed = 0;
    ::GameOptions *opt = &getGameStatus()->options;
    /* Control::getOn is inline (andi 0x40; sltu: the mask is a parameter
     * when the store-flag is expanded) */
    int decide = in->getOn(CONTROL_DECIDE);                     /* sp+0x00 */
    int cancel = in->getOn(CONTROL_CANCEL);
    int up = in->getRepeat(CONTROL_UP, REPLAY_REPEAT_FIRST, REPLAY_REPEAT_INTERVAL);
    int down = in->getRepeat(CONTROL_DOWN, REPLAY_REPEAT_FIRST, REPLAY_REPEAT_INTERVAL);
    int left = in->getRepeat(CONTROL_LEFT, REPLAY_REPEAT_FIRST, REPLAY_REPEAT_INTERVAL);
    int right = in->getRepeat(CONTROL_RIGHT, REPLAY_REPEAT_FIRST, REPLAY_REPEAT_INTERVAL);

    if (cancel) {
        menu_play_se(MENU_SE_CANCEL);
        close();
        return 0;
    }

    /* the row */
    if (cursor < rowCount() - 1 && down != 0) {
        changed = 1;
        cursor = cursor + 1;
    } else if (cursor > 0 && up != 0) {
        changed = 1;
        cursor = cursor - 1;
    }
    if (mode != REPLAY_OPTION_1P && opt->replaySplit != 0) {
        /* split screen: no display mode, no target (down from the screen
         * row lands on the items, up from the items on the screen row) */
        if (cursor == rowViewMode())
            cursor = rowItems1();
        if (cursor == rowTarget())
            cursor = rowScreen();
    }

    if (cursor == rowViewMode()) {
        int col = viewMode;
        if (col > 0 && left != 0) {
            changed = 1;
            viewMode = col - 1;
            col = col - 1;
        }
        if (col < REPLAY_VIEW_MODES - 1 && right != 0) {
            changed = 1;
            viewMode = col + 1;
        }
        if (decide) {
            ReplayViewInfo *info = F_0020EA78(camera, viewMode);
            int newMode = viewMode;
            int newTarget = opt->target[newMode];
            if (((info->targets >> newTarget) & 1) == 0)
                newTarget = info->defTarget;
            target = newTarget;
            F_0020EBA0(camera, org, 0, newMode, newTarget);
            if (info->bgm != 0)
                opt->replayBgm = 1;
            menu_play_se(MENU_SE_CONFIRM);
        }
    } else if (cursor == rowScreen()) {
        int col = screen;
        if (col > 0 && left != 0) {
            col = col - 1;
            changed = 1;
            screen = col;
        }
        if (col <= 0 && right != 0) {
            changed = 1;
            screen = col + 1;
        }
        if (decide) {
            opt->replaySplit = screen != 0;
            F_0020EB38(camera, org, 0);
            menu_play_se(MENU_SE_CONFIRM);
        }
    } else if (cursor == rowItems1() || cursor == rowItems2()) {
        /* one column for both rows: items1 follows items2 (clamped to 2),
         * items2 follows items1 except that it stays on 3 from 2 */
        int first = cursor == rowItems1();
        int split = 0;
        if (mode != REPLAY_OPTION_1P)
            split = opt->replaySplit != 0;
        int col = first ? items1 : items2;
        int cols = first ? REPLAY_ITEMS1 : REPLAY_ITEMS2;
        if (col > 0 && left != 0) {
            col = col - 1;
            changed = 1;
        }
        if (col < cols - 1 && right != 0) {
            col = col + 1;
            if (split != 0 && col == REPLAY_ITEM_CARNAME && first)
                col = 2;                                /* over carname */
            changed = 1;
        }
        if (split != 0 && col == REPLAY_ITEM_CARNAME && first)
            col = 0;                                    /* moved left onto carname */
        if (first) {
            items1 = col;
            items2 = (items2 == 3 && col == 2) ? 3 : col;
        } else {
            items2 = col;
            items1 = (col < 3) ? col : 2;
        }
        int shift = first ? items1 : items2 + REPLAY_ITEMS1;
        int bit = 1 << shift;
        if (decide) {
            opt->displayFlags = opt->displayFlags ^ bit;
            menu_play_se(MENU_SE_CONFIRM);
        }
    } else if (cursor == rowTarget()) {
        int curMode = opt->viewMode;
        ReplayViewInfo *info = F_0020EA78(camera, curMode);
        int col = target;
        int def = info->defTarget;
        if (((info->targets >> col) & 1) == 0) {
            target = def;
            F_0020EBA0(camera, org, 0, curMode, def);
            col = target;
        }
        int old = col;
        if (left != 0) {
            /* the previous allowed target, else back to old */
            for (;;) {
                int prev = col - 1;
                target = prev;
                if (col <= 0)
                    break;
                col = prev;
                if ((info->targets >> prev) & 1) {
                    changed = 1;
                    break;
                }
            }
            if (target < 0)
                target = old;
        }
        if (right != 0) {
            /* the next allowed target, else back to old */
            while (target++ < REPLAY_TARGETS - 1) {
                if ((info->targets >> target) & 1) {
                    changed = 1;
                    break;
                }
            }
            if (target >= REPLAY_TARGETS)
                target = old;
        }
        if (decide) {
            int chosen = target;
            if ((info->targets >> chosen) & 1)
                F_0020EBA0(camera, org, 0, curMode, chosen);
            menu_play_se(MENU_SE_CONFIRM);
        }
    } else if (cursor == rowSound()) {
        /* the mode of view 0 (CameraManager +0x96E4): retail forms the
         * address of kinds[0] (ori 0x96E0) and reads +4 from it */
        CameraViewKind *k = camera->kinds;
        ReplayViewInfo *info = F_0020EA78(camera, k->kind);
        int ownBgm = info->bgm != 0;
        int col = sound;
        if (col > 0 && left != 0) {
            col = col - 1;
            changed = 1;
            sound = col;
        }
        if (col <= 0 && right != 0) {
            changed = 1;
            sound = col + 1;
        }
        if (ownBgm)
            sound = 1;                                  /* the BGM toggle is locked */
        if (decide) {
            if (sound == 0)
                opt->replayBgm = opt->replayBgm != 1;
            else
                opt->replaySe = opt->replaySe != 1;
            menu_play_se(MENU_SE_CONFIRM);
        }
    }

    if (changed != 0) {
        resetBlink();
        menu_play_se(MENU_SE_CURSOR);
    }
    refresh();
    tickBlink();
    obj.virtual_1();                                    /* RootObject::virtual_1, a direct call */
    return 1;
}

/* Caller: ReplayPause::virtual_3.  RaceMenuLayer::draw's steps with the
 * cursor between the tree and the end of the menu draw. */
// FUNC 0x001AF8A0 ReplayOption::draw
void ReplayOption::draw(GSBuffer *buf)
{
    if (active != 0) {
        race_menu_begin_draw(buf);
        drawTree(1);
        drawCursor();
        race_menu_end_draw();
    }
}

// FUNC 0x001AF8F0 ReplayOption::resetBlink
void ReplayOption::resetBlink()
{
    blink = REPLAY_BLINK_START;
}

/* 0.5 down to 0 in 30 frames, then again. */
// FUNC 0x001AF900 ReplayOption::tickBlink
void ReplayOption::tickBlink()
{
    f32 b = blink - REPLAY_BLINK_STEP;
    blink = b;
    if (b < 0.0f)
        blink = REPLAY_BLINK_START;
}

/* The quad g_002C4700 (-1..1) over the cursor image (its centre, half its
 * size: the image exactly), additive, alpha = blink. */
// FUNC 0x001AF938 ReplayOption::drawCursor
void ReplayOption::drawCursor()
{
    if (cursorImage != NULL) {
        gfx_blend_func(BLEND_ADD, 1.0f);
        pgl_color4f(1.0f, 1.0f, 1.0f, blink);
        pgl_push_matrix();
        pgl_translate(cursorImage->x.value, cursorImage->y.value, 0.0f);
        vu0_scale(cursorImage->w * 0.5f, cursorImage->h * 0.5f, 1.0f);
        pgl_draw_strip(g_002C4700, NULL, NULL, NULL, 4);
        pgl_pop_matrix();
    }
}

/* No callers (ReplayPause::F_001A3248 stores the same fields inline). */
// FUNC 0x001AF9F8 ReplayOption::setRace
void ReplayOption::setRace(RaceOrganization *org, CameraManager *cm)
{
    this->org = org;
    camera = cm;
}

/* No callers. */
// FUNC 0x001AFA08 ReplayOption::isOpen
int ReplayOption::isOpen()
{
    return active;
}

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x001AFA10 __tf12ReplayOption
/* __si_type_info __ti12ReplayOption (0x004B23D0), base __ti13RaceMenuLayer. */
extern "C" const type_info &__tf12ReplayOption(void)
{
    if (__ti12ReplayOption[0] == 0) {
        __tf13RaceMenuLayer();
        __rtti_si(__ti12ReplayOption, "12ReplayOption", (const type_info *)__ti13RaceMenuLayer);
    }
    return *(const type_info *)__ti12ReplayOption;
}
#endif
