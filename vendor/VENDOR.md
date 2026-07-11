# Vendored Dependencies

Every dependency is committed as source under `vendor/`. Configuring and building
requires **no network access** — this is a hard invariant, see [Rules](#rules).

Upstream `.git` directories are stripped, so this file is the only record of where
each tree came from. Keep it accurate or provenance is lost again.

## Manifest

| Library | Version | Pinned ref | Upstream | Integration |
|---|---|---|---|---|
| Assimp | 6.0.5 | tag `v6.0.5` | https://github.com/assimp/assimp | `add_subdirectory` |
| cpptrace | 1.0.4+ | commit `9b7a5da27492fb86bb1090a950e4df60ae6ef2af` | https://github.com/jeremy-rifkin/cpptrace | `FetchContent` + `SOURCE_DIR` |
| glfw | 3.4 | tag `3.4` | https://github.com/glfw/glfw | `add_subdirectory` |
| glm | 1.0.1 | tag `1.0.1` | https://github.com/g-truc/glm | `INTERFACE` target `glm` |
| libassert | 2.2.1 | tag `v2.2.1` | https://github.com/jeremy-rifkin/libassert | `FetchContent` + `SOURCE_DIR` |
| Lua | 5.4.7 | sha256 `9fbf5e28ef86c69858f6d3d34eccc32e911c1a28b4120ff3e84aaa70cfbf1e30` | https://www.lua.org/ftp/lua-5.4.7.tar.gz | `STATIC` target `lua` |
| LuaBridge3 | 3.0-rc4 | commit `87f36c9688ed7e153380f6cf78675be1be3fde05` | https://github.com/kunitoki/LuaBridge3 | `INTERFACE` target `luabridge3` |
| nlohmann/json | 3.12.0 | tag `v3.12.0` | https://github.com/nlohmann/json | `INTERFACE` target `nlohmann_json` |
| stb_image | 2.30 | sha256 `594c2fe35d49488b4382dbfaec8f98366defca819d916ac95becf3e75f4200b3` | https://github.com/nothings/stb | `INTERFACE` target `stb_image` |
| Slang | 2026.3.1 | prebuilt binaries | https://github.com/shader-slang/slang/releases | `IMPORTED` target `slang` |
| Vulkan SDK | 1.4.341 | prebuilt loader + headers | https://vulkan.lunarg.com/sdk/home | `IMPORTED` target `Vulkan::Vulkan` |
| VMA | 3.3.0 | bundled inside `vulkan-sdk/include/vma` | https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator | header-only |

### Transitive — pulled in by cpptrace

cpptrace fetches these itself. `vendor/CMakeLists.txt` redirects it to the vendored
trees with `FETCHCONTENT_SOURCE_DIR_LIBDWARF` / `FETCHCONTENT_SOURCE_DIR_ZSTD`, which
is what keeps configure offline. The refs below are cpptrace's own defaults, from
`cpptrace/cmake/OptionVariables.cmake`.

| Library | Version | Pinned ref | Upstream |
|---|---|---|---|
| libdwarf-lite | 2.1.0 | commit `5dfb2cd2aacf2bf473e5bfea79e41289f88b3a5f` | https://github.com/jeremy-rifkin/libdwarf-lite |
| zstd | 1.5.7 | sha256 `eb33e51f49a15e023950cd7825ca74a4a2b43db8354825ac24fc1b7ee09e6fa3` | https://github.com/facebook/zstd |

### Third-party code outside `vendor/`

| Library | Version | Location |
|---|---|---|
| entt | 3.16.0 | `toolkit/entt/entt.hpp` |
| ImGui | 1.92.7 WIP | `toolkit/imgui/` |

ImGui is a snapshot of an unreleased WIP branch, not a tagged release — it cannot be
pinned to a ref. Prefer a tagged release next time it is touched.

## Why cpptrace is pinned to a commit, not a tag

v1.0.4 is still cpptrace's newest release, but the vendored tree is 9 commits past it
and carries two upstream fixes that have **never appeared in a release**:

- an fd leak in the addr2line symbolizer (upstream #304)
- the C++20 modules guard raised from CMake 3.23 to 3.28 (upstream #279)

Updating cpptrace to "the latest release" would silently **revert** both. Move forward
to a newer commit or a future tag — never back to v1.0.4.

## Rules

1. **No network at configure time.** No `FetchContent` `URL` or `GIT_REPOSITORY` may be
   added. Anything a dependency fetches on its own must be vendored and redirected with
   `FETCHCONTENT_SOURCE_DIR_<NAME>`, as done for libdwarf and zstd.
2. **Every tree is pristine upstream.** No local edits. If a patch ever becomes
   unavoidable, commit it separately, record it here, and say why.
3. **Header-only deps expose an `include/` root** (`vendor/<dep>/include/`) and are
   published as `INTERFACE` targets. Never put `vendor/` itself on an include path —
   that makes every vendored tree reachable from every target.
4. **Update this file in the same commit** that changes a vendored tree.

## Updating a dependency

```bash
# source deps: re-clone at the new ref, drop the .git dir, replace the tree
git clone https://github.com/g-truc/glm /tmp/glm && git -C /tmp/glm checkout <ref>
rm -rf /tmp/glm/.git
rm -rf vendor/glm/include/glm && mkdir -p vendor/glm/include
mv /tmp/glm/glm vendor/glm/include/glm
```

Then update the row above and rebuild.

## Verifying a tree is pristine

Any tree can be checked against its upstream ref, which is the point of pinning:

```bash
git clone https://github.com/glfw/glfw /tmp/glfw && git -C /tmp/glfw checkout 3.4
diff -rq /tmp/glfw vendor/glfw    # only .git-related entries should differ
```

## Verifying the build is still offline

```bash
unshare -rn cmake -S . -B /tmp/hermetic -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

Must configure cleanly with no network namespace. If a new dependency starts reaching
out, this fails — that is the intended tripwire.
