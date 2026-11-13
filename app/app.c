/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3 "Recess": eight silly toys for a 3-year-old on the M-VAVE FM-1, and four learning toys behind the
 * School Bell. No menus, no text: a button picks a toy, any key makes it act, every knob does something, and nothing
 * she plays can sound wrong (every note snaps to the mood's scale). Nothing she does can fail, either.
 *
 *   HOME  Unicorn Fart    EDIT  Unicorn Poop    LFO  Unicorn Pee     FX   Unicorn Burp
 *   ENV   Unicorn Sneeze  GLO   Drum Jam        ARP  Toilet Flush    SEQ  Booger Flick
 *   PLAY  auto-play       REC   Drum Jam's loop recorder; elsewhere the last 10 s played back
 *   OCT- / OCT+  the toy's sound presets (hold OCT+: back to the first)
 *   SEL   the next unlocked hat (hold: the next mane)      SAVE  a surprise (hold 1 s: quiet mode, 20 %)
 *   K1 volume (70 % cap)   K5 (SELECT) the mood: scale and colours together   K2 K3 K4 K6 (ALGO) K7 (PRESET) the toy's
 *   Hold the toy's own button: sweep the screen clean (in the Toilet: flush everything).
 * v3 (school.h): OCT- held = the School Bell. In School mode HOME, EDIT, LFO and SEQ open Fart Mix-Up, Poop Parade,
 *   Pee Piano and Booger Wall; a toy's button held 1.5 s opens its learning toy directly; OCT- held 2 s: the Sticker
 *   Book. With School mode off, v2 is unchanged.
 *
 * Part of fm1-base's unity build (device/unity.c, host/plat_host.c), after the platform: gfx.c (cv_px, cv_begin,
 * cv_blit, RGB, swap16), announce.c and lib/touch.h are in scope. This file is the app's own unity build: the modules
 * below are included in order. Everything is drawn procedurally into one 240x240 canvas and sent once a frame. */
#include "util.h"
#include "draw.h"
#include "fx.h"
#include "mood.h"
#include "particles.h"
#include "patch.h"
#include "voice.h"
#include "unicorn.h"
#include "toys.h"
#include "knobs.h"
#include "ui.h"
#include "keymap.h"
#include "seq.h"
#include "learn.h"
#include "stickers.h"
#include "school.h"
#include "fm2.h"
#include "ribbon.h"
#include "ladder.h"

#include "draw.c"
#include "fx.c"
#include "mood.c"
#include "particles.c"
#include "patch.c"
#include "voice.c"
#include "seq.c"
#include "unicorn.c"
#include "knobs.c"
#include "ui.c"
#include "keymap.c"
#include "learn.c"
#include "stickers.c"
#include "school.c"
#include "fm2.c"
#include "ribbon.c"
#include "ladder.c"

#include "toys/common.c"
#include "toys/fart.c"
#include "toys/poop.c"
#include "toys/pee.c"
#include "toys/burp.c"
#include "toys/sneeze.c"
#include "toys/drumjam.c"
#include "toys/toilet.c"
#include "toys/booger.c"
#include "toys/poop_parade.c"
#include "toys/fart_mixup.c"
#include "toys/pee_piano.c"
#include "toys/booger_wall.c"

/* the registry: one line a toy */
static const toy_def_t *TOYREG[NTOYS] = {&TOY_FART_DEF, &TOY_POOP_DEF, &TOY_PEE_DEF, &TOY_BURP_DEF, &TOY_SNEEZE_DEF,
                                         &TOY_DRUM_DEF, &TOY_TOILET_DEF, &TOY_BOOGER_DEF,
                                         &TOY_PARADE_DEF, &TOY_MIXUP_DEF, &TOY_PIANO_DEF, &TOY_WALL_DEF};

/* ============================================================ app state === */
#define LONG_MS 700u                           /* a long press */
static struct {
    uint32_t last_ms;
    uint32_t keys, btn;
    uint32_t key_t0[NKEYS];
    uint32_t btn_t0[NB];
    uint32_t btn_long;                         /* buttons whose long press has fired */
    uint32_t btn_vlong;                        /* v3: and whose very long one (1.5 s: a learning toy; OCT- 2 s: the book) */
    uint32_t btn_switched;                     /* v3: buttons whose press opened a toy (their tap asks for nothing) */
    uint8_t trans_n;
    uint32_t tick_at;                          /* the transition's tick at its end */
    /* auto-play */
    uint8_t autoplay;
    uint32_t auto_next, auto_release;
    int8_t auto_key;
    int8_t auto_deg;
    uint8_t spoke;
    uint32_t megas, mood_changes, cleans;      /* (the tests) */
} G;

static void snd_apply_volume(void)
{
    snd_volume((float)set.vol / 10.0f);
    snd_quiet(set.quiet);
}

/* ======================================================= mood: the magic trick === */
static void seq_pitches(void)
{
    uint32_t k;
    for (k = 0; k < NKEYS; k++)
        seq_set_pitch(k, (float)mood_key_midi(k > 17u ? k - 18u : k, 5));
}

