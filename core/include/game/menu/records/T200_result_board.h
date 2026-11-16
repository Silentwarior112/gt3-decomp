/* T200_result_board.h - the race result board (translation unit T200, unit
 * u062), 0x001B0F28..0x001B27C8.
 *
 * ResultBoard (? name, u062; users u040 / u041 / u046 / u047 / u050: the
 * ResultArcade family and the race modes) is the page "result" of the
 * layout file "result1": a title bar ("result_title": result / point / total point),
 * seven rank bars ("rank_bar": rank, car name, time or gap / laps / points,
 * car icon) and a closing bar ("term_bar").  setup() gives every bar a start
 * time (the title at t, the rows 0.12 s apart from t + 1, the closing bar
 * last); from then each bar starts its opening script and slides to just
 * under the bar above it (ResultBar::animate: 19 % of the distance a
 * frame), drawn clipped below that bar.  The player's row is drawn in
 * "color_mine", the others in "color_others".  CarIconItem draws a car's
 * 64x32 icon (CarIcon, T184) in a rank bar.
 *
 * The bars' constructors, ResultBar's two setters and CarIconItem::
 * setDisplay are inline: ResultBoard's constructor expands them, and their
 * out-of-line copies sit at the end of T200 with the typeinfo functions
 * (0x001B23E8..0x001B26B0).  The bars and CarIconItem have no destructor
 * of their own: slot 0 is egcs's implicit one (0x001B24B0 .. 0x001B2708
 * store no vptr of their class, only the inline ~RaceMenuWindow's; ~Item is
 * called without one).  The slots
 * 1..4 are ResultBar's (the dossier's index names ResultRankBar::virtual_1 /
 * _2, ResultTermBar::virtual_3 / _4 are their addresses' first users). */
#ifndef GAME_T200_RESULT_BOARD_H
#define GAME_T200_RESULT_BOARD_H

#include "game/game.h"
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* base RaceMenuWindow */
#include "game/menu/race_menu/T197_race_menu_page.h"       /* RaceMenuPage, RaceMenuColor (held by value) */
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */

struct CarIcon;                             /* T184_car_icon_maker.h (pointers only) */
class GSBuffer;                             /* T514_gs_buffer.h */
class type_info;                            /* T548 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
/* The __si_type_infos at 0x004B2400 (base __tiQ29gt2k_menu4Item), 0x004B2420
 * (base __ti14RaceMenuWindow), 0x004B1290 / 0x004B12F0 / 0x004B2410 (base
 * __ti9ResultBar). */
const type_info &__tf11CarIconItem(void);   /* 0x001B26B8 */
const type_info &__tf9ResultBar(void);      /* 0x001B2660 */
const type_info &__tf14ResultTitleBar(void);/* 0x001B2550 */
const type_info &__tf13ResultTermBar(void); /* 0x001B24D8 */
const type_info &__tf13ResultRankBar(void); /* 0x001B2460 */
}
#endif

/* vtable 0x00331FF8 (8 slots: [1] Item's, [3] Object's, [4] Item's, [5]
 * Image's, [6] / [7] Transitive's), typeinfo __tf11CarIconItem 0x001B26B8
 * (si, base gt2k_menu::Item).  Size 0x104. */
class CarIconItem : public gt2k_menu::Item {
public:
    CarIconItem(const char *name, gt2k_menu::PageLayout *layout);  /* 0x001B0F28 display = NULL */
    /*  [0] +0x0C 0x001B2708 the implicit destructor (~Item) */
    /* The display's icon (when set) as a textured quad of the item's size
     * at its screen position, white with its alpha. */
    virtual void virtual_2();               /* [2] +0x1C 0x001B0F60 */
    /* No callers (out-of-line copy 0x001B26B0). */
    void setDisplay(CarIcon *d) { display = d; }

    CarIcon *display;                       /* 0x100 (u062 CarDisplay *: the icon of a car) */
};
EGCS_SIZE(CarIconItem, 0x104);

/* vtable 0x00331FC8 (5 slots), typeinfo __tf9ResultBar 0x001B2660 (si, base
 * RaceMenuWindow).  vptr +0x2C (RaceMenuLayer's).  Size 0x3C.  A bar of the
 * board: it slides in after startTime, under the previous bar. */
