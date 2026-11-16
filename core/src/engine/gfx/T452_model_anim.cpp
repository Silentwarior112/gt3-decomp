/* T452_model_anim.cpp - the animation tracks of a ModelSet (unit u102).
 *
 * An AnimSet is a count and that many tracks.  Each track has a type, a
 * speed and a list of target addresses in the model's command streams
 * (unaligned): stepping the track writes its new value into every target
 * with byte copies (U102_COPY4 / U102_COPY2, statement macros).
 *   types 1..4  an angle in degrees, wrapped into [0, 360]     (f32)
 *   type 5      a frame number in [min, max), rounded          (16 bits)
 *   type 6      a position in [0, 1) over the frames min..max:
 *               the fraction (f32 at +0) and the frame (16 bits at +5)
 *   type 7      a value wrapped into [0, 1)                    (f32)
 * T452 has no canonical header (no other unit uses it): the records and
 * prototypes are in model_set_internal.h.
 */
#include "engine/gfx/model_set_internal.h"

// FUNC 0x00225D90 anim_relocate_targets
void anim_relocate_targets(void **p, u32 n, int delta)
{
    u32 i;

    for (i = 0; i < n; i++) {
        U102_RELOC(*p, delta);
        p++;
    }
}

/* ---- types 1..4 --------------------------------------------------------- */

// FUNC 0x00225DC0 anim_angle_relocate
void anim_angle_relocate(AnimTrackF *t, int delta)
{
    anim_relocate_targets((void **)t->targets, t->nTargets, delta);
}

/* cur = start; the targets get start (copied from start, not from cur) */
// FUNC 0x00225DE8 anim_angle_reset
void anim_angle_reset(AnimTrackF *t)
{
    u32 i;

    t->cur = t->start;
    for (i = 0; i < t->nTargets; i++) {
        U102_COPY4(t->targets[i], &t->start);
    }
}

// FUNC 0x00225E58 anim_angle_step
void anim_angle_step(AnimTrackF *t, f32 dt)
{
    f32 angle;
    u32 i;

    if (dt == 0.0f)
        return;
    angle = t->cur + t->speed * dt;
    t->cur = angle;
    if (360.0f < angle)
        t->cur = angle - 360.0f;
    else if (angle < 0.0f)
        t->cur = angle + 360.0f;
    for (i = 0; i < t->nTargets; i++) {
        U102_COPY4(t->targets[i], &t->cur);
    }
}

/* ---- type 5 ------------------------------------------------------------- */

// FUNC 0x00225F18 anim_frame_relocate
void anim_frame_relocate(AnimTrackI *t, int delta)
{
    anim_relocate_targets((void **)t->targets, t->nTargets, delta);
}

/* The frame written is initial rounded: a double add of 0.5 (the soft-float
 * fptodp / dpadd / dptoli, = __extendsfdf2 / __adddf3 / __fixdfsi).  cur is
 * set twice (min first, then initial). */
// FUNC 0x00225F40 anim_frame_reset
void anim_frame_reset(AnimTrackI *t)
{
    s16 frame;
    u32 i;

    t->cur = (f32)t->min;
    frame = (int)(t->initial + 0.5);        /* implicit: fptodp, dpadd, dptoli */
    t->cur = t->initial;
    for (i = 0; i < t->nTargets; i++) {
        U102_COPY2(t->targets[i], &frame);
    }
}

/* Rounded as anim_frame_reset (fptodp / dpadd / dptoli); past max the frame
 * and cur go back by max - min. */
// FUNC 0x00225FE0 anim_frame_step
void anim_frame_step(AnimTrackI *t, f32 dt)
{
    int rounded;
    s16 frame;
    u32 i;

    if (dt == 0.0f)
        return;
    t->cur = t->cur + t->speed * dt;
    rounded = (int)(t->cur + 0.5);          /* implicit: fptodp, dpadd, dptoli */
    if (rounded >= t->max) {
        int range = t->max - t->min;
        rounded -= range;
        t->cur = t->cur - (f32)range;
    }
    frame = rounded;
    for (i = 0; i < t->nTargets; i++) {
        U102_COPY2(t->targets[i], &frame);
    }
}

/* ---- type 6 ------------------------------------------------------------- */

// FUNC 0x002260C0 anim_blend_relocate
void anim_blend_relocate(AnimTrackI *t, int delta)
{
    anim_relocate_targets((void **)t->targets, t->nTargets, delta);
}

/* frames = pos * (max - min) split into the frame (+ min) and the fraction
 * (which subtracts the frame converted as unsigned); written through the
 * pointers (an inline: retail keeps both addresses in registers). */
static inline void anim_blend_split(f32 frames, int min, u32 *frame, f32 *frac)
{
    *frame = (int)frames;
    *frac = frames - (f32)*frame;
    *frame += min;
}

// FUNC 0x002260E8 anim_blend_reset
void anim_blend_reset(AnimTrackI *t)
{
    u32 frame;
    f32 frac;
    int min = t->min;
    f32 pos = t->initial;
    u32 i;

    t->cur = pos;
    anim_blend_split((f32)(t->max - min) * pos, min, &frame, &frac);
    for (i = 0; i < t->nTargets; i++) {
        u8 *dst = t->targets[i];
        U102_COPY4(dst, &frac);
        U102_COPY2(dst + 5, &frame);
    }
}

