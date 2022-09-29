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

    open var encoder: MTLRenderCommandEncoder?

    weak var context: RenderingContext?

    public let id = UUID()

    var texture: (any MTLTexture)?
    var holder: TextureHolder?

    public init() {

    }

    public func prepareRender() {
        let renderPassDescriptor = MTLRenderPassDescriptor()
        renderPassDescriptor.colorAttachments[0]?.texture = texture
    }

    private var lastSize: MCVec2I = MCVec2I(x: 0, y: 0)

    public func setViewportSize(_ newSize: MCVec2I) {
        guard lastSize.x != newSize.x || lastSize.y != newSize.y else {
            return
        }
        lastSize = newSize

        let texDescriptor = MTLTextureDescriptor()
        texDescriptor.textureType = .type2D
        texDescriptor.pixelFormat = .rgba8Unorm
        texDescriptor.usage = [.renderTarget, .shaderRead]
        texDescriptor.width = Int(newSize.x)
        texDescriptor.height = Int(newSize.y)

        texture = MetalContext.current.device.makeTexture(descriptor: texDescriptor)
        if let texture {
            holder = TextureHolder(texture)
        }
    }

    public func textureHolder() -> MCTextureHolderInterface? {
        let image = CIImage(mtlTexture: holder!.texture)!
        let uiImage = UIImage(ciImage: image)
        print(uiImage)
        return holder
    }

    public func getIndex() -> Int32? {
        context?.getIndex(of: self)
    }

    public static func == (lhs: RenderTargetTexture, rhs: RenderTargetTexture) -> Bool {
        return lhs.id == rhs.id
    }

}
