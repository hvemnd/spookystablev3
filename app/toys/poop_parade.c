/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: Poop Parade (School mode + EDIT, design 4): the unicorn's poops are a step sequencer.
 * Three lanes across the screen, hat on top, snare, kick at the bottom; eight columns left to right (16 with K4,
 * two pages). The keybed's thirds are the lanes (keymap.c): the low third the kick, its keys left to right the
 * columns. A step key: she runs to that column, squats, and a poop pops up into its lane with a plop (pitched by the
 * lane); the same key again flushes it away. Play (key 24, or PLAY): she marches left to right, and at each column
 * every poop in it squashes flat under her stomp and plays its drum, all at once (hitstop, shake, the column flashes,
 * the keys light). A poop placed while she marches sneaks in and is stomped when she gets there.
 *   key 24 Play/Stop   key 25 Clear (tap: the other page with 16 columns, else a nervous wobble; hold: flush all)
 *   key 26 Challenge (asks for one; during one, shows it again)
 *   K2 tempo 60-160   K3 swing 0-60 %   K4 8 or 16 columns   K6 drum kit (acoustic / 808 / pots and pans / cartoon)
 *   K7 chaos (particles, squash, the LED chase)   OCT-/OCT+ the plop's voice
 * The challenges (invitations; after Play has been pressed twice, or on the Challenge key):
 *   Copy Me      she poops a pattern with its sounds, it clears, you poop it back (3 -> 4 -> 6 poops)
 *   Count Them   she poops some, then every press touches the next one, left to right, with a rising note and a dot
 *   Left-Right   one poop on column 1, then 2, then 3...; it clears; you go the same way (3 -> 5)
 * Your own pattern is put aside for a challenge and comes back after it: nothing is ever lost. */

#define PP_COLX(c) (26.0f + 27.0f * (float)(c))
#define PP_LANEY(l) (126.0f - 40.0f * (float)(l))   /* kick at the bottom, hat at the top */
#define PP_FLOOR 194.0f
#define PP_SCALE 0.36f
#define NPFLY 6
enum { PK_COPY, PK_COUNT, PK_LR, NPK };

static struct {
    float ux, ux0, utx, run_t0, squat_t0, stomp_t0, hold_t0;
    float squash[NLANES][STEPS_MAX], born[NLANES][STEPS_MAX], gone[NLANES][STEPS_MAX];
    uint16_t big[NLANES];                     /* the held, longer, funnier poops */
    struct { float x0, y0, t0; uint8_t lane, col, on; } fly[NPFLY];
    uint8_t page, plays, held_key, kind_next;
    uint32_t held;
    float clear_t0, wobble_t0, flushall_t0, colflash_t0;
    int8_t colflash;
    /* the challenge */
    uint16_t stash[NLANES];
    uint8_t stashed;
    uint16_t target[NLANES];
    uint8_t order[8], norder, oi, counted;
    uint8_t demo_i, demo_n, demo_on;
    float demo_at;
    float hint_t0;
    uint8_t march, march_cols;
    uint8_t ghosts;
} pp;

static uint32_t pp_stomps;                     /* (the tests: columns stomped with a poop in them) */

static const preset_t PARADE_PRESETS[] = {
    {P_PLOP_STEP, IC_BELLS}, {P_PLOP_WOOD, IC_BLOCK}, {P_PLOP_DROP, IC_DROP}, {P_PLOP_PIZZ, IC_STRING}, {P_PLOP_BOING, IC_SPRING}};

static int pp_cols(void) { return kv(K4) ? 16 : 8; }
static int pp_on(int l, int c) { return (steps_lane[l] >> c) & 1; }
static void pp_save(void)
{
    int l;
    for (l = 0; l < NLANES; l++)
        set.parade[l] = pp.stashed ? pp.stash[l] : steps_lane[l];
    knobs_save_soon();
}
static void pp_config(void)
{
    steps_config(60.0f + 5.0f * (float)kv(K2), 0.6f * kf(K3), kv(K6), pp_cols());
    steps_set_pitch(LANE_KICK, (float)mood_degree_midi(3, 0));
    steps_set_pitch(LANE_SNARE, (float)mood_degree_midi(4, 2));
    steps_set_pitch(LANE_HAT, (float)mood_degree_midi(5, 4));
}
static float pp_plop_midi(int lane, int col) { return (float)mood_degree_midi(4 + (lane > 0), lane * 2 + (col % 3 == 2)); }

static void pp_restore(void)
{
    int l, c;
    if (!pp.stashed)
        return;
    for (l = 0; l < NLANES; l++) {
        steps_lane[l] = pp.stash[l];
        for (c = 0; c < STEPS_MAX; c++)
            if (pp_on(l, c))
                pp.born[l][c] = GT.T + (float)c * 25.0f;   /* her parade comes back, column by column */
    }
    pp.stashed = 0;
    pp.ghosts = 0;
    pp_save();
}

