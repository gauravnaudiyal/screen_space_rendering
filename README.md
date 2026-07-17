# Screen Space Rendering 🎮✨

A graphics project exploring **screen space rendering techniques** implemented in C++ with OpenGL. Includes a detailed written report describing the methodology, implementation decisions, and results.

## Overview

Screen space rendering refers to post-processing effects applied after the main geometry pass using information stored in the screen (G-buffer). This project implements and evaluates one or more screen space techniques to demonstrate their visual impact and performance trade-offs.

## Techniques Explored

- **Screen Space Ambient Occlusion (SSAO)** — simulates soft shadows in crevices for added depth
- **Screen Space Reflections (SSR)** — real-time approximate reflections using depth/normal buffers
- **Deferred Rendering Pipeline** — G-buffer setup for efficient multi-pass shading
- Post-processing effects: bloom, tone mapping, gamma correction

## Tech Stack

| Tool | Purpose |
|------|---------|
| C++ | Core language |
| OpenGL | Rendering API |
| GLSL | Shader language |
| GLFW | Window/context management |
| GLM | Mathematics library |

## Repository Structure

```
screen_space_rendering/
├── src/          # C++ source files
├── shaders/      # GLSL vertex and fragment shaders
├── assets/       # Models and textures
├── report/       # Project report (PDF)
└── README.md
```

## Getting Started

1. Clone the repo:
   ```bash
   git clone https://github.com/gauravnaudiyal/screen_space_rendering.git
   ```
2. Ensure you have a C++17-compatible compiler and OpenGL 4.x support
3. Install dependencies (GLFW, GLAD, GLM)
4. Build with CMake:
   ```bash
   mkdir build && cd build
   cmake ..
   make
   ./screen_space_rendering
   ```

> 📝 **See the report** in the `/report` folder for a full explanation of the implementation and visual results.

## Academic Context

Completed as part of the **Real-Time Rendering** module in the MSc Computer Science programme at Trinity College Dublin.

---

*Developed by [Gaurav Naudiyal](https://github.com/gauravnaudiyal) — MSc Computer Science, Trinity College Dublin*
