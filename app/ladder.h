/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: the pitch ladder (design 7): 3, 5 or 7 rungs up a vertical rail, the pre-staff. Higher rung,
 * higher note. Drawn with its rails, its rungs, a glow on the lit ones; hit-tested by height. */
#pragma once

static float ladder_y(float ytop, float ybot, int rungs, int i);   /* rung i's height (0: the bottom one) */
static int ladder_hit(float ytop, float ybot, int rungs, float y, float tol);   /* the rung at y (within tol), -1 */
static void ladder_draw(float x, float ytop, float ybot, int rungs, float w, const float *lit, const uint8_t *want,
                        float want_pulse);
