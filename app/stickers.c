/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: the Sticker Book (stickers.h). */

static void app_wardrobe_gift(void);         /* app.c: v2's unlock, one item sooner */
static void draw_poop(float x, float y, float sc, float sx, float sy, int style, int32_t hue, float T);   /* toys/poop.c */
static void draw_booger(float x, float y, float r, float sx, float sy, int style, float T);              /* toys/booger.c */
static void draw_apple(float x, float y, float s, int sleepy);                                          /* school.c */

static struct {
    uint8_t open, cur;
    uint32_t open_t0, touch_t0, toast_t0, last_key;
    uint8_t toast_id;
} BK;

static int sticker_has(int id) { return id >= 0 && id < NSTICKERS && (set.stickers >> id & 1u); }
static int sticker_count(void)
{
    int i, n = 0;
    for (i = 0; i < NSTICKERS; i++)
        n += sticker_has(i);
    return n;
}

static void sticker_give(int id)
{
    if (id < 0 || id >= NSTICKERS || sticker_has(id))
        return;
    set.stickers |= 1u << id;
    BK.toast_t0 = plat_ms() | 1u;
    BK.toast_id = (uint8_t)id;
    sched_note(150u, P_CHIME, (float)mood_degree_midi(6, 4), 0.5f);
    sched_note(260u, P_CHIME, (float)mood_degree_midi(7, 0), 0.5f);
    if (sticker_count() % 4 == 0)              /* the stickers feed the wardrobe (design 9.3): every fourth */
        app_wardrobe_gift();
    if ((set.stickers & ((1u << ST_PARADE) | (1u << ST_TWO_FARTS) | (1u << ST_TOP_RUNG) | (1u << ST_MATCH))) ==
        ((1u << ST_PARADE) | (1u << ST_TWO_FARTS) | (1u << ST_TOP_RUNG) | (1u << ST_MATCH)))
        set.stickers |= 1u << ST_ALL_SCHOOL;   /* (a quiet bonus: all four learning toys played) */
    knobs_save_soon();
}

static void book_open(void)
{
    BK.open = 1;
    BK.open_t0 = plat_ms();
    BK.touch_t0 = 0;
    BK.last_key = plat_ms();
    BK.cur = 0;
    snd_play(P_SWOOSH, 0.0f, 0.6f);
    arp_up(P_CHIME, 6, 3, 90u, 0.45f);
}
static void book_close(void)
{
    if (!BK.open)
        return;
    BK.open = 0;
    snd_play(P_POP, (float)mood_degree_midi(5, 0), 0.5f);
}
static int book_is_open(void)
{
    if (BK.open && plat_ms() - BK.last_key > 30000u)   /* forgotten: it closes itself */
        BK.open = 0;
    return BK.open;
}

static void book_key(uint32_t k)
{
    static const uint8_t SP[NSTICKERS] = {P_PLOP_MARIMBA, P_KICK, P_PLOP_WOOD, P_POP, P_PLOP_PIZZ, P_STAB,
                                          P_FART_KAZOO, P_CHIME, P_CHIME, P_FART_DIDGE, P_PEE_OCARINA, P_STAB,
                                          P_PEE_FLUTE, P_PEE_PANPIPE, P_PLOP_DROP, P_STAB, P_FLICK_TWANG, P_POP,
                                          P_SLURP, P_SPLAT, P_STAB, P_BELL_STING, P_CHIME, P_GIGGLE};
    int id = (int)(k < NSTICKERS ? k : (uint32_t)NSTICKERS - 1u);
    BK.cur = (uint8_t)id;
    BK.touch_t0 = plat_ms() | 1u;
    BK.last_key = plat_ms();
    if (sticker_has(id)) {
        snd_play(SP[id], (float)mood_degree_midi(id < 12 ? 4 : 5, id % 7), 0.7f);
        part_burst(PT_SPARKLE, 30.0f + (float)(id % 6) * 36.0f, 52.0f + (float)(id / 6) * 46.0f, 8, 1.0f, 3.0f, 16, CC_GOLD_L, 2);
    } else {
        snd_play(P_TICK, (float)mood_degree_midi(5, id % 5), 0.4f);   /* an empty place: a soft tick */
    }
    ui_led_key(k);
}

