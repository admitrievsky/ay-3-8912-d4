#ifndef AY_CHANNEL_H
#define AY_CHANNEL_H

#include "defines.h"

constexpr size_t FIXED_INTEGER_SIZE = 256;
union FixedPoint {
    struct {
        uint8_t fraction;
        uint8_t integer;
    } parts;
    uint16_t raw = 0;
};

class Channel {
private:
    FixedPoint period;
    FixedPoint wave_position;
    uint8_t volume = 0;
    bool is_tone_enabled = false;
    bool is_noise_enabled = false;

    static FixedPoint tone_period_table[NUMBER_OF_NOTES];
    static uint8_t volumes[];

public:
    static void init_tone_period_table();

    void init_tone(uint8_t tone, int16_t shift = 0);

    void set_volume(uint8_t _volume);

    void enable_tone(bool _is_tone_enabled);

    void enable_noise(bool _is_noise_enabled);

    int8_t render();
};


#endif
