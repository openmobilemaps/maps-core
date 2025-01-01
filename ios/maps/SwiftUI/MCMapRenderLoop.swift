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

class CallbackInterface: MCMapCallbackInterface {
    func invalidate() {

    }
    
    func onMapResumed() {

    }
    

}

@available(visionOS 2.0, *)
final class RendererTaskExecutor: TaskExecutor {
    private let queue = DispatchQueue(label: "RenderThreadQueue", qos: .userInteractive)

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
        let nanoseconds = TimeInterval(components.attoseconds / 1_000_000_000)
        return TimeInterval(components.seconds) + (nanoseconds / TimeInterval(NSEC_PER_SEC))
    }
}


@MainActor
@available(visionOS 2.0, *)
public func MCMapRenderLoop(
    _ layerRenderer: LayerRenderer,
    layers: [MCLayerInterface]
) {
    let mapConfig = MCMapConfig(mapCoordinateSystem: MCCoordinateSystemFactory.getUnitSphereSystem())

    let renderingContext = RenderingContext()
    let mapInterface = MCMapInterface.create(GraphicsFactory(),
                                             shaderFactory: ShaderFactory(),
                                             renderingContext: renderingContext,
                                             mapConfig: mapConfig,
                                             scheduler: MCThreadPoolScheduler.create(),
                                             pixelDensity: Float(DevicePpi.pixelsPerInch),
                                             is3D: true)!

    for layer in layers {
        mapInterface.addLayer(layer)
    }

    let device = layerRenderer.device
    let commandQueue = device.makeCommandQueue()!

    mapInterface.setViewportSize(.init(x: 2732, y: 2048))

    mapInterface.setCallbackHandler(CallbackInterface())

    mapInterface.resume()

    let session = ARKitSession()
    let worldTracking = WorldTrackingProvider()


    Task(executorPreference: RendererTaskExecutor.shared) {

        try await session.run([worldTracking])

        while true {
            switch layerRenderer.state {
                case .paused:
                    layerRenderer.waitUntilRunning()
                case .running:
                    break
                case .invalidated:
                    mapInterface.pause()
                    mapInterface.destroy()
                    return
                @unknown default:
                    break
            }

            guard let nextFrame = layerRenderer.queryNextFrame() else {
                continue
            }

            nextFrame.startUpdate()
            renderingContext.beginFrame()
            nextFrame.endUpdate()

            nextFrame.startSubmission()

            guard let drawable = nextFrame.queryDrawable() else {
                continue
            }


            // Convert the timestamps into units of seconds
            //        let anchorPredictionTime = LayerRenderer.Clock.Instant.epoch.duration(to: trackableAnchorTime)


            let time = LayerRenderer.Clock.Instant.epoch.duration(to: drawable.frameTiming.presentationTime).timeInterval

            let deviceAnchor = worldTracking.queryDeviceAnchor(atTimestamp: time)
            drawable.deviceAnchor = deviceAnchor


            let commandBuffer = commandQueue.makeCommandBuffer()!

            for (eye, _) in drawable.colorTextures.enumerated() {


                let projection = drawable.computeProjection(viewIndex: eye)
                let originFromDevice = deviceAnchor?.originFromAnchorTransform
                let view = drawable.views[eye]
                let deviceFromView = view.transform
                if let originFromDevice {
                    let viewMatrix = (originFromDevice * deviceFromView).inverse
                    // Flatten the float4x4 matrix into a 1-dimensional array of Float
                    var flattenedViewMatrix: [NSNumber] = []
                    for column in 0..<4 {
                        for row in 0..<4 {
                            flattenedViewMatrix.append(NSNumber(value: viewMatrix[column, row]))
                        }
                    }
                    var flattenedProjectionMatrix: [NSNumber] = []
                    for column in 0..<4 {
                        for row in 0..<4 {
                            flattenedProjectionMatrix.append(NSNumber(value: projection[column, row]))
                        }
                    }

                    // Convert each Float to NSNumber
                    mapInterface
                        .getCamera()?
                        .asMapCamera3d()?
                        .setHardwareMatrices(
                            flattenedViewMatrix,
                            projectionMatrix: flattenedProjectionMatrix
                        )
                }


                mapInterface.prepare()


                let renderPassDescriptor = MTLRenderPassDescriptor()

                renderPassDescriptor.colorAttachments[0].texture = drawable.colorTextures[eye]
                renderPassDescriptor.colorAttachments[0].loadAction = .clear
                renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 0.0)
                renderPassDescriptor.colorAttachments[0].storeAction = .store

                renderPassDescriptor.depthAttachment.texture = drawable.depthTextures[eye]
                renderPassDescriptor.depthAttachment.loadAction = .clear
                renderPassDescriptor.depthAttachment.storeAction = .store
                renderPassDescriptor.depthAttachment.clearDepth = 1.0


                renderPassDescriptor.stencilAttachment.texture = drawable.depthTextures[eye]
                renderPassDescriptor.stencilAttachment.loadAction = .clear
                renderPassDescriptor.stencilAttachment.storeAction = .store



                if layerRenderer.configuration.layout == .layered {
                    renderPassDescriptor.renderTargetArrayLength = drawable.views.count
                }
                else {
                    renderPassDescriptor.renderTargetArrayLength = 1
                }
//                        renderPassDescriptor.rasterizationRateMap = drawable.rasterizationRateMaps[0]

                let encoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor)!

                renderingContext.encoder = encoder


                mapInterface.drawFrame()

                encoder.endEncoding()

                renderingContext.secondEye()
            }


            drawable.encodePresent(commandBuffer: commandBuffer)
            commandBuffer.commit()
            nextFrame.endSubmission()
        }
    }


}

#endif
