/* T186 - GT2KMSGRACEDISPLAY::Message, the "RACEDISPLAY" real-time text
 * table (56 keys / texts: the race display's words, "Lap", "1st".."12th"
 * ...), and its global instance g_002C44C8.  0x001AE880..0x001AE990.
 *
 * Data: .rodata 0x00330CD0..0x00331236: the keys' and the texts' strings
 * longer than 8 bytes (only the tables point at them: DATA_START), the
 * constructor's "RACEDISPLAY", the vtable, the one type name (Message:
 * retail's message units have no type name but their own).  .sdata
 * 0x00352258..0x00352380: the strings of 8 bytes or less (-G8), keys then
 * texts.  .data 0x002C4308: the keys, the texts, g_002C44C8, then the
 * frame info. */
// DATA_START 0x00330CD0
// DATA_START 0x00352258
#include "game/data/messages/T186_msg_race_display.h"
#ifndef BOOT_OLD_GCC
#include "runtime/cxx/T548_tinfo.h"                /* __rtti_si */
#endif
#include "game/effects/replay_effects_internal.h"                           /* g_002C4308 */

/* The keys and texts (.data 0x002C4308 / 0x002C43E8), defined first as in
 * the original: their strings open the unit's .rodata and .sdata, in front
 * of the constructor's name, the vtable and the type name.  Equal literals
 * are one string (egcs shares them within a unit). */
