// Copyright (C) 2023 @flowchartsman
// SPDX-License-Identifier: GPL-2.0-or-later
// Adapted from MatrixFireFast by @toggledbits

#ifdef ENABLE_RGB_MATRIX_PIXEL_FLAME
RGB_MATRIX_EFFECT(PIXEL_FLAME)
#    ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

uint8_t pix[MATRIX_ROWS][MATRIX_COLS];

const uint32_t colors[] = {0x000000, 0x100000, 0x300000, 0x600000, 0x800000, 0xA00000, 0xC02000, 0xC04000, 0xC06000, 0xC08000, 0x807080};

// TODO: set dynamically from chosen list
const uint8_t NCOLORS = (sizeof(colors) / sizeof(colors[0]));

/* Flare constants */
#        define MAX_FLARES 8
#        define FLARE_ROWS 3
#        define FLARE_CHANCE 50
/* decay rate of flare radiation; 14 is good */
#        define FLARE_DECAY 14

/* Flare variables*/
uint8_t  nflare = 0;
uint32_t flare[MAX_FLARES];

uint32_t isqrt(uint32_t n) {
    if (n < 2) return n;
    uint32_t smallCandidate = isqrt(n >> 2) << 1;
    uint32_t largeCandidate = smallCandidate + 1;
    return (largeCandidate * largeCandidate > n) ? smallCandidate : largeCandidate;
}

// Set pixels to intensity around flare
void glow(int x, int y, int z) {
    int b = z * 10 / FLARE_DECAY + 1;
    for (int i = (y - b); i < (y + b); ++i) {
        for (int j = (x - b); j < (x + b); ++j) {
            if (i >= 0 && j >= 0 && i < MATRIX_ROWS && j < MATRIX_COLS) {
                int     d = (FLARE_DECAY * isqrt((x - j) * (x - j) + (y - i) * (y - i)) + 5) / 10;
                uint8_t n = 0;
                if (z > d) n = z - d;
                if (n > pix[i][j]) { // can only get brighter
                    pix[i][j] = n;
                }
            }
        }
    }
}

/* Fire functions*/
void newflare(void) {
    if (nflare < MAX_FLARES && rand() % FLARE_CHANCE == 0) {
        int x           = rand() % MATRIX_COLS;
        int y           = rand() % FLARE_ROWS;
        int z           = NCOLORS - 1;
        flare[nflare++] = (z << 16) | (y << 8) | (x & 0xff);
        glow(x, y, z);
    }
}

void make_fire(void) {
    uint16_t i, j;

    // First, move all existing heat points up the display and fade
    for (i = MATRIX_ROWS - 1; i > 0; --i) {
        for (j = 0; j < MATRIX_COLS; ++j) {
            uint8_t n = 0;
            if (pix[i - 1][j] > 0) n = pix[i - 1][j] - 1;
            pix[i][j] = n;
        }
    }

    // Heat the bottom row
    for (j = 0; j < MATRIX_COLS; ++j) {
        i = pix[0][j];
        if (i > 0) {
            pix[0][j] = random() % (NCOLORS - 4) + 2;
        }
    }

    // flare
    for (i = 0; i < nflare; ++i) {
        int x = flare[i] & 0xff;
        int y = (flare[i] >> 8) & 0xff;
        int z = (flare[i] >> 16) & 0xff;
        glow(x, y, z);
        if (z > 1) {
            flare[i] = (flare[i] & 0xffff) | ((z - 1) << 16);
        } else {
            // This flare is out
            for (int j = i + 1; j < nflare; ++j) {
                flare[j - 1] = flare[j];
            }
            --nflare;
        }
    }
    newflare();
}

// const uint8_t NCOLORS = (sizeof(colors)/sizeof(colorsO[0]));
// const uint32_t colorsO[] = {
//   0x000000,
//   0x001000,
//   0x003000,
//   0x006000,
//   0x008000,
//   0x00A000,
//   0x20C000,
//   0x40C000,
//   0x60C000,
//   0x80C000,
//   0x807080
// };
// const uint32_t colorsGS[] = {
//   0x000000,
//   0x2C0000,
//   0x410200,
//   0x610300,
//   0xDB0001,
//   0xFF2E01,
//   0xFFA202,
//   0xF6FD7D,
//   0xF0F0FD,
//   0x0909BD,
//   0x001E64
// };

static bool PIXEL_FLAME(effect_params_t* params) {
    const uint8_t  drop_ticks = 20;
    static uint8_t drop       = 0;

    if (params->init) {
        for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
            for (uint8_t c = 0; c < MATRIX_COLS; c++) {
                pix[r][c] = 0;
            }
        }
        drop = 0;
        rgb_matrix_set_color_all(0, 0, 0);
    }

    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    if (++drop > drop_ticks) {
        drop = 0;
        make_fire();
        for (uint8_t i = 0; i < MATRIX_ROWS; ++i) {
            for (uint8_t j = 0; j < MATRIX_COLS; ++j) {
                // bit shift
                rgb_matrix_set_color(g_led_config.matrix_co[MATRIX_ROWS - i][j], (colors[pix[i][j]] >> 16) & 0xFF, (colors[pix[i][j]] >> 8) & 0xFF, (colors[pix[i][j]]) & 0xFF);
            }
        }
    }

    return rgb_matrix_check_finished_leds(led_max);
}
#    endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS
#endif     // ENABLE_RGB_MATRIX_PIXEL_FLAME
