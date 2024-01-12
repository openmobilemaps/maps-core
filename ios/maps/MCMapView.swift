/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import Foundation
@_exported import MapCoreSharedModule
import MetalKit
import os

open class MCMapView: MTKView {
    public let mapInterface: MCMapInterface
    private let renderingContext: RenderingContext

    private var sizeChanged = false
    private var backgroundDisable = false
    private var saveDrawable = false
    private lazy var renderToImageQueue = DispatchQueue(label: "io.openmobilemaps.renderToImagQueue", qos: .userInteractive)

    private var framesToRender: UInt = 1
    private let framesToRenderAfterInvalidate: UInt = 25

    private let touchHandler: MCMapViewTouchHandler
    private let callbackHandler = MCMapViewCallbackHandler()

    public weak var sizeDelegate: MCMapSizeDelegate?

    public init(mapConfig: MCMapConfig, pixelsPerInch: Float? = nil) {
        let renderingContext = RenderingContext()
        guard let mapInterface = MCMapInterface.create(GraphicsFactory(),
                                                       shaderFactory: ShaderFactory(),
                                                       renderingContext: renderingContext,
                                                       mapConfig: mapConfig,
                                                       scheduler: MCThreadPoolScheduler.create(),
                                                       pixelDensity: pixelsPerInch ?? Float(UIScreen.pixelsPerInch)) else {
            fatalError("Can't create MCMapInterface")
        }
        self.mapInterface = mapInterface
        self.renderingContext = renderingContext
        touchHandler = MCMapViewTouchHandler(touchHandler: mapInterface.getTouchHandler())
        super.init(frame: .zero, device: MetalContext.current.device)
        setup()
    }

    @available(*, unavailable)
    public required init(coder _: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    deinit {
        if Thread.isMainThread {
            // make sure the mapInterface is destroyed from a background thread
            DispatchQueue.global().async { [mapInterface] in
                mapInterface.destroy()
            }
        } else {
            mapInterface.destroy()
        }
    }

    private func setup() {
        renderingContext.sceneView = self

        device = MetalContext.current.device

        colorPixelFormat = MetalContext.current.colorPixelFormat
        framebufferOnly = false

        delegate = self

        depthStencilPixelFormat = .stencil8

        // if #available(iOS 16.0, *) {
        //   depthStencilStorageMode = .private
        // }

        isMultipleTouchEnabled = true

        preferredFramesPerSecond = 120

        sampleCount = 1 // samples per pixel

        callbackHandler.invalidateCallback = { [weak self] in
            self?.invalidate()
        }
        mapInterface.setCallbackHandler(callbackHandler)

        touchHandler.mapView = self

        mapInterface.resume()

        addEventListeners()
    }

    private func addEventListeners() {
        NotificationCenter.default.addObserver(self,
                                               selector: #selector(willEnterForeground),
                                               name: UIApplication.willEnterForegroundNotification,
                                               object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(didEnterBackground),
                                               name: UIApplication.didEnterBackgroundNotification,
                                               object: nil)
    }

    @objc func willEnterForeground() {
        backgroundDisable = false
        invalidate()
    }

    @objc func didEnterBackground() {
        backgroundDisable = true
    }

    /// The view’s background color.
    override public var backgroundColor: UIColor? {
        get {
            super.backgroundColor
        }
        set {
            super.backgroundColor = newValue
            mapInterface.setBackgroundColor(newValue?.mapCoreColor ?? UIColor.clear.mapCoreColor)
            isOpaque = newValue?.isOpaque ?? false
        }
    }

    public func invalidate() {
        isPaused = false
        framesToRender = framesToRenderAfterInvalidate
    }
}

extension MCMapView: MTKViewDelegate {
    open func mtkView(_: MTKView, drawableSizeWillChange _: CGSize) {
        sizeChanged = true
        invalidate()
    }

    public func draw(in view: MTKView) {
        guard !backgroundDisable else {
            return // don't execute metal calls in background
        }

        guard framesToRender != 0 else {
            isPaused = true
            return
        }

        framesToRender -= 1

        guard let renderPassDescriptor = view.currentRenderPassDescriptor,
              let commandBuffer = MetalContext.current.commandQueue.makeCommandBuffer(),
              let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor)
        else {
            return
        }

        renderingContext.encoder = renderEncoder

