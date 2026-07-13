<h1 align="center">Open Mobile Maps</h1>
<br />
<div align="center">
  <img width="200" height="45" src="../logo.svg" />
  <br />
  <br />
  The lightweight and modern Map SDK for Android (6.0+) and iOS (14+)
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
    .package(url: "https://github.com/openmobilemaps/maps-core", from: .init(stringLiteral: "4.0.0"))
]
```


## How to use

### MapView

On iOS, Open Mobile Maps exposes two primary entry points:

- `MapView` for SwiftUI. Its setup closure is `async throws` and receives an `MCMapInterface`.
- `MCMapView` for UIKit. You configure the view directly and use its `camera` / `add(layer:)` helpers.

Convenience APIs live on the underlying types:

- `MCTiled2dMapRasterLayerInterface.webMercator(...)`
- `MCTiled2dMapRasterLayerInterface.make(...)`
- `MCTiled2dMapVectorLayerInterface.make(...)`
- `MCWmtsCapabilitiesResource.from(...)`
- `MCWmtsCapabilitiesResource.rasterLayer(...)`

Raster, vector, and GPS layers can be added directly through `MCMapInterface.addLayer(...)` or `MCMapView.add(layer: ...)`.

#### SwiftUI

`MapView` is available on iOS 17.0 or later.

```swift
import MapCore
import SwiftUI

struct ContentView: View {
    var body: some View {
        MapView { mapInterface in
            let base = try MCTiled2dMapRasterLayerInterface.webMercator(
                "osm",
                urlFormat: "https://tiles.sample.org/{z}/{x}/{y}.png"
            )
            let overlay = try MCTiled2dMapVectorLayerInterface.make(
                "overlay",
                styleURL: "https://www.sample.org/overlay/style.json"
            )

            mapInterface.addLayer(base)
            mapInterface.addLayer(overlay)
            mapInterface.camera.move(
                toCenterPositionZoom: MCCoord(lat: 46.962592372639634, lon: 8.378232525377973),
                zoom: 1_000_000,
                animated: false
            )
        }
    }
}
```

For 3D maps, initialize `MapView` with `is3D: true` or provide a `mapConfig` with the unit-sphere coordinate system.

```swift
MapView(is3D: true) { mapInterface in
    let base = try MCTiled2dMapRasterLayerInterface.webMercator(
        urlFormat: "https://tiles.sample.org/{z}/{x}/{y}.png"
    )
    mapInterface.addLayer(base)
}
```

#### UIKit

```swift
import MapCore
import UIKit

class MapViewController: UIViewController {
    lazy var mapView = MCMapView()

    override func loadView() {
        view = mapView
    }

    override func viewDidLoad() {
        super.viewDidLoad()

        let base = try! MCTiled2dMapRasterLayerInterface.webMercator(
            "osm",
            urlFormat: "https://tiles.sample.org/{z}/{x}/{y}.png"
        )
        mapView.add(layer: base)

        mapView.camera.move(
            toCenterPositionZoom: MCCoord(lat: 46.962592372639634, lon: 8.378232525377973),
            zoom: 1_000_000,
            animated: false
        )
    }
}
```

### Parsing WMTS capabilities

Open Mobile Maps can parse [WMTS](https://en.wikipedia.org/wiki/Web_Map_Tile_Service) capabilities documents and turn them into raster layers.

#### SwiftUI

```swift
import MapCore
import SwiftUI

struct ContentView: View {
    var body: some View {
        MapView { mapInterface in
            let resource = try await MCWmtsCapabilitiesResource.from(
                url: URL(string: "https://example.com/WMTSCapabilities.xml")!
            )
            let layer = try resource.rasterLayer(identifier: "identifier")
            mapInterface.addLayer(layer)
        }
    }
}
```

#### UIKit

```swift
let xml = try String(contentsOf: capabilitiesURL)
let resource = try MCWmtsCapabilitiesResource.from(xmlString: xml)
let layer = try resource.rasterLayer(identifier: "identifier")

