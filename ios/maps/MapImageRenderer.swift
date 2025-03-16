//
//  MapImageRenderer.swift
//  MapCore
//
//  Created by Nicolas Märki on 15.03.2025.
//

/*

public class MapImageRenderer {

    public nonisolated(unsafe) let mapInterface: MCMapInterface
    private let renderingContext: RenderingContext = RenderingContext()

    private lazy var renderToImageQueue = DispatchQueue(
        label: "io.openmobilemaps.renderToImagQueue", qos: .userInteractive)

    func texture() {
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .stencil8,
            width: Int(metalLayer.drawableSize.width),
            height: Int(metalLayer.drawableSize.height),
            mipmapped: false
        )
        descriptor.usage = [.renderTarget]
        descriptor.storageMode = .private

        if let texture = MetalContext.current.device.makeTexture(descriptor: descriptor) {
            stencilTextures.append(texture)
        }
    }

    public func prepareRendering() {

    }

    private func renderFrame() {

        print("Render Frame")

        let size = metalLayer.drawableSize
        guard size != .zero else {
            return
        }

        // Ensure that triple-buffers are not over-used
        renderSemaphore.wait()

        renderingContext.beginFrame()
        mapInterface.prepare()

        // Shared lib stuff

        if size != lastSize {
            mapInterface.setViewportSize(size.vec2)
            sizeDelegate?.sizeChanged()
            lastSize = size

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
                size: size.vec2,
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

        commandBuffer.commit()
        commandBuffer.waitUntilCompleted()
    }

}



 private class MCMapViewMapReadyCallbacks: @preconcurrency
 MCMapReadyCallbackInterface, @unchecked Sendable
 {
 public nonisolated(unsafe) weak var delegate: MCMapView?
 public var callback: ((UIImage?, MCLayerReadyState) -> Void)?
 public var callbackQueue: DispatchQueue?
 public let semaphore = DispatchSemaphore(value: 1)

 @MainActor func stateDidUpdate(_ state: MCLayerReadyState) {
 guard let delegate = self.delegate else { return }

 semaphore.wait()

 //        delegate.draw(in: delegate)

 callbackQueue?
 .async {
 switch state {
 case .NOT_READY:
 break
 case .ERROR, .TIMEOUT_ERROR:
 self.callback?(nil, state)
 case .READY:
 MainActor.assumeIsolated {
 self.callback?(delegate.currentDrawableImage(), state)
 }
 @unknown default:
 break
 }
 self.semaphore.signal()
 }
 }
 }

 extension MCMapView {

 public func renderToImage(
 size: CGSize, timeout: Float, bounds: MCRectCoord,
 callbackQueue: DispatchQueue = .main,
 callback: @escaping @Sendable (UIImage?, MCLayerReadyState) -> Void
 ) {
 renderToImageQueue.async {
 DispatchQueue.main.sync {
 self.frame = CGRect(
 origin: .zero,
 size: .init(
 width: size.width / UIScreen.main.scale,
 height: size.height / UIScreen.main.scale))
 self.setNeedsLayout()
 self.layoutIfNeeded()
 }

 let mapReadyCallbacks = MCMapViewMapReadyCallbacks()
 mapReadyCallbacks.delegate = self
 mapReadyCallbacks.callback = callback
 mapReadyCallbacks.callbackQueue = callbackQueue

 self.mapInterface.drawReadyFrame(
 bounds, timeout: timeout, callbacks: mapReadyCallbacks)
 }
 }
 }

 extension MCMapView {
 fileprivate func currentDrawableImage() -> UIImage? {
 self.saveDrawable = true
 self.invalidate()
 //        self.draw(in: self)
 self.saveDrawable = false

 //        guard let texture = self.currentDrawable?.texture else { return nil }

 //        let context = CIContext()
 //        let kciOptions: [CIImageOption: Any] = [
 //            .colorSpace: CGColorSpaceCreateDeviceRGB()
 //        ]
 //        let cImg = CIImage(mtlTexture: texture, options: kciOptions)!
 //        return context.createCGImage(cImg, from: cImg.extent)?.toImage()
 return UIImage()
 }
 }

 extension CGImage {
 fileprivate func toImage() -> UIImage? {
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
 */
