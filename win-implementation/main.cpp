#include <iostream>
#include <xaudio2.h>
#include <cmath>

#include "defines.h"
#include "music_data.h"
#include "ay_channel.h"
#include "music_state.h"

float output_buffer[OUTPUT_BUFFER_LENGTH];


Channel channel_a, channel_b, channel_c;

MusicState state;

int8_t render_sample(size_t tick) {
    if ((tick % (SAMPLES_PER_SEC / 50)) == 0)
        state.next(channel_a, channel_b, channel_c);
    auto a = channel_a.render();
    auto b = channel_b.render();
    auto c = channel_c.render();
    if (abs((int16_t) a + (int16_t) b + (int16_t) c) > 127)
        std::cout << "overload\n";
    return a + b + c;
}

void render(MusicData &music_data) {
    state.set_data(&music_data);

    for (size_t i = 0; i < OUTPUT_BUFFER_LENGTH; i++)
        output_buffer[i] = float(render_sample(i)) / 128;
}

int main() {
    CoInitialize(0);
    Channel::init_tone_period_table();

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
