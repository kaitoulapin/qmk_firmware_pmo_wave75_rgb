#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t logo_speed;
    uint8_t indicator_percent;
    bool follow_speed;
    bool follow_brightness;
    bool allow_indicators_off;
    bool custom_indicators;
    bool limit_logo_brightness;
} wave75_lighting_settings_t;

extern wave75_lighting_settings_t wave75_lighting_settings;

// Version 4 uses the unused bit 7 of the speed byte for the Logo limit.
static inline uint32_t wave75_lighting_encode(wave75_lighting_settings_t s) {
    return 0x4C400000u | s.logo_speed | ((uint32_t)s.limit_logo_brightness << 7) | ((uint32_t)s.indicator_percent << 8) |
           ((uint32_t)s.follow_speed << 16) | ((uint32_t)s.follow_brightness << 17) |
           ((uint32_t)s.allow_indicators_off << 18) | ((uint32_t)s.custom_indicators << 19);
}

static inline bool wave75_lighting_decode(uint32_t saved, wave75_lighting_settings_t *s) {
    wave75_lighting_settings_t next = {.logo_speed = saved & 0xFFu, .indicator_percent = 20, .follow_brightness = true, .limit_logo_brightness = true};
    if ((saved & 0xFFF00000u) == 0x4C400000u) {
        next.logo_speed &= 0x7Fu;
        next.limit_logo_brightness = (saved & 0x80u) != 0;
    }
    if (next.logo_speed > 100) return false;
    if ((saved & 0xFFF00000u) == 0x4C300000u || (saved & 0xFFF00000u) == 0x4C400000u) {
        next.indicator_percent = (saved >> 8) & 0xFFu;
        if (next.indicator_percent > 100) return false;
        next.follow_speed = (saved & (1u << 16)) != 0;
        next.follow_brightness = (saved & (1u << 17)) != 0;
        next.allow_indicators_off = (saved & (1u << 18)) != 0;
        next.custom_indicators = (saved & (1u << 19)) != 0;
    } else if ((saved & 0xFFFF0F00u) == 0x4C530200u) {
        next.follow_speed = (saved & (1u << 12)) != 0;
        next.follow_brightness = (saved & (1u << 13)) != 0;
    } else if ((saved & 0xFFFFFF00u) != 0x4C530100u) {
        return false;
    }
    *s = next;
    return true;
}

static inline uint8_t wave75_indicator_brightness(wave75_lighting_settings_t s, uint8_t zone_brightness) {
    uint8_t brightness = s.custom_indicators ? ((uint16_t)s.indicator_percent * 255 + 50) / 100 : zone_brightness;
    if (!s.allow_indicators_off && brightness < 13) brightness = 13;
    return brightness;
}

static inline uint8_t wave75_indicator_scale(uint8_t channel, uint8_t brightness, uint8_t source_max) {
    return ((uint16_t)channel * brightness + source_max / 2) / source_max;
}

static inline uint8_t wave75_logo_limit(wave75_lighting_settings_t s, uint8_t brightness) {
    return s.limit_logo_brightness && brightness > 160 ? 160 : brightness;
}

static inline uint8_t wave75_brightness_step(uint8_t brightness, bool increase) {
    if (increase) return brightness > 239 ? 255 : brightness + 16;
    return brightness < 16 ? 0 : brightness - 16;
}

static inline uint8_t wave75_logo_rgb_channel(uint8_t channel, uint8_t saturation, uint8_t brightness) {
    return wave75_indicator_scale(channel | saturation, brightness, 255);
}

static inline uint8_t wave75_logo_ds_channel(uint8_t channel, uint8_t saturation, uint8_t phase, uint8_t brightness) {
    return wave75_logo_rgb_channel(wave75_indicator_scale(channel, phase, 255), saturation, brightness);
}
