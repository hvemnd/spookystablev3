/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: Fart Mix-Up (School mode + HOME, design 5): two unicorns, two farts, one FM synth.
 * They stand in the bottom corners, tails up, facing away. The low half of the keybed is the left one's fart (warm:
 * yellow and orange), the high half the right one's (cool: blue and violet); a held key sustains, letting go fades.
 * Each fart is a wavy ribbon (its wiggle the pitch, its height the level) rising and curving to the glowing mixer in
 * the middle, where the two colours blend into one ribbon that climbs to the cloud. Turn ALGO (K6) from "mix" to
 * "mod": the two plainly separate farts become the left one's sound coloured by the right one (FM), and a fast
 * zigzag rides the merged ribbon with as many bumps a wave as the ratio. The cloud listens: octaves, fifths and
 * fourths make it smile and rain sparkles; seconds and the tritone make it go cross-eyed and sneeze confetti; no
 * "wrong", just a mood. A faint ladder on the right shows how high each fart is.
 *   K2 the left one's pitch, a glide   K3 the right one's   K4 ratio snap: free / 1:1 / 2:1 / 3:1 / 3:2 / 4:3 / 5:4
 *   (the right one follows the left at that ratio; chosen so 2:1 and 3:1 show two and three bumps)
 *   K6 mix <-> mod   K7 chaos (feedback, crunch, shimmer)   OCT-/OCT+ the right one's voice
 * The challenges: Make the Cloud Happy; Copy the Interval (an octave first, then a fifth); Count the Bumps (two or
 * three dots: turn K4 until the zigzag has that many, and play both). */

#define MX_LX 34.0f
#define MX_RX 206.0f
#define MX_Y 234.0f
#define MX_S 0.5f
#define MX_BLOBX 120.0f
enum { MK_HAPPY, MK_COPY, MK_BUMPS, NMK };

static const float MX_RATIO[7] = {0.0f, 1.0f, 2.0f, 3.0f, 1.5f, 1.33333f, 1.25f};
static const uint8_t MX_BUMPS[7] = {0, 1, 2, 3, 3, 4, 5};

static struct {
    uint32_t held_l, held_r;
    int8_t key_l, key_r;
    float act_l, act_r;                       /* 0..1 eased: how much each ribbon is out */
    float lvl_l, lvl_r;
    float cloud_t0, cross_t0, happy_t0, sneeze_t0;
    int8_t cloud, last_cloud;
    float phase;
    /* the challenge */
    float want;                               /* Copy: the interval, semitones; Bumps: the dots */
    float good_t0, bad_t0;
    uint8_t bad_counted, kind_next, demo_on;
    float demo_at;
    uint8_t demo_i;
    float hint_t0;
} mx;

static const preset_t MIXUP_PRESETS[] = {
    {P_FART_MOD, IC_HORN}, {P_PEE_HUM, IC_KAZOO}, {P_PEE_OCARINA, IC_BELLS}, {P_PEE_WHISTLE, IC_SLIDE}};

static float mx_left_midi(void)
{
    int k = mx.key_l < 0 ? 0 : mx.key_l;
    return quantize_to_scale((float)mood_key_midi((uint32_t)ss_mini(k * 2, 26), 3) + (float)(kv(K2) - 10));
}
static float mx_right_midi(void)
{
    int k = mx.key_r < 0 ? 0 : mx.key_r - 14, r = kv(K4);
    if (r > 0 && fm2_left_on())               /* ratio snap: the right one follows the left one */
        return fm2_left_midi() + 12.0f * fm_log2f(MX_RATIO[r]);
    return quantize_to_scale((float)mood_key_midi((uint32_t)ss_mini(k * 2, 26), 4) + (float)(kv(K3) - 10));
}

