/* T200 - the race result board.  0x001B0F28..0x001B27C8.
 *
 * ResultBoard is the page "result" of the layout file "result1": a title
 * bar ("result_title": result / point / total point), seven rank bars
 * ("rank_bar": rank, car name, time or gap / laps / points, car icon) and a
 * closing bar ("term_bar").  setup() gives every bar a start time (the
 * title at t, the rows 0.12 s apart from t + 1, the closing bar last);
 * from then each bar fades in and slides to just under the bar above it
 * (ResultBar::animate: 19 % of the distance per frame), drawn clipped below
 * that bar.  The player's row is drawn in "color_mine", the others in
 * "color_others". */
// DATA_START 0x00331C20
#include <new>                              /* retail's .rodata starts with its "bad_alloc" */
#include "game/menu/race_menu/race_menu_internal.h"
#include "engine/pgl/G001_flare.h"                /* gfx_enable / gfx_disable, gfx_blend_func, gfx_tex_param,
                                               gfx_set_texture_region, the pgl matrix stack, pgl_draw_strip,
                                               pgl_color4f, PrimVertex, PrimUV */
#include "game/car/model/T184_car_icon_maker.h"       /* CarIcon (CarIconItem::display) */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item */
#include "game/menu/framework/T217_operator.h"             /* gt2k_menu::OperatorCode (the bars' scripts) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::PageLayout */
#include "game/common/T494_text_utils.h"           /* format_time, format_speed */
#include "engine/gfx/T512_render_context.h"       /* g_002F4250, the VRAM allocator */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#ifndef BOOT_OLD_GCC
#include "game/race/pause/T169_race_pause.h"           /* __tf14RaceMenuWindow / __ti14RaceMenuWindow (T169's copies) */
#endif
#include "game/menu/race_menu/T197_race_menu_page.h"       /* RaceMenuPage, race_menu_* */
#include "game/data/messages/T201_msg_racemenutext.h"     /* the RACEMENUTEXT texts g_002C5D08 */
#include "game/menu/race_menu/T199_race_menu_layer.h"      /* RaceMenuWindow */
/* A race header after T199's (the original's include order): retail's
 * .rodata has UnitArena<64>'s type name (egcs writes the template's into
 * every object that sees it) between T200's own names and
 * RaceMenuSelectWindow's; T153 declares the template's typeinfo function
 * (which stops egcs) unless GAME_T153_EMITS_TYPEINFO is set. */
#define GAME_T153_EMITS_TYPEINFO
#include "game/race/core/T153_race_course.h"          /* UnitArena<64> */
#include "game/menu/records/T200_result_board.h"

using namespace gt2k_menu;

/* T200's .rodata tables (defined below, each in front of the first function
 * that uses it: retail's .rodata has them between those functions'
 * strings; external, as egcs keeps a static const table until the end). */
/* CarIconItem's textured quad, drawn as a strip (pgl_draw_strip): the unit
 * square scaled to the item's half size. */
extern const PrimUV     g_00331C30[4];      /* 0x00331C30 (0,0) (1,0) (0,1) (1,1) */
extern const PrimVertex g_00331C50[4];      /* 0x00331C50 (-1,-1,0) (1,-1,0) (-1,1,0) (1,1,0) */
/* The bars' operator scripts (RaceMenuLayer::startScript). */
extern const OperatorCode g_00331C80[2];    /* 0x00331C80 close: alpha -2 / s for 0.5 s */
extern const OperatorCode g_00331D28[3];    /* 0x00331D28 the title's: alpha 0, +2 / s for 0.5 s */
extern const OperatorCode g_00331DC8[3];    /* 0x00331DC8 the term bar's: the same */
extern const OperatorCode g_00331E88[3];    /* 0x00331E88 the rows': alpha 0, +2.5 / s for 0.4 s */

/* ------------------------------------------------------------------------ */
/* CarIconItem                                                              */
/* ------------------------------------------------------------------------ */

/* Item(name, layout), its vptr, display = NULL.  Caller: ResultRankBar::init. */
// FUNC 0x001B0F28 CarIconItem::CarIconItem
CarIconItem::CarIconItem(const char *name, PageLayout *layout)
    : Item(name, layout), display(NULL)
{
}

/* The icon's quad: the unit square as a strip (texture coordinates, then
 * positions; scaled to the item's half size). */
