/* T068_memorycard_control_class.h - gt2k_menu::MemorycardControl, the
 * controller of the memory card screens ("menu/memorycard.mbl")
 * (translation unit T068, unit u026), 0x001499B8..0x0014B610: the class
 * alone, with what its members need.
 *
 * F_0014AE00 (main, the arcade / GT-mode menus, the race menus, the logger,
 * the settings) builds the controller on the stack over a 384 KB menu heap
 * (memalign) and runs one mode:
 *   0x0C          load a demo replay of the disc (MemorycardDemoFileManager
 *                 for both slots, page "demo_loader"), sort 8
 *   0x10          save: a MemorycardReplayFile of "ReplayData", sort 7
 *   0x12, 0x14    save: a MemorycardBestlapReplayFile of "GhostData"
 *                 ("MaxSpeed" for licence 0x11), sort 7
 *   0x16          save: a MemorycardSettingFile of "SettingSerialize", sort 7
 *   0x19          save the game: a MemorycardGameFile of "GameStatus", page
 *                 "gamesave" (the slot menu of T075), sort 7
 *   0x11 / 0x13 / 0x15 / 0x17   loads, sort 8 / 9 / 0xC / 0xA
 *   0x18          the file utility ("file_operation" from the game menus)
 *   0x1A          load the game (page "gameload"); 0x1B the autoload (page
 *                 "autoload"); 0x1C / 0x1D page "gameload_g"
 * A save serialises its object (T509, by class name) into a malloc'd buffer
 * and builds the card file around it (thumbnail, date, size).  begin() sets
 * the screens up; one of three loops runs (fileOperation: the file list of
 * T071; gameMenu: the slot menu of T075; autoLoad); then end().  The rest
 * are accessors the menus of T069..T076 call.
 *
 * In its own header since u026's data pass (2026-10-17): retail's T068
 * .rodata has the type names of T073's menu items (List / Cell<
 * MemorycardMenuItem>, MemorycardMenuItem: defined before MemorycardControl)
 * but none of T076's dialogs, so the original's T068 saw the class without
 * T075's screens.  This header brings only what the members need (the two
 * plain structs of T075 from T075_memorycard_slot_menu_root.h);
 * T068_memorycard_control.h is the class with T075's header in front, the
 * view every other includer had so far (same classes, same order). */
#ifndef GAME_T068_MEMORYCARD_CONTROL_CLASS_H
#define GAME_T068_MEMORYCARD_CONTROL_CLASS_H

#include "game/game.h"
#include "game/menu/framework/T247_control_base.h"         /* base gt2k_menu::ControlBase */
#include "game/menu/framework/T246_render_base.h"          /* gt2k_menu::RenderBase (render) */
#include "game/menu/framework/T211_page_manager.h"         /* gt2k_menu::PageManager (pages), ExecuteFuncData */
#include "game/app/T079_demo_file_manager.h"    /* gt2k_menu::MemorycardDemoFileManager (demo) */
#include "game/menu/memorycard/T109_memorycard_file_manager.h"  /* gt2k_menu::MemorycardFileManager (files) */
#include "game/menu/memorycard/T071_memorycard_slots.h"     /* gt2k_menu::MemorycardSlotSet (slots) */
#include "game/menu/memorycard/T075_memorycard_slot_menu_root.h"    /* gt2k_menu::MemorycardSlotMenuRoot (slotMenu),
                                               MemorycardAutoLoad */
#include "game/menu/memorycard/T108_memorycard_file.h"      /* gt2k_game::GameThumbnail (thumbnail) */
#include "game/menu/memorycard/T068_memorycard_entry.h"     /* F_0014AE00, its modes (MODE_* below) */

namespace gt2k_menu {
class MemorycardFile;                       /* T108 (pointers only) */
struct MemorycardSlot;                      /* T071 */

} /* namespace gt2k_menu */

/* MemorycardLoadInfo, F_0014AE00's a8 (the thumbnail and date of a loaded
 * save): in its own header since u018's data pass (the logger has it by
 * value without seeing this header's classes). */
#include "game/menu/memorycard/T068_memorycard_load_info.h"

