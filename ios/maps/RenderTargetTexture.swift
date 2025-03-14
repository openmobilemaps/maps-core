//
//  RenderTargetTexture.swift
//  MapCore
//
//  Created by Nicolas Märki on 15.01.2025.
//

//
//  RenderTargetTexture.swift
//
//
//  Created by Nicolas Märki on 28.09.22.
//

import CoreImage
import Foundation
import MapCoreSharedModule
import Metal
import UIKit

open class RenderTargetTexture: Identifiable, Equatable, MCRenderTargetInterface {
    public let name: String

    private var textures: [(color: MTLTexture, stencilDepth: MTLTexture)] = []

    public init(name: String = UUID().uuidString) {
        self.name = name
    }

    public func prepareOffscreenEncoder(_ commandBuffer: MTLCommandBuffer?, size: MCVec2I, context: RenderingContext) -> MTLRenderCommandEncoder? {

        setSize(size)

        let renderPassDescriptor = MTLRenderPassDescriptor()
        renderPassDescriptor.colorAttachments[0]?.loadAction = .clear
        renderPassDescriptor.colorAttachments[0]?.clearColor = .init(red: 0, green: 0, blue: 0.0, alpha: 0.0)
        renderPassDescriptor.colorAttachments[0]?.storeAction = .store
        renderPassDescriptor.stencilAttachment.storeAction = .dontCare

        renderPassDescriptor
            .colorAttachments[0].texture = textures[context.currentBufferIndex].color
        renderPassDescriptor.stencilAttachment.texture = textures[context.currentBufferIndex].stencilDepth

        if let renderEncoder = commandBuffer?.makeRenderCommandEncoder(descriptor: renderPassDescriptor) {
            renderEncoder.label = "Offscreen Encoder (\(name))"
            return renderEncoder
        }

        return nil
    }

    private var lastSize: MCVec2I = MCVec2I(x: 0, y: 0)

    private func setSize(_ newSize: MCVec2I) {
        guard lastSize.x != newSize.x || lastSize.y != newSize.y else {
            return
        }
        lastSize = newSize

        let texDescriptor = MTLTextureDescriptor()
        texDescriptor.textureType = .type2D
        texDescriptor.usage = [.renderTarget, .shaderRead]
        texDescriptor.storageMode = .private
        texDescriptor.width = Int(newSize.x)
        texDescriptor.height = Int(newSize.y)

        for _ in 0..<RenderingContext.bufferCount {
            texDescriptor.pixelFormat = MetalContext.current.colorPixelFormat
            let texture = MetalContext.current.device.makeTexture(descriptor: texDescriptor)!

            texDescriptor.pixelFormat = .stencil8
            let stencilTexture = MetalContext.current.device.makeTexture(descriptor: texDescriptor)!

            textures.append((color: texture, stencilDepth: stencilTexture))
        }
    }

    public func texture(context: RenderingContext) -> MTLTexture? {
        return textures[context.currentBufferIndex].color
    }

    public static func == (lhs: RenderTargetTexture, rhs: RenderTargetTexture) -> Bool {
        return lhs.id == rhs.id
    }
}
