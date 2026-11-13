/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: the wavy fart ribbon (design 5.2). A ribbon follows a quadratic curve; along it a wave whose
 * frequency is the pitch, whose amplitude is the level and whose thickness is the volume; drawn as two polylines (the
 * body and its light edge) of capsules. With modulation, a fast zigzag rides on it with as many bumps per wave as the
 * ratio: sidebands you can see, and count. At most ~2 x segs + segs draw calls a ribbon. */
#pragma once

typedef struct {
    float x0, y0, cx, cy, x1, y1;             /* from, the bend, to */
} curve_t;

static void curve_at(const curve_t *c, float u, float *x, float *y, float *nx, float *ny);   /* the point and its normal */
static void ribbon_draw(const curve_t *c, float waves, float amp, float th, uint16_t col, float phase, int bumps,
                        float shimmer, int segs);
static uint16_t blend_add(uint16_t a, uint16_t b, float wa, float wb);   /* the colour of two ribbons merged (as paint) */
