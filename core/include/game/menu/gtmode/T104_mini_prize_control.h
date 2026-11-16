/* T104_mini_prize_control.h - MiniPrizeInfo, the trophy of a mini prize
 * screen, and the arcade trophy screen's item factories (translation unit
 * T104, unit u018), 0x00137468..0x00137840.
 *
 * T104 also defines the non-virtual methods of gt2k_menu::
 * MiniPrizeControlBase (loadPages .. run, 0x00137550..0x001377D8): they are
 * declared with the class in T056_gtmode_trophy_control.h (T056 has its
 * vtable, destructor and RTTI).  The arcade trophy screen (T056
 * trophy_prize_screen: an ArcadeTrophyControl) runs MiniPrizeControlBase::
 * run: "menu/arcade_mini.mbl", the trophy model, page "arcade_mini" with
 * the item factories g_0049BCB8 ("rank_disp", "render_model"). */
#ifndef GAME_T104_MINI_PRIZE_CONTROL_H
#define GAME_T104_MINI_PRIZE_CONTROL_H

#include "game/game.h"
#include "game/race/modes/T145_mini_prize.h"           /* base PrizeId */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuHook (g_0049BCB8) */

namespace gt2k_menu {
class MiniPrizeControlBase;                 /* T056 (the class of T104's methods) */
}

/* (? name, u019; u018: ArcadeTrophyInfo) The trophy a mini prize screen
 * shows, MiniPrizeControlBase +0xE84: the PrizeId of the model
 * (MiniPrize::loadPrize takes it as it is) and the prize money.  type is
 * the race's trophy type 1..8 (1..4 arcade, 5..8 GT mode; T056), rank 0
 * gold .. 2 bronze.  (Whether PrizeId is a base or the first member the
 * code cannot tell; a base lets &info be passed to loadPrize.)  Not
 * polymorphic; 0xC bytes. */
struct MiniPrizeInfo : public PrizeId {
    /* Inline (trophy_prize_screen expands it): type and rank -1, the prize,
     * then set(r, t). */
    MiniPrizeInfo(int p, int r, int t)
    {
        type = -1;
        rank = -1;
        prize = p;
        set(r, t);
    }
    /* (No callers: out-of-line copies.) */
    int  isArcade();                        /* 0x00137468 (? name; u018 isPrize) 1 for the types 1..4 */
    void clearRank();                       /* 0x00137488 (u018 clearKind) rank = -1 */
    /* type = t when it is 1..8 (else unchanged); rank = r when it is 0..2,
     * else -1. */
    void set(int r, int t);                 /* 0x00137498 */

    int prize;                              /* 0x8 the prize money (0 on the arcade screen) */
};
EGCS_SIZE(MiniPrizeInfo, 0xC);

/* .ctors entry 0x0028F0D4: g_0049BCB8 = {"rank_disp" (T056 create_rank_disp,
 * p8 = pC = &g_003515B0), "render_model" (T058 create_trophy_arcade)}. */
void T104_static_init(void);                /* 0x001377D8 */

/* g_0049BCB8 (0x0049BCB8, .bss), MiniPrizeControlBase's item factories, is
 * a file static of T104 (u018's data pass: .bss holds only statics of a C++
 * file). */
extern f32 g_003515B0;                     /* 0x003515B0 gp-0x6440 1.25, the "rank_disp" entry's argument */

#endif /* GAME_T104_MINI_PRIZE_CONTROL_H */