const char *g_002C4308[RACEDISPLAY_COUNT] = {   /* 0x002C4308 */
    "TimeLimit",                        /* 0x00330CD0 */
    "GameOver",                         /* 0x00330CE0 */
    "WrongWay",                         /* 0x00330CF0 */
    "PleaseConnect",                    /* 0x00330D00 */
    "WaitOther",                        /* 0x00330D10 */
    "AutoDrive",                        /* 0x00330D20 */
    "ExamFail",                         /* 0x00330D30 */
    "ExamTimeOver",                     /* 0x00330D40 */
    "TimeTrialFail",                    /* 0x00330D50 */
    "PitIn",                            /* 0x00352258 */
    "PolePosition",                     /* 0x00330D60 */
    "StartQualify",                     /* 0x00330D70 */
    "ReadyQualify",                     /* 0x00330D80 */
    "RecordEnd",                        /* 0x00330D90 */
    "EnduranceFinal",                   /* 0x00330DA0 */
    "EnduranceLaps",                    /* 0x00330DB0 */
    "FreeRunReady",                     /* 0x00330DC0 */
    "FreeRunHelp",                      /* 0x00330DD0 */
    "PlayerWin",                        /* 0x00330DE0 */
    "QualifyMode",                      /* 0x00330DF0 */
    "DemoMode",                         /* 0x00330E00 */
    "LiveMode",                         /* 0x00330E10 */
    "ReplayMode",                       /* 0x00330E20 */
    "LapDiffInfo",                      /* 0x00330E30 */
    "PPTimeLabel",                      /* 0x00330E40 */
    "TotalTimeLabel",                   /* 0x00330E50 */
    "BestTimeLabel",                    /* 0x00330E60 */
    "LastTimeLabel",                    /* 0x00330E70 */
    "LapTimeLabel",                     /* 0x00330E80 */
    "RecordTimeLabel",                  /* 0x00330E90 */
    "RecordLapLabel",                   /* 0x00330EA0 */
    "TimeLabel",                        /* 0x00330EB0 */
    "RecordSpeedLabel",                 /* 0x00330EC0 */
    "SessionSpeedLabel",                /* 0x00330ED8 */
    "FastestTimeLabel",                 /* 0x00330EF0 */
    "TrialTimeLabel",                   /* 0x00330F08 */
    "LapCountLabel",                    /* 0x00330F18 */
    "Rank1st",                          /* 0x00352260 */
    "Rank2nd",                          /* 0x00352268 */
    "Rank3rd",                          /* 0x00352270 */
    "Rank4th",                          /* 0x00352278 */
    "Rank5th",                          /* 0x00352280 */
    "Rank6th",                          /* 0x00352288 */
    "Rank7th",                          /* 0x00352290 */
    "Rank8th",                          /* 0x00352298 */
    "Rank9th",                          /* 0x003522A0 */
    "Rank10th",                         /* 0x00330F28 */
    "Rank11th",                         /* 0x00330F38 */
    "Rank12th",                         /* 0x00330F48 */
    "MaxSpeed",                         /* 0x00330F58 */
    "NewRecord",                        /* 0x00330F68 */
    "RecordTime",                       /* 0x00330F78 */
    "RecordSpeed",                      /* 0x00330F88 */
    "PrizeGold",                        /* 0x00330F98 */
    "PrizeSilver",                      /* 0x00330FA8 */
    "PrizeBronze",                      /* 0x00330FB8 */
};
const char *g_002C43E8[RACEDISPLAY_COUNT] = {   /* 0x002C43E8 */
    "Time Limit",                       /* 0x00330FC8 */
    "GAME OVER",                        /* 0x00330FD8 */
    "WRONG WAY",                        /* 0x00330FE8 */
    "\300\334\302\263\245\250\245\351\241\274", /* 0x00330FF8 接続エラー */
    "\301\352\274\352\244\316\261\376\305\372\244\362\302\324\244\303\244\306\244\244\244\336\244\271", /* 0x00331008 相手の応答を待っています */
    "AUTO DRIVE",                       /* 0x00331028 */
    "\274\272\263\312\244\307\244\271", /* 0x00331038 失格です */
    "\245\277\245\244\245\340\245\252\241\274\245\320\241\274", /* 0x00331048 タイムオーバー */
    "\274\272\263\312\244\307\244\271", /* 0x00331038 失格です */
    "%s \244\254\245\324\245\303\245\310\245\244\245\363\244\267\244\336\244\267\244\277", /* 0x00331058 %s がピットインしました */
    "%s \244\254\273\303\304\352\245\335\241\274\245\353\245\335\245\270\245\267\245\347\245\363\244\307\244\271", /* 0x00331070 %s が暫定ポールポジションです */
    "\315\275\301\252\245\242\245\277\245\303\245\257\244\362\263\253\273\317\244\267\244\306\262\274\244\265\244\244", /* 0x00331090 予選アタックを開始して下さい */
    "\243\261\274\376\244\316\245\246\245\251\241\274\245\340\245\242\245\303\245\327\270\345\244\313\245\242\245\277\245\303\245\257\263\253\273\317\244\307\244\271", /* 0x003310B0 １周のウォームアップ後にアタック開始です */
    "\245\352\245\327\245\354\245\244\244\316\265\255\317\277\244\362\275\252\316\273\244\267\244\336\244\267\244\277", /* 0x003310E0 リプレイの記録を終了しました */
    "\245\325\245\241\245\244\245\312\245\353\245\351\245\303\245\327\244\307\244\271", /* 0x00331100 ファイナルラップです */
    "\244\242\244\310 %d \245\351\245\303\245\327\244\307\244\271", /* 0x00331118 あと %d ラップです */
    "\274\241\244\316\245\263\245\363\245\310\245\355\241\274\245\353\245\351\245\244\245\363\244\253\244\351\267\327\302\254\263\253\273\317\244\267\244\336\244\271", /* 0x00331130 次のコントロールラインから計測開始します */
    "\245\273\245\303\245\267\245\347\245\363\275\252\316\273\244\317 START \245\334\245\277\245\363\244\307\244\271", /* 0x00331160 セッション終了は START ボタンです */
    "PLAYER %d WIN!",                   /* 0x00331188 */
    "QUALIFY",                          /* 0x003522A8 */
    "DEMONSTRATION",                    /* 0x00331198 */
    "LIVE",                             /* 0x003522B0 */
    "REPLAY",                           /* 0x003522B8 */
    "%d Laps",                          /* 0x003522C0 */
    "P.P.",                             /* 0x003522C8 */
    "Total",                            /* 0x003522D0 */
    "Best",                             /* 0x003522D8 */
    "Last",                             /* 0x003522E0 */
    "Lap",                              /* 0x003522E8 */
    "Record",                           /* 0x003522F0 */
    "Record Lap",                       /* 0x003311A8 */
    "Time",                             /* 0x003522F8 */
    "Record",                           /* 0x003522F0 */
    "Best",                             /* 0x003522D8 */
    "Fastest",                          /* 0x00352300 */
    "Time",                             /* 0x003522F8 */
    "Lap",                              /* 0x003522E8 */
    "1st",                              /* 0x00352308 */
    "2nd",                              /* 0x00352310 */
    "3rd",                              /* 0x00352318 */
    "4th",                              /* 0x00352320 */
    "5th",                              /* 0x00352328 */
    "6th",                              /* 0x00352330 */
    "7th",                              /* 0x00352338 */
    "8th",                              /* 0x00352340 */
    "9th",                              /* 0x00352348 */
    "10th",                             /* 0x00352350 */
    "11th",                             /* 0x00352358 */
    "12th",                             /* 0x00352360 */
    "Max Speed",                        /* 0x003311B8 */
    "NEW RECORD",                       /* 0x003311C8 */
    "RECORD TIME",                      /* 0x003311D8 */
    "RECORD SPEED",                     /* 0x003311E8 */
    "GOLD",                             /* 0x00352368 */
    "SILVER",                           /* 0x00352370 */
    "BRONZE",                           /* 0x00352378 */
};

