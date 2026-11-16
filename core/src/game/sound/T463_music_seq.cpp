/* T463 - the sequenced music: music/music.inf, the music bank and the
 * sequences, the music driver thread.  0x0022AF18..0x0022B588. */
#include "game/sound/gtsound_internal.h"
#include "game/sound/T463_music_seq.h"

/* The data, in retail's order: initialised words are written where they are
 * defined.  The group table's names are written just before it, each in the
 * table's order: the long ones open T463's .rodata (0x003414B0, before the
 * functions' literals; only the table points at them), those of 8 bytes or
 * less go to .sdata (-G8, 0x00352B28..0x00352B58), between the words defined
 * before and after the table.  The statics follow at the end (.sbss, .bss). */
// DATA_START 0x003414B0
int        g_musicSema = -1;                    /* 0x00352B18 */
MusicInf  *g_musicInf = NULL;                   /* 0x00352B1C */
InstBank  *g_musicBank = NULL;                  /* 0x00352B20 */
int        g_musicSeqPlaying = 0;               /* 0x00352B24 */
/* The names of the music groups (BgmTrack.group of music.inf's tracks);
 * music_init links each to its group of music.inf. */
MusicGroupName g_musicGroups[MUSIC_GROUP_COUNT] = {   /* 0x002E8A88 */
    { "arcade01",    NULL },
    { "battle2p01",  NULL },
    { "dealer01",    NULL },
    { "free01",      NULL },
    { "gorace01",    NULL },
    { "gtmenu01",    NULL },
    { "home01",      NULL },
    { "license01",   NULL },
    { "license02",   NULL },
    { "machtest01",  NULL },
    { "main01",      NULL },
    { "mini01",      NULL },
    { "option01",    NULL },
    { "theater01",   NULL },
    { "timetrial01", NULL },
    { "tune01",      NULL },
    { "wash01",      NULL },
    { "wheelshop01", NULL },
};
int        g_musicThreadQuit = 0;               /* 0x00352B58 */
int        g_musicVblankHandler = -1;           /* 0x00352B5C */
int        g_musicVblankSema = -1;              /* 0x00352B60 */
static SeqgBank  *g_musicSeqBank;               /* 0x00353260 music/music.seq */
static ThreadCtx  g_musicThread;                /* 0x00353268 the music thread */
static Sequence   g_musicSequence;              /* 0x0049D1B0 the sequence player (0x2A8 bytes) */
/* (16-aligned: retail's 0x0049D460 follows the sequence's end 0x0049D458) */
static u8         g_musicThreadStack[0x1000] __attribute__((aligned(16)));  /* 0x0049D460 */

// FUNC 0x0022AF18 music_inf_relocate
void music_inf_relocate(MusicInf *inf)
{
    u32 group;
    long delta = (long)inf - inf->base;     /* 64-bit: lwu, dsubu, truncated once */

    inf->base = (u32)inf;
    for (group = 0; group < inf->ngroups; group++)
        bgm_tracklist_relocate(&inf->group[group], delta, 0);
}

/* Stops the sequence player if a sequence plays (no lock). */
// FUNC 0x0022AF98 music_seq_stop
void music_seq_stop(void)
{
    if (g_musicSeqPlaying != 0) {
        seq_stop(&g_musicSequence);         /* 0x002694E8 */
        g_musicSeqPlaying = 0;
    }
}

/* INTC_VBLANK_E handler: wakes the music thread. */
// FUNC 0x0022AFC8 music_vblank_handler
int music_vblank_handler(int cause)
{
    signal_sema_helper(g_musicVblankSema);
    ExitHandler();                          /* sync.l; ei */
    return 1;
}

/* The music driver thread: one tick per VBLANK_E until g_musicThreadQuit is
 * set: the sequencer, the voices, the ADS stream status, the clock. */
// FUNC 0x0022AFF8 music_thread
void music_thread(void *arg)
{
    while (g_musicThreadQuit == 0) {
        WaitSema(g_musicVblankSema);
        seq_update();                       /* 0x002691F0 */
        snd_flush();                        /* 0x00269530 */
        pdibgm_update();                    /* 0x0025CA78 */
        clock_update();
    }
    worker_thread_exit(&g_musicThread);     /* 0x00269D48 */
}

/* Loads music/music.inf, links the group table to its groups, sets the
 * sound system and the sequencer up and starts the music thread with its
 * VBLANK_E handler. */
