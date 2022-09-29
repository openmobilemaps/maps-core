<h1 align="center">Open Mobile Maps - Gps Layer</h1>
<br />
<div align="center">
  <img width="200" height="45" src="logo.svg" />
  <br />
  <br />
  A Gps Layer Implementation for Open Mobile Maps.
  <br />
  <br />
  <a href="https://openmobilemaps.io/">openmobilemaps.io</a>
</div>
<br />

<div align="center">
    <!-- License -->
    <a href="https://github.com/openmobilemaps/layer-gps/blob/master/LICENSE">
      <img alt="License: MPL 2.0"
      src="https://img.shields.io/badge/License-MPL%202.0-brightgreen.svg">
    </a>
</div>


<h1>iOS</h1>

## Installation

Open Mobile Maps is available through [Swift Package Manager](https://swift.org/package-manager/).

For App integration within Xcode, add this package to your App target. To do this, follow the step by step tutorial [Adding Package Dependencies to Your App](https://developer.apple.com/documentation/xcode/adding_package_dependencies_to_your_app) and add the package with the url:
```
https://github.com/openmobilemaps/layer-gps.git
```

### Swift Package

Once you have your Swift package set up, adding Open Mobile Maps as a dependency is as easy as adding it to the dependencies value of your Package.swift.

```swift
dependencies: [
    .package(url: "https://github.com/openmobilemaps/layer-gps.git", .upToNextMajor(from: "1.0.1"))
]
```

### Add the GPS Layer

The gps layer can be created with:

```swift
let gpsLayer = MCGpsLayer(style: .defaultStyle)

// Add the new layer to the MapView
mapView.add(layer: gpsLayer.asLayerInterface())

// bind the location manager to the gpsLayer
UBLocationManager.shared.bindTo(layer: layerGps, canAskForPermission: true)
```

A default style is already included in the gps layer but a custom one can be provided by creating a `MCGpsStyleInfo` instance.

```swift
public extension MCGpsStyleInfo {
    static var defaultStyle: MCGpsStyleInfo {
        guard let pointImage = UIImage(named: "ic_gps_point", in: .module, compatibleWith: nil),
              let pointCgImage = pointImage.cgImage,
              let pointTexture = try? TextureHolder(pointCgImage),
              let headingImage = UIImage(named: "ic_gps_direction", in: .module, compatibleWith: nil),
              let headingCgImage = headingImage.cgImage,
              let headingTexture = try? TextureHolder(headingCgImage) else {
                  fatalError("gps style assets not found")
              }

        return MCGpsStyleInfo(pointTexture: pointTexture,
                       headingTexture: headingTexture,
                              accuracyColor:  UIColor(red: 112/255,
                                                      green: 173/255,
                                                      blue: 204/255,
                                                      alpha: 0.2).mapCoreColor)
    }
}
```


### Gps Modes

The behavior of the gps layer depends on the selected mode. It can be adjusted by calling:
```swift
gpsLayer.setMode(GpsMode)
```

The available modes are:

**DISABLED**: No gps indicator is rendered on the map.

**STANDARD**: The indicator is drawn  at the current location, along with the current heading (if enabled and a texture is provided)

**FOLLOW**: In addition to the same behavior as `STANDARD`, upon a location update, the camera is animated to keep the indicators position centered in the map.

**FOLLOW_AND_TURN**: While following the indicators location updates (as in `FOLLOW`), the camera is rotated to have the map oriented in the direction of the current heading.


Listening to and rendering the devices current orientation can be enabled or disabled by calling:
```swift
gpsLayer.setHeadingEnabled(enabled)
```

### Location Updates

For the gps layer to work to two folowing keys have to be defined in the info.plist:

**NSLocationUsageDescription**

**NSLocationAlwaysAndWhenInUseUsageDescription**

When initializing the gps layer the gps permission promt will automatically be displayed.

