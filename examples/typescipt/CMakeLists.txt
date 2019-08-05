cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(TSExample)

add_executable(TSExample lib/main.cpp)
set_target_properties(TSExample PROPERTIES CXX_STANDARD 17 COMPILE_FLAGS "-Wall -pedantic -Wextra -Werror")

include(cmake/CPM.cmake)

CPMAddPackage(
  NAME Glue
  VERSION 0.9.1
  GIT_REPOSITORY https://github.com/TheLartians/Glue.git
  GIT_TAG 594ddce8cb3ff991d91d7bd49c5e9f53d49790a3
  OPTIONS
    "GLUE_ENABLE_LUA YES"
    "GLUE_BUILD_LUA YES"
)

CPMAddPackage(
  NAME cxxopts
  GITHUB_REPOSITORY jarro2783/cxxopts
  VERSION 2.2.0
  OPTIONS
    "CXXOPTS_BUILD_EXAMPLES Off"
    "CXXOPTS_BUILD_TESTS Off"
)

target_link_libraries(TSExample Glue cxxopts)

GlueCreateTypescriptTarget(TSExampleCompileTypescript ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(TSExample TSExampleCompileTypescript)
