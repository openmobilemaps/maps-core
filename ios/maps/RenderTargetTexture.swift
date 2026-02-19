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
@preconcurrency import Metal
import UIKit

open class RenderTargetTexture: Identifiable, Equatable, MCRenderTargetInterface {
    public let name: String

    private var textures: [(color: MTLTexture, depthStencil: MTLTexture)] = []

    public init(name: String = UUID().uuidString) {
        self.name = name
    }

    public func prepareOffscreenEncoder(_ commandBuffer: MTLCommandBuffer?, size: MCVec2I, context: RenderingContext) -> MTLRenderCommandEncoder? {

        if size.x == 0 || size.y == 0 {
            return nil
        }

        setSize(size)

        let renderPassDescriptor = MTLRenderPassDescriptor()
        renderPassDescriptor.colorAttachments[0]?.loadAction = .clear
        renderPassDescriptor.colorAttachments[0]?.clearColor = .init(red: 0, green: 0, blue: 0.0, alpha: 0.0)
        renderPassDescriptor.colorAttachments[0]?.storeAction = .store
        renderPassDescriptor.depthAttachment.loadAction = .clear
        renderPassDescriptor.depthAttachment.storeAction = .dontCare
        renderPassDescriptor.depthAttachment.clearDepth = 1.0
        renderPassDescriptor.stencilAttachment.loadAction = .clear
        renderPassDescriptor.stencilAttachment.storeAction = .dontCare
        renderPassDescriptor.stencilAttachment.clearStencil = 0

        renderPassDescriptor
            .colorAttachments[0].texture = textures[context.currentBufferIndex].color
        renderPassDescriptor.depthAttachment.texture = textures[context.currentBufferIndex].depthStencil
        renderPassDescriptor.stencilAttachment.texture = textures[context.currentBufferIndex].depthStencil

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

        textures.removeAll(keepingCapacity: true)

        let colorDescriptor = MTLTextureDescriptor()
        colorDescriptor.textureType = .type2D
        colorDescriptor.usage = [.renderTarget, .shaderRead]
        colorDescriptor.storageMode = .private
        colorDescriptor.width = Int(newSize.x)
        colorDescriptor.height = Int(newSize.y)
        colorDescriptor.pixelFormat = MetalContext.colorPixelFormat

        let depthStencilDescriptor = MTLTextureDescriptor()
        depthStencilDescriptor.textureType = .type2D
        depthStencilDescriptor.usage = [.renderTarget]
        depthStencilDescriptor.storageMode = .private
        depthStencilDescriptor.width = Int(newSize.x)
        depthStencilDescriptor.height = Int(newSize.y)
        depthStencilDescriptor.pixelFormat = MetalContext.depthPixelFormat

        for _ in 0..<RenderingContext.bufferCount {
            guard
                let texture = MetalContext.current.device.makeTexture(descriptor: colorDescriptor),
                let depthStencilTexture = MetalContext.current.device.makeTexture(descriptor: depthStencilDescriptor)
            else {
                textures.removeAll(keepingCapacity: false)
                return
            }

            textures.append((color: texture, depthStencil: depthStencilTexture))
        }
    }

    public func texture(context: RenderingContext) -> MTLTexture? {
        return textures.indices.contains(context.currentBufferIndex) ? textures[context.currentBufferIndex].color : nil
    }

    public static func == (lhs: RenderTargetTexture, rhs: RenderTargetTexture) -> Bool {
        return lhs.id == rhs.id
    }

    public func asGlRenderTargetInterface() -> (any MCOpenGlRenderTargetInterface)? {
        nil
    }
}
