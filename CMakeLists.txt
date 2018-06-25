
cmake_minimum_required (VERSION 2.6)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
      message(FATAL_ERROR "Source and build directories cannot be the same.")
endif()

# ---- create project ----

project(Tape)

set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/source)
set(PROJECT_INCUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)


# ---- add sources ----

file (GLOB sources ${PROJECT_SOURCE_DIR}/*.cpp)
file (GLOB headers ${PROJECT_INCUDE_DIR}/tape/*.h ${PROJECT_SOURCE_DIR}/tape/*.h)

file (GLOB ducktape_sources ${CMAKE_CURRENT_SOURCE_DIR}/ducktape/*.c ${CMAKE_CURRENT_SOURCE_DIR}/ducktape/*.cpp)
file (GLOB ducktape_headers ${CMAKE_CURRENT_SOURCE_DIR}/ducktape/*.h)


set (DUCKTAPE_INCUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ducktape)

# ---- enable c++14 ----

if(NOT CMAKE_CXX_STANDARD GREATER 14)
  set (CMAKE_CXX_STANDARD 14)
endif()

# ---- create module ----

# duktape
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
set_source_files_properties($${ducktape_sources} PROPERTIES LANGUAGE CXX )
add_library(ducktape STATIC ${ducktape_sources} ${ducktape_headers})


# tape
add_library(tape STATIC ${sources} ${headers})
target_link_libraries(tape ducktape)  


# ---- includes ----

IF(LARS_LIBRARY)
  SET(lars_include_dirs ${lars_include_dirs} "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)
  SET(lars_libraries ${lars_libraries} tape ducktape PARENT_SCOPE)
  SET(Lars_INCLUDE_DIRS ${lars_include_dirs})
ELSE()
  message("GLUE has to be built as a lars library module")
ENDIF()

# ---- examples ----

IF( LARS_BUILD_EXAMPLES )
  add_executable (tape_example ${CMAKE_CURRENT_SOURCE_DIR}/example/main.cpp)
  include_directories(${DUCKTAPE_INCUDE_DIR})
  include_directories(${lars_include_dirs})
  target_link_libraries(tape_example tape ducktape)  
endif()



