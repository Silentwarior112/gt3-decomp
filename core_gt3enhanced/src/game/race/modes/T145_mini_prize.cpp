/* T145 - MiniPrize and the trophy model helpers (u048), 0x0018A0E8..0x0018A798.
 *
 * MiniPrize is the PrizeBase of the small trophy screens (the arcade / GT
 * mode trophies, the consolation prize, the licence prizes via
 * LicensePrize): it loads "race/<type prefix><rank file>" (e.g.
 * "race/a_easy_gold.prm", or "race/consolation.prm") unless that trophy is
 * already loaded, and draws it antialiased: two renders at sub-pixel
 * offsets, averaged, then blended over the target.  The rank table (gold /
 * silver / bronze, GOLD / SILVER / BRONZE) is built by this unit's static
 * initialiser (egcs's _GLOBAL_$I$: the table has a constructor), emitted
 * after the functions; then the out-of-line copies of MiniPrize's inline
 * members.
 *
 * Includes (u048's data pass): retail's T145 saw no race class and no <new>.
 * Its .rodata (0x0032C5B0..0x0032C6CE) is the prefix table's long strings
 * ("a_normal_" .. "g_special_"), "NO PRIZE", the functions' literals, the
 * rank table's ("gold.prm" .. "bronze.prm": written with the static
 * initialiser, after the functions), the vtable, then the type names
 * MiniPrize, CallBackWhenPrizeRendering, PrizeModelSetContext.  Its .sdata
 * (0x00351EE0..0x00351F0C) is the prefix table's short strings ("" ..
 * "g_hard_"), then g_00351F08.
 */
// DATA_START 0x0032C5B0
// DATA_START 0x00351EE0
#include "types.h"
#ifndef BOOT_OLD_GCC
#include "cxxrt.h"                          /* type_info (GCC 14's __tf function) */
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#include "game/race/modes/T145_mini_prize.h"           /* own: MiniPrize, PrizeId; + T460 PrizeBase,
                                               PrizeModelSetContext */
#include "game/menu/gtmode/T459_prize_model.h"          /* prize_model_step / prize_model_loop, prize_projection */
#include "game/data/messages/T186_msg_race_display.h"     /* g_002C43E8, RACEDISPLAY_PRIZE_* */
#include "engine/pdistd/T534_sprintf.h"              /* sprintf */
#include "engine/gfx/T512_render_context.h"       /* g_002F4230: bufB38 (g_002F4D68) */
#include "engine/gfx/T514_gs_buffer.h"            /* GSBuffer */
#include "engine/pgl/G001_flare.h"                /* gfx_*, pgl_*, gs_*, vu0_load_identity_regs, PGL_* */
#include "game/race/modes/race_time_trial_internal.h"

/* The ranks of PrizeId (and of the rank table). */
enum { PRIZE_GOLD = 0, PRIZE_SILVER = 1, PRIZE_BRONZE = 2, PRIZE_RANKS = 3 };

/* The GSBuffer pixel formats of drawAntialiased's two work buffers. */
enum { GS_PSMCT32 = 0, GS_PSMCT24 = 1 };

/* ------------------------------------------------------------------------ */
/* The trophy tables (defined here in retail's order)                       */
/* ------------------------------------------------------------------------ */

/* The model prefix of each PrizeId type (.data; the strings of 8 bytes or
 * less in .sdata from 0x00351EE0, the others in .rodata). */
const char *g_002B9698[9] = {                /* 0x002B9698 */
    "", "a_easy_", "a_normal_", "a_hard_", "a_special_",
    "g_easy_", "g_normal_", "g_hard_", "g_special_"
};

/* gold, silver, bronze: built at start-up by the unit's static initialiser
 * (a table of objects with a constructor; .data stays zero). */
PrizeRankEntry g_002B96C0[3] = {             /* 0x002B96C0 */
    PrizeRankEntry("gold.prm", RACEDISPLAY_PRIZE_GOLD),
    PrizeRankEntry("silver.prm", RACEDISPLAY_PRIZE_SILVER),
    PrizeRankEntry("bronze.prm", RACEDISPLAY_PRIZE_BRONZE)
};

/* The sub-pixel offsets of drawAntialiased's two passes. */
PrizeJitter g_002B96D8[2] = {                /* 0x002B96D8 */
    { 0.25f, 0.75f },
    { 0.75f, 0.25f }
};

/* prize_rank_name's text for no rank (.sdata, gp-0x5AE8). */
const char *g_00351F08 = "NO PRIZE";        /* 0x00351F08 */

