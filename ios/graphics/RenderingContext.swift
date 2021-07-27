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

@objc
class RenderingContext: NSObject {
    weak var encoder: MTLRenderCommandEncoder?
    weak var sceneView: MCMapView?

    lazy var mask: MTLDepthStencilState? = {
        let descriptor = MTLStencilDescriptor()
        descriptor.stencilCompareFunction = .always
        descriptor.stencilFailureOperation = .keep
        descriptor.depthFailureOperation = .keep
        descriptor.depthStencilPassOperation = .replace
        descriptor.writeMask = 0b10000000
        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.frontFaceStencil = descriptor
        depthStencilDescriptor.backFaceStencil = descriptor
        return MetalContext.current.device.makeDepthStencilState(descriptor: depthStencilDescriptor)
    }()

    var viewportSize: MCVec2I = .init(x: 0, y: 0)

    /// a Quad that fills the whole viewport
    /// this is needed to clear the stencilbuffer
    lazy var stencilClearQuad: Quad2d = {
        let quad = Quad2d(shader: ClearStencilShader(), metalContext: .current)
        quad.setFrame(.init(topLeft: .init(x: -1, y: 1),
                            topRight: .init(x: 1, y: 1),
                            bottomRight: .init(x: 1, y: -1),
                            bottomLeft: .init(x: -1, y: -1)),
                      textureCoordinates: .init())
        return quad
    }()

    func clearStencilBuffer() {
        guard let encoder = encoder else { return }
        stencilClearQuad.render(encoder: encoder,
                                context: self,
                                renderPass: .init(),
                                mvpMatrix: 0,
                                isMasked: false,
                                screenPixelAsRealMeterFactor: 1)
    }
}

extension RenderingContext: MCRenderingContextInterface {
    func preRenderStencilMask() { }

    func postRenderStencilMask() {
        clearStencilBuffer()
    }

    func setupDrawFrame() {}

    func onSurfaceCreated() {}

    func setViewportSize(_ newSize: MCVec2I) {
        viewportSize = newSize
    }

    func getViewportSize() -> MCVec2I { viewportSize }

    func setBackgroundColor(_ color: MCColor) {
        sceneView?.clearColor = color.metalColor
    }
}
