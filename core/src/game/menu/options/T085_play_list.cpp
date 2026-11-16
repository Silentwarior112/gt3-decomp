/* T085 - the music play list windows of the options screen.
 * 0x00169780..0x0016B290.
 *
 * gt2k_menu::PlayListBox (the "song_edit" / "song_list" hooks of
 * OptionControl, u022) holds a PlayList: one PlayMusicBox row per song of
 * the play list being edited (OptionControl::options.music, GameOptions
 * +0x228) and a PlayModeBox (straight / shuffle).  A row is a check box
 * (the song on / off; checking it plays the song) and a "title / artist"
 * label; deciding the label picks the song up into the PlayMusicBoxPop
 * (PLAY_EVENT_POP_OPEN), with which it is moved up / down the list until it
 * is put down (PLAY_EVENT_POP_CLOSE); cancel stores the rows back into the
 * play list.
 *
 * Per class the RawWindow boilerplate of its own: first its record (the name
 * pointer slot 8 returns, the RawWindowClass {name, create}, the registrar:
 * retail's .sdata order, the name string opening the class's .rodata), then
 * the class name (slot 8), the layout factory, the registry's create,
 * canCreate, the constructor.  egcs builds / destroys the registrars itself
 * (T085_static_init / _dtor below).
 *
 * Includes: retail's .rodata (0x00323040..0x00323938) is the literals, the
 * five vtables, then the type names in the reverse order of the classes'
 * definitions: its own five (PlayListBox, PlayList, PlayModeBox,
 * PlayMusicBoxPop, PlayMusicBox), OperatorList, List<OperatorHolder>,
 * OperatorHolder, Cell<OperatorHolder>, SmartIndexT<int>, the garage's eight
 * (GarageWheelList .. Cell<GarageCar>, GarageCar's base list after its
 * name), Operator (a non-polymorphic base: always last).  No "bad_alloc"
 * (no <new>), no RaceCameraBase.  So the GameStatus first, then T251, the
 * Raw* windows (the operator classes), the other headers (no names), and
 * the play list last.
 */
#include "game/save/T429_game_status.h"          /* (seen: the garage) */
#include "game/common/T251_smart_index.h"          /* (seen: SmartIndexT<int>) */
#include "game/menu/framework/T264_raw_window.h"           /* gt2k_menu::RawWindow, RawWindowArgs, RAW_EVENT_* */
#include "game/menu/framework/T263_raw_list.h"             /* gt2k_menu::RawListItem, RawList */
#include "game/menu/framework/T265_raw_window_register.h"  /* gt2k_menu::RawWindowClass, RawWindowRegister */
#include "game/menu/framework/T273_raw_label.h"            /* gt2k_menu::RawLabel (PlayMusicBox::label) */
#include "game/menu/framework/T275_raw_check_box.h"        /* gt2k_menu::RawCheckBox (checked), RAW_EVENT_CHECK_TOGGLE */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item::setText */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator new / delete, parent) */
#include "game/menu/framework/T247_control_base.h"         /* gt2k_menu::ControlBase (control1) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (noCancel) */
#include "game/menu/options/T062_option_control.h"       /* gt2k_menu::OptionControl (pages, options.music) */
#include "game/menu/framework/T233_menu_sound.h"           /* menu_play_se */
#include "game/save/T430_music_play_list.h"      /* MusicPlayList, MusicEntry, findMusicInfo */
#include "game/sound/T464_bgm.h"                  /* bgm_play_if_changed, bgm_stop */
#include "game/sound/T466_bgm_ads.h"              /* MusicInfo (= BgmTrack: title, artist) */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "engine/media/G002_mpeg_frameq.h"          /* intr_suspend, intr_resume */
#include "game/menu/options/T085_play_list.h"            /* the play list classes: the last defined */
#include "game/menu/options/play_list_internal.h"

namespace gt2k_menu {

/* ======================================================================== */
/* PlayMusicBox                                                             */
/* ======================================================================== */

} /* namespace gt2k_menu */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_003519E0 = "PlayMusicBox";                    /* 0x003519E0 */
gt2k_menu::RawWindowClass g_003519E8 =                      /* 0x003519E8 */
    { "PlayMusicBox", gt2k_menu::PlayMusicBox::create };
gt2k_menu::RawWindowRegister g_003519F0(&g_003519E8);       /* 0x003519F0 */

