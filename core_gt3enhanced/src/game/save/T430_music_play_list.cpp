/* T430 - the music play list (? name, u037 / u044) (0x0021AB08..0x0021AFA0).
 *
 * The songs are the tracks of BGM group 0x14 (bgm_get_tracks), identified
 * by the 64-bit code of their name (nameToCode).  A play list holds up to
 * 64 songs, each enabled or not, and a play order: entry i plays at
 * position entries[i].order.  next() walks the positions, skipping the
 * songs already played in this round (a disabled song counts as played); at
 * the end of the order a new one is made (straight, or shuffled with the
 * 16-bit seed, which is advanced each time).
 */
#include "game/save/game_status_internal.h"
#include "game/save/T430_music_play_list.h"
#include "game/sound/T464_bgm.h"                      /* bgm_get_tracks */
#include "game/sound/T466_bgm_ads.h"                  /* BgmTrackList, MusicInfo (= BgmTrack) */
#include "game/data/T481_car_data_base.h"            /* nameToCode */
#include "engine/pdistd/T531_random.h"                   /* Random::next01, random_int */

// FUNC 0x0021AB08 findMusicInfo
//   The song of BGM group 0x14 whose name has code *code, its number in
//   *index (unless NULL); the first song (index 0) if there is none.
//   Callers: gt2k_menu::PlayMusicBox::preview / setSong,
//   RaceBGMPS2::play.
MusicInfo *findMusicInfo(const u64 *code, u32 *index)
{
    BgmTrackList songs = bgm_get_tracks(MUSIC_BGM_GROUP);  /* sp+0x0; 0x0022B750 */
    for (u32 i = 0; i < songs.count; i++) {
        if (*code == nameToCode(songs.tracks[i].name)) {
            if (index != NULL)
                *index = i;
            return &songs.tracks[i];
        }
    }
    if (index != NULL)
        *index = 0;
    return songs.tracks;
}

// FUNC 0x0021ABB0 MusicPlayList::init
//   ? name.  A random seed (random_int); all 64 entries enabled, not
//   played, no code; then every song of BGM group 0x14 in table order, and
//   a shuffled order (setRandom(1)).  Not a constructor (it does not return
//   this; GameOptions() does not run it).  Callers: RaceBGMPS2::RaceBGMPS2
//   (both of its lists), GameOptions::setDefaults.
void MusicPlayList::init()
{
    /* One counter (s2) and one entry pointer for both loops: set twice, the
     * pointer has no known base for egcs's alias analysis, so the first
     * loop's stores keep the load of songs.count after it (with a pointer
     * per loop GCSE hoists the load above the first loop). */
    u32 i;
    MusicEntry *entry;
    BgmTrackList songs = bgm_get_tracks(MUSIC_BGM_GROUP);  /* sp+0x0; 0x0022B750 */
    seed = random_int();
    for (i = 0; i < MUSIC_ENTRIES; i++) {
        entry = &entries[i];
        entry->played = 0;
        entry->enabled = 1;
        entry->code = 0;
    }
    count = 0;
    for (i = 0; i < songs.count; i++) {
        entry = &entries[count];
        entry->code = nameToCode(songs.tracks[i].name);
        count++;
    }
    setRandom(1);
}

// FUNC 0x0021AC98 MusicPlayList::resetPlayed
//   A new round: an entry counts as played only when it is disabled;
//   allPlayed if every entry is.
void MusicPlayList::resetPlayed()
{
    s8 all = 1;
    for (u32 i = 0; i < count; i++) {
        MusicEntry *e = &entries[i];
        e->played = e->enabled == 0;
        if (e->played == 0)
            all = 0;
    }
    allPlayed = all;
}

// FUNC 0x0021ACE0 MusicPlayList::setRandom
//   Straight (0) or shuffled (1) play, and a new order.  Callers:
//   gt2k_menu::PlayList::save, init.
void MusicPlayList::setRandom(s8 r)
{
    random = r;
    reorder();
}

// FUNC 0x0021AD00 MusicPlayList::reorder
//   A new round and play order: straight or shuffled.
void MusicPlayList::reorder()
{
    if (random == 0)
        orderStraight();
    else
        orderShuffle();
}

// FUNC 0x0021AD38 MusicPlayList::orderStraight
//   A new round; entry i plays at position i.
void MusicPlayList::orderStraight()
{
    resetPlayed();
    u32 songs = count;
    position = 0;
    for (u32 i = 0; i < songs; i++)
        entries[i].order = i;
}

// FUNC 0x0021AD90 MusicPlayList::orderShuffle
//   A new round; positions 0, 1, ... go to random entries (a Random on the
//   stack, from the seed): a random entry, or when it already has a
//   position the next free one after it (wrapping; after count tries the
//   last one looked at is taken anyway).  The seed is then advanced:
//   (int)(next01() * 32768).  The (u32) conversion of the random float is
//   a call of fptoui (__fixunssfsi).
//   98.3%: tries and the i + 1 that GCSE hoists (retail $a2 / $a3) have
//   their registers swapped; everything else is exact.  global-alloc takes
//   the temporary first (priority 3*9/20 against tries' 3*11/34); declared
//   inside the if, tries wins $a2 but its `= 0' then misses the beqz delay
//   slot (97.7%).  Tried: tries / pick / n declaration orders and types,
//   the for / while / goto forms of both loops, the condition at the
//   bottom, i++ in the store; the reference compiler gives the same.
void MusicPlayList::orderShuffle()
{
    u32 i;                                      /* s1: one counter for both loops */
    resetPlayed();
    u32 songs = count;
    for (i = 0; i < songs; i++)
        entries[i].order = -1;
    Random rng;                                 /* sp+0x0 (egcs drops the dead 0x87654321 store) */
    rng.seed = seed;
    position = 0;
    for (i = 0; i < count; i++) {
        f32 r = rng.next01();
        u32 n = count;
        u32 pick = (u32)(r * (f32)n);           /* implicit: fptoui */
        u32 tries = 0;
        /* (entries + pick): retail adds the index to this (`addu v0, a1, s2'),
         * entries[pick] gives this + index */
        if (n != 0 && (entries + pick)->order >= 0) {
            /* a do-while (a for (;;) gets its head copied in front of it);
             * reorg steals the pick++ into the bgez's delay slot */
            do {
                pick++;
                tries++;
                if (!(pick < n))
                    pick = 0;
                if (!(tries < n))
                    break;
            } while ((entries + pick)->order >= 0);
        }
        (entries + pick)->order = i;
    }
    seed = (int)(rng.next01() * MUSIC_SEED_SCALE);
}

// FUNC 0x0021AEE8 MusicPlayList::next
//   The next song to play, NULL when every song is played (or disabled):
//   the entry at the current position, unless it was played in this round;
//   the position moves on either way, a new order is made at the end of
//   the order.  (Loops for ever when no entry has the position.)  Caller:
//   RaceBGMPS2::play.
MusicEntry *MusicPlayList::next()
{
    MusicEntry *found = NULL;
    u32 pos = position;
    for (;;) {
        if (allPlayed != 0)
            return NULL;
        /* count, not a local: GCSE keeps it in a reaching register, which
         * splits the loop's back edge (loop.c leaves this loop alone) */
        for (u32 i = 0; i < count; i++) {
            MusicEntry *entry = &entries[i];
            if ((s16)pos != entry->order)
                continue;
            pos++;
            if (entry->played == 0) {
                entry->played = 1;
                found = entry;
            }
            if (!(pos < count)) {
                reorder();
                pos = 0;
            }
            break;
        }
        if (found != NULL)
            break;
    }
    position = pos;
    return found;
}
