include(${CMAKE_CURRENT_LIST_DIR}/cxx.cmake)

####
# add_library_mapscore(name type:[STATIC|SHARED|...] with_GL:[WITH_GL|WITHOUT_GL])
#
# Add library target "name" for the maps-core shared C++ core without language
# bindings -- with/without OpenGL graphics module.
####
function(add_library_mapscore name type arg_with_GL)

  if(arg_with_GL STREQUAL "WITH_GL")
    set(with_GL TRUE)
  elseif(arg_with_GL STREQUAL "WITHOUT_GL")
    set(with_GL FALSE)
  else()
    message(FATAL_ERROR "invalid argument '${arg_with_GL}', signature is: add_library_mapscore(<name> <type> [WITH_GL|WITHOUT_GL])")
  endif()

  set(mapscore_DIR ${CMAKE_CURRENT_FUNCTION_LIST_DIR})

  file(GLOB_RECURSE mapscore_SRC CONFIGURE_DEPENDS
    "${mapscore_DIR}/shared/public/*.cpp"
    "${mapscore_DIR}/shared/src/*.cpp"
    "${mapscore_DIR}/external/djinni/support-lib/cpp/*.cpp"
  )

  if(with_GL)
    file(GLOB_RECURSE mapscore_GL_SRC CONFIGURE_DEPENDS
      "${mapscore_DIR}/android/src/main/cpp/graphics/*.cpp"
      "${mapscore_DIR}/android/src/main/cpp/utils/*.cpp"
    )
  else()
    set(mapscore_GL_SRC "")
  endif()
  add_library(${name} ${type} ${mapscore_SRC} ${mapscore_GL_SRC})
  
  target_include_directories(${name} PRIVATE
    ${mapscore_DIR}/shared/src
    ${mapscore_DIR}/shared/src/external/pugixml
    ${mapscore_DIR}/shared/src/external/gpc
    ${mapscore_DIR}/shared/src/logger
    ${mapscore_DIR}/shared/src/graphics
    ${mapscore_DIR}/shared/src/graphics/helpers
    ${mapscore_DIR}/shared/src/map
    ${mapscore_DIR}/shared/src/map/camera
    ${mapscore_DIR}/shared/src/map/controls
    ${mapscore_DIR}/shared/src/map/coordinates
    ${mapscore_DIR}/shared/src/map/layers
    ${mapscore_DIR}/shared/src/map/layers/objects
    ${mapscore_DIR}/shared/src/map/layers/tiled
    ${mapscore_DIR}/shared/src/map/layers/tiled/raster
    ${mapscore_DIR}/shared/src/map/layers/tiled/wmts
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/geojson
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/geojson/geojsonvt
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/tiles
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/tiles/circle
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/tiles/raster
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/tiles/polygon
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/tiles/line
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/sourcemanagers
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/sublayers
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/sublayers/raster
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/sublayers/line
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/sublayers/polygon
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/sublayers/symbol
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/sublayers/background
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/symbol
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/description
    ${mapscore_DIR}/shared/src/map/layers/tiled/vector/parsing
    ${mapscore_DIR}/shared/src/map/layers/polygon
    ${mapscore_DIR}/shared/src/map/layers/icon
    ${mapscore_DIR}/shared/src/map/layers/line
    ${mapscore_DIR}/shared/src/map/layers/text
    ${mapscore_DIR}/shared/src/map/scheduling
    ${mapscore_DIR}/shared/src/utils
  )
  target_include_directories(${name} SYSTEM PRIVATE
    ${mapscore_DIR}/external/earcut/earcut/include/mapbox/
    ${mapscore_DIR}/external/protozero/protozero/include/
    ${mapscore_DIR}/external/vtzero/vtzero/include/
  )
  if(with_GL)
    target_include_directories(${name} PUBLIC
      ${mapscore_DIR}/android/src/main/cpp
      ${mapscore_DIR}/android/src/main/cpp/graphics
      ${mapscore_DIR}/android/src/main/cpp/graphics/objects
      ${mapscore_DIR}/android/src/main/cpp/graphics/shader
      ${mapscore_DIR}/android/src/main/cpp/utils
    )
  endif()
  target_include_directories(${name} PUBLIC
    ${mapscore_DIR}/shared/public
  )
  target_include_directories(${name} SYSTEM PUBLIC
    ${mapscore_DIR}/external/djinni/support-lib/
    ${mapscore_DIR}/external/djinni/support-lib/cpp
  )

  set_property(
    SOURCE ${mapscore_DIR}/shared/src/logger/Logger.cpp
    APPEND
    PROPERTY COMPILE_DEFINITIONS
    $<$<CONFIG:Debug>:LOG_LEVEL=4>  # LogTrace
    $<$<OR:$<CONFIG:RelWithDebInfo>,$<CONFIG:Release>>:LOG_LEVEL=1>  # LogWarning
  )

  openmobilemaps__target_default_compile_options(${name})
  if(with_GL)
    target_compile_definitions(${name} PRIVATE OPENMOBILEMAPS_GL=1)
  endif()
  # -Wno-deprecated-declarations: djinni uses some deprecated functions in its cpp support library headers. 
  #   Clang thinks that its important enough to warn about (certain) deprecations even in system headers.
  target_compile_options(${name} PRIVATE -Wno-deprecated-declarations)