namespace gt2k_menu {

// FUNC 0x00169780 gt2k_menu::PlayMusicBox::virtual_8
const char *PlayMusicBox::virtual_8()
{
    return g_003519E0;                      /* "PlayMusicBox" */
}

/* No callers in the image: the layout factory. */
// FUNC 0x00169788 gt2k_menu::PlayMusicBox::factory
RawWindow *PlayMusicBox::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                 int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_003519E8.create). */
// FUNC 0x001697C8 gt2k_menu::PlayMusicBox::create
RawWindow *PlayMusicBox::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new PlayMusicBox(*args); /* Object::operator new(0x2E8); operator delete if
                                           the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x00169840 gt2k_menu::PlayMusicBox::canCreate
int PlayMusicBox::canCreate(const RawWindowArgs *args)
{
    return 1;
}

/* p0 / p1: the layout nodes of the check box / label (NULL: "check_box" /
 * "label_box").  A horizontal list of the two, its size locked; the row
 * starts with the play list's first song.  EH: ~RawList() if it throws. */
// FUNC 0x00169848 gt2k_menu::PlayMusicBox::PlayMusicBox
PlayMusicBox::PlayMusicBox(const RawWindowArgs &args)
    : RawList(args)
{
    owner = static_cast<OptionControl *>(args.owner);
    /* both parameters read first (retail loads p1 before the p0 test and
     * copies it into labelNode after it) */
    int p0 = args.p0;
    int p1 = args.p1;
    const char *checkNode = (const char *)p0;
    if (checkNode == NULL)
        checkNode = "check_box";
    const char *labelNode = (const char *)p1;
    if (labelNode == NULL)
        labelNode = "label_box";

    /* Quirk: a copy of the arguments with p0..p3 = 0 is built on the stack
     * (sp+0x00..0x1C) and never used; the children get args itself.
     * (? a copy meant for the children; egcs keeps the dead stores to a
     * stack object.)  Retail's store order: the four fields, focusProperty,
     * then p2, p3, p0, p1 (no other order of the zeros is exact). */
    RawWindowArgs childArgs;
    childArgs.owner = args.owner;
    childArgs.loader = args.loader;
    childArgs.parent = args.parent;
    childArgs.node = args.node;
    focusProperty = "label_check_focus";
    childArgs.p2 = 0;
    childArgs.p3 = 0;
    childArgs.p0 = 0;
    childArgs.p1 = 0;

    setVertical(0, 1);
    sizeLocked = 1;
    song = owner->options.music.entries[0];     /* two ld / sd of owner +0x1090 */

    checkBox = static_cast<RawCheckBox *>(createChild("RawCheckBox", checkNode, &args, 0, 0, 0, 0));
    checkBox->positionLocked = 1;
    addItem(checkBox, 1);

    label = static_cast<RawLabel *>(createChild("RawLabel", labelNode, &args, 0, 0, 0, 0));
    label->positionLocked = 1;
    addItem(label, 1);

    blink = 0.0f;                           /* sw $zero */
}

// FUNC 0x001699E0 gt2k_menu::PlayMusicBox::~PlayMusicBox
PlayMusicBox::~PlayMusicBox()
{
    /* only ~RawList() (0x001D41C8, flags 0), then Object::operator delete
     * if flags & 1 */
    /* implicit: ~RawList, operator delete (the base / member destructors,
     * flags & 1) */
}

/* Inside a PlayMusicBoxPop (the class of this window or of one above):
 * decide puts the song down (PLAY_EVENT_POP_CLOSE), left / right are
 * swallowed.  Otherwise decide on the label picks the song up
 * (PLAY_EVENT_POP_OPEN), the check box's toggle turns the song on / off and
 * plays it.  The rest goes to RawList::virtual_17 (called directly). */
// FUNC 0x00169A38 gt2k_menu::PlayMusicBox::virtual_17
int PlayMusicBox::virtual_17(RawWindow *sender, int event, int arg)
{
    if (isInClass("PlayMusicBoxPop")) {
        switch (event) {
        case RAW_EVENT_DECIDE:
            virtual_19(PLAY_EVENT_POP_CLOSE, 0);
            return 0;
        case RAW_EVENT_LEFT:
        case RAW_EVENT_RIGHT:
            return 0;
        }
    }
    switch (event) {
    case RAW_EVENT_CHECK_TOGGLE:
        if (checkBox == sender) {
            int checked = static_cast<RawCheckBox *>(sender)->checked;
            menu_play_se(MENU_SE_CONFIRM);
            setEnabled(checked);
            preview(checked);               /* a second time, with the whole word */
            return 0;
        }
        break;
    case RAW_EVENT_DECIDE:
        if (label == sender) {
            virtual_19(PLAY_EVENT_POP_OPEN, 0);
            return 0;
        }
        break;
    }
    return RawList::virtual_17(sender, event, arg);    /* 0x001D42B8 */
}

