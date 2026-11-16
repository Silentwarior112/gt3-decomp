/* T469_sound_effects.h - the sound effects (translation unit T469, unit u104).
 *
 * Three .ins banks (sound/gt3sys.ins at SPU 0x1E0000, gt3se01 / gt3se02.ins
 * from 0x5040 on, headers in a 0x800-byte buffer), the road noise bank
 * (sound/rn_01.es after them) and sound/se.inf: effect name -> code, bank
 * << 24 | program A << 12 | program B.  The effect table (T469 .data
 * 0x002E9380, 0x28 names) is linked to se.inf by name at init.
 *
 * The enhanced engine (Lupo 2, cameras_sound.md S1): 0x33 names, the Pod's
 * 11 after GT3's, and a fourth bank, sound/gt3pod.ins, loaded when the file
 * exists (after rn_01.es, in its own header buffer).  Without it, or without
 * the pod_* entries in se.inf, the Pod is silent and nothing else changes.
 */
#ifndef GAME_T469_SOUND_EFFECTS_H
#define GAME_T469_SOUND_EFFECTS_H

#include "game/game.h"
#include "game/sound/T468_gtsound.h"                  /* GTSOUNDINSTRUMENT (held by value), SoundProgram */

struct SeInf;                                   /* u104: sound/se.inf */

/* The effects: the indexes of the effect table (T469 .data 0x002E9380: the
 * se.inf name of each), se_play / se_program / se_code's se.  0..0xC are
 * the menu sounds (menu_play_se, T233). */
enum SoundEffect {
    SE_BUY          = 0x00,                     /* "buy" */
    SE_CANCEL       = 0x01,                     /* "cancel" */
    SE_CAR_WASH     = 0x02,                     /* "car_wash" */
    SE_CURSOR       = 0x03,                     /* "cursor" */
    SE_DISABLE      = 0x04,                     /* "disable" */
    SE_NEXT         = 0x05,                     /* "next" */
    SE_OIL          = 0x06,                     /* "oil" */
    SE_OK           = 0x07,                     /* "ok" */
    SE_PARAM        = 0x08,                     /* "param" */
    SE_RETURN       = 0x09,                     /* "return" */
    SE_SELECT       = 0x0A,                     /* "select" */
    SE_SLIDE        = 0x0B,                     /* "slide" */
    SE_WHEEL        = 0x0C,                     /* "wheel" */
    SE_B_OFF        = 0x0D,                     /* "b_off" (? the blow-off valve): CarSound one-shot 0 */
    SE_BODY_NOIZ    = 0x0E,                     /* "body_noiz": CarSound slot 3 */
    SE_CLASH        = 0x0F,                     /* "clash": one-shot 1 */
    SE_CLASH2       = 0x10,                     /* "clash2": one-shot 2 */
    SE_CLASH3       = 0x11,                     /* "clash3": one-shot 3 */
    SE_CLASH4       = 0x12,                     /* "clash4" */
    SE_CLASH5       = 0x13,                     /* "clash5" */
    SE_COUNT_D      = 0x14,                     /* "count_d": each second of the last 3 of the countdown (u051) */
    SE_COUNT_S      = 0x15,                     /* "count_s": the start (u051, u052) */
    SE_HERI         = 0x16,                     /* "heri" (the helicopter) */
    SE_IN_CAR       = 0x17,                     /* "in_car": CarSound slot 2 */
    SE_OFF_BOUND    = 0x18,                     /* "off_bound" */
    SE_OFF_ROADNOIZ = 0x19,                     /* "off_roadnoiz": wheel slot 2 */
    SE_OFF_SKILL    = 0x1A,                     /* "off_skill": wheel slots 0 and 4 */
    SE_ON_BOUND     = 0x1B,                     /* "on_bound" */
    SE_ON_ROAD      = 0x1C,                     /* "on_road": wheel slot 5 */
    SE_ON_SKILL     = 0x1D,                     /* "on_skill": wheel slot 1 */
    SE_RAIN_NOIZ    = 0x1E,                     /* "rain_noiz": wheel slot 6 */
    SE_RUMBLE       = 0x1F,                     /* "rumble": wheel slot 3 */
    SE_TURBINE      = 0x20,                     /* "turbine": CarSound slot 1 */
    SE_TURBINE2     = 0x21,                     /* "turbine2": CarSound slot 0 */
    SE_AIR          = 0x22,                     /* "air": ambient channel 0 */
    SE_CITY         = 0x23,                     /* "city": ambient channel 1 */
    SE_CROWDS       = 0x24,                     /* "crowds": ambient channel 3 */
    SE_SEA          = 0x25,                     /* "sea": ambient channel 4 */
    SE_TUNNEL       = 0x26,                     /* "tunnel" */
    SE_TUNNEL2      = 0x27,                     /* "tunnel2" */
    /* The enhanced engine (Lupo 2, cameras_sound.md S1): the Toyota Pod's
     * voice, Lupo 2's effects 40..50 (its table, in its alphabetical order;
     * its se.inf entries 42..52, bank 3 = sound/gt3pod.ins, programs 0..10).
     * CarSound plays them by the Pod's emotion (T176); pod_kowai2 is in the
     * table but no emotion plays it. */
    SE_POD_GENKI    = 0x28,                     /* "pod_genki" */
    SE_POD_IYA      = 0x29,                     /* "pod_iya" */
    SE_POD_KOWAI1   = 0x2A,                     /* "pod_kowai1" */
    SE_POD_KOWAI2   = 0x2B,                     /* "pod_kowai2" (not played) */
    SE_POD_KYORO    = 0x2C,                     /* "pod_kyoro" */
    SE_POD_NAKU     = 0x2D,                     /* "pod_naku" */
    SE_POD_NEMUI    = 0x2E,                     /* "pod_nemui" */
    SE_POD_ODOROKU  = 0x2F,                     /* "pod_odoroku" */
    SE_POD_OKORU    = 0x30,                     /* "pod_okoru" */
    SE_POD_SABISHII = 0x31,                     /* "pod_sabishii" */
    SE_POD_WINK     = 0x32,                     /* "pod_wink" */
    SE_TABLE_COUNT  = 0x33                      /* GT3 0x28 */
};

