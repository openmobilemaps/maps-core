<h1 align="center">Open Mobile Maps</h1>

<br />

<div align="center">
  <img width="200" height="45" src="../logo.svg" />
  <br />
  <br />
  The lightweight and modern Map SDK for Android (8.0+, OpenGl ES 3.2) and iOS (10+)
  <br />
  <br />
  <a href="https://openmobilemaps.io/">openmobilemaps.io</a>
</div>

<br />

<div align="center">
    <!-- License -->
    <a href="https://github.com/openmobilemaps/maps-core/blob/master/LICENSE">
      <img alt="License: MPL 2.0" src="https://img.shields.io/badge/License-MPL%202.0-brightgreen.svg">
    </a>
    <a href="https://search.maven.org/search?q=g:%22io.openmobilemaps%22%20AND%20a:%22mapscore%22">
      <img alt="Maven Central" src="https://img.shields.io/maven-central/v/io.openmobilemaps/mapscore.svg?label=Maven%20Central">
    </a>
    <a href="https://search.maven.org/search?q=g:%22io.openmobilemaps%22%20AND%20a:%22mapscore-dev%22">
      <img alt="Maven Central Dev" src="https://img.shields.io/maven-central/v/io.openmobilemaps/mapscore-dev.svg?label=Maven%20Central">
    </a>
</div>

<h1>Web / WASM</h1>

**Note:**  
Web support is currently in alpha and under active development. Features and functionality may change as improvements are made, and some features may not yet be fully functional.

## Building the WASM Binary

Before building, ensure all submodules are initialized and updated:

```bash
git submodule init
git submodule update
```

You will need Emscripten 4 to build the WASM binary.
To ensure a reproducable build environment, we build in a Docker container:

**Build** (run from the `/web` directory):
  - Configure:  
    ```bash
    docker run -ti --rm -v $(pwd):/src -u $(id -u):$(id -g) emscripten/emsdk:4.0.11 cmake --preset wasm-release
    ```
  - Build:  
    ```bash
    docker run -ti --rm -v $(pwd):/src -u $(id -u):$(id -g) emscripten/emsdk:4.0.11 cmake --build --preset wasm-release
    ```

For ARM64 systems, use the `emscripten/emsdk:4.0.11-arm64` Docker image.

## Generating the TypeScript Module

To enable TypeScript support, run the following command in the `web` directory:

```bash
node generate-module.js --src=./bridging/ts --import-prefix=@djinni/maps-core --out=build-wasm-release/webmapsdk.d.ts
```

This generates a TypeScript declaration file for the entry point.

**Note:**  
To ensure TypeScript recognizes the types correctly, add a path alias from `@djinni/maps-core/*` to `../maps-core/bridging/ts/*`.

## Using the WASM SDK in Your Project

1. Copy the contents of `build-wasm-release` (binary, Emscripten JS file, and TypeScript declaration file if applicable).
2. Copy the TypeScript bridging files (if using TypeScript).
3. Add the necessary TypeScript path aliases (if applicable).

**Important:**
Open Mobile Maps utilizes [Shared Array Buffers](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/SharedArrayBuffer).
For this to work, your web application must be hosted with the following headers:

```
Cross-Origin-Opener-Policy: same-origin

Cross-Origin-Embedder-Policy: require-corp
// or
Cross-Origin-Embedder-Policy: credentialless
```

**Note:**  
We plan to simplify this such that a user only needs to pull a NPM package to use Open Mobile Maps.

For usage examples, refer to the `web/demos` directory. Additional documentation will be provided in the future.
