#include <iostream>

#include "music_state.h"

enum InstrumentType {
    ORNAMENT = 0,
    SAMPLE = 1,
    NOISE = 2
};

void MusicState::init() {
    start_from_the_beginning();
    init_next_pattern();
}

void MusicState::start_from_the_beginning() {
    speed = data->speed;
    speed_counter = 1;
    position_ptr = data->positions_table;
    instrument_set_for_position = data->instrument_table_for_positions;
}

void MusicState::start_from_position(uint8_t position) {
    position_ptr = data->positions_table + position * 2;
    instrument_set_for_position = data->instrument_table_for_positions + position * 2;
}

void MusicState::init_next_pattern() {
    while ((pattern_current_note_ptr = data->read_word(position_ptr)) == 0xFFFF) {
        start_from_the_beginning();
    }
    pattern_counter = 64;
    pattern_current_note_ptr = data->read_word(position_ptr);
    pattern_instrument_set_ptr = data->read_word(instrument_set_for_position);

    position_ptr += 2;
    instrument_set_for_position += 2;
}

void MusicState::parse_note(ChannelState &channel_state) {
    while (true) {
        auto note = data->read_byte(pattern_current_note_ptr++);
        if (!note)
            return;
        if (note < 0x80) {
            note -= 2;
            channel_state.current_note = note;
            channel_state.instrument_playback_ptr = channel_state.instrument_ptr + 1;
            return;
        }
        auto command = note & 0xE0u;
        if (command == 0x80) { // instrument specified
            uint8_t instrument = note & 0x1Fu;
            if (instrument < 0x10) {
                channel_state.instrument_ptr = data->read_word(pattern_instrument_set_ptr + instrument * 2);
            } else {
                // not clear???
            }
            continue;
        }
        if (command == 0xc0) { // go to position
            auto position = note & 0x1Fu;
            pattern_counter = 0;
            if (position)
                start_from_position(position);
            continue;
        }
        if (command == 0xa0) { // change speed
            speed = note & 0x0Fu;
            continue;
        }
        std::cout << "Unknown command: " << std::hex << (uint16_t) command << std::endl;
        throw ParseException();
    }

}

void MusicState::manage_instruments(Channel &channel, ChannelState &channel_state) {
    if (!channel_state.instrument_ptr)
        return;
    auto instrument_type = (InstrumentType) data->read_byte(channel_state.instrument_ptr);
    uint8_t val = data->read_byte(channel_state.instrument_playback_ptr);

    if (instrument_type != NOISE && instrument_type != SAMPLE && instrument_type != ORNAMENT)
        throw ParseException();

    if (instrument_type == NOISE) {
        if (val == 0xFF) {
            channel.enable_tone(false);
            channel.enable_noise(false);
            return;
        }
        if (val >= 0x80) {
            channel.enable_tone(false);
            channel.enable_noise(true);
            // we do not store noise frequency, due to emulation limitations
        } else {
            // play tone from instrument
            channel.enable_tone(true);
            channel.enable_noise(false);
            channel.init_tone((val & 0x3Fu) / 2);
        }
    }

    if (instrument_type == ORNAMENT || instrument_type == SAMPLE) {
        if (val == 0xFF)
            return;
        channel.enable_tone(true);
        channel.enable_noise(false);
        if (instrument_type == ORNAMENT) {
            uint8_t new_note = channel_state.current_note + (
                    val < 0x80 ? val : -(val & 0x80u)
            );
            channel.init_tone(new_note);
        } else { // SAMPLE
            auto shift = (int8_t) val / 48; // magic
            channel.init_tone(channel_state.current_note, (
                    (val < 0x80 ? shift : -shift)
            ));
        }
    }
    channel_state.instrument_playback_ptr++;

    auto volume = data->read_byte(channel_state.instrument_playback_ptr++);
    channel.set_volume(volume);
}

void MusicState::set_data(MusicData *music_data) {
    data = music_data;
    init();
}

void MusicState::next(Channel &channel_a, Channel &channel_b, Channel &channel_c) {
    if (!--speed_counter) {
        speed_counter = speed;

        if (!pattern_counter)
            init_next_pattern();
        pattern_counter--;

        parse_note(state_a);
        parse_note(state_b);
        parse_note(state_c);
        parse_note(state_pseudo); // Pseudo channel, used only for commands
    }

    manage_instruments(channel_a, state_a);
    manage_instruments(channel_b, state_b);
    manage_instruments(channel_c, state_c);
}
