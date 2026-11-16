/* T427_menu_status.h - gt2k_game::MenuStatus, the menu state (translation
 * unit T427, unit u097), 0x00217C30..0x00218250.  (? class name, as in u001 /
 * u013 / u017; no RTTI: the namespace follows GameStatus's.)
 *
 * GameStatus+0x307C8, its last member: the page the menus go to next, the
 * current event, the cars of the shop / garage screens and of the race
 * modes, remembered cursors and arcade choices, the link battle nodes'
 * GameOptions.
 * T427 then has out-of-line copies of the header-inline GarageCar /
 * Cell<GarageCar> destructors and typeinfo functions (0x00218110
 * Cell<GarageCar>::~Cell, 0x00218138 __tf9GarageCar, 0x00218198
 * GarageCar::~GarageCar, 0x00218200 __tfQ29gt2k_menut4Cell1Z9GarageCar), all
 * unreferenced: they belong to GarageCar (T036_garage.h), not here. */
#ifndef GAME_T427_MENU_STATUS_H
#define GAME_T427_MENU_STATUS_H

#include "game/game.h"
#include "game/menu/gtmode/T036_garage_car.h"               /* GarageCar (by value; not T036_garage.h:
                                                   T036_garage_car.h says why) */
#include "game/save/T423_cursor_memory.h"            /* CursorMemory (by value) */
#include "game/save/T428_game_options.h"             /* GameOptions (by value) */

namespace gt2k_menu {
class Car;                                      /* pointers only */
}

namespace gt2k_game {

/* No vtable.  Size 0x4A48 (GameStatus ends with it). */
class MenuStatus {
public:
    /* The scalars before focusCar, the two GarageCars, car .. wheelIndex,
     * the CursorMemory, parts .. gameCardPort1C, the 8 GameOptions; the body
     * clears the three strings. */
    MenuStatus();                               /* 0x00217C30 */
    void reset();                               /* 0x00217DF8 str84, cursor, the game card ports and options are kept */
    void setPage(const char *page, int arg);    /* 0x00217E70 NULL clears it (pageArg -1) */
    const char *getPage(char *out);             /* 0x00217EC0 copies it to out ("root" if none), returns page */
    /* The car of player 1 (player1 != 0) or 2: car `code' built in that
     * player's buffer (kind 0) / `car' itself (kind 1) / a copy of `car' in
     * the buffer (kind 2). */
    void setRaceCarByCode(const char *code, int player1);  /* 0x00217F20 */
    void setRaceCar(GarageCar *car, int player1);          /* 0x00217F88 */
    void setRaceCarCopy(const GarageCar *car, int player1);/* 0x00217FB0 */
    void setEvent(const char *code);            /* 0x00218018 */
    void setStr84(const char *s);               /* 0x00218048 */
    void setCar(GarageCar *car);                /* 0x00218078 car = car; menu_car_spec()->setCar(car's CarInformation) */
    void setFocusCar(const char *code);         /* 0x002180B0 */
    void clearFocusCar();                       /* 0x00218100 */

    char         page[0x40];                    /* 0x000 page to go to ("" = "root") */
    int          pageArg;                       /* 0x040 */
    char         event[0x40];                   /* 0x044 code of the current race / event */
    char         str84[0x40];                   /* 0x084 the free run's course (action_go_free_run's arg, T037; mode 8 gives it to the free run, T000) */
    int          nextMode;                      /* 0x0C4 the mode main() dispatches on next */
    int          raceMode;                      /* 0x0C8 (u013: the nextMode of "go_race"; ? u008 / u050: 2 = the next race of a series) */
    int          runMode;                       /* 0x0CC default 8 (u001: set before a mode is entered) */
    int          raceCarKind[2];                /* 0x0D0 [player 1, 2]: 0 by code, 1 garage entry, 2 copy, 3 none */
    GarageCar    focusCar;                      /* 0x0D8 player 1's car buffer, the dealer's car */
    GarageCar    raceCarBuf2;                   /* 0x308 player 2's car buffer */
    GarageCar   *car;                           /* 0x538 car of the garage / shop screens */
    gt2k_menu::Car *carItem;                    /* 0x53C its 3D view (make_car_render) */
    GarageCar   *raceCar[2];                    /* 0x540 the cars the race modes use ([0]: u013's settingCar, the car of the setting pages) */
    char         wheelName[0x40];               /* 0x548 the wheel chosen at the wheel shop */
    int          color;                         /* 0x588 the colour chosen in car_color_box */
    int          garageIndex;                   /* 0x58C index of the selected garage car */
    int          wheelIndex;                    /* 0x590 index of the selected wheel */
    CursorMemory cursor;                        /* 0x594 (T423) */
    int          parts;                         /* 0x5D8 default 0x58: the focused part */
    int          arcadeMode;                    /* 0x5DC the last arcade mode (ArcadeControl::mode) */
    int          arcadeLevel;                   /* 0x5E0 the last arcade level page cursor */
    int          arcadeCourse;                  /* 0x5E4 the arcade course select's course (T025), kept for arcadeMode */
    int          arcadeClass;                   /* 0x5E8 default 0: the class last chosen on the arcade class select page (u003, T022) */
    int          lastArcadeCar;                 /* 0x5EC car index the arcade car select starts on (T022 clears it when the class changes) */
    int          gameCardPort;                  /* 0x5F0 the port of the last game load / save (modes 0x19..0x1B, the autoload) */
    int          gameCardPort1D;                /* 0x5F4 default 1: the same for mode 0x1D */
    int          gameCardPort1C;                /* 0x5F8 default 1: the same for mode 0x1C */
    u8           unk_5FC[4];                    /* 0x5FC ? (options are 8-byte aligned) */
    ::GameOptions options[8];                   /* 0x600 the game options of the 8 i.LINK battle nodes (T473's F_0022D3C0 loads each node's) */
    int          replayViewMode;                /* 0x4A40 the display mode F_0020E930 chose last (reset: -1) */
    int          replayCount;                   /* 0x4A44 demo replays since mode 1 was forced */
};

} /* namespace gt2k_game */
EGCS_SIZE(gt2k_game::MenuStatus, 0x4A48);

#endif /* GAME_T427_MENU_STATUS_H */
