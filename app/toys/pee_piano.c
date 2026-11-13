/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: Pee Piano (School mode + LFO, design 7): pitch is height.
 * A ladder of 5 rungs stands on the right (3 or 7 with K4): the pre-staff. The unicorn pees an arc at it; the arc's
 * top is where it hits the ladder, and the rung it hits lights and sings its note. Keys are force, left to right:
 * a low key, a weak low arc and a low note; a high key, a strong high arc and a high note (the keybed's own up). Hold
 * a key, the stream runs and the flute sings with it; another key while it runs, the arc climbs or drops there, and
 * the flute glides. Every rung is a note of the mood's scale, so nothing can sound wrong.
 *   K2 stream power   K3 arc spread   K4 rungs: 3 / 5 / 7   K6 tolerance (how near a rung counts)   K7 chaos
 *   OCT-/OCT+ the flute's voice (Pee's five)
 * The challenges: Copy the Tune (3 rungs light in turn, growing to 5: pee them in the same order; a miss is a
 * slide-whistle "womp" and the right rung lights again) and Fill the Bucket (pee up to the line, then stop; too much,
 * she giggles and the bucket tips over, empty again). */

#define PN_X 58.0f
#define PN_Y 202.0f
#define PN_S 0.85f
#define PN_LX 196.0f
#define PN_TOP 46.0f
#define PN_BOT 164.0f
#define PN_RX (PN_X + 50.0f * PN_S)
#define PN_RY (PN_Y - 52.0f * PN_S)
#define BKT_X 192.0f
#define BKT_TOP 150.0f
#define BKT_BOT 206.0f
enum { NK_TUNE, NK_BUCKET, NNK };

static struct {
    uint32_t held;
    int8_t key;
    uint8_t active, tapering;
    float t0, taper_t0, apex, apex_t, rate;
    uint16_t voice;
    float lit[7], drop_t;
    int8_t on_rung, last_rung, judged_rung;
    float rung_t0;
    /* the challenge */
    uint8_t tune[5], ntune, ti, demo_on, demo_i, kind_next;
    float demo_at, hint_t0;
    int8_t hint_rung;
    float fill, line, tip_t0;
    uint8_t bucket;
} pn;

static const preset_t PIANO_PRESETS[] = {
    {P_PEE_FLUTE, IC_PIPE}, {P_PEE_OCARINA, IC_BELLS}, {P_PEE_WHISTLE, IC_SLIDE}, {P_PEE_PANPIPE, IC_WAVE}, {P_PEE_HUM, IC_KAZOO}};

static int pn_rungs(void) { return 3 + 2 * kv(K4); }
static float pn_rung_midi(int i) { return (float)mood_degree_midi(5, i); }
static float pn_tol(void) { return 6.0f + 26.0f * kf(K6); }
static float pn_target(uint32_t k)            /* a key's force -> the arc's top (higher key: higher) */
{
    float f = (float)k / 26.0f * (0.75f + 0.5f * kf(K2)) + (kf(K2) - 0.5f) * 0.25f;
    return ss_lerp(PN_BOT + 10.0f, PN_TOP - 8.0f, ss_clamp01(f));
}
static float pn_midi_at(float y)              /* the flute's note for the arc's height: the nearest rung's */
{
    int n = pn_rungs(), i = ladder_hit(PN_TOP, PN_BOT, n, y, 400.0f);
    return pn_rung_midi(i < 0 ? 0 : i);
}

static void pn_stop(void)
{
    if (pn.voice)
        snd_off(pn.voice);
    pn.voice = 0;
    pn.active = 0;
}

static void piano_init(void)
{
    int i;
    km_force();
    pn_stop();
    pn.held = 0;
    pn.key = -1;
    pn.apex = pn.apex_t = PN_BOT;
    for (i = 0; i < 7; i++)
        pn.lit[i] = 0.0f;
    pn.on_rung = pn.last_rung = pn.judged_rung = -1;
    pn.demo_on = 0;
    pn.bucket = 0;
    pn.fill = 0.0f;
    pn.hint_t0 = pn.tip_t0 = -10000.0f;
    pn.hint_rung = -1;
    learn_enter(LT_PIANO, 3);
}
static void piano_leave(void)
{
    pn_stop();
    learn_leave();
}

