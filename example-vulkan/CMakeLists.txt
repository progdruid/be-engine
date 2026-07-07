file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.h *.hpp)
file(GLOB_RECURSE HLSL_FILES CONFIGURE_DEPENDS assets/shaders/*.hlsl assets/shaders/*.hlsli)
set_source_files_properties(${HLSL_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(example-vulkan ${SOURCES} ${HLSL_FILES})
target_link_libraries(example-vulkan PRIVATE core)

be_apply_defaults(example-vulkan)
be_deploy_app(example-vulkan)
be_copy_runtime_deps(example-vulkan)
