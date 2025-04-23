//
//  RenderToImageRenderPass.swift
//  MapCore
//
//  Created by Marco Zimmermann on 22.04.2025.
//

import MapKit

class RenderToImageRenderPass {
    private var offlineRenderPass : MTLRenderPassDescriptor? = nil

    private let device : MTLDevice
    private var lastSize : CGSize = .zero

    // MARK: - Init

    init(device: MTLDevice) {
        self.device = device
    }

    // MARK: - Public

    public func getRenderpass(size: CGSize)  -> MTLRenderPassDescriptor? {
        if let orp = offlineRenderPass, size == lastSize {
            return orp
        }

        offlineRenderPass = getOfflineRenderPass(size: size)
        return offlineRenderPass
    }

    public func getImage() -> UIImage? {
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

    private func getOfflineRenderPass(size: CGSize) -> MTLRenderPassDescriptor? {
        guard let texture = makeTexture(size: size),
              let stencil = makeStencilTexture(size: size)
        else { return nil }

        let passDescriptor = MTLRenderPassDescriptor()
        passDescriptor.colorAttachments[0].texture = texture
        passDescriptor.colorAttachments[0].loadAction = .clear
        passDescriptor.colorAttachments[0].storeAction = .store
        passDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1)
        // Stencil
        passDescriptor.stencilAttachment.texture = stencil
        passDescriptor.stencilAttachment.loadAction = .clear
        passDescriptor.stencilAttachment.storeAction = .dontCare
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

    private func makeStencilTexture(size: CGSize) -> MTLTexture? {
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: .stencil8,
            width: Int(size.width),
            height: Int(size.height),
            mipmapped: false
        )
        descriptor.usage = [.renderTarget]
        return device.makeTexture(descriptor: descriptor)
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
