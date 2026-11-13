/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: the Sticker Book (design 9.3). 24 procedurally drawn stickers, won by mastery moments and by
 * first-time discoveries. No names, no text, no count: a page of pictures. Hold the Bell (OCT-) for two seconds to
 * open it; each key touches a sticker (the four rows of six are the keybed left to right), which wiggles and sings;
 * the ones near it wiggle a little too. Any panel button closes it. Every fourth new sticker also unlocks the
 * wardrobe's next hat or mane (v2's unlock system). */
#pragma once

#define NSTICKERS 24
enum {
    ST_FIRST_POOP, ST_PARADE, ST_COPY, ST_COUNT, ST_LEFTRIGHT, ST_PARADE_STAR,
    ST_TWO_FARTS, ST_OCTAVE, ST_FIFTH, ST_FM, ST_INTERVAL, ST_MIXUP_STAR,
    ST_TOP_RUNG, ST_TUNE, ST_BUCKET, ST_PIANO_STAR, ST_MATCH, ST_COUNT_BOOGERS,
    ST_SORT, ST_BIG_WALL, ST_WALL_STAR, ST_BELL, ST_SPARKLE, ST_ALL_SCHOOL
};

static void sticker_give(int id);            /* first time: the sticker pops up and flies to the apple */
static int sticker_has(int id);
static int sticker_count(void);
static void book_open(void);
static void book_close(void);
static int book_is_open(void);
static void book_key(uint32_t k);
static void book_draw(float T);
static void sticker_toast_draw(float T);
static void draw_sticker(int id, float x, float y, float s, float T);
