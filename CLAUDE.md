# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

**Linux (active platform)** — uses CMake with Ninja (requires CMake 3.25+):
```bash
# Configure
cmake --preset linux-debug    # or linux-release, or linux-debug-hotreload
# Build
cmake --build out/linux-debug
```

**Windows** — uses CMake with Visual Studio 2022 or premake5:
```bash
# CMake
cmake --preset windows
cmake --build out/windows --config Debug

# Premake (legacy)
./premake5 vs2022
msbuild be.sln /p:Configuration=Debug
```

No tests or linting. C++23 (`/Zc:__cplusplus /Zc:preprocessor` on MSVC). Build outputs land in `out/<preset>/`.

`example-sakura` is the up-to-date showcase; prefer it as the reference. `example-game-1` is outdated and may not build.

## Project Structure

```
be-engine/
├── core/               # Static lib — rendering engine core
│   ├── Be*.h/cpp       # Engine classes
│   ├── sen-rhi/        # RHI abstraction (Vulkan active, DX11 legacy/disabled)
│   └── umbrellas/      # Config headers (glm, libassert, json, common)
├── toolkit/            # Static lib — higher-level abstractions; links against core
│   ├── standard-render-machine/  # Deferred rendering pipeline (SRM)
│   ├── assimp-import/  # BeAssimpImporter — Assimp-based model loading
│   ├── scenes/         # BeScene base + BeSceneManager
│   ├── imgui/          # ImGui source + BeImGuiPass + backends
│   └── entt/           # ECS header-only library
├── example-game-1/     # Simple game example
├── example-sakura/     # Advanced showcase (multi-scene, ECS, SRM)
├── example-vulkan/     # Minimal raw Vulkan/RHI example
├── devtools/
│   └── bechef/         # CLI — content deploy, workspace check, shader boilerplate autogen
└── vendor/             # Third-party libraries
```

## Architecture Overview

**be-engine** is a C++23 graphics engineer's workbench targeting Vulkan. The rendering architecture is pass-based: the user assembles `BeRenderPass` subclasses and the renderer executes them in order each frame.

### Core Engine (`core/`)

- **`BeRenderer`** — manages Vulkan swapchain/device/queues and an ordered list of `BeRenderPass*`. `Render()` executes all passes wrapped in debug annotations. Binds `UniformData` (camera matrices, time, ambient) to cbuffer `b0` before every pass.
- **`BeRenderPass`** — abstract base with virtual `Initialise()` and `Render()`. Subclasses access the backend via protected `_renderer`.
- **`BePipelineBuilder`** — fluent builder for GPU pipeline state. `BePipelineBuilder::Start(shader).SetTopology(...).SetBlend(...).Build()`. Caches by (shader, topology, rasterizer, blend, depthStencil, formats).
- **`BeShader`** — loaded from `.hlsl` via `BeShader::Create(path)`. Parses embedded `@be-shader:` JSON. Compiled at runtime by Slang to SPIR-V. Supports vertex, pixel, hull, domain stages.
- **`BeShaderTools`** — parses `@be-material:` and `@be-shader:` JSON blocks from `.hlsl` comments.
- **`BeTexture`** — builder: `BeTexture::Create(name).SetSize(w,h).SetFormat(...).Build()`. Formats: RGBA8, BGRA8, RGBA16_Float, R11G11B10_Float, Depth32, RGB32/RGBA32/RG32_Float.
- **`BeMaterial` / `BeMaterialScheme`** — typed material properties (float, float2–4, matrix, textures, samplers). `BeMaterial::Create(schemeName)` instantiates from a scheme.
- **`BeMesh`** — `std::vector<BeFullVertex>` + indices + `std::vector<BeMeshSlice>` (multi-material regions).
- **`BeMeshPrimitives`** — namespace with `Plane()`, `Cube()`, `Sphere()` returning `shared_ptr<BeMesh>`.
- **`BeProp`** — renderable object: mesh + shader + per-slice materials + two-sided flags. Use `BeProp::FromMesh(mesh, shader)` for procedural geometry; use `BeAssimpImporter` or `SRM.LoadProp()` for file-loaded models.
- **`BeAssetRegistry`** — static asset manager. Call `InjectRenderer()` first, then `IndexShaderFiles(dir)`. Returns `weak_ptr`.
- **`BeWindow`** — GLFW window wrapper. Modes: Windowed, Fullscreen, BorderlessFullscreen.
- **`BeInput`** — keyboard (`GetKey/Down/Up`), mouse (position, buttons, scroll, capture), gamepad.
- **`BeCamera`** — position, yaw/pitch in degrees, FOV, near/far. `Update()` recalculates matrices and direction vectors.
- **`BeTimer`** — frame timing (delta time, elapsed time).