const PrimUV g_00331C30[4] = {              /* 0x00331C30 */
    { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }
};
const PrimVertex g_00331C50[4] = {          /* 0x00331C50 */
    { -1.0f, -1.0f, 0.0f }, { 1.0f, -1.0f, 0.0f }, { -1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f, 0.0f }
};

/* Draws the 64x32 icon of the display (when one is set) as a textured quad
 * of the item's size at its screen position, white with the item's alpha:
 * no depth / alpha test, no fog, alpha blending, region clamp to the icon,
 * nearest filtering. */
// FUNC 0x001B0F60 CarIconItem::virtual_2
void CarIconItem::virtual_2()
{
    if (display != NULL) {
        g_002F4250.loadImage(0 /* PSMCT32 */, CAR_ICON_W, CAR_ICON_H, display->pixels);
        gfx_disable(PGL_CAP_DEPTH_TEST);
        gfx_disable(PGL_CAP_ALPHA_TEST);
        gfx_disable(PGL_CAP_FOG);
        gfx_enable(PGL_CAP_BLEND);
        gfx_blend_func(PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD), 1.0f);
        pgl_color4f(1.0f, 1.0f, 1.0f, alphaRatio);    /* the strip's vertex colour */
        f32 screenX = getScreenX();
        f32 screenY = getScreenY();
        f32 halfW = w * 0.5f;
        f32 halfH = h * 0.5f;
        gfx_tex_param(PGL_TEXPARAM_WMS, PGL_WRAP_REGION_CLAMP);
        gfx_tex_param(PGL_TEXPARAM_WMT, PGL_WRAP_REGION_CLAMP);
        gfx_set_texture_region(PGL_REGION_U, 0, CAR_ICON_W - 1);
        gfx_set_texture_region(PGL_REGION_V, 0, CAR_ICON_H - 1);
        gfx_tex_param(PGL_TEXPARAM_MMAG, PGL_FILTER_NEAREST);
        gfx_tex_param(PGL_TEXPARAM_MMIN, PGL_FILTER_NEAREST);
        pgl_push_matrix();
        pgl_translate(screenX, screenY, 0.0f);
        vu0_scale(halfW, halfH, 1.0f);
        pgl_draw_strip(g_00331C50, NULL, NULL, g_00331C30, 4);
        pgl_pop_matrix();
    }
}

/* ------------------------------------------------------------------------ */
/* ResultBar                                                                */
/* ------------------------------------------------------------------------ */

/* Slot 1 of every bar class (index: ResultRankBar::virtual_1).  animate()
 * (virtual) once t has passed the start time.  Caller (direct, title /
 * term bar): ResultBoard::update. */
// FUNC 0x001B10E8 ResultBar::update
void ResultBar::update(f32 t)
{
    if (startTime < t)
        animate();
}

/* Slot 2 (index: ResultRankBar::virtual_2).  render() (virtual) once t has
 * passed the start time.  Caller (direct): ResultBoard::draw. */
// FUNC 0x001B1128 ResultBar::draw
void ResultBar::draw(f32 t)
{
    if (startTime < t)
        render();
}

/* Slot 3 of ResultBar / ResultTitleBar / ResultTermBar (index:
 * ResultTermBar::virtual_3, ResultBar's own slot, inherited).  A frame of
 * the layer; then the bar moves 19 % of the way to its place under the
 * previous bar (frames touching: the centres half the heights apart).  The
 * distance is clamped at 0 from above, so a bar still above the previous
 * one (off screen at -1000) is first put at the previous bar's y. */
// FUNC 0x001B1168 ResultBar::animate
void ResultBar::animate()
{
    RaceMenuLayer::update();
    if (prev != NULL) {
        f32 prevHeight = prev->frameHeight();
        f32 height = frameHeight();
        f32 prevY = prev->frameY();
        f32 dist = frameY() - prevY;
        f32 gap = (prevHeight + height) * 0.5f;
        if (dist < 0.0f)
            dist = 0.0f;
        dist += (gap - dist) * 0.19f;
        setFrameY(prevY + dist);
    }
}

/* Slot 4 (index: ResultTermBar::virtual_4).  Draws the layer clipped to the
 * area below the middle of the previous bar (0, prevY + prevHeight / 2,
 * 640, height + 512); unclipped without a previous bar. */
// FUNC 0x001B1220 ResultBar::render
void ResultBar::render()
{
    if (prev != NULL) {
        f32 prevHeight = prev->frameHeight();
        f32 height = frameHeight();
        race_menu_set_scissor(0.0f, prev->frameY() + prevHeight * 0.5f, (f32)RACE_MENU_SCREEN_W,
                              height + 512.0f);
        drawTree(0);
        race_menu_reset_scissor();
    } else {
        drawTree(0);
    }
}

