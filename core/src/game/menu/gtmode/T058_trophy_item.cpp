/* T058 - gt2k_menu::TrophyItem / ChampionShipTrophyItem and the prize model
 * names.  0x00138FC8..0x0013A088.
 *
 * The "render_model" element of every prize screen is a TrophyItem: it shows
 * the screen's prize model (the screen's +0xDE8, a MiniPrize or a PrizeBase)
 * when built, waits `delay' seconds, fades in over `fade' seconds, and draws
 * the model with accumulation-buffer antialiasing and depth of field
 * (draw_trophy_ring): count passes (4..7), each with the eye moved around a
 * circle of radius eyeDelta while the plane at the focus distance stays put
 * (an off-axis projection), and the drawing offset jittered by a sub-pixel
 * pattern (g_0029ED80.., the jitter tables of the OpenGL Programming Guide's
 * accumulation buffer examples), averaged, then blended over the screen with
 * the item's alpha.  The factories differ only in the pass count and the
 * times; the championship one (TrophyGetControl, T060) shifts the drawing
 * offset by 160 pixels.
 *
 * PrizeModelSpec names the .prm file: licence trophies "lic_b", "lic_a",
 * "lic_ib", "lic_ia", "lic_s", "lic_d" (LicenseGetControl::load) and the
 * championship cups "champ_00".."champ_09" (TrophyGetControl, T060), one of
 * them picked at random with weights that grow with the game's completion. */
/* The includes, in the original's class order (egcs writes the type names of
 * the classes it sees in the reverse order of their definitions).  Retail
 * T058's .rodata (0x00317F90..0x0031857E): "bad_alloc" (<new>), "%s Cr. %s"
 * (T099's inline PrizeCounter items), setLicense's jump table, the
 * functions' literals, the two vtables, then the type names
 * ConsolationControl, GTmodeTrophyControl, ArcadeTrophyControl,
 * MiniPrizeControlBase, TrophyGetControl, the three PrizeCounter items,
 * PrizeCounter, LicenseGetControl, PrizeControlBase,
 * ControlBaseWithPageManager, SmartIndexT<int>, ChampionShipTrophyItem,
 * TrophyItem, CallBackWhenPrizeRendering, PrizeModelSetContext, the
 * OperatorHolder group, UnitArena<64>, RegurationResult, RecordsMenuArg, the
 * garage lists, Operator: the own header right after T460's, the screens
 * (T059 whole, then T060, T056) after the page manager.  Its .sdata
 * (0x00351610): T099's inline "" and "Cr. %s", then "lic_b" .. "lic_d".
 * ref/data/u019/notes.md. */
// DATA_START 0x00317F90
// DATA_START 0x00351610
#include <new>                              /* first: "bad_alloc" */
#include "game/save/T429_game_status.h"          /* gt2k_game::GameStatus, getGameStatus (the garage) */
#include "game/menu/records/T089_records_menu_args.h"    /* (no use: RecordsMenuArg's name) */
#include "game/menu/gtmode/T045_reguration_result.h"    /* (no use: RegurationResult's name) */
#define GAME_T153_EMITS_TYPEINFO            /* (no use: UnitArena<64>'s name, which T153_race_course.h */
#include "game/race/core/T153_race_course.h"          /*  leaves out unless asked) */
#include "game/menu/framework/T216_menu_transitive.h"      /* gt2k_menu::Transitive (the OperatorHolder group) */
#include "game/menu/gtmode/T460_prize_base.h"           /* PrizeBase, CallBackWhenPrizeRendering (PrizeModelSetContext) */
#include "game/menu/gtmode/T058_trophy_item.h"          /* TrophyItem, ChampionShipTrophyItem, PrizeModelSpec */
#include "game/menu/framework/T211_page_manager.h"         /* (no use: SmartIndexT<int>) */
#include "game/menu/gtmode/T059_license_get_control.h"  /* PrizeControlBase (ControlBaseWithPageManager, LicenseGetControl) */
#include "game/menu/gtmode/T060_trophy_get.h"           /* (no use: PrizeCounter and its items, TrophyGetControl) */
#include "game/menu/gtmode/T056_gtmode_trophy_control.h" /* MiniPrizeControlBase (.. ConsolationControl) */
/* The rest adds no type name. */
#include "engine/pgl/G001_flare.h"                /* gfx_enable / gfx_disable, gfx_color_mask, gfx_depth_mask,
                                               gfx_offset_xy, gfx_frame_effect, gs_clear_buffers, pgl_*,
                                               vu0_load_identity_regs */
