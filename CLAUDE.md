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

**Windows** — CMake with Visual Studio 2022:
```bash
cmake --preset windows
cmake --build out/windows --config Debug
```

No tests or linting. C++23 (`/Zc:__cplusplus /Zc:preprocessor` on MSVC). Build outputs land in `out/<preset>/`.

`example-sakura` is the up-to-date showcase; prefer it as the reference.

## Project Structure

```
be-engine/
├── core/               # Static lib — rendering engine core
│   ├── Be*.h/cpp       # Engine classes
│   ├── sen-rhi/        # RHI abstraction (Vulkan-only backend)
│   └── umbrellas/      # Config headers (glm, libassert, json, common)
├── toolkit/            # Static lib — higher-level abstractions; links against core
│   ├── standard-render-machine/  # Deferred rendering pipeline (SRM)
│   ├── standard-game/  # BeStandardGame app loop + BeStandardBaseScene/FullScene framework
│   ├── coroutine/      # Unity-style BeCoroutine + BeCoroutineScheduler
│   ├── assimp-import/  # BeAssimpImporter — Assimp-based model loading
│   ├── scenes/         # BeScene base + BeSceneManager
│   ├── lua/            # BeLuaState + BeLuaValue — Lua data reading
│   ├── imgui/          # ImGui source + BeImGuiPass + backends
│   └── entt/           # ECS header-only library
├── example-sakura/     # Advanced showcase (multi-scene, ECS, SRM)
├── example-vulkan/     # Minimal raw Vulkan/RHI example
├── devtools/
│   └── bechef/         # CLI — content cook, workspace check, shader boilerplate autogen
├── bechef              # Built CLI binary, copied to root by CMake (invoke as ./bechef)
├── workspace.bechef    # Workspace roots + ignore globs
└── vendor/             # Third-party libraries
```

The engine is consumable as a git submodule: the root `CMakeLists.txt` builds the examples
only when it is the top-level project; a parent project adds `core`, `toolkit`, and `bechef`
and calls `be_cook_app(<exe>)` on its own app target.

## Architecture Overview

**be-engine** is a C++23 graphics engineer's workbench targeting Vulkan. The rendering architecture is pass-based: the user assembles `BeRenderPass` subclasses and the renderer executes them in order each frame.

### Core Engine (`core/`)

