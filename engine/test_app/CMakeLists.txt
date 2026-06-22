add_executable(test_app test.cpp)
set_target_properties(test_app
  PROPERTIES
  CXX_STANDARD 26
  CXX_STANDARD_REQUIRED ON
  # OUTPUT_NAME "test_app"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
)
get_target_property(std test_app CXX_STANDARD)
get_target_property(extensions test_app CXX_EXTENSIONS)
get_target_property(out_name test_app OUTPUT_NAME)
get_target_property(out_dir test_app RUNTIME_OUTPUT_DIRECTORY)
message(STATUS "-----------------------------------------------")
message(STATUS "TEST             : ${out_name}")
message(STATUS "C++ Standard     : ${std}")
message(STATUS "C++ Extensions   : ${extensions}")
message(STATUS "Output directory : ${out_dir}")

# Link against engine
target_link_libraries(test_app
  PRIVATE
  engine
)

add_custom_command(
  TARGET test_app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_BINARY_DIR}/compile_commands.json
  ${CMAKE_SOURCE_DIR}/compile_commands.json
)
