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
@preconcurrency import Metal
import UIKit

@objc
public class RenderingContext: NSObject, @unchecked Sendable {
    public weak var encoder: MTLRenderCommandEncoder?
    public weak var computeEncoder: MTLComputeCommandEncoder?
    public weak var sceneView: MCMapView?

    public weak var renderTarget: RenderTargetTexture?

    public static let bufferCount = 3  // Triple buffering
    private(set) var currentBufferIndex = 0

    public private(set) var time: Float = 0

    private let start = Date()

    public func beginFrame() {
        currentBufferIndex =
            (currentBufferIndex + 1) % RenderingContext.bufferCount
        time = Float(-start.timeIntervalSinceNow)
    }

    public var cullMode: MCRenderingCullMode?

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
        return MetalContext.current.device.makeDepthStencilState(
            descriptor: depthStencilDescriptor)
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
        return MetalContext.current.device.makeDepthStencilState(
            descriptor: depthStencilDescriptor)
    }()

    public lazy var defaultMask: MTLDepthStencilState? = {
        let descriptor = MTLStencilDescriptor()
        descriptor.stencilCompareFunction = .always
        descriptor.depthStencilPassOperation = .keep
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.frontFaceStencil = descriptor
        depthStencilDescriptor.backFaceStencil = descriptor
        return MetalContext.current.device.makeDepthStencilState(
            descriptor: depthStencilDescriptor)
    }()

    public var aspectRatio: Float {
        viewportQueue.sync {
            Float(viewportSize.x) / Float(viewportSize.y)
        }
    }

    private var viewportSize: MCVec2I = .init(x: 0, y: 0)
    private let viewportQueue = DispatchQueue(label: "RenderingContextViewportQueue", attributes: .concurrent)


    var isScissoringDirty = false

    var currentPipeline: MTLRenderPipelineState?

    open func setRenderPipelineStateIfNeeded(
        _ pipelineState: MTLRenderPipelineState
    ) {
        guard currentPipeline?.hash != pipelineState.hash else {
            return
        }
        currentPipeline = pipelineState
        encoder?.setRenderPipelineState(pipelineState)
    }

    /// a Quad that fills the whole viewport
    /// this is needed to clear the stencilbuffer
    lazy var stencilClearQuad: Quad2d = {
        let quad = Quad2d(
            shader: ClearStencilShader(), metalContext: .current,
            label: "ClearStencil")
        quad.setFrame(
            .init(
                topLeft: .init(x: 1, y: -1, z: 0),
                topRight: .init(x: -1, y: -1, z: 0),
                bottomRight: .init(x: -1, y: 1, z: 0),
                bottomLeft: .init(x: 1, y: 1, z: 0)),
            textureCoordinates: .init(x: 0, y: 0, width: 0, height: 0),
            origin: .init(x: 0, y: 0, z: 0), is3d: false)
        quad.setup(self)
        return quad
    }()

    public func clearStencilBuffer() {
        guard let encoder else { return }
        stencilClearQuad.render(
            encoder: encoder,
            context: self,
            renderPass: .init(renderPass: 0, isPassMasked: false),
            vpMatrix: 0,
            mMatrix: 0,
            origin: .init(x: 0, y: 0, z: 0),
            isMasked: false,
            screenPixelAsRealMeterFactor: 1)
    }
}

extension RenderingContext: MCRenderingContextInterface {
    public func setCulling(_ mode: MCRenderingCullMode) {
        self.cullMode = mode
    }

    public func preRenderStencilMask() {
    }

    public func postRenderStencilMask() {
        clearStencilBuffer()
    }

    public func setupDrawFrame() {
        currentPipeline = nil
        if let cullMode {
            /*
             Set the cullMode inverse in order to be consistent with opengl
             */
            switch cullMode {
                case .BACK:
                    encoder?.setCullMode(.front)
                case .FRONT:
                    encoder?.setCullMode(.back)
                case .NONE:
                    encoder?.setCullMode(.none)
                @unknown default:
                    assertionFailure()
            }
        }
    }

    public func onSurfaceCreated() {
    }

    public func setViewportSize(_ newSize: MCVec2I) {
        viewportQueue.async(flags: .barrier) {
            self.viewportSize = newSize
        }
    }

    public func getViewportSize() -> MCVec2I { 
        viewportQueue.sync {
            viewportSize 
        }
    }

    public func setBackgroundColor(_ color: MCColor) {
        Task { @MainActor in
            self.sceneView?.clearColor = color.metalColor
        }
    }

    public func applyScissorRect(_ scissorRect: MCRectI?) {
        if let sr = scissorRect {
            encoder?.setScissorRect(sr.scissorRect)
            isScissoringDirty = true
        } else if isScissoringDirty {
            var s = CGSize(width: 1.0, height: 1.0)
            if Thread.isMainThread {
                MainActor.assumeIsolated {
                    s =
                        self.sceneView?.frame.size
                        ?? CGSize(width: 1.0, height: 1.0)
                    s.width = UIScreen.main.nativeScale * s.width
                    s.height = UIScreen.main.nativeScale * s.height
                }
            } else {
                DispatchQueue.main.sync {
                    s =
                        self.sceneView?.frame.size
                        ?? CGSize(width: 1.0, height: 1.0)
                    s.width = UIScreen.main.nativeScale * s.width
                    s.height = UIScreen.main.nativeScale * s.height
                }
            }

            var size = viewportSize.scissorRect
            size.width = min(size.width, Int(s.width))
            size.height = min(size.height, Int(s.height))

            encoder?.setScissorRect(size)
            isScissoringDirty = false
        }
    }
}

extension MCRectI {
    fileprivate var scissorRect: MTLScissorRect {
        MTLScissorRect(
            x: Int(x), y: Int(y), width: Int(width), height: Int(height))
    }
}

extension MCVec2I {
    fileprivate var scissorRect: MTLScissorRect {
        MTLScissorRect(x: 0, y: 0, width: Int(x), height: Int(y))
    }
}
