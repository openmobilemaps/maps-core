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
        public let animated: Bool

        public init() {
            mode = .map
            value = nil
            animated = true
        }

        public init(mode: UpdateMode) {
            self.mode = mode
            value = nil
            self.animated = true
        }

        public init(mode: UpdateMode, value: T?, animated: Bool = true) {
            self.mode = mode
            self.value = value
            self.animated = animated
        }
    }

    @Environment(\.mapProjection) var projection: Projection
    @Environment(\.mapCameraConfig) var cameraConfig: CameraConfig

    @Environment(\.mapPadding) var padding: EdgeInsets

    let layers: [(any Layer)?]

    @Binding var camera: Camera

    let touchHandler: TouchHandler?

    public init(
        camera: Binding<Camera>,
        layers: [(any Layer)?],
        touchHandler: TouchHandler? = nil
    ) {
        self.layers = layers
        self._camera = camera
        self.touchHandler = touchHandler
    }

    public func makeUIView(context: Context) -> MCMapView {

        let coordinator = context.coordinator
        let projection = coordinator.projection

        let mapView = MCMapView(mapConfig: MCMapConfig(mapCoordinateSystem: projection.coordinateSystem), is3D: projection.is3D)
        mapView.backgroundColor = .clear
        mapView.camera.addListener(context.coordinator)
        mapView.camera.setRotationEnabled(false)
        if projection.is3D {
            mapView.camera.asMapCamera3d()?.setCameraConfig(cameraConfig.mcConfig, durationSeconds: nil, targetZoom: nil, targetCoordinate: nil)
        }
        mapView.sizeDelegate = context.coordinator
        if let touchHandler {
            mapView.mapInterface.getTouchHandler()?.addListener(touchHandler)
            touchHandler.mapView = mapView
        }

        coordinator.mapView = mapView

        return mapView
    }

    public func makeCoordinator() -> MapViewCoordinator {
        MapViewCoordinator(projection: projection, camera: $camera)
    }

    @MainActor
    fileprivate func updateCamera(_ mapView: MCMapView, _ context: Context) {

        let coordinator = context.coordinator
        var animated = context.transaction.animation != nil

        guard mapView.bounds.size != .zero,
            coordinator.hasSizeChanged
        else {
            return
        }

        if coordinator.lastUpdates.camera == nil {
            animated = false
        }

        if padding != coordinator.lastUpdates.padding {
            let scale = Float(UIScreen.main.nativeScale)
            mapView.camera.setPaddingTop(scale * Float(padding.top))
            mapView.camera.setPaddingBottom(scale * Float(padding.bottom))
            mapView.camera.setPaddingLeft(scale * Float(padding.leading))
            mapView.camera.setPaddingRight(scale * Float(padding.trailing))
            coordinator.lastUpdates.padding = padding
        }

        coordinator.ignoreCallbacks = true

        if coordinator.lastUpdates.camera == nil {
            if let minZoom = camera.minZoom {
                mapView.camera.setMinZoom(minZoom)
            }
            if let maxZoom = camera.maxZoom {
                mapView.camera.setMinZoom(maxZoom)
            }
            mapView.camera.move(toCenterPositionZoom: camera.position.mcCoord, zoom: camera.zoom, animated: false)
            coordinator.lastUpdates.camera = camera
        }

        if let maxZoom = camera.maxZoom, coordinator.lastUpdates.camera!.maxZoom != maxZoom  {
            mapView.camera.setMaxZoom(maxZoom)
        }
        if let minZoom = camera.minZoom, coordinator.lastUpdates.camera!.minZoom != minZoom  {
            mapView.camera.setMaxZoom(minZoom)
        }


        if coordinator.projection.is3D, coordinator.lastUpdates.cameraConfig != cameraConfig {
            var targetZoom: Double?
            if camera.zoom != coordinator.lastUpdates.camera?.zoom {
                targetZoom = camera.zoom
            }
            var targetCoordinate: Coordinate?
            if camera.position != coordinator.lastUpdates.camera?.position {
                targetCoordinate = camera.position
            }
            mapView.camera.asMapCamera3d()?
                .setCameraConfig(
                    cameraConfig.mcConfig,
                    durationSeconds: animated ? NSNumber(
                        value: Double(cameraConfig.mcConfig.animationDurationMs) / 1000.0
                    ) : nil,
                    targetZoom: targetZoom.map { NSNumber(value: $0) },
                    targetCoordinate: targetCoordinate?.mcCoord
                )
            coordinator.lastUpdates.cameraConfig = cameraConfig
            coordinator.lastUpdates.camera = camera
        } else if camera.zoom != coordinator.lastUpdates.camera!.zoom && camera.position != coordinator.lastUpdates.camera!.position {
            mapView.camera.move(toCenterPositionZoom: camera.position.mcCoord, zoom: camera.zoom, animated: animated)
            coordinator.lastUpdates.camera!.position = camera.position
            coordinator.lastUpdates.camera!.zoom = camera.zoom
        }
        else if camera.position != coordinator.lastUpdates.camera!.position {
            mapView.camera.move(toCenterPosition: camera.position.mcCoord, animated: animated)
            coordinator.lastUpdates.camera!.position = camera.position
        }
        else if camera.zoom != coordinator.lastUpdates.camera!.zoom {
            mapView.camera.setZoom(camera.zoom, animated: animated)
            coordinator.lastUpdates.camera!.zoom = camera.zoom
        }


        if let restrictedBounds = camera.restrictedBounds {
            mapView.camera.setBounds(restrictedBounds)
            mapView.camera.setBoundsRestrictWholeVisibleRect(true)
        } else {
            mapView.camera.setBoundsRestrictWholeVisibleRect(false)
            mapView.camera.setBounds(mapView.mapInterface.getMapConfig().mapCoordinateSystem.bounds)
        }

    }

    @MainActor
    fileprivate func updateLayers(_ context: MapView.Context, _ mapView: MCMapView) {
        // Get description-structs of layers
        let oldLayers = context.coordinator.currentLayers
        let newLayers = layers.filter { $0?.interface != nil }.compactMap { $0 }

        var needsInsert: [any Layer] = []
        var needsRemoval: [any MCLayerInterface] = []

        // Find layers that...
        for layer in newLayers {
            if oldLayers.first(where: { $0 === layer.interface! }) != nil {
                // ... existed before
            } else {
                // ... are new
                needsInsert.append(layer)
            }
        }

        // ... are not needed any more
        for layer in oldLayers {
            if !newLayers.contains(where: { $0.interface === layer }) {
                needsRemoval.append(layer)
            }
        }

        context.coordinator.currentLayers = newLayers.compactMap { $0.interface }

        // Schedule Task to load or onload layers if needed
        if !needsInsert.isEmpty || !needsRemoval.isEmpty {
            for layer in needsInsert {
                if let interface = layer.interface {
                    layer.beforeAdding?(interface, mapView)
                }
                if let layerIndex = layer.layerIndex {
                    mapView.insert(layer: layer, at: layerIndex)
                } else {
                    mapView.add(layer: layer)
                }
            }
            for layer in needsRemoval {
                mapView.remove(layer: layer)
            }
        }
    }

    public func updateUIView(_ mapView: MapCore.MCMapView, context: Context) {
        updateCamera(mapView, context)
        updateLayers(context, mapView)
    }
}


