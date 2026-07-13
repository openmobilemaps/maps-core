//
//  RenderToImageRenderPass.swift
//  MapCore
//
//  Created by Marco Zimmermann on 22.04.2025.
//

import CoreImage
@preconcurrency import Metal

class RenderToImageRenderPass {
    private var offlineRenderPass: MTLRenderPassDescriptor? = nil

    private let device: MTLDevice

    // MARK: - Init

    init(device: MTLDevice) {
        self.device = device
    }

    // MARK: - Public

    public func getRenderpass(size: CGSize, clearColor: MTLClearColor) -> MTLRenderPassDescriptor? {
        // Always allocate a fresh render pass + texture. Callers like
        // `IosOffscreenMapRenderHelper` issue two consecutive renders on the same MCMapView;
        // reusing the same texture between them risks residual state from the first render
        // bleeding into the second (the caller keeps `getImage()` around between renders).
        // The allocation cost is dominated by the ensuing tile decoding, so we don't cache.
        offlineRenderPass = getOfflineRenderPass(size: size, clearColor: clearColor)
        return offlineRenderPass
    }

    public func getImage() -> MCPlatformImage? {
        guard let texture = self.offlineRenderPass?.colorAttachments[0].texture else { return nil }

        let context = CIContext()
        let kciOptions: [CIImageOption: Any] = [
            .colorSpace: CGColorSpaceCreateDeviceRGB()
        ]

        guard let cImg = CIImage(mtlTexture: texture, options: kciOptions) else {
            return nil
        }

        return context.createCGImage(cImg, from: cImg.extent)?.toImage()
    }

    // MARK: - Private implementation details

    private func getOfflineRenderPass(size: CGSize, clearColor: MTLClearColor) -> MTLRenderPassDescriptor? {
        guard let texture = makeTexture(size: size),
            let depthStencil = makeDepthStencilTexture(size: size)
        else { return nil }

        let passDescriptor = MTLRenderPassDescriptor()
        passDescriptor.colorAttachments[0].texture = texture
        passDescriptor.colorAttachments[0].loadAction = .clear
        passDescriptor.colorAttachments[0].storeAction = .store
        passDescriptor.colorAttachments[0].clearColor = clearColor
        // Stencil
        passDescriptor.stencilAttachment.texture = depthStencil
        passDescriptor.stencilAttachment.loadAction = .clear
        passDescriptor.stencilAttachment.storeAction = .dontCare
        passDescriptor.stencilAttachment.clearStencil = 0
        // Depth
        passDescriptor.depthAttachment.texture = depthStencil
        passDescriptor.depthAttachment.loadAction = .clear
        passDescriptor.depthAttachment.storeAction = .dontCare
        passDescriptor.depthAttachment.clearDepth = 1.0
        return passDescriptor
    }

    private func makeTexture(size: CGSize) -> MTLTexture? {
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .bgra8Unorm,
            width: Int(size.width),
            height: Int(size.height),
            mipmapped: false
        )
        descriptor.usage = [.renderTarget, .shaderRead]
        return device.makeTexture(descriptor: descriptor)
    }

    private func makeDepthStencilTexture(size: CGSize) -> MTLTexture? {
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: MetalContext.depthPixelFormat,
            width: Int(size.width),
            height: Int(size.height),
            mipmapped: false
        )
        descriptor.storageMode = .private
        descriptor.usage = [.renderTarget]
        return device.makeTexture(descriptor: descriptor)
    }
}
