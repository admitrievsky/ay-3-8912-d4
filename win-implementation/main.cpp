#include <iostream>
#include <xaudio2.h>
#include <cmath>

#include "music_data.h"

constexpr size_t NUMBER_OF_NOTES = 80;
constexpr size_t SINE_TABLE_SIZE = 128;
constexpr unsigned int CHANNEL_OUTPUT_VOLUME = (128 / 4 - 1);
constexpr uint8_t MAX_AY_CHANNEL_VOLUME = 15;
constexpr double M_PI = 3.14159265358979323846;
constexpr unsigned int SAMPLES_PER_SEC = 11025;
constexpr size_t OUTPUT_BUFFER_LENGTH = SAMPLES_PER_SEC * 300;


int8_t sine_table[SINE_TABLE_SIZE];
int16_t tone_period_table[NUMBER_OF_NOTES];

float output_buffer[OUTPUT_BUFFER_LENGTH];

void init_sine_table() {
    for (size_t i = 0; i < SINE_TABLE_SIZE; i++)
        sine_table[i] = CHANNEL_OUTPUT_VOLUME * sin(2 * i * M_PI / SINE_TABLE_SIZE);
}

void init_tone_period_table() {
    for (size_t n = 0; n < NUMBER_OF_NOTES; n++) {
        const auto frequency = 32.703 * std::pow(2, double(n) / 12);
        tone_period_table[n] = lround((SINE_TABLE_SIZE * frequency / SAMPLES_PER_SEC) * 256);
    }
}

class ParseException : std::exception {
};

uint16_t xs = 1;

uint16_t xor_shift() {
    xs ^= xs << 7u;
    xs ^= xs >> 9u;
    xs ^= xs << 8u;
    return xs;
}

class Channel {
private:
    uint16_t period = 0;
    uint16_t wave_position = 0;
    uint8_t volume = 15;
    bool is_tone_enabled = false;
    bool is_noise_enabled = false;

public:
    void init_tone(uint8_t tone, uint16_t shift = 0) {
        period = tone_period_table[tone] + shift;
    }

    void set_volume(uint8_t _volume) {
        volume = _volume;
    }

    void enable_tone(bool _is_tone_enabled) {
        is_tone_enabled = _is_tone_enabled;
    }

    void enable_noise(bool _is_noise_enabled) {
        is_noise_enabled = _is_noise_enabled;
    }

    int8_t render() {
        wave_position += period;
        if (wave_position >= (SINE_TABLE_SIZE << 8u))
            wave_position -= SINE_TABLE_SIZE << 8u;
        return (is_tone_enabled ? volume * int32_t(sine_table[wave_position >> 8u]) / MAX_AY_CHANNEL_VOLUME : 0) +
               (is_noise_enabled ? volume * int16_t(xor_shift()) / 256 / 16 / MAX_AY_CHANNEL_VOLUME : 0);
    }
} channel_a, channel_b, channel_c;

enum InstrumentType {
    ORNAMENT = 0,
    SAMPLE = 1,
    NOISE = 2
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

    ChannelState state_a, state_b, state_c;

    void init() {
        start_from_the_beginning();
        init_next_pattern();
    }

    void start_from_the_beginning() {
        speed = data->speed;
        speed_counter = 1;
        position_ptr = data->positions_table;
        instrument_set_for_position = data->instrument_table_for_positions;
    }

    void start_from_position(uint8_t position) {
        position_ptr = data->positions_table + position * 2;
        instrument_set_for_position = data->instrument_table_for_positions + position * 2;
    }

    void init_next_pattern() {
        while ((pattern_current_note_ptr = data->read_word(position_ptr)) == 0xFFFF) {
            start_from_the_beginning();
        }
        pattern_counter = 64;
        pattern_current_note_ptr = data->read_word(position_ptr);
        pattern_instrument_set_ptr = data->read_word(instrument_set_for_position);

        position_ptr += 2;
        instrument_set_for_position += 2;
    }

public:

    void set_data(MusicData *music_data) {
        data = music_data;
        init();
    }

    void parse_note(Channel &channel, ChannelState &channel_state) {
        while (true) {
            auto note = data->read_byte(pattern_current_note_ptr++);
            if (!note)
                return;
            if (note < 0x80) {
                note -= 2;
                channel.init_tone(note);
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

    void manage_instruments(Channel &channel, ChannelState &channel_state) {
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
                channel.init_tone(channel_state.current_note, (
                        (val < 0x80 ? val : -(val & 0x80u)) / 8 // magic
                ));
            }
        }
        channel_state.instrument_playback_ptr++;

        auto volume = data->read_byte(channel_state.instrument_playback_ptr++);
        channel.set_volume(volume);
    }

    void next() {
        if (!--speed_counter) {
            speed_counter = speed;

            if (!pattern_counter)
                init_next_pattern();
            pattern_counter--;

            parse_note(channel_a, state_a);
            parse_note(channel_b, state_b);
            parse_note(channel_c, state_c);
            parse_note(channel_c, state_c); // Pseudo channel, used only for commands
        }

        manage_instruments(channel_a, state_a);
        manage_instruments(channel_b, state_b);
        manage_instruments(channel_c, state_c);
    }
} state;

int8_t render_sample(size_t tick) {
    if ((tick % (SAMPLES_PER_SEC / 50)) == 0)
        state.next();
    return channel_a.render() + channel_b.render() + channel_c.render();
}

void render(MusicData &music_data) {
    init_sine_table();
    init_tone_period_table();

    state.set_data(&music_data);

    channel_a.enable_tone(true);
    channel_b.enable_tone(true);
    channel_c.enable_tone(true);

    for (size_t i = 0; i < OUTPUT_BUFFER_LENGTH; i++)
        output_buffer[i] = float(render_sample(i)) / 128;
}

int main() {
    CoInitialize(0);

    MusicData music_data_1 = get_music_data_1();
    MusicData music_data_2 = get_music_data_2();

    try {
        render(music_data_1);
    } catch (ParseException) {
        std::cout << "Parse exception";
        return 1;
    }

    IXAudio2 *pXAudio2 = NULL;
    HRESULT hr;
    if (FAILED(hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
        return hr;

    IXAudio2MasteringVoice *pMasterVoice = NULL;
    if (FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasterVoice)))
        return hr;

    WAVEFORMATEX wfx;
    wfx.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = SAMPLES_PER_SEC;
    wfx.nAvgBytesPerSec = SAMPLES_PER_SEC * 4;
    wfx.wBitsPerSample = 32;
    wfx.cbSize = 0;
    wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample) / 8;


    IXAudio2SourceVoice *pSourceVoice;
    if (FAILED(hr = pXAudio2->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX *) &wfx)))
        return hr;

    XAUDIO2_BUFFER xaudio2Buffer;
    xaudio2Buffer.Flags = 0;
    xaudio2Buffer.AudioBytes = OUTPUT_BUFFER_LENGTH * 4;
    xaudio2Buffer.pAudioData = reinterpret_cast<const BYTE *>(output_buffer);
    xaudio2Buffer.PlayBegin = 0;
    xaudio2Buffer.PlayLength = 0;
    xaudio2Buffer.LoopBegin = 0;
    xaudio2Buffer.LoopLength = 0;
    xaudio2Buffer.LoopCount = 0;
    xaudio2Buffer.pContext = nullptr;

    if (FAILED(hr = pSourceVoice->SubmitSourceBuffer(&xaudio2Buffer)))
        return hr;

    if (FAILED(hr = pSourceVoice->Start(0)))
        return hr;

    getchar();

    return 0;
}
