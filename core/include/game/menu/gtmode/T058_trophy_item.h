/* T058_trophy_item.h - gt2k_menu::TrophyItem / ChampionShipTrophyItem, the
 * "render_model" item of the prize screens, and gt2k_menu::PrizeModelSpec,
 * the prize model names (translation unit T058, unit u019),
 * 0x00138FC8..0x0013A088.
 *
 * A TrophyItem shows its screen's prize model (the screen's +0xDE8, a
 * MiniPrize or a PrizeBase) when built, waits `delay' seconds, fades in over
 * `fade' seconds and draws the model with accumulation-buffer antialiasing
 * and depth of field (draw_trophy_ring): 4..7 passes, the eye moved around a
 * circle with the focus plane fixed and the drawing offset jittered by a
 * sub-pixel pattern (g_0029ED80 .. g_0029EDF8: the jitter tables of the
 * OpenGL Programming Guide's accumulation examples), averaged, then blended
 * over the screen with the item's alpha; the championship one
 * (TrophyGetControl, T060) shifts the drawing offset by 160 pixels in x.
 * The factories of every prize screen differ only in the pass count and the
 * times.  PrizeModelSpec names the .prm file: the licence trophies
 * "lic_b", "lic_a", "lic_ib", "lic_ia", "lic_s", "lic_d" and the
 * championship cups "champ_00".."champ_09", one of them picked at random
 * with weights that grow with the game's completion. */
#ifndef GAME_T058_TROPHY_ITEM_H
#define GAME_T058_TROPHY_ITEM_H

#include "game/game.h"
#include "game/menu/framework/T213_menu_item.h"            /* base gt2k_menu::Item */
#include "game/menu/gtmode/T460_prize_base.h"           /* PrizeBase (TrophyItem's inline destructor) */

namespace gt2k_menu {
class Object;                               /* T220 (pointers only) */
class Transitive;                           /* T216 */
class PageLayout;                           /* T229 */
struct MenuLayoutItem;                      /* T229 */

/* (? name) Accumulation-buffer antialiasing and depth of field: the model
 * of prize drawn count times (clamped to 4..7) over a copy of the screen,
 * pass i with the drawing offset jitter point i plus (x, y) pixels and,
 * when focus > 1, the eye moved by eyeDelta at i * 360 / count degrees with
 * the plane at the focus distance fixed (prize_projection_eye); each pass
 * mixed into an accumulation buffer with weight 1 / (i + 1) (the running
 * mean), the mean mixed over the screen with alpha; cb goes to
 * PrizeBase::draw.  (Declared before TrophyItem: its inline draw calls it.)
 * The parameter order is u019's matching pass's: cb after the three floats
 * (the registers are the same, $a0..$a2 / $f12..$f16; the order of the
 * parameter copies decides retail's $s7 for cb and $fp for the depth
 * buffer). */
void draw_trophy_ring(PrizeBase *prize, int count, f32 alpha, f32 focus, f32 eyeDelta,
                      CallBackWhenPrizeRendering *cb, f32 x, f32 y);   /* 0x001394A0 */

/* vtable 0x003180C0 (8 slots: [3] Object's 0x001BFF00, [4] Item's, [5]
 * Image's, [6] / [7] Transitive's).  typeinfo __tfQ29gt2k_menu10TrophyItem
 * 0x0013A038 (si, base Item; storage 0x004B17D0).  vptr +0x1C (Object's);
 * size 0x11C (operator new in the factories). */
class TrophyItem : public Item {
public:
    /* Item(NULL, NULL); shows the prize at once (p may be NULL); start /
     * end = the fade-in frames (60 a second, negative times count as 0). */
    TrophyItem(PrizeBase *p, int n, f32 delay, f32 fade);  /* 0x00139218 */
    /* Inline: ChampionShipTrophyItem's destructor expands it (0x00139FB8 is
     * the out-of-line copy for the vtable).  Hides the prize and unloads
     * its model, then ~Item. */
    virtual ~TrophyItem()                   /* [0] +0x0C 0x00139FB8 */
    {
        if (prize != 0) {
            prize->hide();
            prize->unload();
        }
    }
    /* update: waiting until frame start, fading in until end, then shown;
     * the prize animated once shown; frame++; then Item's.  (Both vtables'
     * slot 1.) */
    virtual void virtual_1();               /* [1] +0x14 0x00139318 */
    /* draw: draw_trophy_ring at the item's place (offset 0, 0; Item's draw
     * is not called).  Inline: 0x00139F58, the copy for the vtable, is
     * emitted with the class's other inline functions (egcs emits them in
     * reverse order of declaration: getAlpha 0x00139EF0, setAlpha, this,
     * ~TrophyItem 0x00139FB8). */
    virtual void virtual_2()                /* [2] +0x1C 0x00139F58 */
    {
        if (prize != 0)
            draw_trophy_ring(prize, count, alpha, prize->getFocus(), prize->getEyeDelta(), 0,
                             0.0f, 0.0f);
    }
    /* Inline (update expands setAlpha); 0x00139EF8 / 0x00139EF0 are
     * out-of-line copies without callers.  The lower bound is compared in
     * double precision. */
    void setAlpha(f32 a)                    /* 0x00139EF8 clamped to 0..1 */
    {
        alpha = a;
        if (alpha < 0.0)
            alpha = 0.0f;
        else if (alpha > 1.0f)
            alpha = 1.0f;
    }
    f32  getAlpha() { return alpha; }       /* 0x00139EF0 */

