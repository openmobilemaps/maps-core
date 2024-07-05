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
public func MCMapRenderLoop(_ layerRenderer: LayerRenderer) {
    let mapConfig = MCMapConfig(mapCoordinateSystem: MCCoordinateSystemFactory.getUnitSphereSystem())

    let renderingContext = RenderingContext()
    let mapInterface = MCMapInterface.create(GraphicsFactory(),
                                             shaderFactory: ShaderFactory(),
                                             renderingContext: renderingContext,
                                             mapConfig: mapConfig,
                                             scheduler: MCThreadPoolScheduler.create(),
                                             pixelDensity: Float(DevicePpi.pixelsPerInch),
                                             is3D: true)!

    let rasterLayer = TiledRasterLayer(webMercatorUrlFormat: "https://a.tile.openstreetmap.org/{z}/{x}/{y}.png")

    mapInterface.addLayer(rasterLayer.interface)

    let device = layerRenderer.device
    let commandQueue = device.makeCommandQueue()!

    mapInterface.setViewportSize(.init(x: 2732, y: 2048))

    mapInterface.setCallbackHandler(CallbackInterface())

    mapInterface.resume()

    let session = ARKitSession()
    let worldTracking = WorldTrackingProvider()

    Task {
        try await session.run([worldTracking])
    }

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
        nextFrame.endUpdate()
        nextFrame.startSubmission()

        let drawable = nextFrame.queryDrawable()!


        // Convert the timestamps into units of seconds
//        let anchorPredictionTime = LayerRenderer.Clock.Instant.epoch.duration(to: trackableAnchorTime)


        let deviceAnchor = worldTracking.queryDeviceAnchor(atTimestamp: CACurrentMediaTime())
        drawable.deviceAnchor = deviceAnchor


        let commandBuffer = commandQueue.makeCommandBuffer()!



        for (eye, _) in drawable.colorTextures.enumerated() {

            let renderPassDescriptor = MTLRenderPassDescriptor()

            renderPassDescriptor.colorAttachments[0].texture = drawable.colorTextures[eye]
            renderPassDescriptor.colorAttachments[0].loadAction = .clear
            renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColor(red: 0.0, green: 0.0, blue: 0.0, alpha: 0.0)
            renderPassDescriptor.colorAttachments[0].storeAction = .store

            renderPassDescriptor.depthAttachment.texture = drawable.depthTextures[eye]
            renderPassDescriptor.depthAttachment.loadAction = .clear
            renderPassDescriptor.depthAttachment.storeAction = .store
            renderPassDescriptor.depthAttachment.clearDepth = 0.0

            renderPassDescriptor.stencilAttachment.texture = drawable.depthTextures[eye]
            renderPassDescriptor.stencilAttachment.loadAction = .clear
            renderPassDescriptor.stencilAttachment.storeAction = .store

            let projection = drawable.computeProjection(viewIndex: eye)
            let originFromDevice = deviceAnchor?.originFromAnchorTransform
            let view = drawable.views[eye]
            let deviceFromView = view.transform
            if let originFromDevice {
                let viewMatrix = projection * (originFromDevice * deviceFromView).inverse
                // Flatten the float4x4 matrix into a 1-dimensional array of Float
                var flattenedMatrix: [Float] = []
                for column in 0..<4 {
                    for row in 0..<4 {
                        flattenedMatrix.append(viewMatrix[column, row])
                    }
                }

                // Convert each Float to NSNumber
                let nsnumberArray: [NSNumber] = flattenedMatrix.map { NSNumber(value: $0) }
                mapInterface.getCamera()?.asMapCamera3d()?.setHardwareVpMatrix(nsnumberArray)
            }

            if layerRenderer.configuration.layout == .layered {
                renderPassDescriptor.renderTargetArrayLength = drawable.views.count
            }
            else {
                renderPassDescriptor.renderTargetArrayLength = 1
            }
            //        renderPassDescriptor.rasterizationRateMap = drawable.rasterizationRateMaps[0]

            let encoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor)!

            renderingContext.encoder = encoder


            mapInterface.drawFrame()

            encoder.endEncoding()
        }


        drawable.encodePresent(commandBuffer: commandBuffer)
        commandBuffer.commit()
        nextFrame.endSubmission()
    }
}

#endif