/* a sticker: a white die-cut disc with a soft outline and its picture */
static void draw_sticker(int id, float x, float y, float s, float T)
{
    int i;
    uint16_t a = mood->accent;
    disc(x, y, 17.0f * s + 1.5f, U_LINE);
    disc(x, y, 17.0f * s, U_WHITE);
    switch (id) {
    case ST_FIRST_POOP: draw_poop(x, y + 9.0f * s, 0.75f * s, 1.0f, 1.0f, 0, -1, T); break;
    case ST_PARADE:                            /* a drum */
        ell(x, y + 6.0f * s, 11.0f * s, 4.0f * s, darken(a, 60u));
        box(x - 11.0f * s, y - 3.0f * s, 22.0f * s, 9.0f * s, darken(a, 60u));
        ell(x, y - 3.0f * s, 11.0f * s, 4.0f * s, U_BONE);
        break;
    case ST_COPY:                              /* two poops, one and its echo */
        draw_poop(x - 6.0f * s, y + 8.0f * s, 0.5f * s, 1.0f, 1.0f, 0, -1, T);
        draw_poop(x + 7.0f * s, y + 8.0f * s, 0.5f * s, 1.0f, 1.0f, 1, -1, T);
        break;
    case ST_COUNT: draw_dots(x, y, 3, 2.6f * s, 3, a, a); break;
    case ST_LEFTRIGHT:                         /* an arrow, left to right */
        cap(x - 10.0f * s, y, x + 6.0f * s, y, 2.5f * s, a);
        tri(x + 3.0f * s, y - 7.0f * s, x + 3.0f * s, y + 7.0f * s, x + 12.0f * s, y, a);
        break;
    case ST_PARADE_STAR:
    case ST_MIXUP_STAR:
    case ST_PIANO_STAR:
    case ST_WALL_STAR:                         /* a toy's mastery: a golden star with its colour */
        star4(x, y, 14.0f * s, CC_GOLD);
        star4(x, y, 7.0f * s, id == ST_PARADE_STAR ? CC_BROWN : id == ST_MIXUP_STAR ? CC_PINK : id == ST_PIANO_STAR ? CC_PEE : CC_SNOT);
        break;
    case ST_TWO_FARTS:                         /* a warm cloud and a cool one */
        disc(x - 5.0f * s, y, 8.0f * s, CC_ORANGE);
        disc(x + 5.0f * s, y, 8.0f * s, CC_BLUE);
        disc(x, y, 4.0f * s, CC_GREEN);
        break;
    case ST_OCTAVE:
    case ST_FIFTH:                             /* a happy cloud (the fifth's has a star) */
        disc(x - 6.0f * s, y + 2.0f * s, 7.0f * s, lighten(a, 160u));
        disc(x + 6.0f * s, y + 2.0f * s, 7.0f * s, lighten(a, 160u));
        disc(x, y - 3.0f * s, 8.0f * s, lighten(a, 160u));
        disc(x - 3.0f * s, y, 1.4f * s, CC_INK);
        disc(x + 3.0f * s, y, 1.4f * s, CC_INK);
        arc_dots(x, y + 2.0f * s, 3.5f * s, 0.4f, 2.7f, 1.2f * s, CC_INK);
        if (id == ST_FIFTH)
            star4(x + 9.0f * s, y - 9.0f * s, 4.0f * s, CC_GOLD);
        break;
    case ST_FM:                                /* a zigzag ribbon */
        for (i = 0; i < 5; i++)
            cap(x - 11.0f * s + (float)i * 5.0f * s, y + (i & 1 ? 5.0f : -5.0f) * s, x - 6.0f * s + (float)i * 5.0f * s,
                y + (i & 1 ? -5.0f : 5.0f) * s, 1.6f * s, hue565(i * 300));
        break;
    case ST_INTERVAL:                          /* two notes on two rungs */
    case ST_TOP_RUNG:
        for (i = 0; i < 3; i++)
            box(x - 9.0f * s, y - 9.0f * s + (float)i * 8.0f * s, 18.0f * s, 2.0f * s, CC_BROWN);
        disc(x - 4.0f * s, y + (id == ST_TOP_RUNG ? -9.0f : 7.0f) * s, 3.0f * s, id == ST_TOP_RUNG ? CC_PEE : CC_ORANGE);
        disc(x + 4.0f * s, y - 9.0f * s, 3.0f * s, id == ST_TOP_RUNG ? CC_PEE : CC_BLUE);
        break;
    case ST_TUNE:                              /* a note */
        ell(x - 3.0f * s, y + 6.0f * s, 4.5f * s, 3.5f * s, a);
        box(x + 0.5f * s, y - 9.0f * s, 2.0f * s, 15.0f * s, a);
        tri(x + 2.5f * s, y - 9.0f * s, x + 2.5f * s, y - 3.0f * s, x + 9.0f * s, y - 4.0f * s, a);
        break;
    case ST_BUCKET:
        quad(x - 9.0f * s, y - 7.0f * s, x + 9.0f * s, y - 7.0f * s, x + 7.0f * s, y + 9.0f * s, x - 7.0f * s, y + 9.0f * s, HEX(0x94A3B8));
        box(x - 8.0f * s, y - 1.0f * s, 16.0f * s, 9.0f * s, CC_PEE);
        box(x - 10.0f * s, y - 2.0f * s, 20.0f * s, 1.2f * s, CC_PINK);
        break;
    case ST_MATCH:
        draw_booger(x - 5.0f * s, y, 5.0f * s, 1.0f, 1.0f, 0, T);
        draw_booger(x + 6.0f * s, y, 5.0f * s, 1.0f, 1.0f, 0, T);
        break;
    case ST_COUNT_BOOGERS: draw_dots(x, y, 5, 2.2f * s, 5, CC_SNOT, CC_SNOT); break;
    case ST_SORT:                              /* a hungry mouth */
        ell(x, y, 11.0f * s, 8.0f * s, CC_INK);
        ell(x, y + 4.0f * s, 6.0f * s, 3.0f * s, CC_PINK);
        disc(x + 1.0f * s, y - 12.0f * s, 3.5f * s, CC_SNOT);
        break;
    case ST_BIG_WALL:                          /* a wall of cubbies */
        for (i = 0; i < 6; i++)
            box(x - 11.0f * s + (float)(i % 3) * 8.0f * s, y - 7.0f * s + (float)(i / 3) * 8.0f * s, 6.0f * s, 6.0f * s,
                i == 4 ? CC_SNOT : mix565(mood->prop1, U_WHITE, 120u));
        break;
    case ST_BELL:                              /* the school bell */
        tri(x - 9.0f * s, y + 6.0f * s, x + 9.0f * s, y + 6.0f * s, x, y - 10.0f * s, CC_GOLD);
        disc(x, y - 4.0f * s, 6.0f * s, CC_GOLD);
        disc(x, y + 8.0f * s, 2.5f * s, CC_GOLD_D);
        break;
    case ST_SPARKLE:
        star4(x, y, 13.0f * s, hue565((int32_t)(T / 4.0f)));
        star4(x + 7.0f * s, y - 7.0f * s, 4.0f * s, CC_GOLD_L);
        break;
    default:                                   /* ST_ALL_SCHOOL: the apple with a heart */
        draw_apple(x, y + 1.0f * s, 1.3f * s, 0);
        heart(x + 8.0f * s, y - 8.0f * s, 0.5f * s, CC_PINK);
        break;
    }
}

