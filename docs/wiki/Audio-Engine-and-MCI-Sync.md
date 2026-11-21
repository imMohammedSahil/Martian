# Audio Engine and MCI Hardware Synchronization

## 1. WinMM MCI Subsystem

Audio playback runs via the Win32 Multimedia API (`mmsystem.h`) using Media Control Interface (MCI) commands:

```cpp
mciSendStringA("open \"soundtrack.mp3\" type mpegvideo alias bgm", NULL, 0, NULL);
mciSendStringA("play bgm", NULL, 0, NULL);
```

---

## 2. Drift Elimination & Polling Discipline

1. **No Per-Frame Status Inquiries**: Calling `mciSendStringA("status bgm position", ...)` every frame blocks the main thread for 1 to 4 ms depending on the audio driver, resulting in micro-stuttering and dropping framerates below 60 FPS.
2. **Clock Source**: The engine uses high-resolution Win32 performance counters (`QueryPerformanceCounter`) as the master timeline clock, syncing audio only at milestone scene transitions.
3. **Seamless Transitions**: The Scene 3 entry transition at 44.0s is locked without forced seek commands to avoid audio pop artifacts.
