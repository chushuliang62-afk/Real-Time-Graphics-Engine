# Real-Time Graphics Engine

A real-time 3D rendering engine built with OpenGL 3.3+, featuring a "Two Worlds" temporal scene that transitions between an ancient Chinese courtyard and a modern-era environment.

## Features

### Rendering Pipeline
- Forward + Deferred rendering with G-Buffer MRT
- Shadow mapping with 5x5 PCF soft shadows (2048x2048)
- Bloom post-processing with Gaussian blur
- Time transition effects between two world states

### Environmental Systems
- Day/night cycle with dynamic skybox switching
- Water rendering with dual-layer normal mapping and Fresnel reflections
- GPU instanced grass with wind animation
- Particle system for rain/snow weather effects
- Procedural terrain with multi-texture blending (sand, rock, mud)

### Procedural Generation
- Voxel-based tree system with configurable density
- Procedural tree generation with foliage placement
- Terrain-aware vegetation positioning

### Asset Rendering
- GLTF model loading (Chinese temple, animated animals)
- Skeletal animation system
- Multiple material support (roof tiles, stone walls, wood)

### Scene Management
- Scene graph with node-based transformations
- FBO manager for off-screen rendering
- Shader hot-reloading (F5)
- Fullscreen cubemap skybox

## Tech Stack

- **Language:** C++
- **Graphics API:** OpenGL 3.3+
- **Framework:** NCLGL (Newcastle Graphics Library)
- **Model Format:** GLTF
- **Texture Loading:** SOIL
- **Build System:** Visual Studio / CMake

## Controls

| Key | Action |
|-----|--------|
| WASD | Camera movement |
| Mouse | Look around |
| F5 | Reload shaders |
| T | Toggle time transition |

## Build

Open `GraphicsTutorials.sln` in Visual Studio and build the `Coursework` project.

## Project Structure

```
Coursework/          # Main application
  Renderer.cpp/h     # Core renderer
  DeferredRenderer   # Deferred pipeline
  ShadowSystem       # Shadow mapping
  WaterRenderer      # Water effects
  GrassRenderer      # Instanced grass
  ParticleSystem     # Weather particles
  DayNightCycle      # Lighting cycle
  SkeletalAnimator   # Character animation
  TreeSystem         # Procedural trees
  BlockRenderer      # Voxel rendering
Shaders/             # GLSL shaders
Textures/            # Texture assets
Meshes/              # 3D model files
nclgl/               # Graphics framework
```
