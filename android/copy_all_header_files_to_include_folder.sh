mkdir -p .cpp_includes
rm -rf .cpp_includes/*
find src/main/cpp/ -type f -name '*.h' -exec cp {} .cpp_includes/ \;
find ../bridging/android/jni -type f -name '*.h' -exec cp {} .cpp_includes/ \;
find ../shared/public -type f -name '*.h' -exec cp {} .cpp_includes/ \;
find ../shared/src -type f -name '*.h' -exec cp {} .cpp_includes/ \;
