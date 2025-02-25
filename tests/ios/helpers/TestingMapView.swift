//
//  MeasureMapView.swift
//  MapCore
//
//  Created by Nicolas Märki on 21.02.2025.
//

import MapCore
import MetalKit
import UIKit
import os

@available(iOS 15.0, *)
class TestingMapView: MCMapView, @unchecked Sendable, MCMapReadyCallbackInterface {
    private var cont: CheckedContinuation<Void, any Error>?
    private var prepared = false

    // Create a signposter that uses the default subsystem.
    static let signposterSubsystem = "MapCore"
    static let signposterCategory = "Rendering"
    static let signposterIntervalAwait: StaticString = "awaitDrawFrame"
    static let signposterIntervalPrepare: StaticString = "prepareDrawFrame"
    static let signposterIntervalDraw: StaticString = "drawFrame"
    let signposter = OSSignposter(subsystem: TestingMapView.signposterSubsystem, category: TestingMapView.signposterCategory)

    init(
        size: CGSize = .init(width: 1206 / 3, height: 2622 / 3),  // iPhone 16 Pro
        mapConfig: MCMapConfig = MCMapConfig(
            mapCoordinateSystem: MCCoordinateSystemFactory.getEpsg3857System()),
        pixelsPerInch: Float? = nil, is3D: Bool = false,
        _ dataProvider: DataProvider? = nil
    ) {
        super.init(mapConfig: mapConfig, pixelsPerInch: pixelsPerInch, is3D: is3D)
        self.frame = .init(origin: .zero, size: size)
        if let dataProvider = dataProvider {
            self.add(layer: VectorLayer(testingStyleURL: DataProvider.styleJsonPlaceholder, loader: dataProvider))
        }
    }

    @discardableResult
    func add(vectorLayer dataProvider: DataProvider) -> VectorLayer {
        let layer = VectorLayer(testingStyleURL: DataProvider.styleJsonPlaceholder, loader: dataProvider)
        self.add(layer: layer)
        return layer
    }

    func prepare(_ region: TestRegion) async throws {
        self.setNeedsLayout()
        self.layoutIfNeeded()
        mapInterface.setViewportSize(
            MCVec2I(
                x: Int32(self.drawableSize.width),
                y: Int32(self.drawableSize.height)))

        try await awaitReady(bounds: region.mcBounds)
        try await awaitReady(bounds: region.mcBounds)

        for layer in mapInterface.getLayers() {
            layer.enableAnimations(false)
        }
        prepared = true
    }

    private func awaitReady(bounds: MCRectCoord) async throws {
        try await withCheckedThrowingContinuation { cont in
            self.cont = cont
            Task.detached {
                self.mapInterface.drawReadyFrame(
                    bounds,
                    timeout: 60,
                    callbacks: self
                )
            }
        }
        self.draw(in: self)
    }

    func drawMeasured(frames: Int = 120) {
        if !prepared {
            assertionFailure()
            print("Warning: Prepare before calling drawMeasured")
        }
        for _ in 0..<frames {
            self.invalidate()
            self.draw(in: self)
        }
    }

    func measureFPS(duration: TimeInterval = 15.0) -> Double {
        if !prepared {
            assertionFailure()
            print("Warning: Prepare before calling drawMeasured")
        }
        let start = Date()
        var frames = 0.0
        repeat {
            self.invalidate()
            self.draw(in: self)
            frames += 1
        } while -start.timeIntervalSinceNow < duration
        return frames / duration
    }

    func drawCaptured() {
        if !prepared {
            assertionFailure()
            print("Warning: Prepare before calling drawMeasured")
        }
        startCapture()
        self.invalidate()
        self.draw(in: self)
        stopCapture()
    }

    override func awaitFrame() {
        let signpostID = signposter.makeSignpostID()

        let state = signposter.beginInterval(Self.signposterIntervalAwait, id: signpostID)

        super.awaitFrame()

        signposter.endInterval(Self.signposterIntervalAwait, state)

    }

    override func prepareDrawFrame() {

        let signpostID = signposter.makeSignpostID()

        let state = signposter.beginInterval(Self.signposterIntervalPrepare, id: signpostID)

        super.prepareDrawFrame()

        signposter.endInterval(Self.signposterIntervalPrepare, state)
    }

    override func drawFrame(in view: MTKView, completion: @escaping (Bool) -> Void) {
        let signpostID = signposter.makeSignpostID()

        let state = signposter.beginInterval(Self.signposterIntervalDraw, id: signpostID)

        super.drawFrame(in: view) { [signposter] in
            signposter.endInterval(Self.signposterIntervalDraw, state)
            completion($0)
        }

    }

    override func currentDrawableImage() -> UIImage? {
        if !prepared {
            assertionFailure()
            print("Warning: Prepare before calling currentDrawableImage")
        }
        return super.currentDrawableImage()
    }

    func stateDidUpdate(_ state: MCLayerReadyState) {
        switch state {
        case .NOT_READY:
            break
        case .ERROR:
            self.cont?.resume(throwing: MeasureError.error)
            self.cont = nil
        case .TIMEOUT_ERROR:
            self.cont?.resume(throwing: MeasureError.timeout)
            self.cont = nil
        case .READY:
            self.cont?.resume()
            self.cont = nil
        @unknown default:
            self.cont?.resume(throwing: MeasureError.unknown)
            self.cont = nil
        }
    }

    enum MeasureError: Error {
        case error
        case timeout
        case unknown
    }

}
