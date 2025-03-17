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
@_exported @preconcurrency import MapCoreSharedModule
import MetalKit
import os

open class MCMapView: UIView, @unchecked Sendable {
    public nonisolated(unsafe) let mapInterface: MCMapInterface
    private let renderingContext: RenderingContext

    public var clearColor: MTLClearColor = .init(red: 0, green: 0, blue: 0, alpha: 1)

    private var backgroundDisable = false

    public var pausesAutomatically = true
    private var framesToRender: Int = 1
    private let framesToRenderAfterInvalidate: Int = 1
    private var lastInvalidate = Date()
    private let renderAfterInvalidate: TimeInterval = 0  // Collision detection might be delayed 3s

    private var renderSemaphore = DispatchSemaphore(value: 3)  // using triple buffers

    private let touchHandler: MCMapViewTouchHandler
    private let callbackHandler = MCMapViewCallbackHandler()

    private var pinch: UIPinchGestureRecognizer!
    private var pan: UIPanGestureRecognizer!

    public weak var sizeDelegate: MCMapSizeDelegate?

    public var renderTargetTextures: [RenderTargetTexture] = []

    private var metalLayer: CAMetalLayer!

    public init(
        mapConfig: MCMapConfig = MCMapConfig(
            mapCoordinateSystem: MCCoordinateSystemFactory.getEpsg3857System()),
        pixelsPerInch: Float? = nil, is3D: Bool = false
    ) {

        let renderingContext = RenderingContext()
        guard
            let mapInterface = MCMapInterface.create(
                GraphicsFactory(),
                shaderFactory: ShaderFactory(),
                renderingContext: renderingContext,
                mapConfig: mapConfig,
                scheduler: MCThreadPoolScheduler.create(),
                pixelDensity: pixelsPerInch ?? Float(UIScreen.pixelsPerInch),
                is3D: is3D)
        else {
            fatalError("Can't create MCMapInterface")
        }
        self.mapInterface = mapInterface
        self.renderingContext = renderingContext
        touchHandler = MCMapViewTouchHandler(
            touchHandler: mapInterface.getTouchHandler())
        super.init(frame: .zero)
        self.metalLayer = self.layer as? CAMetalLayer
        self.pinch = UIPinchGestureRecognizer(
            target: self, action: #selector(pinched))
        self.pan = UIPanGestureRecognizer(
            target: self, action: #selector(panned))
        setup()
        startRenderLoop()
    }

    open override class var layerClass: AnyClass {
        CAMetalLayer.classForCoder()
    }

    @available(*, unavailable)
    public required init(coder _: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    deinit {
        if Thread.isMainThread {
            // make sure the mapInterface is destroyed from a background thread
            DispatchQueue.global()
                .async { [mapInterface] in
                    mapInterface.destroy()
                }
        } else {
            mapInterface.destroy()
        }
    }

    private func setup() {
        metalLayer.device = MetalContext.current.device
        metalLayer.pixelFormat = MetalContext.current.colorPixelFormat
        metalLayer.isOpaque = true

        renderingContext.sceneView = self

        isMultipleTouchEnabled = true

        callbackHandler.invalidateCallback = { [weak self] in
            self?.invalidate()
        }
        mapInterface.setCallbackHandler(callbackHandler)

        touchHandler.mapView = self

        mapInterface.resume()

        addEventListeners()

        setupMacGestureRecognizersIfNeeded()
    }

    //    private func setup() {

    //    }

    private func addEventListeners() {
        NotificationCenter.default.addObserver(
            self,
            selector: #selector(willEnterForeground),
            name: UIApplication.willEnterForegroundNotification,
            object: nil)
        NotificationCenter.default.addObserver(
            self, selector: #selector(didEnterBackground),
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

    open override func willMove(toWindow newWindow: UIWindow?) {
        super.willMove(toWindow: newWindow)
        if newWindow == nil {
            touchHandler.cancelAllTouches()
        }
    }

    /// The view’s background color.
    override public var backgroundColor: UIColor? {
        get {
            super.backgroundColor
        }
        set {
            super.backgroundColor = newValue
            mapInterface.setBackgroundColor(
                newValue?.mapCoreColor ?? UIColor.clear.mapCoreColor)
            isOpaque = newValue?.isOpaque ?? false
            layer.isOpaque = isOpaque
        }
    }

    public func invalidate() {
        framesToRender = framesToRenderAfterInvalidate
        lastInvalidate = Date()
        nextFrameSemaphore.signal()
    }

    public func renderTarget(named name: String) -> RenderTargetTexture {
        for target in renderTargetTextures {
            if target.name == name {
                return target
            }
        }
        let newTarget = RenderTargetTexture(name: name)
        renderTargetTextures.append(newTarget)
        return newTarget
    }

    private func startRenderLoop() {
        Thread.detachNewThread {
            Thread.current.name = "MapSDK_rendering"
            Thread.current.threadPriority = 45 // recommended priority for rendering
            while true {
                autoreleasepool {
                    self.blockUntilNextFrame()
                    self.renderFrame()
                }
            }
        }
    }

    var lastSize: CGSize?
    var nextFrameSemaphore = DispatchSemaphore(value: 0)

    private func blockUntilNextFrame() {
        guard
            (framesToRender > 0
             || -lastInvalidate.timeIntervalSinceNow < renderAfterInvalidate || !pausesAutomatically) && !backgroundDisable
        else {
            nextFrameSemaphore.wait()
            return
        }

        if pausesAutomatically {
            framesToRender -= 1
        }
    }


    open override func layoutSubviews() {
        super.layoutSubviews()
        metalLayer.bounds = bounds
        metalLayer.frame = bounds

        let scale = window?.screen.scale ?? UIScreen.main.scale
        lastSize = CGSize(
            width: bounds.width * scale,
            height: bounds.height * scale)
        contentScaleFactor = scale

        invalidate()

    }

    private var stencilTextures: [MTLTexture] = []
    private var currentStencilIndex = 0

    private func setupStencilTextures() {
        stencilTextures.removeAll()
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .stencil8,
            width: Int(metalLayer.drawableSize.width),
            height: Int(metalLayer.drawableSize.height),
            mipmapped: false
        )
        descriptor.usage = [.renderTarget]
        descriptor.storageMode = .memoryless

        for _ in 0..<3 {
            if let texture = MetalContext.current.device.makeTexture(descriptor: descriptor) {
                stencilTextures.append(texture)
            }
        }
    }

    private func renderFrame() {

        // Ensure that triple-buffers are not over-used
        renderSemaphore.wait()

        renderingContext.beginFrame()
        mapInterface.prepare()

        // Shared lib stuff

        if let size = lastSize {
            metalLayer.drawableSize = size
            mapInterface.setViewportSize(size.vec2)
            sizeDelegate?.sizeChanged()
            lastSize = nil

            setupStencilTextures()
        }

        guard
            let commandBuffer = MetalContext.current.commandQueue
                .makeCommandBuffer()
        else {
            self.renderSemaphore.signal()
            return
        }

        for offscreenTarget in renderTargetTextures {
            let renderEncoder = offscreenTarget.prepareOffscreenEncoder(
                commandBuffer,
                size: metalLayer.drawableSize.vec2,
                context: renderingContext
            )!
            renderingContext.encoder = renderEncoder
            renderingContext.renderTarget = offscreenTarget
            mapInterface.drawOffscreenFrame(offscreenTarget)
            renderEncoder.endEncoding()
        }

        if mapInterface.getNeedsCompute() {
            guard
                let computeEncoder = commandBuffer.makeComputeCommandEncoder()
            else {
                self.renderSemaphore.signal()
                return
            }

            renderingContext.computeEncoder = computeEncoder
            mapInterface.compute()
            computeEncoder.endEncoding()
        }

        guard let drawable = metalLayer.nextDrawable() else {
            self.renderSemaphore.signal()
            return
        }

        let renderPassDescriptor = MTLRenderPassDescriptor()
        renderPassDescriptor.colorAttachments[0].texture = drawable.texture
        renderPassDescriptor.colorAttachments[0].loadAction = .clear
        renderPassDescriptor.colorAttachments[0].storeAction = .store
        renderPassDescriptor.colorAttachments[0].clearColor = clearColor
        if !stencilTextures.isEmpty {
            currentStencilIndex = (currentStencilIndex + 1) % stencilTextures.count
            renderPassDescriptor.stencilAttachment.texture = stencilTextures[currentStencilIndex]
            renderPassDescriptor.stencilAttachment.loadAction = .clear
            renderPassDescriptor.stencilAttachment.storeAction = .dontCare
            currentStencilIndex += 1
        }

        guard
            let renderEncoder = commandBuffer.makeRenderCommandEncoder(
                descriptor: renderPassDescriptor)
        else {
            self.renderSemaphore.signal()
            return
        }

        renderingContext.encoder = renderEncoder
        renderingContext.renderTarget = nil

        mapInterface.drawFrame()

        renderEncoder.endEncoding()

        commandBuffer.addCompletedHandler { _ in
            self.renderSemaphore.signal()
        }

        commandBuffer.present(drawable)
        commandBuffer.commit()
    }
}

extension CGSize {
    var vec2: MCVec2I {
        MCVec2I(
            x: Int32(width),
            y: Int32(height))
    }
}

extension MCMapView {
    public override func touchesBegan(
        _ touches: Set<UITouch>, with event: UIEvent?
    ) {
        super.touchesBegan(touches, with: event)
        touchHandler.touchesBegan(touches, with: event)
    }

    public override func touchesEnded(
        _ touches: Set<UITouch>, with event: UIEvent?
    ) {
        super.touchesEnded(touches, with: event)
        touchHandler.touchesEnded(touches, with: event)
    }

    public override func touchesCancelled(
        _ touches: Set<UITouch>, with event: UIEvent?
    ) {
        super.touchesCancelled(touches, with: event)
        touchHandler.touchesCancelled(touches, with: event)
    }

    public override func touchesMoved(
        _ touches: Set<UITouch>, with event: UIEvent?
    ) {
        super.touchesMoved(touches, with: event)
        touchHandler.touchesMoved(touches, with: event)
    }

    public override func gestureRecognizerShouldBegin(
        _ gestureRecognizer: UIGestureRecognizer
    ) -> Bool {
        if gestureRecognizer == pinch || gestureRecognizer == pan {
            var isiOSAppOnMac = ProcessInfo.processInfo.isMacCatalystApp
            if #available(iOS 14.0, *) {
                isiOSAppOnMac =
                    isiOSAppOnMac || ProcessInfo.processInfo.isiOSAppOnMac
            }
            return isiOSAppOnMac
        } else {
            return super.gestureRecognizerShouldBegin(gestureRecognizer)
        }
    }
}

extension MCMapView {
    public var camera: MCMapCameraInterface {
        mapInterface.getCamera()!
    }

    public var camera3d: MCMapCamera3dInterface? {
        camera.asMapCamera3d()
    }

    public func add(layer: MCLayerInterface?) {
        mapInterface.addLayer(layer)
    }

    public func insert(layer: MCLayerInterface?, at index: Int) {
        mapInterface.insertLayer(at: layer, at: Int32(index))
    }

    public func insert(layer: MCLayerInterface?, above: MCLayerInterface?) {
        mapInterface.insertLayer(above: layer, above: above)
    }

    public func insert(layer: MCLayerInterface?, below: MCLayerInterface?) {
        mapInterface.insertLayer(below: layer, below: below)
    }

    public func remove(layer: MCLayerInterface?) {
        mapInterface.removeLayer(layer)
    }

    public func add(layer: any Layer) {
        mapInterface.addLayer(layer.interface)
    }

    public func insert(layer: any Layer, at index: Int) {
        mapInterface.insertLayer(at: layer.interface, at: Int32(index))
    }

    public func insert(layer: any Layer, above: MCLayerInterface?) {
        mapInterface.insertLayer(above: layer.interface, above: above)
    }

    public func insert(layer: any Layer, below: MCLayerInterface?) {
        mapInterface.insertLayer(below: layer.interface, below: below)
    }

    public func remove(layer: any Layer) {
        mapInterface.removeLayer(layer.interface)
    }
}

extension MCMapView: UIGestureRecognizerDelegate {
    // MARK: - Mac setup

    private func setupMacGestureRecognizersIfNeeded() {
        var isiOSAppOnMac = ProcessInfo.processInfo.isMacCatalystApp
        if #available(iOS 14.0, *) {
            isiOSAppOnMac =
                isiOSAppOnMac || ProcessInfo.processInfo.isiOSAppOnMac
        }

        guard isiOSAppOnMac else { return }

        pinch.delegate = self
        pinch.delaysTouchesBegan = false
        pinch.allowedTouchTypes = []
        self.addGestureRecognizer(pinch)

        pan.delegate = self
        if #available(iOS 13.4, *) {
            pan.allowedScrollTypesMask = .continuous
        }
        pan.delaysTouchesBegan = false
        pan.allowedTouchTypes = []
        self.addGestureRecognizer(pan)

        pan.require(toFail: pinch)
    }

    public func gestureRecognizer(
        _ gestureRecognizer: UIGestureRecognizer,
        shouldRecognizeSimultaneouslyWith otherGestureRecognizer:
            UIGestureRecognizer
    ) -> Bool {
        true
    }

    public func gestureRecognizer(
        _ gestureRecognizer: UIGestureRecognizer, shouldReceive touch: UITouch
    ) -> Bool {
        return true
    }

    @objc func pinched(_ gestureRecognizer: UIPinchGestureRecognizer) {
        self.touchHandler.handlePinch(pinchGestureRecognizer: gestureRecognizer)
    }

    @objc func panned(_ gestureRecognizer: UIPanGestureRecognizer) {
        self.touchHandler.handlePan(panGestureRecognizer: gestureRecognizer)
    }
}

