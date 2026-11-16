/* play_list_internal.h - unit u037 of the GT3 A-Spec US main executable (SCUS-97102),
 * 0x00169780..0x0016C260, second pass.
 *
 * Translation units (link order):
 *   T085  the music play list of the options screen (OptionControl's
 *         "song_edit" / "song_list" hooks, u022): PlayMusicBox (one song:
 *         check box + "title / artist" label), PlayMusicBoxPop (the row a
 *         song is moved with), PlayModeBox (straight / shuffle), PlayList
 *         (the rows plus the mode box) and PlayListBox (the window holding
 *         it) (include/game/menu/options/T085_play_list.h)
 *   T086  the top page of the race logger (T055): LoggerTop, LoggerTrackInfo
 *         (the texts of one recorded car) and LoggerProgressBar
 *         (T086_logger_top.h)
 *
 * Every class and function is declared in its canonical header
 * (include/game/); this header adds what only this unit uses: the event,
 * sound, BGM and logger constants, the colour helpers (scaleRGB,
 * restoreColors), RawWindowArgsInit, EntryDate and the typeinfo storage of
 * T086's classes.  Matching pass: ref/matching/u037/notes.md.
 *
 * Data pass (ref/data/u037/notes.md): each source includes the canonical
 * headers it needs itself, in the order that gives the original's type
 * names, then this header last.  Its own includes make a file see no class
 * the original files did not (T264's operator classes: both have them).
 */
#ifndef U037_H
#define U037_H

#include "types.h"
#include <stddef.h>
#include "game/menu/framework/T264_raw_window.h"           /* gt2k_menu::RawWindowArgs, RawWindow, Color4f (the helpers) */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif

/* ======================================================================== */
/* Menu sounds (menu_play_se, T233)                                         */
/* ======================================================================== */
/* T233's MENU_SE_*: CANCEL (LoggerTop: cancel / Exit), REFUSED, CONFIRM (a song
 * checked, the popup opened, a logger choice made), LIST_MOVE (the popup
 * closed: the moved song is put down). */

/* ======================================================================== */
/* T085 - the music play list                                               */
/* ======================================================================== */

/* The events of the play list after T264's 1..6 and T275's 7: a row's
 * label sends 8 when decided, the popup row 9 (both to the PlayList). */
enum {
    PLAY_EVENT_POP_OPEN  = 8,               /* the song of the sending row is picked up */
    PLAY_EVENT_POP_CLOSE = 9                /* put down */
};

/* A RawWindowArgs built by a constructor: PlayMusicBoxPop's constructor
 * hands its base a temporary built from the eight words (retail: field
 * stores straight into the stack object, then &object to the base
 * constructor).  A compound literal or an inline function returning the
 * struct makes egcs clear / copy a second temporary.  (? the original
 * RawWindowArgs had this constructor itself; T264's is an aggregate, which
 * other units initialise with braces.) */
struct RawWindowArgsInit : gt2k_menu::RawWindowArgs {
    RawWindowArgsInit(gt2k_menu::ControlBase *owner_, gt2k_menu::PageLayout *loader_,
                      gt2k_menu::Object *parent_, gt2k_menu::MenuLayoutItem *node_,
                      int p0_, int p1_, int p2_, int p3_)
    {
        owner = owner_;
        loader = loader_;
        parent = parent_;
        node = node_;
        p0 = p0_;
        p1 = p1_;
        p2 = p2_;
        p3 = p3_;
    }
};

/* The songs of the play list are the tracks of BGM group 0x14 (T430: ADS
 * group 2, T464); a preview starts with the intro (track bit 0x1000). */
enum {
    BGM_GROUP_PLAY_LIST = 0x14,
    BGM_TRACK_INTRO     = 0x1000
};

/* The focus pulse of the rows and the mode box: blink falls by 1/60 a frame
 * from 0.5 to 0 (gp-0x7DC0 / -0x7DBC / -0x7DB8, three .sdata copies of
 * 0x3C888889). */
#define PLAY_BLINK_STEP   (1.0f / 60.0f)
#define PLAY_BLINK_START  0.5f

/* Multiplies r, g, b (not a) of one colour by s; the update functions call it
 * for the four corners in a loop.  (? an inline member of Color4f in the
 * original.)  Per element, as retail: the corner's address is an index sum
 * until loop.c reduces it, so GCSE neither shares &color with an earlier
 * copy loop (PlayMusicBoxPop::virtual_9 recomputes it) nor stops the next
 * part's pointer load from moving above the loop (PlayMusicBox::virtual_9);
 * a loop over a Color4f pointer does both. */
static inline void scaleRGB(Color4f &c, f32 s)
{
    c.r *= s;
    c.g *= s;
    c.b *= s;
}

/* color = baseColor: the corner colours as built (RawWindow +0x120) back
 * into the drawn ones (Drawable +0x40).  Retail's copy is egcs's array
 * assignment loop (build_vec_init: a pointer pair, a count-down from 3 that
 * ends at -1, Color4f::operator= per element, as RawWindow::virtual_21
 * 0x001D68E8), so the original assigned the arrays (baseColor a Color4f[4]
 * there); GCC 14 does not assign arrays: the same copy element by element. */
static inline void restoreColors(gt2k_menu::RawWindow *w)
{
#ifdef BOOT_OLD_GCC
    typedef Color4f Colors[4];
    *(Colors *)w->color = *(Colors *)w->baseColor;
#else
    int i;
    for (i = 0; i < 4; i++)
        w->color[i] = *(Color4f *)w->baseColor[i];
#endif
}

/* ======================================================================== */
/* T086 - the logger's top page                                             */
/* ======================================================================== */

/* RaceBase::virtual_26's argument (RaceEntryInformation::inputId, T121):
 * the logged car is the player's (0x100 + player 0), the second one 1. */
enum {
    INPUT_ID_OTHER  = 1,
    INPUT_ID_PLAYER = 0x100
};

/* RaceEntryCar::date (T121's RaceDate, u64 bit fields) as setInfo reads it:
 * plain bytes and a halfword (retail: lbu +4, lbu +5, lhu +6 through
 * &entry->date, no 64-bit extraction), as gt2k_menu::McDateTime (T108) has
 * the same layout.  (? the original's date type) */
struct EntryDate {
    u8  zero;                               /* 0x0 */
    u8  second;                             /* 0x1 */
    u8  minute;                             /* 0x2 */
    u8  hour;                               /* 0x3 */
    u8  day;                                /* 0x4 */
    u8  month;                              /* 0x5 */
    u16 year;                               /* 0x6 */
};

/* The blink of LoggerTop and LoggerProgressBar: a counter of 30 frames, the
 * blinking texts shown during the first 21. */
enum {
    LOGGER_BLINK_FRAMES = 30,
    LOGGER_BLINK_ON     = 21
};

/* ======================================================================== */
/* RTTI: the typeinfo storage of T086's classes                             */
/* ======================================================================== */
/* egcs generates the __tf functions and their nodes itself; the C
 * equivalents in this unit and these declarations are for the modern
 * compiler only.  __si_type_info nodes (3 words, .bss), only used here
 * (T085's are declared in T085_play_list.h). */
#ifndef BOOT_OLD_GCC
extern "C" {
extern u32 __tiQ29gt2k_menu15LoggerTrackInfo[3];    /* 0x004B1C40 */
extern u32 __tiQ29gt2k_menu9LoggerTop[3];           /* 0x004B1C50 */
extern u32 __tiQ29gt2k_menu17LoggerProgressBar[3];  /* 0x004B1C60 */
}
#endif /* !BOOT_OLD_GCC */

#endif /* U037_H */
