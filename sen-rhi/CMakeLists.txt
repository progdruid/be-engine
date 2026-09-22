file(GLOB_RECURSE SEN_SOURCES CONFIGURE_DEPENDS *.cpp *.h)

add_library(sen STATIC ${SEN_SOURCES})

target_compile_features(sen PUBLIC cxx_std_23)

target_include_directories(sen
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(sen
    PUBLIC
        Vulkan::Vulkan
        libassert::assert
        umbrellas)

be_apply_defaults(sen)
