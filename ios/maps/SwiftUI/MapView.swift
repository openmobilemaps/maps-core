//
//  MapView.swift
//  Meteo
//
//  Created by Nicolas Märki on 01.09.22.
//

import CoreLocation
import SwiftUI

@available(iOS 17.0, *)
public struct MapView: UIViewRepresentable {
    public enum UpdateMode: Equatable {
        case map
        case user
    }

    public struct Updatable<T: Equatable>: Equatable {
        public let mode: UpdateMode
        public let value: T?

        public init() {
            mode = .map
            value = nil
        }

        public init(mode: UpdateMode) {
            self.mode = mode
            value = nil
        }

        public init(mode: UpdateMode, value: T?) {
            self.mode = mode
            self.value = value
        }
    }

    public struct Camera: Equatable {
        public var center: Updatable<MCCoord>
        public var zoom: Updatable<Double>
        public var visibleRect: Updatable<MCRectCoord>

        public init(center: Updatable<MCCoord> = .init(),
                    zoom: Updatable<Double> = .init(),
                    visibleRect: Updatable<MCRectCoord> = .init()) {
            self.center = center
            self.zoom = zoom
            self.visibleRect = visibleRect
        }

        public init(location: CLLocationCoordinate2D, zoom: Double?) {
            self.center = .init(mode: .user, value: MCCoord(systemIdentifier: MCCoordinateSystemIdentifiers.epsg4326(), x: location.longitude, y: location.latitude, z: 0))
            self.zoom = .init(mode: .user, value: zoom)
            self.visibleRect = .init()
        }

        public init(latitude: Double, longitude: Double, zoom: Double?) {
            self.center = .init(mode: .user, value: MCCoord(systemIdentifier: MCCoordinateSystemIdentifiers.epsg4326(), x: longitude, y: latitude, z: 0))
            self.zoom = .init(mode: .user, value: zoom)
            self.visibleRect = .init()
        }

        public var centerCoordinate: MCCoord? {
            center.value
        }

        public static func == (lhs: Camera, rhs: Camera) -> Bool {
            lhs.zoom == rhs.zoom
                && lhs.visibleRect == rhs.visibleRect
                && lhs.center == rhs.center
        }
    }

    let mapConfig: MCMapConfig

    let layers: [(any Layer)?]

    @Binding var camera: Camera

    var paddingInset: EdgeInsets

    public init(camera: Binding<Camera>, mapConfig: MCMapConfig = MCMapConfig(mapCoordinateSystem: MCCoordinateSystemFactory.getEpsg3857System()), paddingInset: EdgeInsets = .init(), layers: [(any Layer)?]) {
        self.layers = layers
        self.mapConfig = mapConfig
        self._camera = camera
        self.paddingInset = paddingInset
    }

    public func makeUIView(context: Context) -> MCMapView {
        let mapView = MCMapView(mapConfig: mapConfig)
        mapView.backgroundColor = .clear
        mapView.camera.addListener(context.coordinator)
        mapView.camera.setRotationEnabled(false)
        mapView.sizeDelegate = context.coordinator
//        MapActivityHandler.shared.mapView = mapView
        context.coordinator.mapView = mapView
        return mapView
    }

    public func makeCoordinator() -> MapViewCoordinator {
        MapViewCoordinator(parent: self)
    }

    @MainActor
    fileprivate func updateCamera(_ mapView: MCMapView, _ coordinator: MapViewCoordinator) {
        guard mapView.bounds.size != .zero,
              coordinator.hasSizeChanged
        else {
            return
        }

        guard camera != coordinator.lastWrittenCamera else {
            return
        }

        // TODO: Allow padding definition
        //        mapView.camera.setPaddingTop(Float(-paddingInset.top))
        //        mapView.camera.setPaddingBottom(0.003)
        //        mapView.camera.setPaddingLeft(Float(-paddingInset.leading))
        //        mapView.camera.setPaddingRight(Float(-paddingInset.trailing))

        coordinator.ignoreCallbacks = true
        let animated = coordinator.lastWrittenCamera != nil
        if let center = camera.center.value, let zoom = camera.zoom.value, camera.center.mode == .user, camera.zoom.mode == .user {
            mapView.camera.move(toCenterPositionZoom: center, zoom: zoom, animated: animated)
        } else if let center = camera.center.value, camera.center.mode == .user {
            mapView.camera.move(toCenterPosition: center, animated: animated)
        } else if let zoom = camera.zoom.value, camera.zoom.mode == .user {
            mapView.camera.setZoom(zoom, animated: animated)
        } else if let visibleRect = camera.visibleRect.value, camera.visibleRect.mode == .user {
            mapView.camera.move(toBoundingBox: visibleRect, paddingPc: 0, animated: animated, minZoom: nil, maxZoom: nil)
        }

        coordinator.ignoreCallbacks = false
        coordinator.lastWrittenCamera = camera
    }

