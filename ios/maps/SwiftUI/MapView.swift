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

    public struct Camera: Equatable {
        public static let basicCamera3dConfig = MCCamera3dConfigFactory.getBasicConfig()

        public var center: Updatable<MCCoord>
        public var zoom: Updatable<Double>
        public var minZoom: Updatable<Double> = .init(mode: .map, value: nil)
        public var maxZoom: Updatable<Double> = .init(mode: .map, value: nil)

        public var cameraConfig: MCCamera3dConfig = Self.basicCamera3dConfig

        public var restrictedBounds: MCRectCoord? = nil

        public var camera3d: MCMapCameraInterface?

        public var mapConfig: MCMapConfig?

        public init(center: Updatable<MCCoord> = .init(),
                    zoom: Updatable<Double> = .init(),
                    minZoom: Updatable<Double> = .init(),
                    maxZoom: Updatable<Double> = .init(),
                    // visibleRect: Updatable<MCRectCoord> = .init(),
                    cameraConfig: MCCamera3dConfig = Self.basicCamera3dConfig,
                    restrictedBounds: MCRectCoord? = nil,
                    camera3d: MCMapCameraInterface? = nil,
                    mapConfig: MCMapConfig? = nil
        ) {
            self.center = center
            self.zoom = zoom
            self.minZoom = minZoom
            self.maxZoom = maxZoom
            // self.visibleRect = visibleRect
            self.cameraConfig = cameraConfig
            self.restrictedBounds = restrictedBounds
            self.camera3d = camera3d
            self.mapConfig = mapConfig
        }

        public init(location: CLLocationCoordinate2D,
                    zoom: Double?,
                    cameraConfig: MCCamera3dConfig = Self.basicCamera3dConfig) {
            self.center = .init(mode: .user, value: MCCoord(systemIdentifier: MCCoordinateSystemIdentifiers.epsg4326(), x: location.longitude, y: location.latitude, z: 0))
            self.zoom = .init(mode: .user, value: zoom)
            // self.visibleRect = .init()
            self.cameraConfig = cameraConfig
        }

        public init(latitude: Double,
                    longitude: Double,
                    zoom: Double?,
                    cameraConfig: MCCamera3dConfig = Self.basicCamera3dConfig) {
            self.center = .init(mode: .user, value: MCCoord(systemIdentifier: MCCoordinateSystemIdentifiers.epsg4326(), x: longitude, y: latitude, z: 0))
            self.zoom = .init(mode: .user, value: zoom)
            // self.visibleRect = .init()
            self.cameraConfig = cameraConfig
        }

        public init(restrictedBounds: MCRectCoord,
                    cameraConfig: MCCamera3dConfig = Self.basicCamera3dConfig) {
            self.center = .init()
            self.zoom = .init()
            // self.visibleRect = .init()
            self.restrictedBounds = restrictedBounds
            self.cameraConfig = cameraConfig
        }

        public var centerCoordinate: MCCoord? {
            center.value
        }

        public static func == (lhs: Camera, rhs: Camera) -> Bool {
            lhs.zoom == rhs.zoom
            //&& lhs.visibleRect == rhs.visibleRect
            && lhs.center == rhs.center
            && lhs.cameraConfig.key == rhs.cameraConfig.key
            && lhs.minZoom == rhs.minZoom
            && lhs.maxZoom == rhs.maxZoom
            && lhs.restrictedBounds == rhs.restrictedBounds
        }
    }

    let is3D: Bool

    let mapConfig: MCMapConfig

    let layers: [(any Layer)?]

    @Binding var camera: Camera

    var paddingInset: EdgeInsets?

    let touchHandler: TouchHandler?

    public init(camera: Binding<Camera>,
                mapConfig: MCMapConfig = MCMapConfig(mapCoordinateSystem: MCCoordinateSystemFactory.getEpsg3857System()),
                paddingInset: EdgeInsets? = nil,
                layers: [(any Layer)?],
                is3D: Bool = false,
                touchHandler: TouchHandler? = nil) {
        self.layers = layers
        self.mapConfig = mapConfig
        self._camera = camera
        self.paddingInset = paddingInset
        self.is3D = is3D
        self.touchHandler = touchHandler
    }

    public func makeUIView(context: Context) -> MCMapView {
        // reset camera values to user if the camera gets reused
        if camera.center.value != nil,
           camera.center.mode == .map {
            camera = MapView.Camera(center: .init(mode: .user, value: camera.center.value),
                                    zoom: .init(mode: .user, value: camera.zoom.value),
                                    minZoom: .init(mode: .user, value: camera.minZoom.value),
                                    maxZoom: .init(mode: .user, value: camera.maxZoom.value),
                                    //visibleRect: camera.visibleRect,
                                    cameraConfig: camera.cameraConfig,
                                    restrictedBounds: camera.restrictedBounds)
        }

        let mapView = MCMapView(mapConfig: mapConfig, is3D: is3D)
        mapView.backgroundColor = .clear
        mapView.camera.addListener(context.coordinator)
        mapView.camera.setRotationEnabled(false)
        if is3D {
            mapView.camera.asMapCamera3d()?.setCameraConfig(camera.cameraConfig, durationSeconds: nil, targetZoom: nil, targetCoordinate: nil)
        }
        mapView.sizeDelegate = context.coordinator
        context.coordinator.mapView = mapView
        if let touchHandler {
            mapView.mapInterface.getTouchHandler()?.addListener(touchHandler)
            touchHandler.mapView = mapView
        }

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

        if let paddingInset {
            let scale = Float(UIScreen.main.nativeScale)
            mapView.camera.setPaddingTop(scale * Float(paddingInset.top))
            mapView.camera.setPaddingBottom(scale * Float(paddingInset.bottom))
            mapView.camera.setPaddingLeft(scale * Float(paddingInset.leading))
            mapView.camera.setPaddingRight(scale * Float(paddingInset.trailing))
        }

        coordinator.ignoreCallbacks = true
        var animated = camera.center.animated
        if coordinator.lastWrittenCamera == nil {
            animated = false
        }

        if is3D, coordinator.lastWrittenCamera?.cameraConfig.key != camera.cameraConfig.key {
            mapView.camera.asMapCamera3d()?.setCameraConfig(camera.cameraConfig, durationSeconds: nil, targetZoom: nil, targetCoordinate: nil)
        }

        if let center = camera.center.value, let zoom = camera.zoom.value, camera.center.mode == .user, camera.zoom.mode == .user {
            mapView.camera.move(toCenterPositionZoom: center, zoom: zoom, animated: animated)
        } else if let center = camera.center.value, camera.center.mode == .user {
            mapView.camera.move(toCenterPosition: center, animated: animated)
        } else if let zoom = camera.zoom.value, camera.zoom.mode == .user {
            mapView.camera.setZoom(zoom, animated: animated)
        }

        if let maxZoom = camera.maxZoom.value, camera.maxZoom.mode == .user {
            mapView.camera.setMaxZoom(maxZoom)
        }
        if let minZoom = camera.minZoom.value, camera.minZoom.mode == .user {
            mapView.camera.setMinZoom(minZoom)
        }

        if let zoom = camera.zoom.value, camera.zoom.mode == .user {
            mapView.camera.setZoom(zoom, animated: camera.zoom.animated)
        }

        if let restrictedBounds = camera.restrictedBounds {
            mapView.camera.setBounds(restrictedBounds)
            mapView.camera.setBoundsRestrictWholeVisibleRect(true)
        } else {
            mapView.camera.setBoundsRestrictWholeVisibleRect(false)
            mapView.camera.setBounds(mapView.mapInterface.getMapConfig().mapCoordinateSystem.bounds)
        }

        coordinator.ignoreCallbacks = false
        coordinator.lastWrittenCamera = camera
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
            if let _ = oldLayers.first(where: { $0 === layer.interface! }) {
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
public class MapViewCoordinator: MCMapCameraListenerInterface {
    var parent: MapView
    init(parent: MapView) {
        self.parent = parent
    }

    weak var mapView: MCMapView?

    var currentLayers: [any MCLayerInterface] = []

    var ignoreCallbacks = false
    var hasSizeChanged = false
    var lastWrittenCamera: MapView.Camera?

    nonisolated
    private func updateCamera() {
        Task { @MainActor in
            guard !ignoreCallbacks else {
                return
            }

            guard mapView?.bounds.size != .zero, hasSizeChanged
            else {
                return
            }

            guard lastWrittenCamera != nil else {
                if let mapView {
                    parent.updateCamera(mapView, self)
                }
                return
            }

            let center = mapView?.camera.getCenterPosition()
            let zoom = mapView?.camera.getZoom()
            let minZoom = mapView?.camera.getMinZoom()
            let maxZoom = mapView?.camera.getMaxZoom()
            let cameraConfig = mapView?.camera.asMapCamera3d()?.getCameraConfig()


            parent.camera = MapView.Camera(
                center: .init(mode: .map, value: center),
                zoom: .init(mode: .map, value: zoom),
                minZoom: .init(mode: .map, value: minZoom),
                maxZoom: .init(mode: .map, value: maxZoom),
                // visibleRect: .init(mode: .map, value: mapView?.camera.getPaddingAdjustedVisibleRect()),
                cameraConfig: cameraConfig ?? parent.camera.cameraConfig,
                restrictedBounds: parent.camera.restrictedBounds,
                camera3d: mapView?.camera,
                mapConfig: mapView?.mapInterface.getMapConfig()
            )

            lastWrittenCamera = parent.camera
        }
    }

    nonisolated
    public func onVisibleBoundsChanged(_ visibleBounds: MCRectCoord, zoom: Double) {
        updateCamera()
    }

    nonisolated
    public func onCameraChange(
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
            updateCamera()
    }


    nonisolated
    public func onRotationChanged(_ angle: Float) {
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
