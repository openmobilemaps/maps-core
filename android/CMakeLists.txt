cmake_minimum_required(VERSION 3.22.0)
project(mapscore_android
  LANGUAGES C CXX
)

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif (CCACHE_FOUND)


####
# maps-core shared C++ core with OpenGL
####
include(../maps-core.cmake)
add_library_mapscore(mapscore_base OBJECT WITH_GL)

####
# maps-core with JNI bindings and android libraries
####
add_library_mapscore_jni(mapscore SHARED)
find_library(LOG_LIBRARIES log)
find_library(OPENGL_LIBRARIES GLESv3)

target_link_libraries(mapscore 
  mapscore_base
  ${LOG_LIBRARIES}
  ${OPENGL_LIBRARIES}
)
