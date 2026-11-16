/* T085_play_list.h - the music play list windows of the options screen
 * (translation unit T085, unit u037), 0x00169780..0x0016B290.
 *
 * gt2k_menu::PlayListBox, the window of OptionControl's "song_edit" (p0 0:
 * it takes the controller's pad) and "song_list" (p0 1) hooks, holds a
 * PlayList: one PlayMusicBox row per song of the controller's MusicPlayList
 * (OptionControl +0x1080, GameOptions +0x228) and a PlayModeBox (straight /
 * shuffle).  A row is a RawCheckBox (the song on / off; checking it plays
 * the song) and a RawLabel "title / artist"; deciding the label opens the
 * PlayMusicBoxPop (event 8), with which the song is moved up / down the
 * list, and event 9 closes it; event 2 (cancel) stores the rows back into the
 * play list.
 *
 * Each class has the RawWindow boilerplate of its own (in T085, unlike the
 * Raw* classes whose boilerplate is T272's): slot 8 (the class name, a
 * .sdata pointer), the layout factory, create (registered in T265's registry
 * by egcs's static initialiser of T085's registrars), canCreate (it gets the
 * arguments: create's call passes them in $a0) and the constructor.  The slot names stay
 * virtual_N (T264_raw_window.h has what each slot does); the signatures are
 * RawWindow's / RawList's.
 *
 * RawWindowArgs and RawWindowClass, planned here, are defined in T264 /
 * T265. */
#ifndef GAME_T085_PLAY_LIST_H
#define GAME_T085_PLAY_LIST_H

#include "game/game.h"
#include "game/menu/framework/T263_raw_list.h"             /* bases gt2k_menu::RawList, RawWindow */
#include "game/menu/framework/T265_raw_window_register.h"  /* gt2k_menu::RawWindowClass (the .sdata records) */
#include "game/save/T430_music_play_list.h"      /* MusicEntry (PlayMusicBox::song) */

namespace gt2k_menu {
class ControlBase;                          /* T247 (pointers only) */
class OptionControl;                        /* T062 */
class Object;                               /* T220 */
class PageManager;                          /* T211 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */
class MenuControl;                          /* T214 */
class RawCheckBox;                          /* T275 */
class RawLabel;                             /* T273 */
class RawWindowRegister;                    /* T265 (extern objects only) */
class PlayMusicBoxPop;
class PlayModeBox;

/* vtable 0x003235B0 (33 slots: [0], [8], [9], [17], [20] its own, [21]
 * RawListItem's, the rest RawList's / RawWindow's).  typeinfo
 * __tfQ29gt2k_menu12PlayMusicBox 0x0016B240 (si, base RawList;
 * __si_type_info 0x004B1C30).  Size 0x2E8 (operator new in create), 8-aligned
 * (MusicEntry's u64).  One song of the list: a RawCheckBox (node p0, NULL:
 * "check_box") and a RawLabel (node p1, NULL: "label_box") as items of a
 * horizontal list; focusProperty "label_check_focus". */
class PlayMusicBox : public RawList {
public:
    PlayMusicBox(const RawWindowArgs &args);    /* 0x00169848 the play list's first song */
    virtual ~PlayMusicBox();                /* [0] +0x0C 0x001699E0 only ~RawList */
    virtual const char *virtual_8();        /* [8] +0x4C 0x00169780 the class name "PlayMusicBox" */
    virtual void virtual_9();               /* [9] +0x54 0x00169B50 with the focus: the colours of the row
                                               and of its focused part pulse (1.0 .. 1.5, blink) */
    virtual int  virtual_17(RawWindow *sender, int event, int arg);   /* [17] +0x94 0x00169A38 inside a
                                               PlayMusicBoxPop: 1 closes it (event 9), 5 / 6 swallowed;
                                               else 1 from the label opens it (event 8), 7 from the check
                                               box turns the song on / off; the rest RawList's */
    virtual void virtual_20(int focus);     /* [20] +0xAC 0x00169D98 RawList's; unless the popup is shown,
                                               plays the song if it is enabled (else stops the music) */

