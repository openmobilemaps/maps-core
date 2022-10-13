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

final class PolygonGroup2d: BaseGraphicsObject {
    private var shader: PolygonGroupShader

    private var verticesBuffer: MTLBuffer?
    private var indicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0

    private var stencilState: MTLDepthStencilState?

    init(shader: MCShaderProgramInterface, metalContext: MetalContext) {
        guard let shader = shader as? PolygonGroupShader else {
            fatalError("PolygonGroup2d only supports PolygonGroupShader")
        }
        self.shader = shader
        super.init(device: metalContext.device,
                   sampler: metalContext.samplerLibrary.value(Sampler.magLinear.rawValue))
    }

    private func setupStencilStates() {
        let ss2 = MTLStencilDescriptor()
        ss2.stencilCompareFunction = .equal
        ss2.stencilFailureOperation = .zero
        ss2.depthFailureOperation = .keep
        ss2.depthStencilPassOperation = .keep
        ss2.readMask = 0b1111_1111
        ss2.writeMask = 0b0000_0000

        let s2 = MTLDepthStencilDescriptor()
        s2.frontFaceStencil = ss2
        s2.backFaceStencil = ss2

        stencilState = device.makeDepthStencilState(descriptor: s2)
    }

    override func render(encoder: MTLRenderCommandEncoder,
                         context: RenderingContext,
                         renderPass _: MCRenderPassConfig,
                         mvpMatrix: Int64,
                         isMasked: Bool,
                         screenPixelAsRealMeterFactor _: Double) {
        guard let verticesBuffer = verticesBuffer,
              let indicesBuffer = indicesBuffer else { return }

        #if DEBUG
        encoder.pushDebugGroup("PolygonGroup2d")
        defer {
            encoder.popDebugGroup()
        }
        #endif

        if isMasked {
            if stencilState == nil {
                setupStencilStates()
            }
            encoder.setDepthStencilState(stencilState)
            encoder.setStencilReferenceValue(0b1100_0000)
        }

        shader.setupProgram(context)
        shader.preRender(context)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix)) {
            encoder.setVertexBytes(matrixPointer, length: 64, index: 1)
        }

        encoder.drawIndexedPrimitives(type: .triangle,
                                      indexCount: indicesCount,
                                      indexType: .uint32,
                                      indexBuffer: indicesBuffer,
                                      indexBufferOffset: 0)

    }
}

extension PolygonGroup2d: MCPolygonGroup2dInterface {
    func setVertices(_ vertices: MCSharedBytes, indices: MCSharedBytes) {
        guard vertices.elementCount > 0 else {
            self.indicesCount = 0
            verticesBuffer = nil
            indicesBuffer = nil
            return
        }

        guard let verticesBuffer = device.makeBuffer(from: vertices),
              let indicesBuffer = device.makeBuffer(from: indices)
        else {
            fatalError("Cannot allocate buffers for the UBTileModel")
        }

        self.indicesCount = Int(indices.elementCount)
        self.verticesBuffer = verticesBuffer
        self.indicesBuffer = indicesBuffer
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? { self }
}