#include "game/menu/framework/T213_menu_item.h"            /* gt2k_menu::Item */
#include "game/menu/framework/T220_object.h"               /* gt2k_menu::Object (operator new / delete) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuLayoutItem */
#include "game/menu/framework/T261_menu_draw_state.h"      /* g_00352758 (the screen's GSBuffer) */
#include "game/race/core/T414_race_random.h"          /* raceRandomFloat */
#include "game/save/T420_license_records.h"      /* LICENSE_B .. LICENSE_R */
#include "engine/gfx/T512_render_context.h"       /* g_002F4D68 (the render context's depth buffer) */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "game/menu/gtmode/trophy_internal.h"

using namespace gt2k_menu;

/* ------------------------------------------------------------------------ */
/* Data                                                                     */
/* ------------------------------------------------------------------------ */

/* The licence trophies' and the championship cups' model names (and the
 * cups' weights): zero in the file, filled by T058_static_init (the original
 * built their elements with inline constructors at run time, so egcs wrote
 * the tables as zeros where they were defined, before the jitter tables). */
const char *g_0029ED18[6] = { NULL };       /* 0x0029ED18 */
TrophyModelEntry g_0029ED30[10] = { { NULL, 0.0f } };  /* 0x0029ED30 */

/* The sub-pixel jitter of draw_trophy_ring's passes (pixels), for 4 .. 7
 * passes. */
TrophyPos g_0029ED80[4] = {                 /* 0x0029ED80 */
    { 0.375f, 0.25f }, { 0.125f, 0.75f }, { 0.875f, 0.25f }, { 0.625f, 0.75f },
};
TrophyPos g_0029EDA0[5] = {                 /* 0x0029EDA0 */
    { 0.5f, 0.5f }, { 0.3f, 0.1f }, { 0.7f, 0.9f }, { 0.9f, 0.3f }, { 0.1f, 0.7f },
};
TrophyPos g_0029EDC8[6] = {                 /* 0x0029EDC8 */
    { 0.464646f, 0.464646f }, { 0.131313f, 0.797979f }, { 0.535353f, 0.868686f },
    { 0.868686f, 0.535353f }, { 0.797979f, 0.131313f }, { 0.20202f, 0.20202f },
};
TrophyPos g_0029EDF8[7] = {                 /* 0x0029EDF8 */
    { 0.5f, 0.5f }, { 0.326795f, 0.6f }, { 0.5f, 0.3f }, { 0.673205f, 0.6f },
    { 0.1f, 0.5f }, { 0.7f, 0.15359f }, { 0.7f, 0.84641f },
};

