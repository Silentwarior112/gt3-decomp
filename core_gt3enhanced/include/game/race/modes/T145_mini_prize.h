/* T145_mini_prize.h - MiniPrize, the trophy model of the small trophy
 * screens, and PrizeId (translation unit T145, unit u048),
 * 0x0018A0E8..0x0018A798.
 *
 * MiniPrize is the PrizeBase (T460) of the arcade / GT mode trophies, the
 * consolation prize and the licence prizes (LicensePrize, T146): it loads
 * "race/<type prefix><rank file>" (e.g. "race/a_easy_gold.prm"; the
 * prefixes g_002B9698, the rank table g_002B96C0 {"gold.prm", GOLD} ...
 * built by the unit's static initialiser) unless that trophy is already
 * loaded, and draws it antialiased (two passes at the sub-pixel offsets
 * g_002B96D8, blended 1/1 then 1/2, then over the buffer with alpha).
 *
 * The out-of-line copies after T145's static initialiser (getLoaded ..
 * getContextConst, 0x0018A690..0x0018A740) are MiniPrize's inline members
 * (u048 matching: they were PrizeBase's before, and T460 then emitted
 * copies retail lacks). */
#ifndef GAME_T145_MINI_PRIZE_H
#define GAME_T145_MINI_PRIZE_H

#include "game/game.h"
#include "game/menu/gtmode/T460_prize_base.h"               /* base PrizeBase, PrizeModelSetContext */

class GSBuffer;                                 /* T514 (references only) */
class type_info;                                /* T548 */

#ifndef BOOT_OLD_GCC                            /* egcs generates it itself */
/* (__si_type_info 0x004B2070, base PrizeBase 0x00353460 gp-0x4590) */
extern "C" const type_info &__tf9MiniPrize(void);  /* 0x0018A748 */
#endif

/* (? name, u048) The trophy of a mini prize: its type (1..8, the race
 * kind: the index of its model prefix in g_002B9698; -1 none) and its
 * rank (0 gold, 1 silver, 2 bronze; -1 none).  The first 8 bytes of u019's
 * MiniPrizeInfo.  Not polymorphic, 8 bytes. */
struct PrizeId {
    /* Inline (? the original's): MiniPrize::loadPrize's `loaded == *id'
     * (retail computes the flag with xor / sltiu, `this' = &loaded in a
     * register). */
    bool operator==(const PrizeId &o) const { return type == o.type && rank == o.rank; }

    int type;                                   /* 0x0 */
    int rank;                                   /* 0x4 */
};
EGCS_SIZE(PrizeId, 8);

/* "GOLD" / "SILVER" / "BRONZE" (the localized names, g_002C43E8[0x35..0x37])
 * for ranks 0..2 (an unsigned test), else "NO PRIZE" (g_00351F08). */
const char *prize_rank_name(int rank);          /* 0x0018A110 */

/* vtable 0x0032C658 (5 slots; 1 and 3 its own, 4 new), typeinfo
 * __tf9MiniPrize 0x0018A748.  vptr +0x90 (PrizeBase's); 0x9C bytes. */
class MiniPrize : public PrizeBase {
public:
    /* PrizeBase, then nothing loaded ({-1, -1}).  Also called by
     * trophy_prize_screen (F_00137DC0) and LicensePrize's constructor. */
    MiniPrize();                                /* 0x0018A1F0 */
    /* Its vptr, then ~PrizeBase with the caller's flags (which deletes). */
    virtual ~MiniPrize();                       /* [1] +0x14 0x0018A230 */
    /* PrizeBase's, then nothing is loaded. */
    virtual void unload();                      /* [3] +0x24 0x0018A258 */
    /* Draws the prize into buf (drawAntialiased with the context). */
    virtual void virtual_4(GSBuffer &buf, f32 alpha);   /* [4] +0x2C 0x0018A3B0 */

    /* Loads trophy id (into buf, size bytes; keep: buf is the caller's)
     * through PrizeBase::load unless it is the one loaded; nothing for a
     * negative type.  keep is an int, passed on to PrizeBase::load's int
     * keep unmasked (as retail; u048 / u019 / u049 had int before the
     * canonical headers). */
    void loadPrize(const PrizeId *id, void *buf, int size, int keep);   /* 0x0018A298 */
    /* virtual_4(buf, alpha).  Called by LicensePrize::render. */
    void render(GSBuffer &buf, f32 alpha);      /* 0x0018A388 */
    /* (? name) When the model is shown (dispFlag == 1): two passes of the
     * model into a work copy of buf at the sub-pixel offsets g_002B96D8,
     * accumulated, then over buf with alpha.  ctx is not used. */
    void drawAntialiased(GSBuffer &buf, PrizeModelSetContext *ctx, f32 alpha);  /* 0x0018A3D0 */
    /* (? name, no callers) prize_model_loop(model, &context) when there is
     * a model and updateFlag is 1. */
    void restartAnimation();                    /* 0x0018A350 */

    /* Inline (? names, no callers): egcs emits copies of a class's inline
     * members at the end of the unit that holds its vtable, in reverse
     * declaration order (T145: after the static initialiser, before
     * __tf9MiniPrize).  setup is also expanded by the screens' load
     * functions (u019's GTmodeTrophyControl / ConsolationControl::load,
     * u018's MiniPrizeControlBase::loadPages: buf is loaded before the two
     * sb, as an inline's arguments are). */
    const PrizeModelSetContext *getContextConst() const { return &context; }  /* 0x0018A740 */
    PrizeModelSetContext *getContext() { return &context; }    /* 0x0018A738 */
    void stepAnimation(PrizeModelSetContext *ctx)               /* 0x0018A710 */
    {
        if (model != 0)
            prize_model_step(model, ctx);
    }
    void resetAnimation(PrizeModelSetContext *ctx)              /* 0x0018A6E8 */
    {
        if (model != 0)
            prize_model_loop(model, ctx);
    }
    /* A static member (the copy loads id->rank from $a0: no `this'). */
    static const char *getRankName(const PrizeId *id)           /* 0x0018A6C8 */
    {
        return prize_rank_name(id->rank);
    }
    /* dispFlag = disp, updateFlag = upd, then loadPrize. */
    void setup(const PrizeId *id, u8 disp, u8 upd, void *buf, int size, int keep)  /* 0x0018A698 */
    {
        setDispFlag(disp);
        setUpdateFlag(upd);
        loadPrize(id, buf, size, keep);
    }
    PrizeId *getLoaded() { return &loaded; }    /* 0x0018A690 */

    PrizeId loaded;                             /* 0x94 the trophy loaded ({-1, -1} none) */
};
EGCS_SIZE(MiniPrize, 0x9C);


#ifndef BOOT_OLD_GCC                            /* egcs generates it itself */
/* The __si_type_info {vptr, name, base} __tf9MiniPrize fills (u048 / u049
 * g_004B2070; the plan had put it in T140). */
extern "C" u32 __ti9MiniPrize[3];               /* 0x004B2070 */
#endif

#endif /* GAME_T145_MINI_PRIZE_H */