namespace GT2KMSGRACEDISPLAY {

/* Caller: the static initialiser of g_002C44C8. */
// FUNC 0x001AE880 GT2KMSGRACEDISPLAY::Message::Message
Message::Message()
    : GT2KRealTimeText("RACEDISPLAY", g_002C4308, g_002C43E8, RACEDISPLAY_COUNT)
{
}

// FUNC 0x001AE8D0 GT2KMSGRACEDISPLAY::Message::~Message
/* Slot 0 (index.txt: GT2KMSGRACEDISPLAY::Message::virtual_0).  The vptr
 * (+0x14) = 0x00331208, then GT2KRealTimeText::~GT2KRealTimeText(this,
 * flags). */
Message::~Message()
{
    /* implicit: ~GT2KRealTimeText (base destructor) */
}

} /* namespace GT2KMSGRACEDISPLAY */

/* The global instance (.data 0x002C44C8: egcs puts a public object that has
 * a constructor in .data, zero-filled, not in .bss). */
GT2KMSGRACEDISPLAY::Message g_002C44C8;

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_002C44C8 (.dtors entry 0x0028F224, .ctors entry 0x0028F144), emitted
 * at the end of the unit, the destructor first: ~Message(2) / Message() on
 * it.  Named after the unit's first global, the keys g_002C4308. */
// FUNC 0x001AE8F8 T186_static_destroy (_GLOBAL_$D$g_002C4308)
/* implicit: ~Message (the static destructor of g_002C44C8) */

// FUNC 0x001AE920 T186_static_init (_GLOBAL_$I$g_002C4308)
/* implicit: Message (the static initialiser of g_002C44C8) */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
// FUNC 0x001AE940 __tfQ218GT2KMSGRACEDISPLAY7Message
/* __si_type_info (0x004B23B0), base __ti16GT2KRealTimeText. */
extern "C" void *__tfQ218GT2KMSGRACEDISPLAY7Message(void)
{
    if (__tiQ218GT2KMSGRACEDISPLAY7Message[0] == 0) {
        __tf16GT2KRealTimeText();
        __rtti_si(__tiQ218GT2KMSGRACEDISPLAY7Message, "Q218GT2KMSGRACEDISPLAY7Message",
                  (const type_info *)__ti16GT2KRealTimeText);
    }
    return __tiQ218GT2KMSGRACEDISPLAY7Message;
}
#endif