namespace gt2k_menu {

/* ------------------------------------------------------------------------ */
/* Prize model names                                                        */
/* ------------------------------------------------------------------------ */

/* rate plus the bonus if it is positive. */
// FUNC 0x00138FC8 gt2k_menu::TrophyModelEntry::getRate
f32 TrophyModelEntry::getRate(f32 bonus)
{
    f32 none = 0.0f;
    if (none < bonus)
        return rate + bonus;
    return rate + none;
}

/* The sum of the ten cups' weights (the loop counts down from 9 in the
 * binary, over the entries in address order). */
// FUNC 0x00138FF0 gt2k_menu::trophy_model_total_rate
f32 trophy_model_total_rate(f32 bonus)
{
    f32 sum = 0.0f;
    for (int i = 0; i < 10; i++)
        sum += g_0029ED30[i].getRate(bonus);
    return sum;
}

/* Licence trophy: index = licence for 0..5, 0 otherwise (the identity on
 * LICENSE_B .. LICENSE_R).  The jump table 0x00317FB0 has six entries, case
 * 0 sharing the default's code (cross-jumped): case LICENSE_B is written
 * out. */
// FUNC 0x00139060 gt2k_menu::PrizeModelSpec::setLicense
void PrizeModelSpec::setLicense(u32 licence)
{
    kind = PRIZE_MODEL_LICENSE;
    switch (licence) {
    case LICENSE_B:
        index = 0;
        break;
    case LICENSE_A:
        index = 1;
        break;
    case LICENSE_IB:
        index = 2;
        break;
    case LICENSE_IA:
        index = 3;
        break;
    case LICENSE_S:
        index = 4;
        break;
    case LICENSE_R:
        index = 5;
        break;
    default:
        index = 0;
        break;
    }
}

/* Championship cup n for n < 10.  Otherwise a random cup: a point taken
 * uniformly (raceRandomFloat(1), 0..1) in the sum of the weights rate +
 * 165.333 * completion^3, so the rare cups (champ_04 rate 2, champ_06 rate
 * 1) get likelier as the game is completed; 0 if the point is past the
 * last cup (rounding). */
// FUNC 0x001390D0 gt2k_menu::PrizeModelSpec::setChampionship
void PrizeModelSpec::setChampionship(u32 n)
{
    kind = PRIZE_MODEL_CUP;
    if (n < 10) {
        index = n;
        return;
    }
    f32 acc = 0.0f;
    f32 completion = getGameStatus()->getCompletionRatio();
    f32 bonus = completion * 165.333f * completion * completion;
    f32 point = trophy_model_total_rate(bonus) * raceRandomFloat(1);
    for (int i = 0; i < 10; i++) {
        acc += g_0029ED30[i].getRate(bonus);
        if (point <= acc) {
            index = i;
            return;
        }
    }
    index = 0;
}

/* "menu/<name>.prm"; the name is NULL for a kind other than 0 / 1 (kind is
 * read as a signed byte).  The tests are the inline isLicense /
 * isChampionship (game/menu/gtmode/T058_trophy_item.h): retail tests their flags
 * (bnez on kind, then xori kind, 1 / bnel); comparing kind in place gives
 * li 1 / bnel. */
// FUNC 0x001391A8 gt2k_menu::PrizeModelSpec::getPath
void PrizeModelSpec::getPath(char *buf)
{
    const char *name = NULL;
    if (isLicense())
        name = g_0029ED18[index];
    else if (isChampionship())
        name = g_0029ED30[index].name;
    sprintf(buf, "menu/%s.prm", name);
}

/* ------------------------------------------------------------------------ */
/* TrophyItem                                                               */
/* ------------------------------------------------------------------------ */

/* Item::Item(NULL, NULL) (no layout item, no texture), then the vptr.  Shows
 * the prize at once; start / end are the fade-in frames (60 a second,
 * negative times count as 0; cvt.w.s truncates). */
// FUNC 0x00139218 gt2k_menu::TrophyItem::TrophyItem
TrophyItem::TrophyItem(PrizeBase *p, int n, f32 delay, f32 fade)
    : Item()
{
    count = n;
    prize = p;
    alpha = 0.0f;
    frame = 0;
    state = TROPHY_ITEM_WAITING;
    if (p != NULL)
        p->show();
    if (delay < 0.0f)
        delay = 0.0f;
    if (fade < 0.0f)
        fade = 0.0f;
    fade = fade + delay;
    start = (int)(delay * 60.0f);
    end = (int)(fade * 60.0f);
}

/* update (slot 1 of both vtables; index.txt names it
 * ChampionShipTrophyItem::virtual_1).  frame is compared unsigned with
 * start / end; state is read signed. */
// FUNC 0x00139318 gt2k_menu::TrophyItem::virtual_1
void TrophyItem::virtual_1()
{
    /* setAlpha is expanded, with its double-precision lower bound (fptodp,
     * then dpcmp against 0.0) */
    /* implicit: fptodp, dpcmp (the inline setAlpha: double-precision compare) */
    switch (state) {
    case TROPHY_ITEM_WAITING:
        if (frame >= (u32)start)
            state = TROPHY_ITEM_FADING_IN;
        break;
    case TROPHY_ITEM_FADING_IN:
        setAlpha(alpha + 1.0f / (f32)(end - start + 1));
        if (frame >= (u32)end)
            state = TROPHY_ITEM_SHOWN;
        break;
    case TROPHY_ITEM_SHOWN:
        setAlpha(1.0f);
        break;
    }
    if (state != TROPHY_ITEM_WAITING && prize != NULL)
        prize->update();
    frame++;
    Item::virtual_1();
}

/* The prize model drawn `count' times (clamped to 4..7) into a copy of the
 * screen and averaged: pass i turns the eye by i * 360 / count degrees on a
 * circle of radius eyeDelta (only when focus > 1: depth of field, the plane
 * at the focus distance fixed by prize_projection_eye) and offsets the
 * drawing by jitter point i (sub-pixel) plus (x, y) pixels; the pass is
 * mixed into the accumulation buffer with weight 1 / (i + 1) (a running
 * mean), and the mean is mixed over the screen with alpha at the end.  The
 * GSBuffers are destroyed on the way out (GSBuffer::~GSBuffer, flags 2; the
 * landing pads do the same).  The depth buffer's address is taken before
 * the loop (retail: lui / addiu into $fp ahead of the loop, not the
 * member's offset inside it), and cb comes after the floats in the
 * parameter list (the order of the parameter copies gives cb $s7, the
 * buffer $fp, as in retail). */
// FUNC 0x001394A0 gt2k_menu::draw_trophy_ring
void draw_trophy_ring(PrizeBase *prize, int count, f32 alpha, f32 focus, f32 eyeDelta,
                      CallBackWhenPrizeRendering *cb, f32 x, f32 y)
{
    const TrophyPos *jitter = NULL;
    if (count < 4)
        count = 4;
    else if (count >= 8)
        count = 7;
    switch (count) {
    case 4:
        jitter = g_0029ED80;
        break;
    case 5:
        jitter = g_0029EDA0;
        break;
    case 6:
        jitter = g_0029EDC8;
        break;
    case 7:
        jitter = g_0029EDF8;
        break;
    }
    int depthOfField = 1;
    if (!(1.0f < focus))
        depthOfField = 0;
    f32 step = 360.0f / (f32)count;

    pgl_push_attrib(PGL_ATTR_DRAW_BUFFER | PGL_ATTR_DEPTH | PGL_ATTR_TARGET_B |
                    PGL_ATTR_VIEWPORT | PGL_ATTR_SCISSOR);     /* 0x887 */
    gfx_enable(PGL_CAP_COLOR_CLAMP);
    gfx_disable(PGL_CAP_FLAG24);
    gfx_depth_mask(1);
    gfx_color_mask(1, 1, 1, 1);
    gfx_disable(PGL_CAP_FOG);
    GSBuffer *screen = g_00352758;
    GSBuffer pass(screen, 0);               /* sp+0x00: like the screen, psm 0, own VRAM */
    GSBuffer accum(screen, 1);              /* sp+0x40: psm 1 */
    GSBuffer *zbuf = &g_002F4D68;           /* $fp: the whole address before the loop */
    for (int i = 0; i < count; i++) {
        f32 s, c;                           /* sp+0x80, sp+0x84 */
        pgl_matrix_mode(PGL_MTX_PROJECTION);
        pgl_push_matrix();
        pgl_sincos_deg(step * (f32)i, &s, &c);
        f32 eyeX = c * eyeDelta;
        f32 eyeY = s * eyeDelta;
        vu0_load_identity_regs();
        if (depthOfField)
            prize_projection_eye(eyeX, eyeY, focus);
        else
            prize_projection(0.0f, 0.0f);
        pass.blit(screen, 0, 0);            /* the screen so far, nearest filtering */
        pass.setDrawBuffer();
        zbuf->setZBuffer();
        gfx_offset_xy(jitter[i].x + x, jitter[i].y + y);
        pgl_matrix_mode(PGL_MTX_MODELVIEW);
        pgl_push_matrix();
        vu0_load_identity_regs();
        if (depthOfField)
            prize_eye_translate(eyeX, eyeY);
        if (prize != NULL) {
            gs_clear_buffers(PGL_CLEAR_DEPTH);
            prize->draw(cb);
        }
        pgl_pop_matrix();
        accum.setTargetB();                 /* target B */
        gfx_frame_effect(PGL_FX_MIX_A_INTO_B, 1.0f / (f32)(i + 1));
        pgl_matrix_mode(PGL_MTX_PROJECTION);
        pgl_pop_matrix();
    }
    screen->setTargetB();
    accum.setDrawBuffer();
    gfx_frame_effect(PGL_FX_MIX_A_INTO_B, alpha);
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    pgl_pop_attrib();
}

} /* namespace gt2k_menu */

