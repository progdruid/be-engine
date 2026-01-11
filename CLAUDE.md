# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

Generate Visual Studio solution:
```bash
./premake5 vs2022
```

Clean generated files:
```bash
./premake5 clean
```

Build from command line (after generating solution):
```bash
msbuild be.sln /p:Configuration=Debug
msbuild be.sln /p:Configuration=Release
```

## Architecture Overview

**be-engine** is a Windows DirectX 11 rendering engine written in C++23.

### Project Structure

- **core** - Static library containing the rendering engine core
- **toolkit** - Static library with higher-level abstractions (render passes, ECS via entt, ImGui, scenes)
- **example-game-1**, **example-sakura** - Example applications linking against core and toolkit
- **vendor** - Third-party dependencies (glfw, glm, Assimp, nlohmann/json, libassert, stb_image)

### Core Engine Components

The renderer uses a pass-based architecture:
- `BeRenderer` - Main DirectX 11 renderer, manages device/context/swapchain and render passes
- `BeRenderPass` - Abstract base class for render passes (Initialise/Render virtual methods)
- `BePipeline` - Shader/material binding state management with caching
- `BeShader` - Shader compilation and management, reads `.hlsl` files
- `BeTexture` - Texture creation with builder pattern, supports render targets and cubemaps
- `BeMaterial` / `BeMaterialScheme` - Material system with typed parameters
- `BeModel` - 3D model loading via Assimp
- `BeAssetRegistry` - Central asset management (textures, shaders, material schemes)

### Toolkit: Basic Render Pipeline (BRP)

Located in `toolkit/basic-render-pipeline/`, implements a deferred rendering pipeline:
- `BeShadowPass` - Shadow map generation for directional and point lights
- `BeGeometryPass` - G-buffer population (diffuse, normals, specular)
- `BeLightingPass` - Deferred lighting with shadow mapping
- `BeBloomPass` - Multi-pass Kawase bloom
- `BeFullscreenEffectPass` - Generic fullscreen post-processing
- `BeBackbufferPass` - Final composite to swapchain
- `BeBRPSubmissionBuffer` - Collects geometry and light submissions per frame

### Shader Format (.hlsl)

Custom shader format combining HLSL with JSON metadata in comments:
```hlsl
/*
@be-material: scheme-name
[
    "PropertyName: type = default",
    "TextureName: texture2d(slot) = default-texture",
    "SamplerName: sampler(slot) = linear-clamp",
]
@be-end

@be-shader: shader-name
{
    "topology": "triangle-list",
    "vertex": "EntryPoint",
    "pixel": "EntryPoint",
    "vertexLayout": ["position", "normal", "uv0"],
    "materials": { ... },
    "targets": { ... }
}
@be-end
*/

// HLSL code follows
```

### Access Modifiers Convention

The codebase uses custom access modifier macros (`umbrellas/access-modifiers.hpp`):
- `expose` - public
- `protect` - protected
- `hide` - private

These are used as section markers before member declarations.

### Example Application Pattern

See `example-sakura/scenes/MainScene.cpp` for the typical usage pattern:
1. Create textures for G-buffer and post-processing via `BeTexture::Create().Build()`
2. Index shader files via `BeAssetRegistry::IndexShaderFiles()`
3. Load models via `BeModel::Create()`
4. Configure and add render passes to renderer
5. Use entt registry for ECS-based scene management
6. Submit geometry/lights to `BeBRPSubmissionBuffer` each frame