    /* The layout factory (no callers): the eight words as a RawWindowArgs, then create. */
    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x00169788 */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x001697C8 the registry's
                                               (g_003519E8): new PlayMusicBox(*args) if canCreate() */
    static int        canCreate(const RawWindowArgs *args);  /* 0x00169840 1 (args unused) */
    void setEnabled(int enabled);           /* 0x00169D18 song.enabled (a byte), then preview(song.enabled
                                               != 0): an int argument, tested through the stored byte */
    void preview(int on);                   /* 0x00169D48 on: track (findMusicInfo's index) | 0x1000 of BGM
                                               group 0x14 (T464 bgm_play_if_changed, volume 1.0); off:
                                               bgm_stop */
    void setSong(const MusicEntry *entry);  /* 0x00169E20 the check box and "<title> / <artist>" */
    void setCheck(int checked);             /* 0x00169E98 checkBox->virtual_24 */
    void setLabel(const char *text);        /* 0x00169ED0 label's text (Item::setText) */
    /* Inline (no callers): egcs writes its copy at the end of T085 with
     * the class's typeinfo function (retail's frame info has it there). */
    MusicEntry *getSong() { return &song; } /* 0x0016B238 */

    char           text[0x80];              /* 0x248 the label's text */
    MusicEntry     song;                    /* 0x2C8 */
    f32            blink;                   /* 0x2D8 0.5 .. 0, -1/60 a frame (with the focus) */
    OptionControl *owner;                   /* 0x2DC args.owner */
    RawCheckBox   *checkBox;                /* 0x2E0 */
    RawLabel      *label;                   /* 0x2E4 */
};
EGCS_SIZE(gt2k_menu::PlayMusicBox, 0x2E8);

/* vtable 0x003234A0 (33 slots: [0], [8], [9], [20] its own, [17]
 * PlayMusicBox's).  typeinfo __tfQ29gt2k_menu15PlayMusicBoxPop 0x0016B1E8
 * (si, base PlayMusicBox; __si_type_info 0x004B1C00).  No
 * data of its own: size 0x2E8 (operator new in create).  The row shown
 * while a song is moved: built with p0 / p1 = "pop_check_box" /
 * "pop_label_box"; noParentClip; focus on the label, no focus control. */
class PlayMusicBoxPop : public PlayMusicBox {
public:
    PlayMusicBoxPop(const RawWindowArgs &args); /* 0x00169FC0 RawList's virtual_31(1) and virtual_25(0)
                                               called directly */
    virtual ~PlayMusicBoxPop();             /* [0] +0x0C 0x0016A088 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x00169EF8 "PlayMusicBoxPop" */
    virtual void virtual_9();               /* [9] +0x54 0x0016A0E8 with the focus: colour = baseColor x
                                               (0.5 .. 1.5) */
    virtual void virtual_20(int focus);     /* [20] +0xAC 0x0016A0E0 only stores the focus */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x00169F00 */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x00169F40 (g_00351A08) */
    static int        canCreate(const RawWindowArgs *args);  /* 0x00169FB8 1 (args unused) */
};
EGCS_SIZE(gt2k_menu::PlayMusicBoxPop, 0x2E8);

/* vtable 0x00323390 (33 slots: [0], [8], [9], [20], [29] its own).
 * typeinfo __tfQ29gt2k_menu11PlayModeBox 0x0016B198 (si, base RawList;
 * __si_type_info 0x004B1C20).  Size 0x24C (operator new in create).  The
 * buttons "button_straight" / "button_shuffle" (RawListItems, focus and
 * hi-light images "<name>_focus") of the play order; the focused one is the
 * selected one. */
class PlayModeBox : public RawList {
public:
    PlayModeBox(const RawWindowArgs &args); /* 0x0016A2B8 */
    virtual ~PlayModeBox();                 /* [0] +0x0C 0x0016A3F8 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x0016A1F0 "PlayModeBox" */
    virtual void virtual_9();               /* [9] +0x54 0x0016A510 with the focus: the colours pulse
                                               (1.0 .. 1.5) */
    virtual void virtual_20(int focus);     /* [20] +0xAC 0x0016A450 RawList's; the focus stops the music */
    virtual void virtual_29(RawListItem *item); /* [29] +0xF4 0x0016A488 RawList's; then the focus item is
                                               the selected one (RawListItem::setSelected) */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x0016A1F8 */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x0016A238 (g_00351A20) */
    static int        canCreate(const RawWindowArgs *args);  /* 0x0016A2B0 1 (args unused) */

