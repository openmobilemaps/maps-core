//
//  LineGroupInstanced2d.swift
//  
//
//  Created by Marco Zimmermann on 09.05.23.
//

import Foundation
import MapCoreSharedModule
import Metal

class LineGroupInstanced2d : BaseGraphicsObject {
    private var verticesBuffer: MTLBuffer?
    private var indicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0
    private var positionsBuffer: MTLBuffer?

    private var instanceCount: Int = 0

    private let label: String
    private var shader: MCShaderProgramInterface

    private var stencilState: MTLDepthStencilState?

    init(shader: MCShaderProgramInterface, metalContext: MetalContext, label: String = "LineGroup2dInstanced") {
        self.label = label
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

    override func isReady() -> Bool {
        guard ready else { return false }
        return true
    }

    override func render(encoder: MTLRenderCommandEncoder,
                         context: RenderingContext,
                         renderPass _: MCRenderPassConfig,
                         mvpMatrix: Int64,
                         isMasked: Bool,
                         screenPixelAsRealMeterFactor _: Double) {
        guard let verticesBuffer = verticesBuffer,
              let indicesBuffer = indicesBuffer,
            instanceCount != 0 else {
            return
        }

        lock.lock()
        defer {
            lock.unlock()
        }

        #if DEBUG
        encoder.pushDebugGroup(label)
        defer {
            encoder.popDebugGroup()
        }
        #endif

//        if isMasked {
//            if stencilState == nil {
//                setupStencilStates()
//            }
//            encoder.setDepthStencilState(stencilState)
//            encoder.setStencilReferenceValue(0b1100_0000)
//        } else if let mask = context.mask, renderAsMask {
//            encoder.setDepthStencilState(mask)
//            encoder.setStencilReferenceValue(0b1100_0000)
//        } else {
            encoder.setDepthStencilState(context.defaultMask)
//        }

        shader.setupProgram(context)
        shader.preRender(context)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
        if let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix)) {
            encoder.setVertexBytes(matrixPointer, length: 64, index: 1)
        }

        encoder.setVertexBuffer(positionsBuffer, offset: 0, index: 2)
//        encoder.setVertexBuffer(scalesBuffer, offset: 0, index: 3)
//        encoder.setVertexBuffer(rotationsBuffer, offset: 0, index: 4)
//
//        encoder.setVertexBuffer(textureCoordinatesBuffer, offset: 0, index: 5)
//
//        encoder.setVertexBuffer(alphaBuffer, offset: 0, index: 6)
//
//        encoder.setFragmentSamplerState(sampler, index: 0)
//
//        if let texture = texture {
//            encoder.setFragmentTexture(texture, index: 0)
//        }
//

            encoder.drawIndexedPrimitives(type: .triangle,
                                          indexCount: indicesCount,
                                          indexType: .uint16,
                                          indexBuffer: indicesBuffer,
                                          indexBufferOffset: 0,
                                          instanceCount: instanceCount)
//
    }
}

extension LineGroupInstanced2d : MCLine2dInstancedInterface {

    func setFrame(_ frame: MCQuad2dD) {
        let vertecies: [Vertex] = [
            Vertex(position: frame.topLeft, textureU: 0, textureV: 1), // A
            Vertex(position: frame.topRight, textureU: 0, textureV: 0), // B
            Vertex(position: frame.bottomRight, textureU: 1, textureV: 0), // C
            Vertex(position: frame.bottomLeft, textureU: 1, textureV: 1), // D
        ]
        let indices: [UInt16] = [
            0, 1, 2, // ABC
            0, 2, 3, // ACD
        ]

        guard let verticesBuffer = device.makeBuffer(bytes: vertecies, length: MemoryLayout<Vertex>.stride * vertecies.count, options: []), let indicesBuffer = device.makeBuffer(bytes: indices, length: MemoryLayout<UInt16>.stride * indices.count, options: []) else {
            fatalError("Cannot allocate buffers")
        }

        lock.withCritical {
            indicesCount = indices.count
            self.verticesBuffer = verticesBuffer
            self.indicesBuffer = indicesBuffer
        }
    }

    func setInstanceCount(_ count: Int32) {
        instanceCount = Int(count)
    }

    func setPositions(_ positions: MCSharedBytes) {
        lock.withCritical {
            positionsBuffer = device.makeBuffer(from: positions)
        }
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? {
        self
    }
}