- **`BeRenderer`** — manages Vulkan swapchain/device/queues and an ordered list of `BeRenderPass*`. `Render()` opens a `SenCommandBuffer` and runs each pass wrapped in debug annotations. Binds the `uniform-material` frame scheme (camera matrices, time, ambient) before every pass.
- **`BeRenderPass`** — abstract base: `Initialise(BeRenderer&)` and `Render(BeRenderer&, SenCommandBuffer&)`, plus `GetPassName()`. Passes are held in a `BePassSequence` (`vector<unique_ptr<BeRenderPass>>`).
- **`BePass`** — fluent per-render command-scope helper built on a `SenCommandBuffer`: `.SetCompute(...)`, `.UseTexture(...)`/`.UseMaterial(...)` (read/storage bindings), `.AddColorTarget(...)`/`.SetDepthTarget(...)`, `.SetViewport(...)`. A pass's `Render()` builds one or more of these to declare its I/O and issue draws/dispatches. This is the recording layer; `BeRenderPass` is the schedulable unit.
- **`BePipelineBuilder`** — fluent builder for GPU pipeline state. `BePipelineBuilder::Start(shader).SetTopology(...).SetBlend(...).Build()`. Caches by (shader, topology, rasterizer, blend, depthStencil, formats).
- **`BeShader`** — loaded from `.hlsl` via `BeShader::Create(path)`. Parses the embedded `@be-shader` DSL block. Compiled at runtime by Slang to SPIR-V. Supports vertex, pixel, hull, domain stages.
- **`BeShaderLibrary`** — `BeRenderer` loads every cooked shader in the runtime `shaders/` dir at init via `BeShaderLibrary::LoadShaders()`. Apps never load shader dirs themselves.
- **`BeShaderTools`** — parses the `@be-material` and `@be-shader` DSL blocks from `.hlsl` comments.
- **`BeMaterialArena`** — per-scheme ring arenas backing material uploads; a material is committed to a GPU slot on bind (replaced the old per-object material pool).
- **`BeFileWatcher`** — generic hot-reload: register a path-provider + handler `WatchId`, poll each frame. Used by `BeStandardFullScene` for live scene/shader reload.
- **`BeTexture`** — builder: `BeTexture::Create(name).SetSize(w,h).SetFormat(...).Build()`. Formats: RGBA8, BGRA8, RGBA16_Float, R11G11B10_Float, Depth32, RGB32/RGBA32/RG32_Float.
- **`BeMaterial` / `BeMaterialScheme`** — typed material properties (float, float2–4, matrix, textures, samplers). `BeMaterial::Create(schemeName)` instantiates from a scheme.
- **`BeMesh`** — `std::vector<BeFullVertex>` + indices + `std::vector<BeMeshSlice>` (multi-material regions).
- **`BeMeshPrimitives`** — namespace with `Plane()`, `Cube()`, `Sphere()` returning `shared_ptr<BeMesh>`.
- **`BeProp`** — renderable object: mesh + shader + per-slice materials + two-sided flags. Use `BeProp::FromMesh(mesh, shader)` for procedural geometry; use `BeAssimpImporter` or `SRM.LoadProp()` for file-loaded models.
- **`BeAssetRegistry`** — instance-based name→asset store for materials, textures, and props (`Add/Get/Has/Remove`, `Get*` returns `weak_ptr`). Owned per-scene (e.g. `BeStandardFullScene` holds one); not a global. Shaders are managed separately by `BeShaderLibrary`.
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
SRM->AddTonemapperPass("hdr-bloomed", "sdr");
SRM->AddBackbufferPass("sdr");
SRM->Build();  // registers all passes with BeRenderer
```

Target declaration: `DeclareGBufferTarget`, `DeclareDepthTarget`, `DeclareTextureTarget`.
Pass builders: `AddShadowPass`, `AddGeometryPass`, `AddSkyboxPass`, `AddEnvironmentBakePass`
(IBL), `AddLightingPass`, `AddBloomPass`, `AddFullscreenPass` (generic post), `AddTonemapperPass`,
`AddBackbufferPass`, and `AddPass` for a raw `BeRenderPass`.

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

### Toolkit: Standard Game (`toolkit/standard-game/`)

The application framework that used to live in `example-sakura`, now reusable.

- **`BeStandardGame`** — owns `BeWindow`, `BeRenderer`, `BeInput`, `BeSceneManager`. Constructed from a `BeStandardGameConfig` (title, window mode, size, present mode). `Run()` drives the main loop (poll input, tick active scene, render). `example-sakura/main.cpp` is the canonical entrypoint: construct the game, register scenes, `Prepare()` each, request the first scene, `return game.Run()`.
- **`BeStandardBaseScene`** — `BeScene` subclass holding a `BeStandardGame*`; adds virtual `Prepare()`, `Tick(deltaTime)`, `Render()`. Project scenes subclass this.
- **`BeStandardFullScene`** — batteries-included scene: owns its own `BeAssetRegistry`, `entt::registry`, `BeCamera`, a `BeStandardRenderMachine`, a `BeLuaState`, and a `BeCoroutineScheduler`. Content is Lua-driven through overridable `DefineSettings/DefineAssets/DefineScene/DefinePasses` plus `ApplyLua*` appliers. `SetWatchFile()` + `Reload(ReloadMask)` (a bitmask over Settings/Assets/Scene/Passes) give hot-reload via `BeFileWatcher`.

### Toolkit: Coroutines (`toolkit/coroutine/`)

Unity-style C++20 coroutines. A `BeCoroutine` function `co_yield`s a float (seconds to wait);
`BeCoroutineScheduler::Tick(deltaTime)` resumes handles whose wait has elapsed. Scenes own a
scheduler (e.g. `BeStandardFullScene`) and start coroutines for timed/sequenced logic.

#### G-Buffer Layout

| Target | Format | Contents |
|--------|--------|----------|
| 0 | `R11G11B10_FLOAT` | Diffuse RGB |
| 1 | `R16G16B16A16_FLOAT` | World Normal XYZ |
| 2 | `R8G8B8A8_UNORM` | Specular RGB + Shininess in A (`value / 2048.0`) |
| 3 | `R11G11B10_FLOAT` | Emissive RGB |
| Depth | `R32_TYPELESS` | Depth |

### Binding Frequencies

A shader's `bind` lines assign a scheme to a register (`s0`, `s1`, ...) with a *frequency*
that says who fills it and how often:

- `frame` — renderer's `uniform-material` (camera, time, ambient), bound by `BeRenderer` every pass.
- `geometry-object` — per-object data (model matrix), bound per draw by the geometry pass.
- `geometry-main` — per-draw surface material in the geometry pass.
- `main` — this shader's own per-draw material (fullscreen/custom passes).

Register numbers must be consistent across a shader's binds; the concrete cbuffer/descriptor
slots are emitted into the auto-boilerplate by `shadergen`.

### Assimp Loading (`toolkit/assimp-import/`)

`BeAssimpImporter::LoadProp(modelPath, shader, materialExtractFunction)` loads a model file and builds a `BeProp`. The `materialExtractFunction` callback receives the raw `aiMaterial*` and must return a `shared_ptr<BeMaterial>`. `BeAssimpImporter::LoadTextureFromAssimpPath()` is a static helper for use inside that callback.

`BeStandardRenderMachine::LoadProp(modelPath, shader, lightingModel)` wraps this with a standard PBR or Phong material extraction.

### Scene System (`toolkit/scenes/`)

- **`BeScene`** — base with `OnLoad()` / `OnUnload()`.
- **`BeSceneManager`** — named registry. `RequestSceneChange()` / `ApplyPendingSceneChange()` for deferred transitions. Typed accessors: `GetActiveScene<T>()`, `GetScene<T>()`.

Project scenes subclass `BeStandardBaseScene` (or `BeStandardFullScene`) from `toolkit/standard-game/`, which extend `BeScene` with `Prepare()` / `Tick(float)` / `Render()` and a `BeStandardGame*` handle. See `example-sakura/scenes/`.

### Lua Data Reading (`toolkit/lua/`)

`BeLuaState` owns a `lua_State`. `DoFile(path)` runs a chunk (call it repeatedly to layer a shared prelude under a data file), `Global(name)` and `Call(name, args...)` return a `BeLuaValue`.

`BeLuaValue` is a nil-safe view. Indexing a missing or non-table value yields a nil value that keeps chaining. `Get<T>()` returns `std::optional<T>`, `GetOr(fallback)` deduces `T` from the fallback. A missing key is silent, a type mismatch logs the dotted path. `Pairs()` and `Array()` return eager vectors.

```cpp
BeLuaState lua;
lua.DoFile(path);
for (const auto& [name, entityTable] : lua.Call("makeScene").Pairs()) {
    comp.Power = entityTable["sunLight"]["power"].GetOr(comp.Power);
}
```

Conversions cover arithmetic types, `bool`, `std::string`, `glm::vec2/3/4` (Lua arrays) and `glm::quat` (euler degrees). `glm::vec3`/`vec4` also accept a hex string, `#` required: `color = "#FFAA33"`. Six digits or eight, where the extra pair is alpha for `vec4` and ignored for `vec3`; a six-digit `vec4` gets alpha 1. Add types by specialising `BeLuaConverter`. Values are views: never outlive the `BeLuaState`. See `SakuraScene::LoadSceneFile()` for scene loading built on it.

