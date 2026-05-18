file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.h *.hpp)

add_executable(shader-boilerplate-autogen ${SOURCES})
target_link_libraries(shader-boilerplate-autogen PRIVATE core)

be_apply_defaults(shader-boilerplate-autogen)

add_custom_command(TARGET shader-boilerplate-autogen POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:shader-boilerplate-autogen>
        ${CMAKE_SOURCE_DIR}/devtools/$<TARGET_FILE_NAME:shader-boilerplate-autogen>)
