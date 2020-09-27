#include <math.h>
#include "ay_channel.h"

FixedPoint Channel::tone_period_table[NUMBER_OF_NOTES];
uint8_t Channel::volumes[] = {0, 0, 0, 0, 1, 1, 1, 2, 3, 4, 6, 7, 9, 11, 13, 15};

void Channel::init_tone_period_table() {
    for (size_t n = 0; n < NUMBER_OF_NOTES; n++) {
        const auto frequency = 32.703 * pow(2, double(n) / 12);
        tone_period_table[n].raw = lround((FIXED_INTEGER_SIZE * frequency / SAMPLES_PER_SEC) * 256);
    }
}

uint16_t xs = 1;

uint16_t xor_shift() {
    xs ^= xs << 7u;
    xs ^= xs >> 9u;
    xs ^= xs << 8u;
    return xs;
}

void Channel::init_tone(uint8_t tone, int16_t shift) {
    period.raw = tone_period_table[tone].raw + shift;
}

void Channel::set_volume(uint8_t _volume) {
    volume = volumes[_volume];
}

void Channel::enable_tone(bool _is_tone_enabled) {
    is_tone_enabled = _is_tone_enabled;
}

void Channel::enable_noise(bool _is_noise_enabled) {
    is_noise_enabled = _is_noise_enabled;
}

int8_t Channel::render() {
    wave_position.raw += period.raw;
        return (is_tone_enabled ?
                (wave_position.parts.integer < (FIXED_INTEGER_SIZE / 2) ? (int8_t) volume
                                                                        : -(int8_t) volume
                ) : 0) + 
                (is_noise_enabled ? volumes[volume] * int16_t(xor_shift()) & 0x1f : 0);
}