// FUNC 0x0018A0E8 PrizeRankEntry::get
//   The model file suffix, or (localized != 0) its RACEDISPLAY text.
const char *PrizeRankEntry::get(int localized) const
{
    if (localized == 0)
        return file;
    return g_002C43E8[msgId];
}

// FUNC 0x0018A110 prize_rank_name
//   "GOLD" / "SILVER" / "BRONZE" for ranks 0..2 (an unsigned test), else
//   g_00351F08 ("NO PRIZE").  Also called by LicenseExaminationMessages
//   (T146).
const char *prize_rank_name(int rank)
{
    if ((u32)rank >= PRIZE_RANKS)
        return g_00351F08;
    return g_002B96C0[rank].get(1);
}

// FUNC 0x0018A150 prize_model_path
//   "race/consolation.prm" when the type or the rank is negative, else
//   "race/<type prefix><rank file>".
void prize_model_path(char *buf, const PrizeId *id)
{
    int type = id->type;
    if (type < 0 || id->rank < 0)
        sprintf(buf, "race/consolation.prm");
    else
        sprintf(buf, "race/%s%s", g_002B9698[type], g_002B96C0[id->rank].get(0));
}

// FUNC 0x0018A1F0 MiniPrize::MiniPrize
//   PrizeBase (0x00229C80), its vptr (+0x90), nothing loaded.  Also called
//   by trophy_prize_screen and LicensePrize's constructor.
MiniPrize::MiniPrize()
{
    /* implicit: PrizeBase (the base's / members' constructors) */
    loaded.type = -1;
    loaded.rank = -1;
}

// FUNC 0x0018A230 MiniPrize::~MiniPrize
//   (index.txt: virtual_1) The vptr, then ~PrizeBase with the caller's
//   flags (which deletes).
MiniPrize::~MiniPrize()
{
    /* implicit: ~PrizeBase (the base / member destructors) */
}

// FUNC 0x0018A258 MiniPrize::unload
//   (index.txt: virtual_3) PrizeBase's, then nothing is loaded (copied from
//   a {-1, -1} temporary with an unaligned 8-byte copy).
void MiniPrize::unload()
{
    PrizeBase::unload();
    PrizeId none;                          /* sp+0x00 */
    none.type = -1;
    none.rank = -1;
    loaded = none;
}

// FUNC 0x0018A298 MiniPrize::loadPrize
//   Loads trophy id (into buf, size bytes; keep: buf is the caller's)
//   unless it is the one loaded.  Nothing at all for a negative type (the
//   consolation path of prize_model_path is then never taken from here).
//   PrizeBase::load is called directly, keep passed on unmasked.
void MiniPrize::loadPrize(const PrizeId *id, void *buf, int size, int keep)
{
    char path[0x80];                       /* sp+0x00 */

    if (id->type < 0)
        return;
    if (loaded == *id)
        return;
    prize_model_path(path, id);
    PrizeBase::load(path, buf, size, keep);
    loaded = *id;                          /* an unaligned 8-byte copy (ldl / ldr) */
}

// FUNC 0x0018A350 MiniPrize::restartAnimation
//   ? name (no callers).
void MiniPrize::restartAnimation()
{
    if (model != 0 && updateFlag == 1)
        prize_model_loop(model, &context);
}

// FUNC 0x0018A388 MiniPrize::render
//   The virtual draw (slot 4).  Called by LicensePrize::render (T146).
void MiniPrize::render(GSBuffer &buf, f32 alpha)
{
    virtual_4(buf, alpha);
}

// FUNC 0x0018A3B0 MiniPrize::virtual_4
//   Draws the prize into buf (antialiased, with the context).
void MiniPrize::virtual_4(GSBuffer &buf, f32 alpha)
{
    drawAntialiased(buf, &context, alpha);
}

