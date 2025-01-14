####
# mapscore-jni
####
find_package(JNI
  OPTIONAL_COMPONENTS AWT JVM)
find_library(OSMESA_LIBRARIES 
  NAMES OSMesa 
  REQUIRED)
find_path(OSMESA_INCLUDE_DIRS 
  NAMES "GL/osmesa.h" 
  REQUIRED)

file(GLOB_RECURSE mapscore_jni_SRC
  "../android/src/main/cpp/scheduling/*.cpp"
  "../external/djinni/support-lib/cpp/*.cpp"
  "../external/djinni/support-lib/jni/*.cpp"
  "../bridging/android/jni/*.cpp"
  "../jvm/src/main/cpp/jni/*.cpp"
)

add_library(mapscore_jni SHARED ${mapscore_jni_SRC})

target_include_directories(mapscore_jni PRIVATE
  ../external/djinni/support-lib/jni
  ${JNI_INCLUDE_DIRS}
  
  ../bridging/android/jni/graphics
  ../bridging/android/jni/graphics/common
  ../bridging/android/jni/graphics/objects
  ../bridging/android/jni/graphics/shader
  ../bridging/android/jni/map
  ../bridging/android/jni/map/controls
  ../bridging/android/jni/map/layers
  ../bridging/android/jni/map/layers/objects
  ../bridging/android/jni/map/layers/tiled
  ../bridging/android/jni/map/layers/tiled/raster
  ../bridging/android/jni/map/layers/tiled/vector
  ../bridging/android/jni/map/layers/polygon
  ../bridging/android/jni/map/layers/icon
  ../bridging/android/jni/map/layers/line
  ../bridging/android/jni/map/layers/text
  ../bridging/android/jni/map/loader
  ../bridging/android/jni/map/scheduling
  ../bridging/android/jni/map/coordinates
  ../bridging/android/jni/map/camera

  ${OSMESA_INCLUDE_DIRS}
)
target_compile_features(mapscore_jni PRIVATE cxx_std_20)
target_compile_definitions(mapscore_jni PRIVATE DATAREF_JNI=1) # see external/djinni/support-lib/cpp/DataRef.hpp
target_compile_options(mapscore_jni PRIVATE -Werror -Wno-deprecated -Wno-reorder)
target_link_libraries(mapscore_jni mapscore ${OSMESA_LIBRARIES})
# Fiddle with cmake system  TARGET_OS/TARGET_ARCH to form string "linux_amd64" for library filename.
string(TOLOWER ${CMAKE_SYSTEM_NAME} TARGET_OS)
string(REPLACE "x86_64" "amd64" TARGET_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
set_target_properties(mapscore_jni PROPERTIES 
  LIBRARY_OUTPUT_NAME_RELEASE mapscore_jni_${TARGET_OS}_${TARGET_ARCHITECTURE}
  LIBRARY_OUTPUT_NAME_RELWITHDEBINFO mapscore_jni_${TARGET_OS}_${TARGET_ARCHITECTURE}
  LIBRARY_OUTPUT_NAME_DEBUG mapscore_jni_${TARGET_OS}_${TARGET_ARCHITECTURE}_debug
)
