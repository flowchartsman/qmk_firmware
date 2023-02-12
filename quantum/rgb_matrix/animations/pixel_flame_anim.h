// Copyright (C) 2023 @flowchartsman
// Copyright (C) 2020 @toggledbits
// SPDX-License-Identifier: GPL-2.0-or-later
// Adapted from MatrixFireFast

#ifdef ENABLE_RGB_MATRIX_PIXEL_FLAME
RGB_MATRIX_EFFECT(PIXEL_FLAME)
#    ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS

// Palettes
#        if defined(PIXEL_FLAME_GREEN)
const uint32_t colors[] = {0x000000, 0x0C1805, 0x10290b, 0x194711, 0x316527, 0x4A843D, 0x7BC269, 0x387D29, 0x377C28, 0x549369, 0x224451};
#        elif defined(PIXEL_FLAME_GAS)
const uint32_t colors[] = {0x000000, 0x2C0000, 0x410200, 0x610300, 0xDB0001, 0xFF2E01, 0xFFA202, 0xF6FD7D, 0xF0F0FD, 0x0909BD, 0x001E64};
#            const uint32_t colors[] = {0x000000, 0x300000, 0x800000, 0xA00000, 0xC04000, 0xC06000, 0xC08000, 0x807080 };
// const uint32_t colors[] = {0x000000, 0x100000, 0x300000, 0x600000, 0x800000, 0xA00000, 0xC02000, 0xC04000, 0xC06000, 0xC08000, 0x807080};
#        endif

const uint8_t N_COLORS = ARRAY_SIZE(colors);
// const uint8_t MAX_DECAY = MATRIX_ROWS / N_COLORS with fuzz

/* Flare constants */
#        define MAX_FLARES 8
#        define FLARE_ROWS 1
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
                if (n > g_rgb_frame_buffer[i][j]) { // can only get brighter
                    g_rgb_frame_buffer[i][j] = n;
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
        int z           = N_COLORS - 1;
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
            if (g_rgb_frame_buffer[i - 1][j] > 0) n = g_rgb_frame_buffer[i - 1][j] - 1;
            g_rgb_frame_buffer[i][j] = n;
        }
    }

    // Heat the bottom row
    for (j = 0; j < MATRIX_COLS; ++j) {
        i = g_rgb_frame_buffer[0][j];
        if (i > 0) {
            g_rgb_frame_buffer[0][j] = random8_min_max(N_COLORS - 6, N_COLORS - 2);
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

static bool PIXEL_FLAME(effect_params_t* params) {
    const uint8_t  drop_ticks = 16;
    static uint8_t drop       = 0;

    if (params->init) {
        rgb_matrix_set_color_all(0, 0, 0);
        memset(g_rgb_frame_buffer, 0, sizeof(g_rgb_frame_buffer));
        drop = 0;
    }

    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    if (++drop > drop_ticks) {
        drop = 0;
        make_fire();
        for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
            for (uint8_t col = 0; col < MATRIX_COLS; col++) {
                uint8_t leds[LED_HITS_TO_REMEMBER];
                uint8_t led_count = rgb_matrix_map_row_column_to_led(MATRIX_ROWS - row - 1, col, leds);
                if (led_count > LED_HITS_TO_REMEMBER) {
                    led_count = LED_HITS_TO_REMEMBER;
                }
                for (uint8_t led_idx = 0; led_idx < led_count; led_idx++) {
                    rgb_matrix_set_color(leds[led_idx], (colors[g_rgb_frame_buffer[row][col]] >> 16) & 0xFF, (colors[g_rgb_frame_buffer[row][col]] >> 8) & 0xFF, (colors[g_rgb_frame_buffer[row][col]]) & 0xFF);
                }
            }
        }
    }

    return rgb_matrix_check_finished_leds(led_max);
}
#    endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS
#endif     // ENABLE_RGB_MATRIX_PIXEL_FLAME