static void parade_init(void)
{
    int l, c;
    km_parade();
    for (l = 0; l < NLANES; l++) {
        steps_lane[l] = set.parade[l];
        for (c = 0; c < STEPS_MAX; c++) {
            pp.squash[l][c] = pp.gone[l][c] = -10000.0f;
            pp.born[l][c] = -10000.0f;
        }
    }
    for (c = 0; c < NPFLY; c++)
        pp.fly[c].on = 0;
    pp.ux = pp.ux0 = pp.utx = PP_COLX(0);
    pp.run_t0 = pp.squat_t0 = pp.stomp_t0 = pp.clear_t0 = pp.wobble_t0 = pp.flushall_t0 = pp.hint_t0 = -10000.0f;
    pp.colflash = -1;
    pp.page = 0;
    pp.held = 0;
    pp.stashed = 0;
    pp.demo_on = 0;
    pp.march = 0;
    pp.ghosts = 0;
    pp_config();
    learn_enter(LT_PARADE, 3);
}
static void parade_leave(void)
{
    steps_play(0);
    pp_restore();
    learn_leave();
}

/* ------------------------------------------------------------ the poops --- */
static void pp_launch(int lane, int col)
{
    int i;
    for (i = 0; i < NPFLY; i++)
        if (!pp.fly[i].on)
            break;
    if (i == NPFLY)
        i = 0;
    pp.fly[i].on = 1;
    pp.fly[i].lane = (uint8_t)lane;
    pp.fly[i].col = (uint8_t)col;
    pp.fly[i].t0 = GT.T;
    pp.fly[i].x0 = pp.ux - 18.0f;             /* from her bottom */
    pp.fly[i].y0 = PP_FLOOR - 20.0f;
}

static void pp_place(int lane, int col, float vel, int from_her)
{
    note_t n;
    steps_lane[lane] |= (uint16_t)(1u << col);
    pp.big[lane] &= (uint16_t)~(1u << col);
    pp.born[lane][col] = GT.T;
    pp.gone[lane][col] = -10000.0f;
    n = snd_defaults(toy_patch(), pp_plop_midi(lane, col), vel_f(vel));
    snd_note(&n);
    if (from_her && !steps_playing()) {        /* she runs there and squats; the poop pops up into its lane */
        pp.ux0 = pp.ux;
        pp.utx = PP_COLX(col % 8);
        pp.run_t0 = GT.T;
        pp.squat_t0 = GT.T;
        pp_launch(lane, col);
        impact(0.08f, 2);
    } else {                                   /* while she marches: it sneaks in */
        part_burst(PT_SPARKLE, PP_COLX(col % 8), PP_LANEY(lane), 5, 0.6f, 1.8f, 12, U_WHITE, 2);
        fx_trauma(0.06f);
    }
    if (!pp.stashed)
        pp_save();
    sticker_give(ST_FIRST_POOP);
}
static void pp_flush(int lane, int col)
{
    steps_lane[lane] &= (uint16_t)~(1u << col);
    pp.gone[lane][col] = GT.T;
    snd_play(P_BLOOP, pp_plop_midi(lane, col) - 5.0f, 0.6f);
    if (!pp.stashed)
        pp_save();
}
static void pp_clear_all(int stash_too)
{
    int l, c;
    for (l = 0; l < NLANES; l++) {
        for (c = 0; c < STEPS_MAX; c++)
            if (pp_on(l, c))
                pp.gone[l][c] = GT.T + (float)(ss_rndi(120));
        steps_lane[l] = 0;
    }
    if (stash_too && !pp.stashed)
        pp_save();
}

/* ------------------------------------------------------------ challenges --- */
static void pp_order_targets(void)            /* the target's cells, left to right (the demo's order) */
{
    int c, l;
    pp.norder = 0;
    for (c = 0; c < STEPS_MAX && pp.norder < 8u; c++)
        for (l = 0; l < NLANES && pp.norder < 8u; l++)
            if (pp.target[l] >> c & 1u)
                pp.order[pp.norder++] = (uint8_t)(l * 16 + c);
}