static void on_mood_landed(int m)
{
    set.mood = (uint8_t)m;
    mood_set(m, 1);
    seq_pitches();
    part_mood_burst(120.0f, 110.0f, 30, 2.0f, 6.0f);
    part_burst(PT_SPARKLE, 120.0f, 110.0f, 16, 1.0f, 4.0f, 24, mood->accent, 2);
    ring_spawn(120.0f, 110.0f, 10.0f, 7.0f, 1.0f, 420.0f, 6.0f, mood->accent);
    chord(P_CHIME, 5, 0, 0.7f);                /* a three-note sting in the new scale */
    ui_led_flash_all(150u);
    fx_trauma(0.15f);
    G.mood_changes++;
    knobs_save_soon();
}

/* ================================================================ toys === */
static void switch_toy(int t)
{
    static const float CORNER[NTOYS][2] = {{0, 0}, {240, 0}, {0, 240}, {240, 240}, {120, 0}, {120, 240}, {0, 120}, {240, 120},
                                           {240, 0}, {0, 0}, {0, 240}, {240, 120}};
    int kind;
    ui_charge_cancel();
    if (TOY && TOY->leave)
        TOY->leave();
    if (ui_replaying())
        ui_replay_toggle();
    part_clear();
    fx_clear();
    minis_clear();
    mood_props_home();
    mood_bg_tint(0, 0.0f);
    toy_crush = 0.0f;
    part_on_floor = 0;
    g_led_lit = g_led_mid = 0;
    cur_toy = (uint8_t)(t % NTOYS);
    knobs_enter_toy();
    uni_rig.init = 0;
    seq_enable(cur_toy == TOY_DRUM);
    TOY->init();
    kind = mood_i == MOOD_GLITCH ? TR_GLITCH : (int)(G.trans_n++ % (NTRANS - 1));
    fx_transition(kind, CORNER[cur_toy][0], CORNER[cur_toy][1], mix565(mood->accent, CC_INK, 140u));
    snd_play(P_WHOOSH, 0.0f, 0.8f);            /* a quick swell, then a tick in time at its end */
    G.tick_at = plat_ms() + 210u;
    G.spoke = 0;
}

/* ================================================================ keys === */
static void unlock_check(void)
{
    if (set.actions % UNLOCK_EVERY != 0u || set.unlocked >= NUNLOCK)
        return;
    set.unlocked++;
    if (set.unlocked & 1u) {                   /* a hat: on at once, so she sees it */
        set.hat = (uint8_t)(hats_unlocked() - 1);
        ui_unlock_flash(HATS[set.hat]);
    } else {
        set.mane = (uint8_t)(manes_unlocked() - 1);
        ui_unlock_flash(cur_hat());
    }
    chord(P_CHIME, 6, 0, 0.6f);
    part_burst(PT_CONFETTI, 120.0f, 96.0f, 24, 2.0f, 6.0f, 36, 0, 4);
    knobs_save_soon();
}

/* everything at once: the mash (5 presses in 200 ms) */
static void app_mega_all(uint32_t k)
{
    G.megas++;
    fx_invert();
    chord(P_STAB, 4, 0, 0.8f);
    snd_play(P_FART_TUBA, (float)mood_degree_midi(2, 0), 0.9f);
    snd_play(P_PLOP_MARIMBA, (float)mood_degree_midi(5, 2), 0.8f);
    part_mood_burst(120.0f, 110.0f, 40, 2.0f, 8.0f);
    part_burst(PT_CONFETTI, 120.0f, 100.0f, 40, 2.0f, 7.0f, 40, 0, 4);
    part_fan(PT_CLOUD, 90.0f, 140.0f, 8, 3.14159f, 0.6f, 1.0f, 3.0f, 30, lighten(CC_PINK, 100u), 9);
    part_fan(PT_BUBBLE, 160.0f, 110.0f, 10, -1.57f, 0.8f, 1.0f, 3.0f, 50, mood->accent, 4);
    part_fan(PT_DROPLET, 100.0f, 150.0f, 14, -2.4f, 0.4f, 3.0f, 6.0f, 50, 0, 2);
    part_fan(PT_POOPLET, 90.0f, 150.0f, 4, -2.2f, 0.4f, 2.0f, 4.0f, 50, 0, 4);
    ring_spawn(120.0f, 120.0f, 10.0f, 8.0f, 1.0f, 500.0f, 7.0f, U_WHITE);
    impact(0.6f, 4);
    ui_led_flash_all(250u);
    if (TOY->on_mega)
        TOY->on_mega(k);
}

static void app_flourish(int big)
{
    chord(P_STAB, big ? 5 : 4, big ? 2 : 0, big ? 0.8f : 0.6f);
    part_burst(PT_CONFETTI, 120.0f, 80.0f, big ? 40 : 20, 2.0f, big ? 8.0f : 5.0f, 40, 0, 4);
    part_mood_burst(120.0f, 80.0f, big ? 30 : 14, 2.0f, 6.0f);
    fx_trauma(big ? 0.5f : 0.25f);
    if (big)
        ui_led_flash_all(400u);
}

