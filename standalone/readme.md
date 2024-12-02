# Standalone openmobilemaps test application

This directory contains a minimal test application that exercises 
OpenGL offscreen rendering with the openmobilemaps library,
that can be used for diagnostics and debugging directly on the developer system.
It also serves as an example to demonstrate how the openmobilemaps library can
be used directly from C++, without the platform specific bindings.

Currently, this can be built and run only on Linux on amd64. Supporting other
platforms should be doable but will require further tweaks in the library.

## Dependencies

### Build

Build tools:
- cmake, make/ninja
- C++ compiler

Libraries:
- OpenGL ES
- OSMesa

On current Debian derivatives, this can be obtained by installing the following packages:
```
apt install --no-install-recommends cmake make clang libgl-dev libgles-dev libosmesa6-dev
```

## How to build

```
cmake -G build-directory
cmake --build build-directory -- testmain

# run
build-directory/standalone/testmain
```
