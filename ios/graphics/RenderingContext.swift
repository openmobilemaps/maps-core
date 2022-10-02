/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

import Foundation
import MapCoreSharedModule
import Metal
import UIKit

@objc
public class RenderingContext: NSObject {

    public func encoder(pass: MCRenderPassConfig) -> MTLRenderCommandEncoder? {
        return currentEncoder!
    }

    public var currentCommandBuffer: MTLCommandBuffer?
    public var currentEncoder: MTLRenderCommandEncoder?
    public weak var sceneView: MCMapView?

    public var viewRenderPassDescriptor: MTLRenderPassDescriptor?

    private var renderTargetTextures = NSHashTable<RenderTargetTexture>.weakObjects()

    public lazy var mask: MTLDepthStencilState? = {
        let descriptor = MTLStencilDescriptor()
        descriptor.stencilCompareFunction = .always
        descriptor.stencilFailureOperation = .keep
        descriptor.depthFailureOperation = .keep
        descriptor.depthStencilPassOperation = .replace
        descriptor.writeMask = 0b1100_0000
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.frontFaceStencil = descriptor
        depthStencilDescriptor.backFaceStencil = descriptor
        return MetalContext.current.device.makeDepthStencilState(descriptor: depthStencilDescriptor)
    }()

    public lazy var polygonMask: MTLDepthStencilState? = {
        let descriptor = MTLStencilDescriptor()
        descriptor.stencilCompareFunction = .always
        descriptor.stencilFailureOperation = .keep
        descriptor.depthFailureOperation = .keep
        descriptor.depthStencilPassOperation = .replace
        descriptor.writeMask = 0b1100_0000
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.frontFaceStencil = descriptor
        depthStencilDescriptor.backFaceStencil = descriptor
        return MetalContext.current.device.makeDepthStencilState(descriptor: depthStencilDescriptor)
    }()

    public lazy var defaultMask: MTLDepthStencilState? = {
        let descriptor = MTLStencilDescriptor()
        descriptor.stencilCompareFunction = .always
        descriptor.depthStencilPassOperation = .keep
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.frontFaceStencil = descriptor
        depthStencilDescriptor.backFaceStencil = descriptor
        return MetalContext.current.device.makeDepthStencilState(descriptor: depthStencilDescriptor)
    }()

    public var viewportSize: MCVec2I = .init(x: 0, y: 0)

    var isScissoringDirty = false

    /// a Quad that fills the whole viewport
    /// this is needed to clear the stencilbuffer
    lazy var stencilClearQuad: Quad2d = {
        let quad = Quad2d(shader: ClearStencilShader(), metalContext: .current, label: "ClearStencil")
        quad.setFrame(.init(topLeft: .init(x: -1, y: 1),
                            topRight: .init(x: 1, y: 1),
                            bottomRight: .init(x: 1, y: -1),
                            bottomLeft: .init(x: -1, y: -1)),
                      textureCoordinates: .init())
        quad.setup(self)
        return quad
    }()

    public func clearStencilBuffer(_ renderPassConfig: MCRenderPassConfig) {
        guard let encoder = encoder(pass: renderPassConfig) else { return }
        stencilClearQuad.render(encoder: encoder,
                                context: self,
                                renderPass: .init(),
                                mvpMatrix: 0,
                                isMasked: false,
                                screenPixelAsRealMeterFactor: 1)
    }
}

extension RenderingContext: MCRenderingContextInterface {
    public func createRenderTargetTexture() -> MCRenderTargetTexture? {
        let texture = RenderTargetTexture()
        texture.setViewportSize(viewportSize)
        renderTargetTextures.add(texture)
        texture.context = self
        return texture
    }

    public func preRenderStencilMask(_ renderPassConfig: MCRenderPassConfig) {
    }

    public func postRenderStencilMask(_ renderPassConfig: MCRenderPassConfig) {
        clearStencilBuffer(renderPassConfig)
    }

    public func setupDrawFrame(_ renderTargetTexture: MCRenderTargetTexture?) {
        if let texture = renderTargetTexture as? RenderTargetTexture,
        let buffer = currentCommandBuffer {
            currentEncoder = texture.prepareOffscreenEncoder(buffer)
        }
        else if let buffer = currentCommandBuffer,
                    let descriptor = viewRenderPassDescriptor  {
            currentEncoder = buffer.makeRenderCommandEncoder(descriptor: descriptor)
        }
    }

    public func endDrawFrame(_ renderTargetTexture: MCRenderTargetTexture?) {
        currentEncoder?.endEncoding()
        currentEncoder = nil
    }

    public func onSurfaceCreated() {
    }

    public func setViewportSize(_ newSize: MCVec2I) {
        viewportSize = newSize
        for texture in renderTargetTextures.allObjects {
            texture.setViewportSize(newSize)
        }
    }

    public func getViewportSize() -> MCVec2I { viewportSize }

    public func setBackgroundColor(_ color: MCColor) {
        sceneView?.clearColor = color.metalColor
    }

    public func applyScissorRect(_ scissorRect: MCRectI?, pass: MCRenderPassConfig) {
        if let sr = scissorRect {
            encoder(pass: pass)?.setScissorRect(sr.scissorRect)
            isScissoringDirty = true
        } else if isScissoringDirty {
            var s = self.sceneView?.frame.size ?? CGSize(width: 1.0, height: 1.0)
            s.width = UIScreen.main.nativeScale * s.width
            s.height = UIScreen.main.nativeScale * s.height

            var size = viewportSize.scissorRect
            size.width = min(size.width, Int(s.width))
            size.height = min(size.height, Int(s.height))

            encoder(pass: pass)?.setScissorRect(size)
            isScissoringDirty = false
        }
    }
}

private extension MCRectI {
    var scissorRect: MTLScissorRect {
        MTLScissorRect(x: Int(x), y: Int(y), width: Int(width), height: Int(height))
    }
}

private extension MCVec2I {
    var scissorRect: MTLScissorRect {
        MTLScissorRect(x: 0, y: 0, width: Int(x), height: Int(y))
    }
}