static void book_draw(float T)
{
    uint32_t now = plat_ms();
    float open = ease_out_back(fm_minf(1.0f, (float)(now - BK.open_t0) / 300.0f));
    int i;
    if (!book_is_open())
        return;
    dr_ox = dr_oy = 0;
    /* the page: a warm paper, a spine, the apple at the top */
    box(0.0f, 240.0f * (1.0f - open), 240.0f, 240.0f, HEX(0xFFF4DC));
    if (open < 0.98f)
        return;
    box(0.0f, 0.0f, 240.0f, 22.0f, mix565(mood->accent, U_WHITE, 120u));
    draw_apple(120.0f, 11.0f, 1.0f, 0);
    for (i = 0; i < NSTICKERS; i++) {
        float x = 30.0f + (float)(i % 6) * 36.0f, y = 52.0f + (float)(i / 6) * 46.0f, w = 0.0f;
        int d = i - (int)BK.cur, dd = d < 0 ? -d : d;
        if (BK.touch_t0 && now - BK.touch_t0 < 900u && dd <= 2) {   /* the one touched wiggles, its neighbours a little */
            float a = (float)(now - BK.touch_t0) / 900.0f;
            w = (dd == 0 ? 1.0f : 0.35f) * (1.0f - a) * fm_sinf(a * 30.0f);
        }
        if (sticker_has(i)) {
            draw_sticker(i, x + 3.0f * w, y - (dd == 0 && BK.touch_t0 && now - BK.touch_t0 < 300u ? 4.0f : 0.0f),
                         1.0f + 0.12f * ss_absf(w), T);
        } else {                               /* still to find: a soft dotted place */
            ring_a(x, y, 16.0f, 16.0f, 1.5f, HEX(0xE0D2B4), 256u);
            disc(x, y, 3.0f + ss_absf(w) * 3.0f, HEX(0xE8DCC0));
        }
    }
}

static void sticker_toast_draw(float T)        /* a new sticker: it pops up big, wiggles, then flies to the apple */
{
    uint32_t now = plat_ms(), a;
    float s, x, y;
    if (!BK.toast_t0 || BK.open)
        return;
    a = now - BK.toast_t0;
    if (a >= 1300u) {
        BK.toast_t0 = 0;
        return;
    }
    dr_ox = dr_oy = 0;
    if (a < 850u) {                           /* (small, high, quick: never over what she is doing for long) */
        s = ease_out_elastic((float)a / 450.0f) * 1.05f;
        x = 196.0f + 2.0f * fm_sinf((float)a / 40.0f) * (a < 450u ? 1.0f : 0.0f);
        y = 44.0f;
    } else {
        float u = ease_in_quad((float)(a - 850u) / 450.0f);
        s = 1.05f - 0.75f * u;
        x = ss_lerp(196.0f, 226.0f, u);
        y = ss_lerp(44.0f, 13.0f, u);
    }
    glow(x, y, 30.0f * s, scale565(CC_GOLD, 120u), 180u);
    draw_sticker(BK.toast_id, x, y, s, T);
}
