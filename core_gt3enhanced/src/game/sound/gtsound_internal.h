/* gtsound_internal.h - Gran Turismo 3 A-Spec US (SCUS-97102) main executable, unit u104,
 * 0x0022AA78..0x0022DA98: translation units T466, T467, T463, T464, T465,
 * T468, T469, T470, T471, T473.
 *
 *   T466  the streamed BGM (ADS): bgm/ads.inf (17 groups of tracks with
 *         their beat markers), plays bgm/[i_]<name> through the T521 stream.
 *   T467  relocation of the track lists of ads.inf / music.inf.
 *   T463  the sequenced music: music/music.inf (named groups), the music
 *         bank (music/music.ins) and sequences (music/music.seq), the music
 *         driver thread woken by a VBLANK_E handler.
 *   T464  the BGM front end: group ids 0..0x11 are sequenced music, 0x12..0x22
 *         ADS streams; master volume, play / play-if-changed / stop under a
 *         semaphore.
 *   T465  the clock: ticked by the music thread every frame; three sceCdCLOCK
 *         dates read with interrupts off.
 *   T468  GTSOUNDINSTRUMENT (a sound bank: its header in EE RAM, its samples
 *         in SPU RAM), the SPU / header allocators, GtSoundHandle (one program
 *         on one voice) and GtSoundVoice.
 *   T469  the sound effects: three .ins banks (gt3sys, gt3se01, gt3se02), the
 *         road noise bank (rn_01.es), sound/se.inf (name -> bank / programs).
 *   T470  sound init / term.
 *   T471  _UnitArenaBase: a pool of fixed-size units with a free list.
 *   T473  the i.LINK (IEEE 1394) session of the link battle: the pdi1394
 *         driver object, a thread exchanging one 0x7C-byte frame per node per
 *         frame, end / pause commands, the exchange of the race selections
 *         and of the game options.
 *
 * The shared classes, functions and globals come from the canonical headers
 * (include/game/).  This header keeps what only u104 uses: the .inf file
 * layouts, the link frames, the unit-internal functions and the unit's
 * globals (descriptive names; each with its address).
 *
 * The headers below make a file see no class with only inline virtual
 * functions (no type name in its .rodata): retail's T463..T471 have only
 * their own (T468 GTSOUNDINSTRUMENT, T471 _UnitArenaBase).  T473, whose
 * original saw the garage lists and pdi1394, includes T429_game_status.h
 * before this header and T541_pdi1394.h after it.  The data
 * each file defines (ref/data/u104/notes.md): the .sdata words in address
 * order, the .data tables (T463, T469), the .bss / .sbss statics in the
 * files themselves (no extern here).  T472 (analog stick curve, data only)
 * does not include this header.
 *
 * Offsets in comments are the ORIGINAL ones (egcs layout).
 */
#ifndef U104_H
#define U104_H

#include "types.h"
#include "libc.h"
#include "cxxrt.h"                              /* __rtti_user */
#include "eekernel.h"                           /* semaphores, INTC handlers, threads */

/* outside the unit */
#include "engine/media/G002_mpeg_frameq.h"              /* snd_* voices, seq_* sequencer, intr_suspend,
                                                   semaphore and worker-thread helpers */
#include "engine/pdi/G003_pdi1394_rpc.h"              /* pdi1394_exchange / _get_status / ... */
#include "runtime/newlib/G007_stdio.h"                    /* rand, EnableIntc / DisableIntc */
#include "game/app/T000_main.h"                     /* LinkPlayerInfo */
#include "game/save/T428_game_options.h"             /* GameOptions (getSize / load / save) */
/* (T429_game_status.h: T473 only, before this header: the garage lists'
 * type names come before pdi1394's there) */
#include "engine/input/T474_control.h"                  /* Control (the link's) */
#include "engine/input/T475_pad_manager.h"              /* PadManager g_002EA7F8 */
#include "engine/io/T502_file_resolve.h"             /* file_locate */
#include "engine/io/T503_vol_thread.h"               /* file_read_blocking, file_read_sound_bank, vol_device_object */
#include "engine/pdi/T519_pdicdvd.h"                  /* pdicdvd_get_time, pdicdvd_time_to_clock*, CdClock */
#include "engine/pdi/T520_pdispu2.h"                  /* F_0025C280 */
#include "engine/pdi/T521_pdibgm.h"                   /* pdibgm_start / stop / set_volume / update */
#include "engine/pdistd/T531_random.h"                   /* Random */
/* (T541_pdi1394.h: T473 only, after this header: pdi1394's destructor is
 * inline, so every file that sees the class gets its type name, and
 * retail's T463..T471 have none; g_link below needs only
 * T473_link_session.h's `class pdi1394;') */

