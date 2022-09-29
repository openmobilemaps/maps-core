<h1 align="center">Open Mobile Maps - Gps Layer</h1>
<br />
<div align="center">
  <img width="200" height="45" src="../logo.svg" />
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
      <img alt="License: MPL 2.0" src="https://img.shields.io/badge/License-MPL%202.0-brightgreen.svg">
    </a>
    <a href="https://search.maven.org/search?q=g:%22io.openmobilemaps%22%20AND%20a:%22layer-gps%22">
      <img alt="Maven Central" src="https://img.shields.io/maven-central/v/io.openmobilemaps/layer-gps.svg?label=Maven%20Central">
    </a>
    <a href="https://search.maven.org/search?q=g:%22io.openmobilemaps%22%20AND%20a:%22layer-gps-dev%22">
      <img alt="Maven Central Dev" src="https://img.shields.io/maven-central/v/io.openmobilemaps/layer-gps-dev.svg?label=Maven%20Central">
    </a>
</div>


<h1>Android</h1>

## How to use

This module is designed to be used together with Open Mobile Maps maps-core.

### Adding the dependency

To add the OpenSwissMaps SDK to your Android project, add the following line to your build.gradle

```groovy
implementation 'io.openmobilemaps:layer-gps:1.0.1'
```

Make sure you have mavenCentral() listed in your project repositories. 

### Initializing the library

To use the library, it needs to be initialized as early as possible, e.g. in the oOnCreate(), of the hosting Application by calling:
```kotlin
LayerGps.initialize()
```

### Adding the GPS Layer

The gps layer can be created with:

```kotlin
val gpsStyleInfo = GpsStyleInfoFactory.createDefaultStyle(context)
val gpsProviderType = GpsProviderType.GPS_ONLY
val gpsLayer = GpsLayer(context, gpsStyleInfo, gpsProviderType)
gpsLayer.registerLifecycle(lifecycle)

// Add the new layer to the MapView
mapView.addLayer(gpsLayer.asLayerInterface())
```

The code above uses the included default style. A custom style for the gps indicator can easily be created with the `GpsStyleInfoFactory`.
Two images (`ic_gps_point` and `ic_gps_direction`) are included in the module as an example for the two drawables needed for the `GpsStyleInfo`.

```kotlin
val gpsStyleInfo = GpsStyleInfoFactory.createGpsStyle(
    gpsPointIdicator, // Drawable or Bitmap. Rendered at the currently provided location
    gpsDirectionIndicator, // Drawable or Bitmap. Rendered below the point indicator, indicating the device orientation
    Color(1.0f, 0f, 0f, 0.2f) // Color used to render the accuracy range of the currently provided location
)
```

### Gps Modes

The behavior of the gps layer depends on the selected mode. It can be adjusted by calling:
```kotlin
gpsLayer.setMode(GpsMode)
```

The available modes are:

**DISABLED**: No gps indicator is rendered on the map.

**STANDARD**: The indicator is drawn  at the current location, along with the current heading (if enabled and a texture is provided)

**FOLLOW**: In addition to the same behavior as `STANDARD`, upon a location update, the camera is animated to keep the indicators position centered in the map.

**FOLLOW_AND_TURN**: While following the indicators location updates (as in `FOLLOW`), the camera is rotated to have the map oriented in the direction of the current heading.

Listening to and rendering the devices current orientation can be enabled or disabled by calling:
```kotlin
gpsLayer.setHeadingEnabled(enabled)
```

### Location Updates

Location Updates are passed to the gps layer either by calling the appropriate functions manually or using a location provider.

```kotlin
gpsLayer.updatePosition(Coord(...))
gpsLayer.updateHeading(orientationInDegrees)
```

Two types of predefined location providers are supplied with this library:

**GPS_ONLY**: A provider that registers to location updates of the devices gps module only.

**GOOGLE_FUSED**: Uses the [FusedLocationProviderClient](https://developers.google.com/android/reference/com/google/android/gms/location/FusedLocationProviderClient.html) of the Google Play Services

**Be aware that you must handle the location permissions of the app yourself!**

When creating the gps layer, you can either conveniently pass the Type of provider (as in the example above) into the constructor or provide a custom implementation of the `LocationProviderInterface`.

```kotlin
val gpsLayer = GpsLayer(context, gpsStyleInfo, GpsProviderType.GOOGLE_FUSED) // use a supplied provider
val gpsLayer = GpsLayer(context, gpsStyleInfo, CustomLocationProvider()) // use a custom implementation of the LocationProviderInterface
```