### Shader Format (.hlsl)

HLSL with metadata in a leading block comment, written in a be-specific DSL (not JSON),
compiled at runtime by Slang.

```hlsl
/*
@be-material: scheme-name {
    PropertyName: float3 = (1.0, 1.0, 1.0)
    TextureName: texture2d = default-texture-name
    SamplerName: sampler = linear-clamp
}

@be-shader shader-name {
    topology triangle-list
    rasterizer back-solid
    blend disable
    depth less

    vertex VSMain(position, normal, uv0)
    pixel PSMain

    bind s0 frame uniform-material
    bind s2 main scheme-name

    target s0 OutputName float3
}
*/
```

- **Material property types**: `float`, `float2`, `float3`, `float4`, `matrix`, `texture2d`, `texturecube`, `sampler`. Defaults in parens (values) or a named asset (textures/samplers). Omitting `= ...` leaves it undefaulted.
- **`bind <slot> <frequency> <scheme> [StructAlias]`** — binds a material scheme at a register. The *frequency* is the update cadence / role, resolved by the render machine: `frame` (renderer's `uniform-material`), `main` (this shader's own per-draw material), and pass-specific roles like `geometry-object` (per-object model matrix) and `geometry-main` (per-draw surface material in the geometry pass). The optional last token names the generated HLSL struct.
- **`target <slot> <Name> <type>`** — a bound render target output.
- **Tessellation**: `topology patch-list-3` plus `hull <Fn>` / `domain <Fn>` lines.
- **Includes** are emitted as bare filenames (flat `shaders/` namespace).