    f32 blink;                              /* 0x248 */
};
EGCS_SIZE(gt2k_menu::PlayModeBox, 0x24C);

/* vtable 0x00323280 (33 slots: [0], [8], [9], [17], [30] its own).
 * typeinfo __tfQ29gt2k_menu8PlayList 0x0016B148 (si, base RawList;
 * __si_type_info 0x004B1C10).  Size 0x25C (operator new in create).  The
 * PlayMusicBoxPop "pop_label_check_box" (hidden), one PlayMusicBox
 * "label_check_box" per song, then the PlayModeBox "mode_select_box" (focus
 * on button 1 when the list is shuffled). */
class PlayList : public RawList {
public:
    PlayList(const RawWindowArgs &args);    /* 0x0016A6B0 */
    virtual ~PlayList();                    /* [0] +0x0C 0x0016A880 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x0016A5E8 "PlayList" */
    virtual void virtual_9();               /* [9] +0x54 0x0016A8D8 RawList's; the popup follows the focus
                                               row (its y) */
    virtual int  virtual_17(RawWindow *sender, int event, int arg);   /* [17] +0x94 0x0016A928 its own 2:
                                               save() (popup hidden); 3 / 4 with the popup: the focused
                                               song swaps with the previous / next one (not past the last
                                               song: sound 4); 8: the popup takes the sender row's song and
                                               the menu control, the page ignores cancel (noCancel); 9:
                                               undoes it; the rest RawList's */
    virtual void virtual_30(RawList *from, RawList *to);   /* [30] +0xFC 0x0016ABD8 RawList's only between
                                               two song rows */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x0016A5F0 */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x0016A630 (g_00351A38) */
    static int        canCreate(const RawWindowArgs *args);  /* 0x0016A6A8 1 (args unused) */
    void load();                            /* 0x0016AC48 the rows and the mode box from the play list
                                               again (no callers; interrupts off) */
    void save();                            /* 0x0016AD00 the rows' songs, in their order, and the mode
                                               into the play list (MusicPlayList::setRandom; interrupts off) */
    /* Inline (no callers): egcs writes its copy at the end of T085 with
     * the class's typeinfo function (retail's frame info has it there). */
    int  isPopupActive() { return pop->isShown(); }    /* 0x0016B128 */

    OptionControl   *owner;                 /* 0x248 args.owner */
    PlayMusicBoxPop *pop;                   /* 0x24C */
    PlayModeBox     *modeBox;               /* 0x250 */
    PageManager     *pageManager;           /* 0x254 &owner->pages (noCancel while the popup is shown) */
    int              songCount;             /* 0x258 the play list's count when built */
};
EGCS_SIZE(gt2k_menu::PlayList, 0x25C);

/* vtable 0x003231B8 (24 slots: [0], [8], [14] its own).  typeinfo
 * __tfQ29gt2k_menu11PlayListBox 0x0016B0D8 (si, base RawWindow;
 * __si_type_info 0x004B1BF0).  Size 0x1E8 (operator new in create).  Clips
 * its children; the PlayList scrolls inside it (list->scroll). */
class PlayListBox : public RawWindow {
public:
    PlayListBox(const RawWindowArgs &args); /* 0x0016AE80 p0 == 0: PlayListBox::virtual_14(the owner's
                                               control1) called directly */
    virtual ~PlayListBox();                 /* [0] +0x0C 0x0016AF78 */
    virtual const char *virtual_8();        /* [8] +0x4C 0x0016ADB8 "PlayListBox" */
    virtual void virtual_14(MenuControl *ctl);  /* [14] +0x7C 0x0016AFD0 RawWindow's, then the list's */

