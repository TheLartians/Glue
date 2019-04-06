
cmake_minimum_required (VERSION 2.6)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
      message(FATAL_ERROR "Source and build directories cannot be the same.")
endif()

# ---- Configuration variables ----
option(GLUE_USE_DUKTAPE "build glue for use with duktape" OFF)
option(GLUE_USE_LUA "build glue for use with lua" OFF)
option(GLUE_BUILD_LUA "build lua with glue" OFF)
option(GLUE_BUILD_DUKTAPE "build duktape with glue" OFF)

# ---- create project ----

project(Glue CXX)

# ---- Dependencies ----

if (GLUE_BUILD_LUA)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lua)
endif()

find_package(LHC 0.1 QUIET)
if(NOT ${LHC_FOUND})
  add_subdirectory(dependencies/LHC)  
endif()

# ---- enable c++14 ----

if(NOT CMAKE_CXX_STANDARD GREATER 14)
  set (CMAKE_CXX_STANDARD 14)
endif()

# ---- create module ----

set(glue_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)

set(sources ${glue_source_dir}/glue.cpp)
file(GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/include/lars/*.h)

if(GLUE_USE_DUKTAPE)
  set(sources ${sources} ${PROJECT_SOURCE_DIR}/duktape.cpp)
  include_directories(${duktape_INCLUDE_DIRS})
endif()

if(GLUE_USE_LUA)
  list(APPEND sources ${glue_source_dir}/lua.cpp)
endif()

add_library(Glue ${sources} ${headers})

target_link_libraries(Glue PUBLIC LHC)

if(GLUE_USE_LUA)
  target_link_libraries(Glue PUBLIC lua)  
endif()

target_include_directories(Glue
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>    
)

# ---- install ----


# ---- examples ----

