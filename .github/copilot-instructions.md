# OpenMobile Maps - GitHub Copilot Instructions

**ALWAYS follow these instructions first** before attempting any development work. Only search for additional information or use fallback methods if the information provided here is incomplete, outdated, or found to be incorrect.

OpenMobile Maps is a cross-platform maps SDK for Android (8.0+), iOS (14+), and JVM. It uses a C++ core with platform-specific bindings generated via Djinni. The architecture enables sharing most of the codebase between platforms while providing native interfaces.

## Essential Setup and Build Process

### Prerequisites and Dependencies
- **C++ Development**: cmake (3.24+), ninja-build, clang++ or gcc
- **Java Development**: JDK 21 (required for JVM bindings, NOT Java 17)
- **System Libraries**: `apt install cmake ninja-build clang libgl-dev libgles-dev libosmesa6-dev`
- **Android**: Android SDK/NDK 27.2.12479018, JDK 17+
- **iOS**: Xcode 16.4+, macOS 15+

### Critical First Steps
```bash
# ALWAYS run these steps first - they are essential
git submodule init
git submodule update  # Takes ~3 seconds
```

### Core C++ Library - Primary Build Target
```bash
# Configure build (takes ~12 seconds)
cmake -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -DBUILD_JVM=OFF -DBUILD_STANDALONE=OFF -B build-debug

# Build core library and tests (takes ~3-4 minutes) 
# NEVER CANCEL: Set timeout to 15+ minutes
cmake --build build-debug -- tests

# Run tests (takes ~6 seconds - very fast)
# NEVER CANCEL: Set timeout to 2+ minutes  
build-debug/shared/test/tests --benchmark-warmup-time 0 --benchmark-samples 1 --benchmark-no-analysis -r console::colour-mode=ansi
```

### JVM Bindings (Linux Only)
**CRITICAL**: Requires Java 21 - will fail with Java 17 or earlier.
```bash
# Configure with JVM support (takes ~5 seconds)
cmake -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -DBUILD_JVM=ON -DBUILD_STANDALONE=ON -B build-jvm

# Build JNI library (takes ~5 minutes)
# NEVER CANCEL: Set timeout to 20+ minutes
cmake --build build-jvm -- mapscore_jni

# Build standalone test app (takes ~2 seconds)
cmake --build build-jvm -- testmain

# Test standalone app works
build-jvm/standalone/testmain  # Should output coordinate bounds
```

### Platform Builds

#### Android Library
**Requirements**: JDK 17, Android SDK/NDK 27.2.12479018
```bash
cd android
# NEVER CANCEL: Takes ~10-15 minutes. Set timeout to 30+ minutes.
./gradlew assembleDebug -P'android.injected.build.abi=arm64-v8a'
# Output: build/outputs/aar/ contains the .aar library
```

#### iOS Library  
**Requirements**: Xcode 16.4+, macOS 15+
```bash
# NEVER CANCEL: Takes ~5-10 minutes. Set timeout to 20+ minutes.
xcodebuild -scheme MapCore -destination 'platform=iOS Simulator,OS=18.6,name=iPhone 16 Pro'
```

## Djinni Bindings (Cross-Platform Interface Generation)

**WARNING**: Djinni binding generation requires reliable internet connectivity and can fail with network issues.

```bash
cd djinni
# NEVER CANCEL: May take several minutes depending on network. Set timeout to 10+ minutes.
make djinni
# If this fails due to network issues, the existing generated bindings in bridging/ are usually sufficient
```

**NEVER** run `make clean djinni` unless you're certain you can regenerate the bindings, as it will delete all existing generated interface files.

## Key Project Structure

### Core Locations
- **C++ Core**: `shared/src/` and `shared/public/` - Main implementation
- **Generated Bindings**: `bridging/android/` (JNI/Kotlin), `bridging/ios/` (Objective-C++)
- **Platform Code**: `android/`, `ios/`, `jvm/`, `standalone/`
- **Tests**: `shared/test/` - Catch2 test framework
- **External Dependencies**: `external/` - Submodules (djinni, earcut, protozero, vtzero)

### Build Artifacts
- **Core Library**: `libmapscore.a`
- **JNI Library**: `libmapscore_jni_linux_amd64_debug.so` 
- **Android AAR**: `android/build/outputs/aar/`
- **Test Executable**: `build-debug/shared/test/tests`

## Validation and Testing

### Always Test These Scenarios After Changes
1. **Core C++ Tests**: Run the full test suite - should complete in ~6 seconds
2. **Standalone App**: Verify `build-jvm/standalone/testmain` runs and outputs coordinate bounds
3. **Build Integration**: Ensure all target builds complete without errors

### Pre-Commit Validation
```bash
# ALWAYS run this before committing C++ changes
build-debug/shared/test/tests
# All tests must pass - there should be 46 test cases with 1282+ assertions
```

## Build Timing Expectations

**CRITICAL**: These are validated timing measurements. **NEVER CANCEL** builds before these times:

- **Submodule Update**: ~3 seconds
- **CMake Configuration**: ~12 seconds  
- **Core C++ Build**: 3-4 minutes (set timeout to 15+ minutes)
- **C++ Tests**: ~6 seconds (set timeout to 2+ minutes)
- **JNI Build**: ~5 minutes (set timeout to 20+ minutes)
- **Standalone Build**: ~2 seconds
- **Android Build**: 10-15 minutes (set timeout to 30+ minutes)
- **iOS Build**: 5-10 minutes (set timeout to 20+ minutes)
- **Djinni Generation**: 2-10 minutes depending on network (set timeout to 10+ minutes)

## Common Issues and Solutions

### Build Failures
- **Java Version**: JVM builds require Java 21, Android builds work with Java 17
- **Missing Dependencies**: Install system packages: `libgl-dev libgles-dev libosmesa6-dev`
- **Network Issues**: Djinni generation may fail; existing bindings usually sufficient
- **Submodules**: Always run `git submodule update` after fresh clone

### Platform Limitations
- **JVM Builds**: Linux only, require OSMesa for off-screen rendering
- **Android**: Requires Android SDK/NDK, cross-compilation setup
- **iOS**: Requires macOS with Xcode, Swift Package Manager integration

## Architecture Overview

The codebase is split into:
1. **Graphics Core**: Generic graphics primitives (OpenGL/Metal abstraction)
2. **Maps Core**: Higher-level map functionality built on graphics core
3. **Platform Bindings**: Native interfaces for Android/iOS/JVM
4. **Cross-Platform**: C++ implementation shared between all targets

Key interfaces are in `shared/public/` with implementations in `shared/src/`. Platform-specific optimizations are in respective platform directories.

## Development Workflow

1. **Make C++ changes** in `shared/src/` or `shared/public/`
2. **Build and test** core library: `cmake --build build-debug -- tests && build-debug/shared/test/tests`
3. **Regenerate bindings** if interfaces changed: `cd djinni && make djinni`
4. **Test platform builds** as needed for your changes
5. **Validate** that the standalone test app still works

**Always prioritize the core C++ build and tests** - they provide the fastest validation cycle for most changes.