namespace gt2k_menu {

/* vtable 0x0031C808 (2 slots: [1] ControlBase's waitFrame 0x001CBD30).
 * typeinfo __tfQ29gt2k_menu17MemorycardControl 0x0014B5B8 (si, base
 * ControlBase; storage 0x004B1990).  ControlBase's data 0x000 .. 0x288, vptr
 * +0x288.  Size 0x307A0 (F_0014AE00's stack object: its locals start at
 * sp+0x307A0; 8-aligned through the u64s). */
class MemorycardControl : public ControlBase {
public:
    /* F_0014AE00's mode: the screen run (? names; the callers pass these:
     * T000, T052..T055, T080, T102's race_memcard); the values are
     * T068_memorycard_entry.h's (the callers that do not see this class). */
    enum MODE {
        MODE_DEMO_REPLAY_LOAD = MEMCARD_DEMO_REPLAY_LOAD,
        MODE_REPLAY_SAVE      = MEMCARD_REPLAY_SAVE,
        MODE_REPLAY_LOAD      = MEMCARD_REPLAY_LOAD,
        MODE_GHOST_SAVE       = MEMCARD_GHOST_SAVE,
        MODE_GHOST_LOAD       = MEMCARD_GHOST_LOAD,
        MODE_GHOST_SAVE2      = MEMCARD_GHOST_SAVE2,
        MODE_LOGGER_LOAD      = MEMCARD_LOGGER_LOAD,
        MODE_SETTING_SAVE     = MEMCARD_SETTING_SAVE,
        MODE_SETTING_LOAD     = MEMCARD_SETTING_LOAD,
        MODE_FILE_UTILITY     = MEMCARD_FILE_UTILITY,
        MODE_GAME_SAVE        = MEMCARD_GAME_SAVE,
        MODE_GAME_LOAD        = MEMCARD_GAME_LOAD,
        MODE_AUTOLOAD         = MEMCARD_AUTOLOAD,
        MODE_GAME_LOAD_GUEST  = MEMCARD_GAME_LOAD_GUEST,
        MODE_GAME_LOAD_GTMODE = MEMCARD_GAME_LOAD_GTMODE
    };
    /* state: what the screen loops run next (the menus set it). */
    enum STATE {
        STATE_GAME_MENU   = 0,              /* gameMenu: the page with the slot menu */
        STATE_FILE_SCREEN = 1,              /* fileOperation: the file page; gameMenu: the file
                                               utility (fileOperation(MODE_FILE_UTILITY)) */
        STATE_NAME_ENTRY  = 2,              /* fileOperation: the name entry, then the page again */
        STATE_LEAVE       = 3               /* the loop ends */
    };

    /* Inline: only F_0014AE00 builds one (T068 defines it).  ControlBase,
     * the vptr, the members in order (render, pages(this), demo's inline
     * constructor, files[0..1], slots(this), slotMenu), then workBuf /
     * workSize, files[i].setPort(i), the words cleared. */
    inline MemorycardControl(void *buf, int size);
    /* Inline (F_0014AE00 expands it; 0x0014B468 is the copy for the
     * vtable): frees data and newFile, then the members and ~ControlBase. */
    inline virtual ~MemorycardControl();    /* [0] +0x0C 0x0014B468 */

    void load();                            /* 0x001499B8 pages: "menu/memorycard.mbl" */
    void unload();                          /* 0x001499E0 */
    /* The managers of both slots (the demo manager twice in mode 0x0C), the
     * page's hooks (g_002A4E20, g_00351810), the list's category and sort,
     * and for a save the file to save (newFile, data, thumbnail). */
    void begin(int mode, u64 race, u64 car, int licence, int test, void *arg);   /* 0x00149A00 */
    void end();                             /* 0x0014A088 */
    void fileOperation(int mode);           /* 0x0014A108 the file screens' loop ("file_operation",
                                               "demo_loader"; state 2 runs the name entry) */
    void gameMenu(int mode);                /* 0x0014A3C0 the game load / save loop (state 1 runs the
                                               file utility) */
    void autoLoad(int mode);                /* 0x0014A590 the autoload at boot (MemorycardAutoLoad) */
    MemorycardFile *getSelectedFile();      /* 0x0014A6B8 slots.getSelectedFile() */
    MemorycardSlot *getCurrentSlot();       /* 0x0014A6E0 slots.getCurrentSlot(): NULL if none */
    MemorycardSlot *getOtherSlot();         /* 0x0014A708 NULL without a current slot */
    int  getCurrentPort();                  /* 0x0014A750 -1 if none; 0x100: the disc's demo files */
    int  getOtherPort();                    /* 0x0014A778 -1 if none */
    MemorycardFileManagerBase *getCurrentManager(); /* 0x0014A7C0 NULL if none */
    MemorycardFileManagerBase *getOtherManager();   /* 0x0014A7E8 NULL if none */
    int  isSaveMode();                      /* 0x0014A830 0x10, 0x12, 0x16, 0x19 */
    int  isLoadMode();                      /* 0x0014A888 0x0C, 0x11, 0x13, 0x15, 0x17, 0x1A..0x1D */
    void setSort(int sort);                 /* 0x0014A8D0 both managers' setSort */
    void setEffectiveFilter(bool on);       /* 0x0014A938 both managers' setEffectiveFilter (on passed as
                                               is: a bool): true for the loads, false for the saves
                                               (first pass: setMatchOnly) */
    void pauseThreads();                    /* 0x0014A9A0 the managers' reader threads, a frame at a time
                                               until paused */
    void restartThreads();                  /* 0x0014AAD0 */
    void assignIconHeaps();                 /* 0x0014ABC8 T077's two icon heaps over the halves of
                                               workBuf (once) */
    void releaseIconHeaps();                /* 0x0014AC90 */
    /* load, begin, loadInfo, the loop of the mode, end, unload; result. */
    int  run(int mode, u64 race, u64 car, int licence, int test, void *arg,
             MemorycardLoadInfo *loadInfo); /* 0x0014ACE0 */

