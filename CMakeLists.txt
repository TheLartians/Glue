
cmake_minimum_required (VERSION 2.6)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
      message(FATAL_ERROR "Source and build directories cannot be the same.")
endif()

# ---- create project ----

project(Tape)

set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(PROJECT_INCUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)


# ---- add sources ----

file (GLOB sources ${PROJECT_SOURCE_DIR}/*.cpp)
file (GLOB headers ${PROJECT_INCUDE_DIR}/lars/*.h ${PROJECT_SOURCE_DIR}/*.h)

# ---- enable c++14 ----

if(NOT CMAKE_CXX_STANDARD GREATER 14)
  set (CMAKE_CXX_STANDARD 14)
endif()

# ---- create module ----

# duktape
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/lib/duktape)
include_directories(${duktape_INCLUDE_DIRS})  

# lua
ADD_SUBDIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/lib/lua)
include_directories(${lua_INCLUDE_DIRS})  

# glue
add_library(lars_glue STATIC ${sources} ${headers})

target_link_libraries(lars_glue ${duktape_LIBRARIES} ${LUA_LIBRARIES})  

# ---- includes ----

IF(LARS_LIBRARY)
  SET(lars_include_dirs ${lars_include_dirs} ${PROJECT_INCUDE_DIR} PARENT_SCOPE)
  SET(lars_libraries ${lars_libraries} lars_glue ${duktape_LIBRARIES} ${LUA_LIBRARIES} PARENT_SCOPE)
  SET(Lars_INCLUDE_DIRS ${lars_include_dirs})
  include_directories(${lars_include_dirs}  ${PROJECT_INCUDE_DIR} )
ELSE()
  message("Glue has to be built as a lars library module")
ENDIF()

# ---- export ----

set(Glue_INCLUDE_DIRS ${lars_include_dirs} ${lua_INCLUDE_DIRS} ${duktape_INCLUDE_DIRS} CACHE INTERNAL "")
set(Glue_LIBRARIES lars_glue ${lua_LIBRARIES} ${duktape_LIBRARIES} CACHE INTERNAL "")

# ---- examples ----

IF(LARS_BUILD_EXAMPLES)
  file( GLOB example_sources ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp )
  foreach( examplesourcefile ${example_sources} )
    get_filename_component(filename ${examplesourcefile} NAME)
    string( REPLACE ".cpp" "" examplename ${filename} )
    set(examplename lars_glue_${examplename})
    add_executable( ${examplename} ${examplesourcefile})
    include_directories(${Lars_INCLUDE_DIRS} ${Glue_INCLUDE_DIRS})
    target_link_libraries(${examplename} ${Glue_LIBRARIES})
  endforeach()
ENDIF()

