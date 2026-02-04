cmake_minimum_required(VERSION 3.22.0)

project(mapscore
  LANGUAGES C CXX
)

# TODO need to reorganize this cmake project; too many differences between targets.
# it seems the only sensible way is to have per-target project files and have
# some shared defintions with cmake-macros etc.

file(GLOB_RECURSE mapscore_SRC CONFIGURE_DEPENDS
  "../shared/public/*.cpp"
  "../shared/src/*.cpp"
  "../android/src/main/cpp/graphics/*.cpp"
  "../android/src/main/cpp/utils/*.cpp"
  "../external/djinni/support-lib/cpp/*.cpp"
)
# No tessellation support in webgl; skipping to even build this, as it requires some functions (glPatchParameteri) that are not available in emscripten gl.
list(FILTER mapscore_SRC EXCLUDE REGEX
  "Polygon2dTessellatedOpenGl.cpp$|Quad2dTessellatedOpenGl.cpp$"
)

file(GLOB webmapsdk_SRC CONFIGURE_DEPENDS
  "../web/src/*.cpp"
  "../bridging/wasm/*.cpp"
  "../external/djinni/support-lib/wasm/*.cpp"
)

# file(GLOB_RECURSE webp_SRC CONFIGURE_DEPENDS
#   "layer-animation/external/libwebp/src/*.c"
# )
# add_library(webp STATIC ${webp_SRC})
# target_include_directories(webp PRIVATE layer-animation/external/libwebp)

# add_library(fpng STATIC "layer-animation/external/fpng/src/fpng.cpp")
# target_include_directories(fpng PUBLIC layer-animation/external/fpng/public)


add_executable(webmapsdk ${mapscore_SRC} ${webmapsdk_SRC}) # ${layeranimation_SRC})

target_include_directories(webmapsdk PRIVATE
  ../external/protozero/protozero/include/
  ../external/vtzero/vtzero/include/
  ../external/earcut/earcut/include/mapbox/
  ../shared/src
  ../shared/src/external/pugixml
  ../shared/src/external/gpc
  ../shared/src/logger
  ../shared/src/graphics
  ../shared/src/graphics/helpers
  ../shared/src/map
  ../shared/src/map/camera
  ../shared/src/map/controls
  ../shared/src/map/coordinates
  ../shared/src/map/layers
  ../shared/src/map/layers/objects
  ../shared/src/map/layers/tiled
  ../shared/src/map/layers/tiled/raster
  ../shared/src/map/layers/tiled/wmts
  ../shared/src/map/layers/tiled/vector
  ../shared/src/map/layers/tiled/vector/geojson
  ../shared/src/map/layers/tiled/vector/geojson/geojsonvt
  ../shared/src/map/layers/tiled/vector/tiles
  ../shared/src/map/layers/tiled/vector/tiles/raster
  ../shared/src/map/layers/tiled/vector/tiles/polygon
  ../shared/src/map/layers/tiled/vector/tiles/line
  ../shared/src/map/layers/tiled/vector/sourcemanagers
  ../shared/src/map/layers/tiled/vector/sublayers
  ../shared/src/map/layers/tiled/vector/sublayers/raster
  ../shared/src/map/layers/tiled/vector/sublayers/line
  ../shared/src/map/layers/tiled/vector/sublayers/polygon
  ../shared/src/map/layers/tiled/vector/sublayers/symbol
  ../shared/src/map/layers/tiled/vector/sublayers/background
  ../shared/src/map/layers/tiled/vector/symbol
  ../shared/src/map/layers/tiled/vector/description
  ../shared/src/map/layers/tiled/vector/parsing
  ../shared/src/map/layers/polygon
  ../shared/src/map/layers/icon
  ../shared/src/map/layers/line
  ../shared/src/map/layers/text
  ../shared/src/map/scheduling
  ../shared/src/utils
  ../android/src/main/cpp
  ../android/src/main/cpp/graphics
  ../android/src/main/cpp/graphics/objects
  ../android/src/main/cpp/graphics/shader
  ../android/src/main/cpp/utils
  ../shared/public
  ../bridging/wasm
  ../external/djinni/support-lib/
  ../external/djinni/support-lib/cpp
  ../external/djinni/support-lib/wasm
)
set_property(
  SOURCE maps-core/shared/src/logger/Logger.cpp
  APPEND
  PROPERTY COMPILE_DEFINITIONS
  $<$<CONFIG:Debug>:LOG_LEVEL=4>    # LogTrace
  $<$<CONFIG:Release>:LOG_LEVEL=1>) # LogWarning

# set_property(
#   SOURCE layer-animation/shared/src/networking/decoder/WebpAnimationDecoder.cpp
#   APPEND
#   PROPERTY INCLUDE_DIRECTORIES
#   ${PROJECT_SOURCE_DIR}/layer-animation/external/libwebp/src/
# )

target_compile_definitions(webmapsdk PRIVATE OPENMOBILEMAPS_GL=1)

target_compile_features(webmapsdk PRIVATE cxx_std_20)
target_compile_options(webmapsdk PRIVATE -sUSE_PTHREADS=1 -Werror -Wno-deprecated -Wno-reorder)

target_link_libraries(webmapsdk PRIVATE) # webp fpng)
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
