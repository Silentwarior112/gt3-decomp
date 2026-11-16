/* T050 - the country setting.  0x00127898..0x001279C8.
 *
 * main (T000) calls set_country with the saved GameStatus::language after a
 * game is loaded.  The country (T516: 0 Japan, 1 USA, 2 Britain, 3 France,
 * 4 Germany, 5 Italy, 6 Spain; ? 7 another) selects the real-time texts, the
 * course data folder and the default units.  Both switches have a jump
 * table of 8 entries (0x003129F0 / 0x00312A10) whose case 7 does what the
 * default does.
 *
 * Data: the two jump tables only (.rodata 0x003129F0..0x00312A30): the
 * original saw no class with a type name. */
#include "game/data/T050_country.h"
#include "game/course/T179_course_data.h"          /* course_set_region */
#include "game/data/T516_country.h"              /* Country g_00352FD8, COUNTRY_* */
#include "game/data/T518_unit_settings.h"        /* UnitSettings g_002F59D8 */
#include "game/menu/gtmode/gtmode_main_internal.h"

/* The course folder (course_set_region, T179): JP, US, UK, and EU for the four
 * European countries; unchanged for 7 and out-of-range values.  Callers:
 * set_country, u059. */
// FUNC 0x00127898 set_course_region
void set_course_region(int country)
{
    switch (country) {
    case COUNTRY_JP:
        course_set_region(COURSE_REGION_JP);
        break;
    case COUNTRY_US:
        course_set_region(COURSE_REGION_US);
        break;
    case COUNTRY_BR:
        course_set_region(COURSE_REGION_UK);
        break;
    case COUNTRY_FR:
    case COUNTRY_GR:
    case COUNTRY_IT:
    case COUNTRY_SP:
        course_set_region(COURSE_REGION_EU);
        break;
    case COUNTRY_OTHER:
        break;
    }
}

/* The units of preset country + 1 (UnitSettings::setPreset, T518: Japan km/h
 * PS kgf.m, USA mph HP ft.lb, ...); preset 0 (km/h kW NM) for 7 and
 * out-of-range values.  Used only by set_country (no header declaration). */
// FUNC 0x00127908 set_units_for_country
void set_units_for_country(int country)
{
    u32 preset = 0;

    switch (country) {
    case COUNTRY_JP:
        preset = 1;
        break;
    case COUNTRY_US:
        preset = 2;
        break;
    case COUNTRY_BR:
        preset = 3;
        break;
    case COUNTRY_FR:
        preset = 4;
        break;
    case COUNTRY_GR:
        preset = 5;
        break;
    case COUNTRY_IT:
        preset = 6;
        break;
    case COUNTRY_SP:
        preset = 7;
        break;
    case COUNTRY_OTHER:
        break;
    }
    g_002F59D8.setPreset(preset);
}

/* The country word (Country g_00352FD8, gp-0x4A18), the course folder and
 * the units.  Caller: main (T000). */
// FUNC 0x00127988 set_country
void set_country(int country)
{
    g_00352FD8.set(country);
    set_course_region(country);
    set_units_for_country(country);
}
