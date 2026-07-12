cmake_minimum_required(VERSION 3.25)
project(be CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)


# ── helpers ───────────────────────────────────────────────────────────────────

function(be_apply_defaults target)
    target_compile_options(${target} PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus /Zc:preprocessor>)
    target_compile_definitions(${target} PRIVATE
        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Release>:NDEBUG>)
endfunction()

function(be_deploy_app exe)
    if(BE_SYMLINK_ASSETS)
        set(mode symlink)
    else()
        set(mode copy)
    endif()
    set(_stamp "${CMAKE_CURRENT_BINARY_DIR}/${exe}_deploy.stamp")
    add_custom_command(OUTPUT "${_stamp}"
        COMMAND $<TARGET_FILE:bechef> shadergen
            --root "${CMAKE_SOURCE_DIR}" --check
        COMMAND $<TARGET_FILE:bechef> deploy
            --root "${CMAKE_SOURCE_DIR}" --app ${exe}
            --out "$<TARGET_FILE_DIR:${exe}>" --mode ${mode}
            --depfile "${_stamp}.d"
        COMMAND ${CMAKE_COMMAND} -E touch "${_stamp}"
        DEPFILE "${_stamp}.d"
        DEPENDS bechef
        COMMENT "Deploying content for ${exe}")
    add_custom_target(${exe}_deploy ALL DEPENDS "${_stamp}")
    add_dependencies(${exe} ${exe}_deploy)
endfunction()

function(be_copy_runtime_deps target)
    if(WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/vendor/slang/bin/slang-compiler.dll"
                "$<TARGET_FILE_DIR:${target}>")
    else()
        set_target_properties(${target} PROPERTIES
            BUILD_RPATH  "$ORIGIN"
            INSTALL_RPATH "$ORIGIN")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND bash -c "cp -P '${CMAKE_SOURCE_DIR}/vendor/slang/linux-x64/lib/'*.so* '$<TARGET_FILE_DIR:${target}>'"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_SOURCE_DIR}/vendor/vulkan-sdk/lib/linux-x64/libvulkan.so.1"
                "$<TARGET_FILE_DIR:${target}>")
    endif()

    # Validation layer travels next to the exe (debug only); Init() points VK_ADD_LAYER_PATH here.
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/vendor/vulkan-sdk/share/vulkan/explicit_layer.d/$<IF:$<PLATFORM_ID:Windows>,windows,linux>"
                "$<TARGET_FILE_DIR:${target}>/layers")
    endif()
endfunction()


# ── targets ───────────────────────────────────────────────────────────────────

add_subdirectory(vendor)
add_subdirectory(core)
add_subdirectory(toolkit)
add_subdirectory(devtools/bechef)
add_subdirectory(example-game-1)
add_subdirectory(example-sakura)
add_subdirectory(example-vulkan)
