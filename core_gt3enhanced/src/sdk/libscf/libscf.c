/* libscf.c - Sony EE libscf (assert file name "libscf.c"),
 * 0x002745C0..0x00274F70: the whole object, one file (the work units split
 * it at 0x00274AD0; its tail, from scf_clock_bin_to_bcd, lies in u131's
 * range and was u131's G004_libscf_u131.c until the merge).
 *
 * System-configuration getters: each one returns the value from the OSD
 * configuration (GetOsdConfigParam word, and for date/time notation and
 * summer time the GetOsdConfigParam2 byte at offset 1), unless the machine
 * is a T10K (rom0:ROMVER character 4 == 'T'), where the values set with
 * sceScfSetT10kConfig are returned instead.  With an OSD config of version
 * 0 (old ROMs) the newer fields are absent and fixed defaults are used
 * (time zone 0x21C = +9 h, date notation 0, ...).  Several getters print the
 * value they read ("Timezone=%d\n", ...): debug prints left in the library.
 *
 * Clock arithmetic on an sceCdCLOCK read from the console RTC.  The RTC
 * keeps Japan time (JST = GMT + 9 h = 540 minutes) in BCD; the two public
 * functions at the end shift it to GMT or to the local time of the OSD
 * settings (time zone + summer time).  scf_clock_adjust converts the clock
 * to binary (scf_clock_bcd_to_bin), steps minutes / hours / days / months /
 * years (two-digit year 0..99, leap year = year % 4 == 0), and converts it
 * back to BCD.  The clock functions assert prtc != NULL (newlib __assert
 * with the libscf.c line number).  The game reaches the two public functions
 * through G002's scf_clock_to_gmt / scf_clock_to_local (u123).
 *
 * Names: Sony SDK libscf names for the public getters and conversions (they
 * fit the field each one reads); the internal helpers are named by what
 * they do.
 */
#include "sdk/libmc/libmc_internal.h"

/* ======================================================================== */
/* Data (file statics; .data 0x00305E90..0x00305EB8, no .bss)               */
/* ======================================================================== */

/* The library id string opens the object's .data (16 chars, no NUL of its
 * own; referenced by no code), as libpad's and libmc's. */
// DATA_START 0x00305E90
static BOOT_USED char s_scfVersion[16] = "PsIIlibscf  2200"; /* 0x00305E90 */
/* The T10K values (sceScfSetT10kConfig replaces them): JST (+9 h), the
 * other fields 0. */
static sceScfT10kConfig s_scfT10kConfig = {           /* 0x00305EA0 */
    SCF_JST_OFFSET_MIN, 0, 0, 0, 0, 0, 0
};
/* rom0:ROMVER cache (SCF_ROMVER_READ_SIZE = 14 bytes read; [0] == 0 = not
 * read yet, [4] == 'T' on a T10K).  Initialised (all zero) in the original:
 * it is in .data, and egcs puts only uninitialised statics in .bss.  16
 * bytes: the object's .data ends at 0x00305EB8, where libgcc2's begins (14
 * or 15 would leave the same zero padding there: the bytes cannot tell). */
static char s_scfRomVer[16] = { 0 };                  /* 0x00305EA8 */

// FUNC 0x002745C0 scf_get_romver
/* Reads rom0:ROMVER (14 bytes) once into the cache s_scfRomVer (tested by its
 * first byte, a signed `lb`) and returns the cache.  Error handling is only
 * a message: a failed sceOpen (-1) still goes on to sceRead / sceClose with
 * fd -1. */
char *scf_get_romver(void)
{
    int fd;

    if (s_scfRomVer[0] == 0) {
        fd = sceOpen("rom0:ROMVER", SCE_RDONLY);
        if (fd == -1)
            printf("Can't open rom0:ROMVER\n");
        if (sceRead(fd, s_scfRomVer, SCF_ROMVER_READ_SIZE) == -1)
            printf("Can't read rom error\n");
        sceClose(fd);
    }
    return s_scfRomVer;
}

// FUNC 0x00274660 scf_is_t10k
/* 1 when ROMVER character 4 is 'T': a T10K tool.  Reads ROMVER first if the
 * cache is still empty (the result pointer is unused).  Char compare is on
 * a signed `lb` byte. */
int scf_is_t10k(void)
{
    if (s_scfRomVer[0] == 0)
        scf_get_romver();
    return s_scfRomVer[SCF_ROMVER_REGION] == SCF_ROMVER_T10K;
}

