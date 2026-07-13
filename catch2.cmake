include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        914aeecfe23b1e16af6ea675a4fb5dbd5a5b8d0a # v3.8.0
  FIND_PACKAGE_ARGS  # try find_package first, i.e. use system package if available
)
FetchContent_MakeAvailable(Catch2)