/* With the focus: the image focusProperty, and the colours of the row and
 * of its focused part pulse x (1.0 .. 1.5), blink going 0.5 -> 0 in 30
 * frames; interrupts off while the colours change.  Without it blink is
 * reset.
 * Matching: 93.9%, the rest is two registers.  In the check box's block
 * retail keeps the reloaded checkBox (the colour pointer) in $a0 and the
 * label (loaded before that loop by GCSE) in $v1, copied to $a0 for its
 * changeImage; egcs here swaps them (label's PRE register is inserted on
 * the edge and takes $a0, the copy preference of the call).  Retail looks
 * like the label load was an existing computation copied into the PRE
 * register (a local the size of the block).  Tried: pointer / reference /
 * RawWindow * forms of the parts, inline focusImage / hasFocus helpers, a
 * Color4f * per loop, loop index placement; the reference compiler
 * 991111-01 gives the same. */
// FUNC 0x00169B50 gt2k_menu::PlayMusicBox::virtual_9
void PlayMusicBox::virtual_9()
{
    int i, intr;
    f32 scale;

    if (focus != 0) {
        blink -= PLAY_BLINK_STEP;           /* gp-0x7DC0 0x0034FC30 */
        if (blink < 0.0f)
            blink = PLAY_BLINK_START;
        scale = blink + 1.0f;

        changeImage(focusProperty, 0, 0);
        intr = intr_suspend();
        for (i = 0; i < 4; i++)
            scaleRGB(color[i], scale);
        intr_resume(intr);

        /* RawWindow's focus (+0x164), not RawCheckBox::boxFocus (+0x204) */
        if (checkBox->focus) {
            checkBox->changeImage(checkBox->focusProperty, 0, 0);
            intr = intr_suspend();
            for (i = 0; i < 4; i++)
                scaleRGB(checkBox->color[i], scale);
            intr_resume(intr);
        }
        if (label->focus) {
            label->changeImage(label->focusProperty, 0, 0);
            intr = intr_suspend();
            for (i = 0; i < 4; i++)
                scaleRGB(label->color[i], scale);
            intr_resume(intr);
        }
    } else {
        blink = 0.0f;                       /* sw $zero */
    }
}

/* song.enabled is a byte: the test reads the stored byte (sll 24, the
 * argument's low byte). */
// FUNC 0x00169D18 gt2k_menu::PlayMusicBox::setEnabled
void PlayMusicBox::setEnabled(int enabled)
{
    song.enabled = enabled;
    preview(song.enabled != 0);
}

/* on: plays the row's song (its track of BGM group 0x14, the intro first,
 * volume 1.0) unless it already plays; off: stops the music. */
// FUNC 0x00169D48 gt2k_menu::PlayMusicBox::preview
void PlayMusicBox::preview(int on)
{
    if (on) {
        u32 track;
        findMusicInfo(&song.code, &track);
        bgm_play_if_changed(BGM_GROUP_PLAY_LIST, track | BGM_TRACK_INTRO, 1.0f, 1);
    } else {
        bgm_stop();
    }
}

/* RawList's (called directly); then, unless the popup is shown, the row's
 * song plays if it is enabled (else the music stops).  The parent is the
 * PlayList: the dynamic_cast's result is not checked. */
// FUNC 0x00169D98 gt2k_menu::PlayMusicBox::virtual_20
void PlayMusicBox::virtual_20(int on)
{
    RawList::virtual_20(on);                /* 0x001D4450 */
    PlayList *list = static_cast<PlayList *>(parentWindow());  /* its dynamic_cast */
    /* implicit: __dynamic_cast (the inline parentWindow) */
    if (list->pop->isShown() == 0)
        preview(song.enabled != 0);
}

/* Shows song: the check box state, "<title> / <artist>" in the label. */
// FUNC 0x00169E20 gt2k_menu::PlayMusicBox::setSong
void PlayMusicBox::setSong(const MusicEntry *entry)
{
    song = *entry;
    setCheck(song.enabled != 0);
    MusicInfo *info = findMusicInfo(&song.code, NULL);
    sprintf(text, "%s / %s", info->title, info->artist);   /* 0x003519F8 */
    setLabel(text);
}

/* checkBox->virtual_24 (a virtual call). */
// FUNC 0x00169E98 gt2k_menu::PlayMusicBox::setCheck
void PlayMusicBox::setCheck(int checked)
{
    if (checkBox != NULL)
        checkBox->virtual_24(checked);
}

// FUNC 0x00169ED0 gt2k_menu::PlayMusicBox::setLabel
void PlayMusicBox::setLabel(const char *s)
{
    if (label != NULL)
        label->setText(s);
}

