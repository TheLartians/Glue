cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

if(TARGET LarsEvent)
  return()
endif()

set(LarsEvent_MIN_VERSION 1.0)

find_package(LarsEvent ${LarsEvent_MIN_VERSION} QUIET)

if(${LarsEvent_FOUND})
  set_target_properties(LarsEvent
    PROPERTIES
      IMPORTED_GLOBAL True
  )
else()
  include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/DownloadProject.cmake)

  download_project(
    PROJ LarsEvent
    GIT_REPOSITORY https://github.com/TheLartians/LarsEvent.git
    GIT_TAG "v${LarsEvent_MIN_VERSION}"
    UPDATE_DISCONNECTED 1
    GIT_SHALLOW 1
  )

  add_subdirectory(${LarsEvent_SOURCE_DIR} ${LarsEvent_BINARY_DIR})
endif()

