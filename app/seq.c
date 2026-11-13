/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: Poop Parade's sequencer (seq.h). steps_advance and steps_fire run in the audio render. */

/* the four kits (K6): v2's drum patches exactly, with v2's kit deltas (patch.c KITS); the cartoon kit's are pitched */
static const uint8_t PARADE_KIT[4][NLANES] = {
    {P_KICK, P_SNARE, P_CHAT},                /* acoustic: Drum Jam's classic kit */
    {P_KICK, P_CLAP, P_CHAT},                 /* "808": the big room's low, long kick and a clap */
    {P_LTOM, P_COWBELL, P_SHAKER},            /* pots and pans: the junk box */
    {P_PLOP_BOING, P_HONK, P_POP}};           /* cartoon */
static const uint8_t PARADE_KITX[4] = {0, 1, 3, 0};

static struct {
    volatile uint8_t on, ncols, kit, start, stop;
    volatile float bpm, swing;
    volatile float pitch[NLANES];
    volatile uint32_t col;
    volatile float samp, len;
    struct { uint8_t col, mask; } ev[16];
    volatile uint32_t ev_w, ev_r;
} stp = {.ncols = 8, .bpm = 90.0f, .pitch = {48.0f, 60.0f, 72.0f}, .len = 6000.0f};

static void steps_fire(uint32_t col)
{
    const kit_t *kit = &KITS[PARADE_KITX[stp.kit & 3u]];
    uint32_t l, mask = 0, w;
    for (l = 0; l < NLANES; l++) {
        if (!(steps_lane[l] >> col & 1u))
            continue;
        {
            int pa = PARADE_KIT[stp.kit & 3u][l];
            note_t n = snd_defaults(pa, 0.0f, l == LANE_KICK ? 0.95f : l == LANE_SNARE ? 0.85f : 0.7f);
            n.midi = PATCHES[pa].base_hz ? (float)kit->tune : stp.pitch[l];
            n.tscale = (float)kit->decay * 0.1f;
            n.crush = (float)kit->crush * 0.01f;
            n.send = (float)kit->delay * 0.01f;
            voice_start(&n);
        }
        mask |= 1u << l;
    }
    w = stp.ev_w;
    if (w - stp.ev_r < 16u) {
        stp.ev[w % 16u].col = (uint8_t)col;
        stp.ev[w % 16u].mask = (uint8_t)mask;
        __asm__ volatile("" ::: "memory");
        stp.ev_w = w + 1u;
    }
}

static void steps_advance(uint32_t m)
{
    float L, sw;
    if (stp.stop) {
        stp.stop = 0;
        stp.on = 0;
    }
    if (stp.start) {                          /* the first column at once: the press is the downbeat */
        stp.start = 0;
        stp.on = 1;
        stp.col = 0;
        stp.samp = 0.0f;
        steps_fire(0);
    }
    if (!stp.on)
        return;
    L = 60.0f / fm_clampf(stp.bpm, 40.0f, 240.0f) / 2.0f * AFS;   /* eighth notes: 8 columns a bar */
    sw = fm_clampf(stp.swing, 0.0f, 0.6f) * 0.5f;
    stp.len = (stp.col & 1u) ? L * (1.0f - sw) : L * (1.0f + sw);
    stp.samp += (float)m;
    if (stp.samp >= stp.len) {
        stp.samp -= stp.len;
        stp.col = (stp.col + 1u) % (stp.ncols ? stp.ncols : 8u);
        steps_fire(stp.col);
    }
}

/* the main side */
static void steps_config(float bpm, float swing, int kit, int ncols)
{
    stp.bpm = bpm;
    stp.swing = swing;
    stp.kit = (uint8_t)(kit & 3);
    stp.ncols = (uint8_t)(ncols == 16 ? 16 : 8);
}
static void steps_set_pitch(int lane, float midi)
{
    if (lane >= 0 && lane < NLANES)
        stp.pitch[lane] = midi;
}
static void steps_play(int on)
{
    if (on) {
        stp.ev_r = stp.ev_w;                  /* (anything the screen has not taken is old) */
        stp.start = 1;
    } else {
        stp.stop = 1;
    }
}
static int steps_playing(void) { return stp.on || stp.start; }
static float steps_pos(void)
{
    if (!stp.on)
        return 0.0f;
    return (float)stp.col + fm_clampf(stp.samp / fm_maxf(stp.len, 1.0f), 0.0f, 0.999f);
}
static int steps_pop(uint8_t *col, uint8_t *mask)
{
    uint32_t r = stp.ev_r;
    if (r == stp.ev_w)
        return 0;
    *col = stp.ev[r % 16u].col;
    *mask = stp.ev[r % 16u].mask;
    __asm__ volatile("" ::: "memory");
    stp.ev_r = r + 1u;
    return 1;
}
