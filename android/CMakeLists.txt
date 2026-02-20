# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)
project(mapscore_android
  LANGUAGES CXX
)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif (CCACHE_FOUND)

file(GLOB_RECURSE cpp_SRC CONFIGURE_DEPENDS
        "../bridging/android/jni/*.cpp"
        "../shared/public/*.cpp"
        "../shared/src/*.cpp"
        "src/main/cpp/*.cpp"
        "../external/djinni/support-lib/cpp/*.cpp"
        "../external/djinni/support-lib/jni/*.cpp"
        )

add_library( # Sets the name of the library.
        mapscore

        # Sets the library as a shared library.
        SHARED

        ${cpp_SRC})

target_include_directories(mapscore SYSTEM PRIVATE
        ../external/djinni/support-lib/jni
        ../external/protozero/protozero/include/
        ../external/vtzero/vtzero/include/
        ../external/earcut/earcut/include/mapbox/
)
target_include_directories(mapscore PRIVATE
        ../bridging/android/jni/graphics
        ../bridging/android/jni/graphics/common
        ../bridging/android/jni/graphics/objects
        ../bridging/android/jni/graphics/shader
        ../bridging/android/jni/map
        ../bridging/android/jni/map/controls
        ../bridging/android/jni/map/layers
        ../bridging/android/jni/map/layers/effect
        ../bridging/android/jni/map/layers/icon
        ../bridging/android/jni/map/layers/line
        ../bridging/android/jni/map/layers/objects
        ../bridging/android/jni/map/layers/polygon
        ../bridging/android/jni/map/layers/skysphere
        ../bridging/android/jni/map/layers/text
        ../bridging/android/jni/map/layers/tiled
        ../bridging/android/jni/map/layers/tiled/raster
        ../bridging/android/jni/map/layers/tiled/vector
        ../bridging/android/jni/map/loader
        ../bridging/android/jni/map/scheduling
        ../bridging/android/jni/map/coordinates
        ../bridging/android/jni/map/camera
        ../bridging/android/jni/utils
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
        ../shared/src/map/layers/effect
        ../shared/src/map/layers/icon
        ../shared/src/map/layers/objects
        ../shared/src/map/layers/polygon
        ../shared/src/map/layers/line
        ../shared/src/map/layers/skysphere
        ../shared/src/map/layers/text
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
        ../shared/src/map/scheduling
        ../shared/src/utils
        src/main/cpp
        src/main/cpp/graphics
        src/main/cpp/graphics/objects
        src/main/cpp/graphics/shader
        src/main/cpp/utils
        src/main/cpp/scheduling
)
target_include_directories(mapscore PUBLIC
        ../shared/public
)
target_include_directories(mapscore SYSTEM PUBLIC
        ../shared/public
        ../external/djinni/support-lib/
        ../external/djinni/support-lib/cpp
)

target_compile_definitions(mapscore PRIVATE OPENMOBILEMAPS_GL=1)
target_compile_options(mapscore PRIVATE -Werror -Wunused -Wundef -Wno-reorder)

# Disable warnings in djinni support lib
set_property(
  SOURCE ../external/djinni/support-lib/jni/djinni_support.cpp
  APPEND
  PROPERTY COMPILE_OPTIONS
  -Wno-deprecated)



# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log)

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
        mapscore

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})

find_library(opengl-lib GLESv3)
target_link_libraries(mapscore ${opengl-lib})
