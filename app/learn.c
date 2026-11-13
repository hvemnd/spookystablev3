/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: the challenge framework (learn.h). */

static void sticker_give(int id);            /* stickers.c */

#define LEARN_CALM_MS 1200u                  /* an invitation waits for this long a pause in her playing */
#define LEARN_GAP_MS 6000u                   /* and at least this long after the last challenge */
#define LEARN_ELSE_MS 3000u                  /* playing something else this long: the challenge dissolves */
#define LEARN_IDLE_MS 8000u                  /* nothing at all this long: the answer is shown again */
#define LEARN_CHEER_MS 1500u

static struct {
    int8_t lt;
    uint8_t phase, kind, misses, redemos, top, requested, first_visit;
    uint16_t free_n;
    uint32_t t0, last_rel, last_other, last_free, last_end;
    uint32_t right_t0, wrong_t0, solved_t0, master_t0, level_t0;
    float fx, fy;
} LN = {.lt = -1};

static learn_save_t *lsave(void) { return &set.learn[LN.lt >= 0 && LN.lt < NLEARN ? LN.lt : 0]; }
static int learn_enabled(void) { return set.school_on && !set.learn_off; }
static int learn_phase(void) { return LN.lt < 0 ? LP_OFF : LN.phase; }
static int learn_level(void) { return LN.lt < 0 ? 0 : ss_mini(lsave()->level, LN.top); }
static int learn_kind(void) { return LN.kind; }
static int learn_misses(void) { return LN.misses; }

static void learn_enter(int lt, int top_level)
{
    learn_save_t *s;
    uint32_t now = plat_ms();
    LN.lt = (int8_t)lt;
    LN.top = (uint8_t)top_level;
    LN.phase = LP_FREE;
    LN.free_n = 0;
    LN.requested = 0;
    LN.last_end = now - LEARN_GAP_MS;
    LN.last_free = now;
    s = lsave();
    if (s->visits < 255u)
        s->visits++;
    LN.first_visit = s->visits <= 1u;          /* a new toy is met by play alone (design 9.4) */
    if (s->level > LN.top)
        s->level = LN.top;
    knobs_save_soon();
}
static void learn_leave(void)
{
    sched_clear();
    LN.lt = -1;
    LN.phase = LP_OFF;
}

static void learn_free(void)
{
    if (LN.lt < 0)
        return;
    LN.last_free = plat_ms();
    if (LN.phase == LP_FREE && LN.free_n < 1000u)
        LN.free_n++;
}

static int learn_offer_due(void)
{
    uint32_t now = plat_ms();
    if (LN.lt < 0 || !learn_enabled() || LN.phase != LP_FREE || LN.first_visit)
        return 0;
    return LN.free_n >= 3u && now - LN.last_free >= LEARN_CALM_MS && now - LN.last_end >= LEARN_GAP_MS;
}
static void learn_request(void)
{
    if (LN.lt >= 0 && learn_enabled() && LN.phase == LP_FREE)
        LN.requested = 1;
}
static int learn_requested(void)
{
    int r = LN.requested;
    LN.requested = 0;
    return r && LN.phase == LP_FREE;
}

static void learn_begin(int kind)
{
    LN.phase = LP_DEMO;
    LN.kind = (uint8_t)kind;
    LN.misses = 0;
    LN.redemos = 0;
    LN.t0 = plat_ms();
    LN.requested = 0;
    arp_up(P_BELL_STING, 6, 3, 110u, 0.55f);  /* the challenge sting: three notes up the mood's scale */
    ui_led_pattern(3, 500u);
}
static void learn_wait(void)
{
    LN.phase = LP_WAIT;
    LN.t0 = LN.last_rel = plat_ms();
    LN.last_other = 0;
}
static void learn_round(int kind)
{
    LN.kind = (uint8_t)kind;
    LN.misses = 0;
    LN.redemos = 0;
    LN.requested = 0;
    learn_wait();
}
static void learn_set_misses(int n) { LN.misses = (uint8_t)(n < 0 ? 0 : n > 250 ? 250 : n); }
static void learn_touch(int relevant)
{
    uint32_t now = plat_ms();
    if (LN.phase != LP_WAIT && LN.phase != LP_DEMO)
        return;
    if (relevant)
        LN.last_rel = now;
    else
        LN.last_other = now | 1u;
}

