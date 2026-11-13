/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: Poop Parade's step sequencer (design 4): 3 lanes (kick, snare, hat) of 8 or 16 eighth-note
 * columns, a BPM and swing clock, and the playhead. Like Drum Jam's grid it runs in the audio render, so every
 * column lands sample-accurately; the main loop only edits the pattern (a lane is 16 bits, one write) and reads back
 * which columns were stomped, for the unicorn's feet. (Named steps_* so it never meets Drum Jam's seq_*.) */
#pragma once

#define STEPS_MAX 16
#define NLANES 3
enum { LANE_KICK, LANE_SNARE, LANE_HAT };

static void steps_advance(uint32_t m);       /* the render, every chunk (voice.c) */
static void steps_config(float bpm, float swing, int kit, int ncols);
static void steps_set_pitch(int lane, float midi);   /* the cartoon kit's notes (the mood's scale) */
static void steps_play(int on);
static int steps_playing(void);
static float steps_pos(void);                /* the playhead: column + how far to the next (0 .. ncols) */
static int steps_pop(uint8_t *col, uint8_t *mask);   /* the columns it played, for the screen */
static uint16_t steps_lane[NLANES];          /* the pattern (the main loop owns it; the render reads) */
