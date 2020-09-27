#ifndef WIN_IMPLEMENTATION_DEFINES_H
#define WIN_IMPLEMENTATION_DEFINES_H

#include <cstdint>

constexpr size_t NUMBER_OF_NOTES = 80;
constexpr unsigned int CHANNEL_OUTPUT_VOLUME = (128 / 4 - 1);
constexpr uint8_t MAX_AY_CHANNEL_VOLUME = 15;
constexpr unsigned int SAMPLES_PER_SEC = 44100;
constexpr size_t OUTPUT_BUFFER_LENGTH = SAMPLES_PER_SEC * 300;


#endif //WIN_IMPLEMENTATION_DEFINES_H
