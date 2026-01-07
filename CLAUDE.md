# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Building and Development

**Clean and Generate Visual Studio 2022 project files:**
```bash
./premake5 vs2022
```

**Clean generated project files:**
```bash
./premake5 clean
```

After generating, open `be.sln` in Visual Studio 2022. The solution contains:
- `core` - Static library with the graphics engine (C++23)
- `toolkit` - Static library with utilities (scene management, ImGui integration)
- `example-game-1` - Example executable demonstrating engine usage
- `misc-configuration` - Non-buildable project containing configuration files

## Project Architecture

### Core Engine (`core/src`)

The engine is a **deferred rendering DirectX 11 graphics engine** built with modern C++ (C++23).

**Key Components:**

1. **Rendering System**
   - `BeRenderer` - Main renderer class managing D3D11 device, swapchain, and render passes
   - `BePipeline` - GPU pipeline state management, shader binding, and resource caching
   - `BeRenderPass` - Base class for deferred rendering passes (geometry, lighting, shadow, bloom, etc.)
   - Deferred lighting pipeline: GeometryPass → ShadowPass → LightingPass → BloomPass → Backbuffer

2. **Shader System**
   - `BeShader` - Wraps D3D11 shader objects (vertex, hull, domain, pixel)
   - Custom `.beshade` shader format with metadata for materials and texture slots
   - `BeShaderIncludeHandler` - Manages shader include resolution and compilation
   - `BeShaderTools` - Utilities for shader compilation and error reporting
   - Shaders use HLSL with include files in `core/src/shaders/`

3. **Material System**
   - `BeMaterial` - Defines material properties and texture bindings
   - `BeMaterialScheme` - Schema for material layout and properties
   - Materials can be bound to material slots in shaders for dynamic property updates

4. **Model and Asset Management**
   - `BeModel` - Loaded 3D models using Assimp (supports FBX, OBJ, etc.)
   - `BeAssetRegistry` - Caches loaded models and textures to prevent duplicate loading
   - Models are baked into draw slices for efficient rendering

5. **Texture System**
   - `BeTexture` - Wraps D3D11 textures, supports various formats
   - Textures loaded via Assimp or directly from disk (DDS, PNG, etc.)
   - Supports standard texture slots for diffuse, normal, roughness, metallic, etc.

6. **Camera and Input**
   - `BeCamera` - View/projection matrices, frustum culling
   - `BeInput` - Keyboard and mouse input handling

7. **Buffers**
   - `BeBuffers.h` - CPU-to-GPU constant buffer definitions
   - Shared vertex and index buffers for efficient batching
   - Uniform buffer for camera, lighting, and global render state

### Toolkit (`toolkit`)

**Scene System:**
- `BeSceneManager` - Manages multiple scenes with deferred scene switching
- `BeScene` - Base class for scenes; override `OnLoad()` and `OnUnload()`

**ImGui Integration:**
- `BeImGuiPass` - Render pass for immediate-mode UI overlays
- Accepts UI callback function for drawing UI each frame

**EnTT Integration:**
- Entity Component System (ECS) available in `toolkit/entt/`

## Key Design Patterns

1. **Access Modifiers Umbrella** (`umbrellas/access-modifiers.hpp`)
   - Uses custom macros: `expose`, `hide`, `protect` for semantic clarity
   - `expose` = public API, `hide` = private implementation, `protect` = protected for inheritance

2. **Deferred Rendering Pipeline**
   - Render passes execute in sequence: each writes to intermediate targets
   - `BeRenderer::Render()` calls each pass in order
   - Draw entries submitted per-frame; passes consume and process them

3. **Shader Binding Cache**
   - `BePipeline` maintains resource and constant buffer caches to avoid redundant D3D11 calls
   - Separate caches for vertex, tessellation, and pixel shader stages

4. **Asset Caching**
   - `BeAssetRegistry` prevents reloading duplicate models/textures
   - Critical for performance in larger projects

## Common Development Tasks

**Adding a new render pass:**
1. Create header in `core/src/passes/BeXxxPass.h` inheriting from `BeRenderPass`
2. Implement `Initialise()` and `Render()` methods
3. Use `_renderer` to access device, pipeline, and draw entries
4. Register in game setup: `renderer->AddRenderPass(new MyPass())`

**Adding a new shader:**
1. Create `.beshade` or `.hlsl` file in `core/src/shaders/`
2. Use `BeShader::Create(path, renderer)` to load and compile
3. Bind to pipeline: `pipeline->BindShader(shader, BeShaderType::All)`
4. Define vertex semantics using `BeVertexElementDescriptor`

**Creating a new scene:**
1. Inherit from `BeScene`
2. Implement `OnLoad()` to set up models, lights, cameras
3. Register with scene manager: `sceneManager->RegisterScene("name", std::make_unique<MyScene>())`

## Dependencies

- **DirectX 11** - Graphics API
- **GLFW3** - Window management
- **Assimp** - Model loading
- **GLM** - Math library
- **libassert** - Assertion/debugging (static linking)
- **nlohmann/json** - JSON parsing for shader metadata
- **ImGui** - UI toolkit (in toolkit/)
- **EnTT** - Entity Component System (in toolkit/)

## Important Notes

- **C++23 required** - Uses modern C++ features (concepts, expected, etc.)
- **Windows-only** - Direct3D 11 is Windows-specific
- **64-bit target** - Architecture set to x86_64
- **Static linking** - libassert is statically linked; copy appropriate config binaries during build
- **Shader compilation errors** are reported with file/line info via libassert
- **Debug vs Release** - Debug has full symbols and no optimization; Release is fully optimized