static void pp_challenge(int kind)
{
    int lvl = learn_level(), i, l, c, n;
    if (steps_playing())
        steps_play(0);
    pp.march = 0;
    if (!pp.stashed) {                         /* her parade is put aside, and comes back afterwards */
        for (l = 0; l < NLANES; l++)
            pp.stash[l] = steps_lane[l];
        pp.stashed = 1;
    }
    pp_clear_all(0);
    for (l = 0; l < NLANES; l++)
        pp.target[l] = 0;
    pp.page = 0;
    switch (kind) {
    case PK_COPY:                              /* 3, 3, 4, 6 poops; the first two levels in one lane, with ghosts */
        n = lvl < 2 ? 3 : lvl == 2 ? 4 : 6;
        for (i = 0; i < n;) {
            l = lvl == 0 ? LANE_KICK : ss_rndi(NLANES);
            c = ss_rndi(8);
            if (!(pp.target[l] >> c & 1u)) {
                pp.target[l] |= (uint16_t)(1u << c);
                i++;
            }
        }
        pp.ghosts = lvl < 2;
        break;
    case PK_COUNT:                             /* 1-3, then 1-5 poops to count */
        n = 1 + ss_rndi(lvl < 2 ? 3 : 5);
        for (i = 0; i < n;) {
            l = ss_rndi(NLANES);
            c = ss_rndi(8);
            if (!(pp.target[l] >> c & 1u)) {
                pp.target[l] |= (uint16_t)(1u << c);
                i++;
            }
        }
        pp.ghosts = 0;
        break;
    default: {                                 /* left to right: 3, 3, 4, 5 columns in a row */
        int len = lvl < 2 ? 3 : lvl == 2 ? 4 : 5, c0 = lvl == 0 ? 0 : ss_rndi(9 - len);
        for (i = 0; i < len; i++)
            pp.target[LANE_KICK] |= (uint16_t)(1u << (c0 + i));
        pp.ghosts = 0;
        break;
    }
    }
    pp_order_targets();
    pp.oi = 0;
    pp.counted = 0;
    pp.demo_on = 1;
    pp.demo_i = 0;
    pp.demo_n = pp.norder;
    pp.demo_at = GT.T + 700.0f;                /* after the sting */
    learn_begin(kind);
}

static void pp_demo_tick(void)                /* the unicorn shows it: one poop each half second */
{
    if (!pp.demo_on || GT.T < pp.demo_at)
        return;
    if (pp.demo_i < pp.demo_n) {
        int l = pp.order[pp.demo_i] / 16, c = pp.order[pp.demo_i] % 16;
        pp_place(l, c, 0.8f, 1);
        pp.demo_i++;
        pp.demo_at = GT.T + 520.0f;
        return;
    }
    pp.demo_on = 0;
    if (learn_kind() != PK_COUNT)              /* it clears (Count leaves them there to be counted) */
        pp_clear_all(0);
    learn_wait();
}

static void pp_solved(void)
{
    learn_solved(120.0f, 90.0f);
    sticker_give(learn_kind() == PK_COPY ? ST_COPY : learn_kind() == PK_COUNT ? ST_COUNT : ST_LEFTRIGHT);
    if (learn_kind() == PK_COUNT) {            /* the counted ones take a bow */
        int l;
        for (l = 0; l < NLANES; l++)
            steps_lane[l] = pp.target[l];
    }
}

/* her press during a challenge's turn: 1 if it was the challenge's (judged here) */
static int pp_judge(int lane, int col)
{
    int k = learn_kind();
    if (learn_phase() != LP_WAIT)
        return 0;
    if (k == PK_COUNT)
        return 0;                              /* (counted with any key, in parade_key) */
    if (k == PK_COPY) {
        if (pp_on(lane, col)) {                /* taking a right one back: fine, no judging */
            pp_flush(lane, col);
            return 1;
        }
        pp_place(lane, col, 0.8f, 1);
        if (pp.target[lane] >> col & 1u) {
            int l, done = 1;
            learn_right(PP_COLX(col), PP_LANEY(lane));
            for (l = 0; l < NLANES; l++)
                if ((steps_lane[l] & pp.target[l]) != pp.target[l])
                    done = 0;
            if (done)
                pp_solved();
        } else {                               /* not that one: it pops back down, the right ones show */
            pp.gone[lane][col] = GT.T + 150.0f;
            steps_lane[lane] &= (uint16_t)~(1u << col);
            learn_wrong(PP_COLX(col), PP_LANEY(lane));
            if (learn_misses() >= 2)
                pp.hint_t0 = GT.T;
        }
        return 1;
    }
    /* left to right: the next column in the row */
    {
        int want = pp.order[pp.oi] % 16;
        if (col == want) {
            pp_place(LANE_KICK, col, 0.8f, 1);
            learn_right(PP_COLX(col), PP_LANEY(LANE_KICK));
            if (++pp.oi >= pp.norder)
                pp_solved();
        } else {
            learn_wrong(PP_COLX(col), PP_LANEY(lane));
            pp.colflash = (int8_t)want;        /* the right column lights again */
            pp.colflash_t0 = GT.T;
            if (learn_misses() >= 2)
                pp.hint_t0 = GT.T;
        }
        return 1;
    }
}

static void pp_count_touch(void)              /* Count Them: each press touches the next poop, left to right */
{
    int l = pp.order[pp.counted] / 16, c = pp.order[pp.counted] % 16;
    pp.squash[l][c] = GT.T;
    snd_play(P_SUCCESS_ARP, (float)mood_degree_midi(5, pp.counted), 0.6f);
    part_burst(PT_STAR, PP_COLX(c), PP_LANEY(l), 6, 1.0f, 2.5f, 14, CC_GOLD_L, 2);
    pp.counted++;
    if (pp.counted >= pp.norder)
        pp_solved();
}