static void mixup_init(void)
{
    km_mixup();
    mx.held_l = mx.held_r = 0;
    mx.key_l = mx.key_r = -1;
    mx.act_l = mx.act_r = 0.0f;
    mx.cloud = mx.last_cloud = CL_NONE;
    mx.cloud_t0 = mx.cross_t0 = mx.happy_t0 = mx.sneeze_t0 = mx.hint_t0 = -10000.0f;
    mx.demo_on = 0;
    learn_enter(LT_MIXUP, 3);
}
static void mixup_leave(void)
{
    fm2_all_off();
    learn_leave();
}

static int mx_last_held(uint32_t mask, int lo, int hi)
{
    int k;
    for (k = hi; k >= lo; k--)
        if (mask >> k & 1u)
            return k;
    return -1;
}

static void mixup_key(uint32_t k, float vel)
{
    int lp = learn_phase();
    if (lp == LP_WAIT || lp == LP_DEMO)
        learn_touch(1);
    else
        learn_free();
    if (km[k].role == KR_LEFT) {
        mx.held_l |= 1u << k;
        mx.key_l = (int8_t)k;
        fm2_left(1, mx_left_midi(), 0.55f + 0.35f * vel_f(vel), P_FART_CARRIER);
        part_fan(PT_CLOUD, MX_LX + 25.0f, MX_Y - 28.0f, 3, -1.0f, 0.6f, 1.0f, 2.5f, 26, lighten(CC_ORANGE, 90u), 6);
    } else {
        mx.held_r |= 1u << k;
        mx.key_r = (int8_t)k;
        fm2_right(1, mx_right_midi(), 0.55f + 0.35f * vel_f(vel), toy_patch());
        part_fan(PT_CLOUD, MX_RX - 25.0f, MX_Y - 28.0f, 3, -2.1f, 0.6f, 1.0f, 2.5f, 26, lighten(CC_BLUE, 90u), 6);
    }
    impact(0.1f, 2);
    if (fm2_left_on() && fm2_right_on())
        sticker_give(ST_TWO_FARTS);
}

static void mixup_release(uint32_t k, uint32_t held)
{
    (void)held;
    if (km[k].role == KR_LEFT) {
        mx.held_l &= ~(1u << k);
        mx.key_l = (int8_t)mx_last_held(mx.held_l, 0, 13);
        if (mx.key_l < 0)
            fm2_left(0, 0.0f, 0.0f, 0);
    } else {
        mx.held_r &= ~(1u << k);
        mx.key_r = (int8_t)mx_last_held(mx.held_r, 14, 26);
        if (mx.key_r < 0)
            fm2_right(0, 0.0f, 0.0f, 0);
    }
}

static void mixup_mega(uint32_t k)            /* a mash: both blast at an octave, the cloud bursts with joy */
{
    (void)k;
    snd_play(P_FART_TUBA, (float)mood_degree_midi(3, 0), 0.9f);
    snd_play(P_FART_KAZOO, (float)mood_degree_midi(4, 0), 0.8f);
    part_burst(PT_SPARKLE, 120.0f, 40.0f, 30, 1.0f, 5.0f, 30, CC_GOLD_L, 3);
    mx.happy_t0 = GT.T;
    impact(0.5f, 4);
}

/* ------------------------------------------------------------ challenges --- */
static void mx_challenge(int kind)
{
    int lvl = learn_level();
    mx.good_t0 = mx.bad_t0 = -10000.0f;
    mx.bad_counted = 0;
    if (kind == MK_COPY) {
        static const float IV[4] = {12.0f, 7.0f, 7.0f, 5.0f};
        mx.want = IV[lvl & 3];
        if (lvl == 2 && ss_rnd() & 1u)
            mx.want = 12.0f;
    } else if (kind == MK_BUMPS) {
        mx.want = (float)(2 + (int)(ss_rnd() & 1u));
    }
    mx.demo_on = 1;
    mx.demo_i = 0;
    mx.demo_at = GT.T + 600.0f;
    learn_begin(kind);
}

