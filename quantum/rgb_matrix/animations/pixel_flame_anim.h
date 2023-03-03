// Copyright (C) 2023 @flowchartsman

#include "print.h"

#ifdef ENABLE_RGB_MATRIX_PIXEL_FLAME
RGB_MATRIX_EFFECT(PIXEL_FLAME)
#    ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS
#        if MATRIX_ROWS < 6
#            define PALETTE_SIZE 8
#        else
#            define PALETTE_SIZE MATRIX_ROWS + 3
#        endif
#        ifndef RGB_MATRIX_PIXEL_FLAME_DELAY_MS
#            define RGB_MATRIX_PIXEL_FLAME_DELAY_MS 60
#        endif
RGB hcolors[PALETTE_SIZE];

uint8_t     user_hue;
led_point_t vpos[RGB_MATRIX_LED_COUNT];
uint8_t    *framebuf;

uint8_t fb_c = 0;
uint8_t fb_r = 0;
void    init_framebuf(void) {
    static bool did_init = false;
    if (did_init) {
        return;
    }
    // Calculate the virtual columns and rows to make the framebuf a rectangular raster.
    uint8_t curcol = 1;
    for (uint8_t i = 1; i < ARRAY_SIZE(g_led_config.point); i++) {
        // Check to see if a new row has started.
        if (curcol > 0 && g_led_config.point[i].x < g_led_config.point[i - 1].x) {
            curcol = 0;
            fb_r++;
        }
        curcol++;
        if (curcol > fb_c) {
            fb_c = curcol;
        }
    }
    // Generate the virtual x/y for all LEDs
    uint8_t currow = 0;
    curcol         = 1;
    vpos[0].x      = 0;
    vpos[0].y      = 0;
    for (uint8_t i = 1; i < ARRAY_SIZE(g_led_config.point); i++) {
        led_point_t last    = g_led_config.point[i - 1];
        led_point_t current = g_led_config.point[i];
        if (current.x < last.x) {
            curcol = 0;
            currow++;
        }
        if (curcol > 0) {
            uint8_t jump = current.x - last.x;
            if (jump > 224 / fb_c * 2) {
                dprintf("jump size %d from led %d -> %d\n", jump, i - 1, i);
                curcol = current.x * fb_c / 224;
            }
        }
        vpos[i].y = currow;
        vpos[i].x = curcol;
        curcol++;
        dprintf("LED %d: x: %d, y: %d\n", i, vpos[i].x, vpos[i].y);
    }

    // Allocate framebuf.
    fb_r++;
    fb_c++;
    framebuf = calloc((fb_r) * (fb_c), sizeof(uint8_t));
    if (framebuf == NULL) {
        dprint("calloc failed\n");
    }

    did_init = true;

    dprintf("rows: %d, columns: %d\n", fb_r, fb_c);
}

void generate_palette(uint8_t user_hue) {
    uint8_t max  = PALETTE_SIZE - 1;
    hcolors[max] = hsv_to_rgb((HSV){user_hue + 35, 100, 255});
    for (uint8_t i = 1; i <= 3; i++) {
        hcolors[max - i] = hsv_to_rgb((HSV){blend8(user_hue + 35, user_hue + 5, (256 / 3) * i), 255, 255});
    }
    // uint8_t ease_in_val = 0;
    //  uint8_t scale;
    for (uint8_t i = 0; i < max - 3; i++) {
        uint8_t scale = 256 / (max - 3) * i;
        if (i == max - 4) {
            scale = 255;
        }
        hcolors[i] = hsv_to_rgb((HSV){user_hue, 255, blend8(0, 255, scale)});
        // uint8_t scale = ease8InOutQuad(ease_in_val);
        // ease_in_val += 256 / (max - 3);
        // hcolors[i] = hsv_to_rgb((HSV){user_hue, 255, lerp8by8(0, 240, scale)});
        // dprintf("i: %d ei: %d - scale: %d - lerp: %d\n", i, ease_in_val, scale, lerp8by8(0, 240, scale));
    }
#        ifdef CONSOLE_ENABLE
    for (uint8_t i = 0; i < PALETTE_SIZE; i++) {
        dprintf("pal[%d]\tR:0x%02X G:0x%02X B:0x%02X\n", i, hcolors[i].r, hcolors[i].g, hcolors[i].b);
    }
    dprintf("pal done\n");
#        endif
}

