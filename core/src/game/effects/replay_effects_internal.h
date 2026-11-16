/* replay_effects_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u061,
 * 0x001AC870..0x001AFA60: translation units T182..T187 and T196.
 *
 *   T182  ReplayBlur: the depth of field of the replays (RacePS2Base +0x97CC).
 *   T183  race_draw_vignette: the ring the course environment draws over
 *         the replays (course flag 0x40).
 *   T184  CarIcon / CarIconMaker: the 64 x 32 icons of the cars of a race,
 *         drawn by the render thread.
 *   T185  ErrorEventView: the error messages of the race event queue
 *         (RacePS2Base +0xBCD0).
 *   T186  GT2KMSGRACEDISPLAY::Message: the "RACEDISPLAY" text table (56).
 *   T187  GT2KMSGRACEMSG::Message: the "RACEMSG" text table (1).
 *   T196  ReplayOption: the option window of the replay pause menu.
 *
 * Every shared class, function and global this unit uses is declared in the
 * canonical headers.  Each source includes the ones it needs itself, in the
 * order that gives the original's type names in its .rodata (egcs writes
 * the names of the classes a unit sees, in reverse order of definition: the
 * data pass, ref/data/u061/notes.md); this header keeps only what is local
 * to the unit: named constants and the declarations of the translation
 * units' own data (defined in them), and includes nothing but types.h.
 *
 * Offsets in comments are the ORIGINAL ones (egcs / GCC 2.9x ee layout).
 */
#ifndef U061_H
#define U061_H

#include "types.h"

/* ======================================================================== */
/* Shared named constants                                                   */
/* ======================================================================== */
/* gfx_blend_func selectors: (A - B) * C / 128 + D (PGL_*: G001_flare.h). */
#define BLEND_ALPHA     PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_AS, PGL_SEL_CD)    /* 0x44 Cs As + Cd (1 - As) */
#define BLEND_ADD       PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_ZERO, PGL_SEL_AS, PGL_SEL_CD)  /* 0x48 Cs As + Cd */
#define BLEND_SUBTRACT  PGL_BLEND_SEL(PGL_SEL_CS, PGL_SEL_CD, PGL_SEL_FIX, PGL_SEL_ZERO) /* 0xA4 (Cs - Cd) FIX */

/* EE rsqrt.s fd, fs, ft: fs / sqrt(ft), a COP1 instruction egcs never
 * emits.  A volatile asm as in the other units (u032 icon_rsqrt, u055,
 * u056, u057: rsqrt.s behaves like one), a macro so that no inline body
 * shifts the unit's label numbers; num / sqrt under GCC 14. */
#if defined(BOOT_OLD_GCC) && !defined(NON_MATCHING)
#define u061_rsqrtf(num, sq)                                                  \
    ({ f32 u061_rsqrt_r_;                                                     \
       __asm__ __volatile__("rsqrt.s %0, %1, %2" : "=f"(u061_rsqrt_r_)        \
                            : "f"((f32)(num)), "f"((f32)(sq)));               \
       u061_rsqrt_r_; })
#else
#define u061_rsqrtf(num, sq)  ((f32)(num) / boot_sqrtf((f32)(sq)))
#endif

/* ln 2: the blur levels are 2^(k / steps) pixels (expf(k ln2 / steps)). */
#define BLUR_LN2        0.6931472f

/* The 640 x 448 screen (the frame copy, the depth inversion, the levels). */
enum { SCREEN_W = 640, SCREEN_H = 448 };

/* ======================================================================== */
/* T182 .rodata                                                             */
/* ======================================================================== */
/* One level of the downsampled chain inside ReplayBlur::levels (640 x 224). */
struct BlurRect {
    int x, y, w, h;
};
enum { BLUR_LEVELS = 5 };
/* 0x00330BD0: {0,0,320,224} {320,0,320,112} {320,112,160,112}
 * {480,112,160,56} {480,168,160,56}: each level half the previous one in
 * one direction, alternately.  T182's .rodata (defined there). */
extern const BlurRect g_00330BD0[BLUR_LEVELS];

/* ======================================================================== */
/* T184                                                                     */
/* ======================================================================== */
/* The light of the icons: light 1 (PGL_CAP_LIGHT0 + 1), T184's .data
 * (defined there, in this order). */
extern f32 g_002C3EF8[4];                   /* 0x002C3EF8 PGL_LIGHT_POSITION: (1, 0.5, -10.5, 0), directional */
extern f32 g_002C3F08[4];                   /* 0x002C3F08 PGL_LIGHT_AMBIENT: 0.8, 0.8, 0.8, 1 */
extern f32 g_002C3F18[4];                   /* 0x002C3F18 PGL_LIGHT_DIFFUSE: 1.5, 1.5, 1.5, 1 */
extern f32 g_002C3F28[4];                   /* 0x002C3F28 PGL_LIGHT_SPECULAR: 1, 1, 1, 1 */
/* (0x00352248, T184's .sdata, the word 32: drawIcon's constant pool, the
 * int it converts to float; no variable.) */

/* ======================================================================== */
/* T185                                                                     */
/* ======================================================================== */
/* Race event types: RACE_EVENT_* of T122_race_event_queue.h. */
enum { ERROR_SHOW_FRAMES = 120 };           /* how long each message stays */
enum { ERROR_ENTRIES = 1 };                 /* ErrorEventView::entries: one node */