### RHI Abstraction Layer (`core/sen-rhi/`)

- **`SenBackend.h`** — selects active backend: `using SenBackend = SenVulkanBackend`.
- **`SenTypes.h`** — GPU type definitions: formats, usage flags, topology, sampler modes, blend/depth states, buffer access.
- **`SenShaderCompiler.h/cpp`** — Slang-based HLSL→SPIR-V compiler.
- **`vulkan/`** — Vulkan backend: device, swapchain, textures, buffers, samplers, pipelines, bind groups, descriptor sets. Uses VMA for memory.

### Toolkit: Standard Render Machine (`toolkit/standard-render-machine/`)

`BeStandardRenderMachine` (SRM) is a fluent builder that owns the texture registry, pass list, and per-frame geometry/light submission buffer. It is the standard deferred pipeline — **an example, not the engine**.

Typical setup:
```cpp
SRM = make_shared<BeStandardRenderMachine>(renderer, width, height);
auto gbuffer0 = SRM->DeclareGBufferTarget("gbuffer0", SenFormat::R11G11B10_Float);
// ... declare other targets and depth ...
SRM->AddShadowPass();
SRM->AddGeometryPass();
SRM->AddLightingPass("hdr");
SRM->AddBloomPass(5, "hdr", "hdr-bloomed");
SRM->AddBackbufferPass("hdr-bloomed");
SRM->Build();  // registers all passes with BeRenderer
```

Per-frame submission:
```cpp
SRM->ClearFrame();
SRM->AddGeometry({ .Name = "cube", .ModelMatrix = ..., .Prop = prop, .CastShadows = true });
SRM->AddSunLight({ .Direction = ..., ... });
SRM->AddPointLight({ ... });
// BeRenderer::Render() executes all passes
```

Passes (all in `toolkit/standard-render-machine/`):
- **`BeStandardShadowPass`** — shadow maps for directional lights (2D) and point lights (cubemap).
- **`BeStandardGeometryPass`** — populates G-buffer (4 color targets + depth).
- **`BeStandardLightingPass`** — deferred lighting with shadows and emissive.
- **`BeStandardBloomPass`** — Kawase bloom (bright extraction → downsample → upsample blend).
- **`BeStandardFullscreenEffectPass`** — generic fullscreen post-process (FXAA, SMAA, custom).
- **`BeStandardBackbufferPass`** — final composite + tonemapping to swapchain.

`BeImGuiPass` lives in `toolkit/imgui/`.

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

### Assimp Loading (`toolkit/assimp-import/`)

`BeAssimpImporter::LoadProp(modelPath, shader, materialExtractFunction)` loads a model file and builds a `BeProp`. The `materialExtractFunction` callback receives the raw `aiMaterial*` and must return a `shared_ptr<BeMaterial>`. `BeAssimpImporter::LoadTextureFromAssimpPath()` is a static helper for use inside that callback.

`BeStandardRenderMachine::LoadProp(modelPath, shader, lightingModel)` wraps this with a standard PBR or Phong material extraction.

### Scene System (`toolkit/scenes/`)

- **`BeScene`** — base with `OnLoad()` / `OnUnload()`.
- **`BeSceneManager`** — named registry. `RequestSceneChange()` / `ApplyPendingSceneChange()` for deferred transitions. Typed accessors: `GetActiveScene<T>()`, `GetScene<T>()`.

Project-level scenes typically subclass a local `BaseScene` that extends `BeScene` with `Prepare()` and `Tick(float deltaTime)` (see `example-sakura/scenes/BaseScene.h`).

### Shader Format (.hlsl)

HLSL with JSON metadata in block comments, compiled at runtime by Slang.

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

Tessellation: `"topology": "patch-list-3"` + `"hull"` / `"domain"` keys.

`@be-auto-boilerplate` regions are **generated by the devtools CLI** — never edit them manually.

#### Presets

