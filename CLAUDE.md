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

There are no tests or linting commands. The project builds with MSVC (Visual Studio 2022) targeting x64 Windows with C++23 (`/Zc:__cplusplus /Zc:preprocessor`).

## Architecture Overview

**be-engine** is a Windows DirectX 11 rendering engine written in C++23.

### Project Structure

- **core** - Static library containing the rendering engine core (DirectX 11 device, renderer, pipeline, shaders, textures, models, materials, input, window)
- **toolkit** - Static library with higher-level abstractions (deferred render passes, ECS via entt, ImGui integration, scene management). Links against core.
- **example-game-1**, **example-sakura** - Example ConsoleApp executables linking against core and toolkit
- **vendor** - Third-party dependencies (GLFW, GLM, Assimp, nlohmann/json, libassert, cpptrace, stb_image)

Build configuration is split across premake files: `premake5.lua` (workspace), `premake-core.lua`, `premake-toolkit.lua`, `premake-example-*.lua`, `premake-vendors.lua`, `premake-helpers.lua`.

### Core Engine (`core/src/`)

The renderer uses a pass-based architecture:
- `BeRenderer` - Main DirectX 11 renderer; manages device/context/swapchain and a sequential list of render passes. Owns a shared vertex/index buffer pool for all models (`BakeModels()` compiles them). Holds `UniformData` (camera matrix, time, ambient) passed to all shaders.
- `BeRenderPass` - Abstract base class with `Initialise()` and `Render()` virtual methods
- `BePipeline` - Shader/material binding state management with resource and sampler caching
- `BeShader` - Shader compilation and management from `.hlsl` files with embedded JSON metadata
- `BeTexture` - Builder pattern for texture creation (render targets, cubemaps, mips, color fill)
- `BeMaterial` / `BeMaterialScheme` - Material system with typed parameters (float, vec2-4, matrix, textures, samplers)
- `BeModel` - 3D model loading via Assimp. Each model contains `BeDrawSlice` entries for indexed draws.
- `BeAssetRegistry` - Static class for central asset management (textures, shaders, material schemes, samplers, models). Call `InjectRenderer()` first, then `IndexShaderFiles()`.
- `BeWindow` - GLFW window with windowed and `BorderlessFullscreen` modes
- `BeInput` - Keyboard, mouse, and gamepad input via GLFW

### Toolkit: Basic Render Pipeline (`toolkit/basic-render-pipeline/`)

Implements a deferred rendering pipeline with these passes:
- `BeShadowPass` - Shadow maps for directional (2D texture) and point lights (cubemap, 6 faces)
- `BeGeometryPass` - G-buffer population: diffuse RGB, world normals, specular+shininess, emissive
- `BeLightingPass` - Deferred lighting with shadow mapping and emissive
- `BeBloomPass` - Multi-pass Kawase bloom (bright extraction, downsample chain, upsample with blend)
- `BeFullscreenEffectPass` - Generic fullscreen post-processing (input/output texture names + shader)
- `BeBackbufferPass` - Final composite + tonemapping to swapchain
- `BeBRPSubmissionBuffer` - Per-frame collector for geometry entries (position, rotation, scale, model, shadow flag) and light entries (sun/point with shadow params)

### Scene System (`toolkit/scenes/`)

- `BeScene` - Abstract base with `OnLoad()` / `OnUnload()`
- `BeSceneManager` - Manages scenes by name. Supports pending scene changes applied once per frame via `RequestSceneChange()` / `ApplyPendingSceneChange()`.

### Shader Format (.hlsl)

Custom format combining HLSL with JSON metadata blocks in comments:
```hlsl
/*
@be-material: scheme-name
[
    "PropertyName: float3 = [1.0, 1.0, 1.0]",
    "TextureName: texture2d(slot) = default-texture",
    "SamplerName: sampler(slot) = linear-clamp",
]
@be-end

@be-shader: shader-name
{
    "topology": "triangle-list",
    "vertex": "VSEntryPoint",
    "pixel": "PSEntryPoint",
    "vertexLayout": ["position", "normal", "uv0"],
    "materials": { "slot-name": { "scheme": "scheme-name", "slot": 1 } },
    "targets": { "OutputName.Channel": 0 }
}
@be-end
*/

// HLSL code follows
```

Parsed by `BeShaderTools`. Shader `.hlsl` files have build action `None` in the VS project (compiled at runtime by the engine, not by MSVC).

### Access Modifiers Convention

The codebase uses custom macros from `core/src/umbrellas/access-modifiers.hpp`:
- `expose` = public
- `protect` = protected
- `hide` = private

Used as section markers before member declarations (e.g., `expose` instead of `public:`, without `:`).

### Include Umbrellas (`core/src/umbrellas/`)

Wrapper headers that configure third-party libraries:
- `include-glm.h` - GLM with `GLM_FORCE_LEFT_HANDED` and `GLM_FORCE_DEPTH_ZERO_TO_ONE`. Also defines `_rad` literal (degrees to radians) and `HexColor()`.
- `include-libassert.h` - Defines `be_assert(condition, ...)` macro wrapping libassert's `DEBUG_ASSERT`
- `json.h` - Aliases `nlohmann::json` as `Json`

### Example Application Pattern

See `example-sakura/Game.cpp` and `example-sakura/scenes/MainScene.cpp`:
1. Create `BeWindow` (windowed or `BorderlessFullscreen`)
2. Create `BeRenderer`, call `LaunchDevice()`
3. `BeAssetRegistry::InjectRenderer()`, create placeholder textures ("white", "black")
4. Index shader files via `BeAssetRegistry::IndexShaderFiles()`
5. Load models via `BeModel::Create()`, configure materials
6. Create G-buffer textures via `BeTexture::Create().Build()`
7. Add render passes to renderer in order
8. Register scenes, call `Prepare()`, then `BakeModels()`
9. Main loop: poll events, update input, tick active scene, render, apply pending scene change

Post-build steps copy shaders, assets, and `assimp-vc143-mt.dll` to the output directory.

### ECS

Uses entt (header-only, in `toolkit/entt/`). Components are defined per-project (e.g., `example-sakura/Components.h`: `TransformComponent`, `RenderComponent`, `NameComponent`, `SunLightComponent`, `PointLightComponent`).

### COM / DirectX Conventions

- COM objects use `Microsoft::WRL::ComPtr`
- Error handling via `ThrowIfFailed()` in `Utils.h`
- All GPU resources created through D3D11 device; context used for binding and draw calls