static void app_key_down(uint32_t k, float vel, int synthetic)
{
    if (k >= NKEYS)
        return;
    if (book_is_open()) {                      /* v3: the Sticker Book has the keys while it is open */
        if (!synthetic) {
            ui_input();
            book_key(k);
        }
        return;
    }
    G.key_t0[k] = plat_ms();
    if (!synthetic) {
        ui_rec_event(k, vel, 1);
        ui_input();
        set.actions++;
        unlock_check();
    }
    ui_led_key(k);
    ui_chaos_press();
    synth_key = (uint8_t)synthetic;            /* (v3: a learning toy never judges auto-play or a replay) */
    TOY->on_key(k, vel);
    synth_key = 0;
    if (!(TOY->flags & TF_HOLD))
        ui_charge_press(k);
    if (chaos > 0.7f && cur_toy != TOY_DRUM)   /* high chaos: an extra sparkle voice on top, in the scale */
        snd_play(P_CHIME, (float)mood_key_midi(k, 6), 0.3f);
    if (ui_mash_press())
        app_mega_all(k);
}

static void app_key_up(uint32_t k, int synthetic)
{
    uint32_t held;
    if (k >= NKEYS)
        return;
    held = plat_ms() - G.key_t0[k];
    if (book_is_open())
        return;
    if (!synthetic)
        ui_rec_event(k, 0.0f, 0);
    TOY->on_release(k, held);
    if (ui_charge_release(k) && TOY->on_mega)
        TOY->on_mega(k);
}

/* ============================================================== buttons === */
static int toy_of_button(uint32_t b)
{
    int t;
    for (t = 0; t < NTOYS_V2; t++)              /* (the play toys: a learning toy shares its play toy's button) */
        if (TOYREG[t]->button_id == b)
            return t;
    return -1;
}
static int play_of(int t) { return TOYREG[t]->flags & TF_LEARN ? TOYREG[t]->play_toy : t; }

/* v3: every fourth new sticker brings the wardrobe's next item (stickers.c) */
static void app_wardrobe_gift(void)
{
    if (set.unlocked >= NUNLOCK)
        return;
    set.unlocked++;
    if (set.unlocked & 1u) {
        set.hat = (uint8_t)(hats_unlocked() - 1);
        ui_unlock_flash(HATS[set.hat]);
    } else {
        set.mane = (uint8_t)(manes_unlocked() - 1);
        ui_unlock_flash(cur_hat());
    }
    knobs_save_soon();
}

static void preset_step(int d)
{
    int n = TOY->preset_count, p = ((int)set.preset[cur_toy] + d + n) % n;
    set.preset[cur_toy] = (uint8_t)p;
    ui_preset_flash(TOY->presets[p].icon, p, n);
    if (cur_toy == TOY_DRUM)                   /* audition: a kit plays a kick and a snare; the rest a note */
        drum_audition();
    else
        snd_play(TOY->presets[p].patch, (float)mood_degree_midi(4, 0), 0.8f);
    knobs_save_soon();
}

static void app_clean(void)
{
    G.cleans++;
    if (TOY->clean)
        TOY->clean();
    if (cur_toy != TOY_TOILET) {               /* (the toilet flushes instead) */
        sweep_start();
        snd_play(P_SWOOSH, 0.0f, 0.8f);
        part_clear();
        minis_clear();
        mood_props_home();
    }
}

static void button_press(uint32_t b)
{
    int t = toy_of_button(b);
    if (t >= 0) {
        t = school_route(t);                   /* v3: in School mode, the learning toy if it has one */
        G.btn_switched &= ~(1u << b);
        if (t != cur_toy) {
            switch_toy(t);                     /* another toy: at once */
            G.btn_switched |= 1u << b;
        }
        return;                                /* (this toy's: a long press cleans, decided on hold) */
    }
    if (TOY->on_button && TOY->on_button(b, BK_PRESS))
        return;
    switch (b) {
    case B_PLAY:
        G.autoplay ^= 1u;
        g_autoplay = G.autoplay;
        G.auto_next = plat_ms() + 300u;
        if (!G.autoplay && G.auto_key >= 0) {
            app_key_up((uint32_t)G.auto_key, 1);
            G.auto_key = -1;
        }
        break;
    case B_OCTDN:
        preset_step(-1);
        break;
    case B_OCTUP:
        preset_step(1);
        break;
    default:
        break;
    }
}

