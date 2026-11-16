/* T008_build_switches.h - constant-returning functions (? build or
 * configuration switches) (translation unit T008, unit u001),
 * 0x00102E38..0x00102E68. */
#ifndef GAME_T008_BUILD_SWITCHES_H
#define GAME_T008_BUILD_SWITCHES_H

#include "game/game.h"

/* 1.  RaceBase::virtual_29 (T129): 0 would set up a temporary RaceInput
 * around the camera update. */
int  F_00102E38(void);                      /* 0x00102E38 */
int  F_00102E40(void);                      /* 0x00102E40 0 (no callers) */
int  F_00102E48(void);                      /* 0x00102E48 0 (no callers) */
int  F_00102E50(void);                      /* 0x00102E50 0; game_boot: non-zero sets g_002F4230 +0x1C (T512) */
/* 2: the IOP loader's media mode (RaceIOPLoadModules::cmdSetMediaMode,
 * IOPLoadModuleBase::setMediaMode: the sceCdMmode argument). */
int  F_00102E58(void);                      /* 0x00102E58 */
void F_00102E60(void);                      /* 0x00102E60 empty (no callers) */

#endif /* GAME_T008_BUILD_SWITCHES_H */
