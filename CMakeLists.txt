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

function(be_copy_assets target)
    file(GLOB_RECURSE _assets CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")
    set(_stamp "${CMAKE_CURRENT_BINARY_DIR}/${target}_assets.stamp")
    add_custom_command(OUTPUT "${_stamp}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:${target}>/assets"
        COMMAND ${CMAKE_COMMAND} -E touch "${_stamp}"
        DEPENDS ${_assets}
        COMMENT "Copying assets for ${target}")
    add_custom_target(${target}_assets ALL DEPENDS "${_stamp}")
    add_dependencies(${target} ${target}_assets)
endfunction()

function(be_symlink_assets target)
    set(_src "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    set(_dst "$<TARGET_FILE_DIR:${target}>/assets")
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${_dst}"
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${_src}" "${_dst}"
    )
endfunction()

function(be_declare_shader_module module_target subdir)
    set_property(TARGET ${module_target} APPEND
        PROPERTY BE_SHADER_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}")
endfunction()

function(_be_link_closure target out_var)
    set(seen "")
    set(pending ${target})
    while(pending)
        list(POP_FRONT pending cur)
        get_target_property(libs ${cur} LINK_LIBRARIES)
        foreach(lib ${libs})
            if(TARGET ${lib} AND NOT lib IN_LIST seen)
                list(APPEND seen ${lib})
                list(APPEND pending ${lib})
            endif()
        endforeach()
    endwhile()
    set(${out_var} "${seen}" PARENT_SCOPE)
endfunction()

function(be_deploy_shader_modules exe)
    _be_link_closure(${exe} closure)
    list(APPEND closure ${exe})

    set(dirs "")
    foreach(t ${closure})
        get_target_property(d ${t} BE_SHADER_DIRS)
        if(d)
            list(APPEND dirs ${d})
        endif()
    endforeach()
    if(NOT dirs)
        return()
    endif()

    set(patterns "")
    set(copy_cmds "")
    set(dst "$<TARGET_FILE_DIR:${exe}>/module-shaders")
    foreach(d ${dirs})
        list(APPEND patterns "${d}/*")
        list(APPEND copy_cmds COMMAND ${CMAKE_COMMAND} -E copy_directory "${d}" "${dst}")
    endforeach()
    file(GLOB_RECURSE srcs CONFIGURE_DEPENDS ${patterns})

    set(stamp "${CMAKE_CURRENT_BINARY_DIR}/${exe}_module_shaders.stamp")
    add_custom_command(OUTPUT "${stamp}" ${copy_cmds}
        COMMAND ${CMAKE_COMMAND} -E touch "${stamp}"
        DEPENDS ${srcs}
        COMMENT "Deploying shader modules for ${exe}")
    add_custom_target(${exe}_module_shaders ALL DEPENDS "${stamp}")
    add_dependencies(${exe} ${exe}_module_shaders)
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
add_subdirectory(example-game-1)
add_subdirectory(example-sakura)
add_subdirectory(example-vulkan)
add_subdirectory(devtools/shader-boilerplate-autogen)