| Category | Values |
|----------|--------|
| Rasterizer | `back-solid`, `back-wireframe`, `front-solid`, `none-solid` |
| Blend | `disable`, `alpha`, `additive`, `multiply` |
| Depth-Stencil | `less`, `lequal`, `equal`, `greater`, `always`, `disable` |

### Sampler String Format

`BeAssetRegistry::GetSampler()` parses `"filter-address[-cmp]"`. Filter: `point`/`linear`. Address: `wrap`/`clamp`/`mirror`. Optional `-cmp` for shadow comparison. Examples: `"linear-clamp"`, `"linear-clamp-cmp"`, `"point-wrap"`.

### Vertex Layout

`BeFullVertex`: Position (vec3), Normal (vec3), Color (vec4, default white), UV0–UV2 (vec2 each). Shader JSON semantic names: `"position"`, `"normal"`, `"color3"`, `"color4"`, `"uv0"`, `"uv1"`, `"uv2"`.

### Access Modifiers Convention

`core/umbrellas/common.hpp` — used as section markers **without** a colon:
- `expose` = `public:`
- `protect` = `protected:`
- `hide` = `private:`

### Include Umbrellas (`core/umbrellas/`)

- `include-glm.h` — GLM with `GLM_FORCE_LEFT_HANDED` + `GLM_FORCE_DEPTH_ZERO_TO_ONE`. Defines `_rad` literal (degrees→radians) and `HexColor("#RRGGBB")` → `glm::vec3`.
- `include-libassert.h` — `be_assert(condition, ...)` wrapping libassert's `DEBUG_ASSERT`.
- `json.h` — aliases `nlohmann::json` as `Json`.
- `common.hpp` — access-modifier markers (`expose`/`protect`/`hide`) plus `ENABLE_BITMASK(EnumType)` for bitwise ops on scoped enums.

### Coordinate System

Left-handed. All geometry (Assimp-loaded and procedural) **negates X** on positions and normals.

### Error Handling

`be_assert(condition, ...)` — debug assertions via libassert (preferred).

### ECS

entt (header-only, `toolkit/entt/entt.hpp`). Components defined per-project. Example in `example-sakura/Components.h`: `TransformComponent`, `RenderComponent`, `NameComponent`, `SunLightComponent`, `PointLightComponent`.

### Devtools: bechef

```bash
# Deploy assets and module shaders for an app (invoked by CMake)
devtools/bechef/bechef deploy --app example-sakura --out out/linux-debug/example-sakura
# Validate the workspace: manifests, shader metadata, bind resolution, name collisions
devtools/bechef/bechef check
# Regenerate @be-auto-boilerplate regions
devtools/bechef/bechef shadergen [--project <name>] [--file <path>] [--check] [--watch]
```

`shadergen` resolves each shader's material schemes against its project's bechef dependency
closure, so cross-module `bind`s work. Includes are emitted as bare filenames, matching the
flat `module-shaders/` namespace. A bind naming a scheme outside the closure is an error.
`be_deploy_app` runs `shadergen --check` before deploying, so a stale region fails the build.

### Vendor Libraries

All dependencies are committed as source under `vendor/`. **The build never touches the
network** — no `FetchContent` `URL`/`GIT_REPOSITORY` may be added. Exact upstream refs,
update instructions, and the rules are in **`vendor/VENDOR.md`**; keep it in sync when
changing a vendored tree.

| Library | Purpose |
|---------|---------|
| GLFW | Window + input |
| GLM | Math (vectors, matrices) |
| Assimp | 3D model loading (FBX, GLTF, OBJ) |
| Slang | Shader compilation → SPIR-V |
| nlohmann/json | JSON parsing |
| libassert + cpptrace | Assertions + stack traces |
| libdwarf + zstd | Symbolization for cpptrace (transitive) |
| stb_image | Image loading |
| Lua + LuaBridge3 | Scripting (`example-sakura` scene loading) |
| entt | ECS (lives in `toolkit/entt/`) |
| ImGui | Debug/editor UI (lives in `toolkit/imgui/`) |
| Vulkan SDK | Graphics API |
| VMA | Vulkan memory allocator |

Header-only deps expose a `vendor/<dep>/include/` root and are consumed as `INTERFACE`
targets (`glm`, `nlohmann_json`, `stb_image`, `luabridge3`). Never put `vendor/` itself
on an include path.