// FUNC 0x002746A0 sceScfGetLanguage
/* T10K: s_scfT10kConfig.Language.  Else the OSD config: with version 0 the
 * single language bit 4 (0 Japanese, 1 English), otherwise bits 16-20.
 * Quirk: GetOsdConfigParam is called twice, once before the T10K test (that
 * result is never used). */
int sceScfGetLanguage(void)
{
    u32 config;
    int language;

    GetOsdConfigParam(&config);              /* dead call */
    if (scf_is_t10k())
        language = s_scfT10kConfig.Language;
    else {
        GetOsdConfigParam(&config);
        if (SCF_OSD_VERSION(config) == 0)
            language = SCF_OSD_JAP_LANGUAGE(config);
        else
            language = SCF_OSD_LANGUAGE(config);
    }
    return language;
}

// FUNC 0x00274700 sceScfSetT10kConfig
/* Copies the caller's 8-byte block into s_scfT10kConfig (an unaligned
 * ldl/ldr + sdl/sdr doubleword copy).  No callers. */
void sceScfSetT10kConfig(sceScfT10kConfig *config)
{
    s_scfT10kConfig = *config;
}

// FUNC 0x00274720 sceScfGetAspect
/* T10K: s_scfT10kConfig.Aspect; else OSD config bits 1-2 (screen type). */
int sceScfGetAspect(void)
{
    u32 config;

    if (scf_is_t10k())
        return s_scfT10kConfig.Aspect;
    GetOsdConfigParam(&config);
    return SCF_OSD_ASPECT(config);
}

// FUNC 0x00274760 sceScfGetSpdif
/* T10K: s_scfT10kConfig.Spdif; else OSD config bit 0. */
int sceScfGetSpdif(void)
{
    u32 config;

    if (scf_is_t10k())
        return s_scfT10kConfig.Spdif;
    GetOsdConfigParam(&config);
    return SCF_OSD_SPDIF(config);
}

// FUNC 0x002747A0 sceScfGetTimeZone
/* Minutes from GMT.  T10K: s_scfT10kConfig.TimeZone (signed short, `lh`).
 * Else 0x21C (540 = +9 h, Japan) for an OSD config of version 0, otherwise
 * the signed 11-bit field at bits 21-31 (arithmetic shift), printed.
 * Caller: sceScfGetLocalTimefromRTC (below). */
int sceScfGetTimeZone(void)
{
    u32 config;
    int timezone;

    if (scf_is_t10k())
        timezone = s_scfT10kConfig.TimeZone;
    else {
        timezone = SCF_DEFAULT_TIMEZONE;
        GetOsdConfigParam(&config);
        if (SCF_OSD_VERSION(config) != 0) {
            timezone = SCF_OSD_TIMEZONE(config);
            printf("Timezone=%d\n", timezone);
        }
    }
    return timezone;
}

/* The three getters below read one byte of the OSD config 2:
 * GetOsdConfigParam2(&byte, 1, 1) copies the byte at offset 1 into a local
 * at sp+4 (the u32 config is at sp+0) and only that byte is read back
 * (`lbu 0x4($sp)`), so the buffer is the single byte the size argument
 * asks for: a ScfOsdConfig2Time (libmc_internal.h; a struct, which is what puts it
 * at sp+4).  The byte is loaded into $v1, which needs GetOsdConfigParam2
 * declared as returning int (eekernel.h): as a void function the load takes
 * $v0. */

// FUNC 0x00274808 sceScfGetDateNotation
/* T10K: s_scfT10kConfig.DateNotation.  Else 0 for an OSD config of version
 * 0, otherwise bits 6-7 of the OSD config 2 byte at offset 1, printed. */
int sceScfGetDateNotation(void)
{
    ScfOsdConfig2Time config2;
    u32 config;
    int notation;

    if (scf_is_t10k())
        notation = s_scfT10kConfig.DateNotation;
    else {
        GetOsdConfigParam(&config);
        if (SCF_OSD_VERSION(config) == 0)
            notation = 0;
        else {
            GetOsdConfigParam2(&config2, sizeof(config2), SCF_OSD2_TIME_BYTE);
            notation = config2.dateNotation;
            printf("DateNotation=%d\n", notation);
        }
    }
    return notation;
}

// FUNC 0x00274888 sceScfGetSummerTime
/* T10K: s_scfT10kConfig.SummerTime.  Else 0 for an OSD config of version 0,
 * otherwise bit 4 of the OSD config 2 byte at offset 1, printed.
 * Caller: sceScfGetLocalTimefromRTC (below). */