    /*         Item                            0x000 .. 0x100 (vptr +0x1C) */
    PrizeBase *prize;                       /* 0x100 the screen's model (+0xDE8), NULL none */
    f32        alpha;                       /* 0x104 0..1 */
    int        count;                       /* 0x108 passes of draw_trophy_ring (4..7) */
    u32        frame;                       /* 0x10C frames since built */
    int        start;                       /* 0x110 delay * 60 */
    int        end;                         /* 0x114 (delay + fade) * 60 */
    s8         state;                       /* 0x118 0 waiting, 1 fading in, 2 shown */
};
EGCS_SIZE(gt2k_menu::TrophyItem, 0x11C);

/* vtable 0x00318078 (8 slots: [1] TrophyItem's).  typeinfo
 * __tfQ29gt2k_menu22ChampionShipTrophyItem 0x00139E20 (si, base
 * TrophyItem; storage 0x004B17C0).  Size 0x11C. */
class ChampionShipTrophyItem : public TrophyItem {
public:
    ChampionShipTrophyItem(PrizeBase *p, int n, f32 delay, f32 fade);  /* 0x00139B68 */
    /* [0] +0x0C 0x00139E70 ~ChampionShipTrophyItem: implicit (no vptr store; ~TrophyItem expanded) */
    /* draw: TrophyItem's with the drawing offset +160 pixels in x. */
    virtual void virtual_2();               /* [2] +0x1C 0x00139BA0 */
};
EGCS_SIZE(gt2k_menu::ChampionShipTrophyItem, 0x11C);

/* PrizeModelSpec::kind. */
enum { PRIZE_MODEL_LICENSE = 0, PRIZE_MODEL_CUP = 1 };

/* (? name) Which .prm file a prize screen loads.  Not polymorphic; 8
 * bytes (a stack object of LicenseGetControl::load and TrophyGetControl::
 * load, which expand the inline constructor). */
struct PrizeModelSpec {
    PrizeModelSpec() : kind(-1), index(0) {}
    /* kind 0; index = licence for 1..5, else 0. */
    void setLicense(u32 licence);           /* 0x00139060 */
    /* kind 1; index = n for n < 10, else a cup picked at random, weighted
     * by rate + 165.333 * completion^3 (g_0029ED30). */
    void setChampionship(u32 n);            /* 0x001390D0 */
    void getPath(char *buf);                /* 0x001391A8 "menu/<name>.prm" */
    /* (? names) The kind tests of getPath (inline: retail tests their
     * flags, bnez on kind, then xori kind, 1 / bnel; kind == 1 in place
     * gives li 1 / bnel). */
    bool isLicense() const { return kind == PRIZE_MODEL_LICENSE; }
    bool isChampionship() const { return kind == PRIZE_MODEL_CUP; }

