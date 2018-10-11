
file (GLOB sources "*.c")
file (GLOB headers "*.h*")

# SET_SOURCE_FILES_PROPERTIES( ${sources} PROPERTIES LANGUAGE CXX )

FILE(GLOB modules ${CMAKE_CURRENT_SOURCE_DIR}/modules/*)
include_directories(${modules} ${CMAKE_CURRENT_SOURCE_DIR})

add_library(lua STATIC ${sources} ${headers} )

set(lua_LIBRARIES lua CACHE STRING "lua libraries")
set(lua_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE STRING "lua include dirs")

install (TARGETS lua DESTINATION lib)

