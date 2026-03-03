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

- **core** - Static library containing the rendering engine core (DirectX 11 device, renderer, pipeline, shaders, textures, meshes, props, materials, input, window)
- **toolkit** - Static library with higher-level abstractions (deferred render passes, ECS via entt, ImGui integration, scene management). Links against core.
- **example-game-1**, **example-sakura** - Example ConsoleApp executables linking against core and toolkit
- **vendor** - Third-party dependencies (GLFW, GLM, Assimp, nlohmann/json, libassert, cpptrace, stb_image, scope_guard, entt, ImGui)

Build configuration is split across premake files: `premake5.lua` (workspace), `premake-core.lua`, `premake-toolkit.lua`, `premake-example-*.lua`, `premake-vendors.lua`, `premake-helpers.lua`. `premake5.exe` is checked into the repo root.

### Core Engine (`core/src/`)

The renderer uses a pass-based architecture:
- `BeRenderer` - Main DirectX 11 renderer; manages device/context/swapchain and a sequential list of render passes. Binds `UniformData` (camera matrix, time, ambient) to cbuffer slot `b0` before every pass.
- `BeRenderPass` - Abstract base class with `Initialise()` and `Render()` virtual methods. Each pass is auto-wrapped in a debug annotation event (for RenderDoc labeling).
- `BePipeline` - Shader/material binding state management with resource and sampler caching
- `BeShader` - Shader compilation and management from `.hlsl` files with embedded JSON metadata. Supports vertex, pixel, hull, and domain shaders.
- `BeTexture` - Builder pattern for texture creation (render targets, cubemaps, mips, color fill)
- `BeMaterial` / `BeMaterialScheme` - Material system with typed parameters (float, vec2-4, matrix, textures, samplers)
- `BeMesh` - Raw geometry data (vertices, indices, slices). Loaded via Assimp or generated from `BeMeshPrimitives`.
- `BeProp` - A renderable object combining a mesh + shader + per-slice materials/two-sided flags. Created via `BeProp::Create()` (from file) or `BeProp::FromMesh()` (from procedural mesh).
- `BeMeshPrimitives` - Namespace with `Plane()`, `Cube()`, `Sphere()` functions returning `shared_ptr<BeMesh>`
- `BeAssetRegistry` - Static class for central asset management (textures, shaders, material schemes, samplers, props). Call `InjectRenderer()` first, then `IndexShaderFiles()`.
- `BeWindow` - GLFW window with windowed and `BorderlessFullscreen` modes
- `BeInput` - Keyboard, mouse, and gamepad input via GLFW

### Toolkit: Basic Render Pipeline (`toolkit/basic-render-pipeline/`)

Implements a deferred rendering pipeline with these passes:
- `BeShadowPass` - Shadow maps for directional (2D texture) and point lights (cubemap, 6 faces)
- `BeGeometryPass` - G-buffer population: diffuse RGB, world normals, specular+shininess, emissive. Handles two-sided geometry per-slice.
- `BeLightingPass` - Deferred lighting with shadow mapping and emissive
- `BeBloomPass` - Multi-pass Kawase bloom (bright extraction, downsample chain, upsample with blend)
- `BeFullscreenEffectPass` - Generic fullscreen post-processing (input/output texture names + shader)
- `BeBackbufferPass` - Final composite + tonemapping to swapchain
- `BeImGuiPass` - ImGui rendering pass; accepts a `SetUICallback()` lambda for draw calls
- `BeBRPSubmissionBuffer` - Per-frame collector for geometry and light entries. Also owns GPU geometry baking: `RegisterMesh()` then `BakeMeshes()` to upload all meshes into a single shared vertex/index buffer.

### G-Buffer Layout

