/* T428 (0x00218250..0x002185D8): GameOptions (? name, as in u014 / u021),
 * the game options of the save data (GameStatus+0x2FF38, 0x888 bytes; 8
 * more in MenuStatus+0x600): their defaults, copy, load and save.  Word 0
 * (`wide', the 16:9 screen) lives in the render context (g_002F4230.wide,
 * 0x002F424C) while the game runs: apply() copies GameStatus's value there,
 * fetch() copies it back before a save.
 *
 * Retail's .rodata (0x003404A0..0x00340586, unreferenced: type names only):
 * the GameStatus's garage type names (GarageWheelList .. Cell<GarageCar>).
 * .lit4: 0.1f (replaySoundLevel). */
// DATA_START 0x003404A0
#include "game/save/game_options_internal.h"
#include "game/camera/T394_replay_view_mode.h" /* ReplayViewInfo, F_0020EA58 (the views' default targets) */
#include "engine/gfx/T512_render_context.h"   /* g_002F4230 (its wide flag, 0x002F424C) */
#include "game/save/T429_game_status.h"      /* getGameStatus, gt2k_game::GameStatus; the garage */
#include "game/save/T428_game_options.h"

// FUNC 0x00218250 GameOptions::GameOptions
GameOptions::GameOptions()
{
    /* implicit: ControllerConfig (compiler-generated: bases, members, deleting form) */
    /* the member constructor ControllerConfig::ControllerConfig (config),
     * then setDefaults().  The original constructs nothing for `music'
     * (MusicPlayList has no constructor; setDefaults calls its init). */
    setDefaults();
}

/* The first 0x638 bytes as one block (inline ld/sd copy), the controller
 * settings with their operator=, then apply(). */
// FUNC 0x00218288 GameOptions::operator=
GameOptions &GameOptions::operator=(const GameOptions &o)
{
    memcpy((void *)this, &o, 0x638);
    config = o.config;
    apply();
    return *this;
}

/* The calls in the original's order: memset, the 16 targets, memset, the
 * play list, the keys (the word stores are scheduled around them).  The
 * stores in retail's order: the sound options together (menu, race,
 * replay), assistAsm / assistTcs before the split battle block. */
// FUNC 0x00218318 GameOptions::setDefaults
void GameOptions::setDefaults()
{
    int view;

    wide = 0;
    menuBgm = 1;
    menuSe = 1;
    raceBgm = 1;
    raceSe = 1;
    replayBgm = 1;
    replaySe = 1;
    vibration1p = 1;
    vibration2p = 1;
    raceLaps = 2;
    tireDamage = 1;
    gearAuto = 1;
    ghost = 1;
    assistAsm = 1;
    assistTcs = 1;
    splitBattle.raceLaps = 2;
    splitBattle.tireDamage = 0;
    splitBattle.handicap = 0;
    splitBattle.boost = 1;
    steeringAssist1p = 0;
    forceFeedback1p = 1;
    steeringAssist2p = 0;
    forceFeedback2p = 1;
    displayFlags = 0;
    replaySplit = 1;
    memset(unk_064, 0, sizeof(unk_064));
    viewMode = 0;
    /* the default camera target of view mode i (mode 0 past the table) */
    for (view = 0; view < 16; view++)
        target[view] = F_0020EA58(view > 3 ? 0 : view)->defTarget;
    replaySoundLevel = 0.1f;
    replayCutStyle = 0;
    onboardMount[0] = 0;
    onboardMount[1] = 0;
    memset(unk_124, 0, sizeof(unk_124));
    /* 0x0021ABB0 on music: a new seed, every song of BGM group 0x14,
     * shuffled (a plain method: it does not return this). */
    music.init();
    config.reset();
}

/* (Called by option_action_set_default, T067.) */
// FUNC 0x00218460 GameOptions::reset
void GameOptions::reset()
{
    setDefaults();
    apply();
}

// FUNC 0x00218488 GameOptions::setWide
void GameOptions::setWide(int w)
{
    wide = w;
    apply();
}

// FUNC 0x002184A8 GameOptions::getSize
int GameOptions::getSize()
{
    return ControllerConfig::getSize() + 0x638;
}

// FUNC 0x002184C8 GameOptions::load
const u8 *GameOptions::load(const u8 *src)
{
    const u8 *end;
    u32 n = 0x638;                              /* in a variable, egcs calls memcpy */

    memcpy((void *)this, src, n);
    end = config.load(src + n);
    apply();
    return end;
}

// FUNC 0x00218518 GameOptions::save
u8 *GameOptions::save(u8 *dst)
{
    u32 n = 0x638;

    fetch();
    memcpy(dst, this, n);
    return config.save(dst + n);
}

/* GameStatus's options, not this object's. */
// FUNC 0x00218568 GameOptions::apply
void GameOptions::apply()
{
    g_002F4230.wide = getGameStatus()->getGameOptions()->wide;  /* 0x002F424C */
}

// FUNC 0x002185A0 GameOptions::fetch
void GameOptions::fetch()
{
    getGameStatus()->options.setWide(g_002F4230.wide);  /* 0x002F424C */
}
