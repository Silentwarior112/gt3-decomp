/* race_time_trial_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u048,
 * 0x00187F98..0x0018A798, second pass: three one-car race modes, their
 * displays, and the trophy model of the small prize screens.
 *
 * Translation units (link order), each with its canonical header:
 *   T140  RaceTimeTrial (T140_race_time_trial.h)
 *   T141  ExaminationDisplayer, TimeTrialExaminationMessages,
 *         RaceTimeTrialDisplay (T141_race_time_trial_display.h)
 *   T142  RaceFreeRun, RaceFreeRunInformation (T142_race_free_run.h)
 *   T143  RaceFreeRunDisplay (T143_race_free_run_display.h)
 *   T144  RaceLicense, RaceLicenseBGM, LicensePause (T144_race_license.h)
 *   T145  MiniPrize, PrizeId (T145_mini_prize.h)
 *
 * Every class is defined in a canonical header.  This header has the unit's
 * named constants and T145's trophy tables only, and needs no game header:
 * each source includes the canonical headers it needs itself, in the order
 * that gives retail's type names (u048's data pass; each file's comment
 * lists them), and this header last.  The race class records of T140 / T142
 * / T144 are declared with their classes (T140_race_time_trial.h,
 * T142_race_free_run.h, T144_race_license.h).
 */
#ifndef U048_H
#define U048_H

#include "types.h"
#include "game/game.h"                      /* EGCS_SIZE */

/* ======================================================================== */
/* Named constants                                                          */
/* ======================================================================== */

/* Race event types: RACE_EVENT_* of T122_race_event_queue.h. */

/* BGM codes (RaceBGMPS2::play: 0x14 the next song of the play list,
 * another code the group of that number). */
enum {
    BGM_CODE_NEXT_SONG      = 0x14,
    BGM_CODE_DISQUALIFIED   = 0x20          /* BGM group 0x20, the time trial's disqualification */
};

/* RACEDISPLAY texts: g_002C43E8[RACEDISPLAY_*] (T186). */

/* PauseBase::virtual_2's results: PAUSE_RESULT_* (T168). */

/* RaceBase::virtual_34's end codes RACE_END_* and virtual_8's replay
 * argument RACE_REPLAY: T129. */

/* AutomobileControlRecord::Manager::mode: one 0x8000-byte slot. */
enum { RECORD_MODE_ONE_SLOT = 2 };

/* The written size of a RaceInfoBlock (its size word): the control records
 * of the RaceSolitaire modes follow it (sizeof is 0x1920: the u64 members
 * align the struct to 8). */
enum { RACE_INFO_BLOCK_SIZE = 0x191C };

struct RaceInfoBlock;                       /* T129 */

/* The saved control records after an info block. */
static inline const u32 *race_info_block_records(RaceInfoBlock *blk)
{
    return (const u32 *)((const u8 *)blk + RACE_INFO_BLOCK_SIZE);
}

/* T144: an empty function between RaceLicense::start and the static initialiser;
 * no callers, no data references (? an empty inline method egcs emitted). */
void F_00189FA8(void);                      /* 0x00189FA8 */

/* ======================================================================== */
/* T145: the trophy tables                                                  */
/* ======================================================================== */

struct PrizeId;                             /* T145 */

/* ? name.  One entry of the rank table g_002B96C0 (built by T145's static
 * initialiser): the model file suffix and its RACEDISPLAY text. */
struct PrizeRankEntry {
    /* Inline: the table's elements are built with it at start-up (egcs's
     * build_vec_init in _GLOBAL_$I$, 0x0018A640: stores through a cursor). */
    PrizeRankEntry(const char *f, int m) : file(f), msgId(m) {}

    const char *file;                       /* 0x0 "gold.prm" ... */
    int         msgId;                      /* 0x4 RACEDISPLAY_PRIZE_* (T186) */
    /* localized != 0: the text, else the file suffix. */
    const char *get(int localized) const;   /* 0x0018A0E8 */
};
EGCS_SIZE(PrizeRankEntry, 8);

/* A sub-pixel offset of the antialiased draw. */
struct PrizeJitter {
    f32 x;                                  /* 0x0 */
    f32 y;                                  /* 0x4 */
};
EGCS_SIZE(PrizeJitter, 8);

/* Defined in T145 (in retail's order: its .data / .sdata / .rodata). */
extern PrizeRankEntry g_002B96C0[3];        /* 0x002B96C0 gold, silver, bronze (.data, built at start-up) */
extern const char    *g_002B9698[];         /* 0x002B9698 the model prefix of each PrizeId type: "", "a_easy_", ...
                                               "g_special_" */
extern PrizeJitter    g_002B96D8[2];        /* 0x002B96D8 {0.25, 0.75}, {0.75, 0.25} */
extern const char    *g_00351F08;           /* 0x00351F08 gp-0x5AE8 "NO PRIZE" */

/* "race/consolation.prm" for a negative type or rank, else
 * "race/<type prefix><rank file>".  Caller: MiniPrize::loadPrize. */
void prize_model_path(char *buf, const PrizeId *id);        /* 0x0018A150 */

#endif /* U048_H */