/* The bars' closing script: alpha -2 / s for 0.5 s. */
const OperatorCode g_00331C80[2] = {        /* 0x00331C80 */
    RESULT_OP(RESULT_OP_FADE_ALPHA, -2.0f, 0.5f, 1),
    RESULT_OP(RESULT_OP_REMOVE, 0.0f, 0.0f, 1)
};

/* The closing script (fade out).  t (the closing time, $f12 from
 * ResultBoard::close) is not used. */
// FUNC 0x001B12D0 ResultBar::close
void ResultBar::close(f32 t)
{
    startScript(g_00331C80);
}

/* ------------------------------------------------------------------------ */
/* ResultTitleBar / ResultTermBar / ResultRankBar                           */
/* ------------------------------------------------------------------------ */

/* The layer "result_title" and its three titles, all hidden; the frame is
 * the "result" title.  Caller: ResultBoard(). */
// FUNC 0x001B12F0 ResultTitleBar::init
void ResultTitleBar::init(PageLayout *layout)
{
    load(layout, "result_title", NULL);
    titleResult = newItem(layout, "b_title_result", 1);
    titlePoint = newItem(layout, "b_title_point", 1);
    Item *total = newItem(layout, "b_title_total_point", 1);
    titleResult->visible = 0;
    titlePoint->visible = 0;
    titleTotal = total;
    total->visible = 0;
    setFrame(titleResult);
}

/* The title's fade in: alpha 0, then +2 / s for 0.5 s. */
const OperatorCode g_00331D28[3] = {        /* 0x00331D28 */
    RESULT_OP(RESULT_OP_SET_ALPHA, 0.0f, 0.0f, 0),
    RESULT_OP(RESULT_OP_FADE_ALPHA, 2.0f, 0.5f, 1),
    RESULT_OP(RESULT_OP_REMOVE, 0.0f, 0.0f, 1)
};

/* Shows (and makes the frame) the title of mode 0 / 1 / 2 (none for
 * another mode), then the fade in.  Caller: ResultBoard::setup. */
// FUNC 0x001B13A0 ResultTitleBar::start
void ResultTitleBar::start(int mode)
{
    titleResult->visible = 0;
    titlePoint->visible = 0;
    titleTotal->visible = 0;
    switch (mode) {
    case RESULT_BOARD_TIMES:
        setFrame(titleResult);
        titleResult->visible = 1;
        break;
    case RESULT_BOARD_POINTS:
        setFrame(titlePoint);
        titlePoint->visible = 1;
        break;
    case RESULT_BOARD_TOTAL_POINTS:
        setFrame(titleTotal);
        titleTotal->visible = 1;
        break;
    }
    resetScripts();
    startScript(g_00331D28);
}

/* The layer "term_bar", frame "b_term". */
// FUNC 0x001B1460 ResultTermBar::init
void ResultTermBar::init(PageLayout *layout)
{
    load(layout, "term_bar", "b_term");
}

/* The term bar's fade in: as the title's. */
const OperatorCode g_00331DC8[3] = {        /* 0x00331DC8 */
    RESULT_OP(RESULT_OP_SET_ALPHA, 0.0f, 0.0f, 0),
    RESULT_OP(RESULT_OP_FADE_ALPHA, 2.0f, 0.5f, 1),
    RESULT_OP(RESULT_OP_REMOVE, 0.0f, 0.0f, 1)
};

/* Off screen (y -1000), then the fade in. */
// FUNC 0x001B1488 ResultTermBar::start
void ResultTermBar::start()
{
    setFrameY(-1000.0f);
    resetScripts();
    startScript(g_00331DC8);
}

/* The layer "rank_bar" (frame "b_bar"), its texts and the car icon (a
 * CarIconItem: Object::operator new(0x104), Object::operator delete if its
 * constructor throws), attached by z; the rank text is index + 1. */
// FUNC 0x001B14D0 ResultRankBar::init
void ResultRankBar::init(PageLayout *layout, int index)
{
    load(layout, "rank_bar", "b_bar");
    rank = newItem(layout, "t_rank", 1);
    carName = newItem(layout, "t_car_name", 1);
    timeItem = newItem(layout, "t_time", 1);
    icon = new CarIconItem("i_car_pict", layout);  /* Object::operator new; Object::operator delete on a throw */ /* implicit: operator new, operator delete (new-expression) */
    attach(icon, 1);
    sprintf(rankText, "%d", index + 1);   /* unbounded into rankText[4]: index is 0..6 */
    rank->setText(rankText);
}

