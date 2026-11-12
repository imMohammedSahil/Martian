# Project MARTIAN: Cinematic Space Sequence (OpenGL / GLUT)

> **Interstellar & Gravity-Grade Cinematic Space Visualizer**  
> *Native C++ / FreeGLUT project pre-configured for Code::Blocks (MinGW 64-bit).*

---

## 1. Quick Start

### Run Immediately:
- Double-click [`Martian.exe`](file:///c:/Holidays/Martian/Martian.exe). It is statically compiled and launches directly into **Fullscreen Theater Mode**.

### Build & Run in Code::Blocks:
1. Double-click [`Martian.cbp`](file:///c:/Holidays/Martian/Martian.cbp) or open it from Code::Blocks.
2. Press **`F9`** (or click `Build -> Build and run`).
3. Zero setup needed: all relative include paths (`include/`), library search paths (`lib/`), and static libraries (`freeglut_static`, `opengl32`, `glu32`, `winmm`, `gdi32`) are pre-configured.

### Theater Controls:
- **`ESC`**: Exit fullscreen theater cleanly.
- **`SPACE`**: Silent freeze-frame pause / resume (zero UI overlays).

---

## 2. Cinematic 42-Second Sequence Architecture

The program runs a continuous, choreographed multi-cut movie sequence (2.39:1 anamorphic letterbox, hidden cursor, zero HUD clutter):

| Shot | Timecode | Scene / Action | Highlights |
| :--- | :--- | :--- | :--- |
| **Shot 1** | `0.0s – 10.0s` | **Dark-Side Eclipse & Title** | Mars night-side sweep, blazing sunrise, anamorphic blue/gold lens flare, and sleek `M A R T I A N` title card (`0.4s – 4.4s`). |
| **Shot 2** | `10.0s – 20.0s` | **Endurance Ring Sweep** | 12-module rotating *Endurance* ring ship orbiting against Mars's curved horizon with planetary bounce light (`GL_LIGHT1`) and moon Phobos. |
| **Shot 3** | `20.0s – 30.0s` | **Ranger Separation** | Hull-mounted camera tracking the *Ranger* lander undocking from the hub, cold-gas RCS puffs, pitch-down maneuver, and blue de-orbit burn. |
| **Shot 4** | `30.0s – 42.0s` | **Hypersonic Atmospheric Entry** | High-speed entry chase, incandescent belly heatshield, plasma bow shock, ionized flame embers, high-velocity dust streaks, and IMAX buffeting camera shake. |

---

## 3. Technical & Computer Graphics Features

| Feature | Technical Implementation |
| :--- | :--- |
| **Anamorphic Lens Flare** | Real-time `gluProject` 2D projection of the Sun with analytical ray-sphere Mars occlusion testing. Renders wide horizontal ice-blue anamorphic streaks, golden core beam, radial bloom, and 4 hexagonal optical lens ghosts. |
| **Dual-Light Realism** | Primary directional solar key light (`GL_LIGHT0`) + secondary warm Martian rust bounce light (`GL_LIGHT1`) illuminating the undersides of orbiting vessels. |
| **Sun-Weighted Atmosphere** | Dual-layer additive scattering shells (`1.018 * R` and `1.052 * R`) modulated per-vertex by `dot(normal, sunDir)` to create a radiant twilight limb while keeping the night rim deep cosmic indigo. |
| **In-Memory Audio Synthesizer** | Procedurally synthesizes a 16-second seamless, click-free Hans Zimmer-inspired pipe organ chord (D2, A2, D3, F3, A3, E4) and spacecraft sub-rumble directly into RAM; plays silently via `PlaySoundA` (`winmm`). |
| **Spacecraft Models** | 12-module rotating Endurance ring ship with radiator fins, spoke trusses, viewport glows, and strobe beacons + Ranger lifting-body lander with thermal emission and plasma shock cones. |
| **Stellar Skybox** | 4,200 cold-palette stars (cyan, blue, ice-white, gold) with sine-wave twinkling and deep cosmic dust band. |

---

## 4. Project Structure

```text
c:\Holidays\Martian\
├── Martian.cbp            <-- Code::Blocks project file
├── main.cpp               <-- Complete C++ Interstellar sequence source code
├── Martian.exe            <-- Standalone ready-to-run executable (statically linked)
├── mars_surface.jpg       <-- High-res 2048x1024 photographic surface map
├── include\               <-- Headers (GL/freeglut.h, stb_image.h)
├── lib\                   <-- Static libraries (libfreeglut_static.a)
├── bin\                   <-- Compiled binaries (Debug/Martian.exe, Release/Martian.exe)
├── HANDOFF.md             <-- Technical project handoff document
├── README_OPENGL.md       <-- This document
└── reference_archive\     <-- Preserved reference files (Blender 3D, Three.js)
```
