file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS *.cpp *.h *.hpp)

add_library(core STATIC ${CORE_SOURCES})

target_compile_features(core PUBLIC cxx_std_23)

target_include_directories(core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(core
    PUBLIC
        glfw
        Vulkan::Vulkan
        libassert::assert
        glm
        nlohmann_json
        stb_image
    PRIVATE
        slang)

be_apply_defaults(core)