static void mx_demo_tick(void)
{
    float root = (float)mood_degree_midi(3, 0);
    if (!mx.demo_on || GT.T < mx.demo_at)
        return;
    switch (learn_kind()) {
    case MK_COPY:                              /* the left one, the right one, then both together */
        if (mx.demo_i == 0)
            snd_play(P_FART_TUBA, root, 0.7f);
        else if (mx.demo_i == 1)
            snd_play(P_FART_KAZOO, root + mx.want, 0.7f);
        else {
            snd_play(P_FART_TUBA, root, 0.6f);
            snd_play(P_FART_KAZOO, root + mx.want, 0.6f);
        }
        mx.hint_t0 = GT.T;
        mx.demo_at = GT.T + 650.0f;
        if (++mx.demo_i >= 3u) {
            mx.demo_on = 0;
            learn_wait();
        }
        break;
    case MK_HAPPY:                             /* the cloud is grumpy: a sulky two-note sigh */
        snd_play(P_FART_SLIDE, root + 1.0f, 0.5f);
        sched_note(300u, P_FART_SLIDE, root, 0.5f);
        mx.sneeze_t0 = GT.T;
        mx.demo_on = 0;
        learn_wait();
        break;
    default:                                   /* the dots appear, one by one */
        snd_play(P_CHIME, (float)mood_degree_midi(6, mx.demo_i), 0.5f);
        mx.demo_at = GT.T + 350.0f;
        if (++mx.demo_i >= (uint8_t)mx.want) {
            mx.demo_on = 0;
            learn_wait();
        }
        break;
    }
}

static void mx_judge(void)                    /* called every frame while it is her turn */
{
    int k = learn_kind(), both = fm2_left_on() && fm2_right_on(), good = 0;
    float iv = fm2_right_midi() - fm2_left_midi();
    int d = (int)(ss_absf(iv) + 0.5f);
    if (!both) {
        mx.good_t0 = mx.bad_t0 = -10000.0f;
        mx.bad_counted = 0;
        return;
    }
    if (k == MK_HAPPY)
        good = mx.cloud == CL_HAPPY && (learn_level() == 0 || d % 12 != 0 || d > 0);
    else if (k == MK_COPY)
        good = mx.want >= 12.0f ? (d % 12 == 0 && d > 0) : d % 12 == (int)mx.want;
    else
        good = kv(K4) > 0 && MX_BUMPS[kv(K4)] == (uint8_t)mx.want && MX_RATIO[kv(K4)] >= 1.9f;
    if (good) {
        if (mx.good_t0 < 0.0f)
            mx.good_t0 = GT.T;
        if (GT.T - mx.good_t0 > 120.0f) {          /* (a moment of it: never a glide passing through) */
            learn_solved(120.0f, 60.0f);
            if (k == MK_COPY)
                sticker_give(ST_INTERVAL);
            mx.happy_t0 = GT.T;
        }
        mx.bad_t0 = -10000.0f;
    } else {
        mx.good_t0 = -10000.0f;
        if (mx.bad_t0 < 0.0f)
            mx.bad_t0 = GT.T;
        if (!mx.bad_counted && GT.T - mx.bad_t0 > (k == MK_BUMPS ? 900.0f : 600.0f)) {
            mx.bad_counted = 1;                 /* once a chord: a soft retry, then a hint after two */
            learn_wrong(120.0f, 50.0f);
            if (learn_misses() >= 2) {
                float root = (float)mood_degree_midi(3, 0);
                mx.hint_t0 = GT.T;
                sched_note(300u, P_FART_TUBA, root, 0.5f);
                sched_note(300u, P_FART_KAZOO, root + (k == MK_COPY ? mx.want : 12.0f), 0.5f);
            }
        }
    }
}

