# Screen Space Rendering

A graphics project implementing screen space rendering techniques in C++ with OpenGL. There is a written report in the repo that goes into the full details of the implementation and results, so I will keep this short.

The project focuses on effects applied in screen space after the main geometry pass, using data from the G-buffer rather than re-rendering geometry. The goal was to understand how these techniques work under the hood and what the visual and performance trade-offs actually look like.

## Techniques covered

- Screen Space Ambient Occlusion (SSAO)
- Screen Space Reflections (SSR)
- Deferred rendering pipeline with G-buffer setup
- Post-processing: bloom, tone mapping, gamma correction

## Built with

- C++
- OpenGL 4.x
- GLSL shaders
- GLFW for window management
- GLM for maths

## Building the project

```bash
git clone https://github.com/gauravnaudiyal/screen_space_rendering.git
cd screen_space_rendering
mkdir build && cd build
cmake ..
make
./screen_space_rendering
```

You will need GLFW, GLAD, and GLM installed, or available in the project directory.

Read the report in `/report` for a proper explanation of what was implemented and why certain decisions were made.

Developed by Gaurav Naudiyal, MSc Computer Science, Trinity College Dublin.