// FUNC 0x0018A3D0 MiniPrize::drawAntialiased
//   ? name.  When the model is shown (dispFlag == 1): the attributes saved,
//   the prize projection; two passes, each a copy of buf (blit) drawn into
//   as the draw buffer with the shared Z buffer, the model drawn at the
//   sub-pixel offset g_002B96D8[i] and mixed into the accumulation buffer
//   with weight 1 / (i + 1) (the second pass gives the mean of the two);
//   then the accumulation buffer mixed over buf with alpha.  ctx is not
//   used.  (The two GSBuffers are destroyed on unwinding too.)
void MiniPrize::drawAntialiased(GSBuffer &buf, PrizeModelSetContext *ctx, f32 alpha)
{
    if (model == NULL || dispFlag != 1)
        return;

    pgl_push_attrib(PGL_ATTR_DRAW_BUFFER | PGL_ATTR_DEPTH | PGL_ATTR_TARGET_B |
                    PGL_ATTR_VIEWPORT | PGL_ATTR_SCISSOR);  /* 0x887 */
    gfx_enable(PGL_CAP_COLOR_CLAMP);
    gfx_disable(PGL_CAP_FLAG24);
    gfx_depth_mask(1);
    gfx_color_mask(1, 1, 1, 1);
    gfx_disable(PGL_CAP_FOG);
    GSBuffer work(&buf, GS_PSMCT32);       /* sp+0x00 */
    GSBuffer accum(&buf, GS_PSMCT24);      /* sp+0x40 */
    GSBuffer *zbuf = &g_002F4230.bufB38;   /* g_002F4D68 (retail: in $s6 from here) */
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_push_matrix();
    vu0_load_identity_regs();
    prize_projection(0.0f, 0.0f);
    for (int pass = 0; pass < 2; pass++) {
        work.blit(&buf, PGL_FILTER_NEAREST, PGL_FILTER_NEAREST);
        work.setDrawBuffer();
        zbuf->setZBuffer();
        gfx_offset_xy(g_002B96D8[pass].x, g_002B96D8[pass].y);
        pgl_matrix_mode(PGL_MTX_MODELVIEW);
        pgl_push_matrix();
        vu0_load_identity_regs();
        gs_clear_buffers(PGL_CLEAR_DEPTH);
        draw(NULL);
        pgl_pop_matrix();
        accum.setTargetB();                /* target B */
        gfx_frame_effect(PGL_FX_MIX_A_INTO_B, 1.0f / (f32)(pass + 1));
    }
    pgl_matrix_mode(PGL_MTX_PROJECTION);
    pgl_pop_matrix();
    buf.setTargetB();                      /* target B */
    accum.setDrawBuffer();
    gfx_frame_effect(PGL_FX_MIX_A_INTO_B, alpha);
    gfx_disable(PGL_CAP_CULL_FACE);
    gfx_disable(PGL_CAP_ALPHA_TEST);
    pgl_pop_attrib();
}

// FUNC 0x0018A640 mini_prize_static_init (_GLOBAL_$I$g_002B9698)
/* implicit: PrizeRankEntry (the static initialiser of g_002B96C0) */

// FUNC 0x0018A690 MiniPrize::getLoaded
/* inline in game/race/modes/T145_mini_prize.h; egcs emits this copy (no callers):
 * &loaded */

// FUNC 0x0018A698 MiniPrize::setup
/* inline in game/race/modes/T145_mini_prize.h; egcs emits this copy (no callers; u019's
 * matching pass made it inline: the screens' load functions expand it): the
 * display / update flags, then loadPrize */
/* implicit: loadPrize (the inline body's copy) */

// FUNC 0x0018A6C8 MiniPrize::getRankName
/* inline in game/race/modes/T145_mini_prize.h; egcs emits this copy (no callers):
 * prize_rank_name(id->rank) */
/* implicit: prize_rank_name (the inline body's copy) */

// FUNC 0x0018A6E8 MiniPrize::resetAnimation
/* inline in game/race/modes/T145_mini_prize.h; egcs emits this copy (no callers):
 * prize_model_loop(model, ctx) when there is a model */
/* implicit: prize_model_loop (the inline body's copy) */

// FUNC 0x0018A710 MiniPrize::stepAnimation
/* inline in game/race/modes/T145_mini_prize.h; egcs emits this copy (no callers):
 * prize_model_step(model, ctx) when there is a model */
/* implicit: prize_model_step (the inline body's copy) */

// FUNC 0x0018A738 MiniPrize::getContext
/* inline in game/race/modes/T145_mini_prize.h; egcs emits this copy (no callers):
 * &context */

// FUNC 0x0018A740 MiniPrize::getContextConst
/* inline in game/race/modes/T145_mini_prize.h; egcs emits this copy (no callers): the
 * same code */

#ifndef BOOT_OLD_GCC
// FUNC 0x0018A748 __tf9MiniPrize
//   Builds the __si_type_info __ti9MiniPrize (base PrizeBase, gp-0x4590) on
//   the first call.  Also called by __tf12LicensePrize.
extern "C" const type_info &__tf9MiniPrize(void)
{
    if (__ti9MiniPrize[0] == 0) {
        __tf9PrizeBase();
        __rtti_si(__ti9MiniPrize, "9MiniPrize", (const type_info *)__ti9PrizeBase);
    }
    return *(const type_info *)__ti9MiniPrize;
}
#endif
