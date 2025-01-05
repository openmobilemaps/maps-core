//
//  MCMapRenderLoop.swift
//
//
//  Created by Nicolas Märki on 03.07.2024.
//

#if os(visionOS)
    import Metal
    import CompositorServices
    import SwiftUI

    @available(visionOS 2.0, *)
    public class ImmersiveMapRenderer {
        let layerRenderer: LayerRenderer
        let layers: [MCLayerInterface]

        var followHand = false
        var handOffset = simd_float4x4(diagonal: .one)

        let rasterSampleCount: Int
        var memorylessTargetIndex: Int = 0
        var memorylessTargets: [(color: MTLTexture, depth: MTLTexture)?] = []

        let maxBuffersInFlight = 3

        var projectionMatrices: [Double] = .init(repeating: 0, count: 32)
        var viewMatrices: [Double] = .init(repeating: 0, count: 32)

        let commandQueue: MTLCommandQueue
        let renderingContext = RenderingContext()

        let session = ARKitSession()
        let worldTracking = WorldTrackingProvider()
        let handTracking = HandTrackingProvider()

        let mapConfig: MCMapConfig
        let mapInterface: MCMapInterface
        let camera: MCMapCamera3dInterface

        public init(
            _ layerRenderer: LayerRenderer,
            layers: [MCLayerInterface]
        ) {
            self.layerRenderer = layerRenderer
            self.layers = layers

            self.renderingContext.amplificationCount =
                layerRenderer.properties.viewCount
            MetalContext.current.setup(
                maxVertexAmplificationCount: self.renderingContext
                    .amplificationCount)

            self.commandQueue = MetalContext.current.device.makeCommandQueue()!

            if MetalContext.current.device.supports32BitMSAA
                && MetalContext.current.device.supportsTextureSampleCount(4)
            {
                rasterSampleCount = 4
            } else {
                rasterSampleCount = 1
            }

            self.memorylessTargets = .init(
                repeating: nil, count: maxBuffersInFlight)

            self.mapConfig = MCMapConfig(
                mapCoordinateSystem:
                    MCCoordinateSystemFactory.getUnitSphereSystem())

            self.mapInterface = MCMapInterface.create(
                GraphicsFactory(),
                shaderFactory: ShaderFactory(),
                renderingContext: renderingContext,
                mapConfig: mapConfig,
                scheduler: MCThreadPoolScheduler.create(),
                pixelDensity: Float(DevicePpi.pixelsPerInch),
                is3D: true)!

            self.camera = mapInterface.getCamera()!.asMapCamera3d()!

            mapInterface.setViewportSize(.init(x: 2732, y: 2048))
            mapInterface.setCallbackHandler(CallbackInterface())

        }
        public func run() {
            Task(executorPreference: RendererTaskExecutor.shared) {
                try await self.setup()
                self.renderLoop()
                self.teardown()
            }
        }

        private func renderLoop() {
            while true {
                switch layerRenderer.state {
                case .paused:
                    mapInterface.pause()
                    layerRenderer.waitUntilRunning()
                    mapInterface.resume()
                case .invalidated:
                    return
                case .running:
                    autoreleasepool {
                        self.renderFrame()
                    }
                @unknown default:
                    break
                }

            }
        }

        private func setup() async throws {
            mapInterface.resume()

            mapInterface
                .getCamera()?
                .move(
                    toCenterPositionZoom: MCCoord(
                        lat: 47.3769, lon: 8.5417),
                    zoom: 100_000,
                    animated: false
                )

            var touchEvents = [
                SpatialEventCollection.Event.ID: SpatialEventCollection
                    .Event
            ]()

            layerRenderer.onSpatialEvent = { events in
                for event in events {
                    if event.kind == .indirectPinch {
                        switch event.phase {
                        case .active:
                            self.followHand = true
                        case .cancelled, .ended:
                            self.followHand = false
                        }
                    }
                }
            }

            //                            if let touchHandler = mapInterface.getTouchHandler() {
            //                    layerRenderer.onSpatialEvent = { events in
            //                        for event in events {
            //                            if event.kind == .indirectPinch {
            //                                switch event.phase {
            //                                case .active:
            //                                    let new = touchEvents[event.id] == nil
            //                                    touchEvents[event.id] = event
            //                                    touchHandler
            //                                        .onTouchEvent(
            //                                            .init(
            //                                                pointers: touchEvents.pointers,
            //                                                touchAction: new ? .DOWN : .MOVE
            //                                            )
            //                                        )
            //                                case .ended:
            //                                    touchEvents[event.id] = nil
            //                                    touchHandler.onTouchEvent(
            //                                        .init(
            //                                            pointers: touchEvents.pointers,
            //                                            touchAction: .UP)
            //                                    )
            //                                case .cancelled:
            //                                    touchEvents[event.id] = nil
            //                                    touchHandler.onTouchEvent(
            //                                        .init(
            //                                            pointers: touchEvents.pointers,
            //                                            touchAction: .CANCEL)
            //                                    )
            //                                @unknown default:
            //                                    fatalError()
            //                                }
            //                            }
            //                        }
            //                    }
            //                }

            for layer in layers {
                mapInterface.addLayer(layer)
            }

            if HandTrackingProvider.isSupported {
                try await session.run([worldTracking, handTracking])
            } else {
                try await self.session.run([self.worldTracking])
            }
        }

        private func teardown() {
            mapInterface.pause()
            mapInterface.destroy()
            self.session.stop()
        }

        private func renderFrame() {
            guard let nextFrame = layerRenderer.queryNextFrame() else {
                return  // continue loop
            }

            nextFrame.startUpdate()
            nextFrame.endUpdate()

            nextFrame.startSubmission()

            guard let drawable = nextFrame.queryDrawable() else {
                return  // continue loop
            }

            let time = LayerRenderer.Clock.Instant.epoch.duration(
                to: drawable.frameTiming.presentationTime
            ).timeInterval

            let deviceAnchor = worldTracking.queryDeviceAnchor(
                atTimestamp: time)
            drawable.deviceAnchor = deviceAnchor

            self.renderingContext.beginFrame()

            // Convert the timestamps into units of seconds
            //        let anchorPredictionTime = LayerRenderer.Clock.Instant.epoch.duration(to: trackableAnchorTime)


            self.updateHardwareMatrices(
                deviceAnchor: deviceAnchor,
                drawable: drawable, camera: camera)

            mapInterface.prepare()

            let renderPassDescriptor = MTLRenderPassDescriptor()

            let commandBuffer = commandQueue.makeCommandBuffer()!

            if self.rasterSampleCount > 1 {
                let renderTargets = self.memorylessRenderTargets(
                    drawable: drawable)
                renderPassDescriptor.colorAttachments[0]
                    .resolveTexture = drawable.colorTextures[0]
                renderPassDescriptor.colorAttachments[0].texture =
                    renderTargets.color

                renderPassDescriptor.depthAttachment.resolveTexture =
                    drawable.depthTextures[0]
                renderPassDescriptor.depthAttachment.texture =
                    renderTargets.depth

                renderPassDescriptor.stencilAttachment.resolveTexture =
                    drawable.depthTextures[0]
                renderPassDescriptor.stencilAttachment.texture =
                    renderTargets.depth

                renderPassDescriptor.colorAttachments[0].storeAction =
                    .multisampleResolve
                renderPassDescriptor.depthAttachment.storeAction =
                    .multisampleResolve
                renderPassDescriptor.stencilAttachment.storeAction =
                    .multisampleResolve
            } else {
                renderPassDescriptor.colorAttachments[0].texture =
                    drawable.colorTextures[0]
                renderPassDescriptor.depthAttachment.texture =
                    drawable.depthTextures[0]
                renderPassDescriptor.stencilAttachment.texture =
                    drawable.depthTextures[0]

                renderPassDescriptor.colorAttachments[0].storeAction =
                    .store
                renderPassDescriptor.depthAttachment.storeAction =
                    .store
            }
            renderPassDescriptor.colorAttachments[0].loadAction = .clear
            renderPassDescriptor.colorAttachments[0].clearColor =
                MTLClearColor(
                    red: 0.0, green: 0.0, blue: 0.0, alpha: 0.0)
            renderPassDescriptor.depthAttachment.loadAction = .clear
            renderPassDescriptor.depthAttachment.clearDepth = 0.0
            renderPassDescriptor.rasterizationRateMap =
                drawable.rasterizationRateMaps.first
            if layerRenderer.configuration.layout == .layered {
                renderPassDescriptor.renderTargetArrayLength =
                    drawable.views.count
            }

            let encoder = commandBuffer.makeRenderCommandEncoder(
                descriptor: renderPassDescriptor)!

            let viewports = drawable.views.map {
                $0.textureMap.viewport
            }

            encoder.setViewports(viewports)

            if drawable.views.count > 1 {
                var viewMappings = (0..<drawable.views.count).map {
                    MTLVertexAmplificationViewMapping(
                        viewportArrayIndexOffset: UInt32($0),
                        renderTargetArrayIndexOffset: UInt32($0))
                }
                encoder.setVertexAmplificationCount(
                    viewports.count, viewMappings: &viewMappings)
            }

            self.renderingContext.encoder = encoder

            mapInterface.drawFrame()

            encoder.endEncoding()

            drawable.encodePresent(commandBuffer: commandBuffer)
            commandBuffer.commit()
            nextFrame.endSubmission()
        }

        private func updateHardwareMatrices(
            deviceAnchor: DeviceAnchor?,
            drawable: LayerRenderer.Drawable,
            camera: MCMapCamera3dInterface
        ) {
            guard let deviceAnchor = deviceAnchor else {
                return
            }

            let originFromDevice = deviceAnchor
                .originFromAnchorTransform

            if HandTrackingProvider.isSupported, self.followHand,
               let handAnchor = handTracking.latestAnchors.rightHand
            {
                handOffset =
                handOffset * 0.99 + handAnchor
                    .originFromAnchorTransform * 0.01
            }

            for eye in 0..<drawable.views.count {
                let projection = drawable.computeProjection(
                    viewIndex: eye)
                let view = drawable.views[eye]
                let deviceFromView = view.transform
                let viewMatrix =
                (originFromDevice * deviceFromView).inverse * handOffset

                for column in 0..<4 {
                    for row in 0..<4 {
                        self.projectionMatrices[
                            eye * 16 + column * 4 + row] =
                            Double(projection[column, row])
                        self.viewMatrices[eye * 16 + column * 4 + row] =
                            Double(viewMatrix[column, row])
                    }
                }
            }

            self.projectionMatrices.withUnsafeMutableBufferPointer {
                projectionMatrixPointer in
                let projectionPointer = Int64(
                    Int(
                        bitPattern: projectionMatrixPointer.baseAddress!
                    ))

                self.viewMatrices.withUnsafeMutableBufferPointer {
                    viewMatrixPointer in
                    let viewPointer = Int64(
                        Int(bitPattern: viewMatrixPointer.baseAddress!))
                    camera
                        .setHardwareMatrices(
                            viewPointer,
                            projectionMatrices: projectionPointer,
                            count: Int32(drawable.views.count)
                        )
                }

            }
        }

        private func memorylessRenderTargets(drawable: LayerRenderer.Drawable)
            -> (
                color: MTLTexture, depth: MTLTexture
            )
        {

            func renderTarget(
                resolveTexture: MTLTexture, cachedTexture: MTLTexture?
            ) -> MTLTexture {
                if let cachedTexture,
                    resolveTexture.width == cachedTexture.width
                        && resolveTexture.height == cachedTexture.height
                {
                    return cachedTexture
                } else {
                    let descriptor = MTLTextureDescriptor.texture2DDescriptor(
                        pixelFormat: resolveTexture.pixelFormat,
                        width: resolveTexture.width,
                        height: resolveTexture.height,
                        mipmapped: false)
                    descriptor.usage = .renderTarget
                    descriptor.textureType = .type2DMultisampleArray
                    descriptor.sampleCount = rasterSampleCount
                    descriptor.storageMode = .memoryless
                    descriptor.arrayLength = resolveTexture.arrayLength
                    return resolveTexture.device.makeTexture(
                        descriptor: descriptor)!
                }
            }

            memorylessTargetIndex =
                (memorylessTargetIndex + 1) % maxBuffersInFlight

            let cachedTargets = memorylessTargets[memorylessTargetIndex]
            let newTargets = (
                renderTarget(
                    resolveTexture: drawable.colorTextures[0],
                    cachedTexture: cachedTargets?.color),
                renderTarget(
                    resolveTexture: drawable.depthTextures[0],
                    cachedTexture: cachedTargets?.depth)
            )

            memorylessTargets[memorylessTargetIndex] = newTargets

            return newTargets
        }

        public struct Configuration: CompositorLayerConfiguration {
            public init() {}
            public func makeConfiguration(
                capabilities: LayerRenderer.Capabilities,
                configuration: inout LayerRenderer.Configuration
            ) {
                configuration.depthFormat = .depth32Float_stencil8
                configuration.colorFormat = .bgra8Unorm_srgb

                let foveationEnabled = capabilities.supportsFoveation
                configuration.isFoveationEnabled = foveationEnabled

                let options:
                    LayerRenderer.Capabilities.SupportedLayoutsOptions =
                        foveationEnabled ? [.foveationEnabled] : []
                let supportedLayouts = capabilities.supportedLayouts(
                    options: options)

                configuration.layout =
                    supportedLayouts.contains(.layered) ? .layered : .dedicated
            }
        }

        class CallbackInterface: MCMapCallbackInterface {
            func invalidate() {

            }

            func onMapResumed() {

            }

        }

    }

    @available(visionOS 2.0, *)
    final class RendererTaskExecutor: TaskExecutor {
        private let queue = DispatchQueue(
            label: "RenderThreadQueue", qos: .userInteractive)

        func enqueue(_ job: UnownedJob) {
            queue.async {
                job.runSynchronously(on: self.asUnownedSerialExecutor())
            }
        }

        func asUnownedSerialExecutor() -> UnownedTaskExecutor {
            return UnownedTaskExecutor(ordinary: self)
        }

        static var shared: RendererTaskExecutor = RendererTaskExecutor()
    }

    extension LayerRenderer.Clock.Instant.Duration {
        var timeInterval: TimeInterval {
            let nanoseconds = TimeInterval(
                components.attoseconds / 1_000_000_000)
            return TimeInterval(components.seconds)
                + (nanoseconds / TimeInterval(NSEC_PER_SEC))
        }
    }

    extension [SpatialEventCollection.Event.ID: SpatialEventCollection.Event] {
        var pointers: [MCVec2F] {
            map {
                .init(
                    x: (Float($0.value.inputDevicePose?.pose3D.position.x ?? 0)
                        / 2.0 + 0.5)
                        * 2732,
                    y: (Float($0.value.inputDevicePose?.pose3D.position.y ?? 0)
                        / -2.0 + 0.5)
                        * 2048
                )
            }
        }
    }

#endif
