file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS *.cpp *.h *.hpp)

add_library(core STATIC ${CORE_SOURCES})

target_include_directories(core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_SOURCE_DIR}/vendor")

target_link_libraries(core
    PUBLIC
        glfw
        Vulkan::Vulkan
        libassert::assert
    PRIVATE
        assimp::assimp
        slang)

be_apply_defaults(core)