// FUNC 0x002261D8 anim_blend_step
void anim_blend_step(AnimTrackI *t, f32 dt)
{
    u32 frame;
    f32 frac;
    u32 i;

    if (dt == 0.0f)
        return;
    t->cur = t->cur + t->speed * dt;
    if (t->cur < 0.0f) {
        do
            t->cur = t->cur + 1.0f;
        while (t->cur < 0.0f);
    } else if (1.0f <= t->cur) {
        do
            t->cur = t->cur - 1.0f;
        while (1.0f <= t->cur);
    }
    anim_blend_split((f32)(t->max - t->min) * t->cur, t->min, &frame, &frac);
    for (i = 0; i < t->nTargets; i++) {
        u8 *dst = t->targets[i];
        U102_COPY4(dst, &frac);
        U102_COPY2(dst + 5, &frame);
    }
}

/* ---- type 7 ------------------------------------------------------------- */

// FUNC 0x00226378 anim_cycle_relocate
void anim_cycle_relocate(AnimTrackF *t, int delta)
{
    anim_relocate_targets((void **)t->targets, t->nTargets, delta);
}

// FUNC 0x002263A0 anim_cycle_reset
void anim_cycle_reset(AnimTrackF *t)
{
    u32 i;

    t->cur = t->start;
    for (i = 0; i < t->nTargets; i++) {
        U102_COPY4(t->targets[i], &t->cur);
    }
}

// FUNC 0x00226410 anim_cycle_step
void anim_cycle_step(AnimTrackF *t, f32 dt)
{
    u32 i;

    if (dt == 0.0f)
        return;
    t->cur = t->cur + t->speed * dt;
    if (t->cur < 0.0f) {
        do
            t->cur = t->cur + 1.0f;
        while (t->cur < 0.0f);
    } else if (1.0f <= t->cur) {
        do
            t->cur = t->cur - 1.0f;
        while (1.0f <= t->cur);
    }
    for (i = 0; i < t->nTargets; i++) {
        U102_COPY4(t->targets[i], &t->cur);
    }
}

/* ---- the set ------------------------------------------------------------ */
/* Each walk reads the track's type before testing the track pointer for
 * NULL (as the original). */

// FUNC 0x00226518 animset_relocate
void animset_relocate(AnimSet *a, int delta)
{
    u32 i;

    anim_relocate_targets((void **)a->tracks, a->count, delta);
    for (i = 0; i < a->count; i++) {
        switch (a->tracks[i]->type) {
        case ANIM_ANGLE_1:
        case ANIM_ANGLE_2:
        case ANIM_ANGLE_3:
        case ANIM_ANGLE_4:
            if (a->tracks[i] != NULL)
                anim_angle_relocate((AnimTrackF *)a->tracks[i], delta);
            break;
        case ANIM_FRAME:
            if (a->tracks[i] != NULL)
                anim_frame_relocate((AnimTrackI *)a->tracks[i], delta);
            break;
        case ANIM_BLEND:
            if (a->tracks[i] != NULL)
                anim_blend_relocate((AnimTrackI *)a->tracks[i], delta);
            break;
        case ANIM_CYCLE:
            if (a->tracks[i] != NULL)
                anim_cycle_relocate((AnimTrackF *)a->tracks[i], delta);
            break;
        }
    }
}

// FUNC 0x00226640 animset_reset
void animset_reset(AnimSet *a)
{
    u32 i;

    for (i = 0; i < a->count; i++) {
        switch (a->tracks[i]->type) {
        case ANIM_ANGLE_1:
        case ANIM_ANGLE_2:
        case ANIM_ANGLE_3:
        case ANIM_ANGLE_4:
            if (a->tracks[i] != NULL)
                anim_angle_reset((AnimTrackF *)a->tracks[i]);
            break;
        case ANIM_FRAME:
            if (a->tracks[i] != NULL)
                anim_frame_reset((AnimTrackI *)a->tracks[i]);
            break;
        case ANIM_BLEND:
            if (a->tracks[i] != NULL)
                anim_blend_reset((AnimTrackI *)a->tracks[i]);
            break;
        case ANIM_CYCLE:
            if (a->tracks[i] != NULL)
                anim_cycle_reset((AnimTrackF *)a->tracks[i]);
            break;
        }
    }
}

// FUNC 0x00226748 animset_step
void animset_step(AnimSet *a, f32 dt)
{
    u32 i;

    for (i = 0; i < a->count; i++) {
        switch (a->tracks[i]->type) {
        case ANIM_ANGLE_1:
        case ANIM_ANGLE_2:
        case ANIM_ANGLE_3:
        case ANIM_ANGLE_4:
            if (a->tracks[i] != NULL)
                anim_angle_step((AnimTrackF *)a->tracks[i], dt);
            break;
        case ANIM_FRAME:
            if (a->tracks[i] != NULL)
                anim_frame_step((AnimTrackI *)a->tracks[i], dt);
            break;
        case ANIM_BLEND:
            if (a->tracks[i] != NULL)
                anim_blend_step((AnimTrackI *)a->tracks[i], dt);
            break;
        case ANIM_CYCLE:
            if (a->tracks[i] != NULL)
                anim_cycle_step((AnimTrackF *)a->tracks[i], dt);
            break;
        }
    }
}
