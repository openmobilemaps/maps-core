include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        914aeecfe23b1e16af6ea675a4fb5dbd5a5b8d0a # v3.8.0
  FIND_PACKAGE_ARGS  # try find_package first, i.e. use system package if available
)
FetchContent_MakeAvailable(Catch2)

add_executable(tests 
  "TestActor.cpp"
  "TestGeoJsonParser.cpp"
  "TestSymbolAnimationCoordinatorMap.cpp"
  "TestTileSource.cpp"
  "TestGeometryHandler.cpp"
  "TestStyleParser.cpp"
  "TestValueEvaluate.cpp"
  "TestVectorSet.cpp"
  "TestStyleParser.cpp"
  "TestInternedString.cpp"
  "helper/TestData.cpp"
  "helper/TestLocalDataProvider.h"
)
# Use mapscore _private_ include to allow testing functionality declared in internal headers.
# Use regex to reconstruct -I vs -isystem for external libs. Not nice, but cant find other cmake way.
target_include_directories(tests SYSTEM PRIVATE
  "$<FILTER:$<TARGET_PROPERTY:mapscore,INCLUDE_DIRECTORIES>,INCLUDE,../external>"
)
target_include_directories(tests PRIVATE
  "$<FILTER:$<TARGET_PROPERTY:mapscore,INCLUDE_DIRECTORIES>,EXCLUDE,../external>"
)
cmake_path(SET TESTDATA_DIR "data/")
cmake_path(ABSOLUTE_PATH TESTDATA_DIR NORMALIZE)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain mapscore)
target_compile_definitions(tests PRIVATE 
  OPENMOBILEMAPS_GL=1
  OPENMOBILEMAPS_TESTDATA_DIR=${TESTDATA_DIR}
)
target_compile_features(tests PRIVATE cxx_std_20)
target_compile_options(tests PRIVATE -Werror -Wunused -Wno-deprecated-declarations)

# Test via CTest 
enable_testing()
include(Catch)
catch_discover_tests(tests
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
