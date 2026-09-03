file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.h *.hpp)

add_executable(bechef ${SOURCES})
target_link_libraries(bechef PRIVATE core)

be_apply_defaults(bechef)

add_custom_command(TARGET bechef POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:bechef>
        ${CMAKE_SOURCE_DIR}/$<TARGET_FILE_NAME:bechef>)
