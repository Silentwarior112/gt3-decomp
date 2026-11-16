/* T108_game_thumbnail.h - gt2k_game::GameThumbnail, the "thumbnail" file of
 * a GT3 save (translation unit T108, units u023 / u024; the struct of
 * T108_memorycard_file.h, which includes this header where it defined it).
 *
 * In a header of its own (u108): the savers' describe functions (T488's
 * SettingSerialize::describe) need only this struct, and
 * T108_memorycard_file.h's templates put the type names of
 * List<MemorycardFile> / Cell<MemorycardFile> into every object that
 * includes it (retail T488's .rodata has none). */
#ifndef GAME_T108_GAME_THUMBNAIL_H
#define GAME_T108_GAME_THUMBNAIL_H

#include "game/game.h"

namespace gt2k_game {

/* The "thumbnail" file of a GT3 save, 0x60 bytes (MemorycardGT3File::
 * thumbnail; Lupo 2's name, from setThumbnail(gt2k_game::GameThumbnail &);
 * the first pass called it gt2k_menu::MemorycardThumbnail): written by the
 * saved object's Serialize::Base::describe (T509) and the save screen
 * (T068), shown by the info panels (T069; the names are those of their
 * "*_pt_*" items).  type: 0 game data (GameStatus,
 * T429), 1 car setting (SettingSerialize, T488), 2 replay, 3 best lap /
 * licence replay (T150).  category / subCategory are what the save name
 * encodes (the licence screens put the licence and test + 1 there). */
struct GameThumbnail {
    enum {                                  /* type */
        TYPE_GAME    = 0,
        TYPE_SETTING = 1,
        TYPE_REPLAY  = 2,
        TYPE_BESTLAP = 3
    };

    int  type;                              /* 0x00 */
    u8   unk_04[4];                         /* 0x04 */
    union {                                 /* 0x08 .. 0x28 by type */
        struct {                            /* 0 MemorycardGameFile */
            s64 money;                      /* 0x08 "g_pt_money" */
            int cars;                       /* 0x10 "g_pt_cars" */
            u32 licence;                    /* 0x14 "g_pt_license": the highest licence passed (< 6) */
            int days;                       /* 0x18 "g_pt_day" */
            f32 achievement;                /* 0x1C "g_pt_g_achievement" (percent) */
            u8  unk_20[8];                  /* 0x20 */
        } game;
        struct {                            /* 1 MemorycardSettingFile */
            u64 car;                        /* 0x08 "s_pt_car" */
            u64 course;                     /* 0x10 "s_pt_course" */
            f32 power;                      /* 0x18 "s_pt_power" (PS) */
            u8  unk_1C[0xC];                /* 0x1C */
        } setting;
        struct {                            /* 2 MemorycardReplayFile */
            int time;                       /* 0x08 "r_pt_time" */
            u8  unk_0C[4];                  /* 0x0C */
            u64 car;                        /* 0x10 "r_pt_car" */
            u64 course;                     /* 0x18 "r_pt_course" */
            int laps;                       /* 0x20 "r_pt_laps" */
            f32 power;                      /* 0x24 "r_pt_power" (PS) */
        } replay;
        struct {                            /* 3 MemorycardBestlapReplayFile */
            union {
                int time;                   /* 0x08 "b_pt_time" */
                f32 speed;                  /* 0x08 the same for licence 0x11 (a top speed) */
            };
            u8  unk_0C[4];                  /* 0x0C */
            u64 car;                        /* 0x10 "b_pt_car" */
            u64 course;                     /* 0x18 "b_pt_course" */
            f32 power;                      /* 0x20 "b_pt_power" (PS) */
            f32 mass;                       /* 0x24 "b_pt_mass" (kg) */
        } bestlap;
    };
    char comment[0x11];                     /* 0x28 "*_pt_comment" (16 characters; 16 go into the title) */
    char licenceName[0x1B];                 /* 0x39 "*_pt_mode": "<licence name>[<test>]" */
    int  category;                          /* 0x54 best lap files: 9..14 a licence test (its icon);
                                               0x11: bestlap.speed instead of the time */
    int  subCategory;                       /* 0x58 */
    u8   unk_5C[4];                         /* 0x5C */
};

} /* namespace gt2k_game */

EGCS_SIZE(gt2k_game::GameThumbnail, 0x60);

#endif /* GAME_T108_GAME_THUMBNAIL_H */
