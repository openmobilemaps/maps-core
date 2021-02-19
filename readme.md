<h1 align="center">Open Mobile Maps</h1>
<br />
<div align="center">
  <img width="200" height="45" src="logo.svg" />
  <br />
  <br />
  The lightweight and modern Map SDK for Android (6.0+) and iOS (10+)
  <br />
  <br />
  <a href="https://openmobilemaps.io/">openmobilemaps.io</a>
</div>
<br />

<div align="center">
    <!-- SPM -->
    <a href="https://github.com/apple/swift-package-manager">
      <img alt="Swift Package Manager"
      src="https://img.shields.io/badge/SPM-%E2%9C%93-brightgreen.svg?style=flat">
    </a>
    <!-- License -->
    <a href="https://github.com/openmobilemaps/maps-core/blob/master/LICENSE">
      <img alt="License: MPL 2.0"
      src="https://img.shields.io/badge/License-MPL%202.0-brightgreen.svg">
    </a>
</div>

## Getting started

[Readme Android](./android/)

[Readme iOS](./ios/)

## Features
* Multi-platform graphics engine based on OpenGL for Android and Metal for iOS
* Touch and gesture handling
* Tiled Map Layers
* Polygon Layers

...and more to come! See our roadmap at [openmobilemaps.io](https://openmobilemaps.io)

## Architecture

To support both the Android and iOS, most of the code-base is written in C++ and shared between the two platforms. The Kotlin and swift interface bindings are generated with a fork of the [Djinni library](https://github.com/UbiqueInnovation/djinni). The Maps library intentionally is designed to have a modular structure, so that most parts of it can be adjusted or completely replaced with custom implementations. Most of the interfaces are also exposed to Swift and Kotlin, so extensions can also conveniently be programmed in those languages.

The internal structure of the project is split into two main modules: the graphics-core and the maps-core. While the first implements a generic structure for rendering basic graphic primitives on Android and iOS, the latter is a collection of classes that provide the basics for creating a digital map.

### Graphics Core

(Diagram Grapics Part)

The rendering concept in the graphics core is built around generic graphics primitives to achieve a versatile base structure. While the basic logic for organizing the primitives is shared, the implementations of those graphics object interfaces are platform-specific. For Android, the OpenGl code is kept in C++, while the Metal-implementations for iOS are written in Swift. The SceneInterface is the shared, central collection of the necessary interfaces for both creating and rendering such graphics objects. 

### Maps Core

(Diagram MapsCore Part)

Building on top of the graphics core, the maps core wraps the generic render code into a more accessible collection of higher-level classes with the functionality to build and display a digital map.  

#### MapView

On both platforms, the MapView builds the central ui element that can directly be added to the app and be interacted with. It handles the interactions between each platform and the shared library, as well as setting up relevant platform-specific components and exposing the necessary methods for manipulating all elements of the map.

#### MapScene

The MapScene, implementing the MapInterface, holds the necessary resources for interacting with the underlying render system and creating the graphics primitives, but also builds the main access point for the other components that are relevant for a functioning map. Such as the map configuration file, the camera, the touch handler and other. It also holds the collection of all layers in the scene and provides the necessary methods to adjust it.

#### Map Configuration & Coordinates

When setting up the Map, a coordinate system for the map needs to be specified. In the current implementation, the system is assumed to be a uniform, two-dimensional grid. 

All map positions handled within maps core are of the type Coord. Along with the three-dimensional position values, it holds a coordinate system identifier with the purpose of specifically exposing the system that the values should be interpreted in at that time. To switch between different coordinate systems, the MapScene holds a CoordinateConversionHelper. This helper uses pre-defined and additionally registered custom converters to transform coordinates between two systems, specified by their system identifier.

This library comes with the implementations for two prominent coordinate systems: EPSG:2056 (LV95/LV03+) and EPSG:3857 (Pseudo-Mercator). They can be created using the CoordinateSystemFactory.

#### Camera



#### Interaction

#### Layers




## License
This project is licensed under the terms of the MPL 2 license. See the [LICENSE](LICENSE) file.