/* The rows' fade in: alpha 0, then +2.5 / s for 0.4 s. */
const OperatorCode g_00331E88[3] = {        /* 0x00331E88 */
    RESULT_OP(RESULT_OP_SET_ALPHA, 0.0f, 0.0f, 0),
    RESULT_OP(RESULT_OP_FADE_ALPHA, 2.5f, 0.4f, 1),
    RESULT_OP(RESULT_OP_REMOVE, 0.0f, 0.0f, 1)
};

/* Off screen (y -1000), the fade in, no name, "Running...". */
// FUNC 0x001B15F8 ResultRankBar::start
void ResultRankBar::start()
{
    setFrameY(-1000.0f);
    resetScripts();
    startScript(g_00331E88);
    setName("", NULL);
    setRunning();
}

/* Empty name and time texts.  Caller: ResultBoard::clearRows. */
// FUNC 0x001B1658 ResultRankBar::clearTexts
void ResultRankBar::clearTexts()
{
    carName->setText("");
    timeItem->setText("");
}

/* Slot 4 (index: ResultRankBar::virtual_4): ResultBar's. */
// FUNC 0x001B16A0 ResultRankBar::render
void ResultRankBar::render()
{
    ResultBar::render();
}

/* Slot 3 (index: ResultRankBar::virtual_3): ResultBar's. */
// FUNC 0x001B16C0 ResultRankBar::animate
void ResultRankBar::animate()
{
    ResultBar::animate();
}

/* The car name text: name, or "name suffix".  Nothing when name is NULL. */
// FUNC 0x001B16E0 ResultRankBar::setName
void ResultRankBar::setName(const char *name, const char *suffix)
{
    if (name != NULL) {
        strcpy(nameText, name);
        if (suffix != NULL) {
            strcat(nameText, " ");
            strcat(nameText, suffix);
        }
        carName->setText(nameText);
    }
}

/* The time text: a time (ms, format_time), after '+' (a gap to the winner)
 * or ' '. */
// FUNC 0x001B1750 ResultRankBar::setTimeGap
void ResultRankBar::setTimeGap(int time, int plus)
{
    format_time(timeText + 1, time);
    timeText[0] = plus != 0 ? '+' : ' ';
    timeItem->setText(timeText);
}

/* The time text: "+N" / " N" and "lap" (N < 2) or "laps" (RACEMENUTEXT). */
// FUNC 0x001B17A0 ResultRankBar::setLaps
void ResultRankBar::setLaps(int laps, int plus)
{
    sprintf(timeText, "%c%d", plus != 0 ? '+' : ' ', laps);
    strcat(timeText, laps < 2 ? g_002C5D08[RACEMENUTEXT_LAP] : g_002C5D08[RACEMENUTEXT_LAPS]);
    timeItem->setText(timeText);
}

/* The time text: "+N" / " N" and "pt" (N < 2) or "pts". */
// FUNC 0x001B1830 ResultRankBar::setPoints
void ResultRankBar::setPoints(int points, int plus)
{
    sprintf(timeText, "%c%d", plus != 0 ? '+' : ' ', points);
    strcat(timeText, points < 2 ? g_002C5D08[RACEMENUTEXT_PT] : g_002C5D08[RACEMENUTEXT_PTS]);
    timeItem->setText(timeText);
}

/* The time text: a speed (m/s, format_speed: in the user's unit). */
// FUNC 0x001B18C0 ResultRankBar::setSpeed
void ResultRankBar::setSpeed(f32 speed)
{
    format_speed(timeText, speed);
    timeItem->setText(timeText);
}

/* The time text: "Running..." (RACEMENUTEXT). */
// FUNC 0x001B1900 ResultRankBar::setRunning
void ResultRankBar::setRunning()
{
    timeItem->setText(g_002C5D08[RACEMENUTEXT_RUNNING]);
}

/* The enhanced engine (new, Lupo 2 L2 0x001B7890: the map pairs it with
 * GT3's setRunning, whose twin is L2 0x001B7868; menus_b.md 14): the time
 * text of a disqualified car, "Disqualified" (RACEMENUTEXT). */
void ResultRankBar::setDisqualified()
{
    timeItem->setText(g_002C5D08[RACEMENUTEXT_DISQUALIFIED]);
}

