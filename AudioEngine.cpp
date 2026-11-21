#include "AudioEngine.h"
#include "Common.h"
#include <cstdio>
#include <cmath>

static bool g_hasTrackFile = false;
static char g_trackPath[MAX_PATH] = "";
static bool g_isPlaying = false;
static float g_lastSyncSeqTime = -100.0f;
static bool g_windTriggered = false;
static std::vector<unsigned char> ambientWavData;

static void buildProceduralSynthFallback() {
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
}

void initAudioEngine() {
    const char* candidates[] = {
        "soundtrack.mp3",
        "soundtrack.wav",
        "no_time_for_caution.mp3",
        "no_time_for_caution.wav",
        "notimeforcaution.mp3",
        "notimeforcaution.wav",
        "interstellar.mp3",
        "interstellar.wav"
    };
    const int numCandidates = sizeof(candidates) / sizeof(candidates[0]);

    g_hasTrackFile = false;
    for (int i = 0; i < numCandidates; ++i) {
        FILE* fp = fopen(candidates[i], "rb");
        if (fp) {
            fclose(fp);
            strncpy(g_trackPath, candidates[i], sizeof(g_trackPath) - 1);
            g_hasTrackFile = true;
            break;
        }
    }

    if (g_hasTrackFile) {
        mciSendStringA("close all", NULL, 0, NULL);
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias soundtrack", g_trackPath);
        if (mciSendStringA(cmd, NULL, 0, NULL) != 0) {
            snprintf(cmd, sizeof(cmd), "open \"%s\" alias soundtrack", g_trackPath);
            mciSendStringA(cmd, NULL, 0, NULL);
        }
        mciSendStringA("set soundtrack time format milliseconds", NULL, 0, NULL);
        mciSendStringA("seek soundtrack to start", NULL, 0, NULL);
        g_isPlaying = false;
        g_lastSyncSeqTime = -10.0f;
    } else {
        // Fallback procedural Hans Zimmer organ synth
        buildProceduralSynthFallback();
        PlaySoundA((LPCSTR)ambientWavData.data(), NULL, SND_MEMORY | SND_ASYNC | SND_LOOP);
    }

    // Pre-open Mars wind SFX on independent MCI channel
    FILE* fWind = fopen("mars_wind_nasa.wav", "rb");
    if (fWind) {
        fclose(fWind);
        mciSendStringA("open \"mars_wind_nasa.wav\" type waveaudio alias mars_wind", NULL, 0, NULL);
    }
}

void resetAudioEngine() {
    if (g_hasTrackFile) {
        mciSendStringA("stop soundtrack", NULL, 0, NULL);
        mciSendStringA("seek soundtrack to start", NULL, 0, NULL);
        mciSendStringA("play soundtrack from 0", NULL, 0, NULL);
        mciSendStringA("stop mars_wind", NULL, 0, NULL);
        mciSendStringA("seek mars_wind to start", NULL, 0, NULL);
        g_isPlaying = true;
        g_lastSyncSeqTime = 0.0f;
        g_windTriggered = false;
    } else {
        PlaySoundA((LPCSTR)ambientWavData.data(), NULL, SND_MEMORY | SND_ASYNC | SND_LOOP);
    }
}

void updateAudioEngine(float seqTime, bool isPaused) {
    // Detect loop wrap-around or fresh replay (seqTime wrapped back to 0)
    if (seqTime < 0.40f && g_lastSyncSeqTime > 5.0f) {
        resetAudioEngine();
        return;
    }

    if (g_hasTrackFile) {
        // Pure digital silence when seqTime reaches g_cutToBlackTime
        if (seqTime >= g_cutToBlackTime) {
            if (g_isPlaying) {
                mciSendStringA("stop soundtrack", NULL, 0, NULL);
                mciSendStringA("stop mars_wind", NULL, 0, NULL);
                g_isPlaying = false;
            }
            return;
        }

        if (isPaused) {
            if (g_isPlaying) {
                mciSendStringA("pause soundtrack", NULL, 0, NULL);
                mciSendStringA("pause mars_wind", NULL, 0, NULL);
                g_isPlaying = false;
            }
            return;
        }

        // Scene 3 entry transition lock (44.0s) - guarantees natural playback matches jumping directly to Scene 3
        static bool s3CutSynced = false;
        if (seqTime < 40.0f) {
            s3CutSynced = false;
        }
        if (!s3CutSynced && seqTime >= 44.0f && seqTime < 44.5f) {
            s3CutSynced = true;
            mciSendStringA("play soundtrack from 44000", NULL, 0, NULL);
            g_isPlaying = true;
            g_lastSyncSeqTime = seqTime;
        }

        // User scrub / jump detection (> 0.40s deviation) or initial start
        if (fabsf(seqTime - g_lastSyncSeqTime) > 0.40f || !g_isPlaying) {
            int ms = (int)(seqTime * 1000.0f);
            char playCmd[128];
            snprintf(playCmd, sizeof(playCmd), "play soundtrack from %d", ms);
            mciSendStringA(playCmd, NULL, 0, NULL);
            g_isPlaying = true;
        }
        g_lastSyncSeqTime = seqTime;
    }

    // Mars wind SFX trigger during atmospheric entry (Scene 3: 44.0s - 64.2s)
    if (seqTime >= 44.0f && seqTime < 64.2f) {
        if (!g_windTriggered) {
            g_windTriggered = true;
            mciSendStringA("play mars_wind from 0", NULL, 0, NULL);
        }
    } else if (seqTime < 44.0f || seqTime >= 64.2f) {
        if (g_windTriggered) {
            mciSendStringA("stop mars_wind", NULL, 0, NULL);
            g_windTriggered = false;
        }
    }
}

void shutdownAudioEngine() {
    mciSendStringA("close all", NULL, 0, NULL);
    PlaySoundA(NULL, NULL, 0);
}