endfunction()



####
# add_library_mapscore_jni(name type:[STATIC|SHARED|...])
#
# Add library target "name" for the maps-core JNI bindings.
####
function(add_library_mapscore_jni name type)
  set(mapscore_DIR ${CMAKE_CURRENT_FUNCTION_LIST_DIR})

  file(GLOB_RECURSE mapscore_jni_SRC CONFIGURE_DEPENDS
    "${mapscore_DIR}/bridging/android/jni/*.cpp"
    "${mapscore_DIR}/external/djinni/support-lib/jni/*.cpp"
  )

  add_library(${name} ${type} ${mapscore_jni_SRC})
  target_include_directories(${name} SYSTEM PRIVATE
    ${mapscore_DIR}/external/djinni/support-lib/jni
  )
  target_include_directories(${name} PRIVATE
    ${mapscore_DIR}/bridging/android/jni/graphics
    ${mapscore_DIR}/bridging/android/jni/graphics/common
    ${mapscore_DIR}/bridging/android/jni/graphics/objects
    ${mapscore_DIR}/bridging/android/jni/graphics/shader
    ${mapscore_DIR}/bridging/android/jni/map
    ${mapscore_DIR}/bridging/android/jni/map/controls
    ${mapscore_DIR}/bridging/android/jni/map/layers
    ${mapscore_DIR}/bridging/android/jni/map/layers/effect
    ${mapscore_DIR}/bridging/android/jni/map/layers/icon
    ${mapscore_DIR}/bridging/android/jni/map/layers/line
    ${mapscore_DIR}/bridging/android/jni/map/layers/objects
    ${mapscore_DIR}/bridging/android/jni/map/layers/polygon
    ${mapscore_DIR}/bridging/android/jni/map/layers/skysphere
    ${mapscore_DIR}/bridging/android/jni/map/layers/text
    ${mapscore_DIR}/bridging/android/jni/map/layers/tiled
    ${mapscore_DIR}/bridging/android/jni/map/layers/tiled/raster
    ${mapscore_DIR}/bridging/android/jni/map/layers/tiled/vector
    ${mapscore_DIR}/bridging/android/jni/map/loader
    ${mapscore_DIR}/bridging/android/jni/map/scheduling
    ${mapscore_DIR}/bridging/android/jni/map/coordinates
    ${mapscore_DIR}/bridging/android/jni/map/camera
    ${mapscore_DIR}/bridging/android/jni/utils
  )
  openmobilemaps__target_default_compile_options(${name})
  target_compile_options(${name} PRIVATE -Wno-deprecated-declarations)
  # Disable warnings in djinni support lib
  set_property(
    SOURCE ${mapscore_DIR}/external/djinni/support-lib/jni/djinni_support.cpp
    APPEND
    PROPERTY COMPILE_OPTIONS
    -Wno-deprecated
  )
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set_property(
      SOURCE ${mapscore_DIR}/external/djinni/support-lib/jni/djinni_support.cpp
      APPEND
      PROPERTY COMPILE_OPTIONS
      -Wno-unused)
  endif()
endfunction()
