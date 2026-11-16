/* T060_trophy_get.h - gt2k_menu::TrophyGetControl, the championship cup
 * screen (translation unit T060, unit u020), 0x0013A8D0..0x0013B380.
 *
 * trophy_get(prize, trophy) (called by T053's run_championship once a
 * championship is won, trophy = -1) runs page "trophy_get" of
 * "menu/trophy_get.mbl" on a 1 MB menu heap: the cup model (a random
 * "champ_NN" cup unless trophy is 0..9; its GameStatus largeTrophy bit 6 +
 * cup is set when it is loaded), the messages "mess0", "get_mess" and, once
 * the prize counter is done, "mess1" and the pad.  Afterwards the prize
 * goes to the money and the total prize money, and the trophy count goes
 * up.  The end of the translation unit has unreferenced copies of the
 * destructors and typeinfo functions of PrizeCounter (inline),
 * PrizeControlBase and ControlBaseWithPageManager (implicit; documented with
 * their classes). */
#ifndef GAME_T060_TROPHY_GET_H
#define GAME_T060_TROPHY_GET_H

#include "game/game.h"
/* only PrizeControlBase (retail T060 has no LicenseGetControl type name) */
#define T059_BASE_ONLY
#include "game/menu/gtmode/T059_license_get_control.h"  /* base gt2k_menu::PrizeControlBase */
#undef T059_BASE_ONLY
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase (render) */
#include "game/menu/gtmode/T099_prize_counter.h"        /* gt2k_menu::PrizeCounter (counter) */
#include "game/save/T429_game_status.h"          /* getGameStatus() (the inline constructor) */
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuHook (g_0049BDF0) */
#include "game/menu/framework/T217_operator.h"             /* gt2k_menu::OperatorCode (g_00318A38) */

namespace gt2k_menu {

/* vtable 0x00318AD8 (2 slots: [1] ControlBase::waitFrame).  typeinfo
 * __tfQ29gt2k_menu16TrophyGetControl 0x0013B140 (si, base
 * PrizeControlBase; storage 0x004B16E0).  Size 0xF10 (sp+0x000 in
 * trophy_get; inline constructor, implicit destructor).  The counter runs
 * from the player's money to money + prize. */
class TrophyGetControl : public PrizeControlBase {
public:
    TrophyGetControl(int money)
        : PrizeControlBase(), money(money), render(),
          counter(getGameStatus()->getMoney(), getGameStatus()->getMoney() + money) {}
    /* counter's vptr, render, ~PrizeBase, ~ControlBase. */
    /* [0] +0x0C 0x0013B190 ~TrophyGetControl: implicit (no vptr store) */
    /* "menu/trophy_get.mbl"; the cup PrizeModelSpec::setChampionship(trophy)
     * is recorded and loaded into a buffer of its own. */
    void load(int trophy);                  /* 0x0013A8D0 */
    void unload();                          /* 0x0013AA40 the pages, the model */
    /* The render with a transition, music 0x18, the item factories
     * g_0049BDF0. */
    void begin();                           /* 0x0013AA70 */
    void end();                             /* 0x0013AB40 */
    void mainLoop();                        /* 0x0013AB88 page "trophy_get" and its messages */
    void run(int trophy);                   /* 0x0013AD80 load, begin, mainLoop, end, unload */

    /*           PrizeControlBase              0x000 .. 0xE7C */
    int          money;                     /* 0xE7C the prize money (create_get_cr_prize, T056) */
    RenderBase   render;                    /* 0xE80 */
    PrizeCounter counter;                   /* 0xED8 */
};
EGCS_SIZE(gt2k_menu::TrophyGetControl, 0xF10);

} /* namespace gt2k_menu */

/* trophy_get (0x0013ADC8): its own header since u017's data pass (its
 * caller T053 saw it without the classes). */
#include "game/menu/gtmode/T060_trophy_get_entry.h"
/* .ctors entry 0x0028F078: g_0049BDF0 = "count", "count_shadow",
 * "get_money_disp" (T099's _T060 hooks), "render_model" (T058
 * create_trophy_championship). */
void T060_static_init(void);                /* 0x0013B088 */

/* g_0049BDF0 (0x0049BDF0, .bss) TrophyGetControl's item factories: a file
 * static of T060 (retail has it in .bss), defined there (u020's data pass). */
extern const char *g_0029F588[3];          /* 0x0029F588 (T060 .data) "mess0", "get_mess", "mess1" */
extern const gt2k_menu::OperatorCode g_00318A38[];  /* 0x00318A38 (T060 .rodata) the script that takes
                                               "mess0" away (Object::disappearAll) */

#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
void *__tfQ29gt2k_menu16TrophyGetControl(void); /* 0x0013B140 */
}
#endif

#endif /* GAME_T060_TROPHY_GET_H */