@available(iOS 17.0, *)
@MainActor
public class MapViewCoordinator {

    weak var mapView: MCMapView?
    let cameraBinding: Binding<Camera>

    let projection: Projection

    init(projection: Projection, camera: Binding<Camera>) {
        self.projection = projection
        self.cameraBinding = camera
    }

    var currentLayers: [any MCLayerInterface] = []

    var ignoreCallbacks = false
    var hasSizeChanged = false

    struct LastUpdates {
        var camera: Camera?
        var cameraConfig: CameraConfig = CameraConfig.basic

        var padding = EdgeInsets()
    }

    var lastUpdates = LastUpdates()

}

@available(iOS 17.0, *)
extension MapViewCoordinator: MCMapCameraListenerInterface {
    public nonisolated func onVisibleBoundsChanged(_ visibleBounds: MCRectCoord, zoom: Double) {

    }

    public nonisolated func onRotationChanged(_ angle: Float) {

    }

    public nonisolated func onMapInteraction() {
        Task { @MainActor in
            guard let mapCamera = self.mapView?.camera else { return }

            let position = Coordinate(mapCamera.getCenterPosition())
            if position != cameraBinding.wrappedValue.position {
                cameraBinding.wrappedValue.position = position
                lastUpdates.camera?.position = position
            }

            let zoom = mapCamera.getZoom()
            if zoom != cameraBinding.wrappedValue.zoom {
                cameraBinding.wrappedValue.zoom = zoom
                lastUpdates.camera?.zoom = zoom
            }

        }
    }

    public nonisolated func onCameraChange(
        _ viewMatrix: [NSNumber],
        projectionMatrix: [NSNumber],
        origin: MCVec3D,
        verticalFov: Float,
        horizontalFov: Float,
        width: Float,
        height: Float,
        focusPointAltitude: Float,
        focusPointPosition: MCCoord,
        zoom: Float
    ) {

    }
}

@available(iOS 17.0, *)
extension MapViewCoordinator: MCMapSizeDelegate {
    public nonisolated
        func sizeChanged()
    {
        Task { @MainActor in
            hasSizeChanged = true
        }
    }
}

open class TouchHandler: MCTouchInterface {

    public weak var mapView: MCMapView?

    public init() {}

    open func onTouchDown(_ posScreen: MCVec2F) -> Bool {
        false
    }

    open func onClickUnconfirmed(_ posScreen: MCVec2F) -> Bool {
        false
    }

    open func onClickConfirmed(_ posScreen: MCVec2F) -> Bool {
        false
    }

    open func onDoubleClick(_ posScreen: MCVec2F) -> Bool {
        false
    }

    open func onMove(_ deltaScreen: MCVec2F, confirmed: Bool, doubleClick: Bool) -> Bool {
        false
    }

    open func onMoveComplete() -> Bool {
        false
    }

    public func onOneFingerDoubleClickMoveComplete() -> Bool {
        false
    }

    open func onTwoFingerClick(_ posScreen1: MCVec2F, posScreen2: MCVec2F) -> Bool {
        false
    }

    open func onTwoFingerMove(_ posScreenOld: [MCVec2F], posScreenNew: [MCVec2F]) -> Bool {
        false
    }

    open func onTwoFingerMoveComplete() -> Bool {
        false
    }

    open func clearTouch() {}

    open func onLongPress(_ posScreen: MCVec2F) -> Bool {
        false
    }
}
