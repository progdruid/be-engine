file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.h *.hpp)
file(GLOB_RECURSE HLSL_FILES CONFIGURE_DEPENDS assets/shaders/*.hlsl assets/shaders/*.hlsli)
set_source_files_properties(${HLSL_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(example-sakura ${SOURCES} ${HLSL_FILES})
target_include_directories(example-sakura PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(example-sakura PRIVATE core toolkit)

be_apply_defaults(example-sakura)
be_copy_assets(example-sakura)
be_copy_runtime_deps(example-sakura)
