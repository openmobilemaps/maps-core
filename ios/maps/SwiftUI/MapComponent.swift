#if canImport(UIKit)
    //
    //  MapComponent.swift
    //  Meteo
    //
    //  Backwards-compat wrapper that matches the pre-feature/kmp-2 `MapComponent` SwiftUI API.
    //  The modern entry point is `MapView`, which hands callers an `MCMapInterface` rather than the
    //  concrete `MCMapView`. fluid-meteogram's reports-ui-ios still drives the map via the MCMapView
    //  surface (layers are added through mapView, coordinate converters are registered on the view,
    //  etc.), so this shim preserves that call site.
    //
    //  TODO(openmobilemaps-kmp-2): drop this once fluid-meteogram migrates to the MCMapInterface-based
    //  `MapView` API.
    //

    import CoreLocation
    import SwiftUI

    public struct MapComponent: UIViewRepresentable {
        public let initialize: (MCMapView) -> Void
        public let mapConfig: MCMapConfig
        public let pixelsPerInch: Float?
        public let is3D: Bool

        public init(
            mapConfig: MCMapConfig = MCMapConfig(
                mapCoordinateSystem: MCCoordinateSystemFactory.getEpsg3857System()
            ),
            pixelsPerInch: Float? = nil,
            initialize: @escaping (MCMapView) -> Void
        ) {
            self.initialize = initialize
            self.mapConfig = mapConfig
            self.pixelsPerInch = pixelsPerInch
            self.is3D = mapConfig.mapCoordinateSystem.identifier == MCCoordinateSystemFactory.getUnitSphereSystem().identifier
        }

        public init(
            is3D: Bool,
            pixelsPerInch: Float? = nil,
            initialize: @escaping (MCMapView) -> Void
        ) {
            self.initialize = initialize
            self.pixelsPerInch = pixelsPerInch
            let mcs: MCMapCoordinateSystem =
                switch is3D {
                    case true: MCCoordinateSystemFactory.getUnitSphereSystem()
                    case false: MCCoordinateSystemFactory.getEpsg3857System()
                }
            self.mapConfig = MCMapConfig(mapCoordinateSystem: mcs)
            self.is3D = is3D
        }

        public func makeUIView(context _: Context) -> some UIView {
            let mapView = MCMapView(mapConfig: mapConfig, pixelsPerInch: pixelsPerInch, is3D: is3D)
            initialize(mapView)
            return mapView
        }

        public func updateUIView(_: UIViewType, context _: Context) {}
    }
#endif