static void button_long(uint32_t b)
{
    int t = toy_of_button(b);
    school_discover();                         /* v3: the first three long presses ever sparkle */
    if (t >= 0 && TOY->button_id == b) {
        app_clean();
        return;
    }
    if (TOY->on_button && TOY->on_button(b, BK_LONG))
        return;
    switch (b) {
    case B_OCTDN:                              /* v3: the School Bell (the tap's preset step is taken back) */
        if (school_hatch_used())
            break;
        set.preset[cur_toy] = (uint8_t)((set.preset[cur_toy] + 1u) % TOY->preset_count);
        UI.preset_t0 = plat_ms() - 100000u;
        school_toggle();
        break;
    case B_OCTUP:                              /* back to the first preset */
        set.preset[cur_toy] = 0;
        ui_preset_flash(TOY->presets[0].icon, 0, TOY->preset_count);
        knobs_save_soon();
        break;
    case B_REC:
        if (cur_toy == TOY_DRUM)
            seq_clear();
        break;
    case B_SEL:                                /* the next mane */
        set.mane = (uint8_t)((set.mane + 1u) % (uint32_t)manes_unlocked());
        ui_unlock_flash(cur_hat());
        snd_play(P_CHIME, (float)mood_degree_midi(6, 4), 0.5f);
        knobs_save_soon();
        break;
    case B_SAVE:                               /* quiet mode: bedtime */
        set.quiet ^= 1u;
        snd_apply_volume();
        ui_quiet_flash();
        knobs_save_soon();
        break;
    default:
        break;
    }
}

static void button_tap(uint32_t b)             /* released before LONG_MS */
{
    int t = toy_of_button(b);
    if (t >= 0 && TOY->button_id == b) {       /* this toy's button: she waves hello */
        ui_input();
        if ((TOY->flags & TF_LEARN) && !(G.btn_switched >> b & 1u))   /* v3: in a learning toy, a wave is also "a
                                                                     * challenge, please" (not the tap that opened it) */
            learn_request();
        snd_play(P_CHIME, (float)mood_degree_midi(5, 0), 0.5f);
        part_mood_burst(120.0f, 90.0f, 8, 1.0f, 3.0f);
        return;
    }
    if (TOY->on_button && TOY->on_button(b, BK_TAP))
        return;
    switch (b) {
    case B_REC:
        if (cur_toy == TOY_DRUM)
            seq_rec();
        else
            ui_replay_toggle();
        break;
    case B_SEL:                                /* the next hat she has */
        set.hat = (uint8_t)((set.hat + 1u) % (uint32_t)hats_unlocked());
        snd_play(P_POP, (float)mood_degree_midi(6, 2), 0.6f);
        part_burst(PT_SPARKLE, 190.0f, 70.0f, 10, 1.0f, 3.0f, 20, mood->accent, 2);
        knobs_save_soon();
        break;
    case B_SAVE:                               /* a surprise */
        app_flourish(0);
        break;
    default:
        break;
    }
}

/* ================================================================ input === */
static void input(void)
{
    uint32_t keys, btn, k, pressed, released, bp, br, now = plat_ms();
    touch_poll();
    keys = touch_keys();
    btn = plat_buttons();
    pressed = keys & ~G.keys;
    released = G.keys & ~keys;
    G.keys = keys;
    for (k = 0; k < NKEYS; k++) {
        if (pressed >> k & 1u)
            app_key_down(k, (float)touch_vel(k) / 127.0f, 0);
        if (released >> k & 1u)
            app_key_up(k, 0);
    }
    bp = btn & ~G.btn;
    br = G.btn & ~btn;
    G.btn = btn;
    for (k = 0; k < NB; k++) {
        if (bp >> k & 1u) {
            G.btn_t0[k] = now;
            G.btn_long &= ~(1u << k);
            G.btn_vlong &= ~(1u << k);
            ui_input();
            if (k == B_OCTDN)
                school_boot_press();
            if (book_is_open()) {              /* v3: any button closes the Sticker Book (and does nothing else) */
                book_close();
                G.btn_long |= 1u << k;
                G.btn_vlong |= 1u << k;
                continue;
            }
            button_press(k);
        }
        if ((btn >> k & 1u) && !(G.btn_long >> k & 1u) && now - G.btn_t0[k] >= (k == B_SAVE ? 1000u : LONG_MS)) {
            G.btn_long |= 1u << k;
            button_long(k);
        }
        if ((btn >> k & 1u) && (G.btn_long >> k & 1u) && !(G.btn_vlong >> k & 1u)) {   /* v3: held longer still */
            if (toy_of_button(k) >= 0 && now - G.btn_t0[k] >= DIRECT_MS) {
                G.btn_vlong |= 1u << k;
                school_direct(toy_of_button(k));
            } else if (k == B_OCTDN && now - G.btn_t0[k] >= 2000u && !school_hatch_used()) {
                G.btn_vlong |= 1u << k;
                book_open();
            }
        }
        if ((br >> k & 1u) && !(G.btn_long >> k & 1u))
            button_tap(k);
    }
    knobs_poll();
}
void app_input_only(void) { input(); }

