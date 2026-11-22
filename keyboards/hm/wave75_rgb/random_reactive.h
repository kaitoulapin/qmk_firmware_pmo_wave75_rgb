#pragma once

enum {
    WAVE75_REACTIVE_OFF,
    WAVE75_REACTIVE_HELD,
    WAVE75_REACTIVE_FADING,
};

extern uint8_t  wave75_reactive_hue[RGB_MATRIX_LED_COUNT];
extern uint32_t wave75_reactive_release_time[RGB_MATRIX_LED_COUNT];
extern uint8_t  wave75_reactive_state[RGB_MATRIX_LED_COUNT];
uint16_t        wave75_reactive_duration_ticks(uint16_t milliseconds);
