/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: physical key -> virtual grid (design 10.3). The FM-1's 27 keys are one keybed, F3..G5: 16 white
 * keys in front, 11 black keys behind (plat.h's PLAT_WHITE_KEYS / PLAT_BLACK_KEYS, the same lists v2's key scan and
 * Drum Jam's thirds use). So the keybed has two physical rows and one long left-to-right axis; every learning toy's
 * layout is a table built from those two facts, never from guessed key ids:
 *   Poop Parade   3 lanes x 8 columns: the keybed's thirds are the lanes (low third the kick, as in Drum Jam), left to
 *                 right in each third the columns; the top three keys (24, 25, 26) Play, Clear and Challenge
 *   Fart Mix-Up   the low half (0..13) the left unicorn, the high half (14..26) the right one
 *   Booger Wall   cubbies in two rows: a black key opens the top cubby above it, a white key the bottom one; the
 *                 columns split the keybed's width evenly. Answer zones (1..5 dots) split it left to right
 *   Pee Piano     every key a stream force, left (weak, low) to right (strong, high) */
#pragma once

enum { KR_NONE, KR_STEP, KR_PLAY, KR_CLEAR, KR_CHALLENGE, KR_ANSWER, KR_CUBBY, KR_LEFT, KR_RIGHT, KR_FORCE };

typedef struct {
    uint8_t phys_row, phys_col;   /* 0 the white row (front), 1 the black row; position in half white keys */
    uint8_t vrow, vcol;           /* where it lands in the toy's grid */
    uint8_t role;                 /* KR_* (KR_ANSWER: vcol is the answer, 1..5) */
} keymap_t;

static keymap_t km[NKEYS];        /* the current toy's table */

static uint8_t km_x2(uint32_t k);                 /* a key's position along the keybed, in half white keys (0..30) */
static int km_black(uint32_t k);
static void km_parade(void);                      /* the tables, into km[] */
static void km_mixup(void);
static void km_cubbies(int cols);                 /* two rows of cols */
static void km_zones(int n);                      /* answers 1..n, left to right */
static void km_force(void);
static int km_find(int role, int vrow, int vcol); /* the first key with that place, or -1 (the demo's "ghost hand") */