static void mixup_update(float dt)
{
    float mix = kf(K6), ch = kf(K7);
    int ev;
    toy_crush = 0.25f * ch;
    /* the knobs glide the held farts; the ratio snap keeps the right one on the left one */
    if (fm2_left_on())
        fm2_glide(0, mx_left_midi());
    if (fm2_right_on())
        fm2_glide(1, mx_right_midi());
    fm2_update(mix, ch);
    mx.act_l += ((fm2_left_on() ? 1.0f : 0.0f) - mx.act_l) * fm_minf(1.0f, dt / (fm2_left_on() ? 60.0f : 260.0f));
    mx.act_r += ((fm2_right_on() ? 1.0f : 0.0f) - mx.act_r) * fm_minf(1.0f, dt / (fm2_right_on() ? 60.0f : 260.0f));
    mx.phase += dt * 0.012f;
    /* the cloud listens */
    mx.cloud = (int8_t)fm2_cloud();
    if (mx.cloud != mx.last_cloud) {
        mx.cloud_t0 = GT.T;
        if (mx.cloud == CL_CROSS) {            /* cross-eyed, and a confetti sneeze */
            mx.sneeze_t0 = GT.T;
            part_fan(PT_CONFETTI, 120.0f, 46.0f, 14, 1.57f, 1.2f, 1.5f, 4.0f, 30, 0, 3);
            snd_play(P_SNEEZE_TINY, (float)mood_degree_midi(6, 0), 0.4f);
        } else if (mx.cloud == CL_HAPPY) {
            int d = (int)(ss_absf(fm2_right_midi() - fm2_left_midi()) + 0.5f);
            mx.happy_t0 = GT.T;
            if (d > 0 && d % 12 == 0)
                sticker_give(ST_OCTAVE);
            else if (d % 12 == 7)
                sticker_give(ST_FIFTH);
        }
        mx.last_cloud = mx.cloud;
    }
    if (mx.cloud == CL_HAPPY && ((uint32_t)GT.T / 90u) % 2u == 0u)   /* sparkle rain while it smiles */
        part_spawn(PT_SPARKLE, 80.0f + ss_rndf() * 80.0f, 62.0f, ss_rnds() * 0.3f, 1.0f, 30, RAINBOW[ss_rnd() % 6u], 2);
    if (mix > 0.9f && fm2_left_on() && fm2_right_on())
        sticker_give(ST_FM);
    /* the challenge */
    mx_demo_tick();
    if (learn_phase() == LP_WAIT)
        mx_judge();
    ev = learn_tick();
    if (ev == LE_REDEMO) {
        mx.demo_on = 1;
        mx.demo_i = 0;
        mx.demo_at = GT.T + 200.0f;
        learn_begin(learn_kind());
    } else if (ev == LE_ABANDON) {
        mx.demo_on = 0;
    }
    if (learn_phase() == LP_FREE && !fm2_left_on() && !fm2_right_on())
        if (learn_requested() || learn_offer_due()) {
            int lvl = learn_level(), kind = mx.kind_next % (lvl >= 2 ? NMK : 2);
            mx_challenge(kind);
            mx.kind_next++;
        }
    /* the lights: the two halves glow while their fart sounds */
    g_led_mid = (fm2_left_on() ? 0x3FFFu : 0u) | (fm2_right_on() ? 0x7FFC000u : 0u);
    g_led_lit = 0;
}

/* ----------------------------------------------------------------- draw --- */
static float mx_height(float midi)            /* pitch as height: the ladder's scale */
{
    float lo = (float)mood_degree_midi(3, 0), hi = (float)mood_degree_midi(6, 0);
    return 166.0f - 100.0f * ss_clamp01((midi - lo) / (hi - lo));
}