    static RawWindow *factory(void *owner, PageLayout *loader, void *parent, MenuLayoutItem *node,
                              int p0, int p1, int p2, int p3);  /* 0x0016ADC0 */
    static RawWindow *create(const RawWindowArgs *args);       /* 0x0016AE00 (g_00351A50) */
    static int        canCreate(const RawWindowArgs *args);  /* 0x0016AE78 1 (args unused) */

    PlayList *list;                         /* 0x1E4 */
};
EGCS_SIZE(gt2k_menu::PlayListBox, 0x1E8);

} /* namespace gt2k_menu */

/* The class records (.sdata, defined in T085 before each class's functions):
 * per class the name pointer slot 8 returns, the registry record {name,
 * create} and its registrar (T265; a public object with a constructor:
 * .sdata, zero-filled). */
extern const char *g_003519E0;                  /* 0x003519E0 gp-0x6010 "PlayMusicBox" */
extern gt2k_menu::RawWindowClass    g_003519E8; /* 0x003519E8 gp-0x6008 {name, PlayMusicBox::create} */
extern gt2k_menu::RawWindowRegister g_003519F0; /* 0x003519F0 gp-0x6000 its registrar */
extern const char *g_00351A00;                  /* 0x00351A00 gp-0x5FF0 "PlayMusicBoxPop" */
extern gt2k_menu::RawWindowClass    g_00351A08; /* 0x00351A08 gp-0x5FE8 */
extern gt2k_menu::RawWindowRegister g_00351A10; /* 0x00351A10 gp-0x5FE0 */
extern const char *g_00351A18;                  /* 0x00351A18 gp-0x5FD8 "PlayModeBox" */
extern gt2k_menu::RawWindowClass    g_00351A20; /* 0x00351A20 gp-0x5FD0 */
extern gt2k_menu::RawWindowRegister g_00351A28; /* 0x00351A28 gp-0x5FC8 */
extern const char *g_00351A30;                  /* 0x00351A30 gp-0x5FC0 "PlayList" */
extern gt2k_menu::RawWindowClass    g_00351A38; /* 0x00351A38 gp-0x5FB8 */
extern gt2k_menu::RawWindowRegister g_00351A40; /* 0x00351A40 gp-0x5FB0 */
extern const char *g_00351A48;                  /* 0x00351A48 gp-0x5FA8 "PlayListBox" */
extern gt2k_menu::RawWindowClass    g_00351A50; /* 0x00351A50 gp-0x5FA0 */
extern gt2k_menu::RawWindowRegister g_00351A58; /* 0x00351A58 gp-0x5F98 */

/* 0x0016B020 (.dtors 0x0028F1F0) / 0x0016B070 (.ctors 0x0028F0B0): egcs's
 * static destructor / initialiser of the five registrars
 * (_GLOBAL_$D$g_003519E0 / _GLOBAL_$I$g_003519E0, T085). */

#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions and nodes itself (and rejects hand
 * declarations of them). */
extern "C" {
void *__tfQ29gt2k_menu11PlayListBox(void);      /* 0x0016B0D8 */
void *__tfQ29gt2k_menu8PlayList(void);          /* 0x0016B148 */
void *__tfQ29gt2k_menu11PlayModeBox(void);      /* 0x0016B198 */
void *__tfQ29gt2k_menu15PlayMusicBoxPop(void);  /* 0x0016B1E8 */
void *__tfQ29gt2k_menu12PlayMusicBox(void);     /* 0x0016B240 */
/* The __si_type_info nodes (3 words) of the five classes (.bss) ... */
extern u32 __tiQ29gt2k_menu11PlayListBox[3];    /* 0x004B1BF0 */
extern u32 __tiQ29gt2k_menu15PlayMusicBoxPop[3];/* 0x004B1C00 */
extern u32 __tiQ29gt2k_menu8PlayList[3];        /* 0x004B1C10 */
extern u32 __tiQ29gt2k_menu11PlayModeBox[3];    /* 0x004B1C20 */
extern u32 __tiQ29gt2k_menu12PlayMusicBox[3];   /* 0x004B1C30 */
}
#endif

#endif /* GAME_T085_PLAY_LIST_H */