mapView.add(layer: layer)
```

### Vector tiles

Use `MCTiled2dMapVectorLayerInterface.make(...)` for the common “style URL + default loaders” case.

```swift
let layer = try MCTiled2dMapVectorLayerInterface.make(
    "base-map",
    styleURL: "https://www.sample.org/base-map/style.json"
)
mapView.add(layer: layer)
```

You can still drop down to `createExplicitly(...)` when you need local data providers, custom zoom info, or custom source URL parameters.

### Overlays

Polygon, icon, and line layers are still created through their dedicated interfaces and then added as `MCLayerInterface`s.

```swift
let polygonLayer = MCPolygonLayerInterface.create()
let polygonInfo = MCPolygonInfo(
    identifier: "switzerland",
    coordinates: MCPolygonCoord(positions: coords, holes: []),
    color: UIColor.red.mapCoreColor,
    highlight: UIColor.red.withAlphaComponent(0.2).mapCoreColor
)

polygonLayer?.add(polygonInfo)
mapView.add(layer: polygonLayer?.asLayerInterface())
```

```swift
let iconLayer = MCIconLayerInterface.create()
let image = UIImage(named: "image")!
let texture = try TextureHolder(image.cgImage!)
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
```

```swift
let lineLayer = MCLineLayerInterface.create()
lineLayer?.add(
    MCLineFactory.createLine(
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
            dashArray: [1, 1],
            lineCap: .BUTT,
            offset: 0.0
        )
    )
)
mapView.add(layer: lineLayer?.asLayerInterface())
```

### Custom raster layer configs

To use a custom raster source, provide your own `MCTiled2dMapLayerConfig` and then create a raster layer from it.

```swift
import MapCore

class TiledLayerConfig: MCTiled2dMapLayerConfig {
    func getZoomInfo() -> MCTiled2dMapZoomInfo {
        MCTiled2dMapZoomInfo(
            zoomLevelScaleFactor: 0.65,
            numDrawPreviousLayers: 1,
            adaptScaleToScreen: true
        )
    }

    public func getCoordinateSystemIdentifier() -> Int32 {
        MCCoordinateSystemIdentifiers.epsg3857()
    }

    func getBounds() -> MCRectCoord {
        let identifier = MCCoordinateSystemIdentifiers.epsg3857()
        return MCRectCoord(
            topLeft: MCCoord(systemIdentifier: identifier, x: -20_037_508.34, y: 20_037_508.34, z: 0),
            bottomRight: MCCoord(systemIdentifier: identifier, x: 20_037_508.34, y: -20_037_508.34, z: 0)
        )
    }

    func getTileUrl(_ x: Int32, y: Int32, zoom: Int32) -> String {
        "https://example.com/tiles/\(zoom)/\(x)/\(y).png"
    }

    func getLayerName() -> String {
        "OSM Layer"
    }

    func getZoomLevelInfos() -> [MCTiled2dMapZoomLevelInfo] {
        [
            .init(
                zoom: 559082264.029,
                tileWidthLayerSystemUnits: 40_075_016,
                numTilesX: 1,
                numTilesY: 1,
                numTilesT: 1,
                zoomLevelIdentifier: 0,
                bounds: getBounds()
            )
            // Add additional zoom levels here.
        ]
    }
}

let layer = try MCTiled2dMapRasterLayerInterface.make(config: TiledLayerConfig())
mapView.add(layer: layer)
```

### Change map projection

Use `MCMapConfig` to pick the map coordinate system independently from the layer source projection.

#### SwiftUI

```swift
MapView(
    mapConfig: .init(mapCoordinateSystem: MCCoordinateSystemFactory.getEpsg2056System())
) { mapInterface in
    let base = try MCTiled2dMapRasterLayerInterface.webMercator(
        "osm",
        urlFormat: "https://tiles.sample.org/{z}/{x}/{y}.png"
    )
    mapInterface.addLayer(base)
}
```

#### UIKit

```swift
let mapView = MCMapView(
    mapConfig: .init(mapCoordinateSystem: MCCoordinateSystemFactory.getEpsg2056System())
)
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

## Third-Party Software
This project depends on:

- [Swift Atomics](https://github.com/apple/swift-atomics) – © 2020 Apple Inc. – Licensed under the [Apache License 2.0](http://www.apache.org/licenses/LICENSE-2.0)
