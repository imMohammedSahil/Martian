#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

void initAudioEngine();
void updateAudioEngine(float seqTime, bool isPaused);
void resetAudioEngine();
void shutdownAudioEngine();

inline void initAndPlaySpaceAmbience() { initAudioEngine(); }

#endif // AUDIO_ENGINE_H