`@be-auto-boilerplate` / `region ... endregion` blocks (cbuffer structs, samplers, I/O) are **generated by `bechef shadergen`** — never edit them manually.

#### Presets

| Category | Values |
|----------|--------|
| Rasterizer | `back-solid`, `back-wireframe`, `front-solid`, `none-solid` |
| Blend | `disable`, `alpha`, `additive`, `multiply` |
| Depth-Stencil | `less`, `lequal`, `equal`, `greater`, `always`, `disable` |

### Sampler String Format

`BeShaderLibrary::GetSampler()` parses `"filter-address[-cmp]"`. Filter: `point`/`linear`. Address: `wrap`/`clamp`/`mirror`. Optional `-cmp` for shadow comparison. Examples: `"linear-clamp"`, `"linear-clamp-cmp"`, `"point-wrap"`.

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

entt (header-only, `toolkit/entt/entt.hpp`). Components can be per-project, but the standard set lives in `toolkit/standard-game/Components.h`: `TransformComponent`, `CirclingComponent`, `RenderComponent`, `NameComponent`, `SunLightComponent`, `PointLightComponent`.

### Devtools: bechef

The CLI is built by CMake and copied to the repo root, so invoke it as `./bechef` (a `--root`
flag overrides the workspace root, which defaults to the cwd's nearest `workspace.bechef`).

```bash
# Cook assets and shaders for an app (invoked by CMake via be_cook_app)
./bechef cook --app example-sakura --out out/linux-debug/example-sakura [--mode copy|symlink]
# Validate the workspace: manifests, shader metadata, bind resolution, name collisions
./bechef check
# Regenerate the shader auto-boilerplate regions
./bechef shadergen [--project <name>] [--file <path>] [--check] [--watch]
```

**Workspace model.** `workspace.bechef` at the root declares `roots <dirs...>` (where projects
are discovered) and `ignore <globs...>`. Each project dir has a `project.bechef` with:
- `kind module | app` — libraries are modules; only apps are cookable.
- `depends <module...>` — the bechef dependency closure (e.g. an app `depends core toolkit`).
- `shaders <dir>` (repeatable) and `assets <dir>` — content dirs relative to the project.

`shadergen` resolves each shader's material schemes against the project's dependency closure,
so cross-module `bind`s work; a bind naming a scheme outside the closure is an error. `cook`
flattens the shader dirs of the app and its whole closure into one runtime `shaders/` (filenames
must be unique across the closure), and copies asset dirs into `assets/` preserving their tree.
Includes are emitted as bare filenames matching the flat namespace. `--mode symlink` deploys
symlinks so edits are live (hot-reload); `copy` is the default. `be_cook_app` runs
`shadergen --check` before cooking, so a stale region fails the build.

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
