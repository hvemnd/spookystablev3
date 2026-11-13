/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v2: drawing primitives on the 240x240 canvas (cv_px, byte-swapped RGB565).
 * World shapes (ell, disc, tri, box, cap ...) are moved by the screen shake (dr_ox, dr_oy); hl / hl_a are raw rows.
 * Everything is scanline fills: no per-pixel maths over the whole screen. */
#pragma once

static int32_t dr_ox, dr_oy;          /* the screen shake's offset for world shapes (fx.c sets it each frame) */
static uint16_t dr_solid;             /* != 0: every shape drawn in this one colour (an impact silhouette) */
static uint8_t dr_solid_on;
static uint32_t dr_glows;             /* glow sprites drawn this frame (at most DR_GLOW_MAX) */
#define DR_GLOW_MAX 8u
#ifdef PLAT_HOST
static uint32_t dr_calls, dr_depth;   /* v3, the tests: shapes drawn, a capsule or an arc counted once (the design's
                                       * per-toy draw-call budgets) */
#define DR_COUNT() (dr_depth ? (void)0 : (void)dr_calls++)
#define DR_IN() (dr_depth++)
#define DR_OUT() (dr_depth--)
static uint32_t dr_part, dr_part_max;  /* a section of a toy's drawing (the ribbons, the sequencer's grid) */
#define DR_MARK() uint32_t dr_mark0 = dr_calls
#define DR_SINCE() do { dr_part = dr_calls - dr_mark0; if (dr_part > dr_part_max) dr_part_max = dr_part; } while (0)
#else
#define DR_MARK() ((void)0)
#define DR_SINCE() ((void)0)
#define DR_COUNT() ((void)0)
#define DR_IN() ((void)0)
#define DR_OUT() ((void)0)
#endif

static void hl(int32_t x0, int32_t x1, int32_t y, uint16_t c);
static void hl_a(int32_t x0, int32_t x1, int32_t y, uint16_t c, uint32_t a);
static void ell_a(float x, float y, float rx, float ry, uint16_t c, uint32_t a);
static void ell(float x, float y, float rx, float ry, uint16_t c);
static void disc(float x, float y, float r, uint16_t c);
static void disc_a(float x, float y, float r, uint16_t c, uint32_t a);
static void ring_a(float x, float y, float rx, float ry, float th, uint16_t c, uint32_t a);
static void tri_a(float x0, float y0, float x1, float y1, float x2, float y2, uint16_t c, uint32_t a);
static void tri(float x0, float y0, float x1, float y1, float x2, float y2, uint16_t c);
static void quad(float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, uint16_t c);
static void box_a(float x, float y, float w, float h, uint16_t c, uint32_t a);
static void box(float x, float y, float w, float h, uint16_t c);
static void cap(float x0, float y0, float x1, float y1, float r, uint16_t c);
static void arc_dots(float x, float y, float r, float a0, float a1, float th, uint16_t c);
static void star4(float x, float y, float r, uint16_t c);
static void heart(float x, float y, float s, uint16_t c);
static void glow(float x, float y, float r, uint16_t tint, uint32_t amt);
static void draw_init(void);
static void dr_solid_begin(uint16_t c);
static void dr_solid_end(void);
static void dot3(float x, float y, uint16_t c);   /* v3: a 3-pixel dot straight on the rows (rulers, counters) */
