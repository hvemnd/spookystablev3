/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: Fart Mix-Up's FM (fm2.h). */

static struct {
    uint16_t hl, hr, hc;                      /* the left voice, the right one, the cloud pad */
    uint8_t lon, ron;
    float lm, rm;
    float vl, vr;                             /* their velocities */
    float s_ratio, s_fmi, s_fbx, s_vr;        /* what was last sent (sent again only when it moves) */
} F2;

static int fm2_left_on(void) { return F2.lon; }
static int fm2_right_on(void) { return F2.ron; }
static float fm2_left_midi(void) { return F2.lm; }
static float fm2_right_midi(void) { return F2.rm; }

static void fm2_cloud_pad(void)
{
    if ((F2.lon || F2.ron) && !F2.hc) {
        note_t n = snd_defaults(P_MOD_CLOUD, (F2.lon ? F2.lm : F2.rm) + 12.0f, 0.5f);
        n.flags = NF_HOLD;
        F2.hc = snd_note(&n);
    } else if (!F2.lon && !F2.ron && F2.hc) {   /* both let go: the cloud rings on and fades (its long release) */
        snd_off(F2.hc);
        F2.hc = 0;
    }
}

static void fm2_left(int on, float midi, float vel, int patch)
{
    if (on) {
        if (F2.lon) {
            fm2_glide(0, midi);
            return;
        }
        {
            note_t n = snd_defaults(patch, midi, vel);
            n.flags = NF_HOLD;
            F2.hl = snd_note(&n);
        }
        F2.lon = 1;
        F2.lm = midi;
        F2.vl = vel;
        F2.s_ratio = F2.s_fmi = F2.s_fbx = -1.0f;   /* (send its FM at the next update) */
    } else if (F2.lon) {
        snd_off(F2.hl);
        F2.hl = 0;
        F2.lon = 0;
    }
    fm2_cloud_pad();
}
static void fm2_right(int on, float midi, float vel, int patch)
{
    if (on) {
        if (F2.ron) {
            fm2_glide(1, midi);
            return;
        }
        {
            note_t n = snd_defaults(patch, midi, vel);
            n.flags = NF_HOLD;
            F2.hr = snd_note(&n);
        }
        F2.ron = 1;
        F2.rm = midi;
        F2.vr = vel;
        F2.s_vr = -1.0f;
    } else if (F2.ron) {
        snd_off(F2.hr);
        F2.hr = 0;
        F2.ron = 0;
    }
    fm2_cloud_pad();
}
static void fm2_glide(int right, float midi)
{
    if (right && F2.ron && ss_absf(midi - F2.rm) > 0.01f) {
        F2.rm = midi;
        snd_glide(F2.hr, midi);
    } else if (!right && F2.lon && ss_absf(midi - F2.lm) > 0.01f) {
        F2.lm = midi;
        snd_glide(F2.hl, midi);
    }
}

static void fm2_update(float mix, float chaos)
{
    float ratio, fmi, fbx, vr;
    if (F2.lon) {                             /* the carrier: modulated by the right one's pitch while it sounds */
        ratio = F2.ron ? fm_exp2f((F2.rm - F2.lm) / 12.0f) : 1.0f;
        fmi = F2.ron ? mix * 170.0f + chaos * 50.0f : chaos * 20.0f;
        fbx = chaos * 70.0f;
        if (ss_absf(ratio - F2.s_ratio) > 0.002f || ss_absf(fmi - F2.s_fmi) > 1.0f || ss_absf(fbx - F2.s_fbx) > 1.0f) {
            snd_fm(F2.hl, ratio, fmi, fbx);
            F2.s_ratio = ratio;
            F2.s_fmi = fmi;
            F2.s_fbx = fbx;
        }
    }
    if (F2.ron) {                             /* the right one fades into the left one's timbre as the mix turns */
        vr = F2.vr * (F2.lon ? 1.0f - 0.92f * mix : 1.0f);
        if (ss_absf(vr - F2.s_vr) > 0.01f) {
            snd_vel(F2.hr, vr);
            F2.s_vr = vr;
        }
    }
}

static void fm2_all_off(void)
{
    fm2_left(0, 0.0f, 0.0f, 0);
    fm2_right(0, 0.0f, 0.0f, 0);
    if (F2.hc)
        snd_off(F2.hc);
    F2.hc = 0;
}

static int fm2_class(float st)
{
    int d = (int)(ss_absf(st) + 0.5f) % 12;
    if (d == 0 || d == 7 || d == 5)
        return CL_HAPPY;                      /* unison and octaves, fifths, fourths: the cloud smiles */
    if (d == 3 || d == 4 || d == 8 || d == 9)
        return CL_OK;                         /* thirds and sixths: content */
    return CL_CROSS;                          /* seconds, the tritone, sevenths: cross-eyed, and it sneezes confetti */
}
static int fm2_cloud(void) { return F2.lon && F2.ron ? fm2_class(F2.rm - F2.lm) : CL_NONE; }