/* ----------------------------------------------------------------- keys --- */
static void pp_play_toggle(void)
{
    if (pp.demo_on)
        return;
    if (steps_playing()) {
        steps_play(0);
        snd_play(P_POP, (float)mood_degree_midi(5, 0), 0.5f);
    } else {
        pp_config();
        steps_play(1);
        pp.plays++;
        sticker_give(ST_PARADE);
    }
    pp.march = 0;
    learn_free();
}

static void parade_key(uint32_t k, float vel)
{
    const keymap_t *m = &km[k];
    int lp = learn_phase();
    pp.held |= 1u << k;
    if (pp.march && m->role == KR_STEP) {      /* she wants her parade back now */
        steps_play(0);
        pp.march = 0;
        pp_restore();
    }
    if (lp == LP_WAIT && learn_kind() == PK_COUNT && !synth_key && pp.counted < pp.norder) {
        learn_touch(1);
        pp_count_touch();
        ui_led_key(k);
        return;
    }
    switch (m->role) {
    case KR_STEP: {
        int col = m->vcol + (pp_cols() == 16 ? 8 * pp.page : 0), lane = m->vrow;
        pp.held_key = (uint8_t)k;
        pp.hold_t0 = GT.T;
        if (lp == LP_DEMO) {                   /* (watching: a little plop of her own, nothing placed) */
            snd_play(P_POP, pp_plop_midi(lane, col), 0.4f);
            part_burst(PT_SPARKLE, PP_COLX(col % 8), PP_LANEY(lane), 3, 0.5f, 1.5f, 10, U_WHITE, 2);
            learn_touch(1);
            return;
        }
        if (lp == LP_WAIT && !synth_key) {
            learn_touch(1);
            if (pp_judge(lane, col))
                return;
        }
        if (pp_on(lane, col))
            pp_flush(lane, col);
        else
            pp_place(lane, col, vel, 1);
        learn_free();
        break;
    }
    case KR_PLAY:
        if (lp == LP_WAIT)
            learn_touch(0);
        pp_play_toggle();
        break;
    case KR_CLEAR:
        pp.clear_t0 = GT.T;
        if (lp == LP_WAIT)
            learn_touch(0);
        break;
    default:                                   /* the Challenge key */
        if (lp == LP_WAIT) {                   /* show it again */
            learn_touch(1);
            pp.hint_t0 = GT.T;
            snd_play(P_BELL_STING, (float)mood_degree_midi(6, 0), 0.4f);
        } else if (learn_enabled() && lp == LP_FREE) {
            learn_request();
        } else {                               /* no challenges today: she twirls, and a fill plays */
            int i;
            pp.stomp_t0 = GT.T;
            for (i = 0; i < 4; i++)
                sched_note((uint32_t)i * 90u, P_LTOM + (i % 3), 0.0f, 0.8f);
            part_burst(PT_STAR, pp.ux, PP_FLOOR - 40.0f, 10, 1.0f, 3.0f, 20, 0, 3);
        }
        break;
    }
}

static void parade_release(uint32_t k, uint32_t held)
{
    const keymap_t *m = &km[k];
    pp.held &= ~(1u << k);
    if (m->role == KR_STEP && held > 450u && learn_phase() != LP_DEMO) {   /* a long strain: a bigger, funnier poop */
        int col = m->vcol + (pp_cols() == 16 ? 8 * pp.page : 0), lane = m->vrow;
        if (pp_on(lane, col)) {
            pp.big[lane] |= (uint16_t)(1u << col);
            pp.born[lane][col] = GT.T;
            snd_play(P_FART_SLIDE, (float)mood_degree_midi(3, 0), 0.6f);
            part_burst(PT_STAR, PP_COLX(col % 8), PP_LANEY(lane), 8, 1.0f, 3.0f, 16, CC_GOLD, 3);
            fx_trauma(0.15f);
        }
    }
    if (m->role == KR_CLEAR && held < 500u && pp.clear_t0 > -1.0f) {   /* a tap: the other page, or a nervous wobble */
        if (pp_cols() == 16) {
            pp.page ^= 1u;
            snd_play(P_SWOOSH, 0.0f, 0.4f);
        } else {
            pp.wobble_t0 = GT.T;
            snd_play(P_GURGLE_BUBBLY, 0.0f, 0.4f);
        }
        pp.clear_t0 = -10000.0f;
    }
}

static int parade_button(uint32_t b, int kind)
{
    if (b == B_PLAY && kind == BK_PRESS) {     /* PLAY is the parade's Play key here */
        pp_play_toggle();
        return 1;
    }
    return 0;
}

static void parade_mega(uint32_t k)           /* a mash: every lane, every column, one huge stomp */
{
    int c;
    (void)k;
    for (c = 0; c < 8; c++)
        pp.squash[c % 3][c] = GT.T;
    chord(P_STAB, 4, 0, 0.7f);
    impact(0.6f, 4);
    part_burst(PT_POOPLET, 120.0f, 100.0f, 16, 2.0f, 5.0f, 40, 0, 4);
}

static void parade_knob(int knob, int v)
{
    (void)v;
    pp_config();
    if (knob == K4)
        pp.page = 0;
}