static void piano_key(uint32_t k, float vel)
{
    int lp = learn_phase();
    pn.held |= 1u << k;
    pn.key = (int8_t)k;
    pn.apex_t = pn_target(k);
    if (lp == LP_WAIT || lp == LP_DEMO)
        learn_touch(1);
    if (!pn.active) {
        note_t n = snd_defaults(toy_patch(), pn_midi_at(pn.apex_t), 0.55f + 0.35f * vel_f(vel));
        n.flags = NF_HOLD;
        n.vib = 0.3f;
        pn.active = 1;
        pn.t0 = GT.T;
        pn.apex = PN_BOT + 10.0f;             /* the arc grows from low up to its height: you see it climb */
        pn.voice = snd_note(&n);
        fx_flash(HEX(0x60C8FF));
        impact(0.08f, 2);
        if (lp == LP_FREE)
            learn_free();
    } else {
        snd_glide(pn.voice, pn_midi_at(pn.apex_t));
    }
    pn.tapering = 0;
}
static void piano_release(uint32_t k, uint32_t held)
{
    int j;
    (void)held;
    pn.held &= ~(1u << k);
    for (j = 26; j >= 0; j--)                 /* another still held: the arc goes back to it */
        if (pn.held >> j & 1u) {
            pn.key = (int8_t)j;
            pn.apex_t = pn_target((uint32_t)j);
            snd_glide(pn.voice, pn_midi_at(pn.apex_t));
            break;
        }
}

static void piano_mega(uint32_t k)            /* a rainbow fountain up every rung at once */
{
    int i;
    (void)k;
    for (i = 0; i < pn_rungs(); i++) {
        pn.lit[i] = 1.0f;
        sched_note((uint32_t)i * 70u, P_CHIME, pn_rung_midi(i) + 12.0f, 0.5f);
    }
    for (i = 0; i < 30; i++)
        part_spawn(PT_DROPLET, PN_RX, PN_RY, 2.0f + 4.0f * ss_rndf(), -3.0f - 4.0f * ss_rndf(), 70, RAINBOW[i % 6], 3);
    impact(0.5f, 4);
}

/* ------------------------------------------------------------ challenges --- */
static void pn_challenge(int kind)
{
    int lvl = learn_level(), i, n = pn_rungs();
    pn.demo_on = 1;
    pn.demo_i = 0;
    pn.demo_at = GT.T + 650.0f;
    pn.ti = 0;
    pn.hint_rung = -1;
    if (kind == NK_TUNE) {
        pn.ntune = (uint8_t)(lvl < 2 ? 3 : lvl == 2 ? 4 : 5);
        for (i = 0; i < pn.ntune; i++) {      /* a little tune: steps up and down, never the same rung twice running */
            int r;
            do
                r = ss_rndi(n);
            while (i > 0 && r == pn.tune[i - 1]);
            pn.tune[i] = (uint8_t)r;
        }
    } else {
        static const float LINE[4] = {0.45f, 0.6f, 0.75f, 0.55f};
        pn.bucket = 1;
        pn.fill = 0.0f;
        pn.line = LINE[lvl & 3] + (lvl == 3 ? 0.2f * ss_rndf() : 0.0f);
    }
    learn_begin(kind);
}

static void pn_demo_tick(void)
{
    if (!pn.demo_on || GT.T < pn.demo_at)
        return;
    if (learn_kind() == NK_TUNE && pn.demo_i < pn.ntune) {   /* the rungs light in turn, singing */
        int r = pn.tune[pn.demo_i];
        pn.lit[r] = 1.0f;
        snd_play(P_CHIME, pn_rung_midi(r), 0.6f);
        snd_play(toy_patch(), pn_rung_midi(r), 0.5f);
        pn.demo_i++;
        pn.demo_at = GT.T + 560.0f;
        return;
    }
    if (learn_kind() == NK_BUCKET && pn.demo_i == 0) {        /* the line blinks with a ding */
        snd_play(P_CHIME, (float)mood_degree_midi(6, 2), 0.5f);
        pn.hint_t0 = GT.T;
        pn.demo_i = 1;
        pn.demo_at = GT.T + 500.0f;
        return;
    }
    pn.demo_on = 0;
    learn_wait();
}

