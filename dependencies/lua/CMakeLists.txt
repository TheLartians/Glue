cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

include(FetchContent)

project(lua C)

FetchContent_Declare(
  lua
  GIT_REPOSITORY https://github.com/lua/lua.git
  GIT_TAG v5-3-4
  GIT_SHALLOW 1
  UPDATE_DISCONNECTED 1
  QUIET
)

FetchContent_GetProperties(lua)
if(NOT lua_POPULATED)
  FetchContent_Populate(lua)
endif()

FILE(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
FILE(GLOB lua_headers ${lua_SOURCE_DIR}/*.h)

add_library(liblua STATIC ${lua_sources} ${lua_headers})

target_include_directories(liblua
  PUBLIC
    $<BUILD_INTERFACE:${lua_SOURCE_DIR}>
)

install(
  TARGETS liblua
  EXPORT libluaTargets
  LIBRARY DESTINATION lib/lua COMPONENT Runtime
  ARCHIVE DESTINATION lib/lua COMPONENT Development
  RUNTIME DESTINATION bin/lua COMPONENT Runtime
  PUBLIC_HEADER DESTINATION include/lua COMPONENT Development
  BUNDLE DESTINATION bin/lua COMPONENT Runtime
)

install(
  EXPORT libluaTargets 
  DESTINATION lib 
)