/* No callers. */
// FUNC 0x001B1928 ResultRankBar::clearTime
void ResultRankBar::clearTime()
{
    timeItem->setText("");
}

/* The colour of the name and time texts. */
// FUNC 0x001B1950 ResultRankBar::setTextColor
void ResultRankBar::setTextColor(const RaceMenuColor *color)
{
    race_menu_set_text_color(color, carName);
    race_menu_set_text_color(color, timeItem);
}

/* The icon's display (CarIconItem::setDisplay inline). */
// FUNC 0x001B1998 ResultRankBar::setCarDisplay
void ResultRankBar::setCarDisplay(CarIcon *d)
{
    icon->setDisplay(d);
}

/* ------------------------------------------------------------------------ */
/* ResultBoard                                                              */
/* ------------------------------------------------------------------------ */

/* The page's RaceMenuPage(); the bars (their inline constructors: RaceMenuLayer's,
 * RootObject::RootObject of obj, the bars' fields; a throw unwinds what was
 * built), displays = NULL, both colours 0.502; then the page "result" of
 * "result1.pmb", the bars' layers and the two colours of the layout.
 * Callers: ResultArcade (0x001758B0) and the race modes RaceArcade,
 * RaceSplitBattle, RaceLinkBattle, RaceChampionship (T134 / T137 / T138 /
 * T151). */
// FUNC 0x001B19A8 ResultBoard::ResultBoard
ResultBoard::ResultBoard()
    : displays(NULL)                        /* the bars: obj's RootObject::RootObject each */
{
    /* implicit: RaceMenuPage (member constructor), RootObject (each bar's inline constructor), ~RaceMenuLayer, ~RaceMenuPage (exception cleanup) */
    count = 0;
    time = 0.0f;
    mode = RESULT_BOARD_TIMES;
    running = 0;
    endTime = 100000.0f;
    page.load("result1", "result");
    PageLayout *lay = &page.layout;
    title.init(lay);
    for (int i = 0; i < RESULT_BOARD_ROWS; i++)
        bars[i].init(lay, i);
    term.init(lay);
    race_menu_get_color(&colors[0], lay, "color_mine", 0);
    race_menu_get_color(&colors[1], lay, "color_others", 0);
}

/* Clears every bar's layer; then the bars' destructors (the rank bars last
 * first, through their vtables, flags 0; the term and title bars inline:
 * RaceMenuWindow's vptr, RaceMenuLayer::~RaceMenuLayer flags 2), then
 * the page's RaceMenuPage::~RaceMenuPage (flags 2); egcs deleting flag:
 * operator delete (__builtin_delete).  Callers: the ResultArcade family's and race modes'
 * destructors. */
// FUNC 0x001B1CE8 ResultBoard::~ResultBoard
ResultBoard::~ResultBoard()
{
    title.clear();
    term.clear();
    for (int i = 0; i < RESULT_BOARD_ROWS; i++)
        bars[i].clear();
    /* egcs: the bars (RaceMenuLayer::~RaceMenuLayer), RaceMenuPage::
     * ~RaceMenuPage(2), operator delete if flags & 1 */
    /* implicit: ~RaceMenuLayer (the bars' inline destructors), ~RaceMenuPage (member destructor), operator delete (deleting destructor) */
}

/* Lays the board out for `rows' rows in `mode', opening at t: the title at
 * t, row i at t + 1 + 0.12 i (each under the previous bar, in
 * "color_others"), the term bar after the last row, the end 1 s after it.
 * Callers: ResultArcade::start, ResultChampionship::update. */
// FUNC 0x001B1DF0 ResultBoard::setup
void ResultBoard::setup(int rows, int mode, f32 t)
{
    ResultBar *above;
    f32 at;

    time = 0.0f;
    title.setStartTime(t);
    count = rows;
    running = 0;
    bestTime = 0;
    bestLaps = 0;
    highlighted = -1;
    this->mode = mode;
    title.start(mode);
    above = &title;
    at = t + 1.0f;
    for (int i = 0; i < rows; i++) {
        bars[i].setStartTime(at);
        bars[i].setPrev(above);
        bars[i].start();
        bars[i].setTextColor(&colors[1]);
        above = &bars[i];
        at += 0.12f;
    }
    term.setStartTime(at);
    term.setPrev(above);
    term.start();
    endTime = at + 1.0f;
}