static void draw_cloud_face(float x, float y, float T)
{
    int c = mx.cloud;
    float a = T - mx.cloud_t0, w = c == CL_CROSS ? 3.0f * fm_sinf(T / 40.0f) * fm_maxf(0.0f, 1.0f - a / 800.0f) : 0.0f;
    int grumpy = learn_phase() == LP_WAIT && learn_kind() == MK_HAPPY && c != CL_HAPPY;
    if (c == CL_CROSS) {                       /* cross-eyed */
        disc(x - 12.0f + w, y, 4.0f, U_WHITE);
        disc(x + 12.0f + w, y, 4.0f, U_WHITE);
        disc(x - 10.0f + w, y + 1.0f, 2.0f, CC_INK);
        disc(x + 10.0f + w, y + 1.0f, 2.0f, CC_INK);
        ell(x + w, y + 12.0f, 5.0f, 4.0f, CC_INK);
    } else if (c == CL_HAPPY) {                /* a big smile, happy eyes */
        arc_dots(x - 12.0f, y + 2.0f, 4.0f, 3.5f, 5.9f, 1.6f, CC_INK);
        arc_dots(x + 12.0f, y + 2.0f, 4.0f, 3.5f, 5.9f, 1.6f, CC_INK);
        arc_dots(x, y + 4.0f, 10.0f, 0.3f, 2.85f, 2.0f, CC_INK);
        disc_a(x - 20.0f, y + 8.0f, 4.0f, CC_PINK, 180u);
        disc_a(x + 20.0f, y + 8.0f, 4.0f, CC_PINK, 180u);
    } else {
        float bl = ((uint32_t)(T / 2800.0f) % 3u == 0u && ss_fmod(T, 2800.0f) < 140.0f) ? 0.2f : 1.0f;
        ell(x - 12.0f, y, 2.5f, 3.0f * bl, CC_INK);
        ell(x + 12.0f, y, 2.5f, 3.0f * bl, CC_INK);
        if (grumpy)                            /* a little pout: waiting to be made happy */
            arc_dots(x, y + 16.0f, 6.0f, 3.6f, 5.8f, 1.6f, CC_INK);
        else if (c == CL_OK)
            arc_dots(x, y + 6.0f, 6.0f, 0.5f, 2.6f, 1.6f, CC_INK);
        else
            ell(x, y + 10.0f, 3.0f, 1.5f, CC_INK);
    }
}

