/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: the toy registry's types (design 8.1). A toy is data: its button, its callbacks, its presets and
 * its five own knobs. Adding or reordering a toy is one line in app.c's TOYREG; nothing else switches on a toy.
 * v3: the four learning toys (School mode, school.c) follow v2's eight; they share their play toy's page button
 * (TF_LEARN), and school_route() picks which one a button opens. */
#pragma once

enum { TOY_FART, TOY_POOP, TOY_PEE, TOY_BURP, TOY_SNEEZE, TOY_DRUM, TOY_TOILET, TOY_BOOGER,
       TOY_PARADE, TOY_MIXUP, TOY_PIANO, TOY_WALL, NTOYS };
#define NTOYS_V2 8                              /* v2's eight: the play toys */
enum { K1, K2, K3, K4, K5, K6, K7, NKNOBS };   /* K1 volume and K5 mood are global; the rest the toy's */
#define NTKNOBS 5                               /* a toy's own: K2 K3 K4 K6 K7 */

/* the knob overlay's icons (knobs.c draws them; no text) */
enum { KI_VOLUME, KI_MOOD, KI_CLOCK, KI_SIZE, KI_PITCH, KI_BEND, KI_STYLE, KI_CHAOS, KI_HEIGHT, KI_FORCE, KI_SWING,
       KI_DRUM, KI_WATER, KI_BUBBLES, KI_GRAVITY, KI_ECHO, KI_GLOW, KI_WHIRL, KI_WALL, KI_CHARGE, KI_SPEED,
       KI_STEPS, KI_RATIO, KI_MIX, KI_LADDER, KI_TARGET, KI_GAME, KI_GRID, NKI };   /* (v3: the last seven) */

typedef struct {
    uint8_t icon, steps, wrap, def;            /* steps: positions (21: a smooth 0..20); wrap: a style that cycles */
} knob_spec_t;

#define TF_HOLD 1u                             /* the toy does its own thing with a held key (no generic charge) */
#define TF_LEARN 2u                            /* v3: a learning toy (School mode); never found by toy_of_button */

/* v3: a toy may take a panel button for itself (on_button); the kinds of press */
enum { BK_PRESS, BK_TAP, BK_LONG };

typedef struct {
    const char *name;                          /* for a screen reader, if one listens */
    uint8_t button_id;
    uint8_t flags;
    void (*init)(void);                        /* entering the toy */
    void (*leave)(void);                       /* leaving it: let go of held notes */
    void (*on_key)(uint32_t key, float vel);   /* any of the 27 keys: the main action, at once */
    void (*on_release)(uint32_t key, uint32_t held_ms);
    void (*on_mega)(uint32_t key);             /* a full charge released, or a mash */
    void (*on_knob)(int knob, int value);      /* optional: a knob of its own moved */
    void (*update)(float dt);                  /* game time; skipped in hitstop */
    void (*draw)(float T);
    void (*clean)(void);                       /* the cleanup gesture */
    const preset_t *presets;
    uint8_t preset_count;
    knob_spec_t knobs[NTKNOBS];                /* K2 K3 K4 K6 K7 */
    int (*on_button)(uint32_t b, int kind);    /* v3, optional: 1 = the toy took this button press (BK_*) */
    uint8_t play_toy;                          /* v3: a learning toy's play toy (TOY_*); a play toy's own index */
} toy_def_t;

static const toy_def_t *TOYREG[NTOYS];          /* app.c fills it */
static uint8_t cur_toy;
#define TOY (TOYREG[cur_toy])

/* the clocks every toy reads */
static struct {
    float T;                                   /* game time, ms: stops in hitstop */
    float fdt;                                 /* this frame's real ms */
} GT;