/* Caller: ResultArcade::virtual_2. */
// FUNC 0x001B1F20 ResultBoard::clearRows
void ResultBoard::clearRows()
{
    for (int i = 0; i < RESULT_BOARD_ROWS; i++)
        bars[i].clearTexts();
}

/* Callers: ResultArcade::virtual_6, ResultChampionship::update. */
// FUNC 0x001B1F60 ResultBoard::start
void ResultBoard::start()
{
    time = 0.0f;
    running = 1;
}

/* One frame (1/60 s) while running: every bar's update (the title and term
 * bars called directly, the rows through their vtables). */
// FUNC 0x001B1F70 ResultBoard::update
void ResultBoard::update()
{
    if (running != 0) {
        time = time + 0.016666668f;
        title.update(time);
        term.update(time);
        for (int i = 0; i < count; i++)     /* through the address: a virtual call (retail's vptr
                                               load has its own induction variable) */
            (&bars[i])->update(time);
    }
}

/* Caller: ResultArcade::virtual_2. */
// FUNC 0x001B2020 ResultBoard::stop
void ResultBoard::stop()
{
    running = 0;
}

/* Draws the bars into buf while running.  Caller: ResultArcade::virtual_4. */
// FUNC 0x001B2028 ResultBoard::draw
void ResultBoard::draw(GSBuffer *buf)
{
    if (running != 0) {
        race_menu_begin_draw(buf);
        title.draw(time);
        term.draw(time);
        for (int i = 0; i < count; i++)
            (&bars[i])->draw(time);
        race_menu_end_draw();
    }
}

/* The bars fade out; the board ends t from now.  Caller:
 * ResultChampionship::update. */
// FUNC 0x001B20D8 ResultBoard::close
void ResultBoard::close(f32 t)
{
    if (running != 0) {
        title.close(t);
        term.close(t);
        for (int i = 0; i < count; i++)
            bars[i].close(t);
        endTime = time + t;
    }
}

/* The car name of a row ("name suffix"). */
// FUNC 0x001B2170 ResultBoard::setName
void ResultBoard::setName(int row, const char *name, const char *suffix)
{
    bars[row].setName(name, suffix);
}

/* Times mode only.  Row 0 sets the reference (time, laps).  A row with
 * other laps than row 0 shows "+N lap(s)" behind; otherwise row 0 shows its
 * time and the others "+gap".  Caller: ResultArcade::virtual_9. */
// FUNC 0x001B21A0 ResultBoard::setTime
void ResultBoard::setTime(int row, int time, int laps)
{
    if (mode == RESULT_BOARD_TIMES) {
        int winner = row == 0;
        if (winner) {
            bestTime = time;
            bestLaps = laps;
        }
        if (bestLaps == laps) {
            bars[row].setTimeGap(winner ? time : time - bestTime, !winner);
        } else {
            bars[row].setLaps(bestLaps - laps, 1);
        }
    }
}

/* Times mode only: " N lap(s)" (row 0 sets the reference laps).  Caller:
 * ResultArcade::virtual_9. */
// FUNC 0x001B2230 ResultBoard::setLaps
void ResultBoard::setLaps(int row, int laps)
{
    if (mode == RESULT_BOARD_TIMES) {
        if (row == 0)
            bestLaps = laps;
        bars[row].setLaps(laps, 0);
    }
}

/* Times mode only: a speed.  No callers. */
// FUNC 0x001B2278 ResultBoard::setSpeed
void ResultBoard::setSpeed(int row, f32 speed)
{
    if (mode == RESULT_BOARD_TIMES)
        bars[row].setSpeed(speed);
}

/* Points modes: the points ('+' before the race's points, ' ' before a
 * total).  Caller: ResultChampionship::update. */
// FUNC 0x001B22B0 ResultBoard::setPoints
void ResultBoard::setPoints(int row, int points)
{
    if ((u32)(mode - RESULT_BOARD_POINTS) < 2)
        bars[row].setPoints(points, mode == RESULT_BOARD_POINTS);
}

/* The icon of row `row' shows car `car''s icon (when there are icons). */
// FUNC 0x001B2300 ResultBoard::setCar
void ResultBoard::setCar(int row, int car)
{
    if (displays != NULL)
        bars[row].setCarDisplay(&displays[car]);
}

/* Row `row' (>= 0) in "color_mine", the one highlighted before it back in
 * "color_others". */
// FUNC 0x001B2348 ResultBoard::highlight
void ResultBoard::highlight(int row)
{
    int old = highlighted;
    if (row != old && row >= 0) {
        if (old >= 0)
            bars[old].setTextColor(&colors[1]);
        highlighted = row;
        bars[row].setTextColor(&colors[0]);
    }
}