static void pn_judge_rung(int r)              /* the stream has stayed on rung r a moment */
{
    int want = pn.tune[pn.ti];
    if (r == want) {
        learn_right(PN_LX, ladder_y(PN_TOP, PN_BOT, pn_rungs(), r));
        pn.hint_rung = -1;
        if (++pn.ti >= pn.ntune) {
            learn_solved(PN_LX - 40.0f, 90.0f);
            sticker_give(ST_TUNE);
        }
    } else {                                   /* a slide-whistle "womp", and the right rung lights again */
        note_t n = snd_defaults(P_SLIDEWH, pn_rung_midi(r), 0.5f);
        n.bend = -1.0f;
        snd_note(&n);
        learn_wrong(PN_LX, ladder_y(PN_TOP, PN_BOT, pn_rungs(), r));
        pn.hint_rung = (int8_t)want;
        pn.hint_t0 = GT.T;
    }
}

/* --------------------------------------------------------------- update --- */
static void piano_update(float dt)
{
    int n = pn_rungs(), i, ev, lp;
    float el = GT.T - pn.t0;
    toy_crush = 0.2f * kf(K7);
    for (i = 0; i < 7; i++)
        pn.lit[i] = fm_maxf(0.0f, pn.lit[i] - dt / 700.0f);
    if (pn.active) {
        if (!pn.tapering && ((!pn.held && el > 400.0f) || el > 9000.0f)) {
            pn.tapering = 1;
            pn.taper_t0 = GT.T;
        }
        pn.rate = pn.tapering ? 1.0f - (GT.T - pn.taper_t0) / 300.0f : 1.0f;
        if (pn.rate <= 0.0f) {
            pn_stop();
            pn.on_rung = -1;
        }
        pn.apex += (pn.apex_t - pn.apex) * fm_minf(1.0f, dt / 140.0f);
    }
    /* the rung the arc's top is on */
    if (pn.active && !pn.bucket && el > 120.0f) {
        int r = ladder_hit(PN_TOP, PN_BOT, n, pn.apex, pn_tol());
        if (r != pn.on_rung) {
            pn.on_rung = (int8_t)r;
            pn.rung_t0 = GT.T;
            pn.judged_rung = -1;
            if (r >= 0) {                      /* a ding and a splash on the rung */
                snd_play(P_CHIME, pn_rung_midi(r) + 12.0f, 0.35f);
                part_burst(PT_DROPLET, PN_LX, ladder_y(PN_TOP, PN_BOT, n, r), 5, 1.0f, 2.5f, 16, 0, 2);
                if (r == n - 1)
                    sticker_give(ST_TOP_RUNG);
            }
        }
        if (r >= 0)
            pn.lit[r] = 1.0f;
        if (r >= 0 && pn.judged_rung != r && GT.T - pn.rung_t0 > 260.0f && learn_phase() == LP_WAIT && learn_kind() == NK_TUNE) {
            pn.judged_rung = (int8_t)r;
            pn_judge_rung(r);
        }
    }
    /* the bucket fills while she pees into it; too much and it tips */
    if (pn.bucket && pn.active && el > 150.0f && GT.T - pn.tip_t0 > 900.0f) {
        pn.fill += dt * (0.00012f + 0.00018f * (float)(pn.key < 0 ? 0 : pn.key) / 26.0f) * pn.rate;
        if (pn.fill > fm_minf(pn.line + 0.14f, 1.0f)) {   /* over the top: a giggle, it tips over, empty again */
            pn.tip_t0 = GT.T;
            pn.fill = 0.0f;
            snd_play(P_GIGGLE, (float)mood_degree_midi(6, 0), 0.7f);
            part_fan(PT_DROPLET, BKT_X, BKT_TOP, 14, -0.4f, 0.8f, 1.5f, 4.0f, 40, 0, 3);
            if (learn_phase() == LP_WAIT)
                learn_wrong(BKT_X, BKT_TOP);
        }
    }
    if (pn.bucket && !pn.active && learn_phase() == LP_WAIT && pn.fill > 0.02f && ss_absf(pn.fill - pn.line) < 0.09f) {
        learn_solved(BKT_X, BKT_TOP - 10.0f);   /* stopped at the line */
        sticker_give(ST_BUCKET);
    }
    pn_demo_tick();
    ev = learn_tick();
    lp = learn_phase();
    if (ev == LE_REDEMO) {
        if (learn_kind() == NK_TUNE) {
            pn.demo_on = 1;
            pn.demo_i = 0;
            pn.ti = 0;
            pn.demo_at = GT.T + 300.0f;
            learn_begin(NK_TUNE);
        } else {
            pn.hint_t0 = GT.T;
            snd_play(P_CHIME, (float)mood_degree_midi(6, 2), 0.5f);
        }
    } else if (ev == LE_ABANDON || ev == LE_CHEERED) {
        pn.demo_on = 0;
        pn.bucket = 0;
        pn.fill = 0.0f;
    }
    if (lp == LP_FREE && !pn.active)
        if (learn_requested() || learn_offer_due()) {
            pn_challenge(pn.kind_next % NNK);
            pn.kind_next++;
        }
    /* the lights: the keys of the rung she is on */
    g_led_lit = 0;
    g_led_mid = pn.held;
    if (pn.active && pn.key >= 0)
        g_led_lit = 1u << pn.key;
}

