cmake_minimum_required(VERSION 3.13)

project(lua C)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/DownloadProject.cmake)

download_project(
  PROJ lua
  GIT_REPOSITORY https://github.com/LuaDist/lua.git
  GIT_TAG 5.3.2
#  SOURCE_DIR ${CMAKE_BINARY_DIR}/lua
  UPDATE_DISCONNECTED 1
  QUIET
)

add_subdirectory(${lua_SOURCE_DIR} ${CMAKE_BINARY_DIR}/lua)

#FILE(GLOB lua_sources ${lua_SOURCE_DIR}/*.c)
#FILE(GLOB lua_headers ${lua_SOURCE_DIR}/*.h)

#add_library(lua STATIC ${lua_sources} ${lua_headers})

#target_include_directories(lua
#  PUBLIC
#    $<BUILD_INTERFACE:${lua_SOURCE_DIR}/..>
#    $<INSTALL_INTERFACE:include>    
#)

#install(
#  TARGETS lua
#  EXPORT luaCTargets
#  LIBRARY DESTINATION lib/lua COMPONENT Runtime
#  ARCHIVE DESTINATION lib/lua COMPONENT Development
#  RUNTIME DESTINATION bin/lua COMPONENT Runtime
#  PUBLIC_HEADER DESTINATION include/lua COMPONENT Development
#  BUNDLE DESTINATION bin/lua COMPONENT Runtime
#)