/* The enhanced engine (new, Lupo 2 L2 0x001B83A0; menus_b.md 14 a): row
 * `row' reads "Disqualified".  Caller: ResultArcade::update (the cars of
 * RACE_EVENT_CAR_DISQUALIFIED, placed from the last row up).  A row outside
 * the board is ignored. */
void ResultBoard::setDisqualified(int row)
{
    if (row >= 0 && row < RESULT_BOARD_ROWS)
        bars[row].setDisqualified();
}

/* The enhanced engine (new, Lupo 2 L2 0x001B8218; menus_b.md 14 b): a row of
 * a laps-only result (a time-limit race).  Times mode only.  Row 0 (the
 * leader) sets the reference (its time and laps) and shows its laps; a row
 * on the leader's lap shows its time gap ("+gap"), any other its laps
 * (GT3's setLaps shows every row's laps).  Caller: ResultArcade::
 * setRowResult. */
void ResultBoard::setLapsGap(int row, int time, int laps)
{
    if (mode == RESULT_BOARD_TIMES) {
        if (row == 0) {
            bestTime = time;
            bestLaps = laps;
            bars[row].setLaps(laps, 0);
        } else if (bestLaps == laps) {
            bars[row].setTimeGap(time - bestTime, 1);
        } else {
            bars[row].setLaps(laps, 0);
        }
    }
}

/* 1 once the board's time has passed its end time.  Callers:
 * ResultArcade::virtual_8, ResultChampionship::update,
 * ResultLinkBattle::waitEnd, Result2PBattle::waitEnd. */
// FUNC 0x001B23C0 ResultBoard::isDone
int ResultBoard::isDone()
{
    return time > endTime;
}

/* ------------------------------------------------------------------------ */
/* Out-of-line copies of the inline members, typeinfo functions, the        */
/* implicit destructors                                                     */
/* ------------------------------------------------------------------------ */

// FUNC 0x001B23E8 ResultRankBar::ResultRankBar
/* inline in game/menu/records/T200_result_board.h; egcs emits this copy (RaceMenuLayer's
 * part with obj's RootObject::RootObject, frame / prev / startTime 0, the
 * vptr, the four items NULL, empty rank / time texts); ResultBoard()
 * expands it */
/* implicit: RootObject (member constructor) */

#ifndef BOOT_OLD_GCC
/* The __si_type_info __ti13ResultRankBar, base __ti9ResultBar. */
// FUNC 0x001B2460 __tf13ResultRankBar
extern "C" const type_info &__tf13ResultRankBar(void)
{
    if (__ti13ResultRankBar[0] == 0) {
        __tf9ResultBar();
        __rtti_si(__ti13ResultRankBar, "13ResultRankBar", (const type_info *)__ti9ResultBar);
    }
    return *(const type_info *)__ti13ResultRankBar;
}
#endif

// FUNC 0x001B24B0 ResultRankBar::~ResultRankBar
/* the implicit destructor (slot 0): RaceMenuWindow's vptr (0x00330140, the
 * inline ~RaceMenuWindow), then RaceMenuLayer::~RaceMenuLayer with the
 * caller's flags */
/* implicit: ~RaceMenuLayer (base destructor) */

#ifndef BOOT_OLD_GCC
// FUNC 0x001B24D8 __tf13ResultTermBar
extern "C" const type_info &__tf13ResultTermBar(void)
{
    if (__ti13ResultTermBar[0] == 0) {
        __tf9ResultBar();
        __rtti_si(__ti13ResultTermBar, "13ResultTermBar", (const type_info *)__ti9ResultBar);
    }
    return *(const type_info *)__ti13ResultTermBar;
}
#endif

// FUNC 0x001B2528 ResultTermBar::~ResultTermBar
/* the implicit destructor, as ResultRankBar's: RaceMenuWindow's vptr, then
 * RaceMenuLayer::~RaceMenuLayer */
/* implicit: ~RaceMenuLayer (base destructor) */

#ifndef BOOT_OLD_GCC
// FUNC 0x001B2550 __tf14ResultTitleBar
extern "C" const type_info &__tf14ResultTitleBar(void)
{
    if (__ti14ResultTitleBar[0] == 0) {
        __tf9ResultBar();
        __rtti_si(__ti14ResultTitleBar, "14ResultTitleBar", (const type_info *)__ti9ResultBar);
    }
    return *(const type_info *)__ti14ResultTitleBar;
}
#endif

