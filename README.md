# Solar System Simulation — OpenGL / C++ Final Evaluation Project

An interactive, real-time **Solar System Simulation** built with **OpenGL 3.3** and **C++17**.

## Scene Description

| Body      | Highlight |
|-----------|-----------|
| ☀️ Sun    | Emissive yellow sphere at the origin — acts as the scene's point-light source |
| ☿ Mercury | Smallest, fastest-orbiting inner planet |
| ♀ Venus   | Slow retrograde self-rotation |
| 🌍 Earth  | Blue-green with an orbiting Moon (hierarchical transform) |
| ♂ Mars    | Red planet |
| ♃ Jupiter | Largest planet, rapid self-rotation |
| ♄ Saturn  | With a procedural flat ring |
| ⛢ Uranus  | Tilted rotation axis |
| ♆ Neptune | Outermost planet |

## OpenGL Techniques Demonstrated

- Procedural UV-sphere geometry (VAO / VBO / EBO)
- **Blinn-Phong lighting** — ambient + diffuse + specular
- Separate **emissive shader** for the Sun
- **Hierarchical model matrices** — Moon orbiting Earth
- **Perspective projection** via GLM
- Delta-time-based animation
- 4× MSAA anti-aliasing
- Wireframe toggle

## Controls

| Key / Input    | Action |
|----------------|--------|
| W / S          | Move forward / backward |
| A / D          | Strafe left / right |
| Q / E          | Fly up / down |
| Mouse          | Look around (cursor captured) |
| Scroll wheel   | Zoom (change FOV) |
| F              | Toggle wireframe mode |
| SPACE          | Pause / resume orbital animation |
| ESC            | Quit |

## Dependencies

| Library | Version tested | Purpose |
|---------|---------------|---------|
| [GLFW](https://www.glfw.org/)   | 3.3+ | Window & input |
| [GLEW](http://glew.sourceforge.net/) | 2.2+ | OpenGL function loader |
| [GLM](https://github.com/g-truc/glm) | 0.9.9+ | Math (vectors, matrices) |
| OpenGL | 3.3 Core | Graphics API |

### Install on Ubuntu / Debian

```bash
sudo apt-get install libglfw3-dev libglew-dev libglm-dev libgl-dev cmake
```

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

## Run

Run the executable **from the project root** (so that the `shaders/` folder is on a
relative path), or from the build directory where CMake copies the shaders:

```bash
cd build
./SolarSystemOpenGL
```

## Project Structure

```
.
├── CMakeLists.txt          Build configuration
├── README.md
├── shaders/
│   ├── planet.vert         Vertex shader  – Blinn-Phong lit bodies
│   ├── planet.frag         Fragment shader – Blinn-Phong lit bodies
│   ├── sun.vert            Vertex shader  – emissive Sun
│   └── sun.frag            Fragment shader – emissive Sun
└── src/
    ├── main.cpp            Scene setup, render loop, input
    ├── Shader.h / .cpp     GLSL shader loader & uniform helpers
    └── Camera.h / .cpp     Free-look FPS camera
```