/* own translation units */
#include "game/sound/T463_music_seq.h"
#include "game/sound/T464_bgm.h"
#include "game/sound/T465_clock.h"
#include "game/sound/T466_bgm_ads.h"                  /* BgmMarkers, BgmTrack, BgmTrackList */
#include "game/sound/T468_gtsound.h"                  /* SoundProgram, SoundBankHeader, GtSoundHandle,
                                                   GtSoundVoice, GTSOUNDINSTRUMENT */
#include "game/sound/T469_sound_effects.h"
#include "game/sound/T470_sound_init.h"
#include "engine/pdistd/T471_unit_arena.h"
#include "engine/system/T473_link_session.h"             /* LinkStatus */

/* Adds a byte offset to a pointer (the relocation of loaded .inf files). */
template <class T> static inline void u104_reloc(T *&p, int delta)
{
    p = (T *)((u8 *)p + delta);
}

/* ======================================================================== */
/* T466 / T467 / T463 / T464: the BGM                                       */
/* ======================================================================== */

/* Group ids of the BGM front end (T464). */
enum {
    MUSIC_GROUP_COUNT = 0x12,                   /* 0..0x11: the sequenced music groups (T463) */
    BGM_GROUP_ADS     = 0x12,                   /* 0x12..0x22: ADS groups 0..0x10 (T466) */
    BGM_GROUP_LAST    = 0x22,
    ADS_GROUP_COUNT   = 17
};
/* bgm_kind */
enum {
    BGM_KIND_SEQ  = 0,                          /* sequenced music */
    BGM_KIND_ADS  = 1,                          /* ADS stream */
    BGM_KIND_NONE = 2
};
/* What plays (g_bgmPlaying): not the same numbers as bgm_kind. */
enum {
    BGM_PLAYING_NONE = 0,
    BGM_PLAYING_ADS  = 1,
    BGM_PLAYING_SEQ  = 2
};
/* Track numbers (ads_play, music_play, bgm_play*). */
#define BGM_TRACK_RANDOM  (-1)                  /* a random track of the group (int: li -1) */
#define BGM_TRACK_INTRO   0x1000                /* ADS: the intro file i_<name> first */

/* bgm/ads.inf: a header, then the 17 groups (offsets relocated in place).
 * `base' of ads.inf / music.inf is an address kept as an integer: retail
 * loads it with lwu (zero-extended) for the 64-bit (long) delta. */
struct AdsInf {
    u8           unk_00[8];                     /* 0x00 */
    u32          base;                          /* 0x08 the address it was built for */
    u32          total;                         /* 0x0C tracks of all groups */
    BgmTrackList group[ADS_GROUP_COUNT];        /* 0x10 */
};

/* music/music.inf: named groups of tracks (BgmTrack.seq = the sequence). */
struct MusicInf {
    u8           unk_00[8];                     /* 0x00 */
    u32          base;                          /* 0x08 the address it was built for */
    u32          ngroups;                       /* 0x0C */
    u32          total;                         /* 0x10 tracks of all groups */
    BgmTrackList group[1];                      /* 0x14 [ngroups] */
};

/* The music group table (.data of T463): the name of music group i (as
 * BgmTrack.group of its tracks) -> its group of music.inf. */
struct MusicGroupName {
    const char   *name;                         /* 0x0 */
    BgmTrackList *group;                        /* 0x4 set by music_init, NULL: not in music.inf */
};

/* libsd effect mode (ps2sdk libsd-common.h SD_EFFECT_MODE_STUDIO_3). */
#define SD_EFFECT_MODE_STUDIO_3  4
/* The sound / sequencer frame rate music_init sets (60 Hz). */
#define U104_SOUND_RATE          0x3C

/* T466 .sdata */
extern AdsInf *g_adsInf;                        /* 0x00352B00 gp-0x4EF0 bgm/ads.inf */
/* T463 (.sbss / .bss statics there: g_musicSeqBank 0x00353260, g_musicThread
 * 0x00353268, g_musicSequence 0x0049D1B0, g_musicThreadStack 0x0049D460) */
extern MusicGroupName g_musicGroups[MUSIC_GROUP_COUNT]; /* 0x002E8A88 (.data) */
extern int        g_musicSema;                  /* 0x00352B18 gp-0x4ED8 the music semaphore (-1) */
extern MusicInf  *g_musicInf;                   /* 0x00352B1C gp-0x4ED4 music/music.inf */
/* The 0x4000-byte header buffer of the music bank: music.ins's header (an
 * INST bank) is copied to its start. */