static int learn_tick(void)
{
    uint32_t now = plat_ms();
    sched_tick();
    if (LN.lt < 0)
        return LE_NONE;
    if (LN.phase == LP_WAIT) {
        if (LN.last_other && (int32_t)(LN.last_other - LN.last_rel) > 0 && now - LN.last_rel >= LEARN_ELSE_MS) {
            learn_end();                       /* she is busy with something else: let it go, no fuss */
            return LE_ABANDON;
        }
        if (now - LN.last_rel >= LEARN_IDLE_MS && now - LN.last_other >= LEARN_IDLE_MS) {
            if (LN.redemos >= 2u) {            /* shown twice and still nothing: she is not interested */
                learn_end();
                return LE_ABANDON;
            }
            LN.redemos++;
            LN.last_rel = now;
            return LE_REDEMO;                  /* the app shows the answer (design 9.1) */
        }
    } else if (LN.phase == LP_CHEER && now - LN.solved_t0 >= LEARN_CHEER_MS) {
        LN.phase = LP_FREE;
        LN.last_end = now;
        LN.free_n = 0;
        return LE_CHEERED;
    }
    return LE_NONE;
}

static void learn_right(float x, float y)
{
    LN.right_t0 = plat_ms() | 1u;
    snd_play(P_SUCCESS_ARP, (float)mood_degree_midi(6, (int)LN.misses % 3 + 2), 0.45f);
    part_burst(PT_SPARKLE, x, y, 8, 1.0f, 3.0f, 18, CC_GOLD_L, 2);
    LN.last_rel = plat_ms();
}

static void learn_wrong(float x, float y)
{
    LN.wrong_t0 = plat_ms() | 1u;
    LN.fx = x;
    LN.fy = y;
    if (LN.misses < 250u)
        LN.misses++;
    snd_play(P_RETRY_BOING, (float)mood_degree_midi(5, 2), 0.55f);   /* soft and downward; never a buzzer */
    part_burst(PT_BUBBLE, x, y, 4, 0.5f, 1.5f, 24, U_WHITE, 3);
    LN.last_rel = plat_ms();
}

static void learn_record(int ok)
{
    learn_save_t *s = lsave();
    int i, n, right = 0;
    if (synth_key)                             /* (auto-play or a replay answered: celebrated, never counted) */
        return;
    s->hist = (uint8_t)((s->hist << 1) | (ok ? 1u : 0u));
    if (s->nhist < 8u)
        s->nhist++;
    n = s->nhist;
    for (i = 0; i < n; i++)
        right += (s->hist >> i) & 1;
    if (n >= 4) {                              /* the rolling window: 70-80 % is the aim */
        if (right * 100 > 85 * n && s->level < LN.top) {
            s->level++;
            s->nhist = 0;
            LN.level_t0 = plat_ms() | 1u;      /* a subtle level-up twinkle (no number, no text) */
            snd_play(P_CHIME, (float)mood_degree_midi(7, 0), 0.3f);
        } else if (right * 100 < 50 * n && s->level > 0u) {
            s->level--;
            s->nhist = 0;
        }
    }
    knobs_save_soon();
}

static void learn_solved(float x, float y)
{
    learn_save_t *s = lsave();
    int ok = LN.misses <= 1u;
    uint32_t now = plat_ms();
    learn_record(ok);
    LN.phase = LP_CHEER;
    LN.solved_t0 = now;
    LN.fx = x;
    LN.fy = y;
    if (s->wins < 255u)
        s->wins++;
    s->streak = ok ? (uint8_t)(s->streak + 1u) : 0u;
    /* the celebration, at once: a rising major-ish arpeggio, sparkle rain, a little shake of joy */
    arp_up(P_SUCCESS_ARP, 5, 4, 70u, 0.6f);
    part_burst(PT_SPARKLE, x, y, 20, 1.5f, 5.0f, 30, CC_GOLD_L, 3);
    part_burst(PT_STAR, x, y, 10, 1.0f, 4.0f, 30, 0, 3);
    {
        int i;                                 /* sparkle rain from the top */
        for (i = 0; i < 14; i++)
            part_spawn(PT_SPARKLE, 10.0f + (float)i * 16.0f, 4.0f, ss_rnds() * 0.3f, 1.0f + ss_rndf(), 50, RAINBOW[i % 6], 2);
    }
    fx_trauma(0.2f);
    ui_led_flash_all(120u);
    if (ok && s->streak > 0u && s->streak % 3u == 0u) {   /* the mastery moment: confetti, a dance, a fanfare */
        LN.master_t0 = now | 1u;
        sched_note(380u, P_STAB, (float)mood_degree_midi(5, 0), 0.6f);
        sched_note(520u, P_STAB, (float)mood_degree_midi(5, 2), 0.6f);
        sched_note(660u, P_STAB, (float)mood_degree_midi(5, 4), 0.6f);
        sched_note(800u, P_STAB, (float)mood_degree_midi(6, 0), 0.7f);
        part_burst(PT_CONFETTI, 120.0f, 90.0f, 40, 2.0f, 7.0f, 40, 0, 4);
        ui_dancers(3000u);
        sticker_give(LN.lt == LT_PARADE ? 5 : LN.lt == LT_MIXUP ? 11 : LN.lt == LT_PIANO ? 15 : 20);
    }
}

