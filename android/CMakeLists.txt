# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

file(GLOB_RECURSE cpp_SRC
        "../bridging/android/jni/*.h"
        "../bridging/android/jni/*.hpp"
        "../bridging/android/jni/*.cpp"
        "../shared/*.h"
        "../shared/*.cpp"
        "../shared/*.hpp"
        "src/main/cpp/*.h"
        "src/main/cpp/*.cpp"
        "../external/djinni/support-lib/*.h"
        "../external/djinni/support-lib/*.hpp"
        "../external/djinni/support-lib/*.cpp"
        )

add_library( # Sets the name of the library.
        mapscore

        # Sets the library as a shared library.
        SHARED

        ${cpp_SRC})

include_directories(../external/djinni/support-lib)
include_directories(../external/djinni/support-lib/jni)
include_directories(../bridging/android/jni/graphics)
include_directories(../bridging/android/jni/graphics/common)
include_directories(../bridging/android/jni/graphics/objects)
include_directories(../bridging/android/jni/graphics/shader)
include_directories(../bridging/android/jni/map)
include_directories(../bridging/android/jni/map/controls)
include_directories(../bridging/android/jni/map/layers)
include_directories(../bridging/android/jni/map/layers/objects)
include_directories(../bridging/android/jni/map/layers/tiled)
include_directories(../bridging/android/jni/map/layers/tiled/raster)
include_directories(../bridging/android/jni/map/layers/polygon)
include_directories(../bridging/android/jni/map/layers/icon)
include_directories(../bridging/android/jni/map/loader)
include_directories(../bridging/android/jni/map/scheduling)
include_directories(../bridging/android/jni/map/coordinates)
include_directories(../bridging/android/jni/map/camera)
include_directories(../shared/public)
include_directories(../shared/src)
include_directories(../shared/src/external/pugixml)
include_directories(../shared/src/logger)
include_directories(../shared/src/graphics)
include_directories(../shared/src/graphics/helpers)
include_directories(../shared/src/map)
include_directories(../shared/src/map/camera)
include_directories(../shared/src/map/controls)
include_directories(../shared/src/map/coordinates)
include_directories(../shared/src/map/layers)
include_directories(../shared/src/map/layers/objects)
include_directories(../shared/src/map/layers/tiled)
include_directories(../shared/src/map/layers/tiled/raster)
include_directories(../shared/src/map/layers/tiled/wmts)
include_directories(../shared/src/map/layers/polygon)
include_directories(../shared/src/map/layers/icon)
include_directories(../shared/src/map/scheduling)
include_directories(src/main/cpp)
include_directories(src/main/cpp/graphics)
include_directories(src/main/cpp/graphics/objects)
include_directories(src/main/cpp/graphics/shader)


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

find_library(opengl-lib
        GLESv2)

target_link_libraries(mapscore
        ${opengl-lib})
