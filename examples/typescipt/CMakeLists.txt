cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(TSExample)

add_executable(TSExample main.cpp)
set_target_properties(TSExample PROPERTIES CXX_STANDARD 17 COMPILE_FLAGS "-Wall -pedantic -Wextra -Werror")

include(cmake/CPM.cmake)

CPMAddPackage(
  NAME Glue
  VERSION 0.9.1
  GIT_REPOSITORY https://github.com/TheLartians/Glue.git
  GIT_TAG 54c4b8c93294404ef927c82fd20603aee500df38
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

function(GlueCreateTypescriptTarget target directory)  
  ADD_CUSTOM_COMMAND(
    OUTPUT ${directory}/node_modules
    DEPENDS ${directory}/package.json
    COMMAND npm install
    COMMAND touch node_modules
    WORKING_DIRECTORY ${directory}
  )

  ADD_CUSTOM_TARGET(
    ${target}_init
    DEPENDS ${directory}/node_modules
  )

  file(GLOB_RECURSE sources "${directory}/src/*.ts")

  ADD_CUSTOM_COMMAND(
    OUTPUT ${directory}/lua
    DEPENDS ${sources}
    COMMAND npm run build
    COMMAND touch lua
    WORKING_DIRECTORY ${directory}
  )

  ADD_CUSTOM_TARGET(
    ${target} ALL
    DEPENDS ${directory}/lua ${directory}/node_modules
  )

  add_dependencies(${target} ${target}_init)

endfunction()

GlueCreateTypescriptTarget(TSExampleCompileTypescript ${CMAKE_CURRENT_SOURCE_DIR})
add_dependencies(TSExample TSExampleCompileTypescript)