/* ======================================================================== */
/* PlayMusicBoxPop                                                          */
/* ======================================================================== */

} /* namespace gt2k_menu */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351A00 = "PlayMusicBoxPop";                 /* 0x00351A00 */
gt2k_menu::RawWindowClass g_00351A08 =                      /* 0x00351A08 */
    { "PlayMusicBoxPop", gt2k_menu::PlayMusicBoxPop::create };
gt2k_menu::RawWindowRegister g_00351A10(&g_00351A08);       /* 0x00351A10 */

namespace gt2k_menu {

// FUNC 0x00169EF8 gt2k_menu::PlayMusicBoxPop::virtual_8
const char *PlayMusicBoxPop::virtual_8()
{
    return g_00351A00;                      /* "PlayMusicBoxPop" */
}

// FUNC 0x00169F00 gt2k_menu::PlayMusicBoxPop::factory
RawWindow *PlayMusicBoxPop::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                    int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_00351A08.create). */
// FUNC 0x00169F40 gt2k_menu::PlayMusicBoxPop::create
RawWindow *PlayMusicBoxPop::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new PlayMusicBoxPop(*args); /* Object::operator new(0x2E8); operator delete if
                                              the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x00169FB8 gt2k_menu::PlayMusicBoxPop::canCreate
int PlayMusicBoxPop::canCreate(const RawWindowArgs *args)
{
    return 1;
}

/* The base is built from a copy of the arguments with p0 / p1 =
 * "pop_check_box" / "pop_label_box", p2 / p3 = 0.  Not clipped by the
 * parents; the focus on the label (item 1), the focus item does not get the
 * menu control: RawList's slots 31 / 25 called directly.  EH:
 * ~PlayMusicBox() if it throws. */
// FUNC 0x00169FC0 gt2k_menu::PlayMusicBoxPop::PlayMusicBoxPop
PlayMusicBoxPop::PlayMusicBoxPop(const RawWindowArgs &args)
    : PlayMusicBox(RawWindowArgsInit(args.owner, args.loader, args.parent, args.node,
                                     (int)"pop_check_box", (int)"pop_label_box", 0, 0))
{
    noParentClip = 1;
    RawList::virtual_31(1);                 /* 0x001D4B88 */
    RawList::virtual_25(0);                 /* 0x001D4608 */
}

// FUNC 0x0016A088 gt2k_menu::PlayMusicBoxPop::~PlayMusicBoxPop
PlayMusicBoxPop::~PlayMusicBoxPop()
{
    /* only ~PlayMusicBox() (0x001699E0, flags 0), then
     * Object::operator delete if flags & 1 */
    /* implicit: ~PlayMusicBox, operator delete (the base / member destructors,
     * flags & 1) */
}

/* Only stores the focus (no base call, no preview). */
// FUNC 0x0016A0E0 gt2k_menu::PlayMusicBoxPop::virtual_20
void PlayMusicBoxPop::virtual_20(int on)
{
    focus = on;
}

/* With the focus: colour = baseColor x (0.5 .. 1.5) (rgb; alpha as built),
 * interrupts off. */
// FUNC 0x0016A0E8 gt2k_menu::PlayMusicBoxPop::virtual_9
void PlayMusicBoxPop::virtual_9()
{
    int i, intr;
    f32 scale;

    if (focus != 0) {
        blink -= PLAY_BLINK_STEP;           /* gp-0x7DBC 0x0034FC34 */
        if (blink < 0.0f)
            blink = PLAY_BLINK_START;
        scale = (blink + blink) + 0.5f;

        intr = intr_suspend();
        restoreColors(this);
        for (i = 0; i < 4; i++)
            scaleRGB(color[i], scale);
        intr_resume(intr);
    } else {
        blink = 0.0f;                       /* sw $zero */
    }
}

/* ======================================================================== */
/* PlayModeBox                                                              */
/* ======================================================================== */

} /* namespace gt2k_menu */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351A18 = "PlayModeBox";                     /* 0x00351A18 */
gt2k_menu::RawWindowClass g_00351A20 =                      /* 0x00351A20 */
    { "PlayModeBox", gt2k_menu::PlayModeBox::create };
gt2k_menu::RawWindowRegister g_00351A28(&g_00351A20);       /* 0x00351A28 */

namespace gt2k_menu {

// FUNC 0x0016A1F0 gt2k_menu::PlayModeBox::virtual_8
const char *PlayModeBox::virtual_8()
{
    return g_00351A18;                      /* "PlayModeBox" */
}

// FUNC 0x0016A1F8 gt2k_menu::PlayModeBox::factory
RawWindow *PlayModeBox::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_00351A20.create). */
// FUNC 0x0016A238 gt2k_menu::PlayModeBox::create
RawWindow *PlayModeBox::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new PlayModeBox(*args);  /* Object::operator new(0x24C); operator delete if
                                           the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x0016A2B0 gt2k_menu::PlayModeBox::canCreate
int PlayModeBox::canCreate(const RawWindowArgs *args)
{
    return 1;
}

/* A horizontal list, its size locked, of one RawListItem per mode: the
 * node, its focus / hi-light image.  The table is a local copy of the
 * .rodata initialiser 0x00323138 (5 words, NULL-terminated).  EH:
 * ~RawList() if it throws. */