// FUNC 0x001B25A0 ResultTitleBar::~ResultTitleBar
/* the implicit destructor, as ResultRankBar's: RaceMenuWindow's vptr, then
 * RaceMenuLayer::~RaceMenuLayer */
/* implicit: ~RaceMenuLayer (base destructor) */

// FUNC 0x001B25C8 ResultBar::setPrev
/* inline in game/menu/records/T200_result_board.h; egcs emits this copy (ResultBoard::
 * setup expands it) */

// FUNC 0x001B25D0 ResultBar::setStartTime
/* inline in game/menu/records/T200_result_board.h; egcs emits this copy (ResultBoard::
 * setup expands it) */

// FUNC 0x001B25D8 ResultBar::~ResultBar
/* inline (empty) in game/menu/records/T200_result_board.h; egcs emits this copy between
 * setStartTime and the constructor (the order shows it was declared there):
 * RaceMenuWindow's vptr, then RaceMenuLayer::~RaceMenuLayer */
/* implicit: ~RaceMenuLayer (base destructor) */

// FUNC 0x001B2600 ResultBar::ResultBar
/* inline in game/menu/records/T200_result_board.h; egcs emits this copy (RaceMenuLayer's
 * part with obj's RootObject::RootObject, the vptr, frame = prev =
 * startTime = 0, the float stored as the integer 0) */
/* implicit: RootObject (member constructor) */

#ifndef BOOT_OLD_GCC
/* The __si_type_info __ti9ResultBar, base __ti14RaceMenuWindow (it calls
 * T169's __tf14RaceMenuWindow 0x001A2E78, the copy that won the link).
 * Callers: the three bars' typeinfo functions. */
// FUNC 0x001B2660 __tf9ResultBar
extern "C" const type_info &__tf9ResultBar(void)
{
    if (__ti9ResultBar[0] == 0) {
        __tf14RaceMenuWindow();
        __rtti_si(__ti9ResultBar, "9ResultBar", (const type_info *)__ti14RaceMenuWindow);
    }
    return *(const type_info *)__ti9ResultBar;
}
#endif

// FUNC 0x001B26B0 CarIconItem::setDisplay
/* inline in game/menu/records/T200_result_board.h; egcs emits this copy
 * (ResultRankBar::setCarDisplay expands it) */

#ifndef BOOT_OLD_GCC
/* The __si_type_info __ti11CarIconItem, base __tiQ29gt2k_menu4Item. */
// FUNC 0x001B26B8 __tf11CarIconItem
extern "C" const type_info &__tf11CarIconItem(void)
{
    if (__ti11CarIconItem[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__ti11CarIconItem, "11CarIconItem", (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return *(const type_info *)__ti11CarIconItem;
}
#endif

// FUNC 0x001B2708 CarIconItem::~CarIconItem
/* the implicit destructor (slot 0): Item::~Item(this, 0) (no vptr store of
 * its own), then gt2k_menu's Object::operator delete for the egcs deleting
 * flag */
/* implicit: ~Item (base destructor), operator delete (deleting destructor) */

// FUNC 0x001B2750 RaceMenuWindow::~RaceMenuWindow
/* inline in game/menu/race_menu/T199_race_menu_layer.h; egcs emits this copy (T200's:
 * RaceMenuWindow's vptr, then RaceMenuLayer::~RaceMenuLayer with the
 * caller's flags; the vtables use T169's 0x001A2E50, so this one is never
 * referenced) */
/* implicit: ~RaceMenuLayer (base destructor) */

#ifndef BOOT_OLD_GCC
/* T200's copy of __tf14RaceMenuWindow (0x001A2E78, T169's, is the one
 * used): the __si_type_info __ti14RaceMenuWindow, base __ti13RaceMenuLayer
 * (gp-0x4610); its name string is T200's (0x003320D8).  A static stand-in:
 * the extern "C" name is T169's. */
// FUNC 0x001B2778 __tf14RaceMenuWindow
static const type_info &T200_tf14RaceMenuWindow(void)
{
    if (__ti14RaceMenuWindow[0] == 0) {
        __tf13RaceMenuLayer();
        __rtti_si(__ti14RaceMenuWindow, "14RaceMenuWindow", (const type_info *)__ti13RaceMenuLayer);
    }
    return *(const type_info *)__ti14RaceMenuWindow;
}
#endif
