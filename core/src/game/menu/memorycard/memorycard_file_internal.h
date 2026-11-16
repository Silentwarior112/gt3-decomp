/* memorycard_file_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, units
 * u023 (0x00142790..0x001447D8) and u024 (0x001447D8..0x00147870).
 *
 * Translation unit T108, whole (src/game/menu/memorycard/T108_memorycard_file.cpp): the
 * memory card files of the gt2k_menu save / load screens.  The text helpers
 * (Shift-JIS <-> EUC-JP: card titles are Shift-JIS, the menu font is
 * EUC-JP), MemorycardDir's date / id helpers, the directory operations every
 * MemorycardFile shares (list, size, delete, copy to the other card), the
 * PS1, PS2, other-PS2, damaged, dummy and deleted files, the GT3 save names,
 * MemorycardGT3File's save / load and its four kinds (game data 'G', replay
 * 'R', best lap / licence replay 'B', car setting 'S'), MemorycardFileList
 * (the pool of file cells) and the out-of-line copies egcs emits at the end
 * of the unit: the template destructors, clones, implicit destructors, small
 * virtual functions, inline accessors and typeinfo functions of the whole
 * hierarchy.
 *
 * Every class is defined in its canonical header (game/menu/memorycard/T108_memorycard_file.h
 * and the headers of the classes it uses; T108_memorycard_file.cpp includes
 * them itself, in the order that gives retail's type names); this header
 * adds what only this unit uses: constants, the data of the unit and of
 * other places it reads, and the type_info storage of its typeinfo
 * functions (GCC 14).
 */
#ifndef U023_H
#define U023_H

#include "types.h"

/* ======================================================================== */
/* Constants                                                                */
/* ======================================================================== */
/* T501 lists a save directory 20 entries at a time; the callers keep at
 * most 18 files of it. */
#define MC_DIR_LIST_MAX     20
#define MC_SAVE_FILES_MAX   18

/* The files a save holds. */
#define MC_ICONSYS_FILE_SIZE 0x3C4          /* icon.sys on the card (sceMcIconSys) */
#define MC_ICON_FILE_MAX    0x195D4C        /* the largest .ico readIcon loads */
#define MC_COPY_CHUNK       0x10000         /* copyTo's piece */

/* The built-in icons of MemorycardIconSprites (T072, the names of its
 * items). */
enum {
    MC_ICON_GAME        = 1,                /* "icon_game" */
    MC_ICON_REPLAY      = 2,                /* "icon_replay" */
    MC_ICON_BEST_REPLAY = 3,                /* "icon_best_replay" */
    MC_ICON_SETTING     = 4,                /* "icon_setting" */
    MC_ICON_OTHER       = 5,                /* "icon_other": another game's save without its icon */
    MC_ICON_PS1         = 6,                /* "icon_ps1": a PS1 save without its header */
    MC_ICON_DAMAGE      = 7,                /* "icon_damage" */
    MC_ICON_READING     = 9,                /* "icon_reading": a placeholder while the card is read */
    MC_ICON_LICENSE     = 10,               /* "icon_license" */
    MC_ICON_BLANK       = 12                /* "icon_blank": a deleted file */
};

/* The PS1 icon animation: a frame every 0.17 s. */
#define MC_PS1_FRAME_TIME   0.17f

/* 'Tex1' (Tex1Image.magic). */
#define TEX1_MAGIC          0x31786554

/* MemorycardDir::id (T108_memorycard_file.h; -2 is a placeholder). */
#define MC_DIR_ID_NEW           (-1)        /* not on the card yet (isNewFile) */
#define MC_DIR_ID_OLD_MAX       (-0x400)    /* setOldId: -0x400 - id, not seen by the last listing */

/* The GT3 save names: "BASCUS-97102" (12 characters), the type letter, 7
 * base-64 digits of the code (6 bits each, low digit first). */
