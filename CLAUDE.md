# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

Generate Visual Studio solution:
```bash
./premake5 vs2022
```

Clean generated files and binaries:
```bash
./premake5 clean
```

Build vendor libraries (first-time or after vendor changes):
```bash
./premake5 cook-vendors
```

Build from command line (after generating solution):
```bash
msbuild be.sln /p:Configuration=Debug
msbuild be.sln /p:Configuration=Release
```

No tests or linting. Builds with MSVC (Visual Studio 2022), x64 Windows, C++23 (`/Zc:__cplusplus /Zc:preprocessor`).

## Project Structure

```
be-engine/
├── core/               # Static lib — rendering engine core
│   ├── Be*.h/cpp       # Engine classes (renderer, pipeline, shader, texture, mesh, etc.)
│   ├── sen-rhi/        # RHI abstraction layer (Vulkan active, DX11 legacy)
│   ├── shaders/        # Engine built-in shaders (copied to target on build)
│   └── umbrellas/      # Config headers (glm, libassert, json, access-modifiers)
├── toolkit/            # Static lib — higher-level abstractions; links against core
│   ├── basic-render-pipeline/  # Deferred rendering pipeline passes (BRP)
│   ├── scenes/         # Scene base and manager
│   ├── imgui/          # ImGui source + backends (DX11, Vulkan, GLFW)
│   └── entt/           # ECS header-only library
├── example-game-1/     # ConsoleApp — simple game example
├── example-sakura/     # ConsoleApp — advanced showcase (multi-scene, ECS, AA)
├── example-vulkan/     # ConsoleApp — minimal raw Vulkan/RHI example
├── devtools/
│   └── shader-boilerplate-autogen/  # CLI tool — generates @be-auto-boilerplate in shaders
├── build/              # Premake build scripts
│   ├── be.lua          # Top-level workspace/project declarations
│   ├── be/projects.lua # Per-project definitions
│   ├── be/default.lua  # Shared C++23 compiler flags
│   └── vendor.lua      # Vendor library build setup
├── vendor/             # Third-party libraries
├── premake5.lua        # Root build entry point (requires build/be, build/vendor)
├── premake5-vendor.lua # Vendor-only build entry point
├── premake5.exe        # Premake binary (checked in)
├── be.sln              # Main solution (core + toolkit + examples + devtools)
└── vendor.sln          # Vendor-only solution
```

## Architecture Overview

**be-engine** is a C++23 graphics engineer's workbench for Windows, primarily targeting Vulkan. The rendering architecture is pass-based: the user assembles a list of `BeRenderPass` subclasses, and the renderer executes them in order each frame.

### Core Engine (`core/`)

- **`BeRenderer`** — manages the GPU backend (Vulkan swapchain, device, queues) and an ordered list of `BeRenderPass*`. `Render()` executes all passes, each auto-wrapped in a debug annotation. Binds `UniformData` (camera matrices, time, ambient) to cbuffer `b0` before every pass.
- **`BeRenderPass`** — abstract base with virtual `Initialise()` and `Render()`. Subclasses access the backend via a protected `_renderer` pointer.
- **`BePipelineBuilder`** — fluent builder for GPU pipeline state. `BePipelineBuilder::Start(shader).SetTopology(...).SetBlend(...).Build()`. Caches by (shader, topology, rasterizer, blend, depthStencil, formats).
- **`BeShader`** — loaded from `.hlsl` files via `BeShader::Create(path)`. Parses embedded `@be-shader:` JSON metadata. Compiled at runtime via Slang to SPIR-V (Vulkan). Supports vertex, pixel, hull, and domain stages.
- **`BeShaderTools`** — parses `@be-material:` and `@be-shader:` JSON blocks from `.hlsl` comments.
- **`BeTexture`** — builder pattern: `BeTexture::Create(name).SetSize(w,h).SetFormat(...).Build()`. Supports 2D, cubemap, mipmaps, render targets, depth targets. Formats: RGBA8, BGRA8, RGBA16_Float, R11G11B10_Float, Depth32, RGB32/RGBA32/RG32_Float.
- **`BeMaterial` / `BeMaterialScheme`** — typed material properties (float, float2–4, matrix, textures, samplers). Scheme defines types/defaults from shader JSON. `BeMaterial::Create(schemeName)` instantiates from a scheme.
- **`BeMesh`** — geometry: `std::vector<BeFullVertex>` + indices + `std::vector<BeMeshSlice>` (multi-material regions).
- **`BeMeshPrimitives`** — namespace with `Plane()`, `Cube()`, `Sphere()` returning `shared_ptr<BeMesh>`.
- **`BeProp`** — renderable object: mesh + shader + per-slice materials + two-sided flags. `BeProp::Create(modelPath, shader, renderer)` (Assimp) or `BeProp::FromMesh(mesh, shader)` (procedural).
- **`BeAssetRegistry`** — static asset manager for textures, shaders, material schemes, samplers, props. Call `InjectRenderer()` first, then `IndexShaderFiles(dir)`. Returns `weak_ptr`.
- **`BeWindow`** — GLFW window wrapper. Modes: Windowed, Fullscreen, BorderlessFullscreen.
- **`BeInput`** — keyboard (`GetKey/Down/Up`), mouse (position, buttons, scroll, capture), gamepad (buttons, sticks, triggers).
- **`BeCamera`** — position, yaw/pitch in degrees, FOV, near/far. `Update()` recalculates view/projection matrices and direction vectors.
- **`BeTimer`** — frame timing (delta time, elapsed time).
- **`BeBuffers.h`** — GPU buffer type definitions.

