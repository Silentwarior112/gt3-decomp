/* T193_race_color_select.h - the random colours of the cars of a race
 * (translation unit T193, unit u041), 0x001767E0..0x00177108.
 *
 * race_pick_random_colors (RaceInformation::assignCars, T124, with the
 * generator g_00351DE0) gives every entry whose colour id is -1 one of its
 * car's colours.  color_table_build lists the candidate colours of every
 * entry (a fixed colour is one candidate of weight 1; otherwise every colour
 * of the car, weights 1, 63/64, (63/64)^2 .. normalised), divides each
 * candidate's weight by 1 + its similarity to every other candidate of the
 * race (the similarity to a fixed colour counts as s^64 * 10000), sorts the
 * candidates by weight (ascending) and normalises.  The pick walks the
 * list: candidate j is taken when a fresh random number 0..1 is below its
 * weight (the last one otherwise).  The table is rebuilt for every entry
 * that gets a colour, so the colours picked so far count as fixed. */
#ifndef GAME_T193_RACE_COLOR_SELECT_H
#define GAME_T193_RACE_COLOR_SELECT_H

#include "game/game.h"

class RaceOrganization;                     /* T125 (references only: forward declarations) */
class Random;                               /* T531 */

/* (? name, u041.)  A candidate colour of an entry.  0x18 bytes. */
struct ColorCandidate {
    u32 color;                              /* 0x00 the colour code */
    f32 weight;                             /* 0x04 */
    f32 r;                                  /* 0x08 the RGB direction (a unit vector) */
    f32 g;                                  /* 0x0C */
    f32 b;                                  /* 0x10 */
    f32 len;                                /* 0x14 the length of the RGB vector (0..sqrt 3; 1 for black) */
};
EGCS_SIZE(ColorCandidate, 0x18);

/* (? name, u041.)  The colours one entry can take.  0x304 bytes. */
struct ColorCandidates {
    int            count;                   /* 0x000 */
    ColorCandidate c[32];                   /* 0x004 */
};
EGCS_SIZE(ColorCandidates, 0x304);

/* (? name, u041.)  The candidates of every entry (a stack object of
 * race_pick_random_colors).  0x1520 bytes. */
struct ColorTable {
    int             count;                  /* 0x0000 the entries */
    ColorCandidates car[7];                 /* 0x0004 */
};
EGCS_SIZE(ColorTable, 0x1520);

/* 1 for identical colours; else the dot product of the RGB directions
 * scaled by (sqrt 3 - the distance of the RGB vectors) / sqrt 3, clamped
 * to 0..1. */
f32  color_similarity(const ColorCandidate *a, const ColorCandidate *b);   /* 0x001767E0 */
/* The colour's RGB (database record, not checked for NULL), 0..1 a
 * channel, as a unit direction and a length. */
void color_candidate_set(ColorCandidate *c, int color);                   /* 0x00176900 */
/* Insertion sort by weight, ascending. */
void color_candidates_sort(ColorCandidates *list);                        /* 0x00176AA8 */
/* The candidates of every entry of org (see the top). */
void color_table_build(RaceOrganization &org, ColorTable *tab);           /* 0x00176C28 */
/* A colour for every entry whose colour id is -1 (CarEquipments +0x8),
 * stored through CarEquipments::setColorCode.  rng: g_00351DE0 (gp-0x5C10). */
void race_pick_random_colors(RaceOrganization &org, Random *rng);         /* 0x00176FD0 */

#endif /* GAME_T193_RACE_COLOR_SELECT_H */