extern InstBank  *g_musicBank;                  /* 0x00352B20 gp-0x4ED0 */
extern int        g_musicSeqPlaying;            /* 0x00352B24 gp-0x4ECC a sequence plays */
extern int        g_musicThreadQuit;            /* 0x00352B58 gp-0x4E98 the music thread ends when set */
extern int        g_musicVblankHandler;         /* 0x00352B5C gp-0x4E94 the VBLANK_E handler id (-1) */
extern int        g_musicVblankSema;            /* 0x00352B60 gp-0x4E90 signalled every VBLANK_E (-1) */
/* T464 */
extern int g_bgmPlaying;                        /* 0x00352B64 gp-0x4E8C BGM_PLAYING_* */
extern int g_bgmSema;                           /* 0x00352B68 gp-0x4E88 the BGM semaphore (-1) */
extern f32 g_bgmMasterVolume;                   /* 0x00352B6C gp-0x4E84 (1.0) */
extern u32 g_bgmGroup;                          /* 0x00352B70 gp-0x4E80 the group playing (-1) */
extern int g_bgmTrack;                          /* 0x00352B74 gp-0x4E7C the track number asked for (-1) */
extern f32 g_bgmVolume;                         /* 0x00352B78 gp-0x4E78 its volume (1.0) */

/* T467 */
/* Words 0x0..0xC of a track; 0x10 (the ADS markers) too when `markers'. */
void bgm_track_relocate(BgmTrack *t, int delta, int markers);       /* 0x0022AE38 */
/* The tracks pointer and every track. */
void bgm_tracklist_relocate(BgmTrackList *l, int delta, int markers);   /* 0x0022AE88 */

/* ======================================================================== */
/* T465: the clock                                                          */
/* ======================================================================== */

/* The three dates of a frame: as read, GMT, local. */
struct CdClock3 {
    CdClock clock[3];                           /* 0x00 */
};

extern u64      g_clockTime;                    /* 0x00352B80 gp-0x4E70 this frame's pdicdvd_get_time */
/* (.bss statics of T465: g_clockDatesNew 0x0049E460, g_clockDates 0x0049E478) */

/* ======================================================================== */
/* T468: sound banks and voices                                             */
/* ======================================================================== */

/* SPU volumes are s16, -0x4000..0x3FFF; 0x4000 (1 << 14) = unity. */
#define SPU_VOLUME_UNITY  0x4000
extern f32  g_seVolume;                         /* 0x00352B88 gp-0x4E68 the effect volume (1.0) */
/* The header allocation pointer: typed as the headers it hands out (load's
 * schedule shows its store conflicting with the loads of
 * GTSOUNDINSTRUMENT::header, the same alias set). */
extern SoundBankHeader *g_soundHeaderNext;      /* 0x00352B90 gp-0x4E60 */
extern int  g_soundHeaderLeft;                  /* 0x00352B94 gp-0x4E5C header bytes left */

/* ======================================================================== */
/* T469: sound effects                                                      */
/* ======================================================================== */

/* se.inf code: bank << 24 | program A << 12 | program B. */
#define SE_CODE_BANK(c)   ((c) >> 24)
#define SE_CODE_A(c)      (((c) >> 12) & 0xFFF)
#define SE_CODE_B(c)      ((c) & 0xFFF)
/* SE_PAN_*, SE_TABLE_COUNT, SE_BANK_*: T469.  The enhanced engine: 4 banks,
 * the Pod's bank 3 (Lupo 2's static init L2 0x0023BAF0 builds 4). */
#define SE_BANK_COUNT     4
/* The enhanced engine: an effect se.inf does not name (g_seNames index; GT3's
 * 0x28 effects keep 0, as retail). */
#define SE_INDEX_NONE     (-1)

/* sound/se.inf */
struct SeInfEntry {
    const char *name;                           /* 0x0 */
    u32         code;                           /* 0x4 */
};
struct SeInf {
    u8         unk_00[8];                       /* 0x00 */
    SeInf     *base;                            /* 0x08 the address it was built for */
    u32        count;                           /* 0x0C */
    SeInfEntry entry[1];                        /* 0x10 [count] */
};
/* The sound effect table (.data of T469): effect id -> entry of se.inf. */
struct SeName {
    const char *name;                           /* 0x0 */
    int         index;                          /* 0x4 set by se_init */
};
extern SeName             g_seNames[SE_TABLE_COUNT];    /* 0x002E9380 (.data) */
/* g_seBanks (0x0049ECC0, gt3sys, gt3se01, gt3se02; the enhanced engine: and
 * gt3pod): static in T469; their headers' buffer (0x0049E4C0, and rn_01's):
 * se_init's static (the Pod bank has one of its own). */
