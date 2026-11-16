/* T529_mpegif.h - mpegif, the MPEG (PSS) movie streamer, and MpegifParam
 * (translation unit T529, unit u120).
 *
 * The feeder thread reads the movie from the IOP stream (pdimpeg, T522) or
 * from a preloaded copy and feeds it to the G002 MPEG player (u121) in 4 KB
 * chunks.  The globals below are shared with the player (u121: C), so they
 * have C linkage and the header is a C header too: the class is C++ only. */
#ifndef GAME_T529_MPEGIF_H
#define GAME_T529_MPEGIF_H

#include "game/game.h"

struct MpegPlayer;                              /* G002 (u121): mpeg_player_create's handle, 0x24 bytes */

/* The constructor's parameter block (MenuBgMovie, T253, passes {1, 4, 10, 11}). */
typedef struct MpegifParam {
    int prioHigh;               /* 0x0 -> mpegif.prioHigh */
    int prioRead;               /* 0x4 -> mpegif.prioRead */
    int prioFeed;               /* 0x8 -> mpegif.prioFeed */
    int prioDecode;             /* 0xC -> mpegif.prioDecode */
} MpegifParam;
EGCS_SIZE(MpegifParam, 0x10);

/* mpegif::control's commands.  The result is 0 when the command is taken,
 * 1 when it is not (the player is not running yet: retry), or the state. */
enum {
    MPEGIF_CMD_STOP_RUNNING = 0,    /* stop (no caller); 1 unless the player runs */
    MPEGIF_CMD_PLAY         = 1,    /* switch to file (mpegif::play, start) */
    MPEGIF_CMD_SWITCH       = 2,    /* the same; 1 when file is already playing (switchTo) */
    MPEGIF_CMD_STOP         = 3,    /* stop; 0 once the feeder thread has ended */
    MPEGIF_CMD_GET_STATE    = 4     /* returns mpegif.state */
};

/* mpegif.state (0x14C). */
enum {
    MPEGIF_FEEDING          = 0,    /* the feeder thread plays its first file */
    MPEGIF_SWITCHED         = 1,    /* it plays the file control() switched to */
    MPEGIF_SWITCH_REQUEST   = 2,    /* control(PLAY / SWITCH): feed() ends, the thread opens mpegif.name */
    MPEGIF_STOP_REQUEST     = 3,    /* control(STOP): feed() ends, then the thread */
    MPEGIF_ENDED            = 4     /* the feeder thread has ended (or not started) */
};

/* g_dec_state (below), written by the player's decoder thread (u121) and
 * mpegif; 0 after the mpegif constructor.  The same macros as mpeg_player_internal.h's. */
#define DEC_STOP_REQUEST    1   /* mpegif asks the decoder to stop */
#define DEC_STOPPED         2   /* the decoder saw the request after a picture */
#define DEC_ENDED           3   /* mpeg_video_decode_main has returned */
#define DEC_RUNNING         4   /* the first picture is decoded */

#ifdef __cplusplus
/* vtable 0x00345A68 (1 slot), typeinfo __tf6mpegif 0x0025F350.  vptr +0x150.
 * Size 0x154 (operator new in T253).  Only one can exist at a time (the
 * semaphores and flags are globals).
 * state (0x14C): MPEGIF_FEEDING .. MPEGIF_ENDED above. */
class mpegif {
public:
    mpegif(const MpegifParam *p, int width, int height);         /* 0x0025E548 */
    virtual ~mpegif();                                           /* [0] +0x0C 0x0025E698 */

