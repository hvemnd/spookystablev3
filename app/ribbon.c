/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: the ribbon renderer (ribbon.h). */

static void curve_at(const curve_t *c, float u, float *x, float *y, float *nx, float *ny)
{
    float v = 1.0f - u, tx, ty, l;
    *x = v * v * c->x0 + 2.0f * v * u * c->cx + u * u * c->x1;
    *y = v * v * c->y0 + 2.0f * v * u * c->cy + u * u * c->y1;
    tx = 2.0f * v * (c->cx - c->x0) + 2.0f * u * (c->x1 - c->cx);
    ty = 2.0f * v * (c->cy - c->y0) + 2.0f * u * (c->y1 - c->cy);
    l = fm_sqrtf(tx * tx + ty * ty);
    if (l < 0.001f)
        l = 1.0f;
    *nx = -ty / l;
    *ny = tx / l;
}

static void ribbon_draw(const curve_t *c, float waves, float amp, float th, uint16_t col, float phase, int bumps,
                        float shimmer, int segs)
{
    float px = 0, py = 0, qx = 0, qy = 0, zx = 0, zy = 0;
    uint16_t edge = lighten(col, 150u);
    int i;
    if (segs < 2)
        segs = 2;
    for (i = 0; i <= segs; i++) {
        float u = (float)i / (float)segs, x, y, nx, ny, env = fm_sinf(3.14159f * fm_minf(u * 1.4f + 0.12f, 1.0f));
        float w = amp * env * fm_sinf(TAU * waves * u - phase), cx, cy, ex, ey, sx, sy;
        curve_at(c, u, &x, &y, &nx, &ny);
        cx = x + nx * w;
        cy = y + ny * w;
        ex = cx - nx * th * 0.35f;            /* the light edge, a little to one side */
        ey = cy - ny * th * 0.35f;
        /* the sidebands: as many bumps per wave as the ratio, riding on the ribbon */
        {
            float z = shimmer * env * fm_sinf(TAU * waves * (float)(bumps > 0 ? bumps : 1) * u * 2.0f - phase * 3.0f);
            sx = cx + nx * z;
            sy = cy + ny * z;
        }
        if (i > 0) {
            cap(px, py, cx, cy, th * 0.5f + 0.8f, darken(col, 70u));
            cap(px, py, cx, cy, th * 0.5f, col);
            cap(qx, qy, ex, ey, fm_maxf(th * 0.18f, 0.6f), edge);
            if (shimmer > 0.3f)
                cap(zx, zy, sx, sy, 0.7f, U_WHITE);
        }
        px = cx;
        py = cy;
        qx = ex;
        qy = ey;
        zx = sx;
        zy = sy;
    }
}

static uint16_t blend_add(uint16_t a, uint16_t b, float wa, float wb)
{
    /* the merged colour, as paint mixes (which is what she knows): a multiply, so yellow and blue make green (light
     * would make white). With one ribbon out it is that one's colour; in between, a slide from one to the mix */
    uint32_t r = ((uint32_t)(a >> 11) * (uint32_t)(b >> 11)) / 31u;
    uint32_t g = ((uint32_t)((a >> 5) & 63u) * (uint32_t)((b >> 5) & 63u)) / 63u;
    uint32_t bl = ((uint32_t)(a & 31u) * (uint32_t)(b & 31u)) / 31u;
    uint16_t m = lighten((uint16_t)(r << 11 | g << 5 | bl), 80u);
    float d = wa - wb;
    if (wa < 0.05f)
        return b;
    if (wb < 0.05f)
        return a;
    return d > 0.0f ? mix565(m, a, (uint32_t)(256.0f * fm_minf(d, 1.0f))) : mix565(m, b, (uint32_t)(256.0f * fm_minf(-d, 1.0f)));
}