#define MC_NAME_PREFIX_LEN      12
#define MC_NAME_TYPE_POS        12
#define MC_NAME_CODE_POS        13
#define MC_NAME_CODE_DIGITS     7
#define MC_NAME_DIGIT_BITS      6
/* The fields of the code (mcMakeName / mcDecodeNameCode). */
#define MC_CODE_CAR_MASK        0xFFF       /* bits 0..11 car index (db_car_index) */
#define MC_CODE_COURSE_SHIFT    12          /* bits 12..20 course index (db_course_index) */
#define MC_CODE_COURSE_MASK     0x1FF
#define MC_CODE_CATEGORY_SHIFT  21          /* bits 21..25 */
#define MC_CODE_CATEGORY_MASK   0x1F
#define MC_CODE_SUBCAT_SHIFT    26          /* bits 26..29 */
#define MC_CODE_SUBCAT_MASK     0xF
#define MC_CODE_FIELDS_MASK     0x3FFFFFFF  /* the 30 bits getNameCode gives */
#define MC_CODE_UNIQUE_SHIFT    30          /* bits 30..41 the number that makes the name unique */
#define MC_CODE_UNIQUE_MASK     0xFFF
#define MC_UNIQUE_TRIES         0x1000      /* makeUniqueName: every 12-bit number once */

/* The save types (the letter of the name; also MemorycardFile::checkSelectable's
 * filter, 0 = any). */
#define MC_TYPE_GAME            'G'
#define MC_TYPE_REPLAY          'R'
#define MC_TYPE_BESTLAP         'B'
#define MC_TYPE_SETTING         'S'

/* GameThumbnail::category of a licence replay (the best lap files of
 * the licence tests): 9..14. */
#define MC_CATEGORY_LICENCE_FIRST 9
#define MC_CATEGORY_LICENCE_COUNT 6

/* MemorycardFileList: the title of the save in icon.sys, and the cards'
 * 1 KB clusters (MemorycardGT3File::calcSize). */
#define MC_TITLE_COMMENT_LEN    16          /* the comment characters that go into the title */
#define MC_CLUSTER_SIZE         1024
#define MC_DIR_ENTRIES_PER_CLUSTER 2        /* 512-byte directory entries */

/* MemorycardFileList: the cell size is the largest file class, 16-aligned. */
#define MC_CELL_ALIGN           16

/* mcCountFiles lists this many entries at a time. */
#define MC_COUNT_CHUNK          16

/* ======================================================================== */
/* Data of the unit                                                         */
/* ======================================================================== */
/* T108 .data (its only variable, in front of the unit's frame block;
 * defined in T108): "BASCUS-97102", built by the first mcGetNamePrefix. */
extern char g_002A2160[16];                 /* 0x002A2160 */
/* -> 0x0031B640 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-":
 * the 64 digits of the name code (defined in T108 before mcDecodeChar: its
 * string follows mcInitIconSys's in .rodata). */
extern const char *g_00351754;              /* 0x00351754 gp-0x629C */

/* The icon files of the kinds (.sdata), {file, NULL} (getIconFiles; defined
 * in T108 in front of each getIconFiles). */
extern const char *g_00351758[2];           /* 0x00351758 gp-0x6298 "menu/icon/ICON_a_spec.ico" */
extern const char *g_00351768[2];           /* 0x00351768 gp-0x6288 "menu/icon/ICON_full.ico" */
extern const char *g_00351778[2];           /* 0x00351778 gp-0x6278 "menu/icon/ICON_best.ico" */
extern const char *g_00351780[2];           /* 0x00351780 gp-0x6270 "menu/icon/ICON_license.ico" */
extern const char *g_00351788[2];           /* 0x00351788 gp-0x6268 "menu/icon/ICON_setting.ico" */

/* ======================================================================== */
/* Data of other places                                                     */
/* ======================================================================== */
/* ASCII -> the full-width EUC-JP character (0xA1A1 the full-width space for
 * ' '), one int per char code; indexed with a signed char. */
