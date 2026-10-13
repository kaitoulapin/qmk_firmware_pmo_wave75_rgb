#pragma once

uint8_t wave75_logo_get_speed(void);
void wave75_logo_set_speed(uint8_t percent);
void wave75_logo_save_speed(void);
bool wave75_logo_get_follow(bool brightness);
void wave75_logo_set_follow(bool brightness, bool enabled);
uint8_t wave75_logo_get_brightness(void);
void wave75_logo_set_brightness(uint8_t brightness);
bool wave75_logo_get_limit(void);
void wave75_logo_set_limit(bool enabled);
