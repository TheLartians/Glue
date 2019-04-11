cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

include(FetchContent)

project(Duktape
  VERSION 2.3.0
  LANGUAGES C
)

FetchContent_Declare(
  duktape
  URL https://duktape.org/duktape-2.3.0.tar.xz
  URL_HASH MD5=352105b39979fc766bbd0b3721e8c2b5
  UPDATE_DISCONNECTED 1
  QUIET
)

FetchContent_GetProperties(duktape)
if(NOT duktape_POPULATED)
  FetchContent_Populate(duktape)
endif()

FILE(GLOB duktape_sources ${duktape_SOURCE_DIR}/src/*.c)
FILE(GLOB duktape_headers ${duktape_SOURCE_DIR}/src/*.h)

add_library(GlueDuktape STATIC ${duktape_sources} ${duktape_headers})

target_include_directories(GlueDuktape
  PUBLIC
    $<BUILD_INTERFACE:${duktape_SOURCE_DIR}/src>
)

# ---- Install ----

include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  "${PROJECT_BINARY_DIR}/GlueDuktapeConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(
  TARGETS GlueDuktape
  EXPORT GlueDuktapeTargets 
  LIBRARY DESTINATION lib COMPONENT Runtime
  ARCHIVE DESTINATION lib COMPONENT Development
  RUNTIME DESTINATION bin COMPONENT Runtime
  PUBLIC_HEADER DESTINATION include COMPONENT Development
  BUNDLE DESTINATION bin COMPONENT Runtime
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/GlueDuktapeConfig.cmake.in"
  "${PROJECT_BINARY_DIR}/GlueDuktapeConfig.cmake"
  INSTALL_DESTINATION lib/cmake/GlueDuktape
)

install(
  EXPORT GlueDuktapeTargets 
  DESTINATION lib/cmake/GlueDuktape
)

install(
  FILES 
    "${PROJECT_BINARY_DIR}/GlueDuktapeConfigVersion.cmake"
    "${PROJECT_BINARY_DIR}/GlueDuktapeConfig.cmake"
  DESTINATION 
    lib/cmake/GlueDuktape
)

install(
  DIRECTORY ${PROJECT_SOURCE_DIR}/include/
  DESTINATION include
)
