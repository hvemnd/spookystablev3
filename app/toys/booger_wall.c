/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: Booger Wall (School mode + SEQ, Booger Flick's button; design 6): counting and memory.
 * A wall of little doors in two rows, as the keybed has two: a black key opens the door in the top row above it, a
 * white key the one in the bottom row; the columns split the keybed left to right (keymap.c). ALGO (K6) picks the
 * game, and every game is a toy first:
 *   Match      behind each door a booger with its own colour, shape and note; two the same stick to the shelf on
 *              top with a sparkle and a happy boing; two different wobble and close again. All found: a new wall,
 *              bigger as she gets good at it (2x2, 3x2, 4x2, 6x2: two rows, because the keybed has two)
 *   Count      the keybed is 3 answer zones (5 later), each shown as a dot group: a zone splats that many boogers,
 *              one by one, each a note higher. Challenge: some boogers land; press the zone with that many
 *   Sort       a booger falls; hungry mouths of the mood's colours wait below, one over each zone; send it to the one
 *              its colour. The wrong mouth spits it back up ("ptooey"), never lost; it waits if nobody catches it
 *   K2 fall speed (Sort)   K3 booger size   K4 the biggest wall   K6 the game   K7 chaos   OCT-/OCT+ the boogers' voice */

#define BW_X0 20.0f
#define BW_W 200.0f
#define BW_Y0 30.0f
#define BW_RH 54.0f
#define BW_BAND 166.0f
#define BW_UX 44.0f
#define BW_UY 238.0f
#define NCUB 12
#define NWB 12
enum { BG_MATCH, BG_COUNT, BG_SORT, NBG };

static const uint8_t BW_COLS[4] = {2, 3, 4, 6};

static struct {
    /* match */
    uint8_t cols, kind[NCUB], state[NCUB];    /* state: 0 closed, 1 open, 2 matched */
    float open_t0[NCUB], match_t0[NCUB];
    int8_t open_a, open_b;
    float close_at;
    uint8_t found, mism, round_on, game;
    float peek_t0, newwall_t0;
    /* count and sort: boogers on the wall / falling */
    struct { float x, y, vy, t0, born; uint8_t on, col, state, shape; } b[NWB];
    uint8_t zones, pend_n, pend_i, target, counting;
    float pend_at, recount_t0;
    uint8_t recount_i;
    float recount_at;
    int8_t mouth_open;
    float mouth_t0;
    uint8_t sorted, sort_miss, kind_next;
    float sneeze_t0, pick_t0;
} bw;

static const preset_t WALL_PRESETS[] = {
    {P_PLOP_MARIMBA, IC_BELLS}, {P_FLICK_TWANG, IC_STRING}, {P_FLICK_SPROING, IC_SPRING}, {P_SPLAT, IC_DROP}};
static const uint16_t BW_COLOR[6] = {CC_SNOT, CC_PINK, CC_YELLOW, CC_BLUE, CC_ORANGE, CC_PURPLE};

static int bw_game(void) { return kv(K6) % NBG; }
static float bw_size(void) { return 0.75f + 0.6f * kf(K3); }
static float bw_cubx(int c) { return BW_X0 + BW_W * ((float)c + 0.5f) / (float)bw.cols; }
static float bw_cuby(int r) { return BW_Y0 + BW_RH * ((float)r + 0.5f); }
static float bw_zonex(int z) { return 240.0f * ((float)z - 0.5f) / (float)bw.zones; }   /* z 1..zones */

static void draw_wall_booger(float x, float y, float r, int kind, float sx, float sy, float T)
{
    uint16_t c = BW_COLOR[kind % 6];
    int shape = kind / 6 + (kind % 3 == 2 ? 1 : 0);   /* (round for most; a star or a square for the rest) */
    if (shape == 1) {
        star4(x, y, r * 1.6f * sx + 1.5f, darken(c, 110u));
        star4(x, y, r * 1.6f * sx, c);
    } else if (shape >= 2) {
        box(x - r * sx - 1.0f, y - r * sy - 1.0f, 2.0f * r * sx + 2.0f, 2.0f * r * sy + 2.0f, darken(c, 110u));
        box(x - r * sx, y - r * sy, 2.0f * r * sx, 2.0f * r * sy, c);
    } else {
        ell(x, y, r * sx + 1.0f, r * sy + 1.0f, darken(c, 110u));
        ell(x, y, r * sx, r * sy, c);
    }
    disc(x - r * 0.35f * sx, y - r * 0.35f * sy, fm_maxf(r * 0.3f, 0.8f), lighten(c, 160u));
    if (r > 5.0f) {                            /* and a face */
        disc(x - r * 0.3f, y, r * 0.16f, CC_INK);
        disc(x + r * 0.3f, y, r * 0.16f, CC_INK);
        arc_dots(x, y + r * 0.2f, r * 0.3f, 0.5f, 2.6f, fm_maxf(r * 0.08f, 0.6f), CC_INK);
    }
    (void)T;
}

/* --------------------------------------------------------------- Match --- */
static void bw_new_wall(void)
{
    int i, n, lvl = learn_level(), cap = BW_COLS[kv(K4) & 3], cols = BW_COLS[lvl & 3];
    bw.cols = (uint8_t)(cols < cap ? cols : cap);
    if (!learn_enabled())
        bw.cols = (uint8_t)cap;                /* (no challenges: the wall she chose, every time) */
    n = bw.cols * 2;
    for (i = 0; i < n; i++)
        bw.kind[i] = (uint8_t)(i / 2);
    for (i = n - 1; i > 0; i--) {              /* shuffled */
        int j = ss_rndi(i + 1);
        uint8_t t = bw.kind[i];
        bw.kind[i] = bw.kind[j];
        bw.kind[j] = t;
    }
    for (i = 0; i < NCUB; i++) {
        bw.state[i] = 0;
        bw.open_t0[i] = bw.match_t0[i] = -10000.0f;
    }
    bw.open_a = bw.open_b = -1;
    bw.found = 0;
    bw.mism = 0;
    bw.newwall_t0 = GT.T;
    km_cubbies(bw.cols);
    if (bw.cols >= 4)
        sticker_give(ST_BIG_WALL);
}

static void bw_close_open(void)
{
    if (bw.open_a >= 0 && bw.state[bw.open_a] == 1u)
        bw.state[bw.open_a] = 0;
    if (bw.open_b >= 0 && bw.state[bw.open_b] == 1u)
        bw.state[bw.open_b] = 0;
    bw.open_a = bw.open_b = -1;
    bw.close_at = 0.0f;
}

static void bw_match_key(uint32_t k, float vel)
{
    int i = km[k].vrow * bw.cols + km[k].vcol, kd;
    if (i >= bw.cols * 2)
        return;
    kd = bw.kind[i];
    bw.pick_t0 = GT.T;
    learn_touch(1);
    if (bw.state[i] == 2u) {                   /* found already: the empty place twinkles */
        part_burst(PT_SPARKLE, bw_cubx(km[k].vcol), bw_cuby(km[k].vrow), 4, 0.5f, 1.5f, 12, U_WHITE, 2);
        snd_play(P_TICK, (float)mood_degree_midi(6, kd), 0.4f);
        return;
    }
    if (bw.state[i] == 1u) {                   /* open: it wiggles and sings again */
        bw.open_t0[i] = GT.T - 200.0f;
        snd_play(toy_patch(), (float)mood_degree_midi(4, kd), vel_f(vel));
        return;
    }
    if (bw.open_b >= 0)                        /* two already open: they close now, this one opens */
        bw_close_open();
    bw.state[i] = 1;
    bw.open_t0[i] = GT.T;
    snd_play(toy_patch(), (float)mood_degree_midi(4, kd), vel_f(vel));
    part_burst(PT_SLIME, bw_cubx(km[k].vcol), bw_cuby(km[k].vrow), 3, 0.5f, 1.5f, 14, BW_COLOR[kd % 6], 2);
    if (bw.open_a < 0) {
        bw.open_a = (int8_t)i;
        return;
    }
    bw.open_b = (int8_t)i;
    if (bw.kind[bw.open_a] == bw.kind[i]) {    /* a pair: both stick to the shelf on top */
        bw.state[bw.open_a] = bw.state[i] = 2;
        bw.match_t0[bw.open_a] = bw.match_t0[i] = GT.T;
        bw.found++;
        sched_note(120u, P_PLOP_BOING, (float)mood_degree_midi(5, kd), 0.6f);
        part_burst(PT_SPARKLE, bw_cubx(i % bw.cols), bw_cuby(i / bw.cols), 10, 1.0f, 3.0f, 18, CC_GOLD_L, 2);
        sticker_give(ST_MATCH);
        bw.open_a = bw.open_b = -1;
        if (bw.found >= bw.cols) {             /* the whole wall: a celebration, and a new wall after it */
            if (learn_enabled() && learn_phase() == LP_WAIT) {
                learn_set_misses(bw.mism <= bw.cols ? 0 : 2);   /* (many tries: still a celebration, counted as one) */
                learn_solved(120.0f, 80.0f);
            } else {
                arp_up(P_SUCCESS_ARP, 5, 4, 70u, 0.6f);
                part_burst(PT_CONFETTI, 120.0f, 80.0f, 30, 2.0f, 6.0f, 36, 0, 4);
            }
            bw.round_on = 0;
            bw.sneeze_t0 = GT.T + 300.0f;
        }
    } else {                                   /* not the same: they wobble, and close in a moment */
        bw.close_at = GT.T + 750.0f;
        snd_play(P_RETRY_BOING, (float)mood_degree_midi(4, 2), 0.35f);
        if (learn_enabled() && learn_phase() == LP_WAIT && bw.mism < 250u)
            bw.mism++;
    }
}

/* ------------------------------------------------------- Count and Sort --- */
static int bw_free_b(void)
{
    int i, oldest = 0;
    float ot = 1e9f;
    for (i = 0; i < NWB; i++) {
        if (!bw.b[i].on)
            return i;
        if (bw.b[i].born < ot) {
            ot = bw.b[i].born;
            oldest = i;
        }
    }
    return oldest;
}
static void bw_splat(int idx_in_count, int total)   /* one booger lands on the wall, a note higher than the last */
{
    int i = bw_free_b();
    float x = 40.0f + 160.0f * ((float)idx_in_count + 0.5f) / (float)(total > 0 ? total : 1) + ss_rnds() * 6.0f;
    float y = 60.0f + ss_rnds() * 22.0f + (float)(idx_in_count & 1) * 18.0f;
    bw.b[i].on = 1;
    bw.b[i].state = 1;                         /* on the wall */
    bw.b[i].x = x;
    bw.b[i].y = y;
    bw.b[i].t0 = bw.b[i].born = GT.T;
    bw.b[i].col = (uint8_t)(idx_in_count % 6);
    bw.b[i].shape = 0;
    snd_play(toy_patch(), (float)mood_degree_midi(4, idx_in_count), 0.7f);
    snd_play(P_SPLAT, 0.0f, 0.35f);
    part_burst(PT_SLIME, x, y, 4, 0.8f, 2.0f, 14, BW_COLOR[idx_in_count % 6], 2);
    fx_trauma(0.06f);
}
static void bw_clear_b(void)
{
    int i;
    for (i = 0; i < NWB; i++)
        if (bw.b[i].on && bw.b[i].state == 1u) {
            bw.b[i].state = 3;                 /* slides off */
            bw.b[i].t0 = GT.T;
        }
}
static int bw_on_wall(void)
{
    int i, n = 0;
    for (i = 0; i < NWB; i++)
        n += bw.b[i].on && bw.b[i].state == 1u;
    return n;
}

static void bw_count_key(uint32_t k)
{
    int z = km[k].vcol, lp = learn_phase();
    bw.pick_t0 = GT.T;
    if (lp == LP_WAIT && learn_kind() == BG_COUNT) {
        learn_touch(1);
        if (z == bw.target) {                  /* that many: the unicorn sneezes for joy */
            learn_solved(bw_zonex(z), BW_BAND);
            sticker_give(ST_COUNT_BOOGERS);
            bw.sneeze_t0 = GT.T;
            snd_play(P_SNEEZE_TRUMPET, (float)mood_degree_midi(5, 0), 0.7f);
            part_fan(PT_CONFETTI, BW_UX + 30.0f, BW_UY - 40.0f, 20, -1.0f, 0.8f, 2.0f, 6.0f, 36, 0, 3);
        } else {                               /* not that one: they count themselves again, one note each */
            learn_wrong(bw_zonex(z), BW_BAND);
            bw.recount_t0 = GT.T;
            bw.recount_i = 0;
            bw.recount_at = GT.T + 300.0f;
        }
        return;
    }
    if (lp == LP_DEMO)
        return;
    bw_clear_b();                              /* free play: that many boogers, one after another */
    bw.pend_n = (uint8_t)z;
    bw.pend_i = 0;
    bw.pend_at = GT.T;
    learn_free();
    ui_led_key(k);
}

static void bw_sort_spawn(void)
{
    int i = bw_free_b();
    bw.b[i].on = 1;
    bw.b[i].state = 2;                         /* falling */
    bw.b[i].x = 40.0f + ss_rndf() * 160.0f;
    bw.b[i].y = 26.0f;
    bw.b[i].vy = 0.0f;
    bw.b[i].born = bw.b[i].t0 = GT.T;
    bw.b[i].col = (uint8_t)ss_rndi(bw.zones);
    bw.b[i].shape = (uint8_t)(learn_level() >= 2 ? ss_rndi(3) : 0);
    snd_play(P_POP, (float)mood_degree_midi(6, bw.b[i].col), 0.4f);
}
static int bw_falling(void)
{
    int i;
    for (i = 0; i < NWB; i++)
        if (bw.b[i].on && (bw.b[i].state == 2u || bw.b[i].state == 4u))
            return i;
    return -1;
}

static void bw_sort_key(uint32_t k)
{
    int z = km[k].vcol, i = bw_falling(), want;
    bw.mouth_open = (int8_t)z;
    bw.mouth_t0 = GT.T;
    bw.pick_t0 = GT.T;
    if (i < 0 || bw.b[i].state != 2u) {       /* nothing falling: the mouth just goes "om nom" */
        snd_play(P_GULP, (float)mood_degree_midi(4, z), 0.4f);
        return;
    }
    want = bw.b[i].col + 1;
    learn_free();
    if (want == z || !learn_enabled()) {      /* yum (with no challenges, every mouth is happy to eat it) */
        bw.b[i].state = 5;                     /* flying into the mouth */
        bw.b[i].t0 = GT.T;
        bw.b[i].x = bw.b[i].x;
        bw.sorted++;
        snd_play(P_SLURP, (float)mood_degree_midi(4, z), 0.6f);
        sched_note(200u, P_SUCCESS_ARP, (float)mood_degree_midi(6, z), 0.4f);
        if (learn_enabled()) {
            learn_touch(1);
            learn_right(bw_zonex(z), BW_BAND);
            if (bw.sorted >= 5u) {             /* five sorted: a round */
                if (learn_phase() == LP_WAIT)
                    learn_solved(120.0f, 100.0f);
                sticker_give(ST_SORT);
                bw.sorted = 0;
            }
        }
    } else {                                   /* "ptooey": back up it goes, to fall again */
        bw.b[i].state = 4;
        bw.b[i].t0 = GT.T;
        bw.b[i].vy = -5.0f;
        bw.b[i].x = bw_zonex(z);
        bw.b[i].y = BW_BAND - 10.0f;
        snd_play(P_FLICK_SPROING, (float)mood_degree_midi(4, 0), 0.5f);
        learn_touch(1);
        if (learn_phase() == LP_WAIT)
            learn_wrong(bw_zonex(z), BW_BAND);
    }
}

/* ------------------------------------------------------------- the toy --- */
static void bw_enter_game(void)
{
    int i;
    bw.game = (uint8_t)bw_game();
    for (i = 0; i < NWB; i++)
        bw.b[i].on = 0;
    bw.pend_n = 0;
    bw.mouth_open = -1;
    bw.sorted = 0;
    bw.round_on = 0;
    bw.zones = (uint8_t)(learn_level() >= 2 ? 5 : 3);
    if (bw.game == BG_SORT)
        bw.zones = (uint8_t)(learn_level() >= 2 ? 4 : 3);
    if (bw.game == BG_MATCH)
        bw_new_wall();
    else
        km_zones(bw.zones);
    if (learn_phase() == LP_WAIT || learn_phase() == LP_DEMO)
        learn_end();
}

static void wall_init(void)
{
    bw.cols = 2;
    bw.peek_t0 = bw.sneeze_t0 = bw.pick_t0 = bw.recount_t0 = -10000.0f;
    learn_enter(LT_WALL, 3);
    bw_enter_game();
}
static void wall_leave(void) { learn_leave(); }

static void wall_key(uint32_t k, float vel)
{
    if (bw.game != (uint8_t)bw_game())
        bw_enter_game();
    switch (bw.game) {
    case BG_MATCH:
        if (learn_enabled() && !bw.round_on && learn_phase() == LP_FREE) {
            learn_round(BG_MATCH);              /* (a wall is a round: judged as one, quietly) */
            bw.round_on = 1;
            bw.mism = 0;
        }
        bw_match_key(k, vel);
        break;
    case BG_COUNT:
        bw_count_key(k);
        break;
    default:
        bw_sort_key(k);
        break;
    }
}
static void wall_release(uint32_t k, uint32_t held) { (void)k; (void)held; }

static void wall_knob(int knob, int v)
{
    (void)v;
    if (knob == K6 || (knob == K4 && bw.game == BG_MATCH))
        bw_enter_game();
}

static void wall_mega(uint32_t k)             /* a mash: every door opens at once for a peek */
{
    (void)k;
    bw.peek_t0 = GT.T;
    part_burst(PT_SLIME, 120.0f, 80.0f, 20, 1.0f, 4.0f, 30, CC_SNOT, 3);
    impact(0.4f, 3);
}

static void wall_update(float dt)
{
    int i, ev, lp;
    float g = 0.02f + 0.12f * kf(K2);
    toy_crush = 0.2f * kf(K7);
    if (bw.game != (uint8_t)bw_game())
        bw_enter_game();
    /* match: two different close again */
    if (bw.close_at > 0.0f && GT.T >= bw.close_at) {
        bw_close_open();
        snd_play(P_TICK, (float)mood_degree_midi(4, 0), 0.3f);
    }
    if (bw.game == BG_MATCH && bw.found >= bw.cols && learn_phase() != LP_CHEER && GT.T - bw.newwall_t0 > 600.0f &&
        GT.T - bw.sneeze_t0 > 1400.0f)
        bw_new_wall();                         /* a new wall (bigger, when she is ready) */
    /* count: the boogers land one by one */
    if (bw.pend_i < bw.pend_n && GT.T >= bw.pend_at) {
        bw_splat(bw.pend_i, bw.pend_n);
        bw.pend_i++;
        bw.pend_at = GT.T + 260.0f;
        if (bw.pend_i >= bw.pend_n && learn_phase() == LP_DEMO)
            learn_wait();
    }
    if (bw.recount_t0 > 0.0f && bw.recount_i < bw_on_wall() && GT.T >= bw.recount_at) {
        int j, n = 0;                          /* the recount: each blinks with its note */
        for (j = 0; j < NWB; j++)
            if (bw.b[j].on && bw.b[j].state == 1u && n++ == bw.recount_i) {
                bw.b[j].t0 = GT.T;
                snd_play(toy_patch(), (float)mood_degree_midi(4, bw.recount_i), 0.6f);
            }
        bw.recount_i++;
        bw.recount_at = GT.T + 330.0f;
    }
    /* the boogers: falling, sliding off, spat back */
    for (i = 0; i < NWB; i++) {
        float k = dt / 16.67f;
        if (!bw.b[i].on)
            continue;
        switch (bw.b[i].state) {
        case 1:                                /* on the wall: free play's slide off after a while */
            if (bw.game == BG_COUNT && learn_phase() == LP_FREE && bw.pend_i >= bw.pend_n && GT.T - bw.b[i].born > 3500.0f) {
                bw.b[i].state = 3;
                bw.b[i].t0 = GT.T;
            }
            break;
        case 2:                                /* falling; it waits above the mouths if nobody catches it */
            bw.b[i].y += g * dt * 0.6f;
            if (bw.b[i].y > BW_BAND - 26.0f)
                bw.b[i].y = BW_BAND - 26.0f + 2.0f * fm_sinf(GT.T / 200.0f);
            break;
        case 3:                                /* sliding off */
            bw.b[i].y += 2.0f * k;
            if (GT.T - bw.b[i].t0 > 500.0f)
                bw.b[i].on = 0;
            break;
        case 4:                                /* spat back up, then falling again */
            bw.b[i].vy += 0.25f * k;
            bw.b[i].y += bw.b[i].vy * k;
            if (bw.b[i].vy > 0.0f && bw.b[i].y < 60.0f)
                bw.b[i].state = 2;
            if (bw.b[i].vy > 0.0f && bw.b[i].y >= 60.0f)
                bw.b[i].state = 2;
            break;
        default:                               /* 5: into the mouth */
            if (GT.T - bw.b[i].t0 > 220.0f)
                bw.b[i].on = 0;
            break;
        }
    }
    if (bw.game == BG_SORT && bw_falling() < 0)
        bw_sort_spawn();
    /* the challenges */
    ev = learn_tick();
    lp = learn_phase();
    if (ev == LE_REDEMO) {
        if (bw.game == BG_MATCH)
            bw.peek_t0 = GT.T;                 /* idle: every door peeks open for a moment (a hint) */
        else if (bw.game == BG_COUNT) {
            bw.recount_t0 = GT.T;
            bw.recount_i = 0;
            bw.recount_at = GT.T;
        }
    } else if (ev == LE_ABANDON || ev == LE_CHEERED) {
        if (bw.game == BG_COUNT)
            bw_clear_b();
        bw.round_on = 0;
    }
    if (lp == LP_FREE && bw.game == BG_COUNT && bw.pend_i >= bw.pend_n)
        if (learn_requested() || learn_offer_due()) {   /* Count: some land; which zone has that many? */
            bw_clear_b();
            bw.target = (uint8_t)(1 + ss_rndi(bw.zones));
            bw.pend_n = bw.target;
            bw.pend_i = 0;
            bw.pend_at = GT.T + 700.0f;
            bw.recount_t0 = -10000.0f;
            learn_begin(BG_COUNT);
        }
    if (lp == LP_FREE && bw.game == BG_SORT && learn_enabled() && (learn_requested() || learn_offer_due())) {
        bw.sorted = 0;
        learn_begin(BG_SORT);
        learn_wait();
    }
    if (lp == LP_FREE && bw.game == BG_MATCH && learn_requested())
        bw.peek_t0 = GT.T;                     /* (asked: a peek) */
    g_led_lit = 0;
    g_led_mid = 0;
}

/* ----------------------------------------------------------------- draw --- */
static void wall_draw(float T)
{
    int i, r, c;
    float sz = bw_size(), pk = T - bw.peek_t0, sa = T - bw.sneeze_t0;
    uint16_t wall = mix565(mood->prop1, mood->ground, 110u), door = mix565(CC_BROWN, mood->accent, 70u), hole = darken(wall, 140u);
    pose_t p;
    mood_draw_ground(196);
    box(BW_X0 - 6.0f, BW_Y0 - 14.0f, BW_W + 12.0f, BW_RH * 2.0f + 22.0f, wall);
    box(BW_X0 - 6.0f, BW_Y0 - 14.0f, BW_W + 12.0f, 8.0f, darken(wall, 50u));   /* the shelf on top */
    if (bw.game == BG_MATCH) {
        int n = bw.cols * 2, f = 0;
        float cw = BW_W / (float)bw.cols, nw = ease_out_back(fm_minf(1.0f, (T - bw.newwall_t0) / 400.0f));
        for (i = 0; i < n; i++) {
            float x = bw_cubx(i % bw.cols), y = bw_cuby(i / bw.cols), w = cw * 0.4f * nw, h = BW_RH * 0.38f * nw;
            float oa = T - bw.open_t0[i];
            int open = bw.state[i] == 1u || (pk >= 0.0f && pk < 1000.0f && bw.state[i] == 0u);
            box(x - w - 1.5f, y - h - 1.5f, 2.0f * w + 3.0f, 2.0f * h + 3.0f, U_LINE);
            box(x - w, y - h, 2.0f * w, 2.0f * h, hole);
            if (bw.state[i] == 2u) {           /* found: the place is empty, a twinkle */
                if (((uint32_t)(T / 300.0f) + (uint32_t)i) % 4u == 0u)
                    star4(x, y, 3.0f, U_WHITE);
                continue;
            }
            if (open) {                        /* the booger peeks out, wiggling (wobbling if not a pair) */
                float e = ease_out_elastic(fm_minf(1.0f, oa / 380.0f)), wob = bw.close_at > 0.0f && (i == bw.open_a || i == bw.open_b)
                                                                               ? 0.15f * fm_sinf(T / 30.0f) : 0.0f;
                draw_wall_booger(x, y, fm_minf(w, h) * 0.7f * sz * (bw.state[i] == 1u ? e : 0.9f), bw.kind[i], 1.0f + wob, 1.0f - wob, T);
            } else {                           /* a little door with a knob */
                box(x - w, y - h, 2.0f * w, 2.0f * h, door);
                box(x - w + 3.0f, y - h + 3.0f, 2.0f * w - 6.0f, h - 4.0f, lighten(door, 40u));
                box(x - w + 3.0f, y + 1.0f, 2.0f * w - 6.0f, h - 4.0f, lighten(door, 40u));
                disc(x + w * 0.6f, y, 2.6f, U_LINE);
                disc(x + w * 0.6f, y, 2.0f, CC_GOLD);
            }
        }
        for (i = 0; i < n; i++)                /* the found pairs, stuck to the shelf */
            if (bw.state[i] == 2u) {
                float a = T - bw.match_t0[i], u = ease_out_cubic(fm_minf(1.0f, a / 400.0f));
                float sx = 24.0f + (float)(f / 2) * 18.0f + (float)(f & 1) * 7.0f, x = ss_lerp(bw_cubx(i % bw.cols), sx, u);
                float y = ss_lerp(bw_cuby(i / bw.cols), BW_Y0 - 10.0f, u);
                draw_wall_booger(x, y, 4.5f * (learn_dancing() ? 1.0f + 0.2f * fm_sinf(T / 80.0f) : 1.0f), bw.kind[i], 1.0f, 1.0f, T);
                f++;
            }
    } else {
        /* count and sort: boogers splatted on the wall, falling, sliding off */
        for (i = 0; i < NWB; i++) {
            float a = T - bw.b[i].t0, s = sz * 7.0f, sx = 1.0f, sy = 1.0f, x = bw.b[i].x, y = bw.b[i].y;
            if (!bw.b[i].on)
                continue;
            if (bw.b[i].state == 1u && a < 300.0f) {   /* the splat */
                float e = ease_out_elastic(a / 300.0f);
                sx = 1.5f - 0.5f * e;
                sy = 0.5f + 0.5f * e;
            }
            if (bw.b[i].state == 5u) {         /* into the mouth */
                float u = ss_clamp01(a / 220.0f);
                x = ss_lerp(x, bw_zonex(bw.b[i].col + 1), u);
                y = ss_lerp(y, BW_BAND, u);
                s *= 1.0f - 0.6f * u;
            }
            if (bw.recount_t0 > 0.0f && bw.b[i].state == 1u && a < 250.0f)
                glow(x, y, 16.0f, scale565(CC_GOLD, 180u), 220u);
            draw_wall_booger(x, y, s, bw.game == BG_SORT ? bw.b[i].col + 6 * bw.b[i].shape : bw.b[i].col, sx, sy, T);
        }
        /* the band: dot-group answers (Count) or the hungry mouths (Sort), each over its zone of keys */
        for (i = 1; i <= bw.zones; i++) {
            float x = bw_zonex(i), y = BW_BAND + 6.0f, open = bw.mouth_open == i ? ss_pulse(T - bw.mouth_t0, 0.0f, 300.0f) : 0.0f;
            float zw = 240.0f / (float)bw.zones;
            box_a(x - zw * 0.5f + 2.0f, BW_BAND - 12.0f, zw - 4.0f, 34.0f, CC_INK, 70u);
            if (bw.game == BG_COUNT) {
                int hl = learn_phase() == LP_WAIT && learn_level() == 0 && i == bw.target;   /* (level 0: the answer glows) */
                draw_dots(x, y, i, 3.0f, i, hl ? CC_GOLD : U_WHITE, U_WHITE);
            } else {
                uint16_t mc = BW_COLOR[(i - 1) % 6];
                ell(x, y, 15.0f + 1.5f, 6.0f + 9.0f * open + 1.5f, U_LINE);
                ell(x, y, 15.0f, 6.0f + 9.0f * open, mc);
                ell(x, y + 1.0f, 10.0f, 2.0f + 7.0f * open, CC_INK);
                ell(x, y + 3.0f + 4.0f * open, 6.0f, 1.5f + 2.0f * open, CC_PINK);
                disc(x - 6.0f, y - 10.0f, 2.5f, U_WHITE);
                disc(x + 6.0f, y - 10.0f, 2.5f, U_WHITE);
                disc(x - 6.0f, y - 10.0f, 1.2f, CC_INK);
                disc(x + 6.0f, y - 10.0f, 1.2f, CC_INK);
            }
        }
        if (bw.game == BG_COUNT && learn_phase() == LP_WAIT && learn_level() == 0) {
            box_a(96.0f, 2.0f, 48.0f, 18.0f, CC_INK, 150u);   /* (level 0: how many, as dots, on top) */
            draw_dots(120.0f, 11.0f, bw.target, 2.5f, bw.target, CC_GOLD, CC_GOLD);
        }
    }
    /* the unicorn, picking her nose (a press), sneezing for joy */
    p = uni_pose(BW_UX, BW_UY, 0.36f);
    p.facing = 1;
    p.mouth_state = MO_GRIN;
    if (T - bw.pick_t0 < 300.0f)
        p.pick = ss_pulse(T - bw.pick_t0, 0.0f, 300.0f);
    if (sa >= 0.0f && sa < 600.0f) {
        p.eye_state = sa < 200.0f ? EY_SQUINT : EY_HAPPY;
        p.mouth_state = sa < 200.0f ? MO_STRAIN : MO_O;
        p.head_rot = sa < 200.0f ? 8.0f : -6.0f;
        if (sa > 200.0f && sa < 230.0f)
            part_fan(PT_CONFETTI, BW_UX + 18.0f, BW_UY - 40.0f, 10, -0.6f, 0.8f, 2.0f, 5.0f, 30, 0, 3);
    }
    if (learn_dancing())
        p.eye_state = EY_HAPPY, p.lift = 6.0f * ss_absf(fm_sinf(T / 120.0f));
    uni_draw(&p);
    (void)r;
    (void)c;
}

static void wall_clean(void)
{
    if (bw.game == BG_MATCH)
        bw_new_wall();
    else
        bw_clear_b();
}

static const toy_def_t TOY_WALL_DEF = {
    "BOOGER WALL", B_SEQ, TF_HOLD | TF_LEARN, wall_init, wall_leave, wall_key, wall_release, wall_mega, wall_knob, wall_update,
    wall_draw, wall_clean, WALL_PRESETS, 4,
    {{KI_SPEED, 21, 0, 6}, {KI_SIZE, 21, 0, 10}, {KI_GRID, 4, 0, 3}, {KI_GAME, 3, 1, 0}, {KI_CHAOS, 21, 0, 4}},
    0, TOY_BOOGER,
};