static void mixup_draw(float T)
{
    float mix = kf(K6), ch = kf(K7), hl = mx_height(fm2_left_midi()), hr = mx_height(fm2_right_midi());
    float al = mx.act_l, ar = mx.act_r, both = fm_minf(al, ar), blob_y = 136.0f - 16.0f * both;
    int r = kv(K4), bumps = r > 0 ? MX_BUMPS[r] : (int)(fm_exp2f((fm2_right_midi() - fm2_left_midi()) / 12.0f) + 0.5f);
    uint16_t warm = mix565(CC_YELLOW, CC_ORANGE, (uint32_t)(ss_clamp01((166.0f - hl) / 100.0f) * 256.0f));
    uint16_t cool = mix565(CC_BLUE, CC_PURPLE, (uint32_t)(ss_clamp01((166.0f - hr) / 100.0f) * 256.0f));
    uint16_t merged = blend_add(warm, cool, al, ar);
    float cl_y = 40.0f, bob = 2.0f * fm_sinf(T / 600.0f), happy = mx.cloud == CL_HAPPY ? 1.0f : 0.0f;
    curve_t c;
    pose_t p;
    if (mx.cloud == CL_HAPPY && mix > 0.5f)
        merged = mix565(merged, U_WHITE, 40u);
    mood_draw_ground(222);
    /* the cloud */
    {
        uint16_t cc = mx.cloud == CL_CROSS ? mix565(lighten(mood->accent, 170u), HEX(0x9A9AA8), 120u) : lighten(mood->accent, 190u);
        float sw = mx.cloud == CL_CROSS ? 2.0f * fm_sinf(T / 50.0f) : 0.0f, puff = 1.0f + 0.06f * happy * fm_sinf(T / 120.0f);
        if (happy > 0.0f)
            glow(120.0f, cl_y + bob, 70.0f, scale565(mood->accent, 100u), 200u);
        disc(78.0f + sw, cl_y + 8.0f + bob, 24.0f * puff + 1.5f, U_LINE);
        disc(162.0f + sw, cl_y + 8.0f + bob, 24.0f * puff + 1.5f, U_LINE);
        disc(120.0f + sw, cl_y + bob, 36.0f * puff + 1.5f, U_LINE);
        disc(78.0f + sw, cl_y + 8.0f + bob, 24.0f * puff, cc);
        disc(162.0f + sw, cl_y + 8.0f + bob, 24.0f * puff, cc);
        disc(120.0f + sw, cl_y + bob, 36.0f * puff, cc);
        box(78.0f + sw, cl_y + 8.0f + bob, 84.0f, 24.0f * puff, cc);
        draw_cloud_face(120.0f + sw, cl_y + 6.0f + bob, T);
    }
    /* the two ribbons rise and curve to the mixer; the merged one climbs to the cloud */
    {
    DR_MARK();
    if (al > 0.02f) {
        c.x0 = MX_LX + 25.0f; c.y0 = MX_Y - 30.0f; c.cx = 50.0f; c.cy = hl - 10.0f; c.x1 = MX_BLOBX - 8.0f; c.y1 = blob_y;
        ribbon_draw(&c, 1.0f + (166.0f - hl) / 34.0f, 6.0f * al, 2.0f + 4.0f * al * (float)set.vol / 10.0f, warm, mx.phase, 0, 0.0f, 12);
    }
    if (ar > 0.02f) {
        c.x0 = MX_RX - 25.0f; c.y0 = MX_Y - 30.0f; c.cx = 190.0f; c.cy = hr - 10.0f; c.x1 = MX_BLOBX + 8.0f; c.y1 = blob_y;
        ribbon_draw(&c, 1.0f + (166.0f - hr) / 34.0f, 6.0f * ar * (1.0f - 0.6f * mix * al), 2.0f + 4.0f * ar * (float)set.vol / 10.0f,
                    cool, mx.phase * 1.1f, 0, 0.0f, 12);
    }
    if (al > 0.02f || ar > 0.02f) {
        float a = fm_maxf(al, ar), sh = both * mix * (4.0f + 3.0f * ch);
        c.x0 = MX_BLOBX; c.y0 = blob_y - 6.0f; c.cx = MX_BLOBX + 20.0f * fm_sinf(T / 900.0f); c.cy = 100.0f; c.x1 = MX_BLOBX; c.y1 = cl_y + 34.0f;
        ribbon_draw(&c, 1.5f + (166.0f - fm_minf(hl, hr)) / 42.0f, 5.0f * a, 3.0f + 5.0f * a, merged, mx.phase * 1.3f,
                    bumps, sh, 12);
        glow(MX_BLOBX, blob_y, 26.0f + 8.0f * both, scale565(merged, 200u), (uint32_t)(255.0f * a));
        disc(MX_BLOBX, blob_y, 10.0f + 4.0f * both + 1.5f, U_LINE);
        disc(MX_BLOBX, blob_y, 10.0f + 4.0f * both, merged);
        disc(MX_BLOBX - 3.0f, blob_y - 3.0f, 3.0f, lighten(merged, 160u));
    } else {                                   /* (the mixer, resting) */
        disc(MX_BLOBX, blob_y, 7.5f, U_LINE);
        disc(MX_BLOBX, blob_y, 6.0f, mix565(mood->accent, U_WHITE, 120u));
    }
    DR_SINCE();                                /* (the tests: the ribbon renderer's shapes, design 10.4: under ~150) */
    }
    /* the ladder: how high each one is (pitch is up and down) */
    {
        float lit[5] = {0, 0, 0, 0, 0};
        int i;
        for (i = 0; i < 5; i++) {
            float y = ladder_y(66.0f, 166.0f, 5, i);
            if (al > 0.3f && ss_absf(y - hl) < 15.0f)
                lit[i] = fm_maxf(lit[i], al * 0.8f);
            if (ar > 0.3f && ss_absf(y - hr) < 15.0f)
                lit[i] = fm_maxf(lit[i], ar * 0.8f);
        }
        ladder_draw(230.0f, 66.0f, 166.0f, 5, 12.0f, lit, 0, 0.0f);
        if (al > 0.05f)
            disc(226.0f, hl, 4.0f * al, warm);
        if (ar > 0.05f)
            disc(234.0f, hr, 4.0f * ar, cool);
    }
    /* the bump counter in the corner: the zigzag's bumps, counted as they pass */
    if (both > 0.5f && mix > 0.15f && bumps > 0 && bumps <= 6) {
        int lit = 1 + (int)(ss_fmod(mx.phase * 0.5f, 1.0f) * (float)bumps);
        box_a(4.0f, 196.0f, 34.0f, 24.0f, CC_INK, 140u);
        draw_dots(21.0f, 208.0f, bumps, 2.4f, lit, U_WHITE, HEX(0x5A5070));
    }
    /* the challenge's signs: Count the Bumps' dots; Copy's two ghost marks on the ladder */
    if (learn_kind() == MK_BUMPS && (learn_phase() == LP_WAIT || learn_phase() == LP_DEMO)) {
        int n = learn_phase() == LP_DEMO ? mx.demo_i : (int)mx.want;
        box_a(86.0f, 86.0f, 68.0f, 26.0f, CC_INK, 150u);
        draw_dots(120.0f, 99.0f, n, 3.5f, n, CC_GOLD, CC_GOLD);
    }
    if (learn_kind() == MK_COPY && T - mx.hint_t0 < 1500.0f) {
        float y0 = mx_height((float)mood_degree_midi(3, 0)), y1 = mx_height((float)mood_degree_midi(3, 0) + mx.want);
        ring_a(226.0f, y0, 6.0f, 6.0f, 1.5f, CC_ORANGE, 220u);
        ring_a(234.0f, y1, 6.0f, 6.0f, 1.5f, CC_BLUE, 220u);
    }
    /* the two unicorns, tails up, facing away */
    p = uni_pose(MX_LX, MX_Y, MX_S);
    p.facing = -1;
    p.tail_phase = 0.4f + 0.6f * al;
    p.eye_state = al > 0.5f ? EY_SQUINT : EY_OPEN;
    p.mouth_state = al > 0.5f ? MO_GRIN : MO_CLOSED;
    p.scale_y *= 1.0f - 0.05f * al * fm_sinf(T / 60.0f);
    p.look_x = 0.8f;
    if (learn_dancing() || happy > 0.0f)
        p.eye_state = EY_HAPPY, p.blush = 0.7f;
    uni_draw(&p);
    {
        pose_t q = pose_rest(MX_RX, MX_Y, MX_S);   /* (the second one: no rig of her own, same outfit) */
        q.accessory_id = (uint8_t)cur_hat();
        q.mane = set.mane;
        q.mane_phase = T + 400.0f;
        q.facing = 1;
        q.tail_phase = 0.4f + 0.6f * ar;
        q.eye_state = ar > 0.5f ? EY_SQUINT : EY_OPEN;
        q.mouth_state = ar > 0.5f ? MO_GRIN : MO_CLOSED;
        q.scale_y *= 1.0f - 0.05f * ar * fm_sinf(T / 55.0f);
        q.look_x = -0.8f;
        if (learn_dancing() || happy > 0.0f)
            q.eye_state = EY_HAPPY, q.blush = 0.7f;
        if (fx_silhouette())
            q.tint = mix565(mood->accent, U_WHITE, 70u);
        draw_unicorn(&q, 0);
    }
}

static void mixup_clean(void) { fm2_all_off(); mx.held_l = mx.held_r = 0; mx.key_l = mx.key_r = -1; }

static const toy_def_t TOY_MIXUP_DEF = {
    "FART MIX UP", B_HOME, TF_HOLD | TF_LEARN, mixup_init, mixup_leave, mixup_key, mixup_release, mixup_mega, 0,
    mixup_update, mixup_draw, mixup_clean, MIXUP_PRESETS, 4,
    {{KI_PITCH, 21, 0, 10}, {KI_PITCH, 21, 0, 10}, {KI_RATIO, 7, 0, 0}, {KI_MIX, 21, 0, 0}, {KI_CHAOS, 21, 0, 4}},
    0, TOY_FART,
};
