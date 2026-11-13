/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: the pitch ladder (ladder.h). */

static float ladder_y(float ytop, float ybot, int rungs, int i)
{
    if (rungs < 2)
        return ybot;
    return ybot - (ybot - ytop) * (float)i / (float)(rungs - 1);
}

static int ladder_hit(float ytop, float ybot, int rungs, float y, float tol)
{
    int i, best = -1;
    float bd = tol;
    for (i = 0; i < rungs; i++) {
        float d = ss_absf(ladder_y(ytop, ybot, rungs, i) - y);
        if (d <= bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

static void ladder_draw(float x, float ytop, float ybot, int rungs, float w, const float *lit, const uint8_t *want,
                        float want_pulse)
{
    int i;
    uint16_t wood = CC_BROWN, wood_l = lighten(CC_BROWN, 90u);
    cap(x - w * 0.5f, ytop - 12.0f, x - w * 0.5f, ybot + 14.0f, 3.0f, U_LINE);
    cap(x + w * 0.5f, ytop - 12.0f, x + w * 0.5f, ybot + 14.0f, 3.0f, U_LINE);
    cap(x - w * 0.5f, ytop - 12.0f, x - w * 0.5f, ybot + 14.0f, 2.0f, wood);
    cap(x + w * 0.5f, ytop - 12.0f, x + w * 0.5f, ybot + 14.0f, 2.0f, wood);
    for (i = 0; i < rungs; i++) {
        float y = ladder_y(ytop, ybot, rungs, i), l = lit ? lit[i] : 0.0f;
        uint16_t c = RAINBOW[i % 6];
        if (want && want[i]) {                /* a rung to reach: it pulses gently */
            float p = 0.5f + 0.5f * fm_sinf(want_pulse);
            glow(x, y, 16.0f + 4.0f * p, scale565(c, 120u), (uint32_t)(140.0f + 100.0f * p));
        }
        if (l > 0.02f)
            glow(x, y, 14.0f + 10.0f * l, scale565(c, 200u), (uint32_t)(255.0f * l));
        cap(x - w * 0.5f, y, x + w * 0.5f, y, 3.2f, U_LINE);
        cap(x - w * 0.5f, y, x + w * 0.5f, y, 2.4f, l > 0.02f ? mix565(wood_l, c, (uint32_t)(256.0f * fm_minf(1.0f, l * 1.5f))) : mix565(wood_l, c, 70u));
    }
}
