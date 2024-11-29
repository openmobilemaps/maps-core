cmake_minimum_required(VERSION 3.24.0)
project(mapscore
  LANGUAGES CXX
)

####
# mapscore OpenGL "proper", i.e. the C++ implementation without language bindings
####
file(GLOB_RECURSE mapscore_SRC
  "shared/*.cpp"
  "android/src/main/cpp/graphics/*.cpp"
)

add_library(mapscore STATIC ${mapscore_SRC})

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
target_compile_definitions(mapscore PRIVATE OPENMOBILEMAPS_GL=1)
target_compile_features(mapscore PRIVATE cxx_std_20)
target_compile_options(mapscore PRIVATE -Werror -Wno-deprecated -Wno-reorder -fPIC) # fPIC so we can "embed" into shared mapscore_jni
target_link_libraries(mapscore ${OPENGL_LIBRARIES})

add_subdirectory(standalone)
add_subdirectory(jvm)
