file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS *.cpp *.h *.hpp)

add_library(core STATIC ${CORE_SOURCES})

target_include_directories(core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/vendor"
        "${CMAKE_SOURCE_DIR}/vendor/slang/include")

target_link_libraries(core
    PUBLIC
        glfw
        Vulkan::Vulkan
        libassert::assert
    PRIVATE
        slang)

be_apply_defaults(core)
be_declare_shader_module(core shaders)