// FUNC 0x0016A2B8 gt2k_menu::PlayModeBox::PlayModeBox
PlayModeBox::PlayModeBox(const RawWindowArgs &args)
    : RawList(args)
{
    const char *modes[5] = {
        "button_straight", "button_straight_focus",     /* 0: straight */
        "button_shuffle",  "button_shuffle_focus",      /* 1: shuffle */
        NULL
    };
    const char **mode;

    focusProperty = "label_check_focus";
    setVertical(0, 1);
    sizeLocked = 1;
    for (mode = modes; mode[0] != NULL; mode += 2) {
        RawListItem *item = static_cast<RawListItem *>(createChild("RawListItem", mode[0], &args,
                                                                   0, 0, 0, 0));
        item->hiLightPropertyName = mode[1];
        item->positionLocked = 1;
        item->focusProperty = mode[1];
        addItem(item, 1);
    }
    blink = 0.0f;                           /* sw $zero */
}

// FUNC 0x0016A3F8 gt2k_menu::PlayModeBox::~PlayModeBox
PlayModeBox::~PlayModeBox()
{
    /* only ~RawList() (0x001D41C8, flags 0), then Object::operator delete
     * if flags & 1 */
    /* implicit: ~RawList, operator delete (the base / member destructors,
     * flags & 1) */
}

/* RawList's (called directly); getting the focus stops the song preview. */
// FUNC 0x0016A450 gt2k_menu::PlayModeBox::virtual_20
void PlayModeBox::virtual_20(int on)
{
    RawList::virtual_20(on);                /* 0x001D4450 */
    if (on)
        bgm_stop();
}

/* RawList's (called directly); then the focused button is also the
 * selected one (its hi-light image). */
// FUNC 0x0016A488 gt2k_menu::PlayModeBox::virtual_29
void PlayModeBox::virtual_29(RawListItem *item)
{
    int i, focusIndex;

    RawList::virtual_29(item);              /* 0x001D4840 */
    focusIndex = getIndex(NULL);
    for (i = 0; i < count; i++) {
        RawListItem *button = getItem(i);
        if (button != NULL)
            button->setSelected(i == focusIndex);
    }
}

/* With the focus: as PlayMusicBox's (1.0 .. 1.5), without the parts. */
// FUNC 0x0016A510 gt2k_menu::PlayModeBox::virtual_9
void PlayModeBox::virtual_9()
{
    int i, intr;
    f32 scale;

    if (focus != 0) {
        blink -= PLAY_BLINK_STEP;           /* gp-0x7DB8 0x0034FC38 */
        if (blink < 0.0f)
            blink = PLAY_BLINK_START;
        scale = blink + 1.0f;

        changeImage(focusProperty, 0, 0);
        intr = intr_suspend();
        for (i = 0; i < 4; i++)
            scaleRGB(color[i], scale);
        intr_resume(intr);
    } else {
        blink = 0.0f;                       /* sw $zero */
    }
}

/* ======================================================================== */
/* PlayList                                                                 */
/* ======================================================================== */

} /* namespace gt2k_menu */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351A30 = "PlayList";                        /* 0x00351A30 */
gt2k_menu::RawWindowClass g_00351A38 =                      /* 0x00351A38 */
    { "PlayList", gt2k_menu::PlayList::create };
gt2k_menu::RawWindowRegister g_00351A40(&g_00351A38);       /* 0x00351A40 */

namespace gt2k_menu {

// FUNC 0x0016A5E8 gt2k_menu::PlayList::virtual_8
const char *PlayList::virtual_8()
{
    return g_00351A30;                      /* "PlayList" */
}

// FUNC 0x0016A5F0 gt2k_menu::PlayList::factory
RawWindow *PlayList::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                             int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_00351A38.create). */
// FUNC 0x0016A630 gt2k_menu::PlayList::create
RawWindow *PlayList::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new PlayList(*args);     /* Object::operator new(0x25C); operator delete if
                                           the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x0016A6A8 gt2k_menu::PlayList::canCreate
int PlayList::canCreate(const RawWindowArgs *args)
{
    return 1;
}

/* The popup (hidden), a row per song, the mode box (the focus on its
 * button 1 when the list is shuffled).  EH: ~RawList() if it throws. */
