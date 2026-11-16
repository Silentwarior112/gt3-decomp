/* race_license_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u049,
 * 0x0018A798..0x0018DD80, second pass: five translation units.
 *
 *   T146  LicenseExaminationMessages (the result screen of a licence test:
 *         the trophy model, its rank name and the time; or the failure
 *         text), LicensePrize, ExamMessageLine, the text helpers, and the
 *         out-of-line copies of ExaminationDisplayer's inline methods and of
 *         Color8's set / setters
 *   T147  RaceLicenseDisplay (the race display of a licence test plus the
 *         pass / fail sequences)
 *   T148  RaceMachineTest (0-400 m, 0-1000 m, max speed: "RaceMachineTest")
 *         and RaceMachineTestInformation
 *   T149  RaceReplayInformation (the race information read back from the
 *         RaceInfoBlock of a saved replay / ghost)
 *   T150  Serialize::ReplayData, GhostData, TrackData, MaxSpeed: the
 *         serialisers of the replay / ghost saves
 *
 * The classes are declared in the canonical headers of the five units
 * (include/game/T146..T150).  Each source includes the canonical headers it
 * needs itself, in the order that gives retail's type names (u049's data
 * pass, ref/data/u049/notes.md: egcs writes a file's type names in reverse
 * class-definition order, and every class with only inline virtual
 * functions that a file sees gets its name there), then this header last.
 * This header keeps only the unit's named constants; the two text helpers
 * no other unit calls are declared in T146, the static data of T148 / T150
 * (the race class registration, the serialisers' factories and
 * registrations) is defined in those files.
 *
 * Offsets in comments are the ORIGINAL (egcs) ones: egcs puts the vptr of
 * the first polymorphic class after that class's data (ExaminationDisplayer
 * +0x4, PrizeBase +0x90, RaceInformation +0x54, ...), GCC 14 at +0.
 */
#ifndef U049_H
#define U049_H

#include "types.h"

/* ------------------------------------------------------------------------ */
/* Named constants                                                          */
/* ------------------------------------------------------------------------ */

/* Race event types: RACE_EVENT_* of T122_race_event_queue.h. */
/* The BGM codes this unit posts with RACE_EVENT_BGM (? names: the jingles;
 * RaceDisplay::virtual_16 posts 0x13 when a car finishes, the time trial's
 * disqualification 0x20). */
enum {
    BGM_CODE_FINISH       = 0x13,
    BGM_CODE_PRIZE_GOLD   = 0x1D,
    BGM_CODE_PRIZE_SILVER = 0x1E,
    BGM_CODE_PRIZE_BRONZE = 0x1F,
    BGM_CODE_FAILED       = 0x20
};

/* The result of a licence test (RaceOrganization::result.rank, then
 * LicenseExaminationMessages::kind): > 0 passed with that trophy, 0 the time
 * is over; LicenseExaminationMessages takes -1 for any failure. */
enum {
    LICENSE_RESULT_FAILED = -1,         /* setup()'s kind of a failure */
    LICENSE_RESULT_TIME_OVER = 0,       /* result.rank: failed, the time is over */
    LICENSE_RESULT_GOLD_1 = 1,          /* 1, 2: gold */
    LICENSE_RESULT_GOLD_2 = 2,
    LICENSE_RESULT_SILVER = 3,
    LICENSE_RESULT_BRONZE_1 = 4,        /* 4, 5: bronze */
    LICENSE_RESULT_BRONZE_2 = 5
};
/* The trophy ranks of PrizeId::rank (T145). */
enum { PRIZE_RANK_GOLD = 0, PRIZE_RANK_SILVER = 1, PRIZE_RANK_BRONZE = 2 };
/* The GameStatus small-trophy bits of the licence trophies (? names). */
enum {
    SMALL_TROPHY_LICENSE_GOLD   = 0x20000,
    SMALL_TROPHY_LICENSE_SILVER = 0x40000,
    SMALL_TROPHY_LICENSE_BRONZE = 0x80000
};
/* The colours of the result screen (0xAABBGGRR, Color8::set). */
enum {
    LICENSE_COLOR_GOLD    = 0x00D7FF,   /* r 0xFF, g 0xD7, b 0 */
    LICENSE_COLOR_SILVER  = 0xE0EEE0,
    LICENSE_COLOR_BRONZE  = 0x7A96D0,   /* r 0xD0, g 0x96, b 0x7A */
    LICENSE_COLOR_DEFAULT = 0x9AFF9A,   /* light green: a kind outside 1..5 */
    LICENSE_COLOR_BAND    = 0xFFA0A0,   /* the band of the failure screen (light blue) */
    LICENSE_COLOR_FAILED  = 0x40C0A0    /* the failure text */
};
/* One trophy model in LicenseExaminationMessages' block of g_002C2F50. */
enum { LICENSE_PRIZE_MODEL_SIZE = 0x32000 };

/* The race texts of a failed licence test: RACEDISPLAY_EXAM_FAIL / _TIME_OVER (T186). */

/* The pad button that cuts the end of the pass sequence short (the Control
 * buttons pressed this frame). */
enum { LICENSE_SKIP_BUTTON = 0x40 };

/* PauseBase::virtual_2's results: PAUSE_RESULT_* (T168). */
/* RaceBase::virtual_34's end codes (RaceBase::endCode): RACE_END_* (T129). */
/* RaceMachineTest::kind: MACHINE_TEST_* (T415). */

/* The data of a replay save that ReplayData / TrackData load (copied to
 * dest, whatever the header's data size). */
enum { REPLAY_DATA_LOAD_SIZE = 0x41940 };
/* GameThumbnail::type (T108). */
enum { MC_THUMBNAIL_REPLAY = 2, MC_THUMBNAIL_BESTLAP = 3 };

/* The inline accessors of g_00351F50 (0x0018DD10..0x0018DD28) are
 * Serialize::ReplayData's static members getRace, getOrganization,
 * setOrganization (T150). */

#endif /* U049_H */
