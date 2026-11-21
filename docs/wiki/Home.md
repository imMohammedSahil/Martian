# Martian Engine Developer Wiki

Welcome to the internal technical specifications and architectural documentation for the **Martian** real-time cinematic engine.

---

## Technical Index

- [Cinematic Sequence and Timeline Pacing](Cinematic-Sequence-Timeline.md)
- [OpenGL Rendering and Optical Pipeline](OpenGL-Rendering-Pipeline.md)
- [Audio Engine and MCI Hardware Synchronization](Audio-Engine-and-MCI-Sync.md)
- [Binary Mesh and Asset Pipeline](Binary-Mesh-and-Asset-Pipeline.md)

---

## Architectural Summary

Martian is a real-time cinematic graphics visualizer running an 84.3-second continuous sequence recreating an interplanetary approach and landing:

- **Language & Standard**: C++17 (`-std=c++17`).
- **Graphics API**: OpenGL 2.1 / Compatibility Profile using FreeGLUT (`-DFREEGLUT_STATIC`).
- **Audio Subsystem**: Win32 Windows Multimedia MCI streaming MP3 with sub-millisecond audio pacing.
- **Export Pipeline**: Hardware-accelerated offscreen FBO pipe writing directly to FFmpeg for 60 FPS 4K render capture.