extern SeInf             *g_seInf;                      /* 0x00352B98 gp-0x4E58 sound/se.inf */

/* ======================================================================== */
/* T473: the i.LINK session                                                 */
/* ======================================================================== */

/* The command byte of a frame (LinkFrame.cmd).  u047's RaceLinkBattle
 * sends END when its race ends and from the pause menu's quit, PAUSE for
 * the pause button; an END received ends the race there. */
enum {
    LINK_CMD_NONE  = 0,
    LINK_CMD_END   = 0x80,
    LINK_CMD_PAUSE = 0x81
};
/* pdi1394_exchange modes. */
enum {
    LINK_XCHG_FIRST   = 0,                      /* the session's first exchange */
    LINK_XCHG_NORMAL  = 1,
    LINK_XCHG_COMMAND = 3                       /* after a command (or result 3) */
};
/* g_linkState (F_0022CD28): T473's LINK_STATE_*. */
#define LINK_NODES       8
#define LINK_FRAME_SIZE  0x7C

/* One node's frame of an exchange, 0x7C bytes. */
struct LinkFrame {
    u8  unk00[4];                               /* 0x00 (bytes only: retail copies a frame
                                                   with lwl / lwr, alignment 1) */
    u8  data[0x77];                             /* 0x04 the game's data (F_0022CC58 / F_0022CC78) */
    u8  cmd;                                    /* 0x7B LINK_CMD_* */
};
static_assert(sizeof(LinkFrame) == LINK_FRAME_SIZE, "LinkFrame");
/* The frames of all nodes (by IOP node id), 0x3E0 bytes. */
struct LinkFrameSet {
    LinkFrame node[LINK_NODES];                 /* 0x000 */
};
/* What pdi1394_exchange receives into, 0x400 bytes. */
struct LinkRecvBuffer {
    LinkFrameSet frames;                        /* 0x000 */
    u8           unk_3E0[0x20];                 /* 0x3E0 */
};

extern Control        g_linkControl;            /* 0x002E9820 the link's Control (pad slot 4) */
extern LinkPlayerInfo g_linkPlayers[LINK_NODES];/* 0x002E98E8 what each slot sent (F_0022D3C0) */
extern LinkFrameSet   g_linkFramesIn;           /* 0x002E9910 the frames received (the game's copy) */
extern LinkFrame      g_linkFrameOut;           /* 0x002E9CF0 the frame to send (the game's copy) */
/* (.bss / .sbss statics of T473: g_linkRecv 0x0049ED40, g_linkSend 0x0049F140,
 * g_linkLastCmd 0x00353270) */
extern const SemaParam g_linkSemaParam;         /* 0x003417F8 (.rodata) both semaphores */

extern int      g_linkGoSema;                   /* 0x00352C98 gp-0x4D58 the thread may run an exchange (-1) */
extern int      g_linkDoneSema;                 /* 0x00352C9C gp-0x4D54 an exchange is done (-1) */
extern Random   g_linkRandom;                   /* 0x00352CA0 gp-0x4D50 (seed 0x87654321) */
/* volatile: F_0022D9A8's store stays out of link_disable's delay slot */
extern volatile int g_linkThreadQuit;           /* 0x00352CA4 gp-0x4D4C the thread ends when set */
extern int      g_linkRandomSum;                /* 0x00352CAC gp-0x4D44 the sum of the nodes' random bytes */
extern int      g_linkState;                    /* 0x00352CB4 gp-0x4D3C LINK_STATE_* */
extern pdi1394 *g_link;                         /* 0x00352CB8 gp-0x4D38 the driver */
extern int      g_linkResetNodes;               /* 0x00352CBC gp-0x4D34 LinkStatus.nodeCount at the last bus reset */
extern int      g_linkSeq;                      /* 0x00352CC0 gp-0x4D30 the exchange sequence number */
extern int      g_linkCmdIn;                    /* 0x00352CC4 gp-0x4D2C the command received */
extern int      g_linkCmdOut;                   /* 0x00352CC8 gp-0x4D28 the command to send */
extern int      g_linkFramesTaken;              /* 0x00352CCC gp-0x4D24 the game took the frames */
extern int      g_linkCmdXchgDue;               /* 0x00352CD0 gp-0x4D20 a command exchange is due */
extern int      g_linkFirstXchgDue;             /* 0x00352CD4 gp-0x4D1C the first exchange is due */
extern int      g_linkEnabled;                  /* 0x00352CD8 gp-0x4D18 the session runs */

#endif /* U104_H */
