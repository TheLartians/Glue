cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

include(FetchContent)

project(GlueLua
  VERSION 5.3.4
  LANGUAGES C
)

CPMAddPackage(
  NAME lua
  GIT_REPOSITORY https://github.com/lua/lua.git
  VERSION 5-3-4
  GIT_SHALLOW YES
  DOWNLOAD_ONLY YES
)

FetchContent_GetProperties(lua)
if(NOT lua_POPULATED)
  FetchContent_Populate(lua)
endif()

FILE(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
FILE(GLOB lua_headers ${lua_SOURCE_DIR}/*.h)

add_library(GlueLua STATIC ${lua_sources} ${lua_headers})

target_include_directories(GlueLua
  PUBLIC
    $<BUILD_INTERFACE:${lua_SOURCE_DIR}>
)

# ---- Install ----

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${PROJECT_BINARY_DIR}/GlueLuaConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(
  TARGETS GlueLua
  EXPORT GlueLuaTargets 
  LIBRARY DESTINATION lib COMPONENT Runtime
  ARCHIVE DESTINATION lib COMPONENT Development
  RUNTIME DESTINATION bin COMPONENT Runtime
  PUBLIC_HEADER DESTINATION include COMPONENT Development
  BUNDLE DESTINATION bin COMPONENT Runtime
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/GlueLuaConfig.cmake.in"
  "${PROJECT_BINARY_DIR}/GlueLuaConfig.cmake"
  INSTALL_DESTINATION lib/cmake/GlueLua
)

install(
  EXPORT GlueLuaTargets 
  DESTINATION lib/cmake/GlueLua
)

install(
  FILES 
    "${PROJECT_BINARY_DIR}/GlueLuaConfigVersion.cmake"
    "${PROJECT_BINARY_DIR}/GlueLuaConfig.cmake"
  DESTINATION 
    lib/cmake/GlueLua
)
