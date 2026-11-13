/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: the School Bell (design 2). A second dimension on the same buttons: Play mode (v2, unchanged)
 * and School mode, where four page buttons open a learning toy instead:
 *   HOME  Fart Mix-Up   EDIT  Poop Parade   LFO  Pee Piano   SEQ (Booger's button)  Booger Wall
 *   FX ENV GLO ARP: their play toy, with the apple in the corner (never a dead button)
 * The Bell is a long press of OCT- (a tap is still the preset back): a bright three-note bell in the mood's scale,
 * the apple pops in. Again: "recess!", a whoosh and confetti, back to Play mode. While School mode is on a small apple
 * sits in the top-right corner, OCT-'s light glows at the middle level and the keybed's dim layer rings like a bell
 * (a double pulse) instead of breathing (the FM-1's key LEDs are one colour, so "apple red" became a rhythm).
 * Holding a toy's own page button 1.5 s opens its learning toy directly and turns School mode on (the 0.7 s cleanup
 * sweep of v2 still comes first). The first three long presses ever get a big sparkle (how she finds out).
 * The parent's switch: hold OCT- within 5 s of power-on and turn K1 up: every challenge off (the learning toys stay,
 * as toys), kept in the flash; the same again turns them back on. No text either way: a sleepy apple, or a bright one. */
#pragma once

static int school_route(int play_toy);       /* the toy a page button opens now */
static int school_learn_of(int play_toy);    /* its learning toy, or -1 */
static void school_toggle(void);             /* the Bell */
static void school_direct(int play_toy);     /* a toy's button held 1.5 s */
static void school_discover(void);           /* a long press: the first three sparkle */
static void school_boot(void);
static int school_hatch(int k1_detents);     /* K1 turned: 1 if it was the parent's switch (OCT- held at boot) */
static int school_hatch_used(void);          /* this OCT- press was the parent's switch (no Bell for it) */
static void school_boot_press(void);         /* OCT- pressed again */
static void school_leds(uint32_t *lit_b, uint32_t *mid_b, uint32_t *ns);
static void school_draw(float T);
static void draw_apple(float x, float y, float s, int sleepy);
