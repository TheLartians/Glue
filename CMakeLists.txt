
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
file (GLOB headers ${PROJECT_INCUDE_DIR}/tape/*.h ${PROJECT_SOURCE_DIR}/tape/*.h)

file (GLOB ducktape_sources ${PROJECT_LIB_DIR}/ducktape/*.c ${PROJECT_LIB_DIR}/ducktape/*.cpp)
file (GLOB ducktape_headers ${PROJECT_LIB_DIR}/ducktape/*.h)

set (DUCKTAPE_INCUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ducktape)

# ---- enable c++14 ----

if(NOT CMAKE_CXX_STANDARD GREATER 14)
  set (CMAKE_CXX_STANDARD 14)
endif()

# ---- create module ----

# duktape
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set_source_files_properties($${ducktape_sources} PROPERTIES LANGUAGE CXX )
add_library(duktape STATIC ${ducktape_sources} ${ducktape_headers})


# tape
add_library(glue STATIC ${sources} ${headers})
target_link_libraries(glue duktape)  


# ---- includes ----

IF(LARS_LIBRARY)
  SET(lars_include_dirs ${lars_include_dirs} ${PROJECT_INCUDE_DIR} PARENT_SCOPE)
  SET(lars_libraries ${lars_libraries} glue duktape PARENT_SCOPE)
  SET(Lars_INCLUDE_DIRS ${lars_include_dirs})
ELSE()
  message("Glue has to be built as a lars library module")
ENDIF()

# ---- examples ----

IF(LARS_BUILD_EXAMPLES)
  file( GLOB example_sources ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp )
  foreach( examplesourcefile ${example_sources} )
    get_filename_component(filename ${examplesourcefile} NAME)
    string( REPLACE ".cpp" "" examplename ${filename} )
    set(examplename lars_glue_${examplename})
    add_executable( ${examplename} ${examplesourcefile})
    include_directories(${Lars_INCLUDE_DIRS} "${PROJECT_INCUDE_DIR}")
    target_link_libraries(${examplename} glue duktape)
  endforeach()
ENDIF()




