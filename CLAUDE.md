# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This project uses Premake5 to generate Visual Studio solutions:
- Generate VS2022 solution: `./premake5 vs2022`
- Generate VS2019 solution: `./premake5 vs2019`
- Clean generated files: `./premake5 clean`

After generating the solution, build using Visual Studio or MSBuild. The premake5.exe executable is included in the repository root.

## Project Structure

The workspace contains three main projects:

### core (Static Library)
The main engine library (`core/src/`) containing:
- Rendering system (Direct3D 11)
- Asset management (models, textures, materials, shaders)
- Core engine components (window, input, camera)
- Custom `.beshade` shader format

### toolkit (Static Library)
Higher-level utilities built on top of core (`toolkit/`):
- Scene management system (`scenes/BeSceneManager.h`)
- EnTT integration for entity-component system
- ImGui integration

### example-game-1 (Executable)
Example game project demonstrating engine usage:
- Links against both `core` and `toolkit`
- Custom game scenes in `example-game-1/scenes/`
- Assets in `example-game-1/assets/`
- Entry point: `example-game-1/main.cpp`, game logic in `Game.cpp`

## Architecture Overview

### Rendering Pipeline
The engine uses a deferred rendering architecture with a multi-pass system:

1. **BeRenderer**: Central renderer managing the D3D11 device, context, and render passes
   - Owns shared vertex/index buffers for all models (batched geometry)
   - Maintains `_drawEntries` submitted each frame via `SubmitDrawEntry()`
   - Executes render passes in sequence via `AddRenderPass()` and `Render()`

2. **BeRenderPass**: Base class for all render passes (core/src/passes/BeRenderPass.h)
   - GeometryPass: Renders scene to G-buffer (diffuse, normals, specular)
   - ShadowPass: Generates shadow maps
   - LightingPass: Deferred lighting calculations
   - BloomPass: HDR bloom effect (bright extraction + Kawase blur)
   - BackbufferPass: Final output with tonemapping

3. **BePipeline**: Manages render targets and intermediate textures for the deferred pipeline

### Material System
The engine features a custom material system with two key components:

1. **BeMaterialScheme**: Defines the structure of a material type
   - Created from `.beshade` files via `@be-material:` blocks
   - Specifies properties (float, float2-4, matrix), textures, and samplers
   - Materials reference schemes by name

2. **BeMaterial**: Instances of material schemes with actual values
   - Automatically manages constant buffers for properties
   - Properties marked dirty and uploaded to GPU via `UpdateGPUBuffers()`
   - Can be "frequently used" for optimization

### Shader System (.beshade files)
Custom shader format combining HLSL with metadata:

- `@be-material:` blocks define material schemes (properties, textures, samplers with defaults)
- `@be-shader:` blocks define shader metadata (topology, entry points, vertex layout, render targets)
- Shaders can declare multiple material slots with different schemes (e.g., `geometry-object` at slot 1, `geometry-main` at slot 2)
- `#include` directives supported for sharing material definitions between shaders (e.g., `#include "objectMaterial.beshade"`)
- Standard HLSL code follows the metadata blocks

**Material Lookup System**:
- **Link names** (e.g., "geometry-main") map to **scheme names** (e.g., "standard-material-for-geometry-pass")
- Use `BeShader::GetMaterialSchemeName(linkName)` to get scheme name from link name
- Use `BeShader::GetMaterialSlotByScheme(schemeName)` for slot lookup (used by `BindMaterialAutomatic`)
- Access material schemes via `BeAssetRegistry::GetMaterialScheme(schemeName)` - never use `BeMaterialScheme::Create`

### Asset Management
**BeAssetRegistry** is a static service (all methods and members are static, no instantiation):
- Call `IndexShaderFiles(filePaths, renderer)` to scan `.beshade` files - auto-registers shaders and material schemes
- Assets accessed via static methods: `GetShader()`, `GetMaterialScheme()`, `GetTexture()`, etc.
- All `Get*()` methods assert if asset not found - use `Has*()` to check existence first
- Static member variables declared in .h must be defined in BeAssetRegistry.cpp (C++ requirement)

### Model Loading
**BeModel** represents 3D geometry:
- Loads via Assimp (supports common formats: FBX, OBJ, etc.)
- Contains `DrawSlices` (sub-meshes with materials), vertices, indices
- Models registered with renderer via `RegisterModels()` then baked into shared buffers via `BakeModels()`
- Each DrawSlice references a BeMaterial

### Scene System (toolkit)
**BeSceneManager** manages game scenes:
- Scenes registered via `RegisterScene(name, unique_ptr<BeScene>)`
- Scene changes requested via `RequestSceneChange()`, applied via `ApplyPendingSceneChange()`
- Active scene accessible via `GetActiveScene()` with optional type-safe template version

## Key Dependencies
- **Direct3D 11**: Graphics API (Windows only)
- **GLFW**: Window and input handling
- **GLM**: Math library
- **Assimp**: Model loading
- **nlohmann/json**: JSON parsing (for material schemes)
- **stb_image**: Image loading
- **EnTT**: Entity-component system (in toolkit)
- **ImGui**: Debug UI (in toolkit)

## Access Modifiers
The codebase uses custom access modifier macros from `umbrellas/access-modifiers.hpp`:
- `expose`: Public members/methods
- `hide`: Private members/methods
- `protect`: Protected members/methods

These replace standard C++ `public:`/`private:`/`protected:` keywords for a different code style.

## Important Build Details
- Language: C++20
- Platform: Windows x64
- Configurations: Debug and Release
- Post-build steps copy shaders and assets to output directory
- Assimp DLL copied to executable directory

## Common Patterns

### Creating and Using Materials
```cpp
// Material scheme automatically registered when shaders indexed
auto scheme = BeAssetRegistry::GetMaterialScheme("standard-material-for-geometry-pass");
auto material = BeMaterial::Create("myMaterial", scheme, false, renderer);
material->SetFloat3("DiffuseColor", glm::vec3(1.0, 0.5, 0.2));
material->SetTexture("DiffuseTexture", myTexture);
BeAssetRegistry::AddMaterial("myMaterial", material);
```

### Submitting Draw Calls
```cpp
BeRenderer::DrawEntry entry;
entry.Position = glm::vec3(0, 0, 0);
entry.Scale = glm::vec3(1, 1, 1);
entry.Model = myModel;
entry.CastShadows = true;
renderer->SubmitDrawEntry(entry);
```

### Setting Up Render Pipeline
```cpp
renderer->AddRenderPass(new BeGeometryPass());
renderer->AddRenderPass(new BeShadowPass());
renderer->AddRenderPass(new BeLightingPass());
renderer->AddRenderPass(new BeBloomPass());
renderer->AddRenderPass(new BeBackbufferPass());
renderer->InitialisePasses();
```
