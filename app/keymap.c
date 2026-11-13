/* SPDX-License-Identifier: MPL-2.0
 * Spooky Stable v3: the keymaps (keymap.h). */

static const uint8_t KM_WHITE[16] = PLAT_WHITE_KEYS;
static const uint8_t KM_BLACK[11] = PLAT_BLACK_KEYS;

static int km_black(uint32_t k)
{
    int i;
    for (i = 0; i < 11; i++)
        if (KM_BLACK[i] == k)
            return 1;
    return 0;
}
static uint8_t km_x2(uint32_t k)
{
    int i;
    for (i = 0; i < 16; i++) {
        if (KM_WHITE[i] == k)
            return (uint8_t)(2 * i);
        if (i < 15 && KM_WHITE[i] < k && KM_WHITE[i + 1] > k)
            return (uint8_t)(2 * i + 1);          /* a black key sits between its two white keys */
    }
    return 30;
}

static void km_set(uint32_t k, int vrow, int vcol, int role)
{
    km[k].phys_row = (uint8_t)km_black(k);
    km[k].phys_col = km_x2(k);
    km[k].vrow = (uint8_t)vrow;
    km[k].vcol = (uint8_t)vcol;
    km[k].role = (uint8_t)role;
}

static void km_parade(void)
{
    uint32_t k;
    for (k = 0; k < NKEYS; k++) {
        if (k < 24u)                              /* lane 0 kick (low third), 1 snare, 2 hat; columns left to right */
            km_set(k, (int)(k / 8u), (int)(k % 8u), KR_STEP);
        else                                      /* and the three utility keys, side by side at the top */
            km_set(k, 0, (int)(k - 24u), k == 24u ? KR_PLAY : k == 25u ? KR_CLEAR : KR_CHALLENGE);
    }
}

static void km_mixup(void)
{
    uint32_t k;
    for (k = 0; k < NKEYS; k++)
        km_set(k, 0, k < 14u ? (int)k : (int)(k - 14u), k < 14u ? KR_LEFT : KR_RIGHT);
}

static void km_cubbies(int cols)
{
    uint32_t k;
    if (cols < 1)
        cols = 1;
    for (k = 0; k < NKEYS; k++) {                 /* (31 half-keys wide; a column each 31 / cols of them) */
        int c = (int)km_x2(k) * cols / 31;
        km_set(k, km_black(k) ? 0 : 1, c < cols ? c : cols - 1, KR_CUBBY);
    }
}

static void km_zones(int n)
{
    uint32_t k;
    if (n < 1)
        n = 1;
    for (k = 0; k < NKEYS; k++) {
        int z = (int)km_x2(k) * n / 31;
        km_set(k, 0, (z < n ? z : n - 1) + 1, KR_ANSWER);
    }
}

static void km_force(void)
{
    uint32_t k;
    for (k = 0; k < NKEYS; k++)
        km_set(k, 0, (int)k, KR_FORCE);
}

static int km_find(int role, int vrow, int vcol)
{
    uint32_t k;
    for (k = 0; k < NKEYS; k++)
        if (km[k].role == role && km[k].vrow == vrow && km[k].vcol == vcol)
            return (int)k;
    return -1;
}
