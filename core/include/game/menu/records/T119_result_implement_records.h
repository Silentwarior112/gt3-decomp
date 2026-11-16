/* T119_result_implement_records.h - ResultImplementRecords, the base of
 * the result screens that look up the place the player's result takes in
 * a record table (translation unit T119, unit u041), 0x001759C8..0x00175F48.
 *
 * finish (the player's car reached the goal) asks checkRecords() for the
 * place the result would take (the find functions of T415..T417 only
 * search: the table is not changed here); render() shows the place ("rank" font, the
 * ordinal text of T158) and a blinking title ("NEW RECORD" / "RECORD
 * TIME", getRecordTitle), then drawRecord() the value.  After `delay'
 * seconds the pad's bit 0x40 or the end of `timer' posts event 1.  Bases
 * of ResultMachineTest (T120), ResultTimeTrial (T189) and ResultFreeRun
 * (T190). */
#ifndef GAME_T119_RESULT_IMPLEMENT_RECORDS_H
#define GAME_T119_RESULT_IMPLEMENT_RECORDS_H

#include "game/game.h"
#include "game/menu/records/T116_race_result_base.h"     /* base RaceResultBase */

class type_info;
class RaceOrganization;                     /* T125 (references only) */
class Control;                              /* T474 */
class GSBuffer;                             /* T514 */

/* The typeinfo function (vtable entry 0).  egcs synthesizes it itself
 * once the class is complete, unless an earlier declaration exists (then
 * the vtable refers to that one and the function is not emitted), so
 * only the GCC 14 build declares it.  It builds the __si_type_info at
 * 0x004B1E10 (name 0x003264C8, base __ti14RaceResultBase 0x00353358). */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf22ResultImplementRecords(void);    /* 0x00175EC8 */
}
#endif

/* vtable 0x00326460 (12 slots).  Size 0x20 (ResultTimeTrial's index is at
 * +0x20).  The constructor and the destructor are implicit. */
class ResultImplementRecords : public RaceResultBase {
public:
    /* [0] +0x0C 0x00175F18 ~ResultImplementRecords: implicit (the copy for
     * the vtable follows the typeinfo function). */
    /* The base's start; enabled = !replay, delay 5 s, timer 10 s, blink and
     * recordPlace 0. */
    virtual void start(RaceOrganization &org, int replay);     /* [1] +0x14 0x001759C8 */
    virtual void cleanup();                 /* [2] +0x1C 0x00175A20 nothing */
    /* The base's update; once enabled and done: first the delay runs out,
     * then the pad's bit 0x40 or a negative timer posts event 1. */
    virtual void update(RaceOrganization &org, Control *pad);  /* [3] +0x24 0x00175A80 */
    /* Nothing before the delay is over; the place and the title only for
     * places 0..11 (unsigned: -1, no record, skips them); then drawRecord. */
    virtual void render(GSBuffer &buf, RaceOrganization &org); /* [4] +0x2C 0x00175C30 */
    /*           [5] +0x34 RaceResultBase::goal */
    /* When enabled: the base's finish, then recordPlace = checkRecords(). */
    virtual void finish(RaceOrganization &org);                 /* [6] +0x3C 0x00175A28 */
    /*           [7] +0x44 RaceResultBase::isPlayer */
    /* The place the result would take in the record table (-1: none; the
     * table is only searched).  Here: no table.  Inline (Lupo 2: linkonce; T119's copy
     * precedes the typeinfo function). */
    virtual int  checkRecords(RaceOrganization &org) { return -1; }  /* [8] +0x4C 0x00175EC0 */
    /* "Lap" and the time getRecord() returns (system font). */
    virtual void drawRecord(RaceOrganization &org);             /* [9] +0x54 0x00175DC0 */
    /* The player's race time (lap times total). */
    virtual int  getRecord(RaceOrganization &org);              /* [10] +0x5C 0x00175EA0 */
    /* Place 0 "NEW RECORD", else "RECORD TIME". */
    virtual const char *getRecordTitle();   /* [11] +0x64 0x00175C10 */
    /* `text' (as a format) twice with the system font: in `shadow' at
     * (x + dx, y + dy), then in `color' at (x, y).  `this' is unused. */
    void drawShadowText(const char *text, int x, int y, int dx, int dy,
                        u32 color, u32 shadow);                 /* 0x00175B50 */

    int enabled;                            /* 0x0C start's replay was 0 */
    f32 delay;                              /* 0x10 seconds before the screen reacts */
    f32 timer;                              /* 0x14 seconds before it leaves on its own */
    int recordPlace;                        /* 0x18 checkRecords(): 0.. the place taken, -1 none */
    int blink;                              /* 0x1C the title's colour, toggled every frame */
};
EGCS_SIZE(ResultImplementRecords, 0x20);

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
extern u32 __ti22ResultImplementRecords[3]; /* 0x004B1E10 (__si_type_info; u041: g_004B1E10) */
}
#endif

#endif /* GAME_T119_RESULT_IMPLEMENT_RECORDS_H */