    @MainActor
    fileprivate func updateLayers(_ context: MapView.Context, _ mapView: MCMapView) {
        // Get description-structs of layers
        let oldLayers = context.coordinator.currentLayers
        let newLayers = layers.compactMap { $0?.interface }

        var needsInsert: [any MCLayerInterface] = []
        var needsRemoval: [any MCLayerInterface] = []

        // Find layers that...
        for layer in newLayers {
            if let _ = oldLayers.first(where: { $0 === layer }) {
                // ... existed before
            } else {
                // ... are new
                needsInsert.append(layer)
            }
        }

        // ... are not needed any more
        for layer in oldLayers {
            if !newLayers.contains(where: { $0 === layer }) {
                needsRemoval.append(layer)
            }
        }

        context.coordinator.currentLayers = newLayers

        // Schedule Task to load or onload layers if needed
        if !needsInsert.isEmpty || !needsRemoval.isEmpty {
            for layer in needsInsert {
                mapView.add(layer: layer)
            }
            for layer in needsRemoval {
                mapView.remove(layer: layer)
            }
        }
    }

    public func updateUIView(_ mapView: MapCore.MCMapView, context: Context) {
        updateLayers(context, mapView)

        updateCamera(mapView, context.coordinator)
    }
}

extension MCRectCoord {
    override open func isEqual(_ object: Any?) -> Bool {
        if let other = object as? MCRectCoord {
            return other.topLeft == topLeft
                && other.bottomRight == bottomRight
        }
        return false
    }
}

extension MCCoord {
    override open func isEqual(_ object: Any?) -> Bool {
        if let other = object as? MCCoord {
            return other.systemIdentifier == systemIdentifier
                && other.x == x
                && other.y == y
                && other.z == z
        }
        return false
    }
}

@available(iOS 17.0, *)
@MainActor
public class MapViewCoordinator: MCMapCamera2dListenerInterface {
    var parent: MapView
    init(parent: MapView) {
        self.parent = parent
    }

    weak var mapView: MCMapView?

    var currentLayers: [any MCLayerInterface] = []

    var ignoreCallbacks = false
    var hasSizeChanged = false
    var lastWrittenCamera: MapView.Camera?

    public nonisolated
    func onVisibleBoundsChanged(_ visibleBounds: MCRectCoord, zoom: Double) {
        Task { @MainActor in
            guard !ignoreCallbacks else {
                return
            }

            guard lastWrittenCamera != nil else {
                if let mapView {
                    parent.updateCamera(mapView, self)
                }
                return
            }

            guard hasSizeChanged else {
                return
            }

            parent.camera = MapView.Camera(
                center: .init(mode: .map, value: mapView?.camera.getCenterPosition()),
                zoom: .init(mode: .map, value: mapView?.camera.getZoom()),
                visibleRect: .init(mode: .map, value: mapView?.camera.getPaddingAdjustedVisibleRect())
            )

            lastWrittenCamera = parent.camera
        }
    }

    public nonisolated
    func onRotationChanged(_ angle: Float) {
    }

    public nonisolated
    func onMapInteraction() {
    }
}

@available(iOS 17.0, *)
extension MapViewCoordinator: MCMapSizeDelegate {
    public nonisolated
    func sizeChanged() {
        Task { @MainActor in
            hasSizeChanged = true
            if let mapView {
                parent.updateCamera(mapView, self)
            }
        }
    }
}
