// SPDX-License-Identifier: GPL-2.0-or-later
#include "gtest/gtest.h"
#include "../../keyboards/hm/wave75_rgb/lighting_settings.h"

TEST(Wave75Lighting, ConfigurationRoundTrip) {
    for (unsigned flags = 0; flags < 32; flags++) {
        for (uint8_t percent : {0, 1, 5, 20, 100}) {
            wave75_lighting_settings_t s = {};
            s.logo_speed = percent;
            s.indicator_percent = percent;
            s.follow_speed = flags & 1;
            s.follow_brightness = flags & 2;
            s.allow_indicators_off = flags & 4;
            s.custom_indicators = flags & 8;
            s.limit_logo_brightness = flags & 16;
            wave75_lighting_settings_t restored = {};
            ASSERT_TRUE(wave75_lighting_decode(wave75_lighting_encode(s), &restored));
            EXPECT_EQ(wave75_lighting_encode(s), wave75_lighting_encode(restored));
        }
    }
}

TEST(Wave75Lighting, FloorAndCustomBrightness) {
    for (bool allow_off : {false, true}) {
        for (bool custom : {false, true}) {
            for (uint8_t percent : {0, 1, 5, 20, 100}) {
                wave75_lighting_settings_t s = {};
                s.indicator_percent = percent;
                s.allow_indicators_off = allow_off;
                s.custom_indicators = custom;
                for (unsigned zone = 0; zone <= 255; zone++) {
                    unsigned expected = custom ? (percent * 255 + 50) / 100 : zone;
                    if (!allow_off && expected < 13) expected = 13;
                    EXPECT_EQ(wave75_indicator_brightness(s, zone), expected);
                }
            }
        }
    }
    wave75_lighting_settings_t s = {};
    EXPECT_EQ(wave75_indicator_brightness(s, 255), 255);
}

TEST(Wave75Lighting, BlinkAndAnimationScaling) {
    for (uint8_t maximum : {160, 255}) {
        for (unsigned brightness = 0; brightness <= 255; brightness++) {
            EXPECT_EQ(wave75_indicator_scale(0, brightness, maximum), 0);
            EXPECT_EQ(wave75_indicator_scale(maximum, brightness, maximum), brightness);
            uint8_t previous = 0;
            for (unsigned channel = 0; channel <= maximum; channel++) {
                uint8_t scaled = wave75_indicator_scale(channel, brightness, maximum);
                EXPECT_GE(scaled, previous);
                EXPECT_LE(scaled, brightness);
                previous = scaled;
            }
        }
    }
    EXPECT_EQ(wave75_indicator_scale(128, 80, 255), 40);
}

TEST(Wave75Lighting, MigratesOldLogoSettings) {
    for (unsigned speed = 0; speed <= 100; speed++) {
        for (unsigned flags = 0; flags < 4; flags++) {
            wave75_lighting_settings_t s = {};
            ASSERT_TRUE(wave75_lighting_decode(0x4C530200u | speed | (flags << 12), &s));
            EXPECT_EQ(s.logo_speed, speed);
            EXPECT_EQ(s.indicator_percent, 20);
            EXPECT_EQ(s.follow_speed, !!(flags & 1));
            EXPECT_EQ(s.follow_brightness, !!(flags & 2));
            EXPECT_FALSE(s.custom_indicators);
            EXPECT_FALSE(s.allow_indicators_off);
            EXPECT_TRUE(s.limit_logo_brightness);
            ASSERT_TRUE(wave75_lighting_decode(0x4C530100u | speed, &s));
            EXPECT_FALSE(s.follow_speed);
            EXPECT_TRUE(s.follow_brightness);
        }
    }
}

TEST(Wave75Lighting, RejectsInvalidSettingsWithoutOverwriting) {
    wave75_lighting_settings_t s = {};
    s.logo_speed = 50;
    s.indicator_percent = 20;
    uint32_t before = wave75_lighting_encode(s);
    for (uint32_t invalid : {0u, 0xFFFFFFFFu, 0x4C300065u, 0x4C306500u, 0x4C530265u, 0x4C4000E5u, 0x4C406500u}) {
        EXPECT_FALSE(wave75_lighting_decode(invalid, &s));
        EXPECT_EQ(wave75_lighting_encode(s), before);
    }
}

TEST(Wave75Lighting, MigratesVersionThreeWithDefaultLimit) {
    for (unsigned flags = 0; flags < 16; flags++) {
        wave75_lighting_settings_t s = {};
        ASSERT_TRUE(wave75_lighting_decode(0x4C300000u | 100u | (20u << 8) | (flags << 16), &s));
        EXPECT_EQ(s.logo_speed, 100);
        EXPECT_EQ(s.indicator_percent, 20);
        EXPECT_EQ(s.follow_speed, !!(flags & 1));
        EXPECT_EQ(s.follow_brightness, !!(flags & 2));
        EXPECT_EQ(s.allow_indicators_off, !!(flags & 4));
        EXPECT_EQ(s.custom_indicators, !!(flags & 8));
        EXPECT_TRUE(s.limit_logo_brightness);
    }
}

TEST(Wave75Lighting, BrightnessStepSaturatesAtHardwareEndpoints) {
    for (unsigned value = 0; value <= 255; value++) {
        EXPECT_EQ(wave75_brightness_step(value, true), value > 239 ? 255 : value + 16);
        EXPECT_EQ(wave75_brightness_step(value, false), value < 16 ? 0 : value - 16);
    }
}

TEST(Wave75Lighting, LogoLimitDoesNotRewriteRequestedBrightness) {
    for (bool follow : {false, true}) {
        for (bool custom : {false, true}) {
            wave75_lighting_settings_t s = {};
            s.follow_brightness = follow;
            s.custom_indicators = custom;
            s.indicator_percent = 100;
            for (uint8_t requested : {0, 1, 140, 160, 254, 255}) {
                s.limit_logo_brightness = true;
                EXPECT_EQ(wave75_logo_limit(s, requested), requested > 160 ? 160 : requested);
                uint8_t indicator = wave75_indicator_brightness(s, requested);
                EXPECT_LE(wave75_logo_limit(s, indicator), 160);
                EXPECT_EQ(indicator, custom ? 255 : (requested < 13 ? 13 : requested));
                s.limit_logo_brightness = false;
                EXPECT_EQ(wave75_logo_limit(s, requested), requested);
            }
        }
    }
}

TEST(Wave75Lighting, LogoPwmUsesExactEndpoints) {
    for (unsigned channel = 0; channel <= 255; channel++) {
        EXPECT_EQ(wave75_logo_rgb_channel(channel, 0, 255), channel);
        EXPECT_EQ(wave75_logo_rgb_channel(channel, 0, 0), 0);
        EXPECT_EQ(wave75_logo_ds_channel(channel, 0, 255, 255), channel);
        EXPECT_EQ(wave75_logo_ds_channel(channel, 0, 255, 0), 0);
        EXPECT_EQ(wave75_logo_ds_channel(channel, 0, 0, 255), 0);
        for (uint8_t brightness : {0, 1, 140, 160, 254, 255}) {
            EXPECT_LE(wave75_logo_rgb_channel(channel, 127, brightness), brightness);
            EXPECT_LE(wave75_logo_ds_channel(channel, 127, 140, brightness), brightness);
        }
    }
}
