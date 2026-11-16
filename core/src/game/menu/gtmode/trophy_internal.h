/* trophy_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u019,
 * 0x00137840..0x0013A8D0, second pass.
 *
 * The trophy / prize screens shown after a race (translation units in
 * address order):
 *   T056  trophy_prize_screen, the entry point of the three "mini prize"
 *         screens (the arcade trophy, ArcadeTrophyControl / T104; the GT
 *         mode trophy with its prize money, GTmodeTrophyControl; the
 *         consolation prize, ConsolationControl / T057), the "rank_disp"
 *         item and the GT mode item table; the RTTI and the vtable
 *         destructors of the MiniPrizeControlBase family.
 *   T057  gt2k_menu::ConsolationControl's methods and its item table.
 *   T058  gt2k_menu::TrophyItem / ChampionShipTrophyItem, the "render_model"
 *         item of every prize screen (the prize model drawn with
 *         accumulation-buffer antialiasing and depth of field), and
 *         PrizeModelSpec, the .prm names of the licence trophies and the
 *         championship cups.
 *   T059  gt2k_menu::LicenseGetControl: the licence acquired screen, its
 *         entry point and item table; PrizeControlBase's destructor / RTTI.
 *
 * Every class is defined in its canonical header (include/game/T056 ..
 * T059 and the headers of the classes they use).  Since the data pass
 * (2026-10-17) each file includes the canonical headers it needs itself, in
 * the order that gives retail's type names (egcs writes the type names of
 * the classes a file sees in the reverse order of their definitions; each
 * file's comment lists retail's), and this header last.  It adds what only
 * this unit uses: names for the trophy types, ranks and bits, the menu
 * music groups, the item-table helper of the static initialisers and the
 * typeinfo storage of the unit's own classes (GCC 14).
 */
#ifndef U019_H
#define U019_H

#include "types.h"
#include <stddef.h>
#include "game/game.h"
#include "game/menu/framework/T229_page_layout.h"          /* gt2k_menu::MenuHook (set_entry, HOOK) */
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si, __rtti_user */
#endif

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */

/* The race's trophy type (RaceDataBase::getTrophyType, T484; MiniPrizeInfo::
 * type): the index of the model prefix in T145's g_002B9698 ("a_easy_" ..
 * "g_special_"). */
enum {
    TROPHY_ARCADE_EASY    = 1,              /* "a_easy_" */
    TROPHY_ARCADE_NORMAL  = 2,              /* "a_normal_" */
    TROPHY_ARCADE_HARD    = 3,              /* "a_hard_" */
    TROPHY_ARCADE_SPECIAL = 4,              /* "a_special_" */
    TROPHY_GT_EASY        = 5,              /* "g_easy_" */
    TROPHY_GT_NORMAL      = 6,              /* "g_normal_" */
    TROPHY_GT_HARD        = 7,              /* "g_hard_" */
    TROPHY_GT_SPECIAL     = 8               /* "g_special_" */
};

/* The place (MiniPrizeControlBase::rank, MiniPrizeInfo::rank): the trophy
 * models gold / silver / bronze; other places get the consolation prize. */
enum { RANK_FIRST = 0, RANK_SECOND = 1, RANK_THIRD = 2 };

/* GameStatus::smallTrophy (T429): GT mode type 5..8 x rank 0..2 in bits
 * 0..11 ((type - 5) * 3 + rank), the arcade first places of type 1..4 in
 * bits 12..15, the consolation prize bit 16. */
enum {
    SMALL_TROPHY_ARCADE_EASY    = 0x1000,
    SMALL_TROPHY_ARCADE_NORMAL  = 0x2000,
    SMALL_TROPHY_ARCADE_HARD    = 0x4000,
    SMALL_TROPHY_ARCADE_SPECIAL = 0x8000,
    SMALL_TROPHY_CONSOLATION    = 0x10000
};

/* The licences (LicenseGetControl::licence; GameStatus::largeTrophy bit
 * 1 << licence; g_0029F2C8 / g_0029ED18 rows): T420's LicenseIndex
 * (LICENSE_R, the rally licence, has the files "lic_d", "d_mess*"). */

/* The menu music groups the screens start (T464: 0x12..0x22 are the ADS
 * stream groups 0..0x10).  (? names) */
enum {
    BGM_PRIZE_FANFARE = 0x17,               /* GTmodeTrophyControl / ConsolationControl::mainLoop */
    BGM_LICENSE_GET   = 0x21                /* LicenseGetControl::begin */
};

/* The buffers of the entry functions. */
enum {
    PRIZE_MODEL_BUF_SIZE  = 0x32000,        /* the trophy model file (MiniPrize::loadPrize) */
    PRIZE_MENU_HEAP_SIZE  = 0x40000,        /* trophy_prize_screen's menu heap */
    LICENSE_MENU_HEAP_SIZE = 0x100000       /* license_get_screen's menu heap */
};

/* TrophyItem::state. */
enum { TROPHY_ITEM_WAITING = 0, TROPHY_ITEM_FADING_IN = 1, TROPHY_ITEM_SHOWN = 2 };

/* (PrizeModelSpec::kind: PRIZE_MODEL_LICENSE / PRIZE_MODEL_CUP, in
 * game/menu/gtmode/T058_trophy_item.h.) */

/* ======================================================================== */
/* The item factory tables                                                  */
/* ======================================================================== */

/* One table entry: the inline constructor of the original's MenuHook (u006's
 * form, see the static initialisers). */
static inline void set_entry(gt2k_menu::MenuHook *e, const char *name, gt2k_menu::MenuHookFn *create,
                             int p8, int pC, int p10, int p14)
{
    e->name   = name;
    e->create = create;
    e->p8     = p8;
    e->pC     = pC;
    e->p10    = p10;
    e->p14    = p14;
}

/* One element as egcs's build_vec_init expands it: the constructor on a
 * copy of the cursor (`base'), then the cursor and the count of the
 * elements left (`left', dead) stepped.  T056_static_init /
 * T057_static_init / T059_static_init: the original defined the tables in
 * those files with MenuHook objects built by a constructor (their .bss is
 * filled at run time); the decomp keeps the tables at their original
 * addresses, so the initialisers are written out the way build_vec_init
 * expands them: an exact form, not the original. */
#define HOOK(name, create, p8, pC, p10, p14)                                  \
    { gt2k_menu::MenuHook *e = base; set_entry(e, name, create, p8, pC, p10, p14); } \
    base++;                                                                   \
    left--

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself; these are for GCC 14 only.
 * The __si_type_info storage (3 words, .bss) of the classes only this unit
 * builds the nodes of (ControlBaseWithPageManager's and PrizeControlBase's
 * are declared in T056 / T059, PrizeCounter's in T099). */
extern "C" {
extern u32 __tiQ29gt2k_menu19GTmodeTrophyControl[3];      /* 0x004B1660 */
extern u32 __tiQ29gt2k_menu18ConsolationControl[3];       /* 0x004B1680 */
extern u32 __tiQ29gt2k_menu20MiniPrizeControlBase[3];     /* 0x004B16C0 */
extern u32 __tiQ29gt2k_menu19ArcadeTrophyControl[3];      /* 0x004B16F0 */
extern u32 __tiQ29gt2k_menu22ChampionShipTrophyItem[3];   /* 0x004B17C0 */
extern u32 __tiQ29gt2k_menu10TrophyItem[3];               /* 0x004B17D0 */
extern u32 __tiQ29gt2k_menu17LicenseGetControl[3];        /* 0x004B17E0 */
}
#endif /* !BOOT_OLD_GCC */

#endif /* U019_H */
