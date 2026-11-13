/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: the School Bell (school.h). */

static void switch_toy(int t);               /* app.c */
static void chord(int patch, int octave, int root_deg, float vel);   /* toys/common.c */

#define APPLE_RED HEX(0xEF4444)
#define DIRECT_MS 1500u                      /* a toy's button held this long: its learning toy */
#define HATCH_MS 5000u                       /* the parent's switch works this long after power-on */

static struct {
    uint32_t bell_t0, recess_t0, disc_t0, hatch_t0, boot_ms;
    uint8_t hatch_press;
} SC;

static const int8_t LEARN_OF[NTOYS_V2] = {TOY_MIXUP, TOY_PARADE, TOY_PIANO, -1, -1, -1, -1, TOY_WALL};

static int school_learn_of(int play_toy) { return play_toy >= 0 && play_toy < NTOYS_V2 ? LEARN_OF[play_toy] : -1; }
static int school_route(int play_toy)
{
    int l = school_learn_of(play_toy);
    return set.school_on && l >= 0 ? l : play_toy;
}

static void bell_ring(void)
{
    SC.bell_t0 = plat_ms() | 1u;
    arp_up(P_BELL_STING, 6, 3, 90u, 0.6f);    /* the school bell: three bright notes, in the mood's scale */
    sched_note(380u, P_BELL_STING, (float)mood_degree_midi(7, 0), 0.4f);
    part_burst(PT_SPARKLE, 226.0f, 13.0f, 10, 1.0f, 3.0f, 20, APPLE_RED, 2);
    ui_led_flash_all(120u);
    sticker_give(ST_BELL);
}

static void school_toggle(void)
{
    int play = TOY->flags & TF_LEARN ? TOY->play_toy : cur_toy;
    set.school_on ^= 1u;
    knobs_save_soon();
    if (set.school_on) {
        bell_ring();
        if (school_learn_of(play) >= 0 && cur_toy != school_learn_of(play))
            switch_toy(school_learn_of(play));
    } else {                                 /* recess! */
        SC.recess_t0 = plat_ms() | 1u;
        snd_play(P_SWOOSH, 0.0f, 0.8f);
        chord(P_STAB, 5, 0, 0.55f);
        part_burst(PT_CONFETTI, 120.0f, 80.0f, 36, 2.0f, 7.0f, 40, 0, 4);
        fx_trauma(0.2f);
        if (cur_toy != play)
            switch_toy(play);
    }
}

static void school_direct(int play_toy)
{
    int l = school_learn_of(play_toy);
    if (l < 0 || cur_toy == l)
        return;
    if (!set.school_on) {
        set.school_on = 1;
        knobs_save_soon();
        bell_ring();
    }
    switch_toy(l);
}

static void school_discover(void)
{
    if (set.discover_n >= 3u)
        return;
    set.discover_n++;
    SC.disc_t0 = plat_ms() | 1u;
    part_burst(PT_STAR, 120.0f, 120.0f, 30, 2.0f, 7.0f, 40, 0, 4);
    part_burst(PT_SPARKLE, 120.0f, 120.0f, 24, 1.0f, 5.0f, 30, CC_GOLD_L, 3);
    arp_up(P_CHIME, 6, 5, 60u, 0.5f);
    sticker_give(ST_SPARKLE);
    knobs_save_soon();
}

static void school_boot(void)
{
    SC.boot_ms = plat_ms();
    SC.hatch_press = 0;
}
static int school_hatch(int k1)
{
    if (k1 <= 0 || plat_ms() - SC.boot_ms >= HATCH_MS || !(plat_buttons() >> B_OCTDN & 1u) || SC.hatch_press)
        return 0;
    SC.hatch_press = 1;
    set.preset[cur_toy] = (uint8_t)((set.preset[cur_toy] + 1u) % TOY->preset_count);   /* (OCT-'s step, taken back) */
    set.learn_off ^= 1u;
    SC.hatch_t0 = plat_ms() | 1u;
    if (set.learn_off) {                     /* off: a sleepy two notes down */
        sched_note(0u, P_CHIME, (float)mood_degree_midi(5, 4), 0.5f);
        sched_note(220u, P_CHIME, (float)mood_degree_midi(5, 0), 0.5f);
    } else {
        arp_up(P_BELL_STING, 6, 3, 90u, 0.5f);
    }
    knobs_save_soon();
    return 1;
}
static int school_hatch_used(void) { return SC.hatch_press; }
static void school_boot_press(void) { SC.hatch_press = 0; }   /* a new OCT- press */

static void school_leds(uint32_t *lit_b, uint32_t *mid_b, uint32_t *ns)
{
    uint32_t now = plat_ms(), t;
    if (!set.school_on)
        return;
    if (SC.bell_t0 && now - SC.bell_t0 < 500u)
        *lit_b |= 1u << B_OCTDN;
    else
        *mid_b |= 1u << B_OCTDN;
    t = now % 2600u;                         /* the dim layer rings: ding-ding ... ding-ding */
    *ns = set.quiet ? 700u : 1000u;
    if (t < 260u || (t >= 420u && t < 680u))
        *ns = set.quiet ? 1300u : 2800u;
}

