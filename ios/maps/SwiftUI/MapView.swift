//
//  MapView.swift
//  Meteo
//
//  Created by Nicolas Märki on 01.09.22.
//

import Combine
import CoreLocation
import SwiftUI

@available(iOS 17.0, *)
public struct MapView: UIViewRepresentable {

    @Binding var camera: Camera

    let layers: [(any Layer)?]

    let onCreate: ((MCMapInterface) -> Void)?

    @Environment(\.mapProjection) var projection: Projection
    @Environment(\.mapCameraConfig) var cameraConfig: CameraConfig

    @Environment(\.mapPadding) var padding: EdgeInsets

    @Environment(\.mapGestures) private var gestures

    public init(
        camera: Binding<Camera>,
        layers: [(any Layer)?],
        onCreate: ((MCMapInterface) -> Void)? = nil
    ) {
        self._camera = camera
        self.layers = layers
        self.onCreate = onCreate


    }

    public func makeCoordinator() -> MapViewCoordinator {
        MapViewCoordinator(projection: projection, parent: self)
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
        mapView.mapInterface.getTouchHandler()?.addListener(coordinator.touchHandler)

        coordinator.mapView = mapView

        onCreate?(mapView.mapInterface)

        return mapView
    }

    public func updateUIView(_ mapView: MapCore.MCMapView, context: Context) {
        let coordinator = context.coordinator
        coordinator.updateCamera(camera, cameraConfig: cameraConfig, context: context)
        coordinator.updateLayers(layers, context: context)
        coordinator.touchHandler.gestures = gestures
    }


}