static void learn_end(void)
{
    if (LN.lt < 0)
        return;
    LN.phase = LP_FREE;
    LN.last_end = plat_ms();
    LN.free_n = 0;
    LN.requested = 0;
}

static float learn_wobble(void)
{
    uint32_t a = plat_ms() - LN.wrong_t0;
    return LN.wrong_t0 && a < 600u ? 1.0f - (float)a / 600.0f : 0.0f;
}
static int learn_dancing(void) { return LN.master_t0 && plat_ms() - LN.master_t0 < 2500u; }

static void learn_draw(void)
{
    uint32_t now = plat_ms();
    dr_ox = dr_oy = 0;
    if (LN.level_t0 && now - LN.level_t0 < 900u) {   /* level up: a twinkle by the apple */
        float a = (float)(now - LN.level_t0) / 900.0f;
        star4(214.0f, 14.0f + 6.0f * a, 6.0f * (1.0f - a), U_WHITE);
        star4(222.0f - 10.0f * a, 26.0f, 4.0f * (1.0f - a), CC_GOLD_L);
    }
    if (LN.phase == LP_CHEER || (LN.solved_t0 && now - LN.solved_t0 < LEARN_CHEER_MS)) {   /* a big happy star pops */
        float a = (float)(now - LN.solved_t0), s = ease_out_elastic(a / 500.0f) * (a > 1200.0f ? 1.0f - (a - 1200.0f) / 300.0f : 1.0f);
        if (s > 0.05f && a < LEARN_CHEER_MS) {
            float x = fm_clampf(LN.fx, 30.0f, 210.0f), y = fm_clampf(LN.fy - 30.0f, 30.0f, 200.0f);
            star4(x, y, 18.0f * s + 2.0f, U_LINE);
            star4(x, y, 18.0f * s, CC_GOLD);
            star4(x, y, 9.0f * s, CC_GOLD_L);
        }
    }
}

/* ------------------------------------------------------------- the scheduler --- */
#define NSCHED 24
static struct {
    uint32_t at;
    uint8_t on, patch;
    float midi, vel;
} SCH[NSCHED];

static void sched_note(uint32_t delay_ms, int patch, float midi, float vel)
{
    int i;
    if (!delay_ms) {
        snd_play(patch, midi, vel);
        return;
    }
    for (i = 0; i < NSCHED; i++)
        if (!SCH[i].on) {
            SCH[i].on = 1;
            SCH[i].at = plat_ms() + delay_ms;
            SCH[i].patch = (uint8_t)patch;
            SCH[i].midi = midi;
            SCH[i].vel = vel;
            return;
        }
}
static int sched_pending(void)
{
    int i, n = 0;
    for (i = 0; i < NSCHED; i++)
        n += SCH[i].on;
    return n;
}
static void sched_clear(void)
{
    int i;
    for (i = 0; i < NSCHED; i++)
        SCH[i].on = 0;
}
static void sched_tick(void)
{
    uint32_t now = plat_ms();
    int i;
    for (i = 0; i < NSCHED; i++)
        if (SCH[i].on && (int32_t)(now - SCH[i].at) >= 0) {
            SCH[i].on = 0;
            snd_play(SCH[i].patch, SCH[i].midi, SCH[i].vel);
        }
}
static void arp_up(int patch, int octave, int n, uint32_t gap, float vel)
{
    int i, step = n <= 3 ? 2 : 1;              /* three notes: a triad; more: up the scale (the mood's, so it fits) */
    for (i = 0; i < n; i++)
        sched_note((uint32_t)i * gap, patch, (float)mood_degree_midi(octave, i * step), vel);
}

/* --------------------------------------------------------------- dot groups --- */
static void draw_dots(float x, float y, int n, float r, int lit, uint16_t on, uint16_t off)
{
    static const int8_t PAT[6][6][2] = {
        {{0, 0}},
        {{-1, -1}, {1, 1}},
        {{-1, -1}, {0, 0}, {1, 1}},
        {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}},
        {{-1, -1}, {1, -1}, {0, 0}, {-1, 1}, {1, 1}},
        {{-1, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {1, 1}}};
    int i;
    float g = r * 2.4f;
    for (i = 0; i < n; i++) {
        float dx, dy;
        if (n <= 6) {
            dx = (float)PAT[n - 1][i][0] * g;
            dy = (float)PAT[n - 1][i][1] * g;
        } else {                               /* more than six: two rows */
            dx = ((float)(i % ((n + 1) / 2)) - (float)((n + 1) / 2 - 1) * 0.5f) * g;
            dy = i < (n + 1) / 2 ? -g * 0.5f : g * 0.5f;
        }
        disc(x + dx, y + dy, r + 1.0f, U_LINE);
        disc(x + dx, y + dy, r, i < lit ? on : off);
    }
}