static void draw_apple(float x, float y, float s, int sleepy)
{
    uint16_t red = sleepy ? mix565(APPLE_RED, HEX(0x8A7A90), 150u) : APPLE_RED;
    disc(x - 3.5f * s, y + 0.5f * s, 7.0f * s + 1.0f, U_LINE);
    disc(x + 3.5f * s, y + 0.5f * s, 7.0f * s + 1.0f, U_LINE);
    disc(x - 3.5f * s, y + 0.5f * s, 7.0f * s, red);
    disc(x + 3.5f * s, y + 0.5f * s, 7.0f * s, red);
    disc(x, y + 3.0f * s, 6.5f * s, red);
    cap(x, y - 5.5f * s, x + 1.5f * s, y - 9.5f * s, 1.2f * s, CC_BROWN_D);
    ell(x + 4.5f * s, y - 8.0f * s, 3.5f * s, 1.8f * s, CC_GREEN);
    disc(x - 5.0f * s, y - 2.0f * s, 1.8f * s, lighten(red, 150u));
    if (s >= 1.2f) {                          /* a face, when big enough to see it */
        if (sleepy) {
            cap(x - 4.0f * s, y + 1.0f * s, x - 1.5f * s, y + 1.0f * s, 0.6f * s, CC_INK);
            cap(x + 1.5f * s, y + 1.0f * s, x + 4.0f * s, y + 1.0f * s, 0.6f * s, CC_INK);
        } else {
            disc(x - 2.8f * s, y + 0.5f * s, 1.1f * s, CC_INK);
            disc(x + 2.8f * s, y + 0.5f * s, 1.1f * s, CC_INK);
            arc_dots(x, y + 2.5f * s, 2.5f * s, 0.4f, 2.7f, 0.7f * s, CC_INK);
        }
    }
}

static void school_draw(float T)
{
    uint32_t now = plat_ms();
    float a;
    (void)T;
    dr_ox = dr_oy = 0;
    if (set.school_on) {                     /* the apple in the corner; it wiggles while a challenge is open */
        float w = 0.0f, s = 1.0f;
        if (learn_phase() == LP_DEMO || learn_phase() == LP_WAIT)
            w = 0.25f * fm_sinf((float)now / 90.0f) * (((now / 1200u) & 1u) ? 1.0f : 0.0f);
        if (SC.bell_t0 && now - SC.bell_t0 < 1000u)   /* the Bell: the apple pops in big, then settles */
            s = 1.0f + 1.4f * (1.0f - ease_out_back((float)(now - SC.bell_t0) / 1000.0f)) + 0.6f * ss_pulse((float)(now - SC.bell_t0), 0.0f, 1000.0f);
        draw_apple(226.0f - (s - 1.0f) * 8.0f + 3.0f * w, 14.0f + (s - 1.0f) * 8.0f, s, set.learn_off);
        if (SC.bell_t0 && now - SC.bell_t0 < 1000u) {   /* and a little bell swings beside it */
            float sw = 0.5f * fm_sinf((float)(now - SC.bell_t0) / 70.0f) * (1.0f - (float)(now - SC.bell_t0) / 1000.0f);
            float bx = 196.0f, by = 20.0f;
            tri(bx - 7.0f + sw * 6.0f, by + 6.0f, bx + 7.0f + sw * 6.0f, by + 6.0f, bx, by - 8.0f, CC_GOLD);
            disc(bx, by - 3.0f, 5.0f, CC_GOLD);
            disc(bx + sw * 8.0f, by + 8.0f, 2.2f, CC_GOLD_D);
        }
    }
    if (SC.recess_t0 && now - SC.recess_t0 < 900u) {   /* recess: the apple flies off the top, spinning */
        a = (float)(now - SC.recess_t0) / 900.0f;
        draw_apple(226.0f - 40.0f * a, 14.0f - 40.0f * ease_in_quad(a) + 20.0f * fm_sinf(a * 3.14f), 1.0f + a, 0);
    }
    if (SC.disc_t0 && now - SC.disc_t0 < 1400u) {   /* a long press found: a ring of stars sweeps out */
        int i;
        a = (float)(now - SC.disc_t0) / 1400.0f;
        for (i = 0; i < 12; i++) {
            float an = (float)i * 0.5236f + a * 4.0f, r = 20.0f + 120.0f * ease_out_cubic(a);
            star4(120.0f + r * fm_cosf(an), 120.0f + r * fm_sinf(an), 8.0f * (1.0f - a) + 2.0f, RAINBOW[i % 6]);
        }
    }
    if (SC.hatch_t0 && now - SC.hatch_t0 < 1500u) {   /* the parent's switch: a sleepy apple, or a bright one */
        a = (float)(now - SC.hatch_t0);
        draw_apple(120.0f, 110.0f, 4.0f * ease_out_back(a / 300.0f) * (a > 1200.0f ? 1.0f - (a - 1200.0f) / 300.0f : 1.0f) + 0.01f,
                   set.learn_off);
    }
}