class ResultBar : public RaceMenuWindow {
public:
    /* Out-of-line copy 0x001B2600. */
    ResultBar() : prev(0), startTime(0.0f) {}
    /* Empty, declared here: egcs emits its copy between setStartTime's
     * and the constructor's (an implicit one comes after __tf9ResultBar). */
    ~ResultBar() {}                         /* [0] +0x0C 0x001B25D8 */
    /* animate() once t has passed startTime. */
    virtual void update(f32 t);             /* [1] +0x14 0x001B10E8 */
    /* render() once t has passed startTime. */
    virtual void draw(f32 t);               /* [2] +0x1C 0x001B1128 */
    /* A frame of the layer; the bar moves 19 % of the way to its place
     * under the previous bar. */
    virtual void animate();                 /* [3] +0x24 0x001B1168 */
    /* The layer, clipped below the middle of the previous bar. */
    virtual void render();                  /* [4] +0x2C 0x001B1220 */
    /* The closing script; t is not used. */
    void close(f32 t);                      /* 0x001B12D0 */
    /* No callers (out-of-line copies 0x001B25C8 / 0x001B25D0, emitted in
     * reverse declaration order). */
    void setStartTime(f32 t) { startTime = t; }
    void setPrev(ResultBar *p) { prev = p; }

    ResultBar *prev;                        /* 0x34 the bar above (NULL: none) */
    f32        startTime;                   /* 0x38 */
};
EGCS_SIZE(ResultBar, 0x3C);

/* vtable 0x00331F98 (5 slots: [1..4] ResultBar's), typeinfo
 * __tf14ResultTitleBar 0x001B2550 (si).  Size 0x48.  "result_title". */
class ResultTitleBar : public ResultBar {
public:
    ResultTitleBar() {}                     /* the items are left unset */
    /*  [0] +0x0C 0x001B25A0 the implicit destructor */
    /* The layer and its three titles, hidden; the frame: titleResult. */
    void init(gt2k_menu::PageLayout *layout);   /* 0x001B12F0 */
    /* The title of mode 0 / 1 / 2 shown (and made the frame), then the
     * opening script. */
    void start(int mode);                   /* 0x001B13A0 */

    gt2k_menu::Item *titleResult;           /* 0x3C "b_title_result" (mode 0) */
    gt2k_menu::Item *titlePoint;            /* 0x40 "b_title_point" (mode 1) */
    gt2k_menu::Item *titleTotal;            /* 0x44 "b_title_total_point" (mode 2) */
};
EGCS_SIZE(ResultTitleBar, 0x48);

/* vtable 0x00331F68 (5 slots: [1..4] ResultBar's), typeinfo
 * __tf13ResultTermBar 0x001B24D8 (si).  Size 0x3C.  "term_bar": the closing
 * bar under the last row. */
class ResultTermBar : public ResultBar {
public:
    ResultTermBar() {}
    /*  [0] +0x0C 0x001B2528 the implicit destructor */
    void init(gt2k_menu::PageLayout *layout);   /* 0x001B1460 the layer "term_bar", frame "b_term" */
    void start();                           /* 0x001B1488 off screen (y -1000), the opening script */
};
EGCS_SIZE(ResultTermBar, 0x3C);

/* vtable 0x00331F38 (5 slots: [1] / [2] ResultBar's), typeinfo
 * __tf13ResultRankBar 0x001B2460 (si).  Size 0xF0.  "rank_bar": one row. */
class ResultRankBar : public ResultBar {
public:
    /* Out-of-line copy 0x001B23E8. */
    ResultRankBar() : rank(0), carName(0), timeItem(0), icon(0)
    {
        rankText[0] = 0;
        timeText[0] = 0;
    }
    /*  [0] +0x0C 0x001B24B0 the implicit destructor */
    virtual void animate();                 /* [3] +0x24 0x001B16C0 */
    virtual void render();                  /* [4] +0x2C 0x001B16A0 */
    /* The layer "rank_bar" (frame "b_bar"), its texts and car icon; the
     * rank text is index + 1. */
    void init(gt2k_menu::PageLayout *layout, int index);   /* 0x001B14D0 */
    /* Off screen (y -1000), the opening script, no name, "Running...". */
    void start();                           /* 0x001B15F8 */
    void clearTexts();                      /* 0x001B1658 empty name and time texts */
    /* The car name: name, or "name suffix"; nothing when name is NULL. */
    void setName(const char *name, const char *suffix);    /* 0x001B16E0 */
    /* The time text: a time (ms) after '+' (plus) or ' '. */
    void setTimeGap(int time, int plus);    /* 0x001B1750 */
    /* "+N" / " N" and "lap" / "laps" (RACEMENUTEXT, T201). */
    void setLaps(int laps, int plus);       /* 0x001B17A0 */
    /* "+N" / " N" and "pt" / "pts". */
    void setPoints(int points, int plus);   /* 0x001B1830 */
    void setSpeed(f32 speed);               /* 0x001B18C0 */
    void setRunning();                      /* 0x001B1900 "Running..." */
    void clearTime();                       /* 0x001B1928 (no callers) */
    void setTextColor(const RaceMenuColor *c);  /* 0x001B1950 the name and time texts */
    void setCarDisplay(CarIcon *d);         /* 0x001B1998 icon->display = d */

