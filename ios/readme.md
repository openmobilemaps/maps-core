<h1 align="center">Open Mobile Maps</h1>
<br />
<div align="center">
  <img width="200" height="45" src="../logo.svg" />
  <br />
  <br />
  The lightweight and modern Map SDK for Android (8.0+, OpenGL ES 3.2) and iOS (14+)
  <br />
  <br />
  <a href="https://openmobilemaps.io/">openmobilemaps.io</a>
</div>
<br />
<div align="center">
    <!-- License -->
    <a href="https://github.com/openmobilemaps/maps-core/blob/master/LICENSE">
      <img alt="License: MPL 2.0"
      src="https://img.shields.io/badge/License-MPL%202.0-brightgreen.svg">
    </a>
</div>

<h1>iOS</h1>

## Table of Contents
- [Quick Start](#quick-start)
- [System Requirements](#system-requirements)
- [Installation](#installation)
- [How to use](#how-to-use)
  - [MapView](#mapview)
  - [Vector Tiles](#vector-tiles)
  - [Camera and Gesture Handling](#camera-and-gesture-handling)
  - [Performance Considerations](#performance-considerations)
  - [Overlays](#overlays)
  - [Customisation](#customisation)
- [Troubleshooting](#troubleshooting)
- [How to build](#how-to-build)
- [Testing and Debugging](#testing-and-debugging)
- [License](#license)
- [Third-Party Software](#third-party-software)

## Quick Start

Get up and running with Open Mobile Maps in just a few steps:

1. **Add the package dependency** to your iOS project (iOS 14.0+ required)
2. **Import MapCore** in your Swift file
3. **Create a MapView** with a raster layer
4. **Display your map**

### SwiftUI (iOS 17.0+)
```swift
import MapCore
import SwiftUI

struct ContentView: View {
    @State private var camera = MapView.Camera(
        latitude: 46.962592372639634,
        longitude: 8.378232525377973,
        zoom: 1000000
    )
    
    var body: some View {
        MapView(
            camera: $camera,
            layers: [
                TiledRasterLayer("osm", webMercatorUrlFormat: "https://tile.openstreetmap.org/{z}/{x}/{y}.png")
            ]
        )
    }
}
```

### UIKit
```swift
import MapCore

class MapViewController: UIViewController {
    lazy var mapView = MCMapView()
    
    override func loadView() { 
        view = mapView 
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        mapView.add(layer: TiledRasterLayer("osm", webMercatorUrlFormat: "https://tile.openstreetmap.org/{z}/{x}/{y}.png"))
        mapView.camera.move(toCenterPositionZoom: MCCoord(lat: 46.962592372639634, lon: 8.378232525377973), zoom: 1000000, animated: true)
    }
}
```

## System Requirements

- **iOS 14.0+** (SwiftUI MapView requires iOS 17.0+)
- **Xcode 14.0+**
- **Swift 5.7+**

## Installation
Open Mobile Maps is available through [Swift Package Manager](https://swift.org/package-manager/).

### Xcode
For App integration within XCode, add this package to your App target. To do this, follow the step by step tutorial [Adding Package Dependencies to Your App](https://developer.apple.com/documentation/xcode/adding_package_dependencies_to_your_app).

### Swift Package

Once you have your Swift package set up, adding Open Mobile Maps as a dependency is as easy as adding it to the dependencies value of your Package.swift.
```swift
dependencies: [
    .package(url: "https://github.com/openmobilemaps/maps-core", from: .init(stringLiteral: "3.4.1"))
]
```


## How to use

### MapView

The framework provides a view that can be filled with layers. The simplest case is to add a raster layer. TiledRasterLayer provides a convenience initializer to create raster layer with web mercator tiles.

#### SwiftUI

For SwiftUI applications, use the `MapView` which provides a declarative interface. Note that the SwiftUI `MapView` requires iOS 17.0 or later.

```swift
import MapCore
import SwiftUI

struct ContentView: View {
    @State private var camera = MapView.Camera(
        latitude: 46.962592372639634,
        longitude: 8.378232525377973,
        zoom: 1000000
    )
    @State private var layers: [any Layer] = []
    
    var body: some View {
        MapView(
            camera: $camera,
            layers: layers
        )
        .onAppear {
            setupLayers()
        }
    }
    
    private func setupLayers() {
        layers = [
            TiledRasterLayer("osm", webMercatorUrlFormat: "https://tiles.sample.org/{z}/{x}/{y}.png")
        ]
    }
}
```

You can combine multiple layers by passing them in the `layers` array:

```swift
struct ContentView: View {
    @State private var camera = MapView.Camera(
        latitude: 46.962592372639634,
        longitude: 8.378232525377973,
        zoom: 1000000
    )
    @State private var layers: [any Layer] = []
    
    var body: some View {
        MapView(
            camera: $camera,
            layers: layers
        )
        .onAppear {
            setupLayers()
        }
    }
    
    private func setupLayers() {
        do {
            layers = [
                TiledRasterLayer("base", webMercatorUrlFormat: "https://tiles.sample.org/{z}/{x}/{y}.png"),
                try VectorLayer("overlay", styleURL: "https://www.sample.org/overlay/style.json")
            ]
        } catch {
            print("Failed to create vector layer: \(error)")
            // Fallback to raster layer only
            layers = [
                TiledRasterLayer("base", webMercatorUrlFormat: "https://tiles.sample.org/{z}/{x}/{y}.png")
            ]
        }
    }
}
```

The `camera` binding allows you to observe and control the map's camera position. The camera will update automatically as users interact with the map, and you can programmatically change the camera position by updating the binding.

For 3D maps, you can enable 3D mode:

```swift
MapView(
    camera: $camera,
    layers: layers,
    is3D: true
)
```

#### UIKit

For UIKit applications, use the `MCMapView` directly:

```swift
import MapCore

class MapViewController: UIViewController {
  lazy var mapView = MCMapView()
  override func loadView() { view = mapView }
  
  override func viewDidLoad() {
    super.viewDidLoad()
      
    mapView.add(layer: TiledRasterLayer("osm", webMercatorUrlFormat: "https://tiles.sample.org/{z}/{x}/{y}.png"))

    mapView.camera.move(toCenterPositionZoom: MCCoord(lat: 46.962592372639634, lon: 8.378232525377973), zoom: 1000000, animated: true)
  }
}
```


#### Parsing a WMTS Capability 

Open Mobile Maps supports the [WMTS standard](https://en.wikipedia.org/wiki/Web_Map_Tile_Service) and can parse their Capability XML file to generate raster layer configurations.

##### SwiftUI

```swift
struct ContentView: View {
    @State private var camera = MapView.Camera(
        latitude: 46.962592372639634,
        longitude: 8.378232525377973,
        zoom: 1000000
    )
    @State private var layers: [any Layer] = []
    
    var body: some View {
        MapView(
            camera: $camera,
            layers: layers
        )
        .onAppear {
            setupWMTSLayer()
        }
    }
    
    private func setupWMTSLayer() {
        guard let resource = MCWmtsCapabilitiesResource.create(xml),
              let wmtsLayer = resource.createLayer("identifier", tileLoader: MCTextureLoader()) else {
            print("Failed to create WMTS layer - falling back to raster layer")
            layers = [
                TiledRasterLayer("fallback", webMercatorUrlFormat: "https://tile.openstreetmap.org/{z}/{x}/{y}.png")
            ]
            return
        }
        layers = [wmtsLayer]
    }
}
```

##### UIKit

```swift
guard let resource = MCWmtsCapabilitiesResource.create(xml) else {
    print("Failed to parse WMTS capabilities")
    return
}
```
The created resource object is then capable of creating a layer object with a given identifier.

```swift
guard let layer = resource.createLayer("identifier", tileLoader: loader) else {
    print("Failed to create WMTS layer with identifier")
    return
}
mapView.add(layer: layer.asLayerInterface())
```

This feature is still being improved to support a wider range of WMTS capabilities.

For this example, we also use the default implementation of the [TextureLoader](https://github.com/openmobilemaps/maps-core/blob/develop/ios/maps/MCTextureLoader.swift), but this can also be implemented by the app itself.


### Vector Tiles

Open Mobile Maps supports most of the [Vector tiles standard](https://docs.mapbox.com/data/tilesets/guides/vector-tiles-standards/). To add a layer simply reference the style URL.

#### SwiftUI

```swift
struct ContentView: View {
    @State private var camera = MapView.Camera(
        latitude: 46.962592372639634,
        longitude: 8.378232525377973,
        zoom: 1000000
    )
    @State private var layers: [any Layer] = []
    
    var body: some View {
        MapView(
            camera: $camera,
            layers: layers
        )
        .onAppear {
            setupLayers()
        }
    }
    
    private func setupLayers() {
        do {
            layers = [
                try VectorLayer("base-map", styleURL: "https://www.sample.org/base-map/style.json")
            ]
        } catch {
            print("Failed to create vector layer: \(error)")
            // Fallback to a simple raster layer
            layers = [
                TiledRasterLayer("osm", webMercatorUrlFormat: "https://tile.openstreetmap.org/{z}/{x}/{y}.png")
            ]
        }
    }
}
```

#### UIKit

```swift
do {
    let vectorLayer = try VectorLayer("base-map", styleURL: "https://www.sample.org/base-map/style.json")
    mapView.add(layer: vectorLayer)
} catch {
    print("Failed to create vector layer: \(error)")
    // Fallback to a raster layer
    mapView.add(layer: TiledRasterLayer("osm", webMercatorUrlFormat: "https://tile.openstreetmap.org/{z}/{x}/{y}.png"))
}
```

Additional features and differences will be documented soon.

### Camera and Gesture Handling

Open Mobile Maps provides comprehensive camera control and gesture handling for both SwiftUI and UIKit.

#### Camera Control

The camera system allows you to programmatically control the map's view, including position, zoom, and in 3D mode, the viewing angle.

##### SwiftUI Camera Binding

In SwiftUI, the camera is bound to your view state, allowing for reactive updates:

```swift
struct ContentView: View {
    @State private var camera = MapView.Camera(
        latitude: 46.962592372639634,
        longitude: 8.378232525377973,
        zoom: 1000000
    )
    
    var body: some View {
        VStack {
            // Camera position updates automatically as user interacts
            Text("Lat: \(camera.center.value?.lat ?? 0, specifier: "%.6f")")
            Text("Lon: \(camera.center.value?.lon ?? 0, specifier: "%.6f")")
            Text("Zoom: \(camera.zoom.value ?? 0, specifier: "%.0f")")
            
            MapView(camera: $camera, layers: layers)
            
            // Programmatic camera control
            HStack {
                Button("Zoom In") {
                    if let currentZoom = camera.zoom.value {
                        camera = MapView.Camera(
                            center: camera.center.value,
                            zoom: currentZoom * 0.5,  // Zoom in
                            animated: true
                        )
                    }
                }
                Button("Reset") {
                    camera = MapView.Camera(
                        latitude: 46.962592372639634,
                        longitude: 8.378232525377973,
                        zoom: 1000000,
                        animated: true
                    )
                }
            }
        }
    }
}
```

##### UIKit Camera Control

For UIKit, use the `MCMapView.camera` property:

```swift
class MapViewController: UIViewController {
    lazy var mapView = MCMapView()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        // Move camera with animation
        mapView.camera.move(
            toCenterPositionZoom: MCCoord(lat: 46.962592372639634, lon: 8.378232525377973),
            zoom: 1000000,
            animated: true
        )
        
        // Set camera bounds (restrict panning area)
        let bounds = MCRectCoord(
            topLeft: MCCoord(lat: 47.8, lon: 5.9),
            bottomRight: MCCoord(lat: 45.8, lon: 10.5)
        )
        mapView.camera.setBounds(bounds, paddingLeft: 50, paddingRight: 50, paddingTop: 100, paddingBottom: 50)
        
        // Set zoom limits
        mapView.camera.setMinZoom(100000)    // Min zoom level
        mapView.camera.setMaxZoom(1000)      // Max zoom level
    }
    
    // Listen to camera changes
    private func setupCameraListener() {
        mapView.callbackHandler = MapCallbackHandler { [weak self] in
            let currentPosition = self?.mapView.camera.getCenterPosition()
            print("Camera moved to: \(currentPosition?.lat ?? 0), \(currentPosition?.lon ?? 0)")
        }
    }
}
```

#### Custom Gesture Handling

You can customize gesture behavior or add custom touch handling:

##### Custom Touch Handler (UIKit)

```swift
class CustomTouchHandler: MCMapViewTouchHandler {
    override func onTap(_ posScreen: MCVec2F, posMap: MCCoord, confirmed: Bool) {
        if confirmed {
            print("Tapped at map coordinate: \(posMap.lat), \(posMap.lon)")
            // Handle tap event
        }
    }
    
    override func onLongPress(_ posScreen: MCVec2F, posMap: MCCoord) {
        print("Long press at: \(posMap.lat), \(posMap.lon)")
        // Handle long press event
    }
    
    override func onPan(_ posScreen: MCVec2F, posMap: MCCoord, translation: MCVec2F, state: MCGestureState) {
        // Handle custom pan behavior
        if state == .BEGAN {
            print("Pan started")
        }
    }
}

// In your view controller:
mapView.touchHandler = CustomTouchHandler()
```

##### Disabling Gestures

```swift
// Disable specific gestures
mapView.setGestureEnabled(.PAN, enabled: false)
mapView.setGestureEnabled(.ZOOM, enabled: false)
mapView.setGestureEnabled(.ROTATION, enabled: false)
```



```

### Performance Considerations

To ensure optimal performance with Open Mobile Maps:

#### Layer Management
- **Limit concurrent layers**: Too many layers can impact rendering performance
- **Remove unused layers**: Call `mapView.remove(layer:)` when layers are no longer needed
- **Use appropriate tile sizes**: Standard 256x256 or 512x512 tiles work best

#### Memory Management
- **Dispose of resources**: Large textures and layers should be properly cleaned up
- **Monitor memory usage**: Use Xcode's memory debugger to track texture and layer memory usage
- **Cache management**: The SDK automatically manages tile caching, but you can customize cache size

```swift
// Configure cache size (in bytes)
MCTileLoader.setMaxCacheSize(100 * 1024 * 1024) // 100 MB cache

// Clear cache when needed
MCTileLoader.clearCache()

// Check cache status
let cacheSize = MCTileLoader.getCurrentCacheSize()
print("Current cache size: \(cacheSize) bytes")
```

#### Offline Support
- **Tile pre-caching**: Download tiles in advance for offline use
- **Local tile sources**: Serve tiles from app bundle or documents directory
- **Cache management**: Monitor storage usage and implement cache eviction policies

```swift
// Example: Pre-cache tiles for a specific area
func precacheTilesForRegion(_ bounds: MCRectCoord, maxZoom: Int) {
    let tileLoader = MCTileLoader()
    
    // Calculate tile coordinates for the bounds and zoom levels
    for zoom in 0...maxZoom {
        let tiles = calculateTilesForBounds(bounds, zoom: zoom)
        for tileCoord in tiles {
            let request = MCTileLoaderRequest(x: tileCoord.x, y: tileCoord.y, zoom: tileCoord.zoom)
            tileLoader.load(request)
        }
    }
}
```

#### Rendering Optimization
- **Minimize overdraw**: Avoid overlapping opaque layers
- **Use appropriate zoom levels**: Don't load unnecessarily high-resolution tiles
- **Batch layer updates**: Group multiple layer changes together when possible

```swift
// Good: Batch layer changes
mapView.beginLayerTransaction()
mapView.add(layer: layer1)
mapView.add(layer: layer2)
mapView.remove(layer: oldLayer)
mapView.commitLayerTransaction()
```

#### Thread Safety
- **Main thread UI updates**: Always update MapView properties on the main thread
- **Background processing**: Heavy processing should be done on background queues

```swift
DispatchQueue.global(qos: .userInitiated).async {
    // Heavy processing
    let processedData = self.processMapData()
    
    DispatchQueue.main.async {
        // Update UI on main thread
        self.mapView.add(layer: processedData)
    }
}
```

### Overlays

#### Polygon layer

Open Mobile Maps provides a simple interface to create a polygon layer. The layer handles the rendering of the given polygons and calls the callback handler in case of user interaction.

##### SwiftUI

For overlay layers like polygons, icons, and lines in SwiftUI, you'll need to manage them through the underlying `MCMapView`. Here's an example using `UIViewRepresentable`:

```swift
struct MapWithPolygonView: UIViewRepresentable {
    @Binding var camera: MapView.Camera
    let coords: [MCCoord]
    
    func makeUIView(context: Context) -> MCMapView {
        let mapView = MCMapView()
        
        // Add base layer
        mapView.add(layer: TiledRasterLayer("osm", webMercatorUrlFormat: "https://tiles.sample.org/{z}/{x}/{y}.png"))
        
        // Add polygon layer
        let polygonLayer = MCPolygonLayerInterface.create()
        let polygonInfo = MCPolygonInfo(
            identifier: "switzerland",
            coordinates: MCPolygonCoord(positions: coords, holes: []),
            color: UIColor.red.mapCoreColor,
            highlight: UIColor.red.withAlphaComponent(0.2).mapCoreColor
        )
        
        polygonLayer?.add(polygonInfo)
        mapView.add(layer: polygonLayer?.asLayerInterface())
        
        // Set initial camera position
        if let center = camera.center.value, let zoom = camera.zoom.value {
            mapView.camera.move(toCenterPositionZoom: center, zoom: zoom, animated: false)
        }
        
        return mapView
    }
    
    func updateUIView(_ uiView: MCMapView, context: Context) {
        // Handle camera updates if needed
    }
}

struct ContentView: View {
    @State private var camera = MapView.Camera(
        latitude: 46.962592372639634,
        longitude: 8.378232525377973,
        zoom: 1000000
    )
    
    var body: some View {
        MapWithPolygonView(
            camera: $camera,
            coords: [
                // your coordinates here
            ]
        )
    }
}
```

##### UIKit

``` swift
let coords : [MCCoord] = [
    /// coordinates
]
let polygonLayer = MCPolygonLayerInterface.create()
let polygonInfo = MCPolygonInfo(identifier: "switzerland",
                                coordinates: MCPolygonCoord(positions: coords, holes: []),
                                color: UIColor.red.mapCoreColor,
                                highlight: UIColor.red.withAlphaComponent(0.2).mapCoreColor)

polygonLayer?.add(polygonInfo)
polygonLayer?.setCallbackHandler(handler)
mapView.add(layer: polygonLayer?.asLayerInterface())
```

#### Icon layer

A simple icon layer is implemented as well. This supports displaying textures at the given coordinates. A scale parameter has to be provided which specifies how the icon should be affected by camera movements. In case of user interaction, the given callback handler will be called.

##### SwiftUI

```swift
struct MapWithIconView: UIViewRepresentable {
    @Binding var camera: MapView.Camera
    let coordinate: MCCoord
    let imageName: String
    
    func makeUIView(context: Context) -> MCMapView {
        let mapView = MCMapView()
        
        // Add base layer
        mapView.add(layer: TiledRasterLayer("osm", webMercatorUrlFormat: "https://tiles.sample.org/{z}/{x}/{y}.png"))
        
        // Add icon layer
        let iconLayer = MCIconLayerInterface.create()
        
        guard let image = UIImage(named: imageName),
              let cgImage = image.cgImage else {
            print("Failed to load image: \(imageName)")
            return mapView
        }
        
        do {
            let texture = try TextureHolder(cgImage)
            let icon = MCIconFactory.createIcon(
                "icon",
                coordinate: coordinate,
                texture: texture,
                iconSize: .init(x: Float(texture.getImageWidth()), y: Float(texture.getImageHeight())),
                scale: .FIXED,
                blendMode: .NORMAL
            )
            iconLayer?.add(icon)
            mapView.add(layer: iconLayer?.asLayerInterface())
        } catch {
            print("Failed to create texture: \(error)")
        }
        
        // Set initial camera position
        if let center = camera.center.value, let zoom = camera.zoom.value {
            mapView.camera.move(toCenterPositionZoom: center, zoom: zoom, animated: false)
        }
        
        return mapView
    }
    
    func updateUIView(_ uiView: MCMapView, context: Context) {
        // Handle updates if needed
    }
}

struct ContentView: View {
    @State private var camera = MapView.Camera(
        latitude: 46.962592372639634,
        longitude: 8.378232525377973,
        zoom: 1000000
    )
    
    var body: some View {
        MapWithIconView(
            camera: $camera,
            coordinate: camera.center.value ?? MCCoord(lat: 46.962592372639634, lon: 8.378232525377973),
            imageName: "your-image-name"
        )
    }
}
```

##### UIKit

```swift
let iconLayer = MCIconLayerInterface.create()

guard let image = UIImage(named: "image"),
      let cgImage = image.cgImage else {
    print("Failed to load image")
    return
}

do {
    let texture = try TextureHolder(cgImage)
    let icon = MCIconFactory.createIcon("icon",
                             coordinate: coordinate,
                             texture: texture,
                             iconSize: .init(x: Float(texture.getImageWidth()), y: Float(texture.getImageHeight())),
                             scale: .FIXED,
                             blendMode: .NORMAL)
    iconLayer?.add(icon)
    iconLayer?.setCallbackHandler(handler)
    mapView.add(layer: iconLayer?.asLayerInterface())
} catch {
    print("Failed to create texture: \(error)")
}
```

#### Line layer

A line layer can be added to the mapView as well. Using the MCLineFactory a LineInfo object can be created. The width can be specified in either SCREEN_PIXEL or MAP_UNIT.

##### SwiftUI

```swift
struct MapWithLineView: UIViewRepresentable {
    @Binding var camera: MapView.Camera
    let coords: [MCCoord]
    
    func makeUIView(context: Context) -> MCMapView {
        let mapView = MCMapView()
        
        // Add base layer
        mapView.add(layer: TiledRasterLayer("osm", webMercatorUrlFormat: "https://tiles.sample.org/{z}/{x}/{y}.png"))
        
        // Add line layer
        let lineLayer = MCLineLayerInterface.create()
        lineLayer?.add(MCLineFactory.createLine(
            "lineIdentifier",
            coordinates: coords,
            style: MCLineStyle(
                color: MCColorStateList(
                    normal: UIColor.systemPink.withAlphaComponent(0.5).mapCoreColor,
                    highlighted: UIColor.blue.withAlphaComponent(0.5).mapCoreColor
                ),
                gapColor: MCColorStateList(
                    normal: UIColor.red.withAlphaComponent(0.5).mapCoreColor,
                    highlighted: UIColor.gray.withAlphaComponent(0.5).mapCoreColor
                ),
                opacity: 1.0,
                widthType: .SCREEN_PIXEL,
                width: 50,
                dashArray: [1,1],
                lineCap: .BUTT,
                offset: 0.0
            )
        ))
        mapView.add(layer: lineLayer?.asLayerInterface())
        
        // Set initial camera position
        if let center = camera.center.value, let zoom = camera.zoom.value {
            mapView.camera.move(toCenterPositionZoom: center, zoom: zoom, animated: false)
        }
        
        return mapView
    }
    
    func updateUIView(_ uiView: MCMapView, context: Context) {
        // Handle updates if needed
    }
}

struct ContentView: View {
    @State private var camera = MapView.Camera(
        latitude: 46.962592372639634,
        longitude: 8.378232525377973,
        zoom: 1000000
    )
    
    var body: some View {
        MapWithLineView(
            camera: $camera,
            coords: [
                // your coordinates here
            ]
        )
    }
}
```

##### UIKit

```swift
let lineLayer = MCLineLayerInterface.create()

lineLayer?.add(MCLineFactory.createLine("lineIdentifier",
                                        coordinates: coords,
                                        style: MCLineStyle(color: MCColorStateList(normal: UIColor.systemPink.withAlphaComponent(0.5).mapCoreColor,
                                                                                   highlighted: UIColor.blue.withAlphaComponent(0.5).mapCoreColor),
                                                           gapColor: MCColorStateList(normal: UIColor.red.withAlphaComponent(0.5).mapCoreColor,
                                                                                      highlighted: UIColor.gray.withAlphaComponent(0.5).mapCoreColor),
                                                           opacity: 1.0,
                                                           widthType: .SCREEN_PIXEL,
                                                           width: 50,
                                                           dashArray: [1,1],
                                                           lineCap: .BUTT,
                                                           offset: 0.0)))
                                                           
    mapView.add(layer: lineLayer?.asLayerInterface())
```



### Customisation

#### MCTiled2dMapLayerConfig

To use different raster tile services, create your own layer config. The layer config contains the information needed for the layer to compute the visible tiles in the current camera configuration, as well as to load and display them.

```swift
import MapCore

class TiledLayerConfig: MCTiled2dMapLayerConfig {
    // Defines both an additional scale factor for the tiles (and if they are scaled 
    // to match the target devices screen density), how many layers above the ideal 
    // one should be loaded an displayed as well, as well as if the layer is drawn,
    // when the zoom is smaller/larger than the valid range
    func getZoomInfo() -> MCTiled2dMapZoomInfo {
      MCTiled2dMapZoomInfo(zoomLevelScaleFactor: 0.65,
                           numDrawPreviousLayers: 1,
                           adaptScaleToScreen: true)
    }

    // Defines to map coordinate system of the layer
    public func getCoordinateSystemIdentifier() -> Int32 {
      MCCoordinateSystemIdentifiers.epsg3857()
    }

    // Defines the bounds of the layer
    func getBounds() -> MCRectCoord {
      let identifer = MCCoordinateSystemIdentifiers.epsg3857()
      let topLeft = MCCoord(systemIdentifier: identifer, 
                            x: -20037508.34, 
                            y: 20037508.34, z: 0.0)
      let bottomRight = MCCoord(systemIdentifier: identifer, 
                                x: 20037508.34, 
                                y: -20037508.34, z: 0.0)
      return MCRectCoord(
        topLeft: topLeft,
        bottomRight: bottomRight)
    }

    // Defines the url-pattern to load tiles. Enter a valid OSM tile server here
    func getTileUrl(_ x: Int32, y: Int32, zoom: Int32) -> String {
      return "https://example.com/tiles/\(zoom)/\(x)/\(y).png"
    }

    // The Layername
    func getLayerName() -> String {
        "OSM Layer"
    }

    // List of valid zoom-levels and their target zoom-value, the tile size in
    // the layers coordinate system, the number of tiles on that level and the
    // zoom identifier used for the tile-url (see getTileUrl above)
    func getZoomLevelInfos() -> [MCTiled2dMapZoomLevelInfo] {
        [
            .init(zoom: 559082264.029, tileWidthLayerSystemUnits: 40_075_016, numTilesX: 1, numTilesY: 1, numTilesT: 1, zoomLevelIdentifier: 0, bounds: getBounds()),
            .init(zoom: 279541132.015, tileWidthLayerSystemUnits: 20_037_508, numTilesX: 2, numTilesY: 2, numTilesT: 1, zoomLevelIdentifier: 1, bounds: getBounds()),
            .init(zoom: 139770566.007, tileWidthLayerSystemUnits: 10_018_754, numTilesX: 4, numTilesY: 4, numTilesT: 1, zoomLevelIdentifier: 2, bounds: getBounds()),
            .init(zoom: 69885283.0036, tileWidthLayerSystemUnits: 5_009_377.1, numTilesX: 8, numTilesY: 8, numTilesT: 1, zoomLevelIdentifier: 3, bounds: getBounds()),
            .init(zoom: 34942641.5018, tileWidthLayerSystemUnits: 2_504_688.5, numTilesX: 16, numTilesY: 16, numTilesT: 1, zoomLevelIdentifier: 4, bounds: getBounds()),
            .init(zoom: 17471320.7509, tileWidthLayerSystemUnits: 1_252_344.3, numTilesX: 32, numTilesY: 32, numTilesT: 1, zoomLevelIdentifier: 5, bounds: getBounds()),
            .init(zoom: 8735660.37545, tileWidthLayerSystemUnits: 626_172.1, numTilesX: 64, numTilesY: 64, numTilesT: 1, zoomLevelIdentifier: 6, bounds: getBounds()),
            .init(zoom: 4367830.18773, tileWidthLayerSystemUnits: 313_086.1, numTilesX: 128, numTilesY: 128, numTilesT: 1, zoomLevelIdentifier: 7, bounds: getBounds()),
            .init(zoom: 2183915.09386, tileWidthLayerSystemUnits: 156_543, numTilesX: 256, numTilesY: 256, numTilesT: 1, zoomLevelIdentifier: 8, bounds: getBounds()),
            .init(zoom: 1091957.54693, tileWidthLayerSystemUnits: 78271.5, numTilesX: 512, numTilesY: 512, numTilesT: 1, zoomLevelIdentifier: 9, bounds: getBounds()),
            .init(zoom: 545978.773466, tileWidthLayerSystemUnits: 39135.8, numTilesX: 1024, numTilesY: 1024, numTilesT: 1, zoomLevelIdentifier: 10, bounds: getBounds()),
            .init(zoom: 272989.386733, tileWidthLayerSystemUnits: 19567.9, numTilesX: 2048, numTilesY: 2048, numTilesT: 1, zoomLevelIdentifier: 11, bounds: getBounds()),
            .init(zoom: 136494.693366, tileWidthLayerSystemUnits: 9783.94, numTilesX: 4096, numTilesY: 4096, numTilesT: 1, zoomLevelIdentifier: 12, bounds: getBounds()),
            .init(zoom: 68247.3466832, tileWidthLayerSystemUnits: 4891.97, numTilesX: 8192, numTilesY: 8192, numTilesT: 1, zoomLevelIdentifier: 13, bounds: getBounds()),
            .init(zoom: 34123.6733416, tileWidthLayerSystemUnits: 2445.98, numTilesX: 16384, numTilesY: 16384, numTilesT: 1, zoomLevelIdentifier: 14, bounds: getBounds()),
            .init(zoom: 17061.8366708, tileWidthLayerSystemUnits: 1222.99, numTilesX: 32768, numTilesY: 32768, numTilesT: 1, zoomLevelIdentifier: 15, bounds: getBounds()),
            .init(zoom: 8530.91833540, tileWidthLayerSystemUnits: 611.496, numTilesX: 65536, numTilesY: 65536, numTilesT: 1, zoomLevelIdentifier: 16, bounds: getBounds()),
            .init(zoom: 4265.45916770, tileWidthLayerSystemUnits: 305.748, numTilesX: 131_072, numTilesY: 131_072, numTilesT: 1, zoomLevelIdentifier: 17, bounds: getBounds()),
            .init(zoom: 2132.72958385, tileWidthLayerSystemUnits: 152.874, numTilesX: 262_144, numTilesY: 262_144, numTilesT: 1, zoomLevelIdentifier: 18, bounds: getBounds()),
            .init(zoom: 1066.36479193, tileWidthLayerSystemUnits: 76.437, numTilesX: 524_288, numTilesY: 524_288, numTilesT: 1, zoomLevelIdentifier: 19, bounds: getBounds()),
            .init(zoom: 533.18239597, tileWidthLayerSystemUnits: 38.2185, numTilesX: 1_048_576, numTilesY: 1_048_576, numTilesT: 1, zoomLevelIdentifier: 20, bounds: getBounds()),
        ]
    }
}
```

#### Change map projection

Open Mobile Maps supports different coordinate systems and projections. The library provides built-in support for common coordinate systems and allows for custom implementations.

##### Supported Coordinate Systems

- **EPSG:3857** (Web Mercator) - Default for most web mapping services
- **EPSG:4326** (WGS84) - Standard latitude/longitude coordinates  
- **EPSG:2056** (LV95/LV03+) - Swiss coordinate system
- **Custom coordinate systems** can be implemented by extending the coordinate system interfaces

To render the map using a different coordinate system, initialize the map view with a Map Config. The library provides a factory for coordinate systems which we can use to initialize the map view. Layers can have a different projection than the map view itself.

##### SwiftUI

```swift
struct ContentView: View {
    @State private var camera = MapView.Camera(
        latitude: 46.962592372639634,
        longitude: 8.378232525377973,
        zoom: 1000000
    )
    @State private var layers: [any Layer] = []
    
    var body: some View {
        MapView(
            camera: $camera,
            mapConfig: .init(mapCoordinateSystem: MCCoordinateSystemFactory.getEpsg2056System()),
            layers: layers
        )
        .onAppear {
            setupLayers()
        }
    }
    
    private func setupLayers() {
        layers = [
            TiledRasterLayer("osm", webMercatorUrlFormat: "https://tiles.sample.org/{z}/{x}/{y}.png")
        ]
    }
}
```

##### UIKit

```swift
MCMapView(mapConfig: .init(mapCoordinateSystem: MCCoordinateSystemFactory.getEpsg2056System()))
```

#### Advanced Customization Examples

##### Custom Map Configuration

```swift
// Create a custom map configuration with specific settings
let mapConfig = MCMapConfig(
    mapCoordinateSystem: MCCoordinateSystemFactory.getEpsg3857System(),
    camera3dConfig: MCCamera3dConfigFactory.getBasicConfig()
)

// Configure rendering settings
mapConfig.renderingBackend = .METAL  // Use Metal rendering on iOS
mapConfig.multisampling = true      // Enable anti-aliasing
```

##### Custom Tile Loading

```swift
// Implement custom tile loader for specialized data sources
class CustomTileLoader: MCTileLoaderInterface {
    func load(_ request: MCTileLoaderRequest) {
        // Custom tile loading logic
        // Could load from local storage, custom server, etc.
        DispatchQueue.global().async {
            // Load tile data
            let tileData = self.loadCustomTileData(request)
            DispatchQueue.main.async {
                request.onLoaded(tileData)
            }
        }
    }
    
    func cancel(_ request: MCTileLoaderRequest) {
        // Cancel loading if needed
    }
}

// Use custom loader
let customLayer = TiledRasterLayer("custom", config: customConfig, tileLoader: CustomTileLoader())
```

##### Custom Rendering Callbacks

```swift
class MapCallbackHandler: MCMapViewCallbackHandler {
    func onMapReady() {
        print("Map is ready for interaction")
    }
    
    func onCameraChanged() {
        print("Camera position changed")
    }
    
    func onRenderingError(_ error: String) {
        print("Rendering error: \(error)")
    }
}

mapView.callbackHandler = MapCallbackHandler()
```

##### Styling and Theming

```swift
// Customize map appearance
extension MCMapView {
    func applyDarkTheme() {
        // Apply dark styling to layers
        backgroundColor = .black
        
        // Update existing raster layers with dark tiles
        layers.compactMap { $0 as? TiledRasterLayer }.forEach { layer in
            // Switch to dark tile variant if available
            layer.updateUrlFormat("https://dark-tiles.example.com/{z}/{x}/{y}.png")
        }
    }
    
    func applyCustomStyling() {
        // Custom visual settings
        layer.cornerRadius = 10
        layer.borderWidth = 2
        layer.borderColor = UIColor.systemBlue.cgColor
    }
}
```

##### Accessibility Support

```swift
// Make map accessible
mapView.isAccessibilityElement = true
mapView.accessibilityLabel = "Interactive map"
mapView.accessibilityHint = "Double tap to interact, drag to pan"

// Custom accessibility for map features
class AccessibleMapView: MCMapView {
    override func accessibilityElementDidBecomeFocused() {
        // Announce current map region
        let center = camera.getCenterPosition()
        UIAccessibility.post(notification: .announcement, 
                           argument: "Map centered at \(center.lat), \(center.lon)")
    }
}
```

## Troubleshooting

### Common Issues and Solutions

#### Build Issues

**"Module 'MapCore' not found"**
- Ensure you've added the package dependency correctly
- Try cleaning your build folder (⇧⌘K)
- Reset package caches: File → Package Dependencies → Reset Package Caches

**"Minimum deployment target"**
- Verify your project deployment target is iOS 14.0 or later
- For SwiftUI MapView, iOS 17.0+ is required

#### Runtime Issues

**"Failed to create vector layer"**
```swift
do {
    let layer = try VectorLayer("layer-id", styleURL: styleURL)
    mapView.add(layer: layer)
} catch VectorLayerError.invalidStyleURL {
    print("Style URL is not valid or accessible")
} catch VectorLayerError.networkError {
    print("Network error loading style")
} catch {
    print("Other error: \(error)")
}
```

**Memory warnings or crashes**
- Reduce the number of simultaneous layers
- Lower tile cache size: `MCTileLoader.setMaxCacheSize(50 * 1024 * 1024)`
- Remove unused layers promptly

**Poor rendering performance**
- Ensure you're not blocking the main thread
- Check for excessive layer overdraw
- Verify map view size constraints are properly set

#### Layer Issues

**Tiles not loading**
- Verify the tile URL pattern is correct
- Check network connectivity
- Ensure tile server supports CORS (for web requests)
- Test the tile URL in a browser: `https://example.com/1/0/0.png`

**Vector layer styling issues**
- Validate your style JSON against the Mapbox style specification
- Check console for style parsing errors
- Ensure sprite and glyph URLs are accessible

### Debugging Tips

#### Enable Debug Logging
```swift
// Enable detailed logging (debug builds only)
#if DEBUG
MCLogger.setLogLevel(.DEBUG)
#endif
```

#### Performance Debugging
```swift
// Monitor rendering performance
mapView.isRenderingDebugEnabled = true  // Shows frame time overlay
```

#### Network Debugging
- Use Charles Proxy or similar tools to monitor network requests
- Check tile loading patterns and response times
- Verify HTTP status codes for tile requests

## How to build

If you'd like to build Open Mobile Maps yourself, make sure you have all submodules initialized and updated. To do this, use
```shell
git submodule init
git submodule update
```

### Updating Djinni bridging files

The bridging interface between Kotlin and C++ are defined in the djinni files under [djinni](../djinni). After modifying those files, the new bridging code can be generated by running

```make clean djinni```

in the folder [djinni](../djinni). This generates the Kotlin bindings, the C++ header files as well as all the Objective C glue code.

### Building the iOS Package.

The [Package.swift](../Package.swift) file can be opened in Xcode and build directly from there. 

## License
This project is licensed under the terms of the MPL 2 license. See the [LICENSE](../LICENSE) file.

## Testing and Debugging

### Unit Testing Map Components

Open Mobile Maps components can be tested using standard XCTest frameworks:

```swift
import XCTest
import MapCore

class MapTests: XCTestCase {
    
    func testMapViewInitialization() {
        let mapView = MCMapView()
        XCTAssertNotNil(mapView)
        XCTAssertNotNil(mapView.camera)
    }
    
    func testLayerCreation() {
        let layer = TiledRasterLayer("test", webMercatorUrlFormat: "https://example.com/{z}/{x}/{y}.png")
        XCTAssertEqual(layer.layerName, "test")
    }
    
    func testVectorLayerCreation() {
        do {
            let layer = try VectorLayer("vector-test", styleURL: "https://example.com/style.json")
            XCTAssertEqual(layer.layerName, "vector-test")
        } catch {
            XCTFail("Vector layer creation failed: \(error)")
        }
    }
}
```

### Integration Testing

For testing map interactions and rendering:

```swift
class MapIntegrationTests: XCTestCase {
    var mapView: MCMapView!
    
    override func setUp() {
        super.setUp()
        mapView = MCMapView()
        // Add to a test window for proper lifecycle
        let window = UIWindow(frame: CGRect(x: 0, y: 0, width: 300, height: 300))
        window.addSubview(mapView)
        mapView.frame = window.bounds
    }
    
    func testLayerAddition() {
        let layer = TiledRasterLayer("test", webMercatorUrlFormat: "https://tile.openstreetmap.org/{z}/{x}/{y}.png")
        
        let expectation = self.expectation(description: "Layer added")
        
        mapView.add(layer: layer)
        
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.1) {
            // Verify layer was added
            XCTAssertTrue(self.mapView.layers.contains { $0.layerName == "test" })
            expectation.fulfill()
        }
        
        wait(for: [expectation], timeout: 1.0)
    }
}
```

### SwiftUI Testing

For SwiftUI MapView testing:

```swift
import SwiftUI
import ViewInspector

@available(iOS 17.0, *)
class SwiftUIMapTests: XCTestCase {
    
    func testMapViewCreation() throws {
        let camera = MapView.Camera(latitude: 0, longitude: 0, zoom: 1000)
        let mapView = MapView(camera: .constant(camera), layers: [])
        
        XCTAssertNoThrow(try mapView.inspect())
    }
}
```

### Performance Testing

Monitor performance characteristics:

```swift
class MapPerformanceTests: XCTestCase {
    
    func testLayerLoadingPerformance() {
        let mapView = MCMapView()
        
        measure {
            for i in 0..<10 {
                let layer = TiledRasterLayer("layer\(i)", webMercatorUrlFormat: "https://example.com/{z}/{x}/{y}.png")
                mapView.add(layer: layer)
            }
        }
    }
}
```

### Debugging Tools

#### Xcode Integration
- Use Xcode's **Memory Graph Debugger** to detect memory leaks
- **View Debugger** can help inspect map view hierarchy
- **Network Link Conditioner** to test poor network conditions

#### Custom Debugging
```swift
extension MCMapView {
    func debugInfo() -> String {
        return """
        Camera Position: \(camera.getCenterPosition())
        Zoom Level: \(camera.getZoom())
        Layer Count: \(layers.count)
        Active Layers: \(layers.map { $0.layerName }.joined(separator: ", "))
        """
    }
}
```

## Third-Party Software
This project depends on:

- [Swift Atomics](https://github.com/apple/swift-atomics) – © 2020 Apple Inc. – Licensed under the [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0)