    void  preload(const char *file, int size);                   /* 0x0025E710 */
    void  freePreload();                                         /* 0x0025E940 */
    /* cmd: MPEGIF_CMD_* above. */
    int   control(int cmd, const char *file);                    /* 0x0025E970 */
    void  startThread(const char *file, int withSound, int oneShot); /* 0x0025EAC0 */
    int   getSize(int *w, int *h);                               /* 0x0025EB58 */
    int   start(const char *file, int withSound, int oneShot, int cmd); /* 0x0025EBD0 */
    void *nextFrame();                                           /* 0x0025EC10 next decoded frame, NULL if none */
    void  releaseFrame();                                        /* 0x0025EC50 */
    void  stopFrames();                                          /* 0x0025EC78 */
    f32   bufferFill();                                          /* 0x0025EC88 */
    void  waitBuffered();                                        /* 0x0025ED30 */
    void  wakeIfBuffered();                                      /* 0x0025ED78 */
    static void threadEntry(void *arg);                          /* 0x0025EE18 */
    void  threadMain();                                          /* 0x0025EE38 */
    void  feed();                                                /* 0x0025EF00 plays one file */
    int   isPlaying();                                           /* 0x0025F1F8 */
    int   isBusy();                                              /* 0x0025F220 */
    int   stop();                                                /* 0x0025F260 */
    int   switchTo(const char *file, int withSound, int oneShot); /* 0x0025F280 */
    int   play(const char *file, int withSound, int oneShot);    /* 0x0025F2A8 */
    void  rewindPreload();                                       /* 0x0025F340 */

    char  name[0x40];                       /* 0x000 file being played */
    char  hostName[0x40];                   /* 0x040 its host: file ("" on the disc) */
    u32   size;                             /* 0x080 its size in bytes */
    int   lsn;                              /* 0x084 its first sector */
    int   width;                            /* 0x088 */
    int   height;                           /* 0x08C */
    char  preloadName[0x40];                /* 0x090 file of the preloaded copy */
    char  preloadHost[0x40];                /* 0x0D0 its host: file */
    u8   *preloadBuf;                       /* 0x110 the preloaded copy, NULL if none */
    u8   *preloadPos;                       /* 0x114 read position in it */
    int   usePreload;                       /* 0x118 1: play the preloaded copy */
    int   withSound;                        /* 0x11C the movie has sound: BGM volume 1.0, pdimpeg mode 2 (0: mode 3) */
    int   oneShot;                          /* 0x120 1: play once (opening, endings); 0: loop, accept switches (menus) */
    int   tid;                              /* 0x124 feeder thread, -1 none */
    void *stack;                            /* 0x128 feeder stack: 0x70001700 (scratchpad), 0xC00 bytes */
    MpegPlayer *dec;                        /* 0x12C */
    int   prioDecode;                       /* 0x130 decoder thread priority */
    int   prioFeed;                         /* 0x134 feeder thread priority */
    int   prioRead;                         /* 0x138 feeder priority while reading */
    int   prioHigh;                         /* 0x13C caller priority in control(), and between files */
    int   sleeper;                          /* 0x140 thread in waitBuffered(), -1 none */
    int   sema;                             /* 0x144 mutex guarding sleeper */
    int   unk_148;                          /* 0x148 (only cleared) */
    int   state;                            /* 0x14C */
    /*    vptr                                 0x150 */
};
EGCS_SIZE(mpegif, 0x154);
#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

/* .sdata, created / reset by the mpegif constructor. */
extern int g_mpeg_start_sema;       /* 0x00353110 gp-0x48E0 semaphore (count 0) the player thread signals once it runs */
extern int g_frameq_filled_once;    /* 0x00353114 gp-0x48DC 1: the frame queue has been full once (frames available) */
extern int g_mpeg_wake_thread;      /* 0x00353118 gp-0x48D8 feeder thread asleep on a full buffer, woken by the player; -1 none */
extern int g_dec_state;             /* 0x00353140 gp-0x48B0 1 stop request, 2 stopped, 3 ended, 4 running */
extern int g_dec_state_sema;        /* 0x00353144 gp-0x48AC mutex (SemaParam 0x00345A20) guarding g_dec_state */

#if defined(__cplusplus) && !defined(BOOT_OLD_GCC)  /* egcs generates them itself */
void *__tf6mpegif(void);            /* 0x0025F350 */
extern u32 __ti6mpegif[2];          /* 0x003534E8 gp-0x4508 (__user_type_info) */
#endif

#ifdef __cplusplus
}
#endif

#endif /* GAME_T529_MPEGIF_H */
