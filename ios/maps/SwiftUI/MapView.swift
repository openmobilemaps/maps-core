#if canImport(UIKit)
    //
    //  MapView.swift
    //  Meteo
    //
    //  Created by Nicolas Märki on 01.09.22.
    //

    import CoreLocation
    import SwiftUI

    public struct MapView: UIViewRepresentable {

        public let initialize: @MainActor @Sendable (MCMapInterface) async throws -> Void
        public let mapConfig: MCMapConfig
        public let pixelsPerInch: Float?
        public let is3D: Bool

        public init(
            mapConfig: MCMapConfig,
            pixelsPerInch: Float? = nil,
            initialize: @escaping @MainActor @Sendable (MCMapInterface) async throws -> Void
        ) {
            self.initialize = initialize
            self.mapConfig = mapConfig
            self.pixelsPerInch = pixelsPerInch
            // Unit sphere system uses identifier 100_000 (MCCoordinateSystemIdentifiers.unitSphere).
            // Compare identifiers instead of object identity since factory returns new objects.
            self.is3D = mapConfig.mapCoordinateSystem.identifier == MCCoordinateSystemFactory.getUnitSphereSystem().identifier
        }

        public init(
            is3D: Bool = true,
            pixelsPerInch: Float? = nil,
            initialize: @escaping @MainActor @Sendable (MCMapInterface) async throws -> Void
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
            let mapInterface = mapView.mapInterface
            Task { @MainActor in
                do {
                    try await initialize(mapInterface)
                } catch {
                    assertionFailure("MapView initialization failed: \(error)")
                }
            }
            return mapView
        }

        public func updateUIView(_: UIViewType, context _: Context) {}
    }

    #Preview {
        MapView(is3D: true) { mapInterface in
            let layer = try MCTiled2dMapRasterLayerInterface.webMercator(
                urlFormat: "https://a.tile.openstreetmap.org/{z}/{x}/{y}.png"
            )
            mapInterface.addLayer(layer)
        }
    }
#endif
