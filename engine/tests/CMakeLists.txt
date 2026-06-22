add_executable(engine_tests
  tests.cpp
)
set_target_properties(engine_tests
  PROPERTIES
  CXX_STANDARD 26
  CXX_STANDARD_REQUIRED ON
  # OUTPUT_NAME "engine_tests"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)
get_target_property(std engine_tests CXX_STANDARD)
get_target_property(extensions engine_tests CXX_EXTENSIONS)
get_target_property(out_name engine_tests OUTPUT_NAME)
get_target_property(out_dir engine_tests RUNTIME_OUTPUT_DIRECTORY)
message(STATUS "-----------------------------------------------")
message(STATUS "TEST             : ${out_name}")
message(STATUS "C++ Standard     : ${std}")
message(STATUS "C++ Extensions   : ${extensions}")
message(STATUS "Output directory : ${out_dir}")

# target_sources(engine_tests
#   PRIVATE
#   # List your source files
#   tests.cpp
# )
# Fetch googletest
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0  # newer is better
)
if(WIN32)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(googletest)

# Link against engine and googletests
target_link_libraries(engine_tests
  PRIVATE
  engine
  GTest::gtest_main
)
# Make googletest see our executable
include(GoogleTest)
gtest_discover_tests(engine_tests
  EXTRA_ARGS --gtest_color=yes
)

add_custom_command(
    TARGET engine_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
)