/* auto-play: the toy plays itself, a little tune walking up and down the scale */
static void autoplay_tick(void)
{
    uint32_t now = plat_ms();
    if (!G.autoplay || cur_toy == TOY_DRUM || TOY->on_button)   /* (Drum Jam's PLAY is its auto-drummer; a toy that
                                                                 * takes PLAY for itself, Poop Parade, plays its own) */
        return;
    if ((int32_t)(now - G.auto_next) >= 0) {
        if (G.auto_key >= 0)
            app_key_up((uint32_t)G.auto_key, 1);
        G.auto_deg = (int8_t)clamp(G.auto_deg + (ss_rnd() & 1u ? 3 : -3) + ss_rndi(3) - 1, 0, 26);
        G.auto_key = G.auto_deg;
        app_key_down((uint32_t)G.auto_key, 0.75f, 1);
        G.auto_release = now + 250u + (uint32_t)ss_rndi(500);
        G.auto_next = now + 1400u + (uint32_t)ss_rndi(600);
    }
    if (G.auto_key >= 0 && (int32_t)(now - G.auto_release) >= 0) {
        app_key_up((uint32_t)G.auto_key, 1);
        G.auto_key = -1;
    }
}


#ifdef PLAT_HOST
static uint32_t toy_draws, toy_draws_max;      /* v3: the toy's shapes a frame, and the most since "draws_reset" */
/* ------------------------------------------------- v3: the tests' answers --- */
/* "answer": the keys the open challenge wants, pressed and released in time, as she would; "answer wrong": one try
 * that is not it. Key -1: Sort's mouth for the booger falling now; -2: K4 to the dots Count the Bumps wants. */