// FUNC 0x0016A6B0 gt2k_menu::PlayList::PlayList
PlayList::PlayList(const RawWindowArgs &args)
    : RawList(args)
{
    int i;

    owner = static_cast<OptionControl *>(args.owner);
    pageManager = &owner->pages;
    pop = static_cast<PlayMusicBoxPop *>(createChild("PlayMusicBoxPop", "pop_label_check_box",
                                                     &args, 0, 0, 0, 0));
    pop->virtual_23(0);

    MusicPlayList *music = &owner->options.music;
    songCount = music->count;
    for (i = 0; i < songCount; i++) {
        PlayMusicBox *row = static_cast<PlayMusicBox *>(createChild("PlayMusicBox", "label_check_box",
                                                                    &args, 0, 0, 0, 0));
        row->setSong(&music->entries[i]);
        addItem(row, 1);
    }

    modeBox = static_cast<PlayModeBox *>(createChild("PlayModeBox", "mode_select_box", &args,
                                                     0, 0, 0, 0));
    addItem(modeBox, 1);
    modeBox->virtual_31(music->random != 0);
}

// FUNC 0x0016A880 gt2k_menu::PlayList::~PlayList
PlayList::~PlayList()
{
    /* only ~RawList() (0x001D41C8, flags 0), then Object::operator delete
     * if flags & 1 */
    /* implicit: ~RawList, operator delete (the base / member destructors,
     * flags & 1) */
}

/* RawList's (called directly); the popup follows the focus row (its y, at
 * rest). */
// FUNC 0x0016A8D8 gt2k_menu::PlayList::virtual_9
void PlayList::virtual_9()
{
    RawList::virtual_9();                   /* 0x001D4220 */
    RawListItem *row = getItem(-1);
    if (row != NULL)
        pop->y.set(row->y.value);
}

/* The list's own events: cancel with the popup hidden stores the rows into
 * the play list (then goes on to RawList's); up / down with the popup shown
 * swap the focus row with the one before / after it (not past the last
 * song: refused, handled), then RawList's moves the focus with the song.
 * PLAY_EVENT_POP_OPEN (from a row's label): the popup shows the sender
 * row's song and gets the menu control, the list's focus item loses it and
 * the page ignores cancel; PLAY_EVENT_POP_CLOSE undoes that.  The rest
 * goes to RawList::virtual_17 (called directly). */
// FUNC 0x0016A928 gt2k_menu::PlayList::virtual_17
int PlayList::virtual_17(RawWindow *sender, int event, int arg)
{
    if (sender == this) {
        switch (event) {
        case RAW_EVENT_CANCEL:
            if (pop->isShown() == 0)
                save();
            break;
        case RAW_EVENT_UP:
            if (pop->isShown() != 0) {
                RawListItem *prev = focusItem->prevItem;
                if (prev != NULL) {
                    int index = getIndex(focusItem);
                    removeItem(prev, 0);
                    insertItem(prev, index, 1);
                    virtual_28(prev);
                }
            }
            break;
        case RAW_EVENT_DOWN:
            if (pop->isShown() != 0) {
                RawListItem *next = focusItem->nextItem;
                if (next != NULL) {
                    int index = getIndex(focusItem);
                    if (index + 2 >= count) {       /* next is the mode box */
                        menu_play_se(MENU_SE_REFUSED);
                        return 0;
                    }
                    removeItem(next, 0);
                    insertItem(next, index, 1);
                    virtual_28(next);
                }
            }
            break;
        }
    }

    switch (event) {
    case PLAY_EVENT_POP_OPEN:
        pop->setSong(&static_cast<PlayMusicBox *>(sender)->song);
        pop->virtual_14(menuControl);
        pop->virtual_23(1);
        virtual_25(0);
        menu_play_se(MENU_SE_CONFIRM);
        pageManager->noCancel = 1;
        return 0;
    case PLAY_EVENT_POP_CLOSE:
        pop->virtual_14(NULL);
        pop->virtual_23(0);
        virtual_25(1);
        menu_play_se(MENU_SE_LIST_MOVE);
        pageManager->noCancel = 0;
        return 0;
    default:
        return RawList::virtual_17(sender, event, arg); /* 0x001D42B8 */
    }
}

/* RawList's (called directly) only when the focus moves between two song
 * rows (the mode box keeps its own focus). */
// FUNC 0x0016ABD8 gt2k_menu::PlayList::virtual_30
void PlayList::virtual_30(RawList *from, RawList *to)
{
    if (getIndex(from) < songCount && getIndex(to) < songCount)
        RawList::virtual_30(from, to);      /* 0x001D4B30 */
}

