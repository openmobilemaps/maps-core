####
# mapscore-standalone
####
find_library(OSMESA_LIBRARIES 
  NAMES OSMesa 
  REQUIRED)
find_path(OSMESA_INCLUDE_DIRS 
  NAMES "GL/osmesa.h" 
  REQUIRED)

add_executable(testmain main.cpp)
target_compile_features(testmain PRIVATE cxx_std_20)
target_compile_options(testmain PRIVATE -Wall -Werror)
target_include_directories(testmain PRIVATE ${OSMESA_INCLUDE_DIRS})
target_link_libraries(testmain mapscore ${OSMESA_LIBRARIES})