static struct { int16_t key; uint8_t down; uint32_t at; } HQ[48];
static int hq_n;
static void hq_push(int key, uint32_t at, uint32_t hold)
{
    if (hq_n + 2 > 48)
        return;
    HQ[hq_n].key = (int16_t)key; HQ[hq_n].down = 1; HQ[hq_n].at = at; hq_n++;
    HQ[hq_n].key = (int16_t)key; HQ[hq_n].down = 0; HQ[hq_n].at = at + hold; hq_n++;
}
static void hq_tick(void)
{
    uint32_t now = plat_ms();
    int i, j;
    for (i = 0; i < hq_n;) {
        if ((int32_t)(now - HQ[i].at) < 0) {
            i++;
            continue;
        }
        if (HQ[i].key == -2) {
            if (HQ[i].down) {
                set.knob[cur_toy][2] = (uint8_t)((int)mx.want - 1);   /* 2:1 is position 2, 3:1 position 3 */
                if (TOY->on_knob)
                    TOY->on_knob(K4, kv(K4));
            }
        } else {
            int k = HQ[i].key;
            if (k == -1) {                     /* Sort: the mouth of the colour falling now */
                int f = bw_falling();
                k = km_find(KR_ANSWER, 0, f >= 0 ? bw.b[f].col + 1 : 1);
            }
            if (k >= 0 && HQ[i].down)
                app_key_down((uint32_t)k, 0.8f, 0);
            else if (k >= 0)
                app_key_up((uint32_t)k, 0);
        }
        for (j = i; j + 1 < hq_n; j++)
            HQ[j] = HQ[j + 1];
        hq_n--;
    }
}
static int key_for_midi(int lo, int hi, float want, int right)   /* Mix-Up: the key that plays this note */
{
    int k;
    for (k = lo; k <= hi; k++) {
        int8_t save = right ? mx.key_r : mx.key_l;
        float m;
        if (right)
            mx.key_r = (int8_t)k;
        else
            mx.key_l = (int8_t)k;
        m = right ? mx_right_midi() : mx_left_midi();
        if (right)
            mx.key_r = save;
        else
            mx.key_l = save;
        if (ss_absf(m - want) < 0.5f)
            return k;
    }
    return -1;
}
static void host_answer(int wrong)
{
    uint32_t t = plat_ms() + 20u;
    int i, l, c, k;
    switch (cur_toy) {
    case TOY_PARADE:
        if (learn_kind() == PK_COUNT) {
            for (i = 0; i < (wrong ? 1 : pp.norder); i++)
                hq_push(26, t + (uint32_t)i * 300u, 60u);
        } else if (learn_kind() == PK_COPY) {
            if (wrong) {                       /* a cell that is not in the pattern */
                for (k = 0; k < 24; k++)
                    if (!(pp.target[km[k].vrow] >> km[k].vcol & 1u) && !pp_on(km[k].vrow, km[k].vcol))
                        break;
                hq_push(k, t, 60u);
                break;
            }
            for (l = 0, i = 0; l < NLANES; l++)
                for (c = 0; c < 8; c++)
                    if (pp.target[l] >> c & 1u)
                        hq_push(km_find(KR_STEP, l, c), t + (uint32_t)(i++) * 300u, 60u);
        } else {
            if (wrong) {
                hq_push(km_find(KR_STEP, 0, (pp.order[pp.oi] % 16 + 3) % 8), t, 60u);
                break;
            }
            for (i = pp.oi; i < pp.norder; i++)
                hq_push(km_find(KR_STEP, 0, pp.order[i] % 16), t + (uint32_t)(i - pp.oi) * 300u, 60u);
        }
        break;
    case TOY_MIXUP: {
        float root = (float)mood_degree_midi(3, 0) + (float)(kv(K2) - 10);
        int kl = key_for_midi(0, 13, quantize_to_scale(root), 0), kr;
        float iv = learn_kind() == MK_COPY ? mx.want : 12.0f;
        if (kl < 0)
            kl = 0;
        if (learn_kind() == MK_BUMPS && !wrong)
            hq_push(-2, t, 10u);
        kr = wrong ? -1 : key_for_midi(14, 26, quantize_to_scale(root) + iv, 1);
        if (wrong) {                           /* a right key the cloud does not like */
            for (k = 14; k <= 26 && kr < 0; k++) {
                int8_t sl = mx.key_l, sr = mx.key_r;
                mx.key_l = (int8_t)kl;
                mx.key_r = (int8_t)k;
                if (fm2_class(mx_right_midi() - mx_left_midi()) == CL_CROSS)
                    kr = k;
                mx.key_l = sl;
                mx.key_r = sr;
            }
        }
        if (kr < 0)
            kr = 14;
        hq_push(kl, t + 30u, 1100u);
        hq_push(kr, t + 60u, 1000u);
        break;
    }
    case TOY_PIANO:
        if (learn_kind() == NK_BUCKET) {
            float rate = 0.00012f + 0.00018f * 13.0f / 26.0f;
            uint32_t ms = (uint32_t)((pn.line + (wrong ? 0.3f : -0.04f)) / rate) + 150u;
            hq_push(13, t, ms);
            break;
        }
        for (i = pn.ti; i < pn.ntune; i++) {
            int r = wrong ? (pn.tune[i] + 1) % pn_rungs() : pn.tune[i];
            for (k = 0; k < NKEYS; k++)
                if (ladder_hit(PN_TOP, PN_BOT, pn_rungs(), pn_target((uint32_t)k), pn_tol()) == r)
                    break;
            hq_push(k, t, 700u);
            t += 1300u;
            if (wrong)
                break;
        }
        break;
    case TOY_WALL:
        if (bw.game == BG_COUNT) {
            hq_push(km_find(KR_ANSWER, 0, wrong ? bw.target % bw.zones + 1 : bw.target), t, 60u);
        } else if (bw.game == BG_SORT) {
            for (i = 0; i < (wrong ? 1 : 5); i++)
                hq_push(-1, t + (uint32_t)i * 400u, 60u);
        } else {                               /* Match: every pair, in turn */
            int n = bw.cols * 2, a, b2;
            for (a = 0; a < n; a++) {
                if (bw.state[a] == 2u)
                    continue;
                for (b2 = a + 1; b2 < n; b2++)
                    if (bw.state[b2] != 2u && bw.kind[b2] == bw.kind[a])
                        break;
                if (b2 >= n)
                    continue;
                if (wrong) {                   /* two that differ */
                    for (b2 = a + 1; b2 < n && bw.kind[b2] == bw.kind[a]; b2++)
                        ;
                }
                hq_push(km_find(KR_CUBBY, a / bw.cols, a % bw.cols), t, 60u);
                hq_push(km_find(KR_CUBBY, b2 / bw.cols, b2 % bw.cols), t + 150u, 60u);
                t += 400u;
                if (wrong)
                    break;
            }
            /* (the pairs found mark state 2 only as they are pressed: a pair found early is skipped by state) */
        }
        break;
    default:
        break;
    }
}
int app_host_cmd(const char *cmd, const char *a, const char *b, const char *c, const char *d)
{
    (void)b;
    (void)c;
    (void)d;
    if (!strcmp(cmd, "draws_reset")) {
        toy_draws_max = 0;
        dr_part_max = 0;
        return 1;
    }
    if (!strcmp(cmd, "fake_v2")) {             /* a v2 unit's settings in the flash (knobs_load migrates them) */
        settings_v2_t v2;
        int t2, k2;
        ss_zero(&v2, sizeof v2);
        v2.magic = SET_MAGIC_V2;
        v2.vol = 3;
        v2.mood = 2;
        v2.unlocked = 2;
        v2.actions = 95;
        for (t2 = 0; t2 < NTOYS_V2; t2++)
            for (k2 = 0; k2 < NTKNOBS; k2++)
                v2.knob[t2][k2] = TOYREG[t2]->knobs[k2].def;
        v2.preset[TOY_POOP] = 2;
        v2.knob[TOY_POOP][0] = 11;
        plat_store_save(OBJ_SETTINGS, &v2, sizeof v2);
        KN.save_at = 0;                        /* (nothing of ours written over it before the power cycle) */
        return 1;
    }
    if (strcmp(cmd, "answer"))
        return 0;
    host_answer(!strcmp(a, "wrong"));
    return 1;
}
#endif

/* ================================================================== app === */
void app_say(const char *a, const char *b)
{
    if (ann_on())
        ann_say(a, b, ANN_NOW);
}
static void speak(void)                         /* for a screen reader, if one listens: which toy */
{
    if (ann_on() && (ann_where() || !G.spoke)) {
        G.spoke = 1;
        ann_say(APP_NAME, TOY->name, ANN_NOW);
    }
    ann_tick(plat_ms());
}

void app_init(void)
{
    voice_init();
    draw_init();
    knobs_load();
    mood_set(set.mood, 0);
    snd_apply_volume();
    seq_pitches();
}