/* --------------------------------------------------------------- update --- */
static void parade_update(float dt)
{
    uint8_t col, mask;
    uint32_t lit = 0, mid = 0, k;
    int l, ev;
    float ch = kf(K7);
    (void)dt;
    pp_config();
    toy_crush = 0.0f;
    /* the stomps the render played */
    while (steps_pop(&col, &mask)) {
        pp.stomp_t0 = GT.T;
        if (pp_cols() == 16)
            pp.page = (uint8_t)(col / 8u);
        if (mask) {
            pp_stomps++;
            for (l = 0; l < NLANES; l++)
                if (mask >> l & 1u)
                    pp.squash[l][col] = GT.T;
            fx_hitstop(3);
            fx_trauma(0.18f + 0.25f * ch);
            pp.colflash = (int8_t)col;
            pp.colflash_t0 = GT.T;
            if (mask & 1u)
                ui_led_kick();
            if (mask & 2u)
                ui_led_snare();
            if (mask & 4u)
                ui_led_hat();
            part_fan(PT_DUST, pp.ux, PP_FLOOR + 2.0f, 3 + (int)(4.0f * ch), -1.57f, 1.3f, 0.5f, 1.8f, 14, 0, 3);
            if (ch > 0.6f)
                part_mood_burst(PP_COLX(col % 8), 90.0f, 4, 1.0f, 3.0f);
        } else {
            part_fan(PT_DUST, pp.ux, PP_FLOOR + 2.0f, 1, -1.57f, 1.0f, 0.3f, 0.8f, 10, 0, 2);
        }
        if (pp.march && col == pp.march_cols - 1u) {   /* the victory march: one loop, then her own parade */
            steps_play(0);
            pp.march = 0;
            pp_restore();
        }
    }
    /* the clear key held: everything flushes away */
    if (pp.clear_t0 > -1.0f && (pp.held >> 25 & 1u) && GT.T - pp.clear_t0 > 500.0f) {
        pp.clear_t0 = -10000.0f;
        pp.flushall_t0 = GT.T;
        pp_clear_all(1);
        if (learn_phase() == LP_WAIT && learn_kind() != PK_COUNT)
            pp.oi = 0;
        snd_play(P_GURGLE_BIG, 0.0f, 0.7f);
        fx_trauma(0.2f);
    }
    /* the challenge's clock */
    pp_demo_tick();
    ev = learn_tick();
    if (ev == LE_REDEMO) {                      /* idle: she shows it again */
        pp.hint_t0 = GT.T;
        if (learn_kind() == PK_COUNT)
            snd_play(P_BELL_STING, (float)mood_degree_midi(6, 0), 0.4f);
        else {
            int i;
            for (i = 0; i < pp.norder; i++)
                sched_note(200u + (uint32_t)i * 400u, toy_patch(), pp_plop_midi(pp.order[i] / 16, pp.order[i] % 16), 0.6f);
        }
    } else if (ev == LE_ABANDON) {
        pp.demo_on = 0;
        pp_clear_all(0);
        pp_restore();
    } else if (ev == LE_CHEERED) {              /* the victory march, then her own parade comes back */
        pp_config();
        pp.march = 1;
        pp.march_cols = (uint8_t)8;
        if (!steps_playing())
            steps_play(1);
    }
    if (learn_phase() == LP_FREE && !pp.march && !steps_playing() && !pp.demo_on) {
        if (learn_requested() || (pp.plays >= 2u && learn_offer_due())) {
            pp_challenge(pp.kind_next);
            pp.kind_next = (uint8_t)((pp.kind_next + 1u) % NPK);
        }
    }
    /* the lights: the steps on this page at the middle level, the stomped column lit, Play lit while she marches */
    for (k = 0; k < 24u; k++) {
        int c = km[k].vcol + (pp_cols() == 16 ? 8 * pp.page : 0);
        if (pp_on(km[k].vrow, c))
            mid |= 1u << k;
        if (pp.colflash == c && GT.T - pp.colflash_t0 < 110.0f)
            lit |= 1u << k;
    }
    if (steps_playing())
        lit |= 1u << 24;
    else
        mid |= 1u << 24;
    mid |= 1u << 25;
    if (learn_enabled())
        if (learn_phase() == LP_WAIT || ((plat_ms() / 400u) & 1u))
            mid |= 1u << 26;
    if (ch > 0.7f && steps_playing())          /* chaos: a chase across the keybed */
        lit |= 1u << ((plat_ms() / 60u) % 24u);
    g_led_lit = lit;
    g_led_mid = mid;
}

