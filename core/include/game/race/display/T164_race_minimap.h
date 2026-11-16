/* T164_race_minimap.h - RaceMiniMap, the course map of the race display
 * (its methods: translation unit T164, unit u056), split from
 * T154_race_display.h (u059's data pass), which includes it where the class
 * was defined: its includers see the same classes in the same order.
 *
 * Why a header of its own: RaceMiniMap's virtual functions are all inline
 * (no key function), so egcs writes its type name "11RaceMiniMap" into
 * every object that sees the class.  Retail's T179 (CourseData, u059) has
 * it between UnitArena<64>'s and RaceCameraBase's names and none of the
 * other race display classes (RaceDisplayBase, the parts, the meters): the
 * original T179 saw the mini map class alone (for the course file's mini
 * map record, CourseData +0x84). */
#ifndef GAME_T164_RACE_MINIMAP_H
#define GAME_T164_RACE_MINIMAP_H

#include "game/game.h"
#include "game/race/display/T156_race_display_object.h"  /* base RaceDisplayObjectBase */

class RaceOrganization;                     /* T125_race_organization.h (pointers only) */
class RaceEntryCar;                         /* T121_race_entry.h */
struct Tex1Image;                           /* G001_flare.h */
struct RaceMiniMapData;                     /* T164 (the course file's mini map record; no header) */
class type_info;

/* The typeinfo function (vtable entry 0), declared before the class for
 * GCC 14 only (T154_race_display.h says why). */
#ifndef BOOT_OLD_GCC
extern "C" {
const type_info &__tf11RaceMiniMap(void);                   /* 0x001959F0 (T164 0x0019E6A0) */
}
#endif

/* ======================================================================== */
/* RaceMiniMap                                                              */
/* ======================================================================== */

/* The course map with a dot per car (RaceDisplay +0x20).  vtables 0x0032E290
 * (T154; the constructor stores it) and 0x0032FCF0 (T164), 10 slots (only
 * [0] its own), typeinfo __tf11RaceMiniMap 0x001959F0 (si, base
 * RaceDisplayObjectBase).  0x30 bytes.  The methods are T164's (u056). */
class RaceMiniMap : public RaceDisplayObjectBase {
public:
    RaceMiniMap();                          /* 0x0019E340 (T164) */
    virtual ~RaceMiniMap() {}               /* [0] +0x0C 0x001959C8 (T164 copy 0x0019E678) */
    /* ? names.  The course's map record (CourseData +0x84), its image's
     * size, the dots "1pcar_dot" / "ecar_dot". */
    void init(RaceOrganization *org);                       /* 0x0019E380 (T164) */
    /* The map and every car's dot (the target car's last). */
    void drawMap(RaceOrganization *org, int targetCar);     /* 0x0019E420 (T164) */
    void drawCar(RaceEntryCar *car, Tex1Image *dot);        /* 0x0019E560 (T164) */

    f32              scaleX;                /* 0x18 (setLayout: w / 100) */
    f32              scaleY;                /* 0x1C (h / 100) */
    s16              mapW;                  /* 0x20 the map image's size (CLAMP MAXU + 1) */
    s16              mapH;                  /* 0x22 (CLAMP MAXV + 1) */
    RaceMiniMapData *data;                  /* 0x24 the course's map record */
    Tex1Image       *playerDot;             /* 0x28 "1pcar_dot" (u051 / u052 unk28) */
    Tex1Image       *enemyDot;              /* 0x2C "ecar_dot" (u051 / u052 unk2C) */
};
EGCS_SIZE(RaceMiniMap, 0x30);

#endif /* GAME_T164_RACE_MINIMAP_H */
