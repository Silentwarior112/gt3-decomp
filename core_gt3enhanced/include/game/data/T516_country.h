/* T516_country.h - the country / language setting (? name: Country)
 * (translation unit T516, unit u112), 0x00245390..0x002454E0.
 *
 * The one instance is 0x00352FD8 (gp-0x4A18, the start of T516's .sdata,
 * zero-filled; built by T516's static constructor): 0 JP, 1 US (the US build's start value), 2 BR (UK),
 * 3 FR, 4 GR, 5 IT, 6 SP; also the language index of text/realtime.dat (T517).
 * The other units read it as an int (value is its only member). */
#ifndef GAME_T516_COUNTRY_H
#define GAME_T516_COUNTRY_H

#include "game/game.h"

/* Country::value (also the language block of text/realtime.dat). */
enum {
    COUNTRY_JP = 0,
    COUNTRY_US = 1,
    COUNTRY_BR = 2,                             /* Britain (UK) */
    COUNTRY_FR = 3,
    COUNTRY_GR = 4,                             /* Germany */
    COUNTRY_IT = 5,
    COUNTRY_SP = 6                              /* Spain */
};

/* Not polymorphic. */
class Country {
public:
    Country();                                  /* 0x00245390 value = 1 */
    ~Country();                                 /* 0x002453A0 */
    const char *code();                         /* 0x002453C8 "JP", "US", "BR", "FR", "GR", "IT", "SP"; else NULL */
    int  isEurope();                            /* 0x00245450 (? name) 0 for JP / US, else 1 */
    void set(int c);                            /* 0x00245480 value = c, realtime_set_language(c) */

    int value;                                  /* 0x0 */
};
EGCS_SIZE(Country, 0x4);

extern Country g_00352FD8;                      /* 0x00352FD8 gp-0x4A18 the country / language */

/* T516 defines g_00352FD8; egcs generates its static destructor
 * _GLOBAL_$D$g_00352FD8 0x002454A0 (.dtors 0x0028F284, ~Country) and
 * initialiser _GLOBAL_$I$g_00352FD8 0x002454C0 (.ctors 0x0028F1B4, Country()). */

#endif /* GAME_T516_COUNTRY_H */
