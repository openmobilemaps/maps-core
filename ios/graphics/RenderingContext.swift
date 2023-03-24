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

    public var encoder: MTLRenderCommandEncoder?

    public var currentMainEncoder: MTLRenderCommandEncoder?
    public var currentCommandBuffer: MTLCommandBuffer?
    public var viewRenderPassDescriptor: MTLRenderPassDescriptor?

    public weak var sceneView: MCMapView?

    private(set) var encoderUsesDepthBuffer: Bool!

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
        descriptor.writeMask = 0b1111_1111
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.frontFaceStencil = descriptor
        depthStencilDescriptor.backFaceStencil = descriptor
        return MetalContext.current.device.makeDepthStencilState(descriptor: depthStencilDescriptor)
    }()

    public lazy var mask3d: MTLDepthStencilState? = {
        let descriptor = MTLStencilDescriptor()
        descriptor.stencilCompareFunction = .always
        descriptor.stencilFailureOperation = .keep
        descriptor.depthFailureOperation = .keep
        descriptor.depthStencilPassOperation = .replace
        descriptor.writeMask = 0b1100_0000
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.frontFaceStencil = descriptor
        depthStencilDescriptor.backFaceStencil = descriptor
        depthStencilDescriptor.depthCompareFunction = .lessEqual
        depthStencilDescriptor.isDepthWriteEnabled = true
        return MetalContext.current.device.makeDepthStencilState(descriptor: depthStencilDescriptor)
    }()

    public lazy var polygonMask3d: MTLDepthStencilState? = {
        let descriptor = MTLStencilDescriptor()
        descriptor.stencilCompareFunction = .always
        descriptor.stencilFailureOperation = .keep
        descriptor.depthFailureOperation = .keep
        descriptor.depthStencilPassOperation = .replace
        descriptor.writeMask = 0b1100_0000
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.frontFaceStencil = descriptor
        depthStencilDescriptor.backFaceStencil = descriptor
        depthStencilDescriptor.depthCompareFunction = .always
        depthStencilDescriptor.isDepthWriteEnabled = true
        return MetalContext.current.device.makeDepthStencilState(descriptor: depthStencilDescriptor)
    }()

    public lazy var defaultMask3d: MTLDepthStencilState? = {
        let descriptor = MTLStencilDescriptor()
        descriptor.stencilCompareFunction = .always
        descriptor.depthStencilPassOperation = .keep
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.frontFaceStencil = descriptor
        depthStencilDescriptor.backFaceStencil = descriptor
        depthStencilDescriptor.depthCompareFunction = .lessEqual
        depthStencilDescriptor.isDepthWriteEnabled = true
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
                      textureCoordinates: .init(x: 0, y: 0, width: 0, height: 0))
        quad.setup(self)
        return quad
    }()

    public func clearStencilBuffer() {
        let pass =  MCRenderPassConfig(renderPass: 0)
        guard let encoder = encoder else { return }
        stencilClearQuad.render(encoder: encoder,
                                context: self,
                                renderPass: pass,
                                mvpMatrix: 0,
                                isMasked: false,
                                screenPixelAsRealMeterFactor: 1)
    }
}

extension RenderingContext: MCRenderingContextInterface {

    public func preRenderStencilMask(_ target: MCRenderTargetTexture?) {
    }

    public func postRenderStencilMask(_ target: MCRenderTargetTexture?) {
        clearStencilBuffer()
    }

    public func setupDrawFrame(_ target: MCRenderTargetTexture?) {
        if let offScreenTexture = target as? RenderTargetTexture {
            if encoder != nil {
                currentMainEncoder = nil
                encoder?.endEncoding()
                encoder = nil
            }
            encoder = offScreenTexture.prepareOffscreenEncoder(currentCommandBuffer)
            encoderUsesDepthBuffer = false
        } else if let currentMainEncoder = currentMainEncoder {
            encoder = currentMainEncoder
            encoderUsesDepthBuffer = true
        } else if let buffer = currentCommandBuffer,
                 let descriptor = viewRenderPassDescriptor  {
            encoder = buffer.makeRenderCommandEncoder(descriptor: descriptor)
            currentMainEncoder = encoder
            encoderUsesDepthBuffer = true
            encoder?.label = "MainEncoder"
        }
    }

    public func endDrawFrame(_ target: MCRenderTargetTexture?) {
        if (encoder?.hash != currentMainEncoder?.hash) {
            encoder?.endEncoding()
            encoder = nil
        }
    }

    public func onSurfaceCreated() {
    }

    public func setViewportSize(_ newSize: MCVec2I) {
        viewportSize = newSize

////        let depthStencilDescriptor = MTLDepthStencilDescriptor()
////        depthStencilDescriptor.depthCompareFunction = .less
////        depthStencilDescriptor.isDepthWriteEnabled = true
////        depthStencilState = MetalContext.current.device.makeDepthStencilState(descriptor: depthStencilDescriptor)!
//
//        let depthTextureDescriptor = MTLTextureDescriptor.texture2DDescriptor(
//            pixelFormat: .depth32Float,
//            width: Int(newSize.x),
//            height: Int(newSize.y),
//            mipmapped: false
//        )
//        depthTextureDescriptor.usage = .renderTarget
//        depthTextureDescriptor.storageMode = .private
//        depthTextureDescriptor.resourceOptions = .storageModePrivate
//        mask.
    }

    public func getViewportSize() -> MCVec2I { viewportSize }

    public func setBackgroundColor(_ color: MCColor) {
        sceneView?.clearColor = color.metalColor
    }

    public func applyScissorRect(_ scissorRect: MCRectI?) {
        if let sr = scissorRect {
            encoder?.setScissorRect(sr.scissorRect)
            isScissoringDirty = true
        } else if isScissoringDirty {
            var s = self.sceneView?.frame.size ?? CGSize(width: 1.0, height: 1.0)
            s.width = UIScreen.main.nativeScale * s.width
            s.height = UIScreen.main.nativeScale * s.height

            var size = viewportSize.scissorRect
            size.width = min(size.width, Int(s.width))
            size.height = min(size.height, Int(s.height))

            encoder?.setScissorRect(size)
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
