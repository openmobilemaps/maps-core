# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

find_program(CCACHE_FOUND ccache)
if (CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif (CCACHE_FOUND)

file(GLOB_RECURSE cpp_SRC
        "../bridging/android/jni/*.h"
        "../bridging/android/jni/*.hpp"
        "../bridging/android/jni/*.cpp"
        "../shared/src/*.h"
        "../shared/src/*.cpp"
        "../shared/src/*.hpp"
        "src/main/cpp/*.h"
        "src/main/cpp/*.cpp"
        "../external/djinni/support-lib/*.h"
        "../external/djinni/support-lib/*.hpp"
        "../external/djinni/support-lib/*.cpp"
        )

add_library( # Sets the name of the library.
        layergps

        # Sets the library as a shared library.
        SHARED

        ${cpp_SRC})

include_directories(../external/djinni/support-lib)
include_directories(../external/djinni/support-lib/jni)
include_directories(../bridging/android/jni)
include_directories(../bridging/android/jni/gps)
include_directories(../shared/src)
include_directories(../shared/src/gps)
include_directories(../shared/public)
include_directories(src/main/cpp)

find_package(mapscore REQUIRED CONFIG)

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
        layergps

        # Links the target library to the log library
        # included in the NDK.
        ${log-lib})

find_library(opengl-lib
        GLESv2)

target_link_libraries(layergps
        ${opengl-lib})

target_link_libraries(layergps mapscore::mapscore)