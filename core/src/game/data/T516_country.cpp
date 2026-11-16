/* T516 - Country (? name): the country / language setting g_00352FD8.
 * The US build starts with COUNTRY_US. */
#include "engine/gfx/vram_internal.h"
#include "game/data/T516_country.h"

/* The one instance, defined first: egcs names the static initialiser /
 * destructor after it, and it opens T516's .sdata (zero-filled: a
 * public object with a constructor), in front of code()'s strings. */
Country g_00352FD8;                             /* 0x00352FD8 gp-0x4A18 */

// FUNC 0x00245390 Country::Country
Country::Country()
{
    value = COUNTRY_US;
}

// FUNC 0x002453A0 Country::~Country
//   Nothing; egcs's deleting form calls operator delete when (flags & 1).
Country::~Country()
{
    /* implicit: operator delete (compiler-generated: bases, members, deleting form) */
}

// FUNC 0x002453C8 Country::code
//   The two-letter code (jump table 0x00343C50, unsigned < 8); NULL for 7
//   and out of range.
const char *Country::code()
{
    switch ((u32)value) {
    case COUNTRY_JP:
        return "JP";
    case COUNTRY_US:
        return "US";
    case COUNTRY_BR:
        return "BR";
    case COUNTRY_FR:
        return "FR";
    case COUNTRY_GR:
        return "GR";
    case COUNTRY_IT:
        return "IT";
    case COUNTRY_SP:
        return "SP";
    case 7:                                 /* (? an eighth value: in the table, no code) */
    default:
        return NULL;
    }
}

// FUNC 0x00245450 Country::isEurope
//   0 for JP / US, 1 for everything else (2..7 and out of range alike).
//   No callers.
int Country::isEurope()
{
    if (value >= 0) {
        if (value < COUNTRY_BR)
            return 0;
        if (value < 8)
            return 1;
    }
    return 1;
}

// FUNC 0x00245480 Country::set
//   Also switches the realtime texts to that language.
void Country::set(int country)
{
    value = country;
    realtime_set_language(country);
}

/* Compiler-generated: egcs's static destructor and initialiser of
 * g_00352FD8 (.dtors entry 0x0028F284, .ctors entry 0x0028F1B4), emitted at
 * the end of the unit, the destructor first: ~Country(2) / Country() on it.
 * Named after the unit's first global, g_00352FD8. */
// FUNC 0x002454A0 T516_static_dtor (_GLOBAL_$D$g_00352FD8)
/* implicit: ~Country (the static destructor of g_00352FD8) */

// FUNC 0x002454C0 T516_static_init (_GLOBAL_$I$g_00352FD8)
/* implicit: Country (the static initialiser of g_00352FD8) */