    /*                          ControlBase    0x00000 .. 0x0028C (vptr +0x288) */
    RenderBase                  render;     /* 0x0028C */
    PageManager                 pages;      /* 0x002E4 */
    MemorycardDemoFileManager   demo;       /* 0x00E40 the demo replays of the disc (mode 0x0C) */
    MemorycardFileManager       files[2];   /* 0x00E50 ports 0 and 1 */
    MemorycardFileManagerBase  *manager[2]; /* 0x30620 &files[0], &files[1]; &demo twice in mode 0x0C */
    MemorycardSlotSet           slots;      /* 0x30628 the file screens' two slots (T071) */
    MemorycardSlotMenuRoot      slotMenu;   /* 0x30694 the slot menu of the game load / save (T075) */
    int                         mode;       /* 0x306AC MODE (the running loop's: 0x18 in the file utility) */
    /* STATE, of the screen loops, set by the menus: fileOperation 1 run the
     * page, 2 name entry, 3 leave; gameMenu 0 run, 1 file utility, 3 leave. */
    int                         state;      /* 0x306B0 */
    int                         result;     /* 0x306B4 run()'s result (set by the menus) */
    bool                        paused;     /* 0x306B8 the reader threads are paused (bool: copied
                                               as is into MemorycardSlot::paused, T071) */
    /* The file to save: a MemorycardGT3File (begin), but typed as the base:
     * the dynamic_casts of it in T074 (SaveWorkingMenu, 0x00155184) and T075
     * (0x001588B8) pass __tfQ29gt2k_menu14MemorycardFile as the source type. */
    MemorycardFile             *newFile;    /* 0x306BC */
    void                       *data;       /* 0x306C0 its serialised data (malloc) */
    int                         dataSize;   /* 0x306C4 */
    gt2k_game::GameThumbnail    thumbnail;  /* 0x306C8 the save's (serialiser's describe + licence) */
    MemorycardLoadInfo         *loadInfo;   /* 0x30728 F_0014AE00's a8 (u026: int unk30728) */
    /*                          (alignment)    0x3072C */
    u64                         race;       /* 0x30730 the race / course code of the list */
    u64                         car;        /* 0x30738 the car code */
    /* The list's category / sub-category (the managers' setSelectCategory /
     * SubCategory): the race menus pass race_name_split's kind (RaceKind,
     * T489: 9..14 the licences, 17 the VMAX test) and index. */
    int                         licence;    /* 0x30740 the kind, >= 0 */
    int                         test;       /* 0x30744 index + 1, 0 without a kind */
    void                       *arg;        /* 0x30748 F_0014AE00's a3: a MemCardRaceInfo, a load's data */
    /* Where the selected file's icon is (screen centre, y up): [0] [1] the
     * list cursor's icon (IconView), also where the operation menu opens;
     * [2] [3] the "op" place (the menu's position + its +0x3C / +0x40);
     * [4] [5] the "confirm" / "working" place (u030: confirmIconX / Y). */
    f32                         iconPos[6]; /* 0x3074C (u026 / u028 / u031: unk3074C) */
    f32                         progressPrev;   /* 0x30764 the progress bar (T069 ProgressBar) */
    f32                         progress;   /* 0x30768 0..1 (u027: progressDone / progressTarget) */
    void                       *workBuf;    /* 0x3076C F_0014AE00's buffer (the icon heaps) */
    int                         workSize;   /* 0x30770 */
    int                         iconHeaps;  /* 0x30774 the icon heaps are set up */
    int                         commentMode;    /* 0x30778 the CommentMenu of the name entry (T074) */
    char                        comment[0x24];  /* 0x3077C the name entry's text (16 characters) */
};
EGCS_SIZE(gt2k_menu::MemorycardControl, 0x307A0);

} /* namespace gt2k_menu */

/* F_0014AE00 (the entry point) and its modes: T068_memorycard_entry.h,
 * included above. */

/* (The page's ExecuteFuncData table g_00351810, which the plan put here, is
 * T070's data: T070_memorycard_actions.h.) */
/* (The Serialize class names g_00351F40 "ReplayData" / g_00351F54
 * "GhostData" are T150's data: T150_serialize_replay_data.h.) */

#ifndef BOOT_OLD_GCC                        /* egcs generates it itself */
extern "C" {
void *__tfQ29gt2k_menu17MemorycardControl(void);   /* 0x0014B5B8 */
}
#endif

#endif /* GAME_T068_MEMORYCARD_CONTROL_CLASS_H */