    s8  kind;                               /* 0x0 0 licence trophy, 1 championship cup, -1 none */
    int index;                              /* 0x4 */
};
EGCS_SIZE(gt2k_menu::PrizeModelSpec, 0x8);

/* (? name) An entry of the cup table g_0029ED30 (T058_static_init). */
struct TrophyModelEntry {
    f32 getRate(f32 bonus);                 /* 0x00138FC8 rate + the bonus if positive */

    const char *name;                       /* 0x0 "champ_00" .. */
    f32         rate;                       /* 0x4 the weight of the random pick */
};
EGCS_SIZE(gt2k_menu::TrophyModelEntry, 0x8);

/* (? name; a jitter point) The sub-pixel offset of one pass of
 * draw_trophy_ring (gfx_offset_xy, pixels 0..1): the tables are the OpenGL
 * Programming Guide's j4 .. j7 jitter patterns. */
struct TrophyPos {
    f32 x;                                  /* 0x0 */
    f32 y;                                  /* 0x4 */
};
EGCS_SIZE(gt2k_menu::TrophyPos, 0x8);

/* The sum of the ten cups' getRate(bonus). */
f32  trophy_model_total_rate(f32 bonus);    /* 0x00138FF0 */

} /* namespace gt2k_menu */

/* The "render_model" factories (MenuHookFn, T229): ctx is the screen (its
 * prize at +0xDE8: a PrizeControlBase for _license / _unused /
 * _championship, a MiniPrizeControlBase for the others); only ctx and the
 * item's z are used.  A TrophyItem (passes, delay, fade): license 6, 2,
 * 1; unused (no callers) 4, 3, 1.5; arcade and gtmode 6, 1, 2; consolation
 * 7, 1, 2; championship a ChampionShipTrophyItem 4, 3, 1.5. */
gt2k_menu::Transitive *create_trophy_license(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00139870 */
gt2k_menu::Transitive *create_trophy_unused(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00139908 */
gt2k_menu::Transitive *create_trophy_arcade(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x001399A0 */
gt2k_menu::Transitive *create_trophy_gtmode(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00139A38 */
gt2k_menu::Transitive *create_trophy_consolation(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00139AD0 */
gt2k_menu::Transitive *create_trophy_championship(void *ctx, gt2k_menu::PageLayout *layout,
        gt2k_menu::Object *parent, gt2k_menu::MenuLayoutItem *item,
        int p8, int pC, int p10, int p14);  /* 0x00139C08 */
/* .ctors entry 0x0028F070: the cup table g_0029ED30 (names and rates) and
 * the licence trophy names g_0029ED18. */
void T058_static_init(void);                /* 0x00139CA0 */

extern const char *g_0029ED18[6];           /* 0x0029ED18 the licence trophies "lic_b" .. "lic_d" */
extern gt2k_menu::TrophyModelEntry g_0029ED30[10];  /* 0x0029ED30 the championship cups */
extern gt2k_menu::TrophyPos g_0029ED80[4];  /* 0x0029ED80 the jitter of 4 passes */
extern gt2k_menu::TrophyPos g_0029EDA0[5];  /* 0x0029EDA0 5 */
extern gt2k_menu::TrophyPos g_0029EDC8[6];  /* 0x0029EDC8 6 */
extern gt2k_menu::TrophyPos g_0029EDF8[7];  /* 0x0029EDF8 7 */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu22ChampionShipTrophyItem(void);  /* 0x00139E20 */
void *__tfQ29gt2k_menu10TrophyItem(void);              /* 0x0013A038 */
}
#endif

#endif /* GAME_T058_TROPHY_ITEM_H */