/* ----------------------------------------------------------------- draw --- */
static void draw_bucket(float T)
{
    float h = BKT_BOT - BKT_TOP, fy = BKT_BOT - h * fm_minf(pn.fill, 1.0f), ly = BKT_BOT - h * pn.line, a = T - pn.tip_t0;
    float tilt = a >= 0.0f && a < 900.0f ? 0.6f * fm_sinf(a / 900.0f * 3.14159f) : 0.0f, bl = T - pn.hint_t0;
    uint16_t metal = HEX(0x94A3B8);
    quad(BKT_X - 24.0f + tilt * 20.0f, BKT_TOP - tilt * 10.0f, BKT_X + 24.0f + tilt * 20.0f, BKT_TOP + tilt * 10.0f, BKT_X + 18.0f,
         BKT_BOT, BKT_X - 18.0f, BKT_BOT, U_LINE);
    quad(BKT_X - 22.0f + tilt * 20.0f, BKT_TOP - tilt * 10.0f + 1.0f, BKT_X + 22.0f + tilt * 20.0f, BKT_TOP + tilt * 10.0f + 1.0f,
         BKT_X + 17.0f, BKT_BOT - 1.0f, BKT_X - 17.0f, BKT_BOT - 1.0f, metal);
    if (pn.fill > 0.01f && tilt < 0.05f) {
        box(BKT_X - 18.0f, fy, 36.0f, BKT_BOT - fy - 1.0f, CC_PEE);
        ell(BKT_X, fy, 18.0f, 2.5f, lighten(CC_PEE, 120u));
    }
    if (learn_phase() != LP_FREE || pn.bucket) {   /* the fill line: a bright band (blinking when shown again) */
        uint32_t al = bl >= 0.0f && bl < 1200.0f ? (uint32_t)(140.0f + 110.0f * fm_sinf(bl / 70.0f)) : 230u;
        box_a(BKT_X - 26.0f, ly - 1.5f, 52.0f, 3.0f, CC_PINK, al);
        star4(BKT_X + 29.0f, ly, 4.0f, CC_PINK);
    }
    ell(BKT_X, BKT_TOP, 23.0f, 4.0f, darken(metal, 60u));
    arc_dots(BKT_X, BKT_TOP - 2.0f, 20.0f, 3.3f, 6.1f, 1.4f, U_LINE);   /* its handle */
}

