cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

if(TARGET LarsVisitor)
  return()
endif()

set(LarsVisitor_MIN_VERSION 0.1)

find_package(LarsVisitor ${LarsVisitor_MIN_VERSION} QUIET)

if(${LarsVisitor_FOUND})
  set_target_properties(LarsVisitor
    PROPERTIES
      IMPORTED_GLOBAL True
  )
else()
  include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/DownloadProject.cmake)

  download_project(
    PROJ LarsVisitor
    GIT_REPOSITORY https://github.com/TheLartians/Visitor.git
    GIT_TAG update-dependencies # "v${LarsVisitor_MIN_VERSION}"
    UPDATE_DISCONNECTED 1
    QUIET
  )

  add_subdirectory(${LarsVisitor_SOURCE_DIR} ${LarsVisitor_BINARY_DIR})
endif()