### RHI Abstraction Layer (`core/sen-rhi/`)

Thin abstraction over the GPU backend. Active backend: **Vulkan**. DX11 backend code exists but is disabled.

- **`SenTypes.h`** — GPU type definitions: formats, usage flags, topology, sampler modes, blend/depth states, buffer access.
- **`SenBackend.h`** — selects active backend: `using SenBackend = SenVulkanBackend`.
- **`SenCommandBuffer.h`** — selects active command buffer type.
- **`SenShaderCompiler.h/cpp`** — Slang-based shader compiler; compiles HLSL to SPIR-V.
- **`vulkan/`** — Vulkan backend (`SenVulkanBackend`): device, swapchain, textures, buffers, samplers, pipelines, bind groups, descriptor sets. Uses VMA for memory.
- **`dx11/`** — DX11 backend (legacy, not used).

### Toolkit: Basic Render Pipeline (`toolkit/basic-render-pipeline/`)

Default deferred rendering pipeline. **BRP is an example, not the engine.** Users are encouraged to write their own pipelines.

Passes in typical order:
- **`BeShadowPass`** — shadow maps for directional lights (2D) and point lights (cubemap, 6 faces).
- **`BeGeometryPass`** — populates G-buffer (4 color targets + depth).
- **`BeLightingPass`** — deferred lighting with shadows and emissive.
- **`BeBloomPass`** — Kawase bloom (bright extraction → downsample chain → upsample blend).
- **`BeFullscreenEffectPass`** — generic fullscreen post-process (FXAA, SMAA, custom effects).
- **`BeBackbufferPass`** — final composite + tonemapping to swapchain.
- **`BeImGuiPass`** — renders ImGui. Accepts a `SetUICallback()` lambda.
- **`BeBRPSubmissionBuffer`** — per-frame geometry/light accumulator. `RegisterMesh()` + `BakeMeshes()` uploads all meshes to shared GPU vertex/index buffers.

#### G-Buffer Layout

| Target | Format | Contents |
|--------|--------|----------|
| 0 | `R11G11B10_FLOAT` | Diffuse RGB |
| 1 | `R16G16B16A16_FLOAT` | World Normal XYZ |
| 2 | `R8G8B8A8_UNORM` | Specular RGB + Shininess in A (`value / 2048.0`) |
| 3 | `R11G11B10_FLOAT` | Emissive RGB |
| Depth | `R32_TYPELESS` | Depth |

### Cbuffer Slot Convention

- `b0` — renderer uniform buffer (camera, time, ambient) — always bound by `BeRenderer`
- `b1` — object material buffer (model matrix) — bound by geometry pass
- `b2+` — surface material buffers (per-shader properties)

### Scene System (`toolkit/scenes/`)

- **`BeScene`** — abstract base with `OnLoad()` / `OnUnload()`.
- **`BeSceneManager`** — named scene registry. `RequestSceneChange()` / `ApplyPendingSceneChange()` for deferred transitions. Typed accessors: `GetActiveScene<T>()`, `GetScene<T>()`.

### Shader Format (.hlsl)

Custom format: HLSL with JSON metadata in block comments. Compiled at runtime by Slang.

