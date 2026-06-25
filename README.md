
# this is *be* engine.

yo, this is my c++23 graphics engine i built for myself and for you. its a custom graphics programming workbench: a pass-based renderer, runtime shader compilation, and a batteries-included deferred pipeline you can take apart and rebuild.

<!-- hero: drop a wide showcase gif/mp4 at the path below (~1200px wide looks best) -->
![be engine showcase](.github/media/hero.gif)

### what's in it

* **pass-based renderer**. you assemble `BeRenderPass` subclasses and the renderer runs them in order each frame. the whole pipeline is yours to rearrange. eventually this'll be replaced with a more proper render graph system.
* **standard render machine (srm)**. a ready, plug-and-play deferred pipeline: g-buffer geometry, directional + point shadow maps, deferred lighting, kawase bloom, fxaa/smaa, tonemapping. it's not the only way though, feel free to write your own rm.
* **runtime shaders**. `.hlsl` with metadata in block comments, compiled to spir-v by slang at load time. hot-reload supported (`linux-debug-hotreload` preset).
* **custom rhi** (`sen-rhi`). today it has only a vulkan backend (vma for memory), but it's built to be expanded.
* **assets + models**. assimp loading (fbx/gltf/obj), typed materials & material schemes, procedural mesh primitives (plane/cube/sphere).
* **the rest**. entt ecs, imgui debug ui, glfw window/input, scene manager.

### building

requires: cmake 3.25+, ninja (linux), vs2022 (windows), vulkan sdk.

**linux**
```bash
cmake --preset linux-debug          # or linux-release / linux-debug-hotreload
cmake --build out/linux-debug
```

**windows**
```bash
cmake --preset windows
cmake --build out/windows --config Debug
```

### what it looks like to use

```cpp
SRM = make_shared<BeStandardRenderMachine>(renderer, width, height);
SRM->DeclareGBufferTarget("gbuffer0", SenFormat::R11G11B10_Float);
// ... declare the other targets + depth ...
SRM->AddShadowPass();
SRM->AddGeometryPass();
SRM->AddLightingPass("hdr");
SRM->AddBloomPass(5, "hdr", "hdr-bloomed");
SRM->AddBackbufferPass("hdr-bloomed");
SRM->Build();
```

then per frame: `ClearFrame()`, submit geometry + lights, and let the renderer run the passes. see `example-sakura` for the full thing.

### structure

* `core/`. rendering engine core (static lib): renderer, passes, pipeline builder, shaders, textures, materials, meshes, camera, input, window. `sen-rhi/` lives here.
* `toolkit/`. higher-level abstractions (static lib, links core): the srm deferred pipeline, scenes, assimp import, imgui, entt.
* `example-sakura/`. the up-to-date showcase: multi-scene, ecs, full deferred pipeline.
* `example-vulkan/`. minimal raw vulkan/rhi example.
* `example-game-1/`. old simple example (outdated, may not build).
* `devtools/shader-boilerplate-autogen`. cli that generates shader boilerplate; has a `--watch` mode.

### shaders

each `.hlsl` file declares its own material scheme and pipeline in a block comment, using a small be-specific dsl. it gets parsed at load and compiled by slang. the `@be-auto-boilerplate` region (cbuffers, samplers, i/o structs) is generated for you by the devtools cli, so you only write the actual shader code.

```hlsl
/*
@be-material: my-material {
    DiffuseColor: float3 = (0.5, 0.5, 0.5)
    DiffuseTexture: texture2d = white
    InputSampler: sampler = point-clamp
}

@be-shader my-shader {
    topology triangle-list
    rasterizer back-solid
    blend disable
    depth less

    vertex VertexFunction(position, normal, uv0)
    pixel PixelFunction

    bind s0 frame uniform-material
    bind s1 main my-material

    target s0 Color float3
}
*/
```

coordinate system is left-handed y-up (depth 0 to 1).

### gallery

<!--
  drop the files into .github/media/ at the paths below and they'll show up.
  gifs/pngs embed inline. for mp4 you can also drag-drop into a github issue and
  paste the resulting url instead of a local path.
-->

| | |
|:-:|:-:|
| ![deferred lighting + shadows](.github/media/lighting.png) | ![bloom + tonemapping](.github/media/bloom.png) |
| *deferred lighting + shadows* | *bloom + tonemapping* |
| ![assimp models](.github/media/models.png) | ![post fx](.github/media/postfx.png) |
| *assimp-loaded models* | *post fx (fxaa/smaa, dof, pixelation)* |

### enjoy!
