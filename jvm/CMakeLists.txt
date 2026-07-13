cmake_minimum_required(VERSION 3.24.0)
project(mapscore_jvm
  LANGUAGES CXX
)

include(../cxx.cmake)
####
# maps-core shared C++ core with OpenGL
####
find_package(OpenGL)

include(../maps-core.cmake)
add_library_mapscore(mapscore STATIC WITH_GL)
target_compile_options(mapscore PRIVATE -fPIC)
target_link_libraries(mapscore ${OPENGL_LIBRARIES})

####
# mapscore-jni
####
find_package(JNI
  OPTIONAL_COMPONENTS AWT JVM)

add_library_mapscore_jni(mapscore_jni STATIC)
target_compile_options(mapscore_jni PRIVATE -fPIC)
target_include_directories(mapscore_jni SYSTEM PRIVATE ${JNI_INCLUDE_DIRS})
target_link_libraries(mapscore_jni PUBLIC mapscore)

####
# mapscore JVM with OSMesa
####
find_library(OSMESA_LIBRARIES 
  NAMES OSMesa 
  REQUIRED)
find_path(OSMESA_INCLUDE_DIRS 
  NAMES "GL/osmesa.h" 
  REQUIRED)

file(GLOB_RECURSE mapscore_jni_osmesa_SRC CONFIGURE_DEPENDS
  "src/main/cpp/jni/*.cpp"
)
add_library(mapscore_jni_osmesa SHARED ${mapscore_jni_osmesa_SRC})
target_include_directories(mapscore_jni_osmesa SYSTEM PRIVATE
  ${JNI_INCLUDE_DIRS}
  ${OSMESA_INCLUDE_DIRS}
)
openmobilemaps__target_default_compile_options(mapscore_jni_osmesa)
target_compile_options(mapscore_jni_osmesa BEFORE PRIVATE -Wall)
target_link_libraries(mapscore_jni_osmesa PRIVATE
  ${OSMESA_LIBRARIES}
  $<LINK_LIBRARY:WHOLE_ARCHIVE,mapscore_jni>
)

# Fiddle with cmake system  TARGET_OS/TARGET_ARCH to form string "linux_amd64" for library filename.
string(TOLOWER ${CMAKE_SYSTEM_NAME} TARGET_OS)
string(REPLACE "x86_64" "amd64" TARGET_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
set_target_properties(mapscore_jni_osmesa PROPERTIES 
  LIBRARY_OUTPUT_NAME_RELEASE mapscore_jni_${TARGET_OS}_${TARGET_ARCHITECTURE}
  LIBRARY_OUTPUT_NAME_RELWITHDEBINFO mapscore_jni_${TARGET_OS}_${TARGET_ARCHITECTURE}
  LIBRARY_OUTPUT_NAME_DEBUG mapscore_jni_${TARGET_OS}_${TARGET_ARCHITECTURE}_debug
)