// FUNC 0x0022B058 music_init
void music_init(void)
{
    MusicInf *inf;
    u32 group;
    int i;

    if (g_musicInf != NULL)
        return;

    g_musicSema = create_mutex_sema(1);
    g_musicSeqPlaying = 0;
    inf = (MusicInf *)file_read_blocking("music/music.inf", NULL, 0, 1, &vol_device_object);
    music_inf_relocate(inf);
    g_musicInf = inf;
    for (group = 0; group < inf->ngroups; group++) {
        const char *name = inf->group[group].tracks->group;
        for (i = 0; i < MUSIC_GROUP_COUNT; i++) {
            if (strcmp(name, g_musicGroups[i].name) == 0) {
                g_musicGroups[i].group = &inf->group[group];
                break;
            }
        }
    }
    snd_init();                             /* 0x00268470 ($a0 is a leftover) */
    snd_set_rate(U104_SOUND_RATE);          /* 0x002684D8 */
    seq_init();                             /* 0x00268508 */
    seq_set_frame_rate(U104_SOUND_RATE);    /* 0x00268598 */
    snd_set_reverb(SD_EFFECT_MODE_STUDIO_3, 0xFFF, 0xFFF);  /* 0x00268990 depth L / R */
    snd_set_effect_volume(0xFFF, 0xFFF);
    F_00268978(1);
    g_musicThreadQuit = 0;
    g_musicVblankSema = create_mutex_sema(0);
    worker_thread_start(&g_musicThread, music_thread, NULL, 6,
                        g_musicThreadStack, sizeof(g_musicThreadStack));   /* 0x00269C90 */
    /* AddIntcHandler (0x00285940, the 3-parameter stub of syscall 0x10) */
    g_musicVblankHandler = AddIntcHandler(INTC_VBLANK_E, music_vblank_handler, -1);
    EnableIntc(INTC_VBLANK_E);
}

// FUNC 0x0022B220 music_term
void music_term(void)
{
    int sema;

    if (g_musicInf == NULL)
        return;

    snd_shutdown();                         /* 0x00268430 */
    music_unload();
    free(g_musicInf);
    DisableIntc(INTC_VBLANK_E);
    RemoveIntcHandler(INTC_VBLANK_E, g_musicVblankHandler);
    sema = g_musicVblankSema;
    g_musicVblankSema = -1;
    DeleteSema(sema);
    worker_thread_join(&g_musicThread);     /* 0x00269D18 */
    sema = g_musicSema;
    g_musicSema = -1;
    DeleteSema(sema);
    g_musicInf = NULL;
}

/* The tracks of music group `group' (-1: all; a group music.inf lacks:
 * none). */
// FUNC 0x0022B298 music_get_tracks
BgmTrackList music_get_tracks(int group)
{
    BgmTrackList *g;

    if (group == -1)
        return BgmTrackList(g_musicInf->group[0].tracks, g_musicInf->total);
    g = g_musicGroups[group].group;
    if (g == NULL)
        return BgmTrackList(NULL, 0);
    return *g;
}

/* 1.0 = 0x4000, clamped to 0x3FFF (no lower bound). */
// FUNC 0x0022B2F8 music_set_volume
void music_set_volume(f32 volume)
{
    int vol = (int)(volume * 16384.0f);

    if (!(vol < SPU_VOLUME_UNITY))
        vol = SPU_VOLUME_UNITY - 1;
    seq_set_master_volume(vol);             /* 0x00268BF8 (u16: andi 0xFFFF) */
}

/* Loads the music bank (music/music.ins at SPU 0xAC000, its header into a
 * new 0x4000-byte buffer) and the sequences (music/music.seq). */
// FUNC 0x0022B338 music_load
void music_load(void)
{
    WaitSema(g_musicSema);
    if (g_musicBank == NULL) {
        g_musicBank = (InstBank *)malloc(0x4000);
        sound_set_header_buffer(g_musicBank, 0x4000);
        {
            GTSOUNDINSTRUMENT bank;
            sound_set_spu_addr(0xAC000);
            bank.loadFile("music/music.ins", 0);
            g_musicSeqBank = (SeqgBank *)file_read_blocking("music/music.seq", NULL, 0, 1,
                                                            &vol_device_object);
            seqg_relocate(g_musicSeqBank);  /* 0x00268718 */
        }
    }
    signal_sema_helper(g_musicSema);
}

// FUNC 0x0022B410 music_unload
void music_unload(void)
{
    WaitSema(g_musicSema);
    music_seq_stop();
    if (g_musicBank != NULL) {
        free(g_musicSeqBank);
        free(g_musicBank);
        g_musicBank = NULL;
    }
    signal_sema_helper(g_musicSema);
}

/* Plays track `track' (BGM_TRACK_RANDOM: a random one) of music group
 * `group' (`arg' is not used); nothing until music_load. */
// FUNC 0x0022B460 music_play
void music_play(int group, int track, f32 volume, int arg)
{
    WaitSema(g_musicSema);
    music_seq_stop();
    if (g_musicBank != NULL) {
        BgmTrackList list = music_get_tracks(group);

        if (track == BGM_TRACK_RANDOM)
            track = (u32)rand() % list.count;
        track = list.tracks[track % list.count].seq;    /* now the sequence (retail: one register) */
        music_set_volume(volume);
        seq_start(&g_musicSequence, &g_musicSeqBank->entries[track], g_musicBank);  /* 0x00268AB0 */
        seq_restart(&g_musicSequence);      /* 0x002688A8 */
        g_musicSeqPlaying = 1;
    }
    signal_sema_helper(g_musicSema);
}

// FUNC 0x0022B558 music_stop
void music_stop(void)
{
    WaitSema(g_musicSema);
    music_seq_stop();
    signal_sema_helper(g_musicSema);
}
