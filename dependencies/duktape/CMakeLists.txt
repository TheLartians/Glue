cmake_minimum_required(VERSION 3.13)

project(duktape C)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/DownloadProject.cmake)

download_project(
  PROJ duktape
  URL https://duktape.org/duktape-2.3.0.tar.xz
  URL_HASH MD5=352105b39979fc766bbd0b3721e8c2b5
  UPDATE_DISCONNECTED 1
  QUIET
)

FILE(GLOB duktape_sources ${duktape_SOURCE_DIR}/src/*.c)
FILE(GLOB duktape_headers ${duktape_SOURCE_DIR}/src/*.h)

add_library(libduktape STATIC ${duktape_sources} ${duktape_headers})

target_include_directories(libduktape
  PUBLIC
    $<BUILD_INTERFACE:${duktape_SOURCE_DIR}/src>
)

install(
  TARGETS libduktape
  EXPORT libduktapeTargets
  LIBRARY DESTINATION lib COMPONENT Runtime
  ARCHIVE DESTINATION lib COMPONENT Development
  RUNTIME DESTINATION bin COMPONENT Runtime
  PUBLIC_HEADER DESTINATION include COMPONENT Development
  BUNDLE DESTINATION bin COMPONENT Runtime
)

install(
  EXPORT libduktapeTargets 
  DESTINATION lib
)