/* ----------------------------------------------------------------- draw --- */
static void draw_mini_poop(float x, float y, float s, float sx, float sy, int32_t hue)
{
    /* four shapes and two raw-row eyes: each tier with its outline (a full grid of 24 stays inside the sequencer's
     * draw budget, design 10.4) */
    uint16_t c = hue >= 0 ? hue565(hue) : CC_BROWN, c2 = hue >= 0 ? hue565(hue + 300) : lighten(CC_BROWN, 30u);
    ell(x, y, 11.0f * s * sx + 1.2f, 6.5f * s * sy + 1.2f, CC_BROWN_D);
    ell(x, y, 11.0f * s * sx, 6.5f * s * sy, c);
    ell(x + 0.5f * s * sx, y - 8.0f * s * sy, 7.2f * s * sx + 1.2f, 6.2f * s * sy + 1.2f, CC_BROWN_D);
    ell(x + 0.5f * s * sx, y - 8.0f * s * sy, 7.2f * s * sx, 6.2f * s * sy, c2);
    if (s * sy > 0.45f) {
        dot3(x - 3.0f * s * sx, y - 8.0f * s * sy, U_WHITE);
        dot3(x + 3.5f * s * sx, y - 8.0f * s * sy, U_WHITE);
    }
}

static void parade_draw(float T)
{
    int l, c, i, ncol = pp_cols(), page = ncol == 16 ? pp.page : 0, lp = learn_phase();
    float pos = steps_playing() ? steps_pos() - 8.0f * (float)page : -1.0f, a, ch = kf(K7);
    pose_t p;
    uint16_t lane_c[NLANES];
    lane_c[LANE_KICK] = mix565(CC_BROWN, mood->ground, 150u);
    lane_c[LANE_SNARE] = mix565(mood->accent, mood->ground, 150u);
    lane_c[LANE_HAT] = mix565(CC_GOLD_L, mood->ground, 150u);
    DR_MARK();
    mood_draw_ground((int32_t)PP_FLOOR + 2);
    /* the lanes and the columns */
    for (l = 0; l < NLANES; l++)
        box_a(10.0f, PP_LANEY(l) - 17.0f, 220.0f, 34.0f, lane_c[l], 110u);
    if (pos >= 0.0f) {                         /* the column she is in glows */
        int pc = (int)pos;
        if (pc >= 0 && pc < 8)
            box_a(PP_COLX(pc) - 13.0f, 26.0f, 26.0f, 120.0f, U_WHITE, 50u);
    }
    if (pp.colflash >= 0 && T - pp.colflash_t0 < 220.0f && pp.colflash / 8 == page)   /* the stomped column flashes */
        box_a(PP_COLX(pp.colflash % 8) - 13.0f, 26.0f, 26.0f, 120.0f, mood->accent, (uint32_t)(200.0f * (1.0f - (T - pp.colflash_t0) / 220.0f)));
    /* ghosts: the pattern to copy, faint (the first levels), and the hint when she needs it */
    a = T - pp.hint_t0;
    if (pp.stashed && (pp.ghosts || (a >= 0.0f && a < 1600.0f)) && lp == LP_WAIT && learn_kind() != PK_COUNT)
        for (l = 0; l < NLANES; l++)
            for (c = 0; c < 8; c++)
                if ((pp.target[l] >> c & 1u) && !pp_on(l, c)) {
                    float bl = a >= 0.0f && a < 1600.0f ? 0.5f + 0.5f * fm_sinf(a / 90.0f) : 0.35f;
                    ring_a(PP_COLX(c), PP_LANEY(l) - 4.0f, 10.0f, 8.0f, 1.5f, U_WHITE, (uint32_t)(220.0f * bl));
                }
    /* the poops */
    for (l = 0; l < NLANES; l++)
        for (c = page * 8; c < page * 8 + 8; c++) {
            float x = PP_COLX(c % 8), y = PP_LANEY(l) + 8.0f, sx = 1.0f, sy = 1.0f, s = l == LANE_KICK ? 1.1f : 0.95f;
            float sq = T - pp.squash[l][c], bn = T - pp.born[l][c], gn = T - pp.gone[l][c];
            int32_t hue = -1;
            if (pp_on(l, c)) {
                if (bn >= 0.0f && bn < 400.0f) {          /* popping in (a sneak while she marches: a wiggle) */
                    float e = ease_out_elastic(bn / 400.0f);
                    s *= ss_clamp01(e);
                    sx = 1.0f + 0.2f * fm_sinf(bn / 30.0f) * (1.0f - bn / 400.0f);
                } else if (bn < 0.0f) {
                    continue;                              /* (coming back, column by column) */
                }
                if (sq >= 0.0f && sq < 120.0f) {          /* stomped flat ... */
                    sy = 0.35f + 0.1f * ch;
                    sx = 1.5f + 0.3f * ch;
                } else if (sq >= 120.0f && sq < 420.0f) { /* ... and springing back (120 ms later) */
                    float e = ease_out_elastic((sq - 120.0f) / 300.0f);
                    sy = 0.45f + 0.55f * e;
                    sx = 1.4f - 0.4f * e;
                }
                if (pp.big[l] >> c & 1u)
                    s *= 1.3f;
                if (learn_wobble() > 0.0f && lp == LP_WAIT)
                    sx *= 1.0f + 0.08f * learn_wobble() * fm_sinf(T / 25.0f);
                if (T - pp.wobble_t0 < 600.0f)            /* (the clear key tapped: they shiver) */
                    x += 2.0f * fm_sinf(T / 20.0f + (float)c) * (1.0f - (T - pp.wobble_t0) / 600.0f);
                if (lp == LP_WAIT && learn_kind() == PK_COUNT) {   /* counted ones turn gold */
                    int j;
                    for (j = 0; j < pp.counted; j++)
                        if (pp.order[j] == l * 16 + c)
                            hue = 200;
                }
                if (pp.march || learn_dancing())
                    hue = (int32_t)(T / 3.0f) + c * 150;
                draw_mini_poop(x, y, s, sx, sy, hue);
                if (hue == 200)
                    star4(x + 9.0f, y - 14.0f, 3.0f, CC_GOLD_L);
            } else if (gn >= 0.0f && gn < 350.0f) {      /* flushed: a little whirlpool spins it away */
                float u = gn / 350.0f;
                ring_a(x, y + 2.0f, 9.0f * (1.0f - u * 0.5f), 3.0f, 1.2f, lighten(HEX(0x4FB8F5), 80u), 200u);
                draw_mini_poop(x + 5.0f * fm_sinf(u * 18.0f) * (1.0f - u), y + 4.0f * u, s * (1.0f - u), 1.0f, 1.0f, -1);
            }
        }
    /* the poops in flight, from her bottom up into their lanes */
    for (i = 0; i < NPFLY; i++) {
        float u, x, y;
        if (!pp.fly[i].on)
            continue;
        u = (T - pp.fly[i].t0) / 260.0f;
        if (u >= 1.0f) {
            pp.fly[i].on = 0;
            part_burst(PT_STAR, PP_COLX(pp.fly[i].col % 8), PP_LANEY(pp.fly[i].lane) + 6.0f, 5, 1.0f, 2.5f, 12, CC_GOLD, 2);
            continue;
        }
        x = ss_lerp(pp.fly[i].x0, PP_COLX(pp.fly[i].col % 8), ease_out_cubic(u));
        y = ss_lerp(pp.fly[i].y0, PP_LANEY(pp.fly[i].lane) + 8.0f, ease_out_back(u)) - 20.0f * fm_sinf(u * 3.14159f);
        draw_mini_poop(x, y, 0.7f, 1.0f, 1.0f, -1);
    }
    /* the flush-everything whirlpool */
    a = T - pp.flushall_t0;
    if (a >= 0.0f && a < 700.0f)
        for (i = 0; i < 3; i++)
            arc_dots(120.0f, 90.0f, (10.0f + 14.0f * (float)i) * (1.0f - a / 700.0f) + 4.0f, a / 80.0f + (float)i * 2.0f,
                     a / 80.0f + (float)i * 2.0f + 3.5f, 2.5f, lighten(HEX(0x4FB8F5), (uint32_t)(40 * i)));
    /* the top bar: a dot a column (the playhead's lit), two page dots with 16 columns, Count's dot group */
    dr_ox = dr_oy = 0;
    for (c = 0; c < 8; c++) {
        int on = pos >= 0.0f && (int)pos == c;
        if (on)
            disc(PP_COLX(c), 11.0f, 4.5f, mood->accent);
        else
            dot3(PP_COLX(c), 11.0f, HEX(0x5A5070));
    }
    if (ncol == 16) {
        disc(4.0f, 8.0f, 2.0f, page == 0 ? U_WHITE : HEX(0x5A5070));
        disc(4.0f, 15.0f, 2.0f, page == 1 ? U_WHITE : HEX(0x5A5070));
    }
    if (lp == LP_WAIT && learn_kind() == PK_COUNT) {
        box_a(70.0f, 0.0f, 100.0f, 22.0f, CC_INK, 160u);
        draw_dots(120.0f, 11.0f, pp.counted, 3.0f, pp.counted, CC_GOLD, CC_GOLD);
    }
    /* the ruler: each column's number as a dot group */
    box_a(10.0f, 200.0f, 220.0f, 24.0f, CC_INK, 110u);
    for (c = 0; c < 8; c++) {
        int n = c + 1 + page * 8, j;
        float x = PP_COLX(c), w = 2.0f * 3.6f;
        int per = n <= 4 ? n : (n + 1) / 2;
        for (j = 0; j < n && j < 16; j++) {
            int row = j / (per > 0 ? per : 1), colj = j % (per > 0 ? per : 1);
            dot3(x + ((float)colj - (float)(per - 1) * 0.5f) * (w * 0.6f), 207.0f + (float)row * 6.0f,
                 pp.colflash == c + page * 8 && T - pp.colflash_t0 < 200.0f ? mood->accent : U_WHITE);
        }
    }
    /* the keyboard mirror: which keys are which (the steps lit, Play, Clear, Challenge) */
    box(0.0f, 226.0f, 240.0f, 14.0f, mix565(CC_INK, mood->accent, 40u));
    for (i = 0; i < NKEYS; i++) {
        float x = 4.0f + (float)i * 8.6f;
        uint16_t kc;
        if (i < 24) {
            int lc = km[i].vcol + page * 8;
            if (!pp_on(km[i].vrow, lc))
                continue;
            kc = lane_c[km[i].vrow];
            kc = lighten(kc, 100u);
        } else {
            kc = i == 24 ? (steps_playing() ? CC_GREEN : darken(CC_GREEN, 80u)) : i == 25 ? HEX(0x4FB8F5) : APPLE_RED;
        }
        box(x, 228.0f, 7.0f, 10.0f, kc);
    }
    for (l = 0; l < 3; l++)                    /* the thirds' borders */
        box(4.0f + (float)(8 * (l + 1)) * 8.6f - 1.4f, 227.0f, 1.0f, 12.0f, U_WHITE);
    DR_SINCE();                                /* (the tests: the sequencer's own shapes, design 10.4: under ~200) */
    dr_ox = dr_oy = 0;
    /* the unicorn: running to a column, marching, stomping */
    {
        float x = pp.ux, ra = T - pp.run_t0, sa = T - pp.squat_t0, st = T - pp.stomp_t0;
        int playing = pos >= 0.0f;
        if (playing) {
            x = PP_COLX(0) + 27.0f * pos;
        } else if (ra >= 0.0f && ra < 180.0f) {
            x = ss_lerp(pp.ux0, pp.utx, ease_out_cubic(ra / 180.0f));
        } else {
            x = pp.utx;
        }
        pp.ux = x;
        p = uni_pose(x, PP_FLOOR, PP_SCALE);
        p.facing = 1;
        p.mouth_state = MO_GRIN;
        if (playing) {                         /* marching: a foot down at every column */
            p.leg_phase = 3.14159f * pos;
            p.lift = 3.0f * ss_absf(fm_sinf(3.14159f * pos));
            p.head_rot = 3.0f * fm_sinf(3.14159f * pos);
            if (st >= 0.0f && st < 140.0f) {
                float e = 1.0f - st / 140.0f;
                p.scale_y *= 1.0f - (0.12f + 0.1f * ch) * e;
                p.scale_x *= 1.0f + (0.1f + 0.08f * ch) * e;
            }
        } else if (ra >= 0.0f && ra < 180.0f) {
            p.leg_phase = ra / 25.0f;
        }
        if (!playing && sa >= 0.0f && sa < 360.0f) {   /* the squat, then the pop */
            float e = sa < 120.0f ? sa / 120.0f : 1.0f - ease_out_elastic((sa - 120.0f) / 240.0f);
            p.scale_y *= 1.0f - 0.16f * e;
            p.scale_x *= 1.0f + 0.12f * e;
            p.tail_phase = 1.0f;
            p.eye_state = sa < 120.0f ? EY_SQUINT : EY_WIDE;
            p.mouth_state = sa < 120.0f ? MO_STRAIN : MO_O;
        }
        if (pp.held & 0xFFFFFFu && T - pp.hold_t0 > 250.0f && !playing) {   /* holding a step: straining hard */
            p.spread = 0.6f;
            p.vibrate = 1.0f;
            p.tail_phase = 1.0f;
            p.eye_state = EY_SQUINT;
            p.mouth_state = MO_STRAIN;
            p.cheeks = 0.8f;
        }
        if (learn_dancing() || pp.march) {     /* the victory march: she prances, eyes happy */
            p.eye_state = EY_HAPPY;
            p.blush = 0.8f;
            p.arm_l = 0.5f + 0.4f * fm_sinf(T / 120.0f);
        }
        if (st >= 0.0f && st < 500.0f && !playing) {   /* (the no-challenge twirl) */
            p.facing = ((int)(st / 120.0f) & 1) ? -1 : 1;
            p.lift = 10.0f * fm_sinf(st / 500.0f * 3.14159f);
        }
        uni_draw(&p);
        if (playing && x > 220.0f) {           /* off the right edge: she comes round again on the left */
            pose_t q = p;
            q.x = x - 27.0f * 8.0f;
            uni_draw(&q);
        }
    }
}

static void parade_clean(void)
{
    steps_play(0);
    pp.march = 0;
    if (learn_phase() == LP_WAIT || learn_phase() == LP_DEMO) {   /* a cleanup during a challenge: it just ends */
        pp.demo_on = 0;
        learn_end();
        pp_clear_all(0);
        pp_restore();
        return;
    }
    pp_clear_all(1);
}

static const toy_def_t TOY_PARADE_DEF = {
    "POOP PARADE", B_EDIT, TF_HOLD | TF_LEARN, parade_init, parade_leave, parade_key, parade_release, parade_mega, parade_knob,
    parade_update, parade_draw, parade_clean, PARADE_PRESETS, 5,
    {{KI_CLOCK, 21, 0, 6}, {KI_SWING, 21, 0, 0}, {KI_STEPS, 2, 0, 0}, {KI_DRUM, 4, 1, 0}, {KI_CHAOS, 21, 0, 6}},
    parade_button, TOY_POOP,
};
