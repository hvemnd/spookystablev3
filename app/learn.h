/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: the challenge framework (design 9). A learning toy is a toy first: free play is the default and
 * a challenge is a short invitation layered on top. This module owns what every challenge shares:
 *   pacing      an invitation only after 3 free-play actions, at a calm moment (she paused), never two without free
 *               play between, never on a toy's first visit (the Challenge key or a tap of the toy's own button asks
 *               for one at any time); if she plays something else for 3 s the challenge quietly dissolves; idle 8 s
 *               the answer is shown again
 *   grammar     no fail states: right = sparkle rain and a rising arpeggio; wrong = a soft downward boing and the
 *               target shown again (never a buzzer, never red, nothing lost); 3 right in a row = the mastery dance
 *   difficulty  one axis per toy, the last 8 attempts, aiming at 70-80 %: above 85 % one step up, below 50 % one
 *               step back; kept in the flash (settings_t.learn)
 *   scheduler   a few notes in the future (stings, arpeggios, a challenge's demo)
 * When School mode is off no learning toy is open, and learn_tick returns at once: nothing here runs. */
#pragma once

enum { LT_PARADE, LT_MIXUP, LT_PIANO, LT_WALL };            /* NLEARN (knobs.h) */
enum { LP_OFF, LP_FREE, LP_DEMO, LP_WAIT, LP_CHEER };
enum { LE_NONE, LE_REDEMO, LE_ABANDON, LE_CHEERED };          /* learn_tick's news for the toy */

static int learn_enabled(void);              /* School mode on and the parent's switch not off */
static void learn_enter(int lt, int top_level);
static void learn_leave(void);
static int learn_phase(void);
static int learn_level(void);
static int learn_kind(void);
static int learn_misses(void);
static void learn_free(void);                /* a free-play action (counts toward an invitation) */
static int learn_offer_due(void);            /* 1: a calm moment to invite one (the toy says what) */
static void learn_request(void);             /* she asked for one (the Challenge key, a tap of the toy's button) */
static int learn_requested(void);            /* 1 once after learn_request */
static void learn_begin(int kind);           /* the challenge sting; the toy's demo starts */
static void learn_wait(void);                /* the demo is done: her turn */
static void learn_round(int kind);           /* a game that is its own challenge (a Match wall): no sting, her turn */
static void learn_set_misses(int n);         /* (such a game judges its own tries) */
static void learn_touch(int relevant);       /* her input during a challenge */
static int learn_tick(void);                 /* LE_* */
static void learn_right(float x, float y);   /* one step of the answer right (a ding) */
static void learn_wrong(float x, float y);   /* a try that was not it: the soft retry */
static void learn_solved(float x, float y);  /* the whole answer: the celebration, the record */
static void learn_end(void);                 /* back to free play */
static float learn_wobble(void);             /* 0..1: the target wobbles after a retry */
static int learn_dancing(void);              /* the mastery dance is on */
static void learn_draw(void);                /* the shared overlays (apple wiggle is school.c's) */

/* a few notes in the future */
static void sched_note(uint32_t delay_ms, int patch, float midi, float vel);
static void sched_clear(void);
static void sched_tick(void);
static void arp_up(int patch, int octave, int n, uint32_t gap, float vel);   /* n degrees up the mood's scale */

/* dot groups: quantity without digits (1..6 in dice patterns, more in rows) */
static void draw_dots(float x, float y, int n, float r, int lit, uint16_t on, uint16_t off);
