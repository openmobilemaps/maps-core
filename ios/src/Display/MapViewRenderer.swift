//
//  UBMapViewRenderer.swift
//  SwisstopoShared
//
//  Created by Joseph El Mallah on 06.02.20.
//  Copyright © 2020 Ubique Innovation AG. All rights reserved.
//
/*
import MetalKit

class UBMapViewRenderer: NSObject {
    public private(set) var sharedLibRenderer: UBMMapViewRenderer?
    private let graphicObjectFactory = UBMGraphicObjectFactoryImpl()
    private let shaderFactory = UBMShaderObjectFactoryImpl()
    private let renderingContext = UBMRenderingContextImpl()
    private var sizeChanged = false
    weak var mapDelegate: UBMapViewDelegate?
    weak var mapView: UBMapView? {
        didSet { renderingContext.mapView = mapView }
    }

    private let inactivityDuration: TimeInterval = 1.0
    private var lastInvalidateDate = Date()

    private var backgroundDisable = false

    private var currentSessionId = UUID()

    init(mapConfig: UBMMapConfig) {
        super.init()

        do {
            guard let renderer = UBMMapViewRenderer.create(self, graphicObjectFactory: graphicObjectFactory, shaderFactory: shaderFactory, renderingContext: renderingContext, mapConfig: mapConfig, density: Float(UIScreen.main.nativeScale))
            else {
                throw MetalError.Fundamental("Cannot create shared Map Renderer")
            }

            renderer.onSurfaceCreated()
            sharedLibRenderer = renderer

            addEventListeners()

        } catch {
            fatalError(error.localizedDescription)
        }
    }

    deinit {
        sharedLibRenderer?.doPause()
    }

    private func start() {
        let renderer = sharedLibRenderer!

        renderer.setResumed()

        let session = UUID()
        currentSessionId = session

        Thread.detachNewThread {
            // Name thread to make debugging easier
            Thread.current.name = "ch.ubique.mapengine.timer-thread"

            var resumed = true

            while resumed {
                autoreleasepool {
                    resumed = renderer.runTimerThread()
                }
                if self.currentSessionId != session {
                    return
                }
            }
        }

        for _ in 0 ..< 8 {
            Thread.detachNewThread {
                // Name thread to make debugging easier
                Thread.current.name = "ch.ubique.mapengine.loader-thread"

                var resumed = true

                while resumed {
                    autoreleasepool {
                        resumed = renderer.runLoaderThread()
                    }
                    if self.currentSessionId != session {
                        return
                    }
                }
            }
        }

        invalidate()
    }

    private func addEventListeners() {
        NotificationCenter.default.addObserver(self, selector: #selector(willEnterForeground), name: UIApplication.willEnterForegroundNotification, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(didEnterBackground), name: UIApplication.didEnterBackgroundNotification, object: nil)
    }

    @objc func willEnterForeground() {
        backgroundDisable = false
        start()
    }

    @objc func didEnterBackground() {
        backgroundDisable = true
        sharedLibRenderer?.doPause()
    }
}

extension UBMapViewRenderer: UBMMapViewRendererDelegate {
    func invalidate() {
        lastInvalidateDate = Date()
        mapView?.isPaused = false
    }

    func loadImage(toTexture url: String, xCoord x: Int32, yCoord y: Int32, zoom: Int32, overlayName: String) -> UBMTextureHolder? {
        guard let data = mapDelegate?.loadImage(baseURL: url, xCoord: Int(x), yCoord: Int(y), zoomLevel: Int(zoom), overlayName: overlayName) else {
            return nil
        }
        var img: UBMTextureHolderImpl?
        DispatchQueue.main.sync {
            img = try? UBMTextureHolderImpl(data)
        }
        return img
    }

    func loadVector(_ fileName: String, xCoord: Int32, yCoord: Int32, zoom: Int32, overlayName: String) -> UBMVectorHolder? {
        guard let data = mapDelegate?.loadVector(baseURL: fileName, xCoord: Int(xCoord), yCoord: Int(yCoord), zoomLevel: Int(zoom), overlayName: overlayName) else {
            return nil
        }
        return UBMVectorHolderImpl(data: data)
    }

    func bindMainDrawable() {}

    func setRulerWidth(_ width: Float, widthInMetres meters: Float) {
        mapDelegate?.setRulerWidth(width: width, widthInMetres: meters)
    }

    func longPressed(_ mapCoordinate: UBMMapCoordinate, radiusInMapUnits: Int32) {
        mapDelegate?.longPressed(coordinate: mapCoordinate, radiusInMapUnits: radiusInMapUnits)
    }

    func didRotate(_ angle: Double) {
        mapView?.didRotate(to: angle)
    }

    func didTap(_ mapCoordinate: UBMMapCoordinate) {
        mapDelegate?.didTap(coordinate: mapCoordinate)
    }
}

extension UBMapViewRenderer: MTKViewDelegate {
    func mtkView(_: MTKView, drawableSizeWillChange _: CGSize) {
        sizeChanged = true
        mapView?.isPaused = false
    }

    func draw(in view: MTKView) {
        guard !backgroundDisable else {
            return // don't execute metal calls in background
        }

        guard let renderPassDescriptor = view.currentRenderPassDescriptor,
              let commandBuffer = MetalContext.current.commandQueue.makeCommandBuffer(),
              let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor) else {
            return
        }

        renderingContext.encoder = renderEncoder

        // Shared lib stuff
        if sizeChanged {
            sharedLibRenderer?.onSurfaceChanged(Int32(view.drawableSize.width), height: Int32(view.drawableSize.height))
            sizeChanged = false
        }

        sharedLibRenderer?.onDrawFrame()

        renderEncoder.endEncoding()

        guard let drawable = view.currentDrawable else {
            return
        }

        commandBuffer.present(drawable)
        commandBuffer.commit()

        if abs(lastInvalidateDate.timeIntervalSinceNow) > inactivityDuration {
            mapView?.isPaused = true
        }
    }
}
*/