void app_ui_init(void)
{
    touch_on(1);
    lcd_fill(0, 0, 240, 240, HEX(0x1E1B4B));
    ss_seed ^= plat_ms() * 2654435761u + 1u;
    G.last_ms = plat_ms();
    G.auto_key = -1;
    G.auto_deg = 8;
    FX.flash_ok_at = plat_ms();
    FX.flash_gap = 0xFFFFFFFFu;
    ui_init();
    school_boot();
    cur_toy = 0;
    switch_toy(school_route(TOY_FART));        /* (v3: School mode is remembered: then Fart Mix-Up) */
}

void app_frame(void)
{
    uint32_t now = plat_ms(), dt = now - G.last_ms;
    float fdt;
    G.last_ms = now;
    if (dt > 50u)
        dt = 50u;
    fdt = (float)dt;
    GT.fdt = fdt;
    input();
    speak();
    autoplay_tick();
    knobs_tick();
    sched_tick();
#ifdef PLAT_HOST
    hq_tick();
#endif                              /* v3: the few notes in the future (stings, arpeggios) */
    if (G.tick_at && (int32_t)(now - G.tick_at) >= 0) {
        G.tick_at = 0;
        snd_play(P_TICK, (float)mood_degree_midi(6, 0), 0.6f);
    }
    /* update: skipped while frozen (hitstop), so the impact reads; drawing goes on */
    if (!fx_frozen()) {
        GT.T += fdt;
        TOY->update(fdt);
        particles_update(fdt);
        fx_update(fdt);
        mood_props_update(fdt);
        minis_update(fdt);
    }
    ui_update(fdt);
    knobs_update(fdt);
    snd_chaos(fm_maxf(0.0f, chaos - 0.55f) * 0.6f + toy_crush);
    fx_frame();
    /* draw: the whole frame on the canvas, then one transfer */
    cv_begin(240, 240, 0);
    dr_glows = 0;
    {
        int32_t sx = dr_ox, sy = dr_oy;
        dr_ox = dr_oy = 0;
        mood_draw_sky();                       /* (the sky never shakes, so the edges never show) */
        dr_ox = sx / 3;
        dr_oy = sy / 3;
        mood_props_draw(GT.T, 0);              /* the far layers shake less: parallax */
        dr_ox = sx;
        dr_oy = sy;
    }
#ifdef PLAT_HOST
    {
        uint32_t c0 = dr_calls;
        TOY->draw(GT.T);
        toy_draws = dr_calls - c0;             /* (the toy's own shapes this frame) */
        if (toy_draws > toy_draws_max)
            toy_draws_max = toy_draws;
    }
#else
    TOY->draw(GT.T);
#endif
    mood_props_draw(GT.T, 1);
    fx_draw_world();
    particles_draw();
    minis_draw();
    ui_draw_dancers(GT.T);
    draw_sweep();
    ui_draw_chaos();
    fx_draw_post();
    ui_draw_overlays();
    if (set.school_on)                         /* v3 */
        learn_draw();
    school_draw(GT.T);
    sticker_toast_draw(GT.T);
    book_draw(GT.T);
    knobs_draw();
    cv_blit(0, 0);
    g_rec_lit = cur_toy == TOY_DRUM ? (uint8_t)(seq_loop_state() == LOOP_REC ? 1 : seq_loop_state() == LOOP_PLAY ? 3 : 0)
                                    : (uint8_t)(ui_replaying() ? 2 : 0);
    ui_leds();
}

#ifdef PLAT_HOST
static int toy_expect(const char *what, int *got)
{
    int i, n = 0;
    if (!strcmp(what, "poops"))
        *got = poop_count();
    else if (!strcmp(what, "minis")) {
        for (i = 0; i < NMINI; i++)
            n += minis[i].on;
        *got = n;
    } else if (!strcmp(what, "pee"))
        *got = pee.active;
    else if (!strcmp(what, "clouds")) {
        for (i = 0; i < NCLOUD; i++)
            n += fart.c[i].on;
        *got = n;
    } else if (!strcmp(what, "burp"))
        *got = burp.phase;
    else if (!strcmp(what, "sneeze"))
        *got = snz.phase;
    else if (!strcmp(what, "boogers"))
        *got = booger_count();
    else if (!strcmp(what, "flushing"))
        *got = toil_flushing();
    else if (!strcmp(what, "swallowed"))
        *got = (int)toil.swallowed;
    /* v3 */
    else if (!strcmp(what, "steps")) {         /* Poop Parade: the poops on the track */
        for (i = 0; i < NLANES * STEPS_MAX; i++)
            n += (steps_lane[i / STEPS_MAX] >> (i % STEPS_MAX)) & 1;
        *got = n;
    } else if (!strcmp(what, "playing"))
        *got = steps_playing();
    else if (!strcmp(what, "stomps"))
        *got = (int)pp_stomps;
    else if (!strcmp(what, "stashed"))
        *got = pp.stashed;
    else if (!strcmp(what, "page"))
        *got = pp.page;
    else if (!strcmp(what, "fm_l"))
        *got = fm2_left_on();
    else if (!strcmp(what, "fm_r"))
        *got = fm2_right_on();
    else if (!strcmp(what, "cloud"))           /* 0 none, 1 happy, 2 fine, 3 cross */
        *got = fm2_cloud();
    else if (!strcmp(what, "interval"))
        *got = (int)(fm2_right_midi() - fm2_left_midi() + (fm2_right_midi() >= fm2_left_midi() ? 0.5f : -0.5f));
    else if (!strcmp(what, "rung"))
        *got = pn.on_rung;
    else if (!strcmp(what, "stream"))
        *got = pn.active;
    else if (!strcmp(what, "fill"))
        *got = (int)(pn.fill * 100.0f);
    else if (!strcmp(what, "bucket"))
        *got = pn.bucket;
    else if (!strcmp(what, "wall_game"))
        *got = bw.game;
    else if (!strcmp(what, "wall_cols"))
        *got = bw.cols;
    else if (!strcmp(what, "wall_found"))
        *got = bw.found;
    else if (!strcmp(what, "wall_on"))
        *got = bw_on_wall();
    else if (!strcmp(what, "zones"))
        *got = bw.zones;
    else
        return 0;
    return 1;
}