/* No callers: the rows and the mode box from the play list again.
 * Interrupts off. */
// FUNC 0x0016AC48 gt2k_menu::PlayList::load
void PlayList::load()
{
    u32 i;
    int intr = intr_suspend();
    MusicPlayList *music = &owner->options.music;

    for (i = 0; i < music->count; i++)
        static_cast<PlayMusicBox *>(getItem(i))->setSong(&music->entries[i]);
    modeBox->virtual_31(music->random != 0);
    intr_resume(intr);
}

/* The rows' songs, in their new order, into the play list; shuffled when
 * the mode box's focus is not on button 0 (MusicPlayList::setRandom makes a
 * new play order).  Interrupts off. */
// FUNC 0x0016AD00 gt2k_menu::PlayList::save
void PlayList::save()
{
    u32 i;
    int intr = intr_suspend();
    MusicPlayList *music = &owner->options.music;

    for (i = 0; i < music->count; i++) {
        MusicEntry *e = &music->entries[i];
        *e = static_cast<PlayMusicBox *>(getItem(i))->song;
    }
    music->setRandom(modeBox->getIndex(NULL) != 0);
    intr_resume(intr);
}

/* ======================================================================== */
/* PlayListBox                                                              */
/* ======================================================================== */

} /* namespace gt2k_menu */

/* The class record (.sdata): the name slot 8 returns, the registry's
 * {name, create} and its registrar (built by the static initialiser). */
const char *g_00351A48 = "PlayListBox";                     /* 0x00351A48 */
gt2k_menu::RawWindowClass g_00351A50 =                      /* 0x00351A50 */
    { "PlayListBox", gt2k_menu::PlayListBox::create };
gt2k_menu::RawWindowRegister g_00351A58(&g_00351A50);       /* 0x00351A58 */

namespace gt2k_menu {

// FUNC 0x0016ADB8 gt2k_menu::PlayListBox::virtual_8
const char *PlayListBox::virtual_8()
{
    return g_00351A48;                      /* "PlayListBox" */
}

/* index.txt: no callers; u022 installs it as the "song_edit" (p0 0) /
 * "song_list" (p0 1) hook of OptionControl. */
// FUNC 0x0016ADC0 gt2k_menu::PlayListBox::factory
RawWindow *PlayListBox::factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                                int p0, int p1, int p2, int p3)
{
    RawWindowArgs args;

    args.owner = (ControlBase *)owner;
    args.loader = loader;
    args.parent = (Object *)parent;
    args.node = node;
    args.p0 = p0;
    args.p1 = p1;
    args.p2 = p2;
    args.p3 = p3;
    return create(&args);
}

/* The registry's create function (g_00351A50.create). */
// FUNC 0x0016AE00 gt2k_menu::PlayListBox::create
RawWindow *PlayListBox::create(const RawWindowArgs *args)
{
    if (canCreate(args))
        return new PlayListBox(*args);  /* Object::operator new(0x1E8); operator delete if
                                           the constructor throws */
        /* implicit: operator new, operator delete (Object's: the new
         * expression and its landing pad) */
    return NULL;
}

// FUNC 0x0016AE78 gt2k_menu::PlayListBox::canCreate
int PlayListBox::canCreate(const RawWindowArgs *args)
{
    return 1;
}

/* Clips its children; the PlayList (on the box's own node) scrolls inside
 * it, the focus row centred, from the box's top left.  p0 == 0
 * ("song_edit"): the box takes the owner's pad 1 as its menu control
 * (PlayListBox::virtual_14 called directly; control1 is the owner's +0).
 * EH: ~RawWindow() if it throws. */
// FUNC 0x0016AE80 gt2k_menu::PlayListBox::PlayListBox
PlayListBox::PlayListBox(const RawWindowArgs &args)
    : RawWindow(args)
{
    int listOnly = args.p0 != 0;

    clipChildren = 1;
    list = static_cast<PlayList *>(createChild("PlayList", NULL, &args, 0, 0, 0, 0));
    list->scrollCenter = 1;
    list->scroll = this;
    list->setLeft(0.0f);
    list->setTop(0.0f);
    if (!listOnly)
        PlayListBox::virtual_14(&args.owner->control1);
}

// FUNC 0x0016AF78 gt2k_menu::PlayListBox::~PlayListBox
PlayListBox::~PlayListBox()
{
    /* only ~RawWindow() (0x001D5F18, flags 0), then Object::operator delete
     * if flags & 1 */
    /* implicit: ~RawWindow, operator delete (the base / member destructors,
     * flags & 1) */
}