```hlsl
/*
@be-material: scheme-name
[
    "PropertyName: float3 = [1.0, 1.0, 1.0]",
    "TextureName: texture2d = default-texture-name",
    "SamplerName: sampler = linear-clamp",
]
@be-end

@be-shader: shader-name
{
    "topology": "triangle-list",
    "rasterizer": "back-solid",
    "blend": "disable",
    "depthStencil": "less",
    "vertex": "VSMain",
    "pixel": "PSMain",
    "vertexLayout": ["position", "normal", "uv0"],
    "materials": { "link-name": { "scheme": "scheme-name", "slot": 2 } },
    "targets": { "OutputName": { "type": "float3", "slot": 0 } }
}
@be-end
*/
```

Tessellation uses `"topology": "patch-list-3"` and adds `"hull"` / `"domain"` keys inside the shader JSON.

`@be-auto-boilerplate` regions in `.hlsl` files are **generated by the CLI tool**, not hand-written. They are codegen artifacts — never edit them manually.

#### Rasterizer Presets
`back-solid`, `back-wireframe`, `front-solid`, `none-solid`, etc.

#### Blend Presets
`disable`, `alpha` (src-alpha/inv-src-alpha), `additive` (one/one), `multiply` (dst-color/zero).

#### Depth-Stencil Presets
`less`, `lequal`, `equal`, `greater`, `always`, `disable`.

### Sampler String Format

`BeAssetRegistry::GetSampler()` parses `"filter-address[-cmp]"`. Filter: `point` or `linear`. Address: `wrap`, `clamp`, `mirror`. Optional `-cmp` for shadow map comparison. Examples: `"linear-clamp"`, `"linear-clamp-cmp"`, `"point-wrap"`.

### Vertex Layout

`BeFullVertex`: Position (vec3), Normal (vec3), Color (vec4, default white), UV0 (vec2), UV1 (vec2), UV2 (vec2). Shader JSON vertex semantic names: `"position"`, `"normal"`, `"color3"`, `"color4"`, `"uv0"`, `"uv1"`, `"uv2"`.

### Access Modifiers Convention

Custom macros in `core/umbrellas/access-modifiers.hpp` — used as section markers **without** a colon:
- `expose` = `public:`
- `protect` = `protected:`
- `hide` = `private:`

### Include Umbrellas (`core/umbrellas/`)

- `include-glm.h` — GLM with `GLM_FORCE_LEFT_HANDED` + `GLM_FORCE_DEPTH_ZERO_TO_ONE`. Defines `_rad` literal (degrees→radians) and `HexColor("#RRGGBB")` → `glm::vec3`.
- `include-libassert.h` — `be_assert(condition, ...)` wrapping libassert's `DEBUG_ASSERT`.
- `json.h` — aliases `nlohmann::json` as `Json`.
- `bitmask.hpp` — `ENABLE_BITMASK(EnumType)` for bitwise ops on scoped enums.

### Coordinate System

Left-handed. All geometry (Assimp-loaded and procedural) **negates X** on positions and normals.

### Error Handling

- `be_assert(condition, ...)` — debug assertions via libassert (preferred).
- `Utils::Check << hrResult1 << hrResult2` — streaming HRESULT checker, throws on failure.
- `ThrowIfFailed(hr)` — single HRESULT check.

### ECS

entt (header-only, `toolkit/entt/entt.hpp`). Components defined per-project. Example in `example-sakura/Components.h`: `TransformComponent`, `RenderComponent`, `NameComponent`, `SunLightComponent`, `PointLightComponent`.

### Devtools: Shader Boilerplate Autogen

`devtools/shader-boilerplate-autogen.exe` scans `.hlsl` files, reads `@be-material:` JSON, and generates matching cbuffer/texture/sampler declarations into `@be-auto-boilerplate` regions.

```bash
# Single file
devtools/shader-boilerplate-autogen.exe --once path/to/shader.hlsl
# Watch mode (live polling)
devtools/shader-boilerplate-autogen.exe --watch assets/shaders/
```

### Vendor Libraries

| Library | Purpose |
|---------|---------|
| GLFW | Window + input |
| GLM | Math (vectors, matrices) |
| Assimp | 3D model loading (FBX, GLTF, OBJ) |
| Slang | Shader compilation → SPIR-V |
| nlohmann/json | JSON parsing |
| libassert + cpptrace | Assertions + stack traces |
| stb_image | Image loading |
| scope_guard | RAII guards |
| entt | ECS |
| ImGui | Debug/editor UI |
| Vulkan SDK | Graphics API |
| VMA | Vulkan memory allocator |
