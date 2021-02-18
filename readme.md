<h1 align="center">Open Mobile Maps</h1>
<br />
<div align="center">
  <img width="200" height="45" src="logo.svg" />
  <br />
  The lightweight and modern Map SDK for Android (6.0+) and iOS (10+)
</div>
<br />
<div align="center">
    <!-- SPM -->
    <a href="https://github.com/apple/swift-package-manager">
      <img alt="Swift Package Manager"
      src="https://img.shields.io/badge/SPM-%E2%9C%93-brightgreen.svg?style=flat">
    </a>
    <!-- License -->
    <a href="https://github.com/UbiqueInnovation/notifyme-app-ios/blob/master/LICENSE">
      <img alt="License: MPL 2.0"
      src="https://img.shields.io/badge/License-MPL%202.0-brightgreen.svg">
    </a>
</div>

### Getting started

[Readme Android](./android/)

[Readme iOS](./ios/)

### Features
* Multi-platform graphics engine based on OpenGL for Android and Metal for iOS
* ...

## Architecture

To support both the Android and iOS, most of the code-base is written in C++ and shared between the two platforms. The Kotlin and swift interface bindings are generated with a fork of the [Djinni library](UBIQUE_DJINNI) (TODO: link zu Ubique Repo mit Djinni). The Maps library intenionally is designed to have a modular structure, so that most parts of it can be adjusted or completely replaced with custom implementations. Most of the interfaces are also exposed to Swift and Kotlin, so extensions can also conveniently be programmed in those languages.

The internal structure of the project is split into two main modules: the graphics-core and the maps-core. While the first implements a generic structure for rendering basic graphic primitives on Android and iOS, the latter is a collection of classes that provide the basics for creating a digital map.

### Graphics Core

(Diagram Grapics Part)

The rendering concept in the graphics core is built around generic graphics primitives to achieve a versatile base structure. While the basic logic for organizing the primitives is shared, the implementations of those graphics object interfaces are platform-specific. For Android, the OpenGl code is kept in C++, while the Metal-implementations for iOS are written in Swift. The SceneInterface is the shared, central collection of the necessary interfaces for both creating and rendering such graphics objects. 

### Maps Core

(Diagram MapsCore Part)

#### MapInterface

#### Coordinates

#### Layers

#### Camera

## Contribution