        // Shared lib stuff
        if sizeChanged {
            mapInterface.setViewportSize(view.drawableSize.vec2)
            sizeDelegate?.sizeChanged()
            sizeChanged = false
        }

        mapInterface.drawFrame()

        renderEncoder.endEncoding()

        guard let drawable = view.currentDrawable else {
            return
        }

        // if we want to save the drawable (offscreen rendering), we commit and wait synchronously
        // until the command buffer completes, also we don't present it
        if self.saveDrawable {
            commandBuffer.commit()
            commandBuffer.waitUntilCompleted()
        } else {
            commandBuffer.present(drawable)
            commandBuffer.commit()
        }
    }

    public func renderToImage(size: CGSize, timeout: Float, bounds: MCRectCoord, callback: @escaping (UIImage?, MCLayerReadyState) -> Void) {
        renderToImageQueue.async {
            self.frame = CGRect(origin: .zero, size: .init(width: size.width / UIScreen.main.scale, height: size.height / UIScreen.main.scale))
            self.setNeedsLayout()
            self.layoutIfNeeded()

            let mapReadyCallbacks = MCMapViewMapReadyCallbacks()
            mapReadyCallbacks.delegate = self
            mapReadyCallbacks.callback = callback

            self.mapInterface.drawReadyFrame(bounds, timeout: timeout, callbacks: mapReadyCallbacks)
        }
    }
}

private extension MCMapView {
    func currentDrawableImage() -> UIImage? {
        self.saveDrawable = true
        self.invalidate()
        self.draw(in: self)
        self.saveDrawable = false

        guard let texture = self.currentDrawable?.texture else { return nil }

        let context = CIContext()
        let kciOptions: [CIImageOption: Any] = [.colorSpace: CGColorSpaceCreateDeviceRGB()]
        let cImg = CIImage(mtlTexture: texture, options: kciOptions)!
        return context.createCGImage(cImg, from: cImg.extent)?.toImage()
    }
}

private extension CGImage {
    func toImage() -> UIImage? {
        let w = Double(width)
        let h = Double(height)
        UIGraphicsBeginImageContext(CGSize(width: w, height: h))
        let context = UIGraphicsGetCurrentContext()
        context?.draw(self, in: CGRect(x: 0, y: 0, width: w, height: h))

        let newImage = UIGraphicsGetImageFromCurrentImageContext()
        UIGraphicsEndImageContext()

        return newImage
    }
}

extension CGSize {
    var vec2: MCVec2I {
        MCVec2I(x: Int32(width),
                y: Int32(height))
    }
}

public extension MCMapView {
    override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesBegan(touches, with: event)
        touchHandler.touchesBegan(touches, with: event)
    }

    override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesEnded(touches, with: event)
        touchHandler.touchesEnded(touches, with: event)
    }

    override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesCancelled(touches, with: event)
        touchHandler.touchesCancelled(touches, with: event)
    }

    override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent?) {
        super.touchesMoved(touches, with: event)
        touchHandler.touchesMoved(touches, with: event)
    }
}

public extension MCMapView {
    var camera: MCMapCamera2dInterface {
        mapInterface.getCamera()!
    }

    func add(layer: MCLayerInterface?) {
        mapInterface.addLayer(layer)
    }

    func insert(layer: MCLayerInterface?, at index: Int) {
        mapInterface.insertLayer(at: layer, at: Int32(index))
    }

    func insert(layer: MCLayerInterface?, above: MCLayerInterface?) {
        mapInterface.insertLayer(above: layer, above: above)
    }

    func insert(layer: MCLayerInterface?, below: MCLayerInterface?) {
        mapInterface.insertLayer(below: layer, below: below)
    }

    func remove(layer: MCLayerInterface?) {
        mapInterface.removeLayer(layer)
    }
}

private class MCMapViewMapReadyCallbacks: MCMapReadyCallbackInterface {
    public weak var delegate: MCMapView?
    public var callback: ((UIImage?, MCLayerReadyState) -> Void)?

    func stateDidUpdate(_ state: MCLayerReadyState) {
        guard let delegate = self.delegate else { return }

        delegate.draw(in: delegate)

        DispatchQueue.main.async {
            switch state {
                case .NOT_READY:
                    break
                case .ERROR, .TIMEOUT_ERROR:
                    self.callback?(nil, state)
                case .READY:
                    self.callback?(delegate.currentDrawableImage(), state)
                @unknown default:
                    break
            }
        }
    }
}
