/* T522_pdimpeg.h - pdimpeg.cxx: the movie streams over the IOP
 * (translation unit T522, unit u119). */
#ifndef GAME_T522_PDIMPEG_H
#define GAME_T522_PDIMPEG_H

#include "game/game.h"

/* The file's RPC objects (an array of two at 0x004A73A0: servers 'MPG1'
 * 0x4D504731 and 'MPG2' 0x4D504732) are of the class RPC of its anonymous
 * namespace (_GLOBAL_$N$.._.._pdimpeg.cxxIPKYuE), derived from PDIClientRpc
 * (T527): vtable 0x00345920, typeinfo 0x0025D2F8, 0xC0 bytes.  RPC() 0x0025CE00,
 * ~RPC() [0] +0x0C 0x0025CE48, [1] PDIClientRpc::onCallEnd; inline: F_0025D178,
 * feed 0x0025D1D8, open 0x0025D248, close 0x0025D2D8.  'MPG1': 1 open, 2 feed,
 * 4 close (blocking); 'MPG2': 3 (NOWAIT).  The class stays defined in
 * src/engine/pdi/T522_pdimpeg.cpp (see T519_pdicdvd.h). */

void pdimpeg_init(void);                                    /* 0x0025CE70 binds 'MPG1' and 'MPG2' */
/* Opens the stream of sectors sectors from lsn (hostName "" = the disc, else
 * a host: file).  mpegif passes mode 1 (preloading), 2 (playing with sound)
 * or 3 (playing without sound). */
void pdimpeg_open(int mode, int lsn, int sectors, const char *hostName); /* 0x0025CEB8 */
/* Reads the next size bytes of the stream into buf (blocking). */
void pdimpeg_feed(void *buf, int size);                     /* 0x0025CF50 */
/* 'MPG2' fno 3 {lo, hi} (NOWAIT): (? the presentation time window; the menu
 * movies pass (0, 0x7FFFFFFFFFFFFFFF) to open it). */
void F_0025CFC0(s64 lo, s64 hi);                            /* 0x0025CFC0 */
void pdimpeg_close(void);                                   /* 0x0025D028 */
/* T522_static_fini 0x0025D050 (.dtors) and T522_static_init 0x0025D0B8 (.ctors) are
 * egcs's own static destructor / initialiser of the array (no source
 * functions: the objects are defined in the .cpp file). */

#if defined(__cplusplus) && !defined(BOOT_OLD_GCC)  /* egcs generates it itself */
extern "C" void *tf_pdimpeg_RPC(void);  /* 0x0025D2F8 __tfQ234_GLOBAL_$N$.._.._pdimpeg.cxxIPKYuE3RPC */
#endif

#endif /* GAME_T522_PDIMPEG_H */
