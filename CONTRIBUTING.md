# Contributing to MARTIAN

Thank you for your interest in contributing to **MARTIAN: Cinematic Space Sequence Engine**!

## How to Contribute

1. **Fork the Repository**: Create your personal fork on GitHub.
2. **Clone & Branch**:
   ```bash
   git clone https://github.com/<your-username>/Martian.git
   cd Martian
   git checkout -b feature/your-feature-name
   ```
3. **Build Locally**:
   Run `build.bat` using a MinGW-w64 64-bit toolchain. Verify that compilation succeeds with zero errors.
4. **Code Standards**:
   - Modern C++17 compatible with OpenGL 2.1 / compatibility profile.
   - Preserve frame accuracy across the 60 FPS timeline pacing.
   - Offscreen FBO rendering (`VideoExporter`) must remain compatible with FFmpeg NVENC/libx264.
5. **Submit a Pull Request**:
   Push your branch and open a Pull Request describing your changes, with test screenshots or short video clips where applicable.

## Reporting Issues

Please use GitHub Issues to report bugs or request features. Include your OS version, GPU hardware, and driver details when reporting rendering anomalies.
