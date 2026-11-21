# Martian Engine — Agent & Developer Instructions

This document defines core architectural conventions and technical constraints for developers and AI agents working on **Martian**.

---

## 1. Core Architecture & Standards

- **Language Standard**: C++17 (`-std=c++17`). Use ISO-standard constructs; avoid compiler-specific non-portable extensions.
- **Graphic Architecture**: OpenGL 2.1 / Compatibility Profile driven by FreeGLUT statically linked (`-DFREEGLUT_STATIC`).
- **Aspect Ratio**: The theatrical sequence renders in 2.39:1 anamorphic format. Viewport clipping and letterboxing must use hardware scissor tests (`glScissor`, `glViewport`) rather than soft software overlays.
- **Framerate Target**: 60 FPS fixed delta timing (`16.666ms`). Time integration must strictly use delta timestamps supplied via `TimerSubsystem` to prevent drift across variable monitor refresh rates.

---

## 2. Rendering & State Discipline

1. **State Cleanliness**: Always restore OpenGL state when exiting custom render routines (pop projection/modelview matrices, restore depth mask, re-enable standard blending modes).
2. **Zero Allocation in Render Loops**: Never invoke `new`, `malloc`, or dynamic vector resizing (`std::vector::push_back` exceeding capacity) inside scene `render()` or `update()` routines. Pre-allocate all buffers at scene initialization.
3. **Binary Meshes**: Mesh assets (`.bin`) utilize pre-baked interleaved vertex buffers. Do not parse raw `.obj` or `.stl` at runtime; execute conversion scripts if geometry needs updating.

---

## 3. Audio Synchronization

1. **WinMM MCI Subsystem**: Audio streaming executes via `mciSendStringA` on Windows Multimedia API.
2. **Polling Discipline**: Never query MCI status every frame; polling the WinMM driver inside a 60 FPS loop introduces micro-stutters. Track audio state via event callbacks or coarse elapsed time checks.
3. **Timeline Cut Alignment**:
   - Scene 1 $\to$ 2: 24.0s
   - Scene 2 $\to$ 3: 44.0s (locked hypersonic entry)
   - Scene 3 $\to$ 4: 66.0s (descent to dune reveal)
   - Finale / Freeze: 84.3s

---

## 4. Build System & Toolchain

- Supported compiler: MinGW64 GCC 13+ on Windows.
- Build command: `build.bat` executed from repo root. Output binary placed in `bin/` or root `Martian.exe`.
