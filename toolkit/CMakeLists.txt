file(GLOB_RECURSE TOOLKIT_SOURCES CONFIGURE_DEPENDS *.cpp *.h *.hpp)

add_library(toolkit STATIC ${TOOLKIT_SOURCES})

target_include_directories(toolkit
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(toolkit 
        PUBLIC core
        PRIVATE assimp::assimp
)

be_apply_defaults(toolkit)
be_declare_shader_module(toolkit standard-render-machine/shaders)
