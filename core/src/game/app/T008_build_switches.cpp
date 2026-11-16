/* T008 - constant-returning stubs (? build or configuration switches).
 * 0x00102E38..0x00102E68. */
#include "game/app/T008_build_switches.h"

/* Caller: RaceBase::virtual_29 (0x0017C7A0). */
// FUNC 0x00102E38 F_00102E38
int F_00102E38(void)
{
    return 1;
}

// FUNC 0x00102E40 F_00102E40
int F_00102E40(void)
{
    return 0;
}

// FUNC 0x00102E48 F_00102E48
int F_00102E48(void)
{
    return 0;
}

/* Caller: game_boot (non-zero would set g_002F4230.wide). */
// FUNC 0x00102E50 F_00102E50
int F_00102E50(void)
{
    return 0;
}

/* Caller: RaceIOPLoadModules::cmdSetMediaMode (the sceCdMmode argument). */
// FUNC 0x00102E58 F_00102E58
int F_00102E58(void)
{
    return 2;
}

// FUNC 0x00102E60 F_00102E60
void F_00102E60(void)
{
}