/* ------------------------------------------------------------------------ */
/* "render_model" factories (MenuHooks; no direct callers)                  */
/* ------------------------------------------------------------------------ */
/* Only ctx (its prize at +0xDE8) and the layout item's z are used.
 * Object::operator new(0x11C); Object::operator delete if the constructor
 * throws.  The prize's address is taken before the new (retail keeps it in
 * $s0 from the prologue; as an argument expression it is computed after
 * the call). */

/* LicenseGetControl's (T059_static_init): 6 passes, 2 s, 1 s. */
// FUNC 0x00139870 create_trophy_license
Transitive *create_trophy_license(void *ctx, PageLayout *layout, Object *parent,
                                  MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    PrizeControlBase *screen = (PrizeControlBase *)ctx;
    PrizeBase *prize = &screen->prize;      /* $s0, before operator new */
    TrophyItem *trophy = new TrophyItem(prize, 6, 2.0f, 1.0f); /* implicit: Object::operator delete (if the constructor throws) */
    trophy->z = item->z;
    return trophy;
}

/* No callers (no table has it): 4 passes, 3 s, 1.5 s, the times of the
 * championship one; ctx is taken as a PrizeControlBase (? the screen type:
 * the prize is at +0xDE8 in both bases). */
// FUNC 0x00139908 create_trophy_unused
Transitive *create_trophy_unused(void *ctx, PageLayout *layout, Object *parent,
                                 MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    PrizeControlBase *screen = (PrizeControlBase *)ctx;
    PrizeBase *prize = &screen->prize;      /* $s0, before operator new */
    TrophyItem *trophy = new TrophyItem(prize, 4, 3.0f, 1.5f); /* implicit: Object::operator delete (if the constructor throws) */
    trophy->z = item->z;
    return trophy;
}

