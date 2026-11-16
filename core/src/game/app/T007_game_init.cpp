/* T007 - the game's start-up and the race begin / end bracket.
 * 0x00102B58..0x00102E38. */
/* The include order is the original's data order: <new> first (the
 * "bad_alloc" of its inline what() opens the .rodata, 0x00308E78), the
 * GameStatus before the arenas before the menu classes (egcs writes the
 * type names in the reverse order of the classes' definitions: retail's
 * OperatorList .. Cell<OperatorHolder>, UnitArena<64>, the garage's, then
 * Operator, a base of OperatorHolder whose name comes with it). */
// DATA_START 0x00308E78
#include <new>
#include "game/app/app_internal.h"
#include "libc.h"                           /* malloc */
#include "engine/pgl/G001_flare.h"                /* gfx_init, gfx_init_default */
#include "engine/media/G002_mpeg_frameq.h"          /* scf_lock_init */
#include "runtime/newlib/G007_stdio.h"                /* strcmp */
#include "game/app/T002_render_thread.h"        /* render_thread_start / release */
#include "game/app/T004_game_status_holder.h"   /* getGameStatusHolder */
#include "game/app/T007_game_init.h"
#include "game/app/T008_build_switches.h"       /* F_00102E50 */
#include "game/app/T009_race_iop_load_modules.h"    /* RaceIOPLoadModules, IOPLoadModuleBase */
#include "game/race/core/T123_race_setup.h"           /* race_pool_assign */
#include "game/course/T179_course_data.h"          /* course_table_init */
#include "game/menu/race_menu/T197_race_menu_page.h"       /* race_menu_heap_alloc / race_menu_heap_free */
#include "game/menu/gtmode/T215_showroom_data.h"        /* showroom_load */
#include "game/menu/gtmode/T232_menu_car_spec.h"        /* menu_car_spec_create */
#include "game/race/dynamics/T318_control_record.h"       /* AutomobileControlRecord::allocBuffers / freeBuffers */
#include "game/data/T435_product_config.h"       /* product_config_init */
#include "game/car/model/T446_car_model.h"            /* car_list_load */
#include "game/car/model/T449_race_tire_model.h"      /* brake_file_load / brake_file_free */
#include "game/sound/T470_sound_init.h"           /* sound_init */
#include "engine/system/T473_link_session.h"         /* F_0022D320 */
#include "engine/input/T475_pad_manager.h"          /* PadManager g_002EA7F8 */
#include "game/data/T481_car_data_base.h"        /* CarDataBase g_002EBD88 */
#include "game/data/T485_car_color.h"            /* CarColorDataBase g_00352D98 */
#include "game/data/T487_race_mode_database.h"   /* RaceModeDatabase g_002F00C0 */
#include "engine/text/T496_font.h"                 /* Font g_002F1968 (the system font) */
#include "engine/io/T503_vol_thread.h"           /* vol_init */
#include "engine/io/T508_loader_thread.h"        /* loader_init */
#include "engine/gfx/T512_render_context.h"       /* RenderContext g_002F4230 */
#include "engine/text/T517_realtime_text.h"        /* realtime_load */
#include "game/save/T429_game_status.h"          /* createGameStatus */
/* UnitArena<64> instantiated by T180's g_002C2F50, with its type name
 * (T153_race_course.h leaves it out unless asked). */
#define GAME_T153_EMITS_TYPEINFO
#include "game/race/core/T153_race_course.h"          /* RaceCourse::initArena / freeArena / loadCameras /
                                               unloadCameras */
#include "game/car/model/T180_race_car_model.h"       /* RaceCarModel::initArena / freeArena */
#include "game/menu/framework/T216_menu_transitive.h"      /* (no use: the menu classes the original saw) */

/* Before every race (13 callers): the course and car model arenas, the
 * control record pool (`records' buffers of `recordSize' bytes: callers
 * pass 0x20000 and 2, the i.LINK battle 8), the course cameras,
 * "race/brake.bin" and the race menu heap. */
// FUNC 0x00102B58 race_begin
void race_begin(int recordSize, int records)
{
    RaceCourse::initArena();
    RaceCarModel::initArena();
    AutomobileControlRecord::allocBuffers(recordSize, records);
    RaceCourse::loadCameras();
    brake_file_load("race/brake.bin");
    race_menu_heap_alloc();
}

/* Undoes race_begin in reverse order. */
// FUNC 0x00102BB8 race_end
void race_end(void)
{
    race_menu_heap_free();
    brake_file_free();
    RaceCourse::unloadCameras();
    AutomobileControlRecord::freeBuffers();
    RaceCarModel::freeArena();
    RaceCourse::freeArena();
}

/* The system font: its file, its kanji, the default face "sw_12", size 1.
 * A function of its own in the original, defined here: egcs writes the
 * strings of a function when it expands it, and retail's .sdata has "sw_12"
 * before game_init's "gt3.vol" (0x00350B48 / 0x00350B50), while its .rodata
 * keeps "race/brake.bin" (race_begin) before the font names. */
static inline void system_font_init(void)
{
    g_002F1968.loadFile("font/system.fnt");
    g_002F1968.loadKanjiFont("font/system.kf");
    g_002F1968.setDefaultFace("sw_12");
    g_002F1968.setSize(1.0f, 1.0f);
}

/* hot (started by the bootstrap with argv[1] == "hot"): gfx_init(1, 1, 2,
 * 0) keeps the display and the render thread starts with the boot-screen
 * fade; otherwise gfx_init_default(2).  Then the render context, the IOP
 * modules (iop->load(), virtual slot 1), gt3.vol, the real-time texts, the
 * pads, the system font, sound, the car / colour / race mode databases, the
 * car list, the course data, the race object pool (0x22000 bytes of malloc),
 * the link session, the loader, the product config, the menu car spec, the
 * GameStatus, the showroom data, the guest GameStatus recreated, and the
 * render thread let run. */
// FUNC 0x00102C00 game_init
void game_init(IOPLoadModuleBase *iop, int hot)
{
    scf_lock_init();
    if (hot != 0) {
        gfx_init(1, 1, 2, 0);
        render_thread_start(1);
    } else {
        gfx_init_default(2);
    }
    g_002F4230.init();
    iop->load();
    vol_init("gt3.vol");
    realtime_load();
    g_002EA7F8.init();
    system_font_init();                     /* implicit: loadFile, loadKanjiFont, setDefaultFace, setSize */
    sound_init();
    g_002EBD88.load();
    g_00352D98.load();
    g_002F00C0.load();
    car_list_load();
    course_table_init();
    race_pool_assign(malloc(RACE_POOL_SIZE), RACE_POOL_SIZE);
    F_0022D320();
    loader_init();
    product_config_init();
    menu_car_spec_create();
    createGameStatus();
    showroom_load();
    getGameStatusHolder()->release();
    getGameStatusHolder()->create();
    render_thread_release();
}

/* Called first by main.  The RaceIOPLoadModules object lives on the stack
 * (sp+0x00) for the duration of game_init.  F_00102E50 (T008) is 0: the
 * render context's `wide' flag stays off. */
// FUNC 0x00102D98 game_boot
void game_boot(int argc, char **argv)
{
    int hot = 0;
    if (argc >= 2)
        hot = (strcmp(argv[1], "hot") == 0);
    RaceIOPLoadModules iop;
    game_init(&iop, hot);
    if (F_00102E50() != 0)
        g_002F4230.wide = 1;
}
