
# this is *be* engine.

yo, this is my c++23 graphics engine i built for myself on vulkan. batteries included: deferred pipeline, shadow maps, bloom, ecs, imgui, assimp loading.

### building

**linux**
```bash
cmake --preset linux-debug
cmake --build out/linux-debug
```

**windows**
```bash
cmake --preset windows
cmake --build out/windows --config Debug
```

requires: cmake 3.25+, ninja (linux), vs2022 (windows), vulkan sdk.

### structure
* `core/` - rendering engine core, static lib
* `toolkit/` - higher-level abstractions (srm, ecs, imgui, assimp), static lib
* `example-game-1/` - simple example (outdated, may not work)
* `example-sakura/` - a bit more advanced showcase: multi-scene, ecs, full deferred pipeline
* `example-vulkan/` - minimal raw vulkan/rhi example
* `devtools/` - shader boilerplate autogen cli

### enjoy!
