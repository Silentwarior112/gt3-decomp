/* T386_camera_bgm_markers.cpp - the camera director's BGM clock
 * (0x0020B1D8..0x0020B788).
 *
 * The race music has marker channels, read through the race's RaceBGMBase
 * (RaceOrganization::getBGM): getMarkerTime(ch) is the time to channel ch's
 * next marker (FLT_MAX: none), getMarkerHit(ch) whether one passed this frame.  Both
 * methods run once a frame from CameraManager::update (0x00208BE8):
 * updateMarkerTime before the views, updateMarkers after them; they set the
 * cut request (changeRequest) and the time to the next cut the mode handlers
 * act on.  Only view 0's kind is looked at.
 *
 * Channels: 1 / 2 cut when armed (marker1Armed / marker2Armed), 3 cuts, 4
 * cuts in kind 1's other style; 4..7 set or clear the marker1Enabled / marker2Random /
 * unk_962B flags that arm 1 and 2; 8 / 9 make mode 1 set the second sound
 * volume factor g_003521E4 (T176) to 1.0 / to soundLevel, the replay's 0..1
 * setting (soundLevelFull, soundLevelApply).  The methods are declared in
 * T383_camera_manager.h (T386 has no header of its own).
 */
/* Retail T386's .rodata (0x0033F168): <new>'s "bad_alloc", the type names
 * "14RaceCameraBase", "t9UnitArena1Ui64" (nothing references them:
 * DATA_START). */
#include <new>                              /* (unused: "bad_alloc") */
#define GAME_T153_EMITS_TYPEINFO            /* UnitArena<64>'s type name */
#include "game/race/core/T153_race_course.h"          /* (seen: UnitArena<64>) */
#include "game/camera/camera_modes_internal.h"
#include "game/camera/T383_camera_manager.h"       /* CameraManager (T386's methods) */
// DATA_START 0x0033F168

// FUNC 0x0020B1D8 camera_kind1_alt_style
//   View kind 1 in its other style (mode1Style >= 0.5): the cuts follow
//   marker channel 4.
int camera_kind1_alt_style(int kind, f32 style)
{
    int other = kind == 1;

    if (style < 0.5f)
        other = 0;
    return other;
}

// FUNC 0x0020B200 CameraManager::updateMarkerTime
//   nextMarkerTime = the time to the next marker that can cut: in kind 1
//   channel 4 (other style) or 3, else channel 3, or 2 / 1 when they are
//   armed and nearer.  Over 80 s (no marker) it is markerTimeout / 60.
void CameraManager::updateMarkerTime(RaceOrganization *org)
{
    RaceBGMBase *bgm = org->getBGM();
    int kind = kinds[0].getKind();
    int kind1 = kind == 1;
    int otherStyle = camera_kind1_alt_style(kind, mode1Style);
    f32 time[5];
    f32 next;

    time[1] = bgm->getMarkerTime(1);
    time[2] = bgm->getMarkerTime(2);
    time[3] = bgm->getMarkerTime(3);
    time[4] = bgm->getMarkerTime(4);
    if (kind1) {
        next = time[4];
        if (!otherStyle)
            next = time[3];
    } else {
        next = time[3];
        if (marker2Armed != 0 && time[2] < next)
            next = time[2];
        if (marker1Armed != 0 && time[1] < next)
            next = time[1];
    }
    nextMarkerTime = next;
    if (80.0f < next)
        nextMarkerTime = (f32)markerTimeout / 60.0f;
}

// FUNC 0x0020B380 CameraManager::updateMarkers
//   Reads the markers passed this frame into the cut request changeRequest
//   (1..4) and the marker flags.  Without a marker within 80 s on the watched
//   channels (2 and 3; 4 in kind 1's other style) a cut (request 1) is made
//   every 120 frames.
//   Quirk: the two "< 10.5" tests compare a random number in [0, 1): always
//   true (the generator is still stepped).
void CameraManager::updateMarkers(RaceOrganization *org)
{
    RaceBGMBase *bgm = org->getBGM();
    f32 chance = 10.5f;                     /* $f21 for the whole function: a variable (cprop cannot put a
                                               constant into c.lt.s) */
    int kind = kinds[0].getKind();
    Random *random = &rng;
    int kind1 = kind == 1;
    int otherStyle = camera_kind1_alt_style(kind, mode1Style);
    int markerNear;
    int ch;

    random->next();
    changeRequest = 0;
    if (org->countDown > 0)
        markerTimeout = 120;

    markerNear = 0;
    for (ch = 1; ch < 7; ch++) {
        int watched = 0;
        if (otherStyle) {
            if (ch == 4)
                watched = 1;
        } else if (ch == 2 || ch == 3)
            watched = 1;
        if (watched) {
            if (bgm->getMarkerTime(ch) < 80.0f)
                markerNear = 1;
        }
    }
    if (!markerNear) {
        if (--markerTimeout < 0) {
            markerTimeout = 120;
            changeRequest = 1;
        }
    }

    if (bgm->getMarkerHit(4)) {
        marker2Random = 0;
        marker1Enabled = 0;
        unk_962B = 0;
    }
    if (bgm->getMarkerHit(5)) {
        marker2Random = 1;
        marker1Enabled = 0;
        unk_962B = 0;
    }
    if (bgm->getMarkerHit(6)) {
        marker2Random = 0;
        random->next();
        if (random->next01() < chance) {
            unk_962B = 0;
            marker1Enabled = 1;
        }
    }
    if (bgm->getMarkerHit(7)) {
        marker2Random = 1;
        random->next();
        if (random->next01() < chance) {
            marker1Enabled = 1;
            unk_962B = 0;
        }
    }
    soundLevelApply = 0;
    if (bgm->getMarkerHit(8)) {
        soundLevelApply = 1;
        soundLevelFull = 1;
    }
    if (bgm->getMarkerHit(9)) {
        soundLevelFull = 0;
        soundLevelApply = 1;
    }

    if (kind1) {
        if (otherStyle) {
            if (bgm->getMarkerHit(4))
                changeRequest = 4;
        } else {
            if (bgm->getMarkerHit(3))
                changeRequest = 3;
        }
        return;
    }
    if (bgm->getMarkerHit(3))
        changeRequest = 3;
    if (bgm->getMarkerHit(1) && !otherStyle) {
        marker1Armed = 0;
        if (marker1Enabled != 0) {
            marker1Armed = 1;
            changeRequest = 1;
        }
    }
    if (bgm->getMarkerHit(2) && !otherStyle) {
        if (marker2Armed != 0)
            changeRequest = 2;
        marker2Armed = 1;
        if (marker2Random != 0) {
            random->next();
            if (random->next01() < 0.5f)
                marker2Armed = 0;
        }
    }
}
