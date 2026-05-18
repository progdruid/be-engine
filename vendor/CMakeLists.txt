include(FetchContent)

# ── cpptrace ──────────────────────────────────────────────────────────────────
FetchContent_Declare(cpptrace
    SOURCE_DIR        "${CMAKE_CURRENT_SOURCE_DIR}/cpptrace"
    OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(cpptrace)

# ── libassert ─────────────────────────────────────────────────────────────────
set(LIBASSERT_USE_EXTERNAL_CPPTRACE ON CACHE BOOL "" FORCE)
FetchContent_Declare(libassert
    SOURCE_DIR        "${CMAKE_CURRENT_SOURCE_DIR}/libassert"
    OVERRIDE_FIND_PACKAGE)
FetchContent_MakeAvailable(libassert)

# ── glfw ──────────────────────────────────────────────────────────────────────
# source tree: clone https://github.com/glfw/glfw into vendor/glfw/
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw EXCLUDE_FROM_ALL)

# ── assimp ────────────────────────────────────────────────────────────────────
# source tree: clone https://github.com/assimp/assimp into vendor/Assimp/
set(ASSIMP_BUILD_TESTS                    OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS             OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES                  OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL                        OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS             OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX           OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER             ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_GLTF_IMPORTER            ON  CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_FBX_IMPORTER             ON  CACHE BOOL "" FORCE)
add_subdirectory(Assimp EXCLUDE_FROM_ALL)

# ── slang (prebuilt) ──────────────────────────────────────────────────────────
# Windows: vendor/slang/lib/ + vendor/slang/bin/  (existing layout)
# Linux:   place libslang-compiler.so in vendor/slang/linux-x64/lib/
#          download from https://github.com/shader-slang/slang/releases
add_library(slang UNKNOWN IMPORTED GLOBAL)
target_include_directories(slang INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/slang/include")
if(WIN32)
    set_target_properties(slang PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/slang/lib/slang-compiler.lib")
else()
    set_target_properties(slang PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/slang/linux-x64/lib/libslang-compiler.so")
endif()

# ── Vulkan (prebuilt) ─────────────────────────────────────────────────────────
# Windows: vendor/vulkan-sdk/lib/vulkan-1.lib  (existing layout, DLL lives in System32)
# Linux:   place libvulkan.so.1 in vendor/vulkan-sdk/lib/linux-x64/
#          download from https://vulkan.lunarg.com/sdk/home
add_library(Vulkan::Vulkan UNKNOWN IMPORTED GLOBAL)
target_include_directories(Vulkan::Vulkan INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/vulkan-sdk/include")
if(WIN32)
    set_target_properties(Vulkan::Vulkan PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/vulkan-sdk/lib/vulkan-1.lib")
else()
    set_target_properties(Vulkan::Vulkan PROPERTIES
        IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/vulkan-sdk/lib/linux-x64/libvulkan.so.1")
endif()
