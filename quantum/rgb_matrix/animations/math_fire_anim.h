#ifdef ENABLE_RGB_MATRIX_MATH_FIRE
RGB_MATRIX_EFFECT(MATH_FIRE)
#    ifdef RGB_MATRIX_CUSTOM_EFFECT_IMPLS
#        ifndef RGB_MATRIX_MATH_FIRE_SCALE_NOISE
#            define RGB_MATRIX_MATH_FIRE_SCALE_NOISE 255
#        endif

#        include "print.h"
// RGB HeatColor( uint8_t temperature)
//
// Approximates a 'black body radiation' spectrum for
// a given 'heat' level.  This is useful for animations of 'fire'.
// Heat is specified as an arbitrary scale from 0 (cool) to 255 (hot).
// This is NOT a chromatically correct 'black body radiation'
// spectrum, but it's surprisingly close, and it's fast and small.
//
// On AVR/Arduino, this typically takes around 70 bytes of program memory,
// versus 768 bytes for a full 256-entry RGB lookup table.
// (modified to use user hue)
// uint8_t user_hue;

RGB HeatColor(uint8_t temperature) {
    RGB heatcolor;

    // Scale 'heat' down from 0-255 to 0-191,
    // which can then be easily divided into three
    // equal 'thirds' of 64 units each.
    uint8_t t192 = scale8_video(temperature, 191);

    // calculate a value that ramps up from
    // zero to 255 in each 'third' of the scale.
    uint8_t heatramp = t192 & 0x3F; // 0..63
    heatramp <<= 2;                 // scale up to 0..252

    // now figure out which third of the spectrum we're in:
    if (t192 & 0x80) {
        // we're in the hottest third
        heatcolor.r = 255;      // full red
        heatcolor.g = 255;      // full green
        heatcolor.b = heatramp; // ramp up blue

    } else if (t192 & 0x40) {
        // we're in the middle third
        heatcolor.r = 255;      // full red
        heatcolor.g = heatramp; // ramp up green
        heatcolor.b = 0;        // no blue

    } else {
        // we're in the coolest third
        heatcolor.r = heatramp; // ramp up red
        heatcolor.g = 0;        // no green
        heatcolor.b = 0;        // no blue
    }

    return heatcolor;
}

led_point_t vpos[RGB_MATRIX_LED_COUNT];
uint8_t     raster_columns = 0;
uint8_t     raster_rows    = 0;
uint8_t     scale_noise    = RGB_MATRIX_MATH_FIRE_SCALE_NOISE;
void        init_raster(void) {
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
            raster_rows++;
        }
        curcol++;
        if (curcol > raster_columns) {
            raster_columns = curcol;
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
            if (jump > 224 / raster_columns * 2) {
                curcol = current.x * raster_columns / 224;
            }
        }
        vpos[i].y = currow;
        vpos[i].x = curcol;
        curcol++;
    }
    if (scale_noise == 255) {
        scale_noise = raster_rows * 8;
    }
}

static bool MATH_FIRE(effect_params_t *params) {
    RGB_MATRIX_USE_LIMITS(led_min, led_max);
    static uint16_t time = 0;

    if (params->iter == 0) {
        if (params->init) {
            rgb_matrix_set_color_all(0, 0, 0);
            // set_raster_dims();
            init_raster();
            // user_hue = rgb_matrix_get_hue();
        }
    }
    time += map8(rgb_matrix_config.speed, 1, 6);
    // time += rgb_matrix_config.speed > 0 ? rgb_matrix_config.speed >> 4 : 1;

    // if (scale16by8(g_rgb_timer, qadd8(rgb_matrix_config.speed, 16)) % 2 == 0) {
    //     time++;
    // }
    // uint8_t time = scale16by8(g_rgb_timer, qadd8(rgb_matrix_config.speed / 4, 1));
    for (uint8_t i = led_min; i < led_max; i++) {
        RGB_MATRIX_TEST_LED_FLAGS();
        RGB led_color = HeatColor(qsub8(srnoise8(vpos[i].x * scale_noise, vpos[i].y * scale_noise + time, time / 3) + 128, abs8(vpos[i].y - (raster_rows - 1)) * 255 / (raster_rows + 4)));
        rgb_matrix_set_color(i, led_color.r, led_color.g, led_color.b);
    }
    return rgb_matrix_check_finished_leds(led_max);
}

#    endif // RGB_MATRIX_CUSTOM_EFFECT_IMPLS
#endif     // ENABLE_RGB_MATRIX_MATH_FIRE