static void piano_draw(float T)
{
    int n = pn_rungs(), i;
    float el = T - pn.t0, sp = 0.5f + 1.5f * kf(K3);
    uint8_t want[7] = {0, 0, 0, 0, 0, 0, 0};
    pose_t p;
    mood_draw_ground(206);
    /* the ladder (or the bucket) */
    if (pn.bucket) {
        draw_bucket(T);
    } else {
        if (learn_phase() == LP_WAIT && learn_kind() == NK_TUNE) {
            if (learn_level() == 0 && pn.ti < pn.ntune)
                want[pn.tune[pn.ti]] = 1;      /* (the first level: the next rung glows faintly) */
            if (pn.hint_rung >= 0 && T - pn.hint_t0 < 1400.0f)
                want[pn.hint_rung] = 1;
        }
        ladder_draw(PN_LX, PN_TOP, PN_BOT, n, 26.0f, pn.lit, want, T / 120.0f);
        if (learn_phase() == LP_WAIT && learn_kind() == NK_TUNE)   /* how far through the tune: dots at the top */
            draw_dots(PN_LX, 18.0f, pn.ntune, 2.5f, pn.ti, CC_GOLD, HEX(0x5A5070));
        ell(PN_LX, 210.0f, 30.0f, 6.0f, mix565(HEX(0x4FB8F5), mood->ground, 110u));   /* a puddle at its foot */
    }
    /* the stream: a parabola from her up to the arc's top at the ladder (or down into the bucket) */
    if (pn.active && el > 80.0f) {
        float x1 = pn.bucket ? BKT_X : PN_LX, y1 = pn.bucket ? BKT_TOP + 8.0f : pn.apex, x0 = PN_RX, y0 = PN_RY;
        float kq = (y0 - y1) / ((x0 - x1) * (x0 - x1)), grow = ss_clamp01((el - 80.0f) / 160.0f) * pn.rate;
        int segs = 14, j;
        uint16_t c1 = CC_PEE, c2 = HEX(0x78E6FF);
        if (pn.bucket) {                       /* into the bucket: up, over and down */
            float top = fm_minf(y0, y1) - 30.0f;
            for (j = 0; j < segs; j++) {
                float u0 = (float)j / (float)segs, u1 = (float)(j + 1) / (float)segs;
                if (u1 > grow)
                    break;
                {
                    float xa = ss_lerp(x0, x1, u0), xb = ss_lerp(x0, x1, u1);
                    float ya = (1 - u0) * (1 - u0) * y0 + 2 * u0 * (1 - u0) * top + u0 * u0 * y1;
                    float yb = (1 - u1) * (1 - u1) * y0 + 2 * u1 * (1 - u1) * top + u1 * u1 * y1;
                    cap(xa, ya, xb, yb, 2.6f, ((j + (int)(T / 60.0f)) & 1) ? c1 : c2);
                }
            }
        } else {
            for (j = 0; j < segs; j++) {
                float u0 = (float)j / (float)segs, u1 = (float)(j + 1) / (float)segs, xa, xb, ya, yb, w;
                if (u1 > grow)
                    break;
                xa = ss_lerp(x0, x1, u0);
                xb = ss_lerp(x0, x1, u1);
                w = sp * fm_sinf(T / 50.0f + (float)j);
                ya = y1 + kq * (xa - x1) * (xa - x1) + w * u0;
                yb = y1 + kq * (xb - x1) * (xb - x1) + w * u1;
                cap(xa, ya, xb, yb, 2.2f + 0.6f * sp, ((j + (int)(T / 60.0f)) & 1) ? c1 : c2);
            }
            if (grow >= 1.0f && ((uint32_t)(T / 50.0f) & 1u))
                part_spawn(PT_DROPLET, x1 - 2.0f, y1, -0.5f - ss_rndf() * sp, -0.5f - ss_rndf(), 14, ss_rnd() & 1u ? c1 : U_WHITE, 2);
        }
    }
    /* the unicorn, facing left, peeing to the right (proudly) */
    p = uni_pose(PN_X, PN_Y, PN_S);
    p.facing = -1;
    p.spread = 1.0f;
    if (pn.active) {
        p.horn_glow = el < 200.0f ? el / 200.0f : 0.6f + 0.4f * fm_sinf(T / 80.0f);
        p.eye_state = el < 200.0f ? EY_WIDE : EY_HAPPY;
        p.mouth_state = MO_GRIN;
        p.tail_phase = 0.8f;
        p.blush = 0.8f;
        p.look_x = 0.9f;
        p.look_y = (pn.apex - 120.0f) / 80.0f;   /* she watches her arc */
    }
    if (learn_dancing())
        p.eye_state = EY_HAPPY, p.arm_l = 0.6f + 0.3f * fm_sinf(T / 120.0f);
    uni_draw(&p);
    for (i = 0; i < 4; i++)                   /* grass in front */
        tri(10.0f + (float)i * 22.0f, 240.0f, 20.0f + (float)i * 22.0f, 240.0f, 15.0f + (float)i * 22.0f, 226.0f, mood->ground2);
}

static void piano_clean(void)
{
    pn_stop();
    pn.fill = 0.0f;
}

static const toy_def_t TOY_PIANO_DEF = {
    "PEE PIANO", B_LFO, TF_HOLD | TF_LEARN, piano_init, piano_leave, piano_key, piano_release, piano_mega, 0, piano_update,
    piano_draw, piano_clean, PIANO_PRESETS, 5,
    {{KI_FORCE, 21, 0, 10}, {KI_HEIGHT, 21, 0, 6}, {KI_LADDER, 3, 0, 1}, {KI_TARGET, 21, 0, 12}, {KI_CHAOS, 21, 0, 6}},
    0, TOY_PEE,
};
