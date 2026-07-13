file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.h *.hpp)
file(GLOB_RECURSE HLSL_FILES CONFIGURE_DEPENDS shaders/*.hlsl shaders/*.hlsli)
set_source_files_properties(${HLSL_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(example-game-1 ${SOURCES} ${HLSL_FILES})
target_include_directories(example-game-1 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(example-game-1 PRIVATE core toolkit)

be_apply_defaults(example-game-1)
be_cook_app(example-game-1)
be_copy_runtime_deps(example-game-1)