/* MiniPrizeControlBase's, the arcade trophy (T104_static_init): 6, 1 s,
 * 2 s. */
// FUNC 0x001399A0 create_trophy_arcade
Transitive *create_trophy_arcade(void *ctx, PageLayout *layout, Object *parent,
                                 MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    MiniPrizeControlBase *screen = (MiniPrizeControlBase *)ctx;
    PrizeBase *prize = &screen->prize;      /* $s0, before operator new */
    TrophyItem *trophy = new TrophyItem(prize, 6, 1.0f, 2.0f); /* implicit: Object::operator delete (if the constructor throws) */
    trophy->z = item->z;
    return trophy;
}

/* GTmodeTrophyControl's (T056_static_init). */
// FUNC 0x00139A38 create_trophy_gtmode
Transitive *create_trophy_gtmode(void *ctx, PageLayout *layout, Object *parent,
                                 MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    MiniPrizeControlBase *screen = (MiniPrizeControlBase *)ctx;
    PrizeBase *prize = &screen->prize;      /* $s0, before operator new */
    TrophyItem *trophy = new TrophyItem(prize, 6, 1.0f, 2.0f); /* implicit: Object::operator delete (if the constructor throws) */
    trophy->z = item->z;
    return trophy;
}

/* ConsolationControl's (T057_static_init): 7 passes. */
// FUNC 0x00139AD0 create_trophy_consolation
Transitive *create_trophy_consolation(void *ctx, PageLayout *layout, Object *parent,
                                      MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    MiniPrizeControlBase *screen = (MiniPrizeControlBase *)ctx;
    PrizeBase *prize = &screen->prize;      /* $s0, before operator new */
    TrophyItem *trophy = new TrophyItem(prize, 7, 1.0f, 2.0f); /* implicit: Object::operator delete (if the constructor throws) */
    trophy->z = item->z;
    return trophy;
}

