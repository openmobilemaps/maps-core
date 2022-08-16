<h1 align="center">Open Mobile Maps</h1>
<br />
<div align="center">
  <img width="200" height="45" src="../logo.svg" />
  <br />
  <br />
  The lightweight and modern Map SDK for Android (6.0+) and iOS (10+)
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

## Installation
Open Mobile Maps is available through [Swift Package Manager](https://swift.org/package-manager/).

### Xcode
For App integration within XCode, add this package to your App target. To do this, follow the step by step tutorial [Adding Package Dependencies to Your App](https://developer.apple.com/documentation/xcode/adding_package_dependencies_to_your_app).

### Swift Package

Once you have your Swift package set up, adding Open Mobile Maps as a dependency is as easy as adding it to the dependencies value of your Package.swift.
```swift
dependencies: [
    .package(url: "https://github.com/openmobilemaps/maps-core.git", .upToNextMajor(from: "1.4.1"))
]
```

### iOS 10
Unfortunately, Swift package is only supported starting with iOS 11. If you need iOS 10 support you have to compile the library as a framework yourself.

## How to use

### Display tiled raster map in [EPSG3857](https://epsg.io/3857) system

#### MCTiled2dMapLayerConfig

To display a map, you first need to add a layer config for your project. The layer config contains the information needed for the layer to compute the visible tiles in the current camera configuration, as well as to load and display them.

```swift
import MapCore

class TiledLayerConfig: MCTiled2dMapLayerConfig {
    // Defines both an additional scale factor for the tiles, as well as how many
 		// layers above the ideal one should be loaded an displayed as well.
    func getZoomInfo() -> MCTiled2dMapZoomInfo {
      MCTiled2dMapZoomInfo(zoomLevelScaleFactor: 0.65,
                           numDrawPreviousLayers: 1,
                           adaptScaleToScreen: true)
    }

    // Defines to map coordinate system of the layer
    public func getCoordinateSystemIdentifier() -> String {
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
            .init(zoom: 500_000_000, tileWidthLayerSystemUnits: 40_075_016, numTilesX: 1, numTilesY: 1, zoomLevelIdentifier: 0, bounds: getBounds()),
            .init(zoom: 250_000_000, tileWidthLayerSystemUnits: 20_037_508, numTilesX: 2, numTilesY: 2, zoomLevelIdentifier: 1, bounds: getBounds()),
            .init(zoom: 150_000_000, tileWidthLayerSystemUnits: 10_018_754, numTilesX: 4, numTilesY: 4, zoomLevelIdentifier: 2, bounds: getBounds()),
            .init(zoom: 70_000_000, tileWidthLayerSystemUnits: 5_009_377.1, numTilesX: 8, numTilesY: 8, zoomLevelIdentifier: 3, bounds: getBounds()),
            .init(zoom: 35_000_000, tileWidthLayerSystemUnits: 2_504_688.5, numTilesX: 16, numTilesY: 16, zoomLevelIdentifier: 4, bounds: getBounds()),
            .init(zoom: 15_000_000, tileWidthLayerSystemUnits: 1_252_344.3, numTilesX: 32, numTilesY: 32, zoomLevelIdentifier: 5, bounds: getBounds()),
            .init(zoom: 10_000_000, tileWidthLayerSystemUnits: 626_172.1, numTilesX: 64, numTilesY: 64, zoomLevelIdentifier: 6, bounds: getBounds()),
            .init(zoom: 4_000_000, tileWidthLayerSystemUnits: 313_086.1, numTilesX: 128, numTilesY: 128, zoomLevelIdentifier: 7, bounds: getBounds()),
            .init(zoom: 2_000_000, tileWidthLayerSystemUnits: 156_543, numTilesX: 256, numTilesY: 256, zoomLevelIdentifier: 8, bounds: getBounds()),
            .init(zoom: 1_000_000, tileWidthLayerSystemUnits: 78271.5, numTilesX: 512, numTilesY: 512, zoomLevelIdentifier: 9, bounds: getBounds()),
            .init(zoom: 500_000, tileWidthLayerSystemUnits: 39135.8, numTilesX: 1024, numTilesY: 1024, zoomLevelIdentifier: 10, bounds: getBounds()),
            .init(zoom: 250_000, tileWidthLayerSystemUnits: 19567.9, numTilesX: 2048, numTilesY: 2048, zoomLevelIdentifier: 11, bounds: getBounds()),
            .init(zoom: 150_000, tileWidthLayerSystemUnits: 9783.94, numTilesX: 4096, numTilesY: 4096, zoomLevelIdentifier: 12, bounds: getBounds()),
            .init(zoom: 70000, tileWidthLayerSystemUnits: 4891.97, numTilesX: 8192, numTilesY: 8192, zoomLevelIdentifier: 13, bounds: getBounds()),
            .init(zoom: 35000, tileWidthLayerSystemUnits: 2445.98, numTilesX: 16384, numTilesY: 16384, zoomLevelIdentifier: 14, bounds: getBounds()),
            .init(zoom: 15000, tileWidthLayerSystemUnits: 1222.99, numTilesX: 32768, numTilesY: 32768, zoomLevelIdentifier: 15, bounds: getBounds()),
            .init(zoom: 8000, tileWidthLayerSystemUnits: 611.496, numTilesX: 65536, numTilesY: 65536, zoomLevelIdentifier: 16, bounds: getBounds()),
            .init(zoom: 4000, tileWidthLayerSystemUnits: 305.748, numTilesX: 131_072, numTilesY: 131_072, zoomLevelIdentifier: 17, bounds: getBounds()),
            .init(zoom: 2000, tileWidthLayerSystemUnits: 152.874, numTilesX: 262_144, numTilesY: 262_144, zoomLevelIdentifier: 18, bounds: getBounds()),
            .init(zoom: 1000, tileWidthLayerSystemUnits: 76.437, numTilesX: 524_288, numTilesY: 524_288, zoomLevelIdentifier: 19, bounds: getBounds()),
            .init(zoom: 500, tileWidthLayerSystemUnits: 38.2185, numTilesX: 1_048_576, numTilesY: 1_048_576, zoomLevelIdentifier: 20, bounds: getBounds()),
        ]
    }
}
```

#### MapViewController

Once we have the layer config, we can add a map view to our view controller.

The library provides a factory for the [EPSG3857](https://epsg.io/4326) Coordinate system, which we can use to initialize the map view. For this example, we also use the default implementation of the [TextureLoader](https://github.com/openmobilemaps/maps-core/blob/develop/ios/maps/MCTextureLoader.swift), but this can also be implemented by the app itself.

```swift
import MapCore
import MapCoreSharedModule

class MapViewController: UIViewController {
  var mapConfig = MCMapConfig(mapCoordinateSystem: MCCoordinateSystemFactory.getEpsg3857System())

  lazy var mapView = MCMapView(mapConfig: mapConfig)
  
  lazy var loader = MCTextureLoader()

  lazy var rasterLayer = MCTiled2dMapRasterLayerInterface.create(TiledLayerConfig(),
                                                    loader: loader)
  
  override func loadView() { view = mapView }
  
  override func viewDidLoad() {
    super.viewDidLoad()
      
    mapView.add(layer: rasterLayer?.asLayerInterface())
  }
}
```

### Parsing a WMTS Capability 

Open Mobile Maps supports the [WMTS standard](https://en.wikipedia.org/wiki/Web_Map_Tile_Service) and can parse their Capability XML file to generate raster layer configurations.

```swift
let resource = MCWmtsCapabilitiesResource.create(xml)!
```
The created resource object is then capable of creating a layer object with a given identifier.

```swift
let layer = resource.createLayer("identifier", tileLoader: loader)
mapView.add(layer: layer?.asLayerInterface())
```
This feature is still being improved to support a wider range of WMTS capabilities.

### Polygon layer

Open Mobile Maps provides a simple interface to create a polygon layer. The layer handles the rendering of the given polygons and calls the callback handler in case of user interaction.

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

### Icon layer

A simple icon layer is implemented as well. This supports displaying textures at the given coordinates. A scale parameter has to be provided which specifies how the icon should be affected by camera movements. In case of user interaction, the given callback handler will be called.

```swift
let iconLayer = MCIconLayerInterface.create()
let image = UIImage(named: "image")
let texture = try! TextureHolder(image!.cgImage!)
let icon = MCIconFactory.createIcon("icon",
                         coordinate: coordinate,
                         texture: texture,
                         iconSize: .init(x: Float(texture.getImageWidth()), y: Float(texture.getImageHeight())),
                         scale: .FIXED)
iconLayer?.add(icon)
iconLayer?.setCallbackHandler(handler)
mapView.add(layer: iconLayer?.asLayerInterface())
```

### Line layer
A line layer can be added to the mapView as well. Using the MCLineFactory a LineInfo object can be created. The width can be specified in either SCREEN_PIXEL or MAP_UNIT.

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
                                                           lineCap: .BUTT)))
                                                           
    mapView.add(layer: lineLayer?.asLayerInterface())
```

#### Adjusting the Camera

The camera position can easily be adjusted by manipulating the Camera2dInterface received from the map. E.g. to set a custom location:
```swift
mapView.camera.move(toCenterPosition: .init(systemIdentifier: MCCoordinateSystemIdentifiers.epsg4326(),
                                                x: 8.378232525377973,
                                                y: 46.962592372639634,
                                                z: 0), animated: true)
```

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
