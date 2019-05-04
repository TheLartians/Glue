
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
      message(FATAL_ERROR "Source and build directories cannot be the same.")
endif()

# ---- Configuration variables ----

option(GLUE_ENABLE_LUA "build glue for use with lua" OFF)
option(GLUE_BUILD_LUA "build lua with glue" OFF)
option(GLUE_ENABLE_DUKTAPE "build glue for use with duktape" OFF)
option(GLUE_BUILD_DUKTAPE "build duktape with glue" OFF)
option(GLUE_ENABLE_TESTS "Enable tests" OFF)

option(GLUE_LUA_LIB "The lua library to link with glue.")
option(GLUE_DUKTAPE_LIB "The duktape library to link with glue.")

# ---- create project ----

project(Glue 
  VERSION 0.7
  LANGUAGES CXX
)

# ---- Dependencies ----

include(cmake/CPM.cmake)

if(TARGET Glue)
  return()
endif()

if (${GLUE_BUILD_LUA})
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lua)
  set(GLUE_LUA_LIB GlueLua)
endif()

if (${GLUE_BUILD_DUKTAPE})
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/duktape)
  set(GLUE_DUKTAPE_LIB GlueDuktape)
endif()

CPMAddPackage(
  NAME LHC
  VERSION 0.7
  GIT_REPOSITORY https://github.com/TheLartians/LHC.git
)

CPMAddPackage(
  NAME LarsEvent
  VERSION 2.1
  GIT_REPOSITORY https://github.com/TheLartians/Event.git
)

CPMAddPackage(
  NAME LarsVisitor
  VERSION 1.6
  GIT_REPOSITORY https://github.com/TheLartians/Visitor.git
)

# ---- enable C++ ----

if(NOT CMAKE_CXX_STANDARD GREATER 17)
  set (CMAKE_CXX_STANDARD 17)
endif()

# ---- create module ----

set(glue_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)

set(sources
  ${glue_source_dir}/element.cpp
)

set(headers
  ${CMAKE_CURRENT_SOURCE_DIR}/include/glue/element.h
)

if(${GLUE_ENABLE_DUKTAPE})
  list(APPEND sources ${glue_source_dir}/duktape.cpp)
  list(APPEND headers ${CMAKE_CURRENT_SOURCE_DIR}/include/glue/duktape.h)
endif()

if(${GLUE_ENABLE_LUA})
  list(APPEND sources ${glue_source_dir}/lua.cpp)
  list(APPEND headers ${CMAKE_CURRENT_SOURCE_DIR}/include/glue/lua.h)
endif()

add_library(Glue ${sources} ${headers})
set_target_properties(Glue PROPERTIES COMPILE_FLAGS "-Wall -pedantic -Wextra -Werror")

if(${GLUE_ENABLE_LUA})
  target_link_libraries(Glue PUBLIC ${GLUE_LUA_LIB})  
endif()

if(${GLUE_ENABLE_DUKTAPE})
  target_link_libraries(Glue PUBLIC ${GLUE_DUKTAPE_LIB})  
endif()

target_link_libraries(Glue PUBLIC LHC LarsEvent LarsVisitor)

target_include_directories(Glue
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>    
)

# ---- Install ----

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${PROJECT_BINARY_DIR}/GlueConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(
  TARGETS Glue
  EXPORT GlueTargets 
  LIBRARY DESTINATION lib COMPONENT Runtime
  ARCHIVE DESTINATION lib COMPONENT Development
  RUNTIME DESTINATION bin COMPONENT Runtime
  PUBLIC_HEADER DESTINATION include COMPONENT Development
  BUNDLE DESTINATION bin COMPONENT Runtime
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/GlueConfig.cmake.in"
  "${PROJECT_BINARY_DIR}/GlueConfig.cmake"
  INSTALL_DESTINATION lib/cmake/Glue
)

install(
  EXPORT GlueTargets 
  DESTINATION lib/cmake/Glue
)

install(
  FILES 
    "${PROJECT_BINARY_DIR}/GlueConfigVersion.cmake"
    "${PROJECT_BINARY_DIR}/GlueConfig.cmake"
  DESTINATION 
    lib/cmake/Glue
)

install(
  DIRECTORY ${PROJECT_SOURCE_DIR}/include/
  DESTINATION include
)

# ---- Test ----

if(${GLUE_ENABLE_TESTS})
  ENABLE_TESTING()  
  add_subdirectory(tests)  
endif()
