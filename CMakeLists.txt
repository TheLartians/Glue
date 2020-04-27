cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# ---- Project ----

# Note: update this to your new project's name and version
project(Glue 
  VERSION 1.0
  LANGUAGES CXX
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there.")
endif()

# --- Import tools ----

include(cmake/tools.cmake)

# ---- Add dependencies via CPM ----

include(cmake/CPM.cmake)

CPMAddPackage(
  NAME PackageProject.cmake
  GITHUB_REPOSITORY TheLartians/PackageProject.cmake
  VERSION 1.2
)

CPMAddPackage(
  NAME EasyIterator
  VERSION 1.4
  GIT_REPOSITORY https://github.com/TheLartians/EasyIterator.git
)

CPMAddPackage(
  NAME Observe
  VERSION 3.0
  GITHUB_REPOSITORY TheLartians/Observe
)

CPMAddPackage(
  NAME Revisited
  VERSION 2.2
  GITHUB_REPOSITORY TheLartians/Revisited
)

CPMAddPackage(
  NAME GroupSourcesByFolder.cmake
  GITHUB_REPOSITORY TheLartians/GroupSourcesByFolder.cmake
  VERSION 1.0
)

# ---- Add source files ----

FILE(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
FILE(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# ---- Create library ----

add_library(Glue ${headers} ${sources})
GroupSourcesByFolder(Glue)

set_target_properties(Glue PROPERTIES CXX_STANDARD 17)

# beeing a cross-platform target, we enforce enforce standards conformance on MSVC
target_compile_options(Glue PUBLIC "$<$<BOOL:${MSVC}>:/permissive->")

target_link_libraries(Glue PUBLIC EasyIterator Revisited Observe)

target_include_directories(Glue
  PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

# ---- Create an installable target ----
# this allows users to install and find the library via `find_package()`.

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  DEPENDENCIES "Observe;Revisited;EasyIterator"
)
