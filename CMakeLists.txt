cmake_minimum_required(VERSION 3.24.0)
project(mapscore)

####
# mapscore "proper", i.e. the C++ implementation without language bindings
####
file(GLOB_RECURSE mapscore_SRC
  "shared/*.cpp"
  "android/src/main/cpp/graphics/*.cpp"
)

add_library(mapscore STATIC ${mapscore_SRC})

# TODO: this picks up mesa (libgl-dev) and makes it a runtime dependency. Should we attempt to use an independent GL loader (glad/glew?) instead?
find_package(OpenGL)

target_include_directories(mapscore PRIVATE
  external/protozero/protozero/include/
  external/vtzero/vtzero/include/
  external/earcut/earcut/include/mapbox/
  shared/src
  shared/src/external/pugixml
  shared/src/external/gpc
  shared/src/logger
  shared/src/graphics
  shared/src/graphics/helpers
  shared/src/map
  shared/src/map/camera
  shared/src/map/controls
  shared/src/map/coordinates
  shared/src/map/layers
  shared/src/map/layers/objects
  shared/src/map/layers/tiled
  shared/src/map/layers/tiled/raster
  shared/src/map/layers/tiled/wmts
  shared/src/map/layers/tiled/vector
  shared/src/map/layers/tiled/vector/geojson
  shared/src/map/layers/tiled/vector/geojson/geojsonvt
  shared/src/map/layers/tiled/vector/tiles
  shared/src/map/layers/tiled/vector/tiles/raster
  shared/src/map/layers/tiled/vector/tiles/polygon
  shared/src/map/layers/tiled/vector/tiles/line
  shared/src/map/layers/tiled/vector/sourcemanagers
  shared/src/map/layers/tiled/vector/sublayers
  shared/src/map/layers/tiled/vector/sublayers/raster
  shared/src/map/layers/tiled/vector/sublayers/line
  shared/src/map/layers/tiled/vector/sublayers/polygon
  shared/src/map/layers/tiled/vector/sublayers/symbol
  shared/src/map/layers/tiled/vector/sublayers/background
  shared/src/map/layers/tiled/vector/symbol
  shared/src/map/layers/tiled/vector/description
  shared/src/map/layers/tiled/vector/parsing
  shared/src/map/layers/polygon
  shared/src/map/layers/icon
  shared/src/map/layers/line
  shared/src/map/layers/text
  shared/src/map/scheduling
  shared/src/utils
  android/src/main/cpp
  android/src/main/cpp/graphics
  android/src/main/cpp/graphics/objects
  android/src/main/cpp/graphics/shader
)
target_include_directories(mapscore PUBLIC
  shared/public
  external/djinni/support-lib/
  external/djinni/support-lib/cpp
)
set_property(
  SOURCE shared/src/logger/Logger.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS
  $<$<CONFIG:Debug>:LOG_LEVEL=4>    # LogTrace
  $<$<CONFIG:Release>:LOG_LEVEL=1>) # LogWarning
target_compile_features(mapscore PRIVATE cxx_std_20)
target_compile_options(mapscore PRIVATE -Wno-deprecated -Wno-reorder -fPIC) # fPIC so we can "embed" into shared mapscore_jni
target_link_libraries(mapscore ${OPENGL_LIBRARIES})

####
# mapscore-jni
####
find_package(JNI)
find_library(OSMESA_LIBRARIES 
  NAMES OSMesa 
  REQUIRED)
find_path(OSMESA_INCLUDE_DIRS 
  NAMES "GL/osmesa.h" 
  REQUIRED)

file(GLOB_RECURSE mapscore_jni_SRC
  "android/src/main/cpp/scheduling/*.cpp"
  "external/djinni/support-lib/cpp/*.cpp"
  "external/djinni/support-lib/jni/*.cpp"
  "bridging/android/jni/*.cpp"
  "jvm/src/main/cpp/jni/*.cpp"
)

add_library(mapscore_jni SHARED ${mapscore_jni_SRC})

target_include_directories(mapscore_jni PRIVATE
  android/src/main/cpp/scheduling
  external/djinni/support-lib/jni
  ${JNI_INCLUDE_DIRS}
  
  bridging/android/jni/graphics
  bridging/android/jni/graphics/common
  bridging/android/jni/graphics/objects
  bridging/android/jni/graphics/shader
  bridging/android/jni/map
  bridging/android/jni/map/controls
  bridging/android/jni/map/layers
  bridging/android/jni/map/layers/objects
  bridging/android/jni/map/layers/tiled
  bridging/android/jni/map/layers/tiled/raster
  bridging/android/jni/map/layers/tiled/vector
  bridging/android/jni/map/layers/polygon
  bridging/android/jni/map/layers/icon
  bridging/android/jni/map/layers/line
  bridging/android/jni/map/layers/text
  bridging/android/jni/map/loader
  bridging/android/jni/map/scheduling
  bridging/android/jni/map/coordinates
  bridging/android/jni/map/camera

  ${OSMESA_INCLUDE_DIRS}
)
target_compile_features(mapscore_jni PRIVATE cxx_std_20)
target_compile_definitions(mapscore_jni PRIVATE DATAREF_JNI=1) # see external/djinni/support-lib/cpp/DataRef.hpp
target_compile_options(mapscore_jni PRIVATE -Wno-deprecated -Wno-reorder)
target_link_libraries(mapscore_jni mapscore ${OSMESA_LIBRARIES})
# Fiddle with cmake system  TARGET_OS/TARGET_ARCH to form string "linux_amd64" for library filename.
string(TOLOWER ${CMAKE_SYSTEM_NAME} TARGET_OS)
string(REPLACE "x86_64" "amd64" TARGET_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
set_target_properties(mapscore_jni PROPERTIES 
  LIBRARY_OUTPUT_NAME_RELEASE mapscore_jni_${TARGET_OS}_${TARGET_ARCHITECTURE}
  LIBRARY_OUTPUT_NAME_DEBUG mapscore_jni_${TARGET_OS}_${TARGET_ARCHITECTURE}_debug
)


add_executable(testmain main.cpp)
target_compile_features(testmain PRIVATE cxx_std_20)
target_compile_options(testmain PRIVATE -Wall)
target_include_directories(testmain PRIVATE ${OSMESA_INCLUDE_DIRS})
target_link_libraries(testmain mapscore ${OSMESA_LIBRARIES})
