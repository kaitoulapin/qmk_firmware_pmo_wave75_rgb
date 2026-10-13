// SPDX-License-Identifier: GPL-2.0-or-later
#include QMK_KEYBOARD_H
#include "lighting_settings.h"
#include "indicator_lighting.h"
#include "../../lib/rdr_lib/rdr_common.h"

extern void User_Point_Show(void);
extern void Logo_Mode_Show(void);
extern void Led_Rf_Mode_Show(void);
extern void Led_Power_Low_Show(void);
extern void Led_Point_Flash_Show(void);
extern void Led_Batt_Number_Show(void);

typedef struct {
    void (*function)(void);
    uint16_t return_offset;
    uint8_t source_max;
} indicator_call_t;

// Return PC = BL offset + 4, with the Thumb bit masked off. These offsets
// belong to the fingerprinted vendor archive, NOT to the final ELF addresses.
// Ordinary Logo animations, black initialization and factory tests are absent.
static const indicator_call_t indicator_calls[] = {
    {User_Led_Show, 0x7c, 160},
    {User_Point_Show, 0x40, 160},
    {Logo_Mode_Show, 0x38, 160},
    {Logo_Mode_Show, 0x7e, 160},
    {Led_Rf_Mode_Show, 0x40, 255},
    {Led_Rf_Mode_Show, 0x8a, 255},
    {Led_Rf_Mode_Show, 0xac, 255},
    {Led_Power_Low_Show, 0x30, 160},
    {Led_Point_Flash_Show, 0x26, 160},
    {Led_Point_Flash_Show, 0x5a, 160},
    {Led_Point_Flash_Show, 0x80, 160},
    {Led_Point_Flash_Show, 0x96, 160},
    {Led_Point_Flash_Show, 0xd4, 160},
    {Led_Batt_Number_Show, 0x36, 255},
    {Led_Batt_Number_Show, 0x68, 255},
    {Led_Batt_Number_Show, 0xa8, 255},
};

uint8_t wave75_indicator_get_value(uint8_t id) {
    if (id == 7) return wave75_lighting_settings.allow_indicators_off;
    if (id == 8) return wave75_lighting_settings.custom_indicators;
    if (id == 9) return wave75_lighting_settings.indicator_percent;
    return 0;
}

void wave75_indicator_set_value(uint8_t id, uint8_t value) {
    if (id == 7) wave75_lighting_settings.allow_indicators_off = value != 0;
    if (id == 8) wave75_lighting_settings.custom_indicators = value != 0;
    if (id == 9) wave75_lighting_settings.indicator_percent = value > 100 ? 100 : value;
}

void __real_rgb_matrix_set_color(int index, uint8_t red, uint8_t green, uint8_t blue);

// Keep this a real function: the return address identifies the library caller.
__attribute__((noinline)) void __wrap_rgb_matrix_set_color(int index, uint8_t red, uint8_t green, uint8_t blue) {
    if (index >= 0 && index < RGB_MATRIX_LED_COUNT && (red || green || blue)) {
        uintptr_t caller = (uintptr_t)__builtin_return_address(0) & ~(uintptr_t)1;
        for (uint8_t i = 0; i < sizeof(indicator_calls) / sizeof(indicator_calls[0]); i++) {
            uintptr_t start = (uintptr_t)indicator_calls[i].function & ~(uintptr_t)1;
            if (caller != start + indicator_calls[i].return_offset) continue;
            uint8_t zone = index >= 82 ? (wave75_lighting_settings.follow_brightness ? rgb_matrix_get_val() : Keyboard_Info.Logo_Brightness) : rgb_matrix_get_val();
            uint8_t brightness = wave75_indicator_brightness(wave75_lighting_settings, zone);
            red = wave75_indicator_scale(red, brightness, indicator_calls[i].source_max);
            green = wave75_indicator_scale(green, brightness, indicator_calls[i].source_max);
            blue = wave75_indicator_scale(blue, brightness, indicator_calls[i].source_max);
            break;
        }
    }
    __real_rgb_matrix_set_color(index, red, green, blue);
}
