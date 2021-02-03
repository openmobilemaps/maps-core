import Foundation
import MapCoreSharedModule
import Metal

class Polygon2d: BaseGraphicsObject {
    private var shader: MCShaderProgramInterface

    private var verticesBuffer: MTLBuffer?
    private var indicesBuffer: MTLBuffer?
    private var indicesCount: Int = 0

    private var stencilStatePrepare: MTLDepthStencilState?
    private var stencilState: MTLDepthStencilState?

    private let allIsConvex: Bool = true

    init(shader: MCShaderProgramInterface, metalContext: MetalContext) {
        self.shader = shader
        super.init(device: metalContext.device,
                   sampler: metalContext.samplerLibrary.value(.magLinear))
    }

    override func render(encoder: MTLRenderCommandEncoder,
                         context: RenderingContext,
                         renderPass _: MCRenderPassConfig,
                         mvpMatrix: Int64)
    {
        guard let verticesBuffer = verticesBuffer,
              let indicesBuffer = indicesBuffer
        else { return }

        if stencilStatePrepare == nil || stencilState == nil {
            setupStencilStates()
        }

        // stencil prepare pass
        shader.setupProgram(context)
        shader.preRender(context)

        encoder.setDepthStencilState(stencilStatePrepare)
        encoder.setStencilReferenceValue(0x0)

        var c = SIMD4<Float>([0.0, 0.0, 0.0, 0.0])
        encoder.setFragmentBytes(&c, length: MemoryLayout<SIMD4<Float>>.stride, index: 1)

        encoder.setVertexBuffer(verticesBuffer, offset: 0, index: 0)
        let matrixPointer = UnsafeRawPointer(bitPattern: Int(mvpMatrix))!
        encoder.setVertexBytes(matrixPointer, length: 64, index: 1)

        encoder.drawIndexedPrimitives(type: .triangle, indexCount: indicesCount, indexType: .uint16, indexBuffer: indicesBuffer, indexBufferOffset: 0)

        // rendering pass
        shader.preRender(context)

        encoder.setDepthStencilState(stencilState)
        encoder.setStencilReferenceValue(0x1)

        encoder.drawIndexedPrimitives(type: .triangle, indexCount: indicesCount, indexType: .uint16, indexBuffer: indicesBuffer, indexBufferOffset: 0)
    }

    private func setupStencilStates() {
        if allIsConvex {
            // principal idea:
            // - we have two rendering passes, where we render the triangle fan of the polygon
            // - in this case we increment the stencil of every pixel
            // - triangle fan with indices (0,1,2), (0,2,3)... draws in a convex polygon draws
            //   all pixel to be drawn 1 time
            // - so checking if 1, then draw and zero out is the second step (this also zeroes
            //   out everything, holes just increment that they do not get drawn, holes can overlap)
            let ss = MTLStencilDescriptor()
            ss.stencilCompareFunction = .always
            ss.stencilFailureOperation = .incrementClamp
            ss.depthFailureOperation = .keep
            ss.depthStencilPassOperation = .incrementClamp

            let s = MTLDepthStencilDescriptor()
            s.frontFaceStencil = ss
            s.backFaceStencil = ss

            stencilStatePrepare = device.makeDepthStencilState(descriptor: s)

            let ss2 = MTLStencilDescriptor()
            ss2.stencilCompareFunction = .equal
            ss2.stencilFailureOperation = .zero
            ss2.depthFailureOperation = .zero
            ss2.depthStencilPassOperation = .zero

            let s2 = MTLDepthStencilDescriptor()
            s2.frontFaceStencil = ss2
            s2.backFaceStencil = ss2

            stencilState = device.makeDepthStencilState(descriptor: s2)
        } else {
            // principal idea:
            // - we have two rendering passes, where we render the triangle fan of the polygon
            // - in this case we invert the last bit in every pixel
            // - triangle fan with indices (0,1,2), (0,2,3)... draws in a potential concave polygon
            //   the pixels that need to be drawn an odd time
            // - so checking if 1, then draw and zero out is the second step (this also zeroes
            //   out everything)
            // (by the same logic holes also invert, so if the added hole number is also odd, the
            //  number gets non-odd (limitation: holes cannot overlap, most standards ensure that
            //  (e.g. mapbox tiles))
            let ss = MTLStencilDescriptor()
            ss.stencilCompareFunction = .always
            ss.stencilFailureOperation = .invert
            ss.depthFailureOperation = .keep
            ss.depthStencilPassOperation = .invert
            ss.writeMask = 0x1
            ss.readMask = 0x1

            let s = MTLDepthStencilDescriptor()
            s.frontFaceStencil = ss
            s.backFaceStencil = ss

            stencilStatePrepare = device.makeDepthStencilState(descriptor: s)

            let ss2 = MTLStencilDescriptor()
            ss2.stencilCompareFunction = .equal
            ss2.stencilFailureOperation = .zero
            ss2.depthFailureOperation = .zero
            ss2.depthStencilPassOperation = .zero
            ss2.writeMask = 0x1
            ss2.readMask = 0x1

            let s2 = MTLDepthStencilDescriptor()
            s2.frontFaceStencil = ss2
            s2.backFaceStencil = ss2

            stencilState = device.makeDepthStencilState(descriptor: s2)
        }
    }
}

extension Polygon2d: MCPolygon2dInterface {
    func setPolygonPositions(_ positions: [MCVec2D], holes: [[MCVec2D]], isConvex _: Bool) {
        stencilStatePrepare = nil
        stencilState = nil

        guard positions.count > 2 else {
            verticesBuffer = nil
            indicesBuffer = nil
            indicesCount = 0
            return
        }

        var vertices: [Vertex] = []
        var indices: [UInt16] = []

        for c in positions {
            vertices.append(Vertex(x: c.xF, y: c.yF, textureU: 0.0, textureV: 0.0))
        }

        for i in 0 ..< positions.count - 2 {
            indices.append(UInt16(0))
            indices.append(UInt16(i + 1))
            indices.append(UInt16(i + 2))
        }

        var startI = positions.count

        for h in holes {
            for c in h {
                vertices.append(Vertex(x: c.xF, y: c.yF, textureU: 0.0, textureV: 0.0))
            }

            for i in 0 ..< h.count - 2 {
                indices.append(UInt16(startI + 0))
                indices.append(UInt16(startI + i + 1))
                indices.append(UInt16(startI + i + 2))
            }

            startI = startI + h.count
        }

        guard let v = device.makeBuffer(bytes: vertices, length: MemoryLayout<Vertex>.stride * vertices.count, options: []),
              let i = device.makeBuffer(bytes: indices, length: MemoryLayout<UInt16>.stride * indices.count, options: [])
        else {
            fatalError("Cannot allocate buffers for the UBTileModel")
        }

        indicesCount = indices.count
        verticesBuffer = v
        indicesBuffer = i
    }

    func asGraphicsObject() -> MCGraphicsObjectInterface? { self }
}
