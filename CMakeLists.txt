
cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

if (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
      message(FATAL_ERROR "Source and build directories cannot be the same.")
endif()

# ---- Configuration variables ----

option(GLUE_USE_DUKTAPE "build glue for use with duktape" OFF)
option(GLUE_USE_LUA "build glue for use with lua" OFF)
option(GLUE_BUILD_LUA "build lua with glue" OFF)
option(GLUE_BUILD_DUKTAPE "build duktape with glue" OFF)
option(ENABLE_LARS_GLUE_TESTS "Enable tests" OFF)

# ---- create project ----

project(LarsGlue 
  VERSION 0.1
  LANGUAGES CXX
)

# ---- Dependencies ----

if (GLUE_BUILD_LUA)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/dependencies/lua)
endif()

add_subdirectory(dependencies/LHC)
add_subdirectory(dependencies/Event)
add_subdirectory(dependencies/Visitor)

# ---- enable c++14 ----

if(NOT CMAKE_CXX_STANDARD GREATER 17)
  set (CMAKE_CXX_STANDARD 17)
endif()

# ---- create module ----

set(glue_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/source)

set(sources ${glue_source_dir}/glue.cpp)
file(GLOB headers ${CMAKE_CURRENT_SOURCE_DIR}/include/lars/*.h)

if(GLUE_USE_DUKTAPE)
  list(APPEND sources ${glue_source_dir}/duktape.cpp)
endif()

if(GLUE_USE_LUA)
  list(APPEND sources ${glue_source_dir}/lua.cpp)
endif()

add_library(Glue ${sources} ${headers})

target_link_libraries(Glue PUBLIC LHC LarsEvent LarsGlue)

if(GLUE_USE_LUA)
  target_link_libraries(Glue PUBLIC lua)  
endif()

target_include_directories(Glue
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>    
)

# ---- Install ----

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${PROJECT_BINARY_DIR}/LarsGlueConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(
  TARGETS LarsGlue
  EXPORT LarsGlueTargets
  LIBRARY DESTINATION lib COMPONENT Runtime
  ARCHIVE DESTINATION lib COMPONENT Development
  RUNTIME DESTINATION bin COMPONENT Runtime
  PUBLIC_HEADER DESTINATION include COMPONENT Development
  BUNDLE DESTINATION bin COMPONENT Runtime
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/LarsGlueConfig.cmake.in"
  "${PROJECT_BINARY_DIR}/LarsGlueConfig.cmake"
  INSTALL_DESTINATION lib/cmake/LarsGlue
)

install(
  EXPORT LarsGlueTargets 
  DESTINATION lib/cmake/LarsGlue
)

install(
  FILES 
    "${PROJECT_BINARY_DIR}/LarsGlueConfigVersion.cmake"
    "${PROJECT_BINARY_DIR}/LarsGlueConfig.cmake"
  DESTINATION 
    lib/cmake/LarsGlue
)

install(
  DIRECTORY ${PROJECT_SOURCE_DIR}/include/
  DESTINATION include
)

# ---- Test ----

if(${ENABLE_LARS_GLUE_TESTS})
  ENABLE_TESTING()  
  add_subdirectory(tests)  
endif()
