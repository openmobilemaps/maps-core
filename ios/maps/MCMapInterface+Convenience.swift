//
//  MCMapInterface+Convenience.swift
//
//
//  Created by OpenAI Codex on 05.05.2026.
//

import Foundation

public extension MCMapInterface {
    var mapConfig: MCMapConfig {
        getMapConfig()
    }

    var coordinateConverterHelper: MCCoordinateConversionHelperInterface {
        guard let helper = getCoordinateConverterHelper() else {
            fatalError("MCMapInterface.getCoordinateConverterHelper() unexpectedly returned nil")
        }
        return helper
    }

    var camera: MCMapCameraInterface {
        guard let camera = getCamera() else {
            fatalError("MCMapInterface.getCamera() unexpectedly returned nil")
        }
        return camera
    }

    var camera3d: MCMapCamera3dInterface? {
        camera.asMapCamera3d()
    }

    var touchHandler: MCTouchHandlerInterface {
        guard let touchHandler = getTouchHandler() else {
            fatalError("MCMapInterface.getTouchHandler() unexpectedly returned nil")
        }
        return touchHandler
    }

    var performanceLoggers: [any MCPerformanceLoggerInterface] {
        get { getPerformanceLoggers() }
        set { setPerformanceLoggers(newValue) }
    }

    var layers: [any MCLayerInterface] {
        getLayers()
    }

    var layersIndexed: [any MCIndexedLayerInterface] {
        getLayersIndexed()
    }

    var needsCompute: Bool {
        getNeedsCompute()
    }

    var is3D: Bool {
        is3d()
    }
}
