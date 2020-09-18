#include <iostream>
#include <xaudio2.h>
#include <cmath>

#include "music_data.h"

constexpr size_t NUMBER_OF_NOTES = 80;
constexpr size_t SINE_TABLE_SIZE = 128;
constexpr unsigned int CHANNEL_OUTPUT_VOLUME = (128 / 4 - 1);
constexpr uint8_t MAX_AY_CHANNEL_VOLUME = 15;
constexpr double M_PI = 3.14159265358979323846;
constexpr unsigned int SAMPLES_PER_SEC = 44100;
constexpr size_t OUTPUT_BUFFER_LENGTH = SAMPLES_PER_SEC * 10;


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

class Channel {
private:
    uint16_t period = 0;
    uint16_t wave_position = 0;
    uint8_t volume = 15;
    bool is_tone_enabled = false;
    bool is_noise_enabled = false;

public:
    void init_tone(uint8_t tone) {
        period = tone_period_table[tone];
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
        return (is_tone_enabled ? volume * int32_t(sine_table[wave_position >> 8u]) / MAX_AY_CHANNEL_VOLUME : 0);
    }
} channel_a, channel_b, channel_c;

int8_t render_sample() {
    return channel_a.render() + channel_b.render() + channel_c.render();
}

void render(MusicData music_data) {
    init_sine_table();
    init_tone_period_table();

    channel_a.enable_tone(true);
    channel_b.enable_tone(true);
    channel_c.enable_tone(true);

    for (float &sample : output_buffer)
        sample = float(render_sample()) / 128;
}

int main() {
    CoInitialize(0);

    MusicData music_data_1 = get_music_data_1();
    MusicData music_data_2 = get_music_data_2();

    render(music_data_1);

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
