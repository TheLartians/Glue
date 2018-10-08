
file (GLOB_RECURSE sources "*.cpp")
file (GLOB_RECURSE headers "*.h*")

add_library(duktape STATIC ${sources} ${headers} )

set(duktape_LIBRARIES duktape CACHE STRING "duktape libraries")
set(duktape_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include CACHE STRING "duktape include dirs")

install (TARGETS duktape DESTINATION lib)

