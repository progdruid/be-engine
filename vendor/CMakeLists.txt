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

# ── Lua 5.4 ───────────────────────────────────────────────────────────────────
FetchContent_Declare(lua
    URL https://www.lua.org/ftp/lua-5.4.7.tar.gz
    URL_HASH SHA256=9fbf5e28ef86c69858f6d3d34eccc32e911c1a28b4120ff3e84aaa70cfbf1e30)
FetchContent_MakeAvailable(lua)

# Compile Lua from source
file(GLOB LUA_SOURCES "${lua_SOURCE_DIR}/src/*.c")
list(FILTER LUA_SOURCES EXCLUDE REGEX "(lua|luac)\\.c$")
add_library(lua STATIC ${LUA_SOURCES})
target_include_directories(lua PUBLIC "${lua_SOURCE_DIR}/src")
set_target_properties(lua PROPERTIES POSITION_INDEPENDENT_CODE ON)

# ── LuaBridge3 (header-only) ──────────────────────────────────────────────────
FetchContent_Declare(luabridge3
    GIT_REPOSITORY https://github.com/kunitoki/LuaBridge3.git
    GIT_TAG 3.0-rc4)
FetchContent_MakeAvailable(luabridge3)
add_library(luabridge3 INTERFACE)
target_include_directories(luabridge3 INTERFACE "${luabridge3_SOURCE_DIR}/Source")
target_link_libraries(luabridge3 INTERFACE lua)

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