int sceScfGetSummerTime(void)
{
    ScfOsdConfig2Time config2;
    u32 config;
    int summer_time;

    if (scf_is_t10k())
        summer_time = s_scfT10kConfig.SummerTime;
    else {
        GetOsdConfigParam(&config);
        if (SCF_OSD_VERSION(config) == 0)
            summer_time = 0;
        else {
            GetOsdConfigParam2(&config2, sizeof(config2), SCF_OSD2_TIME_BYTE);
            summer_time = config2.summerTime;
            printf("SummerTime=%d\n", summer_time);
        }
    }
    return summer_time;
}

// FUNC 0x00274908 sceScfGetTimeNotation
/* T10K: s_scfT10kConfig.TimeNotation.  Else 0 for an OSD config of version
 * 0, otherwise bit 5 of the OSD config 2 byte at offset 1, printed. */
int sceScfGetTimeNotation(void)
{
    ScfOsdConfig2Time config2;
    u32 config;
    int notation;

    if (scf_is_t10k())
        notation = s_scfT10kConfig.TimeNotation;
    else {
        GetOsdConfigParam(&config);
        if (SCF_OSD_VERSION(config) == 0)
            notation = 0;
        else {
            GetOsdConfigParam2(&config2, sizeof(config2), SCF_OSD2_TIME_BYTE);
            notation = config2.timeNotation;
            printf("TimeNotation=%d\n", notation);
        }
    }
    return notation;
}

// FUNC 0x00274988 scf_bin_to_bcd
/* Binary 0..99 -> packed BCD: (c / 10) * 6 + c (unsigned divide).
 * assert(c <=99) at libscf.c line 281 (0x119).  Caller:
 * scf_clock_bin_to_bcd (below). */
u8 scf_bin_to_bcd(u8 c)
{
    SCF_ASSERT(c <=99, 281);        /* implicit: __assert (the macro's call) */
    return (u8)((c / 10) * 6 + c);
}

// FUNC 0x002749F0 scf_bcd_to_bin
/* Packed BCD 0x00..0x99 -> binary: c - (c >> 4) * 6.
 * assert(c <= 0x99) at libscf.c line 294 (0x126).  The adjustment is a u8
 * local: retail keeps the product in LO (`mult $0, ...; mflo`), which the
 * QImode temporary gives; written in place, egcs multiplies into a GPR. */
u8 scf_bcd_to_bin(u8 c)
{
    u8 adjust;

    SCF_ASSERT(c <= 0x99, 294);     /* implicit: __assert (the macro's call) */
    adjust = (c >> 4) * 6;
    return c - adjust;
}

// FUNC 0x00274A48 scf_clock_bcd_to_bin
/* Converts an RTC reading (sceCdCLOCK, BCD fields) to binary in place, in
 * the order year, month, day, hour, minute, second (stat and pad are left
 * alone).  assert(prtc != NULL) at libscf.c line 306 (0x132).
 * Caller: scf_clock_adjust (below). */
void scf_clock_bcd_to_bin(sceCdCLOCK *prtc)
{
    SCF_ASSERT(prtc != NULL, 306);  /* implicit: __assert (the macro's call) */
    prtc->year   = scf_bcd_to_bin(prtc->year);
    prtc->month  = scf_bcd_to_bin(prtc->month);
    prtc->day    = scf_bcd_to_bin(prtc->day);
    prtc->hour   = scf_bcd_to_bin(prtc->hour);
    prtc->minute = scf_bcd_to_bin(prtc->minute);
    prtc->second = scf_bcd_to_bin(prtc->second);
}

// FUNC 0x00274AD0 scf_clock_bin_to_bcd
/* Binary -> BCD for year, month, day, hour, minute, second (in that order;
 * stat and pad untouched). */
void scf_clock_bin_to_bcd(sceCdCLOCK *prtc)
{
    SCF_ASSERT(prtc != NULL, 321);  /* implicit: __assert (the macro's call) */
    prtc->year   = scf_bin_to_bcd(prtc->year);
    prtc->month  = scf_bin_to_bcd(prtc->month);
    prtc->day    = scf_bin_to_bcd(prtc->day);
    prtc->hour   = scf_bin_to_bcd(prtc->hour);
    prtc->minute = scf_bin_to_bcd(prtc->minute);
    prtc->second = scf_bin_to_bcd(prtc->second);
}

// FUNC 0x00274B58 scf_clock_next_day
/* One day forward (binary clock): past the month's last day -> day 1 of the
 * next month; past December -> January of the next year (99 wraps to 0).
 * The month table is a local copy of the .rodata initialiser at 0x00347C68;
 * February gets 29 days when year % 4 == 0 (the table is signed char: lb).
 * Matching: `year % 4' (shortened to an unsigned char and, not the `& 3' of
 * the first pass) keeps the year load out of GCSE's reach, so the year is
 * loaded again for the == 99 test as in retail. */
