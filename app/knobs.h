/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: the knobs (design 4) and what is remembered across power cycles. All seven are live in every toy:
 * K1 volume and K5 mood everywhere, the other five the toy's own (each toy remembers its own). A turn shows an icon
 * and its level for 800 ms; smooth knobs ease so nothing jitters.
 * Panel: K1..K4 are the FM-1's K1..K4; K5 its SELECT knob, K6 ALGO, K7 PRESET (the encoders have no push switch). */
#pragma once

#define SET_MAGIC_V2 0x53535432u           /* "SST2": v2's record, migrated at load */
#define SET_MAGIC 0x53535433u              /* "SST3" */
#define NLEARN 4                           /* the learning toys that keep a difficulty (learn.c) */

/* v2's record, frozen: what a v2 unit has in its flash (knobs_load migrates it) */
typedef struct {
    uint32_t magic;
    uint8_t vol, quiet, mood, hat, mane, unlocked, rsv[2];
    uint8_t preset[NTOYS_V2];
    uint8_t knob[NTOYS_V2][NTKNOBS];
    uint32_t actions;
} settings_v2_t;

/* a learning toy's adaptive difficulty (learn.c): 8 bytes */
typedef struct {
    uint8_t level;                         /* its one difficulty axis, 0 .. the toy's top */
    uint8_t hist;                          /* the last 8 attempts, 1 = right (bit 0 the newest) */
    uint8_t nhist;                         /* how many of them count (0..8) */
    uint8_t visits;                        /* visits (to 255): the challenges begin on the second */
    uint8_t streak;                        /* right in a row */
    uint8_t wins;                          /* to 255 */
    uint8_t rsv[2];
} learn_save_t;

typedef struct {
    uint32_t magic;
    uint8_t vol;                           /* 0..10 (K1) */
    uint8_t quiet;
    uint8_t mood;                          /* K5 */
    uint8_t hat, mane;                     /* the wardrobe: 0 = the mood's own */
    uint8_t unlocked;                      /* wardrobe items unlocked */
    uint8_t rsv[2];
    uint8_t preset[NTOYS];
    uint8_t knob[NTOYS][NTKNOBS];
    uint32_t actions;                      /* every key press ever (the wardrobe's progression) */
    /* v3 */
    uint8_t school_on;                     /* School mode (the Bell) */
    uint8_t learn_off;                     /* the parent's switch: no challenges at all (Bell + K1 up at boot) */
    uint8_t discover_n;                    /* long presses seen (the first three get the big sparkle) */
    uint8_t rsv4;
    learn_save_t learn[NLEARN];
    uint32_t stickers;                     /* the Sticker Book: bit i = sticker i */
    uint16_t parade[3];                    /* Poop Parade's pattern: a lane's 16 steps a bit each (kick, snare, hat) */
    uint8_t rsv3[2];
} settings_t;
static settings_t set;

static void knobs_load(void);
static void knobs_save_soon(void);
static void knobs_tick(void);              /* saves when due */
static void knobs_poll(void);              /* the encoders */
static void knobs_enter_toy(void);         /* the eased values jump to the new toy's */
static int kv(int k);                      /* a knob's value (K2..K7 the current toy's) */
static float kf(int k);                    /* 0..1, eased */
static void knobs_update(float dt);
static void knobs_draw(void);
static void knobs_show(int icon, int steps, int value, uint32_t ms);
static void draw_kicon(int icon, float x, float y, float s, uint16_t c);
static int knob_slot(int k);               /* K2..K7 -> 0..4 (K5: -1) */