    gt2k_menu::Item *rank;                  /* 0x3C "t_rank" */
    gt2k_menu::Item *carName;               /* 0x40 "t_car_name" */
    gt2k_menu::Item *timeItem;              /* 0x44 "t_time" */
    CarIconItem     *icon;                  /* 0x48 "i_car_pict" */
    char             rankText[4];           /* 0x4C */
    char             timeText[0x20];        /* 0x50 */
    char             nameText[0x80];        /* 0x70 */
};
EGCS_SIZE(ResultRankBar, 0xF0);

/* ResultBoard::setup's mode (ResultBoard::mode): what the rows show.  (The
 * sweep moved u041 / u062's copies here.) */
enum {
    RESULT_BOARD_TIMES        = 0,          /* the race times / gaps / laps behind (ResultArcade) */
    RESULT_BOARD_POINTS       = 1,          /* the race's points ("+N pt(s)") */
    RESULT_BOARD_TOTAL_POINTS = 2           /* the championship total (" N pt(s)"), the standings */
};

/* ? name.  Not polymorphic.  Size 0x914 (u041 / u046 / u047 / u050 pads).
 * The RaceMenuPage is a member at +0 (as PauseMenu's page), not a base: the
 * destructor passes it flags 2 and does the deleting itself (u062
 * matching; as a base egcs passes the caller's flags on). */
class ResultBoard {
public:
    /* The page, the bars, displays = NULL, both colours 0.502; then the
     * page "result" of "result1.pmb", the bars' layers and the two colours
     * of the layout. */
    ResultBoard();                          /* 0x001B19A8 */
    /* Every bar's layer cleared; the bars, then ~RaceMenuPage. */
    ~ResultBoard();                         /* 0x001B1CE8 */
    /* `rows' rows in `mode' (RESULT_BOARD_*), opening at
     * t: the title at t, row i at t + 1 + 0.12 i, the term bar after the
     * last row, the end 1 s after it. */
    void setup(int rows, int mode, f32 t);  /* 0x001B1DF0 */
    void clearRows();                       /* 0x001B1F20 */
    void start();                           /* 0x001B1F60 time = 0, running */
    void update();                          /* 0x001B1F70 one frame (1/60 s) while running */
    void stop();                            /* 0x001B2020 running = 0 */
    void draw(GSBuffer *buf);               /* 0x001B2028 the bars while running */
    /* The closing scripts; the board ends t from now. */
    void close(f32 t);                      /* 0x001B20D8 */
    void setName(int row, const char *name, const char *suffix);   /* 0x001B2170 */
    /* Mode 0: row 0 sets the reference; fewer laps: "+N lap(s)", else row 0
     * its time and the others "+gap". */
    void setTime(int row, int time, int laps);  /* 0x001B21A0 */
    void setLaps(int row, int laps);        /* 0x001B2230 mode 0: " N lap(s)" */
    void setSpeed(int row, f32 speed);      /* 0x001B2278 mode 0 (no callers) */
    void setPoints(int row, int points);    /* 0x001B22B0 modes 1 / 2 ('+' in mode 1) */
    void setCar(int row, int car);          /* 0x001B2300 the row's icon: displays[car] */
    /* Row `row' (>= 0) in colors[0], the one before it back in colors[1]. */
    void highlight(int row);                /* 0x001B2348 */
    int  isDone();                          /* 0x001B23C0 endTime < time */

    RaceMenuPage   page;                    /* 0x000 */
    ResultTitleBar title;                   /* 0x1BC */
    ResultTermBar  term;                    /* 0x204 */
    ResultRankBar  bars[7];                 /* 0x240 */
    CarIcon       *displays;                /* 0x8D0 the cars' icons (the race's CarIcon[7]) */
    int            bestTime;                /* 0x8D4 row 0's time */
    int            bestLaps;                /* 0x8D8 row 0's laps */
    int            highlighted;             /* 0x8DC the row in colors[0] (-1: none) */
    RaceMenuColor  colors[2];               /* 0x8E0 "color_mine", "color_others" */
    int            count;                   /* 0x900 the rows shown */
    f32            time;                    /* 0x904 seconds since start() */
    int            mode;                    /* 0x908 0 times, 1 points, 2 total points */
    int            running;                 /* 0x90C */
    f32            endTime;                 /* 0x910 */
};
EGCS_SIZE(ResultBoard, 0x914);

#endif /* GAME_T200_RESULT_BOARD_H */
