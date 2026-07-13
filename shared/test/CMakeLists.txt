cmake_minimum_required(VERSION 3.24.0)
project(mapscore_shared_test
  LANGUAGES CXX
)

include(../../cxx.cmake)
include(../../catch2.cmake)

###
# mapscore library built specifically for testing
###
include(../../maps-core.cmake)
add_library_mapscore(mapscore STATIC WITHOUT_GL)
target_compile_definitions(mapscore PRIVATE CATCH_TESTING=1)

###
# mapscore tests
###
add_library(mapscore_testobjs OBJECT
  "TestActor.cpp"
  "TestGeoJsonParser.cpp"
  "TestGeometryHandler.cpp"
  "TestInternedString.cpp"
  "TestOwnedBytes.cpp"
  "TestReleasableAllocator.cpp"
  "TestStyleParser.cpp"
  "TestSymbolAnimationCoordinatorMap.cpp"
  "TestTileSource.cpp"
  "TestValueEvaluate.cpp"
  "TestVectorSet.cpp"
  "helper/TestData.cpp"
  "helper/TestLocalDataProvider.h"
)
# Use mapscore _private_ include to allow testing functionality declared in internal headers.
# Use regex to reconstruct -I vs -isystem for external libs. Not nice, but cant find other cmake way.
target_include_directories(mapscore_testobjs SYSTEM PRIVATE
  "$<FILTER:$<TARGET_PROPERTY:mapscore,INCLUDE_DIRECTORIES>,INCLUDE,../external>"
)
target_include_directories(mapscore_testobjs PRIVATE
  "$<FILTER:$<TARGET_PROPERTY:mapscore,INCLUDE_DIRECTORIES>,EXCLUDE,../external>"
)
cmake_path(SET TESTDATA_DIR "data/")
cmake_path(ABSOLUTE_PATH TESTDATA_DIR NORMALIZE)
target_compile_definitions(mapscore_testobjs PRIVATE 
  OPENMOBILEMAPS_TESTDATA_DIR=${TESTDATA_DIR}
)
openmobilemaps__target_default_compile_options(mapscore_testobjs)
target_compile_options(mapscore_testobjs BEFORE PRIVATE -Wall)
target_compile_options(mapscore_testobjs PRIVATE -Wno-deprecated-declarations)
target_link_libraries(mapscore_testobjs PUBLIC mapscore Catch2::Catch2)

###
# mapscore Catch2 executable
###
add_executable(mapscore_tests $<TARGET_OBJECTS:mapscore_testobjs>)
target_link_libraries(mapscore_tests mapscore_testobjs Catch2::Catch2WithMain)
set_target_properties(mapscore_tests PROPERTIES OUTPUT_NAME tests)
