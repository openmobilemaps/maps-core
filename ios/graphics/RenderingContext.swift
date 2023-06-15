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
    public weak var encoder: MTLRenderCommandEncoder?
    public weak var sceneView: MCMapView?

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

    var currentPipeline: MTLRenderPipelineState?

    func setRenderPipelineStateIfNeeded(_ pipelineState: MTLRenderPipelineState) {
        if currentPipeline?.hash == pipelineState.hash {
            return
        }
        currentPipeline = pipelineState
        encoder?.setRenderPipelineState(pipelineState)
    }


    var currentSampler: (MTLSamplerState?, Int)?
    func setFragmentSamplerState(_ sampler: MTLSamplerState?, index: Int) {
        if currentSampler?.0?.hash == sampler?.hash,
              currentSampler?.1 == index {
            return
        }
        currentSampler = (sampler, index)
        encoder?.setFragmentSamplerState(sampler, index: index)
    }

    var vertextBindings: [Int: (Int?, Int)] = [:]
    func setVertexBytes(_ bytes: UnsafeRawPointer, length: Int, index: Int) {
        if let currentBinding = vertextBindings[index],
              currentBinding.0 == bytes.hashValue,
              currentBinding.1 == length  {
            return
        }
        encoder?.setVertexBytes(bytes, length: length, index: index)
        vertextBindings[index] = (bytes.hashValue, length)
    }
    func setVertexBuffer(_ buffer: MTLBuffer?, offset: Int, index: Int) {
        if let currentBinding = vertextBindings[index],
              currentBinding.0 == buffer?.hash,
              currentBinding.1 == offset {
            return
        }
        encoder?.setVertexBuffer(buffer, offset: offset, index: index)
        vertextBindings[index] = (buffer?.hash, offset)
    }

    var fragmentBindings: [Int: (Int?, Int)] = [:]
    func setFragmentBytes(_ bytes: UnsafeRawPointer, length: Int, index: Int) {
        if let currentBinding = fragmentBindings[index],
              currentBinding.0 == bytes.hashValue,
              currentBinding.1 == length  {
            return
        }
        encoder?.setFragmentBytes(bytes, length: length, index: index)
        fragmentBindings[index] = (bytes.hashValue, length)
    }

    func setFragmentBuffer(_ buffer: MTLBuffer?, offset: Int, index: Int) {
        if let currentBinding = fragmentBindings[index],
              currentBinding.0 == buffer?.hash,
              currentBinding.1 == offset {
            return
        }
        encoder?.setFragmentBuffer(buffer, offset: offset, index: index)
        fragmentBindings[index] = (buffer?.hash, offset)
    }

    var currentDepthStencilState: MTLDepthStencilState?
    func setDepthStencilState(_ depthStencilState: MTLDepthStencilState?) {
        if currentDepthStencilState?.hash == depthStencilState?.hash {
            return
        }
        currentDepthStencilState = depthStencilState
        encoder?.setDepthStencilState(depthStencilState)
    }

    var currentTexture: (MTLTexture?, Int)?
    func setFragmentTexture(_ texture: MTLTexture?, index: Int) {
        if currentTexture?.0?.hash == texture?.hash,
              currentTexture?.1 == index {
            return
        }
        currentTexture = (texture, index)
        encoder?.setFragmentTexture(texture, index: index)
    }

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
        guard let encoder = encoder else { return }
        stencilClearQuad.render(encoder: encoder,
                                context: self,
                                renderPass: .init(renderPass: 0),
                                mvpMatrix: 0,
                                isMasked: false,
                                screenPixelAsRealMeterFactor: 1)
    }
}

extension RenderingContext: MCRenderingContextInterface {
    public func preRenderStencilMask() {
    }

    public func postRenderStencilMask() {
        clearStencilBuffer()
    }

    public func setupDrawFrame() {
        currentPipeline = nil
        currentSampler = nil
        currentTexture = nil
        currentDepthStencilState = nil
        vertextBindings.removeAll(keepingCapacity: true)
        fragmentBindings.removeAll(keepingCapacity: true)
    }

    public func onSurfaceCreated() {
    }

    public func setViewportSize(_ newSize: MCVec2I) {
        viewportSize = newSize
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
