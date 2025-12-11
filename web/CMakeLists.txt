cmake_minimum_required(VERSION 3.22.0)

project(mapscore
  LANGUAGES C CXX
)

# TODO need to reorganize this cmake project; too many differences between targets.
# it seems the only sensible way is to have per-target project files and have
# some shared defintions with cmake-macros etc.

file(GLOB_RECURSE mapscore_SRC CONFIGURE_DEPENDS
  "maps-core/shared/public/*.cpp"
  "maps-core/shared/src/*.cpp"
  "maps-core/android/src/main/cpp/graphics/*.cpp"
  "maps-core/android/src/main/cpp/utils/*.cpp"
  "maps-core/external/djinni/support-lib/cpp/*.cpp"
)
file(GLOB webmapsdk_SRC CONFIGURE_DEPENDS
  "maps-core/web-extras/*.cpp"
  "maps-core/bridging/wasm/*.cpp"
  "maps-core/external/djinni/support-lib/wasm/*.cpp"
)

file(GLOB_RECURSE layeranimation_SRC CONFIGURE_DEPENDS
  "layer-animation/shared/src/*.cpp"
  "layer-animation/bridging/wasm/*.cpp"
  # XXX: Filtered WindParticle 
  layer-animation/android/src/main/cpp/graphics/LayerAnimationGraphicsFactoryOpenGl.cpp
  layer-animation/android/src/main/cpp/graphics/LayerAnimationShaderFactoryOpenGl.cpp
  layer-animation/android/src/main/cpp/graphics/objects/Quad3dInterpolatedOpenGl.cpp
  layer-animation/android/src/main/cpp/graphics/shader/CloudInterpolationShaderOpenGl.cpp
  layer-animation/android/src/main/cpp/graphics/shader/FullSpritePatternInterpolationShaderOpenGl.cpp
  layer-animation/android/src/main/cpp/graphics/shader/InterpolationShaderCommonsOpenGl.cpp
  layer-animation/android/src/main/cpp/graphics/shader/PrecipitationShaderOpenGl.cpp
  layer-animation/android/src/main/cpp/graphics/shader/TextureInterpolationShaderOpenGl.cpp
  layer-animation/android/src/main/cpp/graphics/shader/WindAreaInterpolationShaderOpenGl.cpp
  layer-animation/android/src/main/cpp/graphics/shader/WindPatternShaderOpenGl.cpp

  layer-animation/web-extras/WebAnimationLayerCallback.cpp
  layer-animation/web-extras/BitmapTextureHolder.cpp
)

file(GLOB_RECURSE webp_SRC CONFIGURE_DEPENDS
  "layer-animation/external/libwebp/src/*.c"
)
add_library(webp STATIC ${webp_SRC})
target_include_directories(webp PRIVATE layer-animation/external/libwebp)

add_library(fpng STATIC "layer-animation/external/fpng/src/fpng.cpp")
target_include_directories(fpng PUBLIC layer-animation/external/fpng/public)


add_executable(webmapsdk ${mapscore_SRC} ${webmapsdk_SRC} ${layeranimation_SRC})