void scf_clock_next_day(sceCdCLOCK *prtc)
{
    char days_in_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    SCF_ASSERT(prtc != NULL, 336);  /* implicit: __assert (the macro's call) */
    prtc->day++;
    if (prtc->year % 4 == 0)
        days_in_month[1] = 29;
    if (days_in_month[prtc->month - 1] < prtc->day) {
        prtc->day = 1;
        prtc->month++;
        if (prtc->month == 13) {
            if (prtc->year == 99)
                prtc->year = 0;
            else
                prtc->year++;
            prtc->month = 1;
        }
    }
}

// FUNC 0x00274C40 scf_clock_prev_day
/* One day back: day 0 -> the last day of the previous month; month 0 ->
 * December of the previous year (0 wraps to 99).  The leap-year test uses
 * the year before any wrap (harmless: it only matters for February). */
void scf_clock_prev_day(sceCdCLOCK *prtc)
{
    char days_in_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    SCF_ASSERT(prtc != NULL, 360);  /* implicit: __assert (the macro's call) */
    prtc->day--;
    if (prtc->year % 4 == 0)
        days_in_month[1] = 29;
    if (prtc->day == 0) {
        prtc->month--;
        if (prtc->month == 0) {
            if (prtc->year == 0)
                prtc->year = 99;
            else
                prtc->year--;
            prtc->month = 12;
        }
        prtc->day = days_in_month[prtc->month - 1];
    }
}

// FUNC 0x00274D18 scf_clock_next_hour
void scf_clock_next_hour(sceCdCLOCK *prtc)
{
    SCF_ASSERT(prtc != NULL, 385);  /* implicit: __assert (the macro's call) */
    prtc->hour++;
    if (prtc->hour == 24) {
        prtc->hour = 0;
        scf_clock_next_day(prtc);           /* tail call */
    }
}

// FUNC 0x00274D88 scf_clock_prev_hour
void scf_clock_prev_hour(sceCdCLOCK *prtc)
{
    SCF_ASSERT(prtc != NULL, 398);  /* implicit: __assert (the macro's call) */
    if (prtc->hour == 0) {
        prtc->hour = 23;
        scf_clock_prev_day(prtc);           /* tail call */
    } else {
        prtc->hour--;
    }
}

// FUNC 0x00274DF0 scf_clock_adjust
/* Adds diff minutes (|diff| <= 24 h) to a BCD clock.  Quirk of the library,
 * kept: the forward loop runs while minute > 60 (slti 61), so a sum of
 * exactly 60 is stored as minute 60 instead of carrying into the hour. */
void scf_clock_adjust(sceCdCLOCK *prtc, int diff)
{
    int minute;

    SCF_ASSERT(prtc != NULL, 416);  /* implicit: __assert (the macro's call) */
    SCF_ASSERT(-60*24<=diff && diff <= 60*24, 417);  /* implicit: __assert (the macro's call) */
    scf_clock_bcd_to_bin(prtc);
    minute = prtc->minute + diff;
    if (minute >= 0) {                      /* this order: retail's block layout */
        while (minute > 60) {
            minute -= 60;
            scf_clock_next_hour(prtc);
        }
    } else {
        while (minute < 0) {
            minute += 60;
            scf_clock_prev_hour(prtc);
        }
    }
    prtc->minute = minute;
    scf_clock_bin_to_bcd(prtc);             /* tail call */
}

// FUNC 0x00274EC0 sceScfGetGMTfromRTC
/* RTC (JST) -> GMT: -540 minutes.  (Name: the libscf API this matches.) */
void sceScfGetGMTfromRTC(sceCdCLOCK *prtc)
{
    SCF_ASSERT(prtc != NULL, 444);  /* implicit: __assert (the macro's call) */
    scf_clock_adjust(prtc, -SCF_JST_OFFSET_MIN);    /* tail call */
}

// FUNC 0x00274F08 sceScfGetLocalTimefromRTC
/* RTC (JST) -> local time: + time zone (minutes from GMT) + 60 if summer
 * time - 540.  The two getters run before the NULL assert. */
void sceScfGetLocalTimefromRTC(sceCdCLOCK *prtc)
{
    int diff = sceScfGetTimeZone() - SCF_JST_OFFSET_MIN + sceScfGetSummerTime() * 60;

    SCF_ASSERT(prtc != NULL, 455);  /* implicit: __assert (the macro's call) */
    scf_clock_adjust(prtc, diff);           /* tail call */
}
