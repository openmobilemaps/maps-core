//
//  MapViewCoordinator.swift
//  MapCore
//
//  Created by Nicolas Märki on 20.08.2025.
//

import SwiftUI

@available(iOS 17.0, *)
@MainActor
public class MapViewCoordinator {

    weak var mapView: MCMapView?
    let parent: MapView

    let projection: Projection

    let touchHandler = TouchHandler()

    init(projection: Projection, parent: MapView) {
        self.projection = projection
        self.parent = parent
    }

    var currentLayers: [any MCLayerInterface] = []

    struct LastUpdates {
        var camera: Camera?
        var cameraConfig: CameraConfig = CameraConfig.basic

        var padding = EdgeInsets()
    }

    var lastUpdates = LastUpdates()

    
    func updatePadding(_ padding: EdgeInsets) {
        guard let mapView = mapView else { return }

        if padding != lastUpdates.padding {
            let scale = Float(UIScreen.main.nativeScale)
            mapView.camera.setPaddingTop(scale * Float(padding.top))
            mapView.camera.setPaddingBottom(scale * Float(padding.bottom))
            mapView.camera.setPaddingLeft(scale * Float(padding.leading))
            mapView.camera.setPaddingRight(scale * Float(padding.trailing))
            lastUpdates.padding = padding
        }
    }

    func updateCamera(_ camera: Camera, cameraConfig: CameraConfig, context: MapView.Context?) {
        guard let mapView = mapView else { return }

        var animated = context?.transaction.animation != nil

        if lastUpdates.camera == nil {
            animated = false
        }

        if lastUpdates.camera == nil {
            if let minZoom = camera.minZoom {
                mapView.camera.setMinZoom(minZoom)
            }
            if let maxZoom = camera.maxZoom {
                mapView.camera.setMinZoom(maxZoom)
            }
            mapView.camera.move(toCenterPositionZoom: MCCoord(camera.position), zoom: camera.zoom, animated: false)
            lastUpdates.camera = camera
        }

        if let maxZoom = camera.maxZoom, lastUpdates.camera!.maxZoom != maxZoom {
            mapView.camera.setMaxZoom(maxZoom)
        }
        if let minZoom = camera.minZoom, lastUpdates.camera!.minZoom != minZoom {
            mapView.camera.setMaxZoom(minZoom)
        }

        if projection.is3D, lastUpdates.cameraConfig != cameraConfig {
            var targetZoom: Double?
            if camera.zoom != lastUpdates.camera?.zoom {
                targetZoom = camera.zoom
            }
            var targetCoordinate: Coordinate?
            if camera.position != lastUpdates.camera?.position {
                targetCoordinate = camera.position
            }
            mapView.camera.asMapCamera3d()?
                .setCameraConfig(
                    cameraConfig.mcConfig,
                    durationSeconds: animated
                    ? NSNumber(
                        value: Double(cameraConfig.mcConfig.animationDurationMs) / 1000.0
                    ) : nil,
                    targetZoom: targetZoom.map { NSNumber(value: $0) },
                    targetCoordinate: targetCoordinate.map { MCCoord($0) }
                )
            lastUpdates.cameraConfig = cameraConfig
            lastUpdates.camera = camera
        } else if camera.zoom != lastUpdates.camera!.zoom && camera.position != lastUpdates.camera!.position {
            mapView.camera.move(toCenterPositionZoom: MCCoord(camera.position), zoom: camera.zoom, animated: animated)
            lastUpdates.camera!.position = camera.position
            lastUpdates.camera!.zoom = camera.zoom
        } else if camera.position != lastUpdates.camera!.position {
            mapView.camera.move(toCenterPosition: MCCoord(camera.position), animated: animated)
            lastUpdates.camera!.position = camera.position
        } else if camera.zoom != lastUpdates.camera!.zoom {
            mapView.camera.setZoom(camera.zoom, animated: animated)
            lastUpdates.camera!.zoom = camera.zoom
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
    func updateLayers(_ layers: [(any Layer)?], context: MapView.Context?) {
        guard let mapView = mapView else { return }
        
        // Get description-structs of layers
        let oldLayers = currentLayers
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

        currentLayers = newLayers.compactMap { $0.interface }

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

}


@available(iOS 17.0, *)
extension MapViewCoordinator: MCMapCameraListenerInterface {
    public nonisolated func onVisibleBoundsChanged(_ visibleBounds: MCRectCoord, zoom: Double) {

    }

    public nonisolated func onRotationChanged(_ angle: Float) {

    }

    public nonisolated func onMapInteraction() {
        if Thread.isMainThread {
            MainActor.assumeIsolated {
                guard let mapCamera = self.mapView?.camera else { return }

                let position = Coordinate(mapCamera.getCenterPosition())
                if position != parent.camera.position {
                    parent.camera.position = position
                    lastUpdates.camera?.position = position
                }

                let zoom = mapCamera.getZoom()
                if zoom != parent.camera.zoom {
                    parent.camera.zoom = zoom
                    lastUpdates.camera?.zoom = zoom
                }
            }
        } else {
            DispatchQueue.main.sync {
                onMapInteraction()
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
//        if Thread.isMainThread {
//            MainActor.assumeIsolated {
//                guard let camera = lastUpdates.camera else { return }
//                self.updateCamera(camera, cameraConfig: lastUpdates.cameraConfig, context: nil)
//            }
//        } else {
//            DispatchQueue.main.sync {
//                sizeChanged()
//            }
//        }
    }
}
