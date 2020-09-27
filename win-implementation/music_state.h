#ifndef WIN_IMPLEMENTATION_MUSIC_STATE_H
#define WIN_IMPLEMENTATION_MUSIC_STATE_H

#include <exception>
#include "defines.h"
#include "music_data.h"
#include "ay_channel.h"

class ParseException : std::exception {
};

struct ChannelState {
    uint8_t current_note{};
    uint16_t instrument_ptr = 0;
    uint16_t instrument_playback_ptr{};
};

class MusicState {
private:
    MusicData *data{};
    uint8_t speed{};
    uint8_t speed_counter{};
    uint16_t position_ptr{};
    uint16_t pattern_current_note_ptr{};
    uint8_t pattern_counter{};
    uint16_t instrument_set_for_position{};
    uint16_t pattern_instrument_set_ptr{};

    ChannelState state_a, state_b, state_c, state_pseudo;

    void init();

    void start_from_the_beginning();

    void start_from_position(uint8_t position);

    void init_next_pattern();

    void parse_note(ChannelState &channel_state);

    void manage_instruments(Channel &channel, ChannelState &channel_state);

public:
    void set_data(MusicData *music_data);

    void next(Channel &channel_a, Channel &channel_b, Channel &channel_c);
};

#endif //WIN_IMPLEMENTATION_MUSIC_STATE_H