- Target 0 (`R11G11B10_FLOAT`): Diffuse RGB
- Target 1 (`R16G16B16A16_FLOAT`): World Normal XYZ
- Target 2 (`R8G8B8A8_UNORM`): Specular RGB + Shininess in A (shininess = `value / 2048.0`)
- Target 3 (`R11G11B10_FLOAT`): Emissive RGB
- Depth (`R32_TYPELESS`): depth/stencil

### Cbuffer Slot Convention

- `b0` - Renderer uniform buffer (camera matrices, time, ambient) - always bound by `BeRenderer`
- `b1` - Object material buffer (model matrix, etc.) - bound by geometry pass infrastructure
- `b2` - Surface material buffer (per-shader material properties)

### Scene System (`toolkit/scenes/`)

- `BeScene` - Abstract base with `OnLoad()` / `OnUnload()`
- `BeSceneManager` - Manages scenes by name. Supports pending scene changes applied once per frame via `RequestSceneChange()` / `ApplyPendingSceneChange()`. Has typed accessors `GetActiveScene<T>()` and `GetScene<T>()`.

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
```

Tessellation is supported via a `"tesselation"` key in the shader metadata:
```json
{
    "topology": "patch-list-3",
    "tesselation": { "hull": "HullFunction", "domain": "DomainFunction" }
}
```

Parsed by `BeShaderTools`. Shader `.hlsl` files have build action `None` in the VS project (compiled at runtime by the engine, not by MSVC).

### Sampler String Format

`BeAssetRegistry::GetSampler()` creates samplers from a hyphen-delimited string: `"filter-address"` or `"filter-address-cmp"`. Filter: `point` or `linear`. Address: `wrap`, `clamp`, `mirror`. Optional `-cmp` suffix enables comparison mode for shadow maps. Examples: `"linear-clamp"`, `"linear-clamp-cmp"`.

### Vertex Layout

`BeFullVertex`: Position (vec3), Normal (vec3), Color (vec4, default white), UV0 (vec2), UV1 (vec2), UV2 (vec2). Vertex semantic names for shader JSON: `"position"`, `"normal"`, `"color3"`, `"color4"`, `"uv0"`, `"uv1"`, `"uv2"`.

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

### Coordinate System Convention

Left-handed coordinate system. All geometry (both Assimp-loaded and procedural primitives) **negates the X axis** on positions and normals (`vertex.Position = {-position.x, position.y, position.z}`).

### Error Handling

- `Utils::Check << hrResult1 << hrResult2` - streaming HRESULT checker that throws `com_exception` on failure. Preferred for COM call chains.
- `ThrowIfFailed(hr)` - single HRESULT check
- `be_assert(condition, ...)` - debug assertions via libassert
- `ENABLE_BITMASK(EnumType)` macro in `Utils.h` for bitwise ops on scoped enums

### Example Application Pattern

See `example-sakura/Game.cpp` and `example-sakura/scenes/`:
1. Create `BeWindow` (windowed or `BorderlessFullscreen`)
2. Create `BeRenderer`, call `LaunchDevice()`
3. `BeAssetRegistry::InjectRenderer()`, create placeholder textures ("white", "black")
4. Index shader files via `BeAssetRegistry::IndexShaderFiles()`
5. Load props via `BeProp::Create()`, configure materials
6. Create G-buffer textures via `BeTexture::Create().Build()`
7. Create `BeBRPSubmissionBuffer`, call `RegisterMesh()` for each prop's mesh, then `BakeMeshes()`
8. Add render passes to renderer in order
9. Register scenes, request initial scene
10. Main loop: poll events, update input, tick active scene, render, apply pending scene change

### COM / DirectX Conventions

- COM objects use `Microsoft::WRL::ComPtr`
- All GPU resources created through D3D11 device; context used for binding and draw calls

### ECS

Uses entt (header-only, in `toolkit/entt/`). Components are defined per-project (e.g., `example-sakura/Components.h`: `TransformComponent`, `RenderComponent`, `NameComponent`, `SunLightComponent`, `PointLightComponent`).
