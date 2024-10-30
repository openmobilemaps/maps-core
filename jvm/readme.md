# Java bindings for off-screen use of openmobilemaps

This directory contains a java library to use the OpenGL-variant of the
openmobilemaps in a non-android Java environment.
For now, this only explicitly supports offscreen rendering via
[OSMesa](https://docs.mesa3d.org/osmesa.html).
Currently, this can be built and run only on Linux on amd64. Supporting other
platforms should be doable but will require further tweaks.

## Dependencies

### Build

Build tools:
- maven
- JDK 21
- cmake, make/ninja
- C++ compiler

Libraries:
- OpenGL ES
- OSMesa

On current Debian derivatives, this can be obtained by installing the following packages:
```
apt install --no-install-recommends maven cmake make clang libgl-dev libgles-dev libosmesa6-dev openjdk-21-jdk-headless
```

### Runtime

The java library has the following runtime dependencies:
- OpenGL ES
- OSMesa

On current Debian derivatives, this can be obtained by installing the following packages:
```
apt install --no-install-recommends libosmesa6 libgl1 && \ 
```


## How to build

Make sure you have all submodules initialized and updated. To do this, use

```
git submodule init
git submodule update
```


To build the Java library (and run the simple tests)
```
mvn clean install
```
