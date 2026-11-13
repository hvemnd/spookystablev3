/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: Fart Mix-Up's two-operator FM (design 5.4), on v2's voice chain: the left unicorn's fart is the
 * carrier (FART_CARRIER, a plain sine), the right one's is the modulator. The mix <-> mod knob moves between the two
 * farts heard side by side (0: both plainly, the right at full level) and the right one turned into the left one's
 * timbre (1: the right voice silent, its pitch now the carrier's modulator at a high index): FM, taught by ear. Live,
 * through two v3 queue commands (snd_fm: ratio, index, feedback; snd_vel: level), so the 12-voice synth, its stealing,
 * its limiter and its 70 % cap are all v2's. A slow pad (MOD_CLOUD) under the merged voice is the cloud's own sound. */
#pragma once

enum { CL_NONE, CL_HAPPY, CL_OK, CL_CROSS };  /* the cloud's mood: consonance, by ear */

static void fm2_left(int on, float midi, float vel, int patch);
static void fm2_right(int on, float midi, float vel, int patch);
static void fm2_glide(int right, float midi);
static void fm2_update(float mix, float chaos);   /* every frame: the live FM follows the knobs and pitches */
static void fm2_all_off(void);
static int fm2_left_on(void);
static int fm2_right_on(void);
static float fm2_left_midi(void);
static float fm2_right_midi(void);
static int fm2_class(float semitones);       /* CL_*: octaves, fifths, fourths happy; thirds and sixths fine */
static int fm2_cloud(void);                  /* the cloud's mood now (CL_NONE: not both sounding) */