extern const int g_003029E8[];              /* 0x003029E8 */

/* The two parts of the prefix, arrays of another unit (only mcGetNamePrefix
 * reads them; lui / addiu, so not small data here): "BA" in the .sdata and
 * "SCUS-97102" in the .rodata of T005..T009 (between T004's holder and
 * T007's "sw_12"; between T005's type names and T009's table). */
extern char g_00350B38[];                   /* 0x00350B38 "BA" */
extern const char g_00308AF0[];             /* 0x00308AF0 "SCUS-97102" */

/* The MEMORYCARDTEXT texts (g_002B1C68, T095; full-width EUC-JP) the unit
 * uses as titles; the names are their keys (g_002B1C28).  CorruptFile: the
 * damaged file's title; the others: the first line of the icon.sys titles. */
enum {
    MCTEXT_CORRUPT_FILE            = 9,     /* 0x002B1C8C -> 0x00325D00 "hason deeta" (damaged data) */
    MCTEXT_GT3_GAME_DATA           = 10,    /* 0x002B1C90 -> 0x00325D10 "GT3 game data" */
    MCTEXT_GT3_REPLAY_DATA         = 11,    /* 0x002B1C94 -> 0x00325D28 "GT3 replay data" + a
                                               full-width space */
    MCTEXT_GT3_LICENSE_REPLAY_DATA = 12,    /* 0x002B1C98 -> 0x00325D40 "GT3 licence replay data" */
    MCTEXT_GT3_BEST_LAP_REPLAY_DATA = 13,   /* 0x002B1C9C -> 0x00325D60 "GT3 best lap replay data" */
    MCTEXT_GT3_SETTING_DATA        = 14     /* 0x002B1CA0 -> 0x00325D88 "GT3 setting data" */
};

/* ======================================================================== */
/* type_info storage of the unit's typeinfo functions                       */
/* ======================================================================== */
/* __si_type_info nodes (3 words), .bss.  egcs generates the __tf functions
 * and their nodes itself; these are for the GCC 14 build only. */
#ifndef BOOT_OLD_GCC
extern "C" {
extern u32 __tiQ29gt2k_menu18MemorycardFileList[3];             /* 0x004B1890 */
extern u32 __tiQ29gt2k_menut4List1ZQ29gt2k_menu14MemorycardFile[3]; /* 0x004B1930 */
extern u32 __tiQ29gt2k_menu21MemorycardSettingFile[3];          /* 0x004B18B0 */
extern u32 __tiQ29gt2k_menu27MemorycardBestlapReplayFile[3];    /* 0x004B1920 */
extern u32 __tiQ29gt2k_menu20MemorycardReplayFile[3];           /* 0x004B18D0 */
extern u32 __tiQ29gt2k_menu18MemorycardGameFile[3];             /* 0x004B1960 */
extern u32 __tiQ29gt2k_menu17MemorycardGT3File[3];              /* 0x004B18E0 */
extern u32 __tiQ29gt2k_menu21MemorycardDeletedFile[3];          /* 0x004B18F0 */
extern u32 __tiQ29gt2k_menu19MemorycardDummyFile[3];            /* 0x004B1950 */
extern u32 __tiQ29gt2k_menu20MemorycardDamageFile[3];           /* 0x004B18A0 */
extern u32 __tiQ29gt2k_menu22MemorycardOtherPS2File[3];         /* 0x004B1940 */
extern u32 __tiQ29gt2k_menu17MemorycardPS2File[3];              /* 0x004B1900 */
extern u32 __tiQ29gt2k_menu17MemorycardPS1File[3];              /* 0x004B18C0 */
extern u32 __tiQ29gt2k_menu14MemorycardFile[3];                 /* 0x004B1910 */
extern u32 __tiQ29gt2k_menut4Cell1ZQ29gt2k_menu14MemorycardFile[3]; /* 0x004B1880 */
}
#endif /* !BOOT_OLD_GCC */

#endif /* U023_H */
