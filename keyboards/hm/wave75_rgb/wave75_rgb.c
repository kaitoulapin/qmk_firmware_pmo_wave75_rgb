/* Copyright 2023 Finalkey
 * Copyright 2023 LiWenLiu <https://github.com/Linger7857>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../../lib/rdr_lib/rdr_common.h"
#include "usb_device_state.h"
#include "random_reactive.h"
#include <stdlib.h>
#include "logo_speed.h"
#include "sync_timer.h"

// The vendor animations consume this phase; their own stepping is disabled
// only during rendering, leaving their status indicators and settings intact.
extern uint8_t Logo_Play_Point;
static uint8_t logo_speed_percent = 50;
static bool logo_follow_speed;
static bool logo_follow_brightness;
static uint32_t logo_last_frame;
static uint32_t logo_phase_fraction;

uint8_t wave75_logo_get_speed(void) {
    return logo_speed_percent;
}

void wave75_logo_set_speed(uint8_t percent) {
    logo_speed_percent = percent > 100 ? 100 : percent;
}

bool wave75_logo_get_follow(bool brightness) {
    return brightness ? logo_follow_brightness : logo_follow_speed;
}

void wave75_logo_set_follow(bool brightness, bool enabled) {
    if (brightness) {
        logo_follow_brightness = enabled;
    } else {
        logo_follow_speed = enabled;
    }
}

void wave75_logo_save_speed(void) {
    uint32_t saved = 0x4C530200u | logo_speed_percent |
                     ((uint32_t)logo_follow_speed << 12) |
                     ((uint32_t)logo_follow_brightness << 13);
#if EECONFIG_USER_DATA_SIZE > 0
    eeconfig_update_user_datablock(&saved, 0, sizeof(saved));
#else
    eeconfig_update_user(saved);
#endif
}

uint8_t wave75_reactive_hue[RGB_MATRIX_LED_COUNT];
uint32_t wave75_reactive_birth[RGB_MATRIX_LED_COUNT];
bool wave75_reactive_active[RGB_MATRIX_LED_COUNT];

uint16_t wave75_reactive_duration_ticks(uint16_t milliseconds) {
    // Correct only this effect's duration, without changing the vendor's OS tick.
    // The FS026 port records the AHB clock during HAL initialization.
    extern unsigned int SystemFrequency_AHBClk;
    const uint32_t clock_khz = SystemFrequency_AHBClk / 1000;
    const uint32_t reload = SysTick->LOAD + 1;
    if (!clock_khz || !reload) return milliseconds;
    uint32_t ticks = ((uint32_t)milliseconds * clock_khz + reload - 1) / reload;
    return ticks ? ticks : 1;
}

void matrix_io_delay(void) {
}

void matrix_output_select_delay(void) {
}

void matrix_output_unselect_delay(uint8_t line, bool key_pressed) {
}

led_config_t g_led_config = { {
	{ 0        , 1        , 2        , 3        , 4        , 5        , 6        , 7        , 8        , 9        , 10       , 11       , 12       , NO_LED   , NO_LED   , NO_LED    },
	{ 15       , 16       , 17       , 18       , 19       , 20       , 21       , 22       , 23       , 24       , 25       , 26       , 27       , 28       , NO_LED   , 14        },
	{ 30       , 31       , 32       , 33       , 34       , 35       , 36       , 37       , 38       , 39       , 40       , 41       , 42       , 43       , 13       , 29        },
	{ 45       , 47       , 48       , 49       , 50       , 51       , 52       , 53       , 54       , 55       , 56       , 57       , NO_LED   , 58       , 44       , 59        },
	{ 60       , NO_LED   , 61       , 62       , 63       , 64       , 65       , 66       , 67       , 68       , 69       , 70       , NO_LED   , 71       , 72       , NO_LED    },
	{ 73       , 74       , 75       , NO_LED   , NO_LED   , 76       , NO_LED   , NO_LED   , NO_LED   , 77       , 78       , NO_LED   , NO_LED   , 79       , 80       , 81        },
    { NO_LED   , NO_LED   , NO_LED   , NO_LED   , NO_LED   , NO_LED   , NO_LED   , NO_LED   , NO_LED   , NO_LED   , NO_LED   , NO_LED   , NO_LED   , NO_LED   , NO_LED   , NO_LED    }
},{
    // "Fine-tuned" complex configuration
                { 0,   10},   { 15,  10},  { 30,  10},  { 45,  10}, { 60,  10}, { 75,  10}, { 90, 10}, { 105, 10}, { 120, 10}, { 135, 10}, { 150, 10}, { 165, 10}, { 180, 10}, { 195, 10}, { 208, 10},
                { 0,   20},   { 15,  20},  { 30,  20},  { 45,  20}, { 60,  20}, { 75,  20}, { 90, 20}, { 105, 20}, { 120, 20}, { 135, 20}, { 150, 20}, { 165, 20}, { 180, 20}, { 195, 20}, { 208, 20},
                { 0,   30},   { 15,  30},  { 30,  30},  { 45,  30}, { 60,  30}, { 75,  30}, { 90, 30}, { 105, 30}, { 120, 30}, { 135, 30}, { 150, 30}, { 165, 30}, { 180, 30}, { 195, 30}, { 208, 30},
    { 0,   40}, { 0,   40},   { 15,  40},  { 30,  40},  { 45,  40}, { 60,  40}, { 75,  40}, { 90, 40}, { 105, 40}, { 120, 40}, { 135, 40}, { 150, 40}, { 165, 40},             { 195, 40}, { 208, 40},
                { 0,   50},                { 15,  50},  { 30,  50}, { 45,  50}, { 60,  50}, { 75, 50}, { 90, 50},  { 105, 50}, { 120, 50}, { 135, 50}, { 150, 50}, { 165, 50}, { 180, 50},
                { 0,   60},   { 15,  60},  { 30,  60},                          { 75,  60},                                    { 135, 60}, { 150, 60},             { 180, 60}, { 195, 60}, { 208, 60},

                { 225, 65},   { 225,  65}, { 225,  65}, { 225,  65}
}, {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    1,
    1, 1, 1,       1,          1, 1,    1, 1, 1,

    0, 0, 0, 0
} };

bool rgb_matrix_indicators_advanced_user(uint8_t led_min, uint8_t led_max) {
    // Advance once per frame even if QMK renders the LED buffer in chunks.
    if (led_min == 0) {
        uint32_t now = timer_read32();
        uint32_t elapsed = now - logo_last_frame;
        logo_last_frame = now;
        if (elapsed > 100) elapsed = 100; // Avoid jumps after sleep/wakeup.
        uint8_t speed = logo_follow_speed ? ((uint16_t)rgb_matrix_get_speed() * 100 + 127) / 255 : logo_speed_percent;
        if (speed != 0) {
            logo_phase_fraction += elapsed * (4 + 2 * speed);
            uint32_t steps = logo_phase_fraction / 1000;
            logo_phase_fraction %= 1000;
            uint16_t period = (Keyboard_Info.Logo_Mode == LOGO_BREATH_MODE || Keyboard_Info.Logo_Mode == LOGO_WAVE_DS_MODE) ? 128 : 256;
            Logo_Play_Point = (Logo_Play_Point + period - steps % period) % period;
        }
    }
    if (Keyboard_Info.Logo_Mode == LOGO_BREATH_MODE || Keyboard_Info.Logo_Mode == LOGO_WAVE_DS_MODE) {
        Logo_Play_Point %= 128;
    }
    uint8_t vendor_speed = Keyboard_Info.Logo_Speed;
    uint8_t vendor_brightness = Keyboard_Info.Logo_Brightness;
    if (logo_follow_brightness) Keyboard_Info.Logo_Brightness = rgb_matrix_get_val();
    Keyboard_Info.Logo_Speed = 0;
    User_Led_Show();
    Keyboard_Info.Logo_Speed = vendor_speed;
    Keyboard_Info.Logo_Brightness = vendor_brightness;
    return false;
}

void notify_usb_device_state_change_user(struct usb_device_state usb_state)  {
    if (Keyboard_Info.Key_Mode == QMK_USB_MODE) {
        if (usb_state.configure_state == USB_DEVICE_STATE_CONFIGURED) {
            Usb_If_Ok_Led = true;
        } else {
            Usb_If_Ok_Led = false;
        }
    } else {
        Usb_If_Ok_Led = false;
    }
}

void housekeeping_task_user(void) {
    User_Keyboard_Reset();
    es_chibios_user_idle_loop_hook();
}

void board_init(void) {
    User_Keyboard_Init();
}

void keyboard_post_init_user(void) {
    User_Keyboard_Post_Init();
    uint32_t saved = 0;
#if EECONFIG_USER_DATA_SIZE > 0
    eeconfig_read_user_datablock(&saved, 0, sizeof(saved));
#else
    saved = eeconfig_read_user();
#endif
    if ((saved & 0xFFFF0F00u) == 0x4C530200u && (saved & 0xFFu) <= 100) {
        logo_speed_percent = saved & 0xFFu;
        logo_follow_speed = (saved & (1u << 12)) != 0;
        logo_follow_brightness = (saved & (1u << 13)) != 0;
    } else if ((saved & 0xFFFFFF00u) == 0x4C530100u && (saved & 0xFFu) <= 100) {
        logo_speed_percent = saved & 0xFFu;
    } else {
        wave75_logo_set_speed(Keyboard_Info.Logo_Speed * 25);
    }
    logo_last_frame = timer_read32();
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {   /*键盘只要有按键按下就会调用此函数*/
    if (keycode == LOGO_SPI || keycode == LOGO_SPD) {
        if (record->event.pressed) {
            int16_t speed = logo_speed_percent + (keycode == LOGO_SPI ? 5 : -5);
            wave75_logo_set_speed(speed < 0 ? 0 : speed > 100 ? 100 : speed);
            wave75_logo_save_speed();
        }
        Usb_Change_Mode_Delay = 0;
        Usb_Change_Mode_Wakeup = false;
        return false;
    }
    if (record->event.pressed && record->event.key.row < MATRIX_ROWS && record->event.key.col < MATRIX_COLS && rgb_matrix_get_mode() == RGB_MATRIX_CUSTOM_RANDOM_REACTIVE) {
        uint8_t leds[RGB_MATRIX_LED_COUNT];
        uint8_t count = rgb_matrix_map_row_column_to_led(record->event.key.row, record->event.key.col, leds);
        for (uint8_t i = 0; i < count; i++) {
            uint8_t led = leds[i];
            wave75_reactive_hue[led] = (uint8_t)rand();
            wave75_reactive_birth[led] = sync_timer_read32();
            wave75_reactive_active[led] = true;
        }
    }
    Usb_Change_Mode_Delay = 0;                                      /*只要有按键就不会进入休眠*/
    Usb_Change_Mode_Wakeup = false;

    return Key_Value_Dispose(keycode, record);
}

