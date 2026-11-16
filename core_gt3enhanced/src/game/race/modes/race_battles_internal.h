/* race_battles_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u047,
 * 0x001848A8..0x00187F98, second pass.
 *
 * Translation units (link order), each with its canonical header:
 *   T138  RaceLinkBattle (one console of the i.LINK battle, up to 8 PS2s),
 *         its base RaceBattleT<8>, RaceLinkBattleInformation (and its base
 *         RaceBattleInformationT<8>), the inputs RaceLinkBattleInput (the
 *         local player) and RaceLinkBattleNodeInput (one per i.LINK node)
 *         (include/game/race/modes/T138_race_link_battle.h)
 *   T139  RaceSolitaire (the base of the one-player modes: time trial, free
 *         run, licence, machine test), RaceSolitaireInformation,
 *         RaceSolitaireEntry (include/game/race/modes/T139_race_solitaire.h)
 *
 * Every class is defined in a canonical header; this header has what only
 * this unit uses: the constants of the link session and of the screen wall,
 * the record helper of T139, the typeinfo storage of the unit's own classes
 * and the unit's data.
 *
 * Includes (u047's data pass): each source includes the canonical headers
 * it needs itself, in the order that gives retail's type names (egcs writes
 * the names of the classes a file sees into its .rodata, in reverse
 * definition order: see each file's head), and this header last.  It
 * includes no game header.  (Last also because of the data externs below:
 * egcs decides $gp addressing at a variable's first declaration, so the
 * registry structs (T129) must be complete there.)
 */
#ifndef U047_H
#define U047_H

#include "types.h"

class RaceInput;                            /* T413 */
struct RaceClassInfo;                       /* T129 */
struct RaceClassNode;                       /* T129 */

/* ======================================================================== */
/* Race events (T122; the types RaceLinkBattle and RaceSolitaire post)      */
/* ======================================================================== */
/* Race event types: RACE_EVENT_* of T122_race_event_queue.h. */

/* ======================================================================== */
/* Values of other units' data the sources test or pass                     */
/* ======================================================================== */

/* PadManager::attach slots (T475; u105's SLOT_LINK / SLOT_STEERING). */
enum {
    PAD_SLOT_LINK      = 0x0C,              /* 12-19: i.LINK node 0..7 */
    PAD_SLOT_STEERING1 = 0x14               /* 20: the pad + wheel of port 1 (the player) */
};
enum { LINK_NODES = 8 };                    /* the nodes of an i.LINK session */

/* RaceEntryInformation::inputId (T121): the RaceBase::virtual_26 argument. */
enum {
    INPUT_ID_SECOND = 1,                    /* the second (ghost) car */
    INPUT_ID_PLAYER = 0x100                 /* 0x100 + player / node */
};

/* COURSE_KIND_* (RaceDataBase::getCourseKind): game/data/T484_race_database.h. */

/* AutomobileControlRecord::Manager::mode (T318). */
enum {
    RECORD_MODE_BUFFER = 0,                 /* the record covers a whole buffer of the pool */
    RECORD_MODE_LAPS   = 1,                 /* four 0x8000-byte slots, one per lap */
    RECORD_MODE_SLOT   = 2                  /* one slot: a OneLapGhostInfo and its record */
};

/* Font::setAlign: G001 font_set_align's FONT_ALIGN_* (G001_flare.h). */

/* ======================================================================== */
/* T138: the i.LINK session (T473) and RaceLinkBattle's link state          */
/* ======================================================================== */

/* F_0022CD28, the session state: T473's LINK_STATE_*. */

/* RaceLinkBattle::state. */
enum {
    LINK_BATTLE_RUNNING = 0,                /* the race runs; watches the pause button and the session */
    LINK_BATTLE_PAUSED  = 1,                /* this console asked for the pause: its pause menu runs */
    LINK_BATTLE_WAITING = 2,                /* waits for the other nodes to take the command exchange */
    LINK_BATTLE_ERROR   = 3                 /* the link was lost: "connection error" over the race */
};