int app_expect(const char *what, int *got)
{
    if (!strcmp(what, "toy"))
        *got = cur_toy;
    else if (!strcmp(what, "mood"))
        *got = mood_i;
    else if (!strcmp(what, "mood_changes"))
        *got = (int)G.mood_changes;
    else if (!strcmp(what, "auto"))
        *got = G.autoplay;
    else if (!strcmp(what, "voices"))
        *got = snd_voices();
    else if (!strcmp(what, "particles"))
        *got = part_count();
    else if (!strcmp(what, "vol"))
        *got = set.vol;
    else if (!strcmp(what, "quiet"))
        *got = set.quiet;
    else if (!strcmp(what, "preset"))
        *got = set.preset[cur_toy];
    else if (!strcmp(what, "k2") || !strcmp(what, "k3") || !strcmp(what, "k4") || !strcmp(what, "k6") || !strcmp(what, "k7"))
        *got = kv(what[1] == '2' ? K2 : what[1] == '3' ? K3 : what[1] == '4' ? K4 : what[1] == '6' ? K6 : K7);
    else if (!strcmp(what, "flashes"))
        *got = (int)FX.flashes;
    else if (!strcmp(what, "flash_gap"))      /* the shortest time between two full-screen flashes, ms */
        *got = FX.flash_gap > 100000u ? 100000 : (int)FX.flash_gap;
    else if (!strcmp(what, "hitstops"))
        *got = (int)FX.hitstops;
    else if (!strcmp(what, "megas"))
        *got = (int)G.megas;
    else if (!strcmp(what, "cleans"))
        *got = (int)G.cleans;
    else if (!strcmp(what, "chaos"))
        *got = (int)(chaos * 100.0f);
    else if (!strcmp(what, "idle"))
        *got = ui_idle_state();
    else if (!strcmp(what, "charge"))
        *got = (int)(ui_charge() * 100.0f);
    else if (!strcmp(what, "replay"))
        *got = ui_replaying();
    else if (!strcmp(what, "loop"))
        *got = seq_loop_state();
    else if (!strcmp(what, "unlocked"))
        *got = set.unlocked;
    else if (!strcmp(what, "hat"))
        *got = set.hat;
    else if (!strcmp(what, "props_away"))
        *got = mood_props_away();
    /* v3: School mode */
    else if (!strcmp(what, "school"))
        *got = set.school_on;
    else if (!strcmp(what, "learn_off"))
        *got = set.learn_off;
    else if (!strcmp(what, "learn_phase"))     /* 0 off, 1 free, 2 demo, 3 her turn, 4 cheer */
        *got = learn_phase();
    else if (!strcmp(what, "learn_kind"))
        *got = learn_kind();
    else if (!strcmp(what, "learn_level"))
        *got = learn_level();
    else if (!strcmp(what, "learn_misses"))
        *got = learn_misses();
    else if (!strcmp(what, "learn_wins"))      /* the current learning toy's challenges won, ever */
        *got = LN.lt >= 0 ? set.learn[LN.lt].wins : 0;
    else if (!strcmp(what, "stickers"))
        *got = sticker_count();
    else if (!strcmp(what, "book"))
        *got = book_is_open();
    else if (!strcmp(what, "discover"))
        *got = set.discover_n;
    else if (!strcmp(what, "sched"))           /* notes waiting in the scheduler */
        *got = sched_pending();
    else if (!strcmp(what, "draws"))           /* the most shapes the toy drew in one frame since draws_reset */
        *got = (int)toy_draws_max;
    else if (!strcmp(what, "part_draws"))      /* and in its measured section (Mix-Up's ribbons, the Parade's grid) */
        *got = (int)dr_part_max;
    else if (!toy_expect(what, got))
        return 0;
    return 1;
}
#endif