namespace gt2k_menu {

/* ------------------------------------------------------------------------ */
/* ChampionShipTrophyItem                                                   */
/* ------------------------------------------------------------------------ */

// FUNC 0x00139B68 gt2k_menu::ChampionShipTrophyItem::ChampionShipTrophyItem
ChampionShipTrophyItem::ChampionShipTrophyItem(PrizeBase *p, int n, f32 delay, f32 fade)
    : TrophyItem(p, n, delay, fade)
{
}

/* draw (index.txt: ChampionShipTrophyItem::virtual_2): TrophyItem's with the
 * drawing offset +160 pixels in x.  Not exact (76.0%, as TrophyItem's copy
 * 0x00139F58, 67.7%): retail reloads prize->model for getEyeDelta and
 * prize for the first argument after the two accessors' conditional
 * blocks, i.e. CSE2 did not carry the equivalences around the first
 * accessor's `if (model)' (ours does: -fcse-skip-blocks; compiled with
 * -fno-cse-skip-blocks the two reloads appear, 85.0% / 84.4%, and only the
 * argument loads' order is left: retail sets $a1, $a0, $f12, then $a2 = 0
 * in the jal slot, with `this' in $a2).  Tried: the accessors as if /
 * return, if / else, a local result, `model == 0 ?', `!model ?'; the
 * accessors' results in locals; inline helpers taking the item, the prize
 * or x / y; an early return; a local copy of prize; the callback as a
 * local; every placement of cb in draw_trophy_ring's parameters; the
 * reference compiler 991111-01 (the same code). */
// FUNC 0x00139BA0 gt2k_menu::ChampionShipTrophyItem::virtual_2
void ChampionShipTrophyItem::virtual_2()
{
    if (prize != NULL)
        draw_trophy_ring(prize, count, alpha, prize->getFocus(), prize->getEyeDelta(), NULL,
                         160.0f, 0.0f);
}

} /* namespace gt2k_menu */

/* TrophyGetControl's (T060 static initialiser 0x0013B088): 4 passes, 3 s,
 * 1.5 s. */
// FUNC 0x00139C08 create_trophy_championship
Transitive *create_trophy_championship(void *ctx, PageLayout *layout, Object *parent,
                                       MenuLayoutItem *item, int p8, int pC, int p10, int p14)
{
    PrizeControlBase *screen = (PrizeControlBase *)ctx;
    PrizeBase *prize = &screen->prize;      /* $s0, before operator new */
    ChampionShipTrophyItem *trophy = new ChampionShipTrophyItem(prize, 4, 3.0f, 1.5f); /* implicit: Object::operator delete (if the constructor throws) */
    trophy->z = item->z;
    return trophy;
}

/* One element of each table as egcs's build_vec_init expands it (see
 * T058_static_init): the inline constructor on a copy of the cursor, then
 * the cursor and the count of the elements left stepped. */
static inline void set_licence_name(const char **e, const char *name)
{
    *e = name;
}
static inline void set_cup(TrophyModelEntry *e, const char *name, f32 rate)
{
    e->name = name;
    e->rate = rate;
}
#define LICENCE_NAME(name)                                                    \
    { const char **e = lic; set_licence_name(e, name); }                      \
    lic++;                                                                    \
    licLeft--
#define CUP(name, rate)                                                       \
    { TrophyModelEntry *e = cup; set_cup(e, name, rate); }                    \
    cup++;                                                                    \
    cupLeft--

/* Static initialiser (.ctors entry 0x0028F070): the licence trophy names
 * and the cup table (name and weight).  Compiler-generated in the original:
 * both tables were defined here with elements built by inline constructors
 * (their .data, 0x0029ED18 and 0x0029ED30, is zero in the file), which egcs
 * builds at run time (cp/init.c build_vec_init), the licence names first
 * (sched1 interleaves the two runs of stores).  Written out the way
 * build_vec_init expands it (a walking cursor, a copy of it per element,
 * the dead count of the elements left): an exact form, not the original; it
 * stores the same 6 + 10 entries. */
// FUNC 0x00139CA0 T058_static_init
void T058_static_init(void)
{
    {
        const char **lic = g_0029ED18;
        int licLeft = 5;                    /* build_vec_init's iterator: elements - 1 */
        LICENCE_NAME("lic_b");              /* LICENSE_B */
        LICENCE_NAME("lic_a");              /* LICENSE_A */
        LICENCE_NAME("lic_ib");             /* LICENSE_IB */
        LICENCE_NAME("lic_ia");             /* LICENSE_IA */
        LICENCE_NAME("lic_s");              /* LICENSE_S */
        LICENCE_NAME("lic_d");              /* LICENSE_R */
    }
    {
        TrophyModelEntry *cup = g_0029ED30;
        int cupLeft = 9;                    /* build_vec_init's iterator: elements - 1 */
        CUP("champ_00", 400.0f);
        CUP("champ_01", 350.0f);
        CUP("champ_02", 250.0f);
        CUP("champ_03", 450.0f);
        CUP("champ_04", 2.0f);
        CUP("champ_05", 160.0f);
        CUP("champ_06", 1.0f);
        CUP("champ_07", 500.0f);
        CUP("champ_08", 25.0f);
        CUP("champ_09", 120.0f);
    }
}