/* The first of the Pod's effects and their number. */
#define SE_POD_FIRST    SE_POD_GENKI
#define SE_POD_COUNT    (SE_TABLE_COUNT - SE_POD_FIRST)

/* The sound effect banks (se.inf code >> 24): GT3's three and, in the
 * enhanced engine, the Pod's (Lupo 2 L2 0x0023B820: sound/gt3pod.ins into
 * bank 3).  SE_BANK_COUNT is in gtsound_internal.h. */
enum {
    SE_BANK_SYS = 0,                            /* sound/gt3sys.ins */
    SE_BANK_SE1 = 1,                            /* sound/gt3se01.ins */
    SE_BANK_SE2 = 2,                            /* sound/gt3se02.ins */
    SE_BANK_POD = 3                             /* sound/gt3pod.ins (the enhanced engine; when the file exists) */
};

/* se_play's pan (0x40 the centre) and prio (< 0: the program's). */
enum {
    SE_PAN_LEFT     = 0,
    SE_PAN_CENTRE   = 0x40,
    SE_PAN_RIGHT    = 0x7F,
    SE_PRIO_PROGRAM = -1
};

/* se_program's `which': 2 selects the effect's program B, any other value
 * program A (CarSound passes 0, RaceAmbientSound 1). */
enum {
    SE_PROGRAM_A = 1,
    SE_PROGRAM_B = 2
};

void se_inf_relocate(struct SeInf *inf);        /* 0x0022C3A0 */
/* Loads the banks and se.inf (once). */
void se_init(void);                             /* 0x0022C3E8 */
void se_term(void);                             /* 0x0022C560 */
/* The se.inf code of effect `se'. */
u32  se_code(int se);                           /* 0x0022C588 */
/* The program of effect `se': B for which == 2, else A. */
SoundProgram *se_program(int se, int which);    /* 0x0022C5B0 (F_0022C5B0) */
/* Sets the effect volume (interrupts off); returns the old one. */
f32  se_set_volume(f32 volume);                 /* 0x0022C620 */
/* Plays effect `se' (pan 0x40 = centre, prio -1: the program's).  When its
 * programs A and B are the same, A plays twice, hard left and hard right,
 * and pan is ignored; else A only. */
void se_play(int se, int pan, int prio, f32 volume);    /* 0x0022C658 (F_0022C658) */
/* The enhanced engine (cameras_sound.md S1): program A of effect `se' when
 * se.inf names it, on bank `bank' (< 0: any) and that bank is loaded and
 * has the program; else NULL.  GT3's se_program has no such checks (its
 * effects are always there); the Pod's may be missing (no gt3pod.ins, an
 * se.inf without the pod_* entries).  Not in Lupo 2. */
SoundProgram *se_find_program(int se, int bank);
/* 0x0022C718 (.dtors 0x0028F24C) / 0x0022C7C8 (.ctors 0x0028F178): egcs's
 * _GLOBAL_$D$ / _GLOBAL_$I$ of T469's bank arrays (compiler-generated, no
 * declaration). */

#ifdef __cplusplus
/* The road noise bank, sound/rn_01.es (u059 copies its header for the
 * third engine bank of a CarSound). */
extern GTSOUNDINSTRUMENT g_roadNoiseBank[1];    /* 0x002E9360 (g_002E9360) */
#endif

#endif /* GAME_T469_SOUND_EFFECTS_H */
