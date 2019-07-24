cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

include(FetchContent)

project(GlueLua
  VERSION 5.3.5
  LANGUAGES C
)

CPMAddPackage(
  NAME GlueLua
  GIT_REPOSITORY https://github.com/lua/lua.git
  VERSION 5.3.5
  GIT_SHALLOW YES
  DOWNLOAD_ONLY YES
)

if (GlueLua_ADDED)
  # lua has no CMakeLists, so we create our own target
  FILE(GLOB GlueLua_sources ${GlueLua_SOURCE_DIR}/*.c)
  add_library(GlueLua STATIC ${GlueLua_sources})

  target_include_directories(GlueLua
    PUBLIC
      $<BUILD_INTERFACE:${GlueLua_SOURCE_DIR}>
  )
endif()

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
