file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS *.cpp *.h *.hpp)

add_executable(example-quickstart ${SOURCES})
target_link_libraries(example-quickstart PRIVATE core toolkit)

be_apply_defaults(example-quickstart)
be_cook_app(example-quickstart)
be_copy_runtime_deps(example-quickstart)