void generate_palette2(uint8_t user_hue) {
    // 	var i uint8
    // for i = 0; i < 6; i++ {
    // 	fmt.Println((uint8(255) / 5) * i)
    // }
    for (uint8_t i = 0; i < PALETTE_SIZE; i++) {
        uint8_t v = 255 / (PALETTE_SIZE - 1) * i;
        dprintf("v: %d v*2: %d v/3: %d\n", v, v * 2, v / 3);
        hcolors[i] = hsv_to_rgb((HSV){v / 3, 255, v * 2 > 255 ? 255 : v * 2});
    }

#        ifdef CONSOLE_ENABLE
    for (uint8_t i = 0; i < PALETTE_SIZE; i++) {
        dprintf("pal2[%d]\tR:0x%02X G:0x%02X B:0x%02X\n", i, hcolors[i].r, hcolors[i].g, hcolors[i].b);
    }
#        endif
}

/* Flare constants */
#        define MAX_FLARES 8
#        define FLARE_ROWS 2
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
            if (i < 0 || i >= fb_r || j < 0 || j >= fb_c) continue;
            int     d = (FLARE_DECAY * isqrt((x - j) * (x - j) + (y - i) * (y - i)) + 5) / 10;
            uint8_t n = 0;
            if (z > d) n = z - d;
            if (n > framebuf[i * fb_c + j]) { // can only get brighter
                framebuf[i * fb_c + j] = n;
            }
        }
    }
}

/* Fire functions*/
void newflare(void) {
    if (nflare < MAX_FLARES && rand() % FLARE_CHANCE == 0) {
        int x           = rand() % fb_c;
        int y           = rand() % FLARE_ROWS;
        int z           = PALETTE_SIZE - 1;
        flare[nflare++] = (z << 16) | (y << 8) | (x & 0xff);
        glow(x, y, z);
    }
}

void init_fire(void) {
    // Heat the bottom row
    for (uint8_t j = 0; j < fb_c; ++j) {
        framebuf[j] = random8_min_max(PALETTE_SIZE - 4, PALETTE_SIZE - 2);
    }
}

void make_fire(void) {
    uint8_t i, j;

    // First, move all existing heat points up the display and fade
    for (i = fb_r - 1; i > 0; --i) {
        for (j = 0; j < fb_c; ++j) {
            uint8_t n = 0;
            if (framebuf[(i - 1) * fb_c + j] > 0) n = framebuf[(i - 1) * fb_c + j] - 1;
            framebuf[i * fb_c + j] = n;
        }
    }

    // Heat the bottom row
    for (j = 0; j < fb_c; ++j) {
        i = framebuf[j];
        if (i > 0) {
            framebuf[j] = random8_min_max(PALETTE_SIZE - 4, PALETTE_SIZE - 2);
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

// void map_led_to_virt_row_column(uint8_t led_idx, uint8_t *row, uint8_t *col) {
//     uint16_t newrow = g_led_config.point[led_idx].y * (fb_r - 1) / 64;
//     uint16_t newcol = g_led_config.point[led_idx].x * (fb_c - 1) / 224;
//     *row            = (uint8_t)newrow;
//     *col            = (uint8_t)newcol;
// }

// A timer to track the last time we updated the flame render.
static uint16_t flame_render_timer;
// Whether we should render the flame during the next update.
static bool render_flame;

bool found = false;

static bool PIXEL_FLAME(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);

    if (params->iter == 0) {
        if (params->init) {
            init_framebuf();
            rgb_matrix_set_color_all(0, 0, 0);
            user_hue = rgb_matrix_get_hue();
            generate_palette(user_hue);
            init_fire();
        }
        render_flame = timer_elapsed(flame_render_timer) >= RGB_MATRIX_PIXEL_FLAME_DELAY_MS;
        if (render_flame) {
            if (user_hue != rgb_matrix_get_hue()) {
                user_hue = rgb_matrix_get_hue();
                generate_palette(user_hue);
            }
            make_fire();
            flame_render_timer = timer_read();
        }
    }

    for (uint8_t i = led_min; i < led_max; i++) {
        RGB led_color = hcolors[framebuf[(fb_r - 1 - vpos[i].y) * fb_c + vpos[i].x]];
        RGB_MATRIX_TEST_LED_FLAGS();
        rgb_matrix_set_color(i, led_color.r, led_color.g, led_color.b);
    }
    return rgb_matrix_check_finished_leds(led_max);
    // free(framebuf);
}
#    endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS
#endif     // ENABLE_RGB_MATRIX_PIXEL_FLAME