/* ======================================================================== */
/* T186 / T187 .data / .sdata                                               */
/* ======================================================================== */
/* The tables are defined in T186 / T187, the instances g_002C44C8 /
 * g_002C45A8 too (their headers declare them).  g_00352388 is also
 * RaceEntryCar's default driver name (T121_race_entry.h declares it). */
extern const char *g_002C4308[56];          /* 0x002C4308 the RACEDISPLAY keys ("TimeLimit" ..) */
extern const char *g_00352380[1];           /* 0x00352380 gp-0x5670 the RACEMSG key "SessionBest" */
extern const char *g_00352388[1];           /* 0x00352388 gp-0x5668 the RACEMSG text "SESSION BEST" */

/* ======================================================================== */
/* T196                                                                     */
/* ======================================================================== */
/* ReplayOption::mode. */
enum { REPLAY_OPTION_1P = 0, REPLAY_OPTION_2P = 1 };

/* ReplayOption::images: where each row's images start, the image of
 * display mode i is images[REPLAY_IMG_MODE + i] and so on. */
enum {
    REPLAY_IMG_MODE          = 0,           /* 4: mode_traditional, mode_avantgarde, mode_broadcast, mode_crs */
    REPLAY_IMG_ITEM          = 4,           /* 7: item_easymeter, carname, time | map, steering, tire, accel */
    REPLAY_IMG_ITEM2         = 7,           /* the second items row (items 3..6) */
    REPLAY_IMG_TARGET        = 11,          /* 3: target_manual, target_top, target_auto */
    REPLAY_IMG_SOUND         = 14,          /* 2: sound_bgm, sound_se */
    REPLAY_IMG_NAMED         = 16,          /* the images named by g_002C4690 */
    REPLAY_IMG_SCREEN_SINGLE = 17,          /* 2P: screen_single */
    REPLAY_IMG_SCREEN_SPLIT  = 18           /* 2P: screen_v_split (wide screen) / screen_h_split */
};
enum {
    REPLAY_VIEW_MODES = 4,                  /* traditional, avantgarde, broadcast, crs */
    REPLAY_TARGETS    = 3,                  /* manual (the car), top (the leader), auto */
    REPLAY_ITEMS1     = 3,                  /* the columns of the first items row */
    REPLAY_ITEMS2     = 4,                  /* of the second (items 3..6) */
    REPLAY_ITEM_CARNAME = 1                 /* item 1, disabled in a split-screen replay */
};
/* ReplayOption::colors (the images color_off .. color_select_on). */
enum {
    REPLAY_COLOR_OFF = 0, REPLAY_COLOR_ON = 1, REPLAY_COLOR_DISABLE = 2,
    REPLAY_COLOR_SELECT_OFF = 3, REPLAY_COLOR_SELECT_ON = 4, REPLAY_COLORS = 5
};
/* The pad auto-repeat of the option window (Control::getRepeat). */
enum { REPLAY_REPEAT_FIRST = 30, REPLAY_REPEAT_INTERVAL = 3 };
/* menu_play_se sounds: T233's MENU_SE_CANCEL / MENU_SE_CURSOR / MENU_SE_CONFIRM. */
/* The cursor's blink: 0.5 down to 0 by 1/60 a frame. */
#define REPLAY_BLINK_START  0.5f
#define REPLAY_BLINK_STEP   0.016666668f

/* T196's .data (defined there, in this order; their names in its .rodata).
 * g_002C4690: [0..3] mode_traditional, mode_avantgarde, mode_broadcast,
 * mode_crs; [4..10] item_easymeter, item_carname, item_time, item_map,
 * item_steering, item_tire, item_accel; [11..13] target_manual, target_top,
 * target_auto; [14..15] sound_bgm, sound_se. */
extern const char *g_002C4690[REPLAY_IMG_NAMED];   /* 0x002C4690 */
extern const char *g_002C46D0[3];           /* 0x002C46D0 screen_single, screen_v_split, screen_h_split */
extern const char *g_002C46E0[REPLAY_COLORS];   /* 0x002C46E0 color_off, color_on, color_disable,
                                               color_select_off, color_select_on */
extern f32 g_002C4700[4][3];                /* 0x002C4700 the cursor quad (a strip): (-1,1,0) (1,1,0)
                                               (-1,-1,0) (1,-1,0); 16-byte aligned */

/* ======================================================================== */
/* Typeinfo storage of the unit's classes (filled by their __tf functions)  */
/* ======================================================================== */
#ifndef BOOT_OLD_GCC                        /* egcs generates them itself */
extern "C" {
extern u32 __ti12CarIconMaker[3];           /* 0x004B23A0 __si_type_info */
extern u32 __ti14ErrorEventView[2];         /* 0x003533D8 gp-0x4618 __user_type_info */
extern u32 __tiQ218GT2KMSGRACEDISPLAY7Message[3];  /* 0x004B23B0 __si_type_info */
extern u32 __tiQ214GT2KMSGRACEMSG7Message[3];      /* 0x004B23C0 __si_type_info */
extern u32 __ti12ReplayOption[3];           /* 0x004B23D0 __si_type_info */
}
#endif

#endif /* U061_H */
