//
//  RenderTargetTexture.swift
//
//
//  Created by Nicolas Märki on 28.09.22.
//

import Foundation
import Metal
import MapCoreSharedModule
import CoreImage
import UIKit

open class RenderTargetTexture: Identifiable, Equatable, MCRenderTargetTexture {
    public let id = UUID()

    var texture: (any MTLTexture)?
    var stencilTexture: (any MTLTexture)?
    var holder: TextureHolder?

    let renderPipelineState: MTLRenderPipelineState
    let renderPassDescriptor = MTLRenderPassDescriptor()

    public init(_ size: MCVec2I) {
        let pipelineStateDescriptor = PipelineDescriptorFactory.pipelineDescriptor(pipeline: .alphaShader, depth: false)
        pipelineStateDescriptor.label = "Offscreen Render Pipeline"
        pipelineStateDescriptor.sampleCount = 1
        pipelineStateDescriptor.colorAttachments[0]?.pixelFormat = MetalContext.colorPixelFormat
        renderPipelineState = try! MetalContext.current.device.makeRenderPipelineState(descriptor: pipelineStateDescriptor)

        renderPassDescriptor.colorAttachments[0]?.loadAction = .clear
        renderPassDescriptor.colorAttachments[0]?.clearColor = .init(red: 0, green: 0, blue: 0.0, alpha: 0.0)
        renderPassDescriptor.colorAttachments[0]?.storeAction = .store
        renderPassDescriptor.stencilAttachment.storeAction = .dontCare

        setSize(size)
    }

    public func prepareOffscreenEncoder(_ commandBuffer: MTLCommandBuffer?) -> MTLRenderCommandEncoder? {

        if let renderEncoder = commandBuffer?.makeRenderCommandEncoder(descriptor: renderPassDescriptor) {
            renderEncoder.setRenderPipelineState(renderPipelineState)
            renderEncoder.label = "Offscreen Encoder"
            return renderEncoder
        }

        return nil
    }

    private var lastSize: MCVec2I = MCVec2I(x: 0, y: 0)

    public func setSize(_ newSize: MCVec2I) {
        guard lastSize.x != newSize.x || lastSize.y != newSize.y else {
            return
        }
        lastSize = newSize

        let texDescriptor = MTLTextureDescriptor()
        texDescriptor.textureType = .type2D
        texDescriptor.pixelFormat = MetalContext.colorPixelFormat
        texDescriptor.usage = [.renderTarget, .shaderRead]
        texDescriptor.storageMode = .private
        texDescriptor.width = Int(newSize.x)
        texDescriptor.height = Int(newSize.y)

        texture = MetalContext.current.device.makeTexture(descriptor: texDescriptor)
        renderPassDescriptor.colorAttachments[0]?.texture = texture
        if let texture {
            holder = TextureHolder(texture)
        }

        texDescriptor.pixelFormat = .stencil8
        stencilTexture = MetalContext.current.device.makeTexture(descriptor: texDescriptor)
        renderPassDescriptor.stencilAttachment.texture = stencilTexture
    }

    public func textureHolder() -> MCTextureHolderInterface? {
        return holder
    }

    public static func == (lhs: RenderTargetTexture, rhs: RenderTargetTexture) -> Bool {
        return lhs.id == rhs.id
    }
}