/* PauseBase::virtual_2's results: PAUSE_RESULT_* (T168). */

/* RaceLinkBattle::virtual_27: frames a command exchange may wait before the
 * link is reset (F_0026EB90). */
enum { LINK_WAIT_RESET_FRAMES = 0x259 };    /* 601 */

/* ======================================================================== */
/* T138: the screen wall (ScreenLayout, T383)                               */
/* ======================================================================== */

/* ScreenLayout::flags / modeX / modeY: WALL_* (T383). */

/* (The instances <8> of the T137 templates are defined in T138: their
 * explicit specializations are declared at its head.) */

/* The messages of RaceLinkBattle: g_002C43E8[RACEDISPLAY_*] (T186). */

/* ======================================================================== */
/* T139                                                                     */
/* ======================================================================== */

/* The control records a RaceSolitaire saves follow its RaceInfoBlock: the
 * block's size word is 0x191C (the struct itself is 8-byte aligned). */
enum { RACE_INFO_BLOCK_SIZE = 0x191C };

/* The replay memory: three blocks of the arena g_002C2F50. */
enum {
    REPLAY_BLOCK_SIZE = 0xA8000,
    REPLAY_MEM_SIZE   = 3 * REPLAY_BLOCK_SIZE   /* 0x1F8000 */
};

/* input->rec.setBuffer(buf).  Only virtual_57 calls it.  (h24 / i2: a T139
 * function; nothing shows it is an inline RaceInput method.) */
void race_input_set_record_buffer(RaceInput *input, u8 *buf);  /* 0x00187288 */

/* ======================================================================== */
/* RTTI of the unit's own classes                                           */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC
/* egcs generates the typeinfo functions itself; these are for GCC 14 only.
 * The __si_type_info storage {vptr, name, base} (.bss) of the classes only
 * this unit builds (__ti24RaceSolitaireInformation and __ti13RaceSolitaire
 * are in T139_race_solitaire.h). */
extern "C" {
extern u32 __tit22RaceBattleInformationT1i8[3];     /* 0x004B1250 (u047 g_004B1250) */
extern u32 __tit11RaceBattleT1i8[3];                /* 0x004B1340 (u047 g_004B1340) */
extern u32 __ti14RaceLinkBattle[3];                 /* 0x004B1F80 (u047 g_004B1F80) */
extern u32 __ti23RaceLinkBattleNodeInput[3];        /* 0x004B1F90 (u047 g_004B1F90) */
extern u32 __ti25RaceLinkBattleInformation[3];      /* 0x004B1FA0 (u047 g_004B1FA0) */
extern u32 __ti19RaceLinkBattleInput[3];            /* 0x004B1FB0 (u047 g_004B1FB0) */
extern u32 __ti18RaceSolitaireEntry[3];             /* 0x004B1FD0 (u047 g_004B1FD0) */
}
#endif /* !BOOT_OLD_GCC */

/* ======================================================================== */
/* Data of the unit                                                         */
/* ======================================================================== */

/* {"RaceLinkBattle", 0x35000, race_link_battle_create,
 * race_link_battle_construct} and its registry node (defined in T138). */
extern RaceClassInfo g_002B7960;            /* 0x002B7960 */
extern RaceClassNode g_00351E78;            /* 0x00351E78 gp-0x5B78 */
/* {"RaceSolitaire", 0x21800, race_solitaire_create,
 * race_solitaire_construct} and its registry node (defined in T139). */
extern RaceClassInfo g_002B8180;            /* 0x002B8180 */
extern RaceClassNode g_00351E90;            /* 0x00351E90 gp-0x5B60 */
/* (RaceLinkBattle::virtual_27's frame count g_0035322C is a file static of
 * T138: retail's .sbss.) */

#endif /* U047_H */