target_include_directories(webmapsdk PRIVATE
  maps-core/external/protozero/protozero/include/
  maps-core/external/vtzero/vtzero/include/
  maps-core/external/earcut/earcut/include/mapbox/
  maps-core/shared/src
  maps-core/shared/src/external/pugixml
  maps-core/shared/src/external/gpc
  maps-core/shared/src/logger
  maps-core/shared/src/graphics
  maps-core/shared/src/graphics/helpers
  maps-core/shared/src/map
  maps-core/shared/src/map/camera
  maps-core/shared/src/map/controls
  maps-core/shared/src/map/coordinates
  maps-core/shared/src/map/layers
  maps-core/shared/src/map/layers/objects
  maps-core/shared/src/map/layers/tiled
  maps-core/shared/src/map/layers/tiled/raster
  maps-core/shared/src/map/layers/tiled/wmts
  maps-core/shared/src/map/layers/tiled/vector
  maps-core/shared/src/map/layers/tiled/vector/geojson
  maps-core/shared/src/map/layers/tiled/vector/geojson/geojsonvt
  maps-core/shared/src/map/layers/tiled/vector/tiles
  maps-core/shared/src/map/layers/tiled/vector/tiles/raster
  maps-core/shared/src/map/layers/tiled/vector/tiles/polygon
  maps-core/shared/src/map/layers/tiled/vector/tiles/line
  maps-core/shared/src/map/layers/tiled/vector/sourcemanagers
  maps-core/shared/src/map/layers/tiled/vector/sublayers
  maps-core/shared/src/map/layers/tiled/vector/sublayers/raster
  maps-core/shared/src/map/layers/tiled/vector/sublayers/line
  maps-core/shared/src/map/layers/tiled/vector/sublayers/polygon
  maps-core/shared/src/map/layers/tiled/vector/sublayers/symbol
  maps-core/shared/src/map/layers/tiled/vector/sublayers/background
  maps-core/shared/src/map/layers/tiled/vector/symbol
  maps-core/shared/src/map/layers/tiled/vector/description
  maps-core/shared/src/map/layers/tiled/vector/parsing
  maps-core/shared/src/map/layers/polygon
  maps-core/shared/src/map/layers/icon
  maps-core/shared/src/map/layers/line
  maps-core/shared/src/map/layers/text
  maps-core/shared/src/map/scheduling
  maps-core/shared/src/utils
  maps-core/android/src/main/cpp
  maps-core/android/src/main/cpp/graphics
  maps-core/android/src/main/cpp/graphics/objects
  maps-core/android/src/main/cpp/graphics/shader
  maps-core/android/src/main/cpp/utils
  maps-core/shared/public
  maps-core/bridging/wasm
  maps-core/external/djinni/support-lib/
  maps-core/external/djinni/support-lib/cpp
  maps-core/external/djinni/support-lib/wasm
  layer-animation/shared/src
  layer-animation/shared/src/models
  layer-animation/shared/src/animation
  layer-animation/shared/src/animation/layerObject
  layer-animation/shared/src/animation/util
  layer-animation/shared/src/animation/tiled
  layer-animation/shared/src/animation/tiled/source
  layer-animation/shared/src/networking
  layer-animation/shared/src/networking/decoder
  layer-animation/shared/public
  layer-animation/bridging/wasm
  layer-animation/android/src/main/cpp/graphics
  layer-animation/android/src/main/cpp/graphics/objects
  layer-animation/android/src/main/cpp/graphics/shader
)
set_property(
  SOURCE maps-core/shared/src/logger/Logger.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS
  $<$<CONFIG:Debug>:LOG_LEVEL=4>    # LogTrace
  $<$<CONFIG:Release>:LOG_LEVEL=1>) # LogWarning

set_property(
  SOURCE layer-animation/shared/src/networking/decoder/WebpAnimationDecoder.cpp
  APPEND
  PROPERTY INCLUDE_DIRECTORIES
  ${PROJECT_SOURCE_DIR}/layer-animation/external/libwebp/src/
)

target_compile_definitions(webmapsdk PRIVATE OPENMOBILEMAPS_GL=1)

target_compile_features(webmapsdk PRIVATE cxx_std_20)
target_compile_options(webmapsdk PRIVATE -sUSE_PTHREADS=1 -Werror -Wno-deprecated -Wno-reorder)

target_link_libraries(webmapsdk PRIVATE webp fpng)
target_link_options(webmapsdk PRIVATE
  -lembind
  -sMODULARIZE=1
  -sEXPORT_ES6=1
  -sEXPORT_NAME=MainModuleFactory
  -sMIN_WEBGL_VERSION=2
  -sMAX_WEBGL_VERSION=2
  -sGL_PREINITIALIZED_CONTEXT
  -sFETCH
  -sUSE_PTHREADS=1
  -sPTHREAD_POOL_SIZE=3
  -sSTACK_SIZE=128MB
  -sDEFAULT_PTHREAD_STACK_SIZE=128MB
  -sINITIAL_MEMORY=2000MB
  -sSHARED_MEMORY=1
  -sMALLOC=mimalloc
  -sENVIRONMENT=web,worker
  --no-entry
  $<$<CONFIG:Debug>:-sASSERTIONS=2 -sEXCEPTION_DEBUG=1>
  $<$<OR:$<CONFIG:RelWithDebInfo>,$<CONFIG:Debug>>:-gseparate-dwarf -gsource-map>
)

set_target_properties(webmapsdk PROPERTIES 
  OUTPUT_NAME webmapsdk.js
)