#undef LICENCE_NAME
#undef CUP

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base TrophyItem). */
// FUNC 0x00139E20 __tfQ29gt2k_menu22ChampionShipTrophyItem
extern "C" void *__tfQ29gt2k_menu22ChampionShipTrophyItem(void)
{
    if (__tiQ29gt2k_menu22ChampionShipTrophyItem[0] == 0) {
        __tfQ29gt2k_menu10TrophyItem();
        __rtti_si(__tiQ29gt2k_menu22ChampionShipTrophyItem, "Q29gt2k_menu22ChampionShipTrophyItem",
                  (const type_info *)__tiQ29gt2k_menu10TrophyItem);
    }
    return __tiQ29gt2k_menu22ChampionShipTrophyItem;
}
#endif

// FUNC 0x00139E70 gt2k_menu::ChampionShipTrophyItem::~ChampionShipTrophyItem
/* the implicit destructor (game/menu/gtmode/T058_trophy_item.h), no vptr store of its
 * own: TrophyItem's inline destructor expanded (TrophyItem's vptr
 * 0x003180C0, PrizeBase::hide and the virtual unload (slot 3) of the prize),
 * then Item::~Item(this, 0) and Object::operator delete (0x001BF458) if
 * flags & 1 */
/* implicit: hide (the inline ~TrophyItem), ~Item (base destructor), operator delete (deleting destructor) */

// FUNC 0x00139EF0 gt2k_menu::TrophyItem::getAlpha
/* inline in game/menu/gtmode/T058_trophy_item.h; egcs emits this copy (no callers) */

// FUNC 0x00139EF8 gt2k_menu::TrophyItem::setAlpha
/* inline in game/menu/gtmode/T058_trophy_item.h; egcs emits this copy (no callers;
 * virtual_1 expands it): the lower bound in double precision (fptodp,
 * dpcmp against 0.0), the upper in float */
/* implicit: fptodp, dpcmp (double-precision compare in the inline setAlpha) */

// FUNC 0x00139F58 gt2k_menu::TrophyItem::virtual_2
/* inline in game/menu/gtmode/T058_trophy_item.h; egcs emits this copy for the vtable
 * (draw: draw_trophy_ring at the item's place, offset 0, 0, with the
 * prize's getFocus / getEyeDelta expanded; Item's draw is not called).
 * 67.7%: see ChampionShipTrophyItem::virtual_2 (the same reloads). */
/* implicit: draw_trophy_ring (the inline virtual_2 in the header) */

// FUNC 0x00139FB8 gt2k_menu::TrophyItem::~TrophyItem
/* inline in game/menu/gtmode/T058_trophy_item.h; egcs emits this copy for the vtable:
 * the vptr, then PrizeBase::hide and the virtual unload (slot 3) of the
 * prize, then Item::~Item(this, 0) and Object::operator delete (0x001BF458)
 * if flags & 1 */
/* implicit: hide (the inline ~TrophyItem in the header), ~Item (base destructor), operator delete (deleting destructor) */

#ifndef BOOT_OLD_GCC
/* Compiler-generated typeinfo function (__si_type_info, base Item). */
// FUNC 0x0013A038 __tfQ29gt2k_menu10TrophyItem
extern "C" void *__tfQ29gt2k_menu10TrophyItem(void)
{
    if (__tiQ29gt2k_menu10TrophyItem[0] == 0) {
        __tfQ29gt2k_menu4Item();
        __rtti_si(__tiQ29gt2k_menu10TrophyItem, "Q29gt2k_menu10TrophyItem",
                  (const type_info *)__tiQ29gt2k_menu4Item);
    }
    return __tiQ29gt2k_menu10TrophyItem;
}
#endif
