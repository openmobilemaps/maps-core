cmake_minimum_required(VERSION 3.22.0)

project(mapscore
  LANGUAGES C CXX
)

include(../cxx.cmake)

####
# maps-core shared C++ with OpenGL
####
include(../maps-core.cmake)
add_library_mapscore(mapscore STATIC WITH_GL)
target_compile_options(mapscore PRIVATE -sUSE_PTHREADS=1)

####
# maps-core WASM bindings
####
file(GLOB mapscore_web_SRC CONFIGURE_DEPENDS
  "../web/src/*.cpp"
  "../bridging/wasm/*.cpp"
  "../external/djinni/support-lib/wasm/*.cpp"
)
add_library(mapscore_web OBJECT ${mapscore_web_SRC})
target_include_directories(mapscore_web PUBLIC
  ../bridging/wasm
  ../external/djinni/support-lib/wasm
)
openmobilemaps__target_default_compile_options(mapscore_web)
target_link_libraries(mapscore_web PUBLIC mapscore)
target_compile_options(mapscore_web PRIVATE -sUSE_PTHREADS=1)
