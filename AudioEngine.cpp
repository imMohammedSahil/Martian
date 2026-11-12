#include "AudioEngine.h"
#include "Common.h"

static std::vector<unsigned char> ambientWavData;

void initAndPlaySpaceAmbience() {
    const int sampleRate  = 22050;
    const int numChannels = 2;
    const int bitsPerSamp = 16;
    const int durationSec = 16;
    const int totalSamples = sampleRate * durationSec;
    const int dataBytes    = totalSamples * numChannels * (bitsPerSamp / 8);

    ambientWavData.resize(44 + dataBytes);
    unsigned char* p = ambientWavData.data();

    // RIFF Header
    memcpy(p, "RIFF", 4);
    int chunkSize = 36 + dataBytes;
    memcpy(p + 4, &chunkSize, 4);
    memcpy(p + 8, "WAVE", 4);

    // fmt Subchunk
    memcpy(p + 12, "fmt ", 4);
    int sub1Size = 16;
    short audioFormat = 1; // PCM
    short numChan = numChannels;
    int sRate = sampleRate;
    int byteRate = sampleRate * numChannels * (bitsPerSamp / 8);
    short blockAlign = numChannels * (bitsPerSamp / 8);
    short bPerSample = bitsPerSamp;

    memcpy(p + 16, &sub1Size, 4);
    memcpy(p + 20, &audioFormat, 2);
    memcpy(p + 22, &numChan, 2);
    memcpy(p + 24, &sRate, 4);
    memcpy(p + 28, &byteRate, 4);
    memcpy(p + 32, &blockAlign, 2);
    memcpy(p + 34, &bPerSample, 2);

    // data Subchunk
    memcpy(p + 36, "data", 4);
    memcpy(p + 40, &dataBytes, 4);

    // Hans Zimmer Pipe Organ Chord: D2, A2, D3, F3, A3, E4
    const float freqs[] = { 73.42f, 110.00f, 146.83f, 174.61f, 220.00f, 329.63f };
    const float amps[]  = { 0.30f,  0.22f,   0.20f,   0.16f,   0.12f,   0.08f };
    const int nFreqs = sizeof(freqs) / sizeof(freqs[0]);

    short* samples = (short*)(p + 44);

    for (int i = 0; i < totalSamples; ++i) {
        float t = (float)i / (float)sampleRate;

        // Slow breathing LFO modulation
        float lfo = 0.85f + 0.15f * sinf(2.0f * (float)M_PI * 0.08f * t);

        // Sub-bass spacecraft hull rumble (38 Hz & 54 Hz beat)
        float subRumble = 0.22f * sinf(2.0f * (float)M_PI * 38.0f * t)
                        + 0.14f * sinf(2.0f * (float)M_PI * 54.0f * t);

        float organL = 0.0f, organR = 0.0f;
        for (int k = 0; k < nFreqs; ++k) {
            float phaseL = 2.0f * (float)M_PI * freqs[k] * t;
            float phaseR = phaseL + (float)k * 0.45f;
            float harmonic = sinf(phaseL) + 0.25f * sinf(phaseL * 2.0f);
            organL += amps[k] * harmonic;
            organR += amps[k] * (sinf(phaseR) + 0.25f * sinf(phaseR * 2.0f));
        }

        float mixL = (organL * lfo + subRumble) * 0.48f;
        float mixR = (organR * lfo + subRumble) * 0.48f;

        // Loop crossfade
        float edgeFade = 1.0f;
        const float fadeTime = 0.8f;
        if (t < fadeTime) {
            edgeFade = t / fadeTime;
        } else if (t > durationSec - fadeTime) {
            edgeFade = (durationSec - t) / fadeTime;
        }
        mixL *= edgeFade;
        mixR *= edgeFade;

        int sL = (int)(clampf(mixL, -1.0f, 1.0f) * 32760.0f);
        int sR = (int)(clampf(mixR, -1.0f, 1.0f) * 32760.0f);

        samples[i * 2 + 0] = (short)sL;
        samples[i * 2 + 1] = (short)sR;
    }

    PlaySoundA((LPCSTR)ambientWavData.data(), NULL, SND_MEMORY | SND_ASYNC | SND_LOOP);
}