/* RawWindow's (called directly), then the list's (a virtual call). */
// FUNC 0x0016AFD0 gt2k_menu::PlayListBox::virtual_14
void PlayListBox::virtual_14(MenuControl *ctl)
{
    RawWindow::virtual_14(ctl);             /* 0x001D6678 */
    list->virtual_14(ctl);
}

} /* namespace gt2k_menu */

using namespace gt2k_menu;

/* ======================================================================== */
/* Static destructor / constructor, inline functions, RTTI                  */
/* ======================================================================== */

/* Compiler-generated: egcs's static destructor and initialiser of the five
 * registrars above (named after the unit's first global), emitted here,
 * after the last out-of-line function. */
// FUNC 0x0016B020 T085_static_dtor (_GLOBAL_$D$g_003519E0)
//   (.dtors entry 0x0028F1F0) ~RawWindowRegister (flags 2) of the
//   registrars, in reverse order.
/* implicit: ~RawWindowRegister */
// FUNC 0x0016B070 T085_static_init (_GLOBAL_$I$g_003519E0)
//   (.ctors entry 0x0028F0B0) RawWindowRegister(&class) of the five.
/* implicit: RawWindowRegister */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function. */
// FUNC 0x0016B0D8 __tfQ29gt2k_menu11PlayListBox
extern "C" void *__tfQ29gt2k_menu11PlayListBox(void)
{
    if (__tiQ29gt2k_menu11PlayListBox[0] == 0) {
        __tfQ29gt2k_menu9RawWindow();
        __rtti_si(__tiQ29gt2k_menu11PlayListBox, "Q29gt2k_menu11PlayListBox",
                  (const type_info *)__tiQ29gt2k_menu9RawWindow);
    }
    return __tiQ29gt2k_menu11PlayListBox;
}
#endif

// FUNC 0x0016B128 gt2k_menu::PlayList::isPopupActive
/* inline in game/menu/options/T085_play_list.h; egcs emits this copy (no callers) with
 * PlayList's typeinfo function: pop->isShown() */
/* implicit: isShown (the header's inline body) */

#ifndef BOOT_OLD_GCC
// FUNC 0x0016B148 __tfQ29gt2k_menu8PlayList
extern "C" void *__tfQ29gt2k_menu8PlayList(void)
{
    if (__tiQ29gt2k_menu8PlayList[0] == 0) {
        __tfQ29gt2k_menu7RawList();
        __rtti_si(__tiQ29gt2k_menu8PlayList, "Q29gt2k_menu8PlayList",
                  (const type_info *)__tiQ29gt2k_menu7RawList);
    }
    return __tiQ29gt2k_menu8PlayList;
}

// FUNC 0x0016B198 __tfQ29gt2k_menu11PlayModeBox
extern "C" void *__tfQ29gt2k_menu11PlayModeBox(void)
{
    if (__tiQ29gt2k_menu11PlayModeBox[0] == 0) {
        __tfQ29gt2k_menu7RawList();
        __rtti_si(__tiQ29gt2k_menu11PlayModeBox, "Q29gt2k_menu11PlayModeBox",
                  (const type_info *)__tiQ29gt2k_menu7RawList);
    }
    return __tiQ29gt2k_menu11PlayModeBox;
}

// FUNC 0x0016B1E8 __tfQ29gt2k_menu15PlayMusicBoxPop
extern "C" void *__tfQ29gt2k_menu15PlayMusicBoxPop(void)
{
    if (__tiQ29gt2k_menu15PlayMusicBoxPop[0] == 0) {
        __tfQ29gt2k_menu12PlayMusicBox();
        __rtti_si(__tiQ29gt2k_menu15PlayMusicBoxPop, "Q29gt2k_menu15PlayMusicBoxPop",
                  (const type_info *)__tiQ29gt2k_menu12PlayMusicBox);
    }
    return __tiQ29gt2k_menu15PlayMusicBoxPop;
}
#endif

// FUNC 0x0016B238 gt2k_menu::PlayMusicBox::getSong
/* inline in game/menu/options/T085_play_list.h; egcs emits this copy (no callers) with
 * PlayMusicBox's typeinfo function: &song */

#ifndef BOOT_OLD_GCC
// FUNC 0x0016B240 __tfQ29gt2k_menu12PlayMusicBox
extern "C" void *__tfQ29gt2k_menu12PlayMusicBox(void)
{
    if (__tiQ29gt2k_menu12PlayMusicBox[0] == 0) {
        __tfQ29gt2k_menu7RawList();
        __rtti_si(__tiQ29gt2k_menu12PlayMusicBox, "Q29gt2k_menu12PlayMusicBox",
                  (const type_info *)__tiQ29gt2k_menu7RawList);
    }
    return __tiQ29gt2k_menu12PlayMusicBox;
}
#endif
