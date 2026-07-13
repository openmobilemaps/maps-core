#if canImport(UIKit) || (canImport(AppKit) && !canImport(UIKit))
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
    @preconcurrency import MetalKit
    import os
    #if canImport(UIKit)
        import UIKit
    #elseif canImport(AppKit) && !canImport(UIKit)
        import AppKit
    #endif

    open class MCMapView: MTKView {
        @objc public let mapInterface: MCMapInterface
        private let renderingContext: RenderingContext

        var renderToImageRenderPass: RenderToImageRenderPass? = nil

        private var sizeChanged = false
        private var backgroundDisable = false
        private var renderToImage = false
        private lazy var renderToImageQueue = DispatchQueue(
            label: "io.openmobilemaps.renderToImagQueue", qos: .userInteractive)

        public var pausesAutomatically = true
        private var framesToRender: Int = 1
        private let framesToRenderAfterInvalidate: Int = 2
        private var renderSemaphore = DispatchSemaphore(value: 3)  // using triple buffers

        private let touchHandler: MCMapViewTouchHandler
        private let callbackHandler = MCMapViewCallbackHandler()

        #if canImport(UIKit)
            private var touchForwardingGestureRecognizer: TouchForwardingGestureRecognizer!
            private var pinch: UIPinchGestureRecognizer!
            private var pan: UIPanGestureRecognizer!
        #elseif canImport(AppKit) && !canImport(UIKit)
            private var magnificationGestureRecognizer: NSMagnificationGestureRecognizer!
            private var panGestureRecognizer: NSPanGestureRecognizer!
            private var clickGestureRecognizer: NSClickGestureRecognizer!
        #endif

        public weak var sizeDelegate: MCMapSizeDelegate?

        private var depthTextures: [MTLTexture] = []
        private var depthTextureSize: CGSize = .zero

        // Obj-C-visible `(mapConfig:pixelsPerInch:is3D:)` initializer. The designated Swift init
        // below uses `Float?`, which isn't representable in Obj-C; Kotlin/Native bridges therefore
        // can't see it. Wrap `pixelsPerInch` in `NSNumber?` so Kotlin callers (e.g.
        // `IosOffscreenMapRenderHelper.renderMap`) can actually hand in their own `KMMapConfig`
        // instead of falling back to the no-arg `MCMapView()` with hard-coded EPSG:3857 / 2D.
        @objc(initWithMapConfig:pixelsPerInch:is3D:)
        public convenience init(
            mapConfig: MCMapConfig,
            pixelsPerInch: NSNumber?,
            is3D: Bool
        ) {
            self.init(
                mapConfig: mapConfig,
                pixelsPerInch: pixelsPerInch?.floatValue,
                is3D: is3D
            )
        }

        // Obj-C-visible no-arg initializer. Swift's default arguments aren't exposed to Obj-C, so
        // Kotlin/Native callers that instantiate `MCMapView()` would otherwise land on MTKView's
        // `init(frame:device:)` — which MCMapView's designated init suppresses, leading to a
        // "Use of unimplemented initializer" fatalError at runtime.
        @objc public convenience init() {
            self.init(
                mapConfig: MCMapConfig(mapCoordinateSystem: MCCoordinateSystemFactory.getEpsg3857System()),
                pixelsPerInch: Float?.none,
                is3D: false
            )
        }

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
                    pixelDensity: pixelsPerInch ?? Float(MCDisplayMetrics.pixelsPerInch(for: nil)),
                    is3D: is3D)
            else {
                fatalError("Can't create MCMapInterface")
            }
            self.mapInterface = mapInterface
            self.renderingContext = renderingContext
            touchHandler = MCMapViewTouchHandler(touchHandler: mapInterface.touchHandler)
            super.init(frame: .zero, device: MetalContext.current.device)
            #if canImport(UIKit)
                self.pinch = UIPinchGestureRecognizer(
                    target: self, action: #selector(pinched))
                self.pan = UIPanGestureRecognizer(
                    target: self, action: #selector(panned))
            #endif
            setup()
        }

        @available(*, unavailable)
        public required init(coder _: NSCoder) {
            fatalError("init(coder:) has not been implemented")
        }

        deinit {
            // nasty workaround for the dispatch_semaphore crash
            for _ in 0..<3 {
                renderSemaphore.signal()
            }
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
            renderingContext.sceneView = self

            device = MetalContext.current.device

            colorPixelFormat = MetalContext.colorPixelFormat
            framebufferOnly = false

            delegate = self

            depthStencilPixelFormat = MetalContext.depthPixelFormat

            #if canImport(UIKit)
                if #available(iOS 16.0, *) {
                    depthStencilStorageMode = .memoryless
                }

                isMultipleTouchEnabled = true
            #endif

            preferredFramesPerSecond = 120

            sampleCount = 1  // samples per pixel

            presentsWithTransaction = true

            callbackHandler.invalidateCallback = { [weak self] in
                self?.invalidate()
            }
            mapInterface.setCallbackHandler(callbackHandler)

            touchHandler.mapView = self

            setupPlatformGestureRecognizers()

            #if canImport(UIKit)
                if #available(iOS 13.4, *) {
                    let hover = UIHoverGestureRecognizer(target: self, action: #selector(hovered))
                    hover.delegate = self
                    addGestureRecognizer(hover)
                }
            #endif

            mapInterface.resume()

            addPlatformEventListeners()
        }

        private func setupPlatformGestureRecognizers() {
            #if canImport(UIKit)
                touchForwardingGestureRecognizer = TouchForwardingGestureRecognizer(
                    touchHandler: touchHandler)
                touchForwardingGestureRecognizer.delegate = self
                touchForwardingGestureRecognizer.delaysTouchesBegan = false
                touchForwardingGestureRecognizer.delaysTouchesEnded = false
                touchForwardingGestureRecognizer.cancelsTouchesInView = false
                addGestureRecognizer(touchForwardingGestureRecognizer)

                setupMacGestureRecognizersIfNeeded()
            #elseif canImport(AppKit) && !canImport(UIKit)
                magnificationGestureRecognizer = NSMagnificationGestureRecognizer(
                    target: self,
                    action: #selector(magnified)
                )
                addGestureRecognizer(magnificationGestureRecognizer)

                panGestureRecognizer = NSPanGestureRecognizer(
                    target: self,
                    action: #selector(macPanned)
                )
                addGestureRecognizer(panGestureRecognizer)

                clickGestureRecognizer = NSClickGestureRecognizer(
                    target: self,
                    action: #selector(clicked)
                )
                clickGestureRecognizer.buttonMask = 0x1
                clickGestureRecognizer.numberOfClicksRequired = 1
                addGestureRecognizer(clickGestureRecognizer)
            #endif
        }

        private func addPlatformEventListeners() {
            #if canImport(UIKit)
                NotificationCenter.default.addObserver(
                    self,
                    selector: #selector(willEnterForeground),
                    name: UIApplication.willEnterForegroundNotification,
                    object: nil)
                NotificationCenter.default.addObserver(
                    self, selector: #selector(didEnterBackground),
                    name: UIApplication.didEnterBackgroundNotification,
                    object: nil)
            #endif
        }

        #if canImport(UIKit)
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
        #elseif canImport(AppKit) && !canImport(UIKit)
            open override func viewWillMove(toWindow newWindow: NSWindow?) {
                super.viewWillMove(toWindow: newWindow)
                if newWindow == nil {
                    touchHandler.cancelAllTouches()
                }
            }
        #endif

        /// The view’s background color.
        #if canImport(UIKit)
            override public var backgroundColor: UIColor? {
                get {
                    super.backgroundColor
                }
                set {
                    super.backgroundColor = newValue
                    mapInterface.setBackgroundColor(
                        newValue?.mapCoreColor ?? UIColor.clear.mapCoreColor)
                    isOpaque = newValue?.isOpaque ?? false
                    // Mirror `UIView.backgroundColor` onto `MTKView.clearColor` so the metal render
                    // pass (onscreen and the offscreen render-to-image path) actually clears to the
                    // caller's requested colour instead of MTKView's default opaque black. Offscreen
                    // callers like `IosOffscreenMapRenderHelper` set `backgroundColor = .whiteColor`
                    // expecting a white background; without this mirroring the render pass would
                    // clear to black and any transparent edges in the base style composite as dark
                    // shadows on the resulting thumbnail.
                    self.clearColor = (newValue ?? UIColor.clear).metalClearColor
                }
            }
        #elseif canImport(AppKit) && !canImport(UIKit)
            open var backgroundColor: NSColor? {
                didSet {
                    mapInterface.setBackgroundColor(
                        backgroundColor?.mapCoreColor ?? NSColor.clear.mapCoreColor
                    )
                    wantsLayer = true
                    layer?.backgroundColor = backgroundColor?.cgColor
                    clearColor = (backgroundColor ?? NSColor.clear).metalClearColor
                }
            }
        #endif

        public func invalidate() {
            isPaused = false
            framesToRender = framesToRenderAfterInvalidate
        }

        public func renderTarget(named name: String) -> RenderTargetTexture {
            guard let target = renderingContext.getCreateOffscreenRenderTarget(name) as? RenderTargetTexture else {
                fatalError("RenderingContext returned a non-Metal render target for '\(name)'")
            }
            return target
        }

        /// Objective-C-visible accessor for ``renderTarget(named:)``. Returns the render target
        /// typed as ``MCRenderTargetInterface`` so Kotlin/Native cinterop (which can't see the
        /// Swift-only ``RenderTargetTexture`` class) can call it.
        @objc public func renderTargetForName(_ name: String) -> any MCRenderTargetInterface {
            return renderTarget(named: name)
        }
    }
    extension MCMapView: MTKViewDelegate {
        open func mtkView(_: MTKView, drawableSizeWillChange _: CGSize) {
            sizeChanged = true
            invalidate()
        }

        public func draw(in view: MTKView) {
            guard !backgroundDisable else {
                isPaused = true
                mapInterface.resetIsInvalidated()
                return  // don't execute metal calls in background
            }

            guard
                framesToRender > 0 || !pausesAutomatically
            else {
                isPaused = true
                mapInterface.resetIsInvalidated()
                return
            }

            if pausesAutomatically {
                framesToRender -= 1
            }

            // Ensure that triple-buffers are not over-used
            renderSemaphore.wait()

            renderingContext.beginFrame()
            mapInterface.prepare()

            // Shared lib stuff
            if sizeChanged {
                mapInterface.setViewportSize(view.drawableSize.vec2)
                sizeDelegate?.sizeChanged()
                sizeChanged = false
            }

            guard
                let commandBuffer = MetalContext.current.commandQueue
                    .makeCommandBuffer()
            else {
                self.renderSemaphore.signal()
                return
            }

            let offscreenTargets = renderingContext.getOffscreenRenderTargets().compactMap { $0 as? RenderTargetTexture }

            for offscreenTarget in offscreenTargets {
                let renderEncoder = offscreenTarget.prepareOffscreenEncoder(
                    commandBuffer,
                    size: view.drawableSize.vec2,
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

            guard let renderPassDescriptor = renderToImage ? getToImageRenderpass() : view.currentRenderPassDescriptor else {
                self.renderSemaphore.signal()
                return
            }

            ensureMainDepthAttachment(renderPassDescriptor, drawableSize: view.drawableSize)

            guard let renderEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor) else {
                self.renderSemaphore.signal()
                return
            }

            renderingContext.encoder = renderEncoder
            renderingContext.renderTarget = nil

            mapInterface.drawFrame()

            renderEncoder.endEncoding()

            commandBuffer.addCompletedHandler { @Sendable [weak renderSemaphore] _ in
                renderSemaphore?.signal()
            }

            // if we want to save the drawable (image rendering), we commit and wait synchronously
            // until the command buffer completes, also we don't present it
            if self.renderToImage {
                commandBuffer.commit()
                commandBuffer.waitUntilCompleted()
            } else {

                guard let drawable = view.currentDrawable else {
                    self.renderSemaphore.signal()
                    return
                }

                commandBuffer.commit()
                commandBuffer.waitUntilScheduled()
                drawable.present()
            }
        }

        private func getToImageRenderpass() -> MTLRenderPassDescriptor? {
            if self.renderToImageRenderPass == nil {
                guard let device else { return nil }
                self.renderToImageRenderPass = RenderToImageRenderPass(device: device)
            }

            // Respect the MTKView's configured clear color so offscreen thumbnails composite over
            // the host's requested background instead of an opaque black, which otherwise bleeds
            // through transparent edges in the base style and renders as darkened terrain.
            return self.renderToImageRenderPass?.getRenderpass(size: self.drawableSize, clearColor: self.clearColor)
        }

        private func ensureMainDepthAttachment(
            _ renderPassDescriptor: MTLRenderPassDescriptor,
            drawableSize: CGSize
        ) {
            guard !renderToImage else { return }
            guard
                let depthTexture = depthTexture(
                    for: drawableSize, bufferIndex: renderingContext.currentBufferIndex)
            else { return }

            renderPassDescriptor.depthAttachment.texture = depthTexture
            renderPassDescriptor.depthAttachment.loadAction = .clear
            renderPassDescriptor.depthAttachment.storeAction = .dontCare
            renderPassDescriptor.depthAttachment.clearDepth = 1.0
            renderPassDescriptor.stencilAttachment.texture = depthTexture
            renderPassDescriptor.stencilAttachment.loadAction = .clear
            renderPassDescriptor.stencilAttachment.storeAction = .dontCare
            renderPassDescriptor.stencilAttachment.clearStencil = 0
        }

        private func depthTexture(for size: CGSize, bufferIndex: Int) -> MTLTexture? {
            let width = max(Int(size.width), 1)
            let height = max(Int(size.height), 1)
            let normalizedSize = CGSize(width: width, height: height)

            if depthTextureSize != normalizedSize || depthTextures.count != RenderingContext.bufferCount {
                depthTextures.removeAll(keepingCapacity: true)
                for _ in 0..<RenderingContext.bufferCount {
                    guard let texture = makeDepthTexture(width: width, height: height) else {
                        depthTextures.removeAll(keepingCapacity: false)
                        return nil
                    }
                    depthTextures.append(texture)
                }
                depthTextureSize = normalizedSize
            }

            return depthTextures.indices.contains(bufferIndex) ? depthTextures[bufferIndex] : nil
        }

        private func makeDepthTexture(width: Int, height: Int) -> MTLTexture? {
            let descriptor = MTLTextureDescriptor.texture2DDescriptor(
                pixelFormat: MetalContext.depthPixelFormat,
                width: width,
                height: height,
                mipmapped: false
            )
            descriptor.storageMode = .private
            descriptor.usage = [.renderTarget]
            return MetalContext.current.device.makeTexture(descriptor: descriptor)
        }

        @objc public func renderToImage(
            size: CGSize, timeout: Float, bounds: MCRectCoord, boundsPaddingPc: Float = 0.0,
            callbackQueue: DispatchQueue = .main,
            callback: @escaping @Sendable (MCPlatformImage?, MCLayerReadyState) -> Void
        ) {
            renderToImageQueue.async { @Sendable [weak self] in
                self?
                    .renderToImageWhenApplicationIsActive(
                        size: size,
                        timeout: timeout,
                        bounds: bounds,
                        boundsPaddingPc: boundsPaddingPc,
                        callbackQueue: callbackQueue,
                        callback: callback)
            }
        }
    }

    extension MCMapView {
        fileprivate static let renderToImageMaxFrames: Int = 10

        fileprivate nonisolated var canRenderToImageNow: Bool {
            #if canImport(UIKit)
                MainActor.assumeIsolated {
                    UIApplication.shared.applicationState == .active && !backgroundDisable
                }
            #else
                true
            #endif
        }

        fileprivate nonisolated func renderToImageWhenApplicationIsActive(
            size: CGSize,
            timeout: Float,
            bounds: MCRectCoord,
            boundsPaddingPc: Float,
            callbackQueue: DispatchQueue?,
            callback: @escaping @Sendable (MCPlatformImage?, MCLayerReadyState) -> Void
        ) {
            waitForApplicationActive()

            DispatchQueue.main.sync {
                MainActor.assumeIsolated {
                    // set the drawable size to get correctly sized texture
                    self.drawableSize = size
                }
            }

            let mapReadyCallbacks = MCMapViewMapReadyCallbacks(
                delegate: self,
                size: size,
                timeout: timeout,
                bounds: bounds,
                boundsPaddingPc: boundsPaddingPc,
                callback: callback,
                callbackQueue: callbackQueue)

            mapInterface.drawReadyFrame(
                bounds,
                paddingPc: boundsPaddingPc,
                timeout: timeout,
                callbacks: mapReadyCallbacks)
        }

        private nonisolated func waitForApplicationActive() {
            #if canImport(UIKit)
                let semaphore = DispatchSemaphore(value: 0)
                var applicationActiveWaiter: MCMapViewApplicationActiveWaiter?

                DispatchQueue.main.sync {
                    if self.canRenderToImageNow {
                        semaphore.signal()
                    } else {
                        applicationActiveWaiter = MCMapViewApplicationActiveWaiter(
                            delegate: self,
                            semaphore: semaphore)
                        applicationActiveWaiter?.start()
                    }
                }

                semaphore.wait()
                _ = applicationActiveWaiter
            #endif
        }

        fileprivate func retryRenderToImageWhenApplicationIsActive(
            size: CGSize,
            timeout: Float,
            bounds: MCRectCoord,
            boundsPaddingPc: Float,
            callbackQueue: DispatchQueue?,
            callback: @escaping @Sendable (MCPlatformImage?, MCLayerReadyState) -> Void
        ) {
            renderToImageQueue.async { @Sendable [weak self] in
                self?
                    .renderToImageWhenApplicationIsActive(
                        size: size,
                        timeout: timeout,
                        bounds: bounds,
                        boundsPaddingPc: boundsPaddingPc,
                        callbackQueue: callbackQueue,
                        callback: callback)
            }
        }

        /// Render the current map state into the offscreen texture and return the resulting image.
        ///
        /// A single `draw(in:)` is often insufficient — layers can call `invalidate()` while
        /// being drawn (e.g. a tile finishes decoding or an animation wants the next frame),
        /// which bumps `framesToRender` back up via the map's callback handler. Keep redrawing
        /// until the map settles (no pending invalidations) or we hit `renderToImageMaxFrames`
        /// to avoid pathological busy-looping if a layer keeps re-invalidating itself.
        fileprivate func renderToImageResult() -> MCPlatformImage? {
            self.renderToImage = true
            self.invalidate()

            var iterations = 0
            while framesToRender > 0 && iterations < Self.renderToImageMaxFrames {
                self.draw(in: self)
                iterations += 1
            }

            self.renderToImage = false

            return self.renderToImageRenderPass?.getImage()
        }
    }

    extension CGSize {
        var vec2: MCVec2I {
            MCVec2I(
                x: Int32(width),
                y: Int32(height))
        }
    }

    #if canImport(UIKit)
        extension MCMapView {
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
    #elseif canImport(AppKit) && !canImport(UIKit)
        extension MCMapView {
            @objc func magnified(_ gestureRecognizer: NSMagnificationGestureRecognizer) {
                touchHandler.handleMagnification(magnificationGestureRecognizer: gestureRecognizer)
            }

            @objc func macPanned(_ gestureRecognizer: NSPanGestureRecognizer) {
                touchHandler.handlePan(panGestureRecognizer: gestureRecognizer)
            }

            @objc func clicked(_ gestureRecognizer: NSClickGestureRecognizer) {
                guard gestureRecognizer.state == .ended else { return }
                let point = gestureRecognizer.location(in: self)
                touchHandler.handleClick(point: point)
            }

            open override func scrollWheel(with event: NSEvent) {
                super.scrollWheel(with: event)
                let point = convert(event.locationInWindow, from: nil)
                touchHandler.handleScroll(point: point, deltaY: event.scrollingDeltaY)
            }
        }
    #endif

    extension MCMapView {
        public var camera: MCMapCameraInterface {
            mapInterface.camera
        }

        public var camera3d: MCMapCamera3dInterface? {
            mapInterface.camera3d
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

        public func add(layer: any MapLayer) {
            mapInterface.addLayer(layer)
        }

        public func insert(layer: any MapLayer, at index: Int) {
            mapInterface.insertLayer(layer, at: index)
        }

        public func insert(layer: any MapLayer, above: MCLayerInterface?) {
            mapInterface.insertLayer(layer, above: above)
        }

        public func insert(layer: any MapLayer, above: any MapLayer) {
            mapInterface.insertLayer(layer, above: above)
        }

        public func insert(layer: any MapLayer, below: MCLayerInterface?) {
            mapInterface.insertLayer(layer, below: below)
        }

        public func insert(layer: any MapLayer, below: any MapLayer) {
            mapInterface.insertLayer(layer, below: below)
        }

        public func remove(layer: any MapLayer) {
            mapInterface.removeLayer(layer)
        }
    }

    #if canImport(UIKit)
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

            @available(iOS 13.4, *)
            @objc func hovered(_ gestureRecognizer: UIHoverGestureRecognizer) {
                self.touchHandler.handleHover(hoverGestureRecognizer: gestureRecognizer)
            }
        }
    #endif

    #if canImport(UIKit)
        private final class TouchForwardingGestureRecognizer: UIGestureRecognizer {
            private let touchHandler: MCMapViewTouchHandler
            private var trackedTouches = Set<UITouch>()
            private var initialTouchPoint: CGPoint?
            private let leftEdgeThreshold: CGFloat = 44.0

            init(touchHandler: MCMapViewTouchHandler) {
                self.touchHandler = touchHandler
                super.init(target: nil, action: nil)
            }

            var startedAwayFromLeftEdge: Bool {
                guard let point = initialTouchPoint else { return false }
                return point.x > leftEdgeThreshold
            }

            override func touchesBegan(_ touches: Set<UITouch>, with event: UIEvent) {
                super.touchesBegan(touches, with: event)
                trackedTouches.formUnion(touches)
                if initialTouchPoint == nil, let touch = touches.first, let view {
                    initialTouchPoint = touch.location(in: view)
                }
                if state == .possible {
                    state = .began
                } else {
                    state = .changed
                }
                touchHandler.touchesBegan(touches, with: event)
            }

            override func touchesMoved(_ touches: Set<UITouch>, with event: UIEvent) {
                super.touchesMoved(touches, with: event)
                if state == .possible {
                    state = .began
                } else {
                    state = .changed
                }
                touchHandler.touchesMoved(touches, with: event)
            }

            override func touchesEnded(_ touches: Set<UITouch>, with event: UIEvent) {
                super.touchesEnded(touches, with: event)
                touchHandler.touchesEnded(touches, with: event)
                trackedTouches.subtract(touches)
                state = trackedTouches.isEmpty ? .ended : .changed
            }

            override func touchesCancelled(_ touches: Set<UITouch>, with event: UIEvent) {
                super.touchesCancelled(touches, with: event)
                touchHandler.touchesCancelled(touches, with: event)
                trackedTouches.subtract(touches)
                state = .cancelled
            }

            override func reset() {
                super.reset()
                trackedTouches.removeAll()
                initialTouchPoint = nil
            }

            override func shouldBeRequiredToFail(by otherGestureRecognizer: UIGestureRecognizer) -> Bool {
                if startedAwayFromLeftEdge {
                    return true
                } else {
                    return super.shouldBeRequiredToFail(by: otherGestureRecognizer)
                }
            }

            override func shouldRequireFailure(of otherGestureRecognizer: UIGestureRecognizer) -> Bool {
                if startedAwayFromLeftEdge {
                    return super.shouldRequireFailure(of: otherGestureRecognizer)
                } else {
                    return true
                }
            }

        }
    #endif

    #if canImport(UIKit)
        private final class MCMapViewApplicationActiveWaiter: NSObject {
            private nonisolated(unsafe) weak var delegate: MCMapView?
            private let semaphore: DispatchSemaphore
            private var didSignal = false

            init(delegate: MCMapView?, semaphore: DispatchSemaphore) {
                self.delegate = delegate
                self.semaphore = semaphore
            }

            func start() {
                NotificationCenter.default.addObserver(
                    self,
                    selector: #selector(applicationDidBecomeActive),
                    name: UIApplication.didBecomeActiveNotification,
                    object: nil)
            }

            @objc private func applicationDidBecomeActive() {
                guard !didSignal else { return }
                guard let delegate else {
                    signal()
                    return
                }
                guard delegate.canRenderToImageNow else { return }

                signal()
            }

            deinit {
                NotificationCenter.default.removeObserver(self)
            }

            private func signal() {
                didSignal = true
                NotificationCenter.default.removeObserver(self)
                semaphore.signal()
            }
        }
    #endif

    private final class MCMapViewMapReadyCallbacks:
        MCMapReadyCallbackInterface, Sendable
    {
        public nonisolated(unsafe) weak var delegate: MCMapView?
        private let size: CGSize
        private let timeout: Float
        private let bounds: MCRectCoord
        private let boundsPaddingPc: Float
        public let callback: (@Sendable (MCPlatformImage?, MCLayerReadyState) -> Void)?
        public let callbackQueue: DispatchQueue?
        public let semaphore = DispatchSemaphore(value: 1)
        private let completionSemaphore = DispatchSemaphore(value: 1)
        private nonisolated(unsafe) var completed = false

        init(
            delegate: MCMapView?,
            size: CGSize,
            timeout: Float,
            bounds: MCRectCoord,
            boundsPaddingPc: Float,
            callback: (@Sendable (MCPlatformImage?, MCLayerReadyState) -> Void)?,
            callbackQueue: DispatchQueue?
        ) {
            self.delegate = delegate
            self.size = size
            self.timeout = timeout
            self.bounds = bounds
            self.boundsPaddingPc = boundsPaddingPc
            self.callback = callback
            self.callbackQueue = callbackQueue
        }

        func stateDidUpdate(_ state: MCLayerReadyState) {

            semaphore.wait()

            Task { @MainActor in
                guard let delegate = self.delegate else {
                    // Delegate went away — we still own the semaphore and must release it so
                    // subsequent `renderToImage` invocations don't deadlock.
                    self.semaphore.signal()
                    return
                }

                guard !self.isCompleted else {
                    self.semaphore.signal()
                    return
                }

                if !delegate.canRenderToImageNow {
                    guard state != .NOT_READY else {
                        self.semaphore.signal()
                        return
                    }

                    guard self.markCompleted() else {
                        self.semaphore.signal()
                        return
                    }

                    self.semaphore.signal()
                    delegate.retryRenderToImageWhenApplicationIsActive(
                        size: self.size,
                        timeout: self.timeout,
                        bounds: self.bounds,
                        boundsPaddingPc: self.boundsPaddingPc,
                        callbackQueue: self.callbackQueue,
                        callback: { image, state in
                            self.callback?(image, state)
                        })
                    return
                }

                delegate.draw(in: delegate)

                // Callers may pass a nil `callbackQueue` (Kotlin/Native bridges can't observe the
                // Swift default of `.main`) — fall back to the main queue instead of silently
                // dropping the callback and leaving the semaphore held forever.
                (callbackQueue ?? .main)
                    .async {
                        switch state {
                            case .NOT_READY:
                                // Non-terminal state: keep waiting for a later update. Signal the
                                // semaphore so the next state update can proceed.
                                break
                            case .ERROR, .TIMEOUT_ERROR:
                                if self.markCompleted() {
                                    self.callback?(nil, state)
                                }
                            case .READY:
                                if self.markCompleted() {
                                    MainActor.assumeIsolated {
                                        self.callback?(delegate.renderToImageResult(), state)
                                    }
                                }
                            @unknown default:
                                break
                        }
                        self.semaphore.signal()
                    }
            }
        }

        private var isCompleted: Bool {
            completionSemaphore.wait()
            defer { completionSemaphore.signal() }
            return completed
        }

        private func markCompleted() -> Bool {
            completionSemaphore.wait()
            defer { completionSemaphore.signal() }

            guard !completed else { return false }

            completed = true
            return true
        }
    }
